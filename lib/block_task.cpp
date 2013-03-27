// Copyright (C) by Josh Blum. See LICENSE.txt for licensing information.

#include <gras_impl/block_actor.hpp>
#include "tag_handlers.hpp"

using namespace gras;

void BlockActor::mark_done(void)
{
    if (this->block_state == BLOCK_STATE_DONE) return; //can re-enter checking done first

    this->stats.stop_time = time_now();
    this->block_ptr->notify_inactive();

    //flush partial output buffers to the downstream
    for (size_t i = 0; i < this->get_num_outputs(); i++)
    {
        if (not this->output_queues.ready(i)) continue;
        SBuffer &buff = this->output_queues.front(i);
        if (buff.length == 0) continue;
        InputBufferMessage buff_msg;
        buff_msg.buffer = buff;
        this->post_downstream(i, buff_msg);
        this->output_queues.pop(i);
    }

    this->interruptible_thread.reset();

    //mark down the new state
    this->block_state = BLOCK_STATE_DONE;

    //release upstream, downstream, and executor tokens
    this->token_pool.clear();

    //release all buffers in queues
    this->input_queues.flush_all();
    this->output_queues.flush_all();

    //release all tags and msgs
    for (size_t i = 0; i < this->get_num_inputs(); i++)
    {
        this->input_msgs[i].clear();
        this->input_tags[i].clear();
    }

    //tell the upstream and downstram to re-check their tokens
    //this is how the other blocks know who is interested,
    //and can decide based on interest to set done or not
    for (size_t i = 0; i < this->get_num_inputs(); i++)
    {
        this->post_upstream(i, OutputCheckMessage());
    }
    for (size_t i = 0; i < this->get_num_outputs(); i++)
    {
        this->post_downstream(i, InputCheckMessage());
    }

    if (ARMAGEDDON) std::cerr
        << "==================================================\n"
        << "== The " << block_ptr->to_string() << " is done...\n"
        << "==================================================\n"
        << std::flush;
}

void BlockActor::input_fail(const size_t i)
{
    //input failed, accumulate and try again
    if (not this->input_queues.is_accumulated(i))
    {
        this->input_queues.accumulate(i);
        this->task_kicker();
        return;
    }

    //otherwise check for done, else wait for more
    if (this->inputs_done[i])
    {
        this->mark_done();
        return;
    }

    //check that the input is not already maxed
    if (this->input_queues.is_front_maximal(i))
    {
        throw std::runtime_error("input_fail called on maximum_items buffer");
    }

    //mark fail: not ready until a new buffer appears
    this->input_queues.fail(i);
}

void BlockActor::output_fail(const size_t i)
{
    SBuffer &buff = this->output_queues.front(i);

    //check that the input is not already maxed
    const size_t front_items = buff.length/this->output_configs[i].item_size;
    if (front_items >= this->output_configs[i].maximum_items)
    {
        throw std::runtime_error("output_fail called on maximum_items buffer");
    }

    //mark fail: not ready until a new buffer appears
    this->output_queues.fail(i);
}

