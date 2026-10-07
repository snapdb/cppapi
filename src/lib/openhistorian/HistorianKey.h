//******************************************************************************************************
//  HistorianKey.h - Gbtc
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

#include "../snap/SnapTypes.h"

namespace snapdb::openhistorian
{
    class MetadataCache;

    // Defines the measurement metadata field to use when displaying a historian key.
    enum class MeasurementNameField
    {
        PointTag,
        SignalReference,
        Description
    };

    // The standard SNAPdb key used for the openHistorian.
    class HistorianKey final
    {
    public:
        // Guid uniquely defining this SNAPdb type.
        static const Guid TypeID;

        // Size of this SNAPdb type when serialized.
        static constexpr uint32_t Size = 24U;

        // Timestamp of the key, in ticks, i.e., 100-nanosecond intervals since 1/1/0001 UTC.
        uint64_t Timestamp {};

        // Unique point ID of the measurement.
        uint64_t PointID {};

        // Entry number of the key, normally zero. Used to distinguish duplicate timestamp and point ID keys.
        uint64_t EntryNumber {};

        HistorianKey() = default;
        HistorianKey(uint64_t pointID, uint64_t timestamp, uint64_t entryNumber = 0ULL);
        HistorianKey(uint64_t pointID, const datetime_t& timestamp, uint64_t entryNumber = 0ULL);

        // Sets key to its minimum value.
        void SetMin();

        // Sets key to its maximum value.
        void SetMax();

        // Clears the key.
        void Clear();

        // Reads key from the stream.
        void Read(io::BinaryStream& stream);

        // Writes key to the stream.
        void Write(io::BinaryStream& stream) const;

        // Copies key to the destination.
        void CopyTo(HistorianKey& destination) const;

        // Compares key to other key. Returns -1, 0 or 1 for less than, equal to, or greater than respectively.
        int32_t CompareTo(const HistorianKey& other) const;

        // Gets timestamp rounded down to the nearest millisecond, in ticks.
        uint64_t GetMillisecondTimestamp() const;

        // Sets timestamp rounded down to the nearest millisecond, in ticks.
        void SetMillisecondTimestamp(uint64_t value);

        // Attempts to get the timestamp of this key as a date/time. Returns false if timestamp is out of range.
        bool TryGetDateTime(datetime_t& value) const;

        // Gets the timestamp of this key as a date/time. Returns `DateTime::MinValue` if timestamp is out of range.
        datetime_t GetDateTime() const;

        // Sets the timestamp of this key from a date/time.
        void SetDateTime(const datetime_t& value);

        // Gets key as a string, e.g., "1234 @ 2026-10-07 12:00:00.000".
        std::string ToString() const;

        // Gets key as a string with associated measurement metadata, e.g.,
        // "1234: DEVICE-FQ [FREQ] @ 2026-10-07 12:00:00.000".
        std::string ToString(const MetadataCache& metadata, MeasurementNameField nameField = MeasurementNameField::SignalReference) const;

        bool operator==(const HistorianKey& other) const;
        bool operator<(const HistorianKey& other) const;
    };
}
