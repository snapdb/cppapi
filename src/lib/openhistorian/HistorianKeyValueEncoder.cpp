//******************************************************************************************************
//  HistorianKeyValueEncoder.cpp - Gbtc
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

#include "HistorianKeyValueEncoder.h"
#include "../Convert.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::snap;
using namespace snapdb::openhistorian;

// {0418B3A7-F631-47AF-BBFA-8B9BC0378328}
const Guid HistorianKeyValueEncoder::SnapTypeID = ParseGuid("0418b3a7-f631-47af-bbfa-8b9bc0378328");

const EncodingDefinition HistorianKeyValueEncoder::Encoding = EncodingDefinition::CreateKeyValueEncoding(SnapTypeID);

const EncodingDefinition& HistorianKeyValueEncoder::Definition() const
{
    return Encoding;
}

bool HistorianKeyValueEncoder::UsesPreviousKey() const
{
    return true;
}

bool HistorianKeyValueEncoder::UsesPreviousValue() const
{
    return false;
}

uint32_t HistorianKeyValueEncoder::MaxCompressionSize() const
{
    return 55U; // 3 extra bytes just to be safe
}

bool HistorianKeyValueEncoder::ContainsEndOfStreamSymbol() const
{
    return true;
}

uint8_t HistorianKeyValueEncoder::EndOfStreamSymbol() const
{
    return EndOfStream;
}

// Encoding is a code byte followed by optional fields. When the high bit of the code byte is clear, the
// point shares previous timestamp, has no entry number, only a 32-bit Value1 and no Value2 or Value3,
// i.e., a very common case for streaming float measurements, and lower 6-bits define the point ID delta
// (XOR) from the previous point ID; bit 6 (64) determines if a 32-bit Value1 follows. Otherwise, the bits
// of the code byte define which fields follow:
//     Bit 7 (128): Always set for full encoding
//     Bit 6 (64): Timestamp changed, 7-bit encoded XOR delta follows
//     Bit 5 (32): Entry number is non-zero, 7-bit encoded value follows
//     Bit 4 (16): Value1 is 64-bit
//     Bit 3 (8): Value1 is 32-bit, non-zero
//     Bit 2 (4): Value2 is non-zero, 64-bit
//     Bit 1 (2): Value3 is 64-bit
//     Bit 0 (1): Value3 is 32-bit, non-zero
// Point ID is always 7-bit encoded XOR delta in full encoding. Code byte 0xFF marks end of stream.
void HistorianKeyValueEncoder::Encode(BinaryStream& stream, const HistorianKey& prevKey, const HistorianValue&, const HistorianKey& key, const HistorianValue& value)
{
    const uint64_t pointIDDelta = key.PointID ^ prevKey.PointID;

    if (key.Timestamp == prevKey.Timestamp && pointIDDelta < 64ULL && key.EntryNumber == 0ULL &&
        value.Value1 <= UInt32::MaxValue && value.Value2 == 0ULL && value.Value3 == 0ULL)
    {
        if (value.Value1 == 0ULL)
        {
            stream.WriteByte(static_cast<uint8_t>(pointIDDelta));
        }
        else
        {
            stream.WriteByte(static_cast<uint8_t>(pointIDDelta | 64ULL));
            stream.WriteUInt32(static_cast<uint32_t>(value.Value1));
        }

        return;
    }

    uint8_t code = 128;

    if (key.Timestamp != prevKey.Timestamp)
        code |= 64;

    if (key.EntryNumber != 0ULL)
        code |= 32;

    if (value.Value1 > UInt32::MaxValue)
        code |= 16;
    else if (value.Value1 > 0ULL)
        code |= 8;

    if (value.Value2 != 0ULL)
        code |= 4;

    if (value.Value3 > UInt32::MaxValue)
        code |= 2;
    else if (value.Value3 > 0ULL)
        code |= 1;

    stream.WriteByte(code);

    if (key.Timestamp != prevKey.Timestamp)
        stream.Write7BitUInt64(key.Timestamp ^ prevKey.Timestamp);

    stream.Write7BitUInt64(pointIDDelta);

    if (key.EntryNumber != 0ULL)
        stream.Write7BitUInt64(key.EntryNumber);

    if (value.Value1 > UInt32::MaxValue)
        stream.WriteUInt64(value.Value1);
    else if (value.Value1 > 0ULL)
        stream.WriteUInt32(static_cast<uint32_t>(value.Value1));

    if (value.Value2 != 0ULL)
        stream.WriteUInt64(value.Value2);

    if (value.Value3 > UInt32::MaxValue)
        stream.WriteUInt64(value.Value3);
    else if (value.Value3 > 0ULL)
        stream.WriteUInt32(static_cast<uint32_t>(value.Value3));
}

bool HistorianKeyValueEncoder::Decode(BinaryStream& stream, const HistorianKey& prevKey, const HistorianValue&, HistorianKey& key, HistorianValue& value)
{
    const uint8_t code = stream.ReadByte();

    if (code == EndOfStream)
        return true;

    if (code < 128)
    {
        key.Timestamp = prevKey.Timestamp;
        key.EntryNumber = 0ULL;
        value.Value2 = 0ULL;
        value.Value3 = 0ULL;

        if (code < 64)
        {
            key.PointID = prevKey.PointID ^ code;
            value.Value1 = 0ULL;
        }
        else
        {
            key.PointID = prevKey.PointID ^ code ^ 64ULL;
            value.Value1 = stream.ReadUInt32();
        }

        return false;
    }

    // Timestamp changed
    if ((code & 64) != 0)
        key.Timestamp = prevKey.Timestamp ^ stream.Read7BitUInt64();
    else
        key.Timestamp = prevKey.Timestamp;

    key.PointID = prevKey.PointID ^ stream.Read7BitUInt64();

    // Entry number is non-zero
    if ((code & 32) != 0)
        key.EntryNumber = stream.Read7BitUInt64();
    else
        key.EntryNumber = 0ULL;

    // Value1 is 64-bit or 32-bit
    if ((code & 16) != 0)
        value.Value1 = stream.ReadUInt64();
    else if ((code & 8) != 0)
        value.Value1 = stream.ReadUInt32();
    else
        value.Value1 = 0ULL;

    // Value2 is non-zero
    if ((code & 4) != 0)
        value.Value2 = stream.ReadUInt64();
    else
        value.Value2 = 0ULL;

    // Value3 is 64-bit or 32-bit
    if ((code & 2) != 0)
        value.Value3 = stream.ReadUInt64();
    else if ((code & 1) != 0)
        value.Value3 = stream.ReadUInt32();
    else
        value.Value3 = 0ULL;

    return false;
}
