//******************************************************************************************************
//  BufferStream.cpp - Gbtc
//
//  Copyright © 2026, Grid Protection Alliance.  All Rights Reserved.
//
//  Licensed to the Grid Protection Alliance (GPA) under one or more contributor license agreements. See
//  the NOTICE file distributed with this work for additional information regarding copyright ownership.
//  The GPA licenses this file to you under the MIT License (MIT), the "License"; you may not use this
//  file except in compliance with the License. You may obtain a copy of the License at:
//
//      http://opensource.org/licenses/MIT
//
//  Unless agreed to in writing, the subject software distributed under the License is distributed on an
//  "AS-IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. Refer to the
//  License for the specific language governing permissions and limitations.
//
//  Code Modification History:
//  ----------------------------------------------------------------------------------------------------
//  10/07/2026 - J. Ritchie Carroll
//       Generated original version of source code.
//
//******************************************************************************************************

#include "BufferStream.h"
#include <cstring>

using namespace std;
using namespace snapdb;
using namespace snapdb::io;

BufferStream::BufferStream() :
    m_position(0U)
{
}

BufferStream::BufferStream(vector<uint8_t> buffer) :
    m_buffer(move(buffer)),
    m_position(0U)
{
}

uint32_t BufferStream::Read(uint8_t* buffer, const uint32_t offset, const uint32_t count)
{
    const uint32_t length = std::min(count, Remaining());

    if (length > 0)
    {
        memcpy(buffer + offset, m_buffer.data() + m_position, length);
        m_position += length;
    }

    return length;
}

void BufferStream::Write(const uint8_t* buffer, const uint32_t offset, const uint32_t count)
{
    m_buffer.insert(m_buffer.end(), buffer + offset, buffer + offset + count);
}

const vector<uint8_t>& BufferStream::Buffer() const
{
    return m_buffer;
}

uint32_t BufferStream::Position() const
{
    return m_position;
}

void BufferStream::SetPosition(const uint32_t position)
{
    if (position > m_buffer.size())
        throw out_of_range("Position exceeds buffer length");

    m_position = position;
}

uint32_t BufferStream::Remaining() const
{
    return ConvertUInt32(m_buffer.size()) - m_position;
}

void BufferStream::Clear()
{
    m_buffer.clear();
    m_position = 0U;
}
