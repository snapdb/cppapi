//******************************************************************************************************
//  BinaryStream.cpp - Gbtc
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

#include "BinaryStream.h"
#include "../Convert.h"
#include "../SnapException.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::io;

BinaryStream::BinaryStream(StreamPtr stream, const bool littleEndian) :
    m_stream(std::move(stream)),
    m_littleEndian(littleEndian),
    m_receiveBuffer(IOBufferSize),
    m_receivePosition(0U),
    m_receiveLength(0U),
    m_sendBuffer(IOBufferSize),
    m_sendLength(0U)
{
    if (m_stream == nullptr)
        throw invalid_argument("Base stream cannot be null");
}

const StreamPtr& BinaryStream::BaseStream() const
{
    return m_stream;
}

bool BinaryStream::IsLittleEndian() const
{
    return m_littleEndian;
}

uint32_t BinaryStream::ReceiveBufferAvailable() const
{
    return m_receiveLength - m_receivePosition;
}

uint32_t BinaryStream::SendBufferLength() const
{
    return m_sendLength;
}

uint8_t BinaryStream::ReadNextByte()
{
    uint8_t value;
    ReadAll(&value, 0, 1);
    return value;
}

uint32_t BinaryStream::Read(uint8_t* buffer, const uint32_t offset, const uint32_t count)
{
    if (count == 0)
        return 0U;

    uint32_t available = ReceiveBufferAvailable();

    // Serve request from receive buffer when data is available
    if (available > 0)
    {
        const uint32_t length = std::min(available, count);
        memcpy(buffer + offset, m_receiveBuffer.data() + m_receivePosition, length);
        m_receivePosition += length;
        return length;
    }

    // Large requests bypass receive buffer and read directly into destination
    if (count >= IOBufferSize / 2)
        return m_stream->Read(buffer, offset, count);

    // Refill receive buffer
    m_receivePosition = 0U;
    m_receiveLength = m_stream->Read(m_receiveBuffer.data(), 0, IOBufferSize);
    available = m_receiveLength;

    if (available == 0)
        return 0U;

    const uint32_t length = std::min(available, count);
    memcpy(buffer + offset, m_receiveBuffer.data(), length);
    m_receivePosition = length;

    return length;
}

void BinaryStream::ReadAll(uint8_t* buffer, uint32_t offset, uint32_t count)
{
    while (count > 0)
    {
        const uint32_t bytesRead = Read(buffer, offset, count);

        if (bytesRead == 0)
            throw SnapException("End of stream encountered while reading");

        offset += bytesRead;
        count -= bytesRead;
    }
}

vector<uint8_t> BinaryStream::ReadBytes(const uint32_t count)
{
    vector<uint8_t> buffer(count);

    if (count > 0)
        ReadAll(buffer.data(), 0, count);

    return buffer;
}

vector<uint8_t> BinaryStream::ReadBuffer()
{
    return ReadBytes(Read7BitUInt32());
}

string BinaryStream::ReadString()
{
    const vector<uint8_t> buffer = ReadBuffer();
    return { buffer.begin(), buffer.end() };
}

Guid BinaryStream::ReadGuid()
{
    uint8_t buffer[16];
    ReadAll(buffer, 0, 16);
    return ParseGuid(buffer, true);
}

void BinaryStream::Write(const uint8_t* buffer, const uint32_t offset, const uint32_t count)
{
    if (count == 0)
        return;

    if (IOBufferSize - m_sendLength < count)
        Flush();

    // Large writes bypass send buffer and write directly to base stream
    if (count >= IOBufferSize)
    {
        m_stream->Write(buffer, offset, count);
        return;
    }

    memcpy(m_sendBuffer.data() + m_sendLength, buffer + offset, count);
    m_sendLength += count;
}

void BinaryStream::Write(const vector<uint8_t>& buffer)
{
    Write(buffer.data(), 0, ConvertUInt32(buffer.size()));
}

void BinaryStream::WriteBuffer(const vector<uint8_t>& value)
{
    Write7BitUInt32(ConvertUInt32(value.size()));
    Write(value);
}

void BinaryStream::WriteString(const string& value)
{
    const uint32_t length = ConvertUInt32(value.size());
    Write7BitUInt32(length);
    Write(reinterpret_cast<const uint8_t*>(value.data()), 0, length);
}

void BinaryStream::WriteGuid(const Guid& value)
{
    // Guid values are serialized using .NET (Microsoft) byte order
    Guid swapped = value;
    SwapGuidEndianness(swapped);
    Write(swapped.data, 0, 16);
}

void BinaryStream::Flush()
{
    if (m_sendLength > 0)
    {
        const uint32_t length = m_sendLength;
        m_sendLength = 0U;
        m_stream->Write(m_sendBuffer.data(), 0, length);
    }

    m_stream->Flush();
}

void BinaryStream::Close()
{
    m_sendLength = 0U;
    m_receivePosition = 0U;
    m_receiveLength = 0U;
    m_stream->Close();
}

bool BinaryStream::IsOpen() const
{
    return m_stream->IsOpen();
}