void BlockActor::handle_task(void)
{
    TimerAccumulate ta_prep(this->stats.total_time_prep);

    //------------------------------------------------------------------
    //-- Decide if its possible to continue any processing:
    //-- Handle task may get called for incoming buffers,
    //-- however, not all ports may have available buffers.
    //------------------------------------------------------------------
    if GRAS_UNLIKELY(not this->is_work_allowed()) return;

    const size_t num_inputs = this->get_num_inputs();
    const size_t num_outputs = this->get_num_outputs();

    //------------------------------------------------------------------
    //-- initialize input buffers before work
    //------------------------------------------------------------------
    size_t output_inline_index = 0;
    this->input_items.min() = ~0;
    this->input_items.max() = 0;
    for (size_t i = 0; i < num_inputs; i++)
    {
        this->sort_tags(i);
        this->num_input_msgs_read[i] = 0;

        ASSERT(this->input_queues.ready(i));
        const SBuffer &buff = this->input_queues.front(i);
        const void *mem = buff.get();
        size_t items = buff.length/this->input_configs[i].item_size;

        this->input_items[i].get() = mem;
        this->input_items[i].size() = items;
        this->input_items.min() = std::min(this->input_items.min(), items);
        this->input_items.max() = std::max(this->input_items.max(), items);

        //inline dealings, how and when input buffers can be inlined into output buffers
        //*
        if GRAS_UNLIKELY(
            buff.unique() and
            input_configs[i].inline_buffer and
            output_inline_index < num_outputs and
            buff.get_affinity() == this->buffer_affinity
        ){
            //copy buffer reference but push with zero length, same offset
            SBuffer new_obuff = buff;
            new_obuff.length = 0;
            this->flush_output(output_inline_index);
            this->output_queues.push(output_inline_index, new_obuff); //you got inlined!
            output_inline_index++; //done do this output port again
        }
        //*/
    }

    //------------------------------------------------------------------
    //-- initialize output buffers before work
    //------------------------------------------------------------------
    this->output_items.min() = ~0;
    this->output_items.max() = 0;
    for (size_t i = 0; i < num_outputs; i++)
    {
        ASSERT(this->output_queues.ready(i));
        SBuffer &buff = this->output_queues.front(i);
        ASSERT(buff.length == 0); //assumes it was flushed last call
        void *mem = buff.get();
        const size_t bytes = buff.get_actual_length() - buff.offset;
        size_t items = bytes/this->output_configs[i].item_size;

        this->output_items[i].get() = mem;
        this->output_items[i].size() = items;
        this->output_items.min() = std::min(this->output_items.min(), items);
        this->output_items.max() = std::max(this->output_items.max(), items);
    }

    //------------------------------------------------------------------
    //-- the work
    //------------------------------------------------------------------
    ta_prep.done();
    this->stats.work_count++;
    if GRAS_UNLIKELY(this->interruptible_thread)
    {
        TimerAccumulate ta_work(this->stats.total_time_work);
        this->interruptible_thread->call();
    }
    else
    {
        TimerAccumulate ta_work(this->stats.total_time_work);
        this->task_work();
    }
    this->stats.time_last_work = time_now();
    TimerAccumulate ta_post(this->stats.total_time_post);

    //------------------------------------------------------------------
    //-- Post-work output tasks
    //------------------------------------------------------------------
    for (size_t i = 0; i < num_outputs; i++)
    {
        this->flush_output(i);
    }

    //------------------------------------------------------------------
    //-- Post-work input tasks
    //------------------------------------------------------------------
    for (size_t i = 0; i < num_inputs; i++)
    {
        this->trim_msgs(i);

        //update the inputs available bit field
        const bool has_input_bufs = not this->input_queues.empty(i);
        const bool has_input_msgs = not this->input_msgs[i].empty();
        this->inputs_available.set(i, has_input_bufs or has_input_msgs);

        //missing at least one upstream provider?
        //since nothing else is coming in, its safe to mark done
        if GRAS_UNLIKELY(this->is_input_done(i)) this->mark_done();
    }

    //still have IO ready? kick off another task
    this->task_kicker();
}

void BlockActor::consume(const size_t i, const size_t items)
{
    #ifdef ITEM_CONSPROD
    std::cerr << name << " consume " << items << std::endl;
    #endif
    this->stats.items_consumed[i] += items;
    const size_t bytes = items*this->input_configs[i].item_size;
    this->input_queues.consume(i, bytes);
    this->trim_tags(i);
}

void BlockActor::produce(const size_t i, const size_t items)
{
    #ifdef ITEM_CONSPROD
    std::cerr << name << " produce " << items << std::endl;
    #endif
    SBuffer &buff = this->output_queues.front(i);
    ASSERT((buff.length % output_configs[i].item_size) == 0);
    this->stats.items_produced[i] += items;
    const size_t bytes = items*this->output_configs[i].item_size;
    buff.length += bytes;
    this->produce_outputs[i] = true;
}

void BlockActor::produce_buffer(const size_t i, const SBuffer &buffer)
{
    this->flush_output(i);
    ASSERT((buffer.length % output_configs[i].item_size) == 0);
    const size_t items = buffer.length/output_configs[i].item_size;
    this->stats.items_produced[i] += items;
    InputBufferMessage buff_msg;
    buff_msg.buffer = buffer;
    this->post_downstream(i, buff_msg);
}

GRAS_FORCE_INLINE void BlockActor::flush_output(const size_t i)
{
    if GRAS_UNLIKELY(this->output_queues.empty(i) or this->output_queues.front(i).length == 0) return;
    SBuffer &buff = this->output_queues.front(i);
    if GRAS_LIKELY(this->produce_outputs[i])
    {
        this->produce_outputs[i] = false;
        InputBufferMessage buff_msg;
        buff_msg.buffer = buff;
        this->post_downstream(i, buff_msg);
    }

    //increment buffer for next use
    buff.offset += buff.length;
    buff.length = 0;

    //release whatever has been used of the output buffer
    this->output_queues.pop(i);
}
