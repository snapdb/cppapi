//******************************************************************************************************
//  MetadataCache.cpp - Gbtc
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

#include "MetadataCache.h"
#include "../Convert.h"
#include "../SnapException.h"
#include "../pugixml.hpp"
#include <unordered_set>

using namespace std;
using namespace pugi;
using namespace snapdb;
using namespace snapdb::openhistorian;

namespace
{
    string GetElementText(const xml_node& row, const char* elementName)
    {
        return Trim(row.child_value(elementName));
    }

    // Parses openHistorian instance name and point ID from measurement key, e.g., "PPA:123"
    bool TryGetMeasurementKey(const xml_node& row, string& instanceName, uint64_t& pointID)
    {
        const string elementText = GetElementText(row, "ID");
        const size_t index = elementText.find(':');

        if (index == string::npos || elementText.find(':', index + 1) != string::npos)
            return false;

        instanceName = Trim(elementText.substr(0, index));
        return TryParseUInt64(Trim(elementText.substr(index + 1)), pointID) && pointID > 0;
    }

    Guid GetGuid(const xml_node& row, const char* elementName)
    {
        Guid value;

        // Missing or invalid identifiers get a new unique value
        if (!TryParseGuid(GetElementText(row, elementName), value))
            value = NewGuid();

        return value;
    }

    int32_t GetInt(const xml_node& row, const char* elementName, const int32_t defaultValue = 0)
    {
        int32_t value;
        TryParseInt32(GetElementText(row, elementName), value, defaultValue);
        return value;
    }

    decimal_t GetDecimal(const xml_node& row, const char* elementName)
    {
        decimal_t value;
        TryParseDecimal(GetElementText(row, elementName), value);
        return value;
    }

    char GetChar(const xml_node& row, const char* elementName)
    {
        const string elementText = GetElementText(row, elementName);
        return elementText.empty() ? ' ' : elementText[0];
    }

    datetime_t GetUpdatedOn(const xml_node& row)
    {
        const string elementText = GetElementText(row, "UpdatedOn");
        datetime_t value;

        if (elementText.empty() || !TryParseTimestamp(elementText.c_str(), value))
            value = UtcNow();

        return value;
    }

    bool MatchesInstance(const MeasurementRecordPtr& record, const string& instanceName)
    {
        return instanceName.empty() || IsEqual(record->InstanceName, instanceName);
    }

    template<class T>
    SharedPtr<T> Lookup(const unordered_map<uint64_t, SharedPtr<T>>& map, const uint64_t key)
    {
        const auto iterator = map.find(key);
        return iterator == map.end() ? nullptr : iterator->second;
    }

    template<class T>
    SharedPtr<T> Lookup(const unordered_map<Guid, SharedPtr<T>>& map, const Guid& key)
    {
        const auto iterator = map.find(key);
        return iterator == map.end() ? nullptr : iterator->second;
    }

    template<class T>
    SharedPtr<T> Lookup(const StringMap<SharedPtr<T>>& map, const string& key)
    {
        const auto iterator = map.find(key);
        return iterator == map.end() ? nullptr : iterator->second;
    }
}

