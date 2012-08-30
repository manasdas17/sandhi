//
// Copyright 2012 Josh Blum
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with io_sig program.  If not, see <http://www.gnu.org/licenses/>.

#include "element_impl.hpp"

using namespace gnuradio;

void ElementImpl::handle_input_msg(const tsbe::TaskInterface &handle, const size_t index, const tsbe::Wax &msg)
{
    if (msg.type() == typeid(tsbe::Buffer))
    {
        if (this->done) return;
        this->input_queues[index].push(msg.cast<tsbe::Buffer>());
        this->inputs_ready.set(index, true);
        this->handle_task(handle);
        return;
    }
    if (msg.type() == typeid(Tag))
    {
        this->input_tags[index].push_back(msg.cast<Tag>());
        this->input_tags_changed[index] = true;
        return;
    }
    if (msg.type() == typeid(Token))
    {
        this->token_pool.insert(msg.cast<Token>());
        return;
    }
    if (msg.type() == typeid(CheckTokensMessage))
    {
        if (this->input_tokens[index].unique())
        {
            this->mark_done(handle);
        }
        return;
    }
}

void ElementImpl::handle_output_msg(const tsbe::TaskInterface &handle, const size_t index, const tsbe::Wax &msg)
{
    if (msg.type() == typeid(tsbe::Buffer))
    {
        if (this->done) return;
        this->output_queues[index].push(msg.cast<tsbe::Buffer>());
        this->outputs_ready.set(index, true);
        this->handle_task(handle);
        return;
    }
    if (msg.type() == typeid(Token))
    {
        this->token_pool.insert(msg.cast<Token>());
        return;
    }
    if (msg.type() == typeid(CheckTokensMessage))
    {
        if (this->output_tokens[index].unique())
        {
            this->mark_done(handle);
        }
        return;
    }
}