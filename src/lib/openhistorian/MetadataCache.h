//******************************************************************************************************
//  MetadataCache.h - Gbtc
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

#include "HistorianTypes.h"

namespace snapdb::openhistorian
{
    // Represents a collection of openHistorian metadata. Records cross-reference each other, e.g.,
    // a device record references its measurement records, and these references are released when
    // the cache is destroyed. Keep the cache alive while using record relationships. Lookups by
    // name, e.g., point tag or device acronym, are case-insensitive.
    class MetadataCache final
    {
    private:
        std::vector<MeasurementRecordPtr> m_measurementRecords;
        std::vector<DeviceRecordPtr> m_deviceRecords;
        std::vector<PhasorRecordPtr> m_phasorRecords;
        std::unordered_map<uint64_t, MeasurementRecordPtr> m_pointIDMeasurementMap;
        std::unordered_map<Guid, MeasurementRecordPtr> m_signalIDMeasurementMap;
        StringMap<MeasurementRecordPtr> m_pointTagMeasurementMap;
        StringMap<MeasurementRecordPtr> m_signalRefMeasurementMap;
        StringMap<DeviceRecordPtr> m_deviceAcronymDeviceMap;
        std::unordered_map<Guid, DeviceRecordPtr> m_deviceIDDeviceMap;

    public:
        // Creates a new metadata cache from openHistorian metadata XML, i.e.,
        // an XML serialized data set with "MeasurementDetail", "DeviceDetail"
        // and "PhasorDetail" tables.
        MetadataCache(const std::string& metadataXml);
        ~MetadataCache();

        MetadataCache(const MetadataCache&) = delete;
        MetadataCache& operator=(const MetadataCache&) = delete;

        // Gets all measurement records.
        const std::vector<MeasurementRecordPtr>& MeasurementRecords() const;

        // Gets all device records.
        const std::vector<DeviceRecordPtr>& DeviceRecords() const;

        // Gets all phasor records.
        const std::vector<PhasorRecordPtr>& PhasorRecords() const;

        // Gets measurement record for point ID, or nullptr if not found.
        MeasurementRecordPtr LookupMeasurementByPointID(uint64_t pointID) const;

        // Gets measurement record for signal ID, or nullptr if not found.
        MeasurementRecordPtr LookupMeasurementBySignalID(const Guid& signalID) const;

        // Gets measurement record for point tag, or nullptr if not found.
        MeasurementRecordPtr LookupMeasurementByPointTag(const std::string& pointTag) const;

        // Gets measurement record for signal reference, or nullptr if not found.
        MeasurementRecordPtr LookupMeasurementBySignalReference(const std::string& signalReference) const;

        // Gets measurement records with the specified signal type, optionally filtered to an instance name.
        std::vector<MeasurementRecordPtr> GetMeasurementsBySignalType(SignalType signalType, const std::string& instanceName = {}) const;

        // Gets measurement records that match search value, optionally filtered to an instance name. Value
        // matches exact point tag or signal reference, or is contained in description or device acronym.
        std::vector<MeasurementRecordPtr> GetMeasurementsByTextSearch(const std::string& searchValue, const std::string& instanceName = {}) const;

        // Gets device record for acronym, or nullptr if not found.
        DeviceRecordPtr LookupDeviceByAcronym(const std::string& deviceAcronym) const;

        // Gets device record for device ID, or nullptr if not found.
        DeviceRecordPtr LookupDeviceByID(const Guid& deviceID) const;

        // Gets device records that match search value, optionally filtered to devices with measurements in an
        // instance name. Value is contained in acronym, name, parent acronym, company, vendor or vendor device name.
        std::vector<DeviceRecordPtr> GetDevicesByTextSearch(const std::string& searchValue, const std::string& instanceName = {}) const;

        // Gets distinct point IDs for measurement records.
        static std::vector<uint64_t> ToPointIDList(const std::vector<MeasurementRecordPtr>& records);
    };

    typedef SharedPtr<MetadataCache> MetadataCachePtr;
}