MetadataCache::MetadataCache(const string& metadataXml)
{
    xml_document document;
    const xml_parse_result result = document.load_buffer(metadataXml.data(), metadataXml.size());

    if (!result)
        throw SnapException("Failed to parse openHistorian metadata XML: " + string(result.description()));

    const xml_node root = document.document_element();

    // Extract measurement records from MeasurementDetail table rows
    for (const xml_node& row : root.children("MeasurementDetail"))
    {
        string instanceName;
        uint64_t pointID;

        if (!TryGetMeasurementKey(row, instanceName, pointID))
            continue;

        const MeasurementRecordPtr measurement = NewSharedPtr<MeasurementRecord>();

        measurement->InstanceName = instanceName;
        measurement->PointID = pointID;
        measurement->SignalID = GetGuid(row, "SignalID");
        measurement->PointTag = GetElementText(row, "PointTag");
        measurement->SignalReference = GetElementText(row, "SignalReference");
        measurement->SignalTypeName = GetElementText(row, "SignalAcronym");
        measurement->AsSignalType = ParseSignalType(measurement->SignalTypeName);
        measurement->DeviceAcronym = GetElementText(row, "DeviceAcronym");
        measurement->Description = GetElementText(row, "Description");
        measurement->UpdatedOn = GetUpdatedOn(row);

        if (measurement->SignalTypeName.empty())
            measurement->SignalTypeName = "UNKN";

        m_measurementRecords.push_back(measurement);
        m_pointIDMeasurementMap[measurement->PointID] = measurement;
        m_signalIDMeasurementMap[measurement->SignalID] = measurement;

        if (!measurement->PointTag.empty())
            m_pointTagMeasurementMap[measurement->PointTag] = measurement;

        if (!measurement->SignalReference.empty())
            m_signalRefMeasurementMap[measurement->SignalReference] = measurement;
    }

    // Extract device records from DeviceDetail table rows
    for (const xml_node& row : root.children("DeviceDetail"))
    {
        const DeviceRecordPtr device = NewSharedPtr<DeviceRecord>();

        device->NodeID = GetGuid(row, "NodeID");
        device->DeviceID = GetGuid(row, "UniqueID");
        device->Acronym = GetElementText(row, "Acronym");
        device->Name = GetElementText(row, "Name");
        device->AccessID = GetInt(row, "AccessID");
        device->ParentAcronym = GetElementText(row, "ParentAcronym");
        device->ProtocolName = GetElementText(row, "ProtocolName");
        device->FramesPerSecond = GetInt(row, "FramesPerSecond", 30);
        device->CompanyAcronym = GetElementText(row, "CompanyAcronym");
        device->VendorAcronym = GetElementText(row, "VendorAcronym");
        device->VendorDeviceName = GetElementText(row, "VendorDeviceName");
        device->Longitude = GetDecimal(row, "Longitude");
        device->Latitude = GetDecimal(row, "Latitude");
        device->UpdatedOn = GetUpdatedOn(row);

        m_deviceRecords.push_back(device);
        m_deviceIDDeviceMap[device->DeviceID] = device;

        if (!device->Acronym.empty())
            m_deviceAcronymDeviceMap[device->Acronym] = device;
    }

    // Associate measurements with parent devices
    for (const MeasurementRecordPtr& measurement : m_measurementRecords)
    {
        if (const DeviceRecordPtr device = LookupDeviceByAcronym(measurement->DeviceAcronym))
        {
            measurement->Device = device;
            device->Measurements.push_back(measurement);
        }
    }

    // Extract phasor records from PhasorDetail table rows
    for (const xml_node& row : root.children("PhasorDetail"))
    {
        const PhasorRecordPtr phasor = NewSharedPtr<PhasorRecord>();

        phasor->ID = GetInt(row, "ID");
        phasor->DeviceAcronym = GetElementText(row, "DeviceAcronym");
        phasor->Label = GetElementText(row, "Label");
        phasor->Type = GetChar(row, "Type");
        phasor->Phase = GetChar(row, "Phase");
        phasor->SourceIndex = GetInt(row, "SourceIndex");
        phasor->BaseKV = GetInt(row, "BaseKV", 500);
        phasor->UpdatedOn = GetUpdatedOn(row);

        m_phasorRecords.push_back(phasor);

        // Associate phasor with parent device and associated angle/magnitude measurements
        const DeviceRecordPtr device = LookupDeviceByAcronym(phasor->DeviceAcronym);

        if (device == nullptr)
            continue;

        phasor->Device = device;
        device->Phasors.push_back(phasor);

        const MeasurementRecordPtr angle = LookupMeasurementBySignalReference(device->Acronym + "-PA" + to_string(phasor->SourceIndex));
        const MeasurementRecordPtr magnitude = LookupMeasurementBySignalReference(device->Acronym + "-PM" + to_string(phasor->SourceIndex));

        if (angle == nullptr || magnitude == nullptr)
            continue;

        angle->Phasor = phasor;
        magnitude->Phasor = phasor;

        phasor->Measurements.clear();
        phasor->Measurements.push_back(angle);      // Must be index 0
        phasor->Measurements.push_back(magnitude);  // Must be index 1
    }
}

