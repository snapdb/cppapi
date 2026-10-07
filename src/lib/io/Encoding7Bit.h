//******************************************************************************************************
//  Encoding7Bit.h - Gbtc
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

#include "../CommonTypes.h"

namespace snapdb::io
{
    // Defines 7-bit encoding/decoding functions compatible with GSF and SNAPdb.
    // Values are written seven bits at a time, least-significant bits first, with the
    // high bit of each byte set when more bytes follow. For 32-bit values the fifth byte,
    // and for 64-bit values the ninth byte, holds all remaining bits.
    struct Encoding7Bit
    {
        // Maximum number of bytes needed to 7-bit encode a 32-bit value.
        static constexpr uint32_t MaxUInt32Size = 5U;

        // Maximum number of bytes needed to 7-bit encode a 64-bit value.
        static constexpr uint32_t MaxUInt64Size = 9U;

        // Gets the number of bytes needed to 7-bit encode the specified value.
        static uint32_t GetSize(uint64_t value)
        {
            uint32_t size = 1U;

            while (value >= 128ULL && size < MaxUInt64Size)
            {
                value >>= 7;
                size++;
            }

            return size;
        }

        // Writes 32-bit unsigned integer value using 7-bit encoding to the provided buffer, which
        // must have at least MaxUInt32Size bytes available. Returns number of bytes written.
        static uint32_t WriteUInt32(uint8_t* buffer, uint32_t value)
        {
            for (uint32_t i = 0; i < MaxUInt32Size - 1; i++)
            {
                if (value < 128U)
                {
                    buffer[i] = static_cast<uint8_t>(value);
                    return i + 1;
                }

                buffer[i] = static_cast<uint8_t>(value | 128U);
                value >>= 7;
            }

            buffer[MaxUInt32Size - 1] = static_cast<uint8_t>(value);
            return MaxUInt32Size;
        }

        // Writes 64-bit unsigned integer value using 7-bit encoding to the provided buffer, which
        // must have at least MaxUInt64Size bytes available. Returns number of bytes written.
        static uint32_t WriteUInt64(uint8_t* buffer, uint64_t value)
        {
            for (uint32_t i = 0; i < MaxUInt64Size - 1; i++)
            {
                if (value < 128ULL)
                {
                    buffer[i] = static_cast<uint8_t>(value);
                    return i + 1;
                }

                buffer[i] = static_cast<uint8_t>(value | 128ULL);
                value >>= 7;
            }

            buffer[MaxUInt64Size - 1] = static_cast<uint8_t>(value);
            return MaxUInt64Size;
        }

        // Reads 32-bit unsigned integer value using 7-bit encoding from the provided function
        // that reads next byte from a stream, i.e., signature: uint8_t readByte().
        template<class TReadByte>
        static uint32_t ReadUInt32(TReadByte readByte)
        {
            uint32_t value = 0U;

            for (uint32_t i = 0; i < MaxUInt32Size - 1; i++)
            {
                const uint32_t next = readByte();
                value |= (next & 127U) << (7 * i);

                if (next < 128U)
                    return value;
            }

            return value | static_cast<uint32_t>(readByte()) << 28;
        }

        // Reads 64-bit unsigned integer value using 7-bit encoding from the provided function
        // that reads next byte from a stream, i.e., signature: uint8_t readByte().
        template<class TReadByte>
        static uint64_t ReadUInt64(TReadByte readByte)
        {
            uint64_t value = 0ULL;

            for (uint32_t i = 0; i < MaxUInt64Size - 1; i++)
            {
                const uint64_t next = readByte();
                value |= (next & 127ULL) << (7 * i);

                if (next < 128ULL)
                    return value;
            }

            return value | static_cast<uint64_t>(readByte()) << 56;
        }
    };
}
