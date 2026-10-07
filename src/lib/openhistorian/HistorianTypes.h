//******************************************************************************************************
//  HistorianTypes.h - Gbtc
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

namespace snapdb::openhistorian
{
    // Defines measurement quality flags, i.e., openHistorian `MeasurementStateFlags`.
    enum class QualityFlags : uint32_t
    {
        Normal = 0x0,                       // Defines normal state.
        BadData = 0x1,                      // Defines bad data state.
        SuspectData = 0x2,                  // Defines suspect data state.
        OverRangeError = 0x4,               // Defines over range error, i.e., unreasonable high value.
        UnderRangeError = 0x8,              // Defines under range error, i.e., unreasonable low value.
        AlarmHigh = 0x10,                   // Defines alarm for high value.
        AlarmLow = 0x20,                    // Defines alarm for low value.
        WarningHigh = 0x40,                 // Defines warning for high value.
        WarningLow = 0x80,                  // Defines warning for low value.
        FlatlineAlarm = 0x100,              // Defines alarm for flat-lined value, i.e., latched value test alarm.
        ComparisonAlarm = 0x200,            // Defines comparison alarm, i.e., outside threshold of comparison with a real-time value.
        ROCAlarm = 0x400,                   // Defines rate-of-change alarm.
        ReceivedAsBad = 0x800,              // Defines bad value received.
        CalculatedValue = 0x1000,           // Defines calculated value state.
        CalculationError = 0x2000,          // Defines calculation error with the value.
        CalculationWarning = 0x4000,        // Defines calculation warning with the value.
        ReservedQualityFlag = 0x8000,       // Defines reserved quality flag.
        BadTime = 0x10000,                  // Defines bad time state.
        SuspectTime = 0x20000,              // Defines suspect time state.
        LateTimeAlarm = 0x40000,            // Defines late time alarm.
        FutureTimeAlarm = 0x80000,          // Defines future time alarm.
        UpSampled = 0x100000,               // Defines up-sampled state.
        DownSampled = 0x200000,             // Defines down-sampled state.
        DiscardedValue = 0x400000,          // Defines discarded value state.
        ReservedTimeFlag = 0x800000,        // Defines reserved time flag.
        UserDefinedFlag1 = 0x1000000,       // Defines user defined flag 1.
        UserDefinedFlag2 = 0x2000000,       // Defines user defined flag 2.
        UserDefinedFlag3 = 0x4000000,       // Defines user defined flag 3.
        UserDefinedFlag4 = 0x8000000,       // Defines user defined flag 4.
        UserDefinedFlag5 = 0x10000000,      // Defines user defined flag 5.
        SystemError = 0x20000000,           // Defines system error state.
        SystemWarning = 0x40000000,         // Defines system warning state.
        MeasurementError = 0x80000000       // Defines measurement error flag.
    };

    constexpr QualityFlags operator|(QualityFlags left, QualityFlags right)
    {
        return static_cast<QualityFlags>(static_cast<uint32_t>(left) | static_cast<uint32_t>(right));
    }

    constexpr QualityFlags operator&(QualityFlags left, QualityFlags right)
    {
        return static_cast<QualityFlags>(static_cast<uint32_t>(left) & static_cast<uint32_t>(right));
    }

    constexpr QualityFlags operator^(QualityFlags left, QualityFlags right)
    {
        return static_cast<QualityFlags>(static_cast<uint32_t>(left) ^ static_cast<uint32_t>(right));
    }

    constexpr QualityFlags operator~(QualityFlags value)
    {
        return static_cast<QualityFlags>(~static_cast<uint32_t>(value));
    }

    inline QualityFlags& operator|=(QualityFlags& left, QualityFlags right)
    {
        return left = left | right;
    }

    inline QualityFlags& operator&=(QualityFlags& left, QualityFlags right)
    {
        return left = left & right;
    }

