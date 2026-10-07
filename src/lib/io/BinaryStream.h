//******************************************************************************************************
//  BinaryStream.h - Gbtc
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

#pragma once

#include "Stream.h"
#include "Encoding7Bit.h"
#include "../EndianConverter.h"
#include <cstring>

namespace snapdb::io
{
    // Establishes buffered binary I/O around a base stream, e.g., a socket. Values are
    // little-endian encoded by default, matching the SNAPdb wire protocol.
    class BinaryStream final
    {
    public:
        // Defines the size of the buffers used for socket I/O.
        static constexpr uint32_t IOBufferSize = 65536U;

    private:
        StreamPtr m_stream;
        bool m_littleEndian;
        std::vector<uint8_t> m_receiveBuffer;
        uint32_t m_receivePosition;
        uint32_t m_receiveLength;
        std::vector<uint8_t> m_sendBuffer;
        uint32_t m_sendLength;

        uint8_t ReadNextByte();

        template<class T>
        T ConvertByteOrder(T value) const
        {
            return m_littleEndian ?
                EndianConverter::Default.ConvertLittleEndian(value) :
                EndianConverter::Default.ConvertBigEndian(value);
        }

        template<class T>
        T ReadValue()
        {
            T value;

            if (m_receiveLength - m_receivePosition >= sizeof(T))
            {
                memcpy(&value, m_receiveBuffer.data() + m_receivePosition, sizeof(T));
                m_receivePosition += sizeof(T);
            }
            else
            {
                ReadAll(reinterpret_cast<uint8_t*>(&value), 0, sizeof(T));
            }

            return ConvertByteOrder(value);
        }

        template<class T>
        void WriteValue(T value)
        {
            value = ConvertByteOrder(value);

            if (IOBufferSize - m_sendLength >= sizeof(T))
            {
                memcpy(m_sendBuffer.data() + m_sendLength, &value, sizeof(T));
                m_sendLength += sizeof(T);
            }
            else
            {
                Write(reinterpret_cast<const uint8_t*>(&value), 0, sizeof(T));
            }
        }

    public:
        // Creates a new binary stream around the provided base stream.
        BinaryStream(StreamPtr stream, bool littleEndian = true);

        // Binary streams maintain buffer state and cannot be copied
        BinaryStream(const BinaryStream&) = delete;
        BinaryStream& operator=(const BinaryStream&) = delete;

        // Gets the base stream.
        const StreamPtr& BaseStream() const;

        // Gets flag that determines if values are encoded in little-endian byte order.
        bool IsLittleEndian() const;

        // Gets the number of bytes available in the receive buffer.
        uint32_t ReceiveBufferAvailable() const;

        // Gets the number of bytes pending in the send buffer.
        uint32_t SendBufferLength() const;

        // Reads up to count bytes into buffer at offset. Returns number of bytes read,
        // which may be less than count; returns zero at end of stream.
        uint32_t Read(uint8_t* buffer, uint32_t offset, uint32_t count);

        // Reads exactly count bytes into buffer at offset. Throws on end of stream.
        void ReadAll(uint8_t* buffer, uint32_t offset, uint32_t count);

        // Reads exactly count bytes.
        std::vector<uint8_t> ReadBytes(uint32_t count);

        // Reads a 7-bit encoded length prefixed byte buffer.
        std::vector<uint8_t> ReadBuffer();

        // Reads a 7-bit encoded length prefixed UTF-8 string.
        std::string ReadString();

        // Reads a .NET encoded Guid.
        Guid ReadGuid();

        uint8_t ReadByte()
        {
            if (m_receivePosition < m_receiveLength)
                return m_receiveBuffer[m_receivePosition++];

            return ReadNextByte();
        }

        bool ReadBoolean() { return ReadByte() != 0; }
        int16_t ReadInt16() { return ReadValue<int16_t>(); }
        uint16_t ReadUInt16() { return ReadValue<uint16_t>(); }
        int32_t ReadInt32() { return ReadValue<int32_t>(); }
        uint32_t ReadUInt32() { return ReadValue<uint32_t>(); }
        int64_t ReadInt64() { return ReadValue<int64_t>(); }
        uint64_t ReadUInt64() { return ReadValue<uint64_t>(); }
        uint32_t Read7BitUInt32() { return Encoding7Bit::ReadUInt32([this] { return ReadByte(); }); }
        uint64_t Read7BitUInt64() { return Encoding7Bit::ReadUInt64([this] { return ReadByte(); }); }

        // Writes count bytes from buffer at offset.
        void Write(const uint8_t* buffer, uint32_t offset, uint32_t count);

        // Writes all bytes from buffer.
        void Write(const std::vector<uint8_t>& buffer);

        // Writes a 7-bit encoded length prefixed byte buffer.
        void WriteBuffer(const std::vector<uint8_t>& value);

        // Writes a 7-bit encoded length prefixed UTF-8 string.
        void WriteString(const std::string& value);

        // Writes a .NET encoded Guid.
        void WriteGuid(const Guid& value);

        void WriteByte(const uint8_t value)
        {
            if (m_sendLength < IOBufferSize)
                m_sendBuffer[m_sendLength++] = value;
            else
                Write(&value, 0, 1);
        }

        void WriteBoolean(const bool value) { WriteByte(value ? 1 : 0); }
        void WriteInt16(const int16_t value) { WriteValue(value); }
        void WriteUInt16(const uint16_t value) { WriteValue(value); }
        void WriteInt32(const int32_t value) { WriteValue(value); }
        void WriteUInt32(const uint32_t value) { WriteValue(value); }
        void WriteInt64(const int64_t value) { WriteValue(value); }
        void WriteUInt64(const uint64_t value) { WriteValue(value); }

        void Write7BitUInt32(const uint32_t value)
        {
            if (IOBufferSize - m_sendLength >= Encoding7Bit::MaxUInt32Size)
            {
                m_sendLength += Encoding7Bit::WriteUInt32(m_sendBuffer.data() + m_sendLength, value);
            }
            else
            {
                uint8_t buffer[Encoding7Bit::MaxUInt32Size];
                Write(buffer, 0, Encoding7Bit::WriteUInt32(buffer, value));
            }
        }

        void Write7BitUInt64(const uint64_t value)
        {
            if (IOBufferSize - m_sendLength >= Encoding7Bit::MaxUInt64Size)
            {
                m_sendLength += Encoding7Bit::WriteUInt64(m_sendBuffer.data() + m_sendLength, value);
            }
            else
            {
                uint8_t buffer[Encoding7Bit::MaxUInt64Size];
                Write(buffer, 0, Encoding7Bit::WriteUInt64(buffer, value));
            }
        }

        // Writes any pending data in send buffer to the base stream.
        void Flush();

        // Closes the base stream.
        void Close();

        // Determines if the base stream is open.
        bool IsOpen() const;
    };

    typedef SharedPtr<BinaryStream> BinaryStreamPtr;
}
