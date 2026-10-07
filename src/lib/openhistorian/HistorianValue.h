//******************************************************************************************************
//  HistorianValue.h - Gbtc
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
#include "HistorianTypes.h"

namespace snapdb::openhistorian
{
    // The standard SNAPdb value used for the openHistorian.
    class HistorianValue final
    {
    public:
        // Guid uniquely defining this SNAPdb type.
        static const Guid TypeID;

        // Size of this SNAPdb type when serialized.
        static constexpr uint32_t Size = 24U;

        // Value1 should be where the first 64 bits of the field is stored. For 32-bit values, use this field only.
        uint64_t Value1 {};

        // Should only be used if value cannot be entirely stored in Value1. Compression penalty occurs when using this field.
        uint64_t Value2 {};

        // Should contain any kind of digital data such as Quality. Compression penalty occurs when used for any other type of field.
        uint64_t Value3 {};

        HistorianValue() = default;
        HistorianValue(float32_t value, QualityFlags quality = QualityFlags::Normal);

        // Sets value to its minimum value.
        void SetMin();

        // Sets value to its maximum value.
        void SetMax();

        // Clears the value.
        void Clear();

        // Reads value from the stream.
        void Read(io::BinaryStream& stream);

        // Writes value to the stream.
        void Write(io::BinaryStream& stream) const;

        // Copies value to the destination.
        void CopyTo(HistorianValue& destination) const;

        // Compares value to other value. Returns -1, 0 or 1 for less than, equal to, or greater than respectively.
        int32_t CompareTo(const HistorianValue& other) const;

        // Gets `Value1` type cast as a single-precision float, the standard openHistorian measurement value.
        float32_t AsSingle() const;

        // Sets `Value1` type cast from a single-precision float, the standard openHistorian measurement value.
        void SetSingle(float32_t value);

        // Gets `Value1` type cast as a double-precision float.
        float64_t AsDouble() const;

        // Sets `Value1` type cast from a double-precision float.
        void SetDouble(float64_t value);

        // Gets the low 32-bits of `Value3` type cast as quality flags.
        QualityFlags AsQuality() const;

        // Sets `Value3` type cast from quality flags.
        void SetQuality(QualityFlags value);

        // Gets all values type cast as an alarmed flag, an associated event ID and quality flags representing an
        // alarm. `Value1` and `Value2` store the Guid-based event ID. `Value3` stores the alarmed flag in the high
        // 32-bits (with a value of 0 or 1) and quality flags in the low 32-bits.
        void GetAlarm(bool& alarmed, Guid& eventID, QualityFlags& flags) const;

        // Sets all values type cast from an alarmed flag, an associated event ID and quality flags representing an alarm.
        void SetAlarm(bool alarmed, const Guid& eventID, QualityFlags flags);

        // Gets `Value1` and `Value2` type cast as a 16 character ASCII string.
        std::string AsString() const;

        // Sets `Value1` and `Value2` type cast from a 16 character ASCII string. Throws if string is longer than 16 characters.
        void SetString(const std::string& value);

        // Gets value as a string, e.g., "1000.980 [WarningHigh]".
        std::string ToString() const;

        bool operator==(const HistorianValue& other) const;
    };
}
