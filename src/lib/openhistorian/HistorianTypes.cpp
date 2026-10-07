//******************************************************************************************************
//  HistorianTypes.cpp - Gbtc
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

#include "HistorianTypes.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::openhistorian;

namespace
{
    struct QualityFlagName
    {
        QualityFlags Flag;
        const char* Name;
    };

    constexpr QualityFlagName QualityFlagNames[] =
    {
        { QualityFlags::BadData, "BadData" },
        { QualityFlags::SuspectData, "SuspectData" },
        { QualityFlags::OverRangeError, "OverRangeError" },
        { QualityFlags::UnderRangeError, "UnderRangeError" },
        { QualityFlags::AlarmHigh, "AlarmHigh" },
        { QualityFlags::AlarmLow, "AlarmLow" },
        { QualityFlags::WarningHigh, "WarningHigh" },
        { QualityFlags::WarningLow, "WarningLow" },
        { QualityFlags::FlatlineAlarm, "FlatlineAlarm" },
        { QualityFlags::ComparisonAlarm, "ComparisonAlarm" },
        { QualityFlags::ROCAlarm, "ROCAlarm" },
        { QualityFlags::ReceivedAsBad, "ReceivedAsBad" },
        { QualityFlags::CalculatedValue, "CalculatedValue" },
        { QualityFlags::CalculationError, "CalculationError" },
        { QualityFlags::CalculationWarning, "CalculationWarning" },
        { QualityFlags::ReservedQualityFlag, "ReservedQualityFlag" },
        { QualityFlags::BadTime, "BadTime" },
        { QualityFlags::SuspectTime, "SuspectTime" },
        { QualityFlags::LateTimeAlarm, "LateTimeAlarm" },
        { QualityFlags::FutureTimeAlarm, "FutureTimeAlarm" },
        { QualityFlags::UpSampled, "UpSampled" },
        { QualityFlags::DownSampled, "DownSampled" },
        { QualityFlags::DiscardedValue, "DiscardedValue" },
        { QualityFlags::ReservedTimeFlag, "ReservedTimeFlag" },
        { QualityFlags::UserDefinedFlag1, "UserDefinedFlag1" },
        { QualityFlags::UserDefinedFlag2, "UserDefinedFlag2" },
        { QualityFlags::UserDefinedFlag3, "UserDefinedFlag3" },
        { QualityFlags::UserDefinedFlag4, "UserDefinedFlag4" },
        { QualityFlags::UserDefinedFlag5, "UserDefinedFlag5" },
        { QualityFlags::SystemError, "SystemError" },
        { QualityFlags::SystemWarning, "SystemWarning" },
        { QualityFlags::MeasurementError, "MeasurementError" }
    };

    struct SignalTypeName
    {
        SignalType Type;
        const char* Name;
    };

    constexpr SignalTypeName SignalTypeNames[] =
    {
        { SignalType::IPHM, "IPHM" },
        { SignalType::IPHA, "IPHA" },
        { SignalType::VPHM, "VPHM" },
        { SignalType::VPHA, "VPHA" },
        { SignalType::FREQ, "FREQ" },
        { SignalType::DFDT, "DFDT" },
        { SignalType::ALOG, "ALOG" },
        { SignalType::FLAG, "FLAG" },
        { SignalType::DIGI, "DIGI" },
        { SignalType::CALC, "CALC" },
        { SignalType::STAT, "STAT" },
        { SignalType::ALRM, "ALRM" },
        { SignalType::QUAL, "QUAL" }
    };
}

string openhistorian::ToString(const QualityFlags flags)
{
    if (flags == QualityFlags::Normal)
        return "Normal";

    string result;

    for (const auto& [flag, name] : QualityFlagNames)
    {
        if (!HasFlag(flags, flag))
            continue;

        if (!result.empty())
            result.push_back('|');

        result.append(name);
    }

    return result;
}

SignalType openhistorian::ParseSignalType(const string& signalTypeName)
{
    for (const auto& [type, name] : SignalTypeNames)
    {
        if (IsEqual(signalTypeName, name))
            return type;
    }

    return SignalType::UNKN;
}

string openhistorian::ToString(const SignalType signalType)
{
    for (const auto& [type, name] : SignalTypeNames)
    {
        if (type == signalType)
            return name;
    }

    return "UNKN";
}

MeasurementRecordPtr PhasorRecord::AngleMeasurement() const
{
    constexpr size_t index = static_cast<size_t>(CompositePhasorMeasurement::Angle);
    return Measurements.size() > index ? Measurements[index] : nullptr;
}

MeasurementRecordPtr PhasorRecord::MagnitudeMeasurement() const
{
    constexpr size_t index = static_cast<size_t>(CompositePhasorMeasurement::Magnitude);
    return Measurements.size() > index ? Measurements[index] : nullptr;
}