    // Determines if any of the specified flags are set.
    constexpr bool HasFlag(QualityFlags value, QualityFlags flags)
    {
        return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flags)) != 0U;
    }

    // Gets quality flags as a string, e.g., "WarningHigh|CalculatedValue".
    std::string ToString(QualityFlags flags);

    // Represents common signal types for openHistorian metadata. This list may not be
    // exhaustive for some openHistorian deployments. If value is set to `UNKN`, check
    // the string based `SignalTypeName` in the `MeasurementRecord`.
    enum class SignalType : int32_t
    {
        IPHM = 1,   // Current phase magnitude
        IPHA = 2,   // Current phase angle
        VPHM = 3,   // Voltage phase magnitude
        VPHA = 4,   // Voltage phase angle
        FREQ = 5,   // Frequency
        DFDT = 6,   // Frequency derivative, i.e., delta-freq / delta-time
        ALOG = 7,   // Analog value (scalar)
        FLAG = 8,   // Status flags (16-bit)
        DIGI = 9,   // Digital value (16-bit)
        CALC = 10,  // Calculated value
        STAT = 11,  // Statistic value
        ALRM = 12,  // Alarm state
        QUAL = 13,  // Quality flags (16-bit)
        UNKN = -1   // Unknown type, see `SignalTypeName`
    };

    // Parses signal type acronym, e.g., "FREQ", returning `SignalType::UNKN` if not recognized.
    SignalType ParseSignalType(const std::string& signalTypeName);

    // Gets signal type acronym, e.g., "FREQ".
    std::string ToString(SignalType signalType);

    // Defines the measurement index of a composite phasor measurement.
    enum class CompositePhasorMeasurement
    {
        Angle = 0,
        Magnitude = 1
    };

    struct DeviceRecord;
    struct PhasorRecord;

    typedef SharedPtr<DeviceRecord> DeviceRecordPtr;
    typedef SharedPtr<PhasorRecord> PhasorRecordPtr;

    // Represents a record of measurement metadata in the openHistorian.
    struct MeasurementRecord
    {
        std::string InstanceName;                   // <MeasurementDetail>/<ID> (left part of measurement key), source historian instance name
        uint64_t PointID {};                        // <MeasurementDetail>/<ID> (right part of measurement key), openHistorian point ID
        Guid SignalID {};                           // <MeasurementDetail>/<SignalID>, unique measurement identifier
        std::string PointTag;                       // <MeasurementDetail>/<PointTag>, unique point tag
        std::string SignalReference;                // <MeasurementDetail>/<SignalReference>, unique signal reference
        std::string SignalTypeName { "UNKN" };      // <MeasurementDetail>/<SignalAcronym>, signal type name
        SignalType AsSignalType { SignalType::UNKN }; // Signal type enumeration parsed from `SignalTypeName`
        std::string DeviceAcronym;                  // <MeasurementDetail>/<DeviceAcronym>, associated device acronym
        std::string Description;                    // <MeasurementDetail>/<Description>, measurement description
        datetime_t UpdatedOn;                       // <MeasurementDetail>/<UpdatedOn>, time of last metadata update
        DeviceRecordPtr Device;                     // Associated device record, if any
        PhasorRecordPtr Phasor;                     // Associated phasor record, if any
    };

    typedef SharedPtr<MeasurementRecord> MeasurementRecordPtr;

    // Represents a record of device metadata in the openHistorian.
    struct DeviceRecord
    {
        Guid NodeID {};                             // <DeviceDetail>/<NodeID>, openHistorian node identifier
        Guid DeviceID {};                           // <DeviceDetail>/<UniqueID>, unique device identifier
        std::string Acronym;                        // <DeviceDetail>/<Acronym>, unique alpha-numeric device identifier
        std::string Name;                           // <DeviceDetail>/<Name>, free form device name
        int32_t AccessID {};                        // <DeviceDetail>/<AccessID>, access ID, a.k.a., ID code
        std::string ParentAcronym;                  // <DeviceDetail>/<ParentAcronym>, parent device acronym, if any
        std::string ProtocolName;                   // <DeviceDetail>/<ProtocolName>, source protocol name
        int32_t FramesPerSecond { 30 };             // <DeviceDetail>/<FramesPerSecond>, data reporting rate
        std::string CompanyAcronym;                 // <DeviceDetail>/<CompanyAcronym>, associated company acronym
        std::string VendorAcronym;                  // <DeviceDetail>/<VendorAcronym>, associated vendor acronym
        std::string VendorDeviceName;               // <DeviceDetail>/<VendorDeviceName>, associated vendor device name
        decimal_t Longitude {};                     // <DeviceDetail>/<Longitude>, device longitude
        decimal_t Latitude {};                      // <DeviceDetail>/<Latitude>, device latitude
        datetime_t UpdatedOn;                       // <DeviceDetail>/<UpdatedOn>, time of last metadata update
        std::vector<MeasurementRecordPtr> Measurements; // Measurement records associated with device
        std::vector<PhasorRecordPtr> Phasors;       // Phasor records associated with device
    };

    // Represents a record of phasor metadata in the openHistorian.
    struct PhasorRecord
    {
        int32_t ID {};                              // <PhasorDetail>/<ID>, unique integer identifier
        std::string DeviceAcronym;                  // <PhasorDetail>/<DeviceAcronym>, associated device acronym
        std::string Label;                          // <PhasorDetail>/<Label>, free form phasor label
        char Type { ' ' };                          // <PhasorDetail>/<Type>, phasor type, i.e., 'I' or 'V' for current or voltage
        char Phase { ' ' };                         // <PhasorDetail>/<Phase>, phasor phase, e.g., 'A', 'B', 'C', '+', '-', '0', etc.
        int32_t SourceIndex {};                     // <PhasorDetail>/<SourceIndex>, 1-based ordering index of phasor in original context
        int32_t BaseKV { 500 };                     // <PhasorDetail>/<BaseKV>, base, i.e., nominal, kV level
        datetime_t UpdatedOn;                       // <PhasorDetail>/<UpdatedOn>, time of last metadata update
        DeviceRecordPtr Device;                     // Associated device record, if any
        std::vector<MeasurementRecordPtr> Measurements; // Associated angle (index 0) and magnitude (index 1) measurement records

        // Gets the associated angle measurement record, or nullptr if not available.
        MeasurementRecordPtr AngleMeasurement() const;

        // Gets the associated magnitude measurement record, or nullptr if not available.
        MeasurementRecordPtr MagnitudeMeasurement() const;
    };
}
