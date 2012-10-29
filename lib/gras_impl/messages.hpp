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
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef INCLUDED_LIBGRAS_IMPL_MESSAGES_HPP
#define INCLUDED_LIBGRAS_IMPL_MESSAGES_HPP

#include <gras/sbuffer.hpp>
#include <gras/tags.hpp>
#include <gras/sbuffer.hpp>
#include <gras_impl/token.hpp>

namespace gras
{

//----------------------------------------------------------------------
//-- message from the top block/executor
//-- these messages must be ack'd
//----------------------------------------------------------------------

struct TopAllocMessage
{
    //empty
};

struct TopActiveMessage
{
    //empty
};

struct TopInertMessage
{
    //empty
};

struct TopTokenMessage
{
    Token token;
};

//----------------------------------------------------------------------
//-- message to an input port
//-- do not ack
//----------------------------------------------------------------------

struct InputTagMessage
{
    InputTagMessage(const Tag &tag):tag(tag){}
    size_t index;
    Tag tag;
};

struct InputBufferMessage
{
    size_t index;
    SBuffer buffer;
};

struct InputTokenMessage
{
    size_t index;
    Token token;
};

struct InputAllocMessage
{
    size_t index;
    SBufferToken token;
    size_t recommend_length;
};

struct InputCheckMessage
{
    size_t index;
};

//----------------------------------------------------------------------
//-- message to an output port
//-- do not ack
//----------------------------------------------------------------------

struct OutputBufferMessage
{
    size_t index;
    SBuffer buffer;
};

struct OutputTokenMessage
{
    size_t index;
    Token token;
};

struct OutputCheckMessage
{
    size_t index;
};

struct OutputHintMessage
{
    size_t index;
    size_t history_bytes;
    size_t reserve_bytes;
    WeakToken token;
};

struct OutputAllocMessage
{
    size_t index;
    SBufferToken token;
};

//----------------------------------------------------------------------
//-- message to just the block
//-- do not ack
//----------------------------------------------------------------------

struct SelfKickMessage
{
    //empty
};

struct UpdateInputsMessage
{
    //empty
};

} //namespace gras

#include <Theron/Register.h>
#include <gras/top_block.hpp>
#include <gras_impl/messages.hpp>
#include <gras_impl/interruptible_thread.hpp>

THERON_DECLARE_REGISTERED_MESSAGE(gras::TopAllocMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::TopActiveMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::TopInertMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::TopTokenMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::GlobalBlockConfig);
THERON_DECLARE_REGISTERED_MESSAGE(gras::SharedThreadGroup);

THERON_DECLARE_REGISTERED_MESSAGE(gras::InputTagMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::InputBufferMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::InputTokenMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::InputCheckMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::InputAllocMessage);

THERON_DECLARE_REGISTERED_MESSAGE(gras::OutputBufferMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::OutputTokenMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::OutputCheckMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::OutputHintMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::OutputAllocMessage);

THERON_DECLARE_REGISTERED_MESSAGE(gras::SelfKickMessage);
THERON_DECLARE_REGISTERED_MESSAGE(gras::UpdateInputsMessage);

#endif /*INCLUDED_LIBGRAS_IMPL_MESSAGES_HPP*/
