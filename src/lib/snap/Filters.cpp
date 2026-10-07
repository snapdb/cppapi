//******************************************************************************************************
//  Filters.cpp - Gbtc
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

#include "Filters.h"
#include "../Convert.h"
#include <algorithm>

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::snap;

// {0F0F9478-DC42-4EEF-9F26-231A942EF1FA}
const Guid TimestampSeekFilter::SnapTypeID = ParseGuid("0f0f9478-dc42-4eef-9f26-231a942ef1fa");

// {8E0A841F-03C3-4B55-87CB-9F9A732F86DC}
const Guid TimestampPointIDSeekFilter::SnapTypeID = ParseGuid("8e0a841f-03c3-4b55-87cb-9f9a732f86dc");

// {2034A3E3-F92E-4749-9306-B04DC36FD743}
const Guid PointIDMatchFilter::SnapTypeID = ParseGuid("2034a3e3-f92e-4749-9306-b04dc36fd743");

namespace
{
    uint64_t ToUInt64Ticks(const datetime_t& value)
    {
        const int64_t ticks = ToTicks(value);

        if (ticks < 0)
            throw out_of_range("Timestamp is before 1/1/0001");

        return static_cast<uint64_t>(ticks);
    }

    uint64_t ToUInt64Ticks(const TimeSpan& value)
    {
        if (value.is_negative())
            throw out_of_range("Time interval cannot be negative");

        return static_cast<uint64_t>(ToTicks(value));
    }
}

// TimestampSeekFilter

TimestampSeekFilter::TimestampSeekFilter(const uint64_t firstTime, const uint64_t lastTime, const uint64_t mainInterval, const uint64_t subInterval, const uint64_t tolerance) :
    m_firstTime(firstTime),
    m_lastTime(lastTime),
    m_mainInterval(mainInterval),
    m_subInterval(subInterval),
    m_tolerance(tolerance)
{
    if (firstTime > lastTime)
        throw out_of_range("Timestamp seek filter first time must be before last time");

    if (IsFixedRange())
        return;

    if (mainInterval < subInterval)
        throw out_of_range("Timestamp seek filter main interval must be larger than the sub-interval");

    if (tolerance > subInterval)
        throw out_of_range("Timestamp seek filter tolerance must be smaller than the sub-interval");
}

const Guid& TimestampSeekFilter::TypeID() const
{
    return SnapTypeID;
}

void TimestampSeekFilter::Save(BinaryStream& stream) const
{
    const bool fixedRange = IsFixedRange();

    stream.WriteByte(fixedRange ? 1 : 2);
    stream.WriteUInt64(m_firstTime);
    stream.WriteUInt64(m_lastTime);

    if (fixedRange)
        return;

    stream.WriteUInt64(m_mainInterval);
    stream.WriteUInt64(m_subInterval);
    stream.WriteUInt64(m_tolerance);
}

uint64_t TimestampSeekFilter::FirstTime() const
{
    return m_firstTime;
}

uint64_t TimestampSeekFilter::LastTime() const
{
    return m_lastTime;
}

uint64_t TimestampSeekFilter::MainInterval() const
{
    return m_mainInterval;
}

uint64_t TimestampSeekFilter::SubInterval() const
{
    return m_subInterval;
}

uint64_t TimestampSeekFilter::Tolerance() const
{
    return m_tolerance;
}

bool TimestampSeekFilter::IsFixedRange() const
{
    return m_mainInterval == 0 && m_subInterval == 0 && m_tolerance == 0;
}

SeekFilterBasePtr TimestampSeekFilter::CreateFromRange(const datetime_t& firstTime, const datetime_t& lastTime)
{
    return CreateFromRange(ToUInt64Ticks(firstTime), ToUInt64Ticks(lastTime));
}

SeekFilterBasePtr TimestampSeekFilter::CreateFromRange(const uint64_t firstTime, const uint64_t lastTime)
{
    return NewSharedPtr<TimestampSeekFilter>(firstTime, lastTime);
}