MetadataCache::~MetadataCache()
{
    // Release record cross-references so records can be freed
    for (const MeasurementRecordPtr& measurement : m_measurementRecords)
    {
        measurement->Device.reset();
        measurement->Phasor.reset();
    }

    for (const DeviceRecordPtr& device : m_deviceRecords)
    {
        device->Measurements.clear();
        device->Phasors.clear();
    }

    for (const PhasorRecordPtr& phasor : m_phasorRecords)
    {
        phasor->Device.reset();
        phasor->Measurements.clear();
    }
}

const vector<MeasurementRecordPtr>& MetadataCache::MeasurementRecords() const
{
    return m_measurementRecords;
}

const vector<DeviceRecordPtr>& MetadataCache::DeviceRecords() const
{
    return m_deviceRecords;
}

const vector<PhasorRecordPtr>& MetadataCache::PhasorRecords() const
{
    return m_phasorRecords;
}

MeasurementRecordPtr MetadataCache::LookupMeasurementByPointID(const uint64_t pointID) const
{
    return Lookup(m_pointIDMeasurementMap, pointID);
}

MeasurementRecordPtr MetadataCache::LookupMeasurementBySignalID(const Guid& signalID) const
{
    return Lookup(m_signalIDMeasurementMap, signalID);
}

MeasurementRecordPtr MetadataCache::LookupMeasurementByPointTag(const string& pointTag) const
{
    return Lookup(m_pointTagMeasurementMap, pointTag);
}

MeasurementRecordPtr MetadataCache::LookupMeasurementBySignalReference(const string& signalReference) const
{
    return Lookup(m_signalRefMeasurementMap, signalReference);
}

vector<MeasurementRecordPtr> MetadataCache::GetMeasurementsBySignalType(const SignalType signalType, const string& instanceName) const
{
    vector<MeasurementRecordPtr> records;

    for (const MeasurementRecordPtr& record : m_measurementRecords)
    {
        if (record->AsSignalType == signalType && MatchesInstance(record, instanceName))
            records.push_back(record);
    }

    return records;
}

vector<MeasurementRecordPtr> MetadataCache::GetMeasurementsByTextSearch(const string& searchValue, const string& instanceName) const
{
    vector<MeasurementRecordPtr> records;

    if (searchValue.empty())
        return records;

    for (const MeasurementRecordPtr& record : m_measurementRecords)
    {
        if (!MatchesInstance(record, instanceName))
            continue;

        if (IsEqual(record->PointTag, searchValue) ||
            IsEqual(record->SignalReference, searchValue) ||
            Contains(record->Description, searchValue) ||
            Contains(record->DeviceAcronym, searchValue))
            records.push_back(record);
    }

    return records;
}

DeviceRecordPtr MetadataCache::LookupDeviceByAcronym(const string& deviceAcronym) const
{
    return Lookup(m_deviceAcronymDeviceMap, deviceAcronym);
}

DeviceRecordPtr MetadataCache::LookupDeviceByID(const Guid& deviceID) const
{
    return Lookup(m_deviceIDDeviceMap, deviceID);
}

vector<DeviceRecordPtr> MetadataCache::GetDevicesByTextSearch(const string& searchValue, const string& instanceName) const
{
    vector<DeviceRecordPtr> records;

    if (searchValue.empty())
        return records;

    for (const DeviceRecordPtr& record : m_deviceRecords)
    {
        if (!(Contains(record->Acronym, searchValue) ||
              Contains(record->Name, searchValue) ||
              Contains(record->ParentAcronym, searchValue) ||
              Contains(record->CompanyAcronym, searchValue) ||
              Contains(record->VendorAcronym, searchValue) ||
              Contains(record->VendorDeviceName, searchValue)))
            continue;

        if (!instanceName.empty() && ranges::none_of(record->Measurements, [&](const MeasurementRecordPtr& measurement) { return MatchesInstance(measurement, instanceName); }))
            continue;

        records.push_back(record);
    }

    return records;
}

vector<uint64_t> MetadataCache::ToPointIDList(const vector<MeasurementRecordPtr>& records)
{
    vector<uint64_t> pointIDs;
    unordered_set<uint64_t> distinctPointIDs;

    pointIDs.reserve(records.size());

    for (const MeasurementRecordPtr& record : records)
    {
        if (distinctPointIDs.insert(record->PointID).second)
            pointIDs.push_back(record->PointID);
    }

    return pointIDs;
}
