//******************************************************************************************************
//  HistorianValue.cpp - Gbtc
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

#include "HistorianValue.h"
#include "../Convert.h"
#include <cstring>
#include <iomanip>

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::openhistorian;

// {24DDE7DC-67F9-42B6-A11B-E27C3E62D9EF}
const Guid HistorianValue::TypeID = ParseGuid("24dde7dc-67f9-42b6-a11b-e27c3e62d9ef");

HistorianValue::HistorianValue(const float32_t value, const QualityFlags quality)
{
    SetSingle(value);
    SetQuality(quality);
}

void HistorianValue::SetMin()
{
    Value1 = 0ULL;
    Value2 = 0ULL;
    Value3 = 0ULL;
}

void HistorianValue::SetMax()
{
    Value1 = UInt64::MaxValue;
    Value2 = UInt64::MaxValue;
    Value3 = UInt64::MaxValue;
}

void HistorianValue::Clear()
{
    SetMin();
}

void HistorianValue::Read(BinaryStream& stream)
{
    Value1 = stream.ReadUInt64();
    Value2 = stream.ReadUInt64();
    Value3 = stream.ReadUInt64();
}

void HistorianValue::Write(BinaryStream& stream) const
{
    stream.WriteUInt64(Value1);
    stream.WriteUInt64(Value2);
    stream.WriteUInt64(Value3);
}

void HistorianValue::CopyTo(HistorianValue& destination) const
{
    destination.Value1 = Value1;
    destination.Value2 = Value2;
    destination.Value3 = Value3;
}

int32_t HistorianValue::CompareTo(const HistorianValue& other) const
{
    if (Value1 < other.Value1)
        return -1;

    if (Value1 > other.Value1)
        return 1;

    if (Value2 < other.Value2)
        return -1;

    if (Value2 > other.Value2)
        return 1;

    if (Value3 < other.Value3)
        return -1;

    if (Value3 > other.Value3)
        return 1;

    return 0;
}

float32_t HistorianValue::AsSingle() const
{
    const uint32_t bits = static_cast<uint32_t>(Value1);
    float32_t value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

void HistorianValue::SetSingle(const float32_t value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    Value1 = bits;
}

float64_t HistorianValue::AsDouble() const
{
    float64_t value;
    memcpy(&value, &Value1, sizeof(value));
    return value;
}

void HistorianValue::SetDouble(const float64_t value)
{
    memcpy(&Value1, &value, sizeof(Value1));
}

QualityFlags HistorianValue::AsQuality() const
{
    return static_cast<QualityFlags>(static_cast<uint32_t>(Value3));
}

void HistorianValue::SetQuality(const QualityFlags value)
{
    Value3 = static_cast<uint32_t>(value);
}

void HistorianValue::GetAlarm(bool& alarmed, Guid& eventID, QualityFlags& flags) const
{
    // Event ID is stored as .NET encoded Guid bytes, little-endian, across Value1 and Value2
    uint8_t bytes[16];

    for (int32_t i = 0; i < 8; i++)
    {
        bytes[i] = static_cast<uint8_t>(Value1 >> (8 * i));
        bytes[i + 8] = static_cast<uint8_t>(Value2 >> (8 * i));
    }

    alarmed = (Value3 >> 32) > 0ULL;
    eventID = ParseGuid(bytes, true);
    flags = AsQuality();
}

void HistorianValue::SetAlarm(const bool alarmed, const Guid& eventID, const QualityFlags flags)
{
    Guid swapped = eventID;
    SwapGuidEndianness(swapped);

    Value1 = 0ULL;
    Value2 = 0ULL;

    for (int32_t i = 0; i < 8; i++)
    {
        Value1 |= static_cast<uint64_t>(swapped.data[i]) << (8 * i);
        Value2 |= static_cast<uint64_t>(swapped.data[i + 8]) << (8 * i);
    }

    Value3 = (alarmed ? 1ULL : 0ULL) << 32 | static_cast<uint32_t>(flags);
}

string HistorianValue::AsString() const
{
    char data[17] {};

    for (int32_t i = 0; i < 8; i++)
    {
        data[i] = static_cast<char>(Value1 >> (8 * i));
        data[i + 8] = static_cast<char>(Value2 >> (8 * i));
    }

    return { data, strnlen(data, 16) };
}

void HistorianValue::SetString(const string& value)
{
    if (value.size() > 16)
        throw overflow_error("String cannot be larger than 16 characters");

    uint8_t data[16] {};
    memcpy(data, value.data(), value.size());

    Value1 = 0ULL;
    Value2 = 0ULL;

    for (int32_t i = 0; i < 8; i++)
    {
        Value1 |= static_cast<uint64_t>(data[i]) << (8 * i);
        Value2 |= static_cast<uint64_t>(data[i + 8]) << (8 * i);
    }
}

string HistorianValue::ToString() const
{
    stringstream stream;
    stream << fixed << setprecision(3) << AsSingle() << " [" << openhistorian::ToString(AsQuality()) << "]";
    return stream.str();
}

bool HistorianValue::operator==(const HistorianValue& other) const
{
    return CompareTo(other) == 0;
}
