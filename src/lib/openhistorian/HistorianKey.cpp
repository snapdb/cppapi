//******************************************************************************************************
//  HistorianKey.cpp - Gbtc
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

#include "HistorianKey.h"
#include "MetadataCache.h"
#include "../Convert.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::openhistorian;

// {6527D41B-9D04-4BFA-8133-05273D521D46}
const Guid HistorianKey::TypeID = ParseGuid("6527d41b-9d04-4bfa-8133-05273d521d46");

HistorianKey::HistorianKey(const uint64_t pointID, const uint64_t timestamp, const uint64_t entryNumber) :
    Timestamp(timestamp),
    PointID(pointID),
    EntryNumber(entryNumber)
{
}

HistorianKey::HistorianKey(const uint64_t pointID, const datetime_t& timestamp, const uint64_t entryNumber) :
    PointID(pointID),
    EntryNumber(entryNumber)
{
    SetDateTime(timestamp);
}

void HistorianKey::SetMin()
{
    Timestamp = 0ULL;
    PointID = 0ULL;
    EntryNumber = 0ULL;
}

void HistorianKey::SetMax()
{
    Timestamp = UInt64::MaxValue;
    PointID = UInt64::MaxValue;
    EntryNumber = UInt64::MaxValue;
}

void HistorianKey::Clear()
{
    SetMin();
}

void HistorianKey::Read(BinaryStream& stream)
{
    Timestamp = stream.ReadUInt64();
    PointID = stream.ReadUInt64();
    EntryNumber = stream.ReadUInt64();
}

void HistorianKey::Write(BinaryStream& stream) const
{
    stream.WriteUInt64(Timestamp);
    stream.WriteUInt64(PointID);
    stream.WriteUInt64(EntryNumber);
}

void HistorianKey::CopyTo(HistorianKey& destination) const
{
    destination.Timestamp = Timestamp;
    destination.PointID = PointID;
    destination.EntryNumber = EntryNumber;
}

int32_t HistorianKey::CompareTo(const HistorianKey& other) const
{
    if (Timestamp < other.Timestamp)
        return -1;

    if (Timestamp > other.Timestamp)
        return 1;

    if (PointID < other.PointID)
        return -1;

    if (PointID > other.PointID)
        return 1;

    if (EntryNumber < other.EntryNumber)
        return -1;

    if (EntryNumber > other.EntryNumber)
        return 1;

    return 0;
}

uint64_t HistorianKey::GetMillisecondTimestamp() const
{
    constexpr uint64_t ticksPerMillisecond = Ticks::PerMillisecond;
    return Timestamp / ticksPerMillisecond * ticksPerMillisecond;
}

void HistorianKey::SetMillisecondTimestamp(const uint64_t value)
{
    constexpr uint64_t ticksPerMillisecond = Ticks::PerMillisecond;
    Timestamp = value / ticksPerMillisecond * ticksPerMillisecond;
}

bool HistorianKey::TryGetDateTime(datetime_t& value) const
{
    if (Timestamp > static_cast<uint64_t>(Ticks::MaxValue))
    {
        value = DateTime::MinValue;
        return false;
    }

    value = FromTicks(static_cast<int64_t>(Timestamp));
    return true;
}

datetime_t HistorianKey::GetDateTime() const
{
    datetime_t value;
    TryGetDateTime(value);
    return value;
}

void HistorianKey::SetDateTime(const datetime_t& value)
{
    const int64_t ticks = ToTicks(value);

    if (ticks < 0)
        throw out_of_range("Timestamp is before 1/1/0001");

    Timestamp = static_cast<uint64_t>(ticks);
}

string HistorianKey::ToString() const
{
    char timestamp[64];

    if (Timestamp > static_cast<uint64_t>(Ticks::MaxValue) || TicksToString(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S.%f", static_cast<int64_t>(Timestamp)) == 0)
        return to_string(PointID) + " @ " + to_string(Timestamp) + " ticks";

    return to_string(PointID) + " @ " + timestamp;
}

string HistorianKey::ToString(const MetadataCache& metadata, const MeasurementNameField nameField) const
{
    const MeasurementRecordPtr measurement = metadata.LookupMeasurementByPointID(PointID);

    if (measurement == nullptr)
        return ToString();

    string name;

    switch (nameField)
    {
        case MeasurementNameField::PointTag:
            name = measurement->PointTag;
            break;
        case MeasurementNameField::Description:
            name = measurement->Description;
            break;
        default:
            name = measurement->SignalReference;
            break;
    }

    string result = ToString();
    const size_t index = result.find(" @ ");

    return to_string(PointID) + ": " + name + " [" + measurement->SignalTypeName + "]" + result.substr(index);
}

bool HistorianKey::operator==(const HistorianKey& other) const
{
    return CompareTo(other) == 0;
}

bool HistorianKey::operator<(const HistorianKey& other) const
{
    return CompareTo(other) < 0;
}
