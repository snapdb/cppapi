//******************************************************************************************************
//  Filters.h - Gbtc
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

#include "SnapTypes.h"

namespace snapdb::snap
{
    // Represents a filter that is based on a series of ranges of the key value. Seek filters
    // are evaluated server-side, the client API only needs to serialize the filter definition.
    class SeekFilterBase
    {
    public:
        virtual ~SeekFilterBase() = default;

        // Gets the Guid uniquely defining this SNAPdb seek filter type.
        virtual const Guid& TypeID() const = 0;

        // Serializes the filter to a stream.
        virtual void Save(io::BinaryStream& stream) const = 0;
    };

    typedef SharedPtr<SeekFilterBase> SeekFilterBasePtr;

    // Represents a filter that matches based on the keys and values. Match filters are
    // evaluated server-side, the client API only needs to serialize the filter definition.
    class MatchFilterBase
    {
    public:
        virtual ~MatchFilterBase() = default;

        // Gets the Guid uniquely defining this SNAPdb match filter type.
        virtual const Guid& TypeID() const = 0;

        // Serializes the filter to a stream.
        virtual void Save(io::BinaryStream& stream) const = 0;
    };

    typedef SharedPtr<MatchFilterBase> MatchFilterBasePtr;

    // Creates a seek filter based on time ranges and optional intervals. For use with
    // SNAPdb key types that include a timestamp, e.g., `HistorianKey`.
    class TimestampSeekFilter final : public SeekFilterBase
    {
    private:
        uint64_t m_firstTime;
        uint64_t m_lastTime;
        uint64_t m_mainInterval;
        uint64_t m_subInterval;
        uint64_t m_tolerance;

    public:
        // Guid that defines the timestamp seek filter type.
        static const Guid SnapTypeID;

        // Creates a seek filter over a single date range (inclusive) with optional interval data,
        // all values are in ticks, i.e., 100-nanosecond intervals since 1/1/0001.
        //   firstTime: the first time of the query (inclusive).
        //   lastTime: the last time of the query (inclusive).
        //   mainInterval: the smallest interval that is exact.
        //   subInterval: the interval that will be parsed (round possible).
        //   tolerance: the width of every window.
        TimestampSeekFilter(uint64_t firstTime, uint64_t lastTime, uint64_t mainInterval = 0, uint64_t subInterval = 0, uint64_t tolerance = 0);

        const Guid& TypeID() const override;
        void Save(io::BinaryStream& stream) const override;

        uint64_t FirstTime() const;
        uint64_t LastTime() const;
        uint64_t MainInterval() const;
        uint64_t SubInterval() const;
        uint64_t Tolerance() const;

        // Gets flag that determines if filter is a fixed range, i.e., has no interval data.
        bool IsFixedRange() const;

        // Creates a seek filter over a single date range (inclusive).
        static SeekFilterBasePtr CreateFromRange(const datetime_t& firstTime, const datetime_t& lastTime);

        // Creates a seek filter over a single date range (inclusive), with times specified in ticks.
        static SeekFilterBasePtr CreateFromRange(uint64_t firstTime, uint64_t lastTime);

        // Creates a seek filter over a single date range (inclusive), skipping values
        // based on specified interval and tolerance, e.g., interval of 0.1 seconds
        // and a tolerance of 0.001 seconds.
        static SeekFilterBasePtr CreateFromIntervalData(const datetime_t& firstTime, const datetime_t& lastTime, const TimeSpan& interval, const TimeSpan& tolerance);

        // Creates a seek filter over a single date range (inclusive), skipping values based on
        // specified main and sub intervals and tolerance, e.g., main interval of 0.1 seconds,
        // sub-interval of 0.0333333 seconds and a tolerance of 0.001 seconds.
        static SeekFilterBasePtr CreateFromIntervalData(const datetime_t& firstTime, const datetime_t& lastTime, const TimeSpan& mainInterval, const TimeSpan& subInterval, const TimeSpan& tolerance);

        // Creates a seek filter over a single date range (inclusive), with all values specified in ticks.
        static SeekFilterBasePtr CreateFromIntervalData(uint64_t firstTime, uint64_t lastTime, uint64_t mainInterval, uint64_t subInterval, uint64_t tolerance);
    };

    // Creates a seek filter that finds a single key with an exact timestamp and point ID.
    // Note: this filter requires a SNAPdb server version that supports it.
    class TimestampPointIDSeekFilter final : public SeekFilterBase
    {
    private:
        uint64_t m_timestamp;
        uint64_t m_pointID;

    public:
        // Guid that defines the timestamp and point ID seek filter type.
        static const Guid SnapTypeID;

        TimestampPointIDSeekFilter(uint64_t timestamp, uint64_t pointID);

        const Guid& TypeID() const override;
        void Save(io::BinaryStream& stream) const override;

        uint64_t Timestamp() const;
        uint64_t PointID() const;

        // Creates a seek filter that finds the key with the specified timestamp, in ticks, and point ID.
        static SeekFilterBasePtr FindKey(uint64_t timestamp, uint64_t pointID);

        // Creates a seek filter that finds the key with the specified timestamp and point ID.
        static SeekFilterBasePtr FindKey(const datetime_t& timestamp, uint64_t pointID);
    };

    // Creates a match filter based on a set of point IDs. For use with SNAPdb
    // key types that include a point ID, e.g., `HistorianKey`.
    class PointIDMatchFilter final : public MatchFilterBase
    {
    private:
        std::vector<uint64_t> m_pointIDs;
        uint64_t m_maxValue;

    public:
        // Guid that defines the point ID match filter type.
        static const Guid SnapTypeID;

        // Creates a match filter from the set of point IDs provided. Duplicates are removed.
        PointIDMatchFilter(std::vector<uint64_t> pointIDs);

        const Guid& TypeID() const override;
        void Save(io::BinaryStream& stream) const override;

        // Gets the sorted, distinct point IDs for this filter.
        const std::vector<uint64_t>& PointIDs() const;

        // Creates a match filter from the list of point IDs provided.
        static MatchFilterBasePtr CreateFromList(const std::vector<uint64_t>& pointIDs);

        // Creates a match filter for a single point ID.
        static MatchFilterBasePtr CreateFromPointID(uint64_t pointID);
    };
}