SeekFilterBasePtr TimestampSeekFilter::CreateFromIntervalData(const datetime_t& firstTime, const datetime_t& lastTime, const TimeSpan& interval, const TimeSpan& tolerance)
{
    return CreateFromIntervalData(firstTime, lastTime, interval, interval, tolerance);
}

SeekFilterBasePtr TimestampSeekFilter::CreateFromIntervalData(const datetime_t& firstTime, const datetime_t& lastTime, const TimeSpan& mainInterval, const TimeSpan& subInterval, const TimeSpan& tolerance)
{
    return CreateFromIntervalData(ToUInt64Ticks(firstTime), ToUInt64Ticks(lastTime), ToUInt64Ticks(mainInterval), ToUInt64Ticks(subInterval), ToUInt64Ticks(tolerance));
}

SeekFilterBasePtr TimestampSeekFilter::CreateFromIntervalData(const uint64_t firstTime, const uint64_t lastTime, const uint64_t mainInterval, const uint64_t subInterval, const uint64_t tolerance)
{
    return NewSharedPtr<TimestampSeekFilter>(firstTime, lastTime, mainInterval, subInterval, tolerance);
}

// TimestampPointIDSeekFilter

TimestampPointIDSeekFilter::TimestampPointIDSeekFilter(const uint64_t timestamp, const uint64_t pointID) :
    m_timestamp(timestamp),
    m_pointID(pointID)
{
}

const Guid& TimestampPointIDSeekFilter::TypeID() const
{
    return SnapTypeID;
}

void TimestampPointIDSeekFilter::Save(BinaryStream& stream) const
{
    stream.WriteUInt64(m_timestamp);
    stream.WriteUInt64(m_pointID);
}

uint64_t TimestampPointIDSeekFilter::Timestamp() const
{
    return m_timestamp;
}

uint64_t TimestampPointIDSeekFilter::PointID() const
{
    return m_pointID;
}

SeekFilterBasePtr TimestampPointIDSeekFilter::FindKey(const uint64_t timestamp, const uint64_t pointID)
{
    return NewSharedPtr<TimestampPointIDSeekFilter>(timestamp, pointID);
}

SeekFilterBasePtr TimestampPointIDSeekFilter::FindKey(const datetime_t& timestamp, const uint64_t pointID)
{
    return FindKey(ToUInt64Ticks(timestamp), pointID);
}

// PointIDMatchFilter

PointIDMatchFilter::PointIDMatchFilter(vector<uint64_t> pointIDs) :
    m_pointIDs(std::move(pointIDs))
{
    ranges::sort(m_pointIDs);
    m_pointIDs.erase(ranges::unique(m_pointIDs).begin(), m_pointIDs.end());
    m_maxValue = m_pointIDs.empty() ? 0ULL : m_pointIDs.back();
}

const Guid& PointIDMatchFilter::TypeID() const
{
    return SnapTypeID;
}

void PointIDMatchFilter::Save(BinaryStream& stream) const
{
    // Version 1 serializes point IDs as 32-bit values, version 2 as 64-bit values
    const bool targetUInt32 = m_maxValue <= UInt32::MaxValue;

    stream.WriteByte(targetUInt32 ? 1 : 2);
    stream.WriteUInt64(m_maxValue);
    stream.WriteInt32(ConvertInt32(m_pointIDs.size()));

    if (targetUInt32)
    {
        for (const uint64_t pointID : m_pointIDs)
            stream.WriteUInt32(static_cast<uint32_t>(pointID));
    }
    else
    {
        for (const uint64_t pointID : m_pointIDs)
            stream.WriteUInt64(pointID);
    }
}

const vector<uint64_t>& PointIDMatchFilter::PointIDs() const
{
    return m_pointIDs;
}

MatchFilterBasePtr PointIDMatchFilter::CreateFromList(const vector<uint64_t>& pointIDs)
{
    return NewSharedPtr<PointIDMatchFilter>(pointIDs);
}

MatchFilterBasePtr PointIDMatchFilter::CreateFromPointID(const uint64_t pointID)
{
    return NewSharedPtr<PointIDMatchFilter>(vector{ pointID });
}
