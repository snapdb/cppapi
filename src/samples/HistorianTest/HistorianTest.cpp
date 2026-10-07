//******************************************************************************************************
//  HistorianTest.cpp - Gbtc
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

#include "../../lib/openhistorian/HistorianConnection.h"
#include "../../lib/Convert.h"
#include <iostream>

using namespace std;
using namespace boost::posix_time;
using namespace snapdb;
using namespace snapdb::snap;
using namespace snapdb::openhistorian;

void TestWriteAndVerify(const HistorianInstancePtr& instance);
void TestRead(const HistorianInstancePtr& instance, const MetadataCache& metadata, const datetime_t& startTime, const datetime_t& endTime, const vector<uint64_t>& pointIDs);

// Sample application to demonstrate the openHistorian API.
//
// Application connects to the openHistorian, displays available client database instances and their
// supported encodings, refreshes metadata, writes a test value and verifies it, then reads recent
// frequency and voltage magnitude data along with the data for devices matching a search value.
int main(int argc, char* argv[])
{
    // Get optional host address, e.g., "localhost:38402", and device search value
    const string hostAddress = argc > 1 ? argv[1] : "localhost";
    const string deviceSearch = argc > 2 ? argv[2] : "SUBSTATION";

    cout << "Creating openHistorian API" << endl;
    HistorianConnection historian(hostAddress);
    HistorianInstancePtr instance;

    cout << "Connecting to openHistorian..." << endl;

    try
    {
        string initialInstance;

        historian.Connect();

        if (!historian.IsConnected())
        {
            cout << "Not connected? Unexpected." << endl;
            return 1;
        }

        cout << "Connected to \"" << historian.HostAddress() << "\"!" << endl;

        // Suppress default output with: historian.RefreshMetadata();
        historian.RefreshMetadata(HistorianConnection::DefaultSttpPort, [](const string& message) { cout << message << endl; });

        cout << "Available openHistorian instances:" << endl;

        for (const DatabaseInfoPtr& instanceInfo : historian.Instances())
        {
            cout << "   " << instanceInfo->DatabaseName() << endl;

            if (initialInstance.empty())
                initialInstance = instanceInfo->DatabaseName();

            const string keyTypeName = instanceInfo->KeyTypeName().empty() ? "Unregistered key type" : instanceInfo->KeyTypeName();
            const string valueTypeName = instanceInfo->ValueTypeName().empty() ? "Unregistered value type" : instanceInfo->ValueTypeName();

            cout << "          Key Type: " << keyTypeName << " {" << ToString(instanceInfo->KeyTypeID()) << "}" << endl;
            cout << "        Value Type: " << valueTypeName << " {" << ToString(instanceInfo->ValueTypeID()) << "}" << endl;
            cout << "    Encoding Modes: " << instanceInfo->SupportedEncodings().size() << endl << endl;

            int32_t i = 0;

            for (const EncodingDefinition& mode : instanceInfo->SupportedEncodings())
            {
                cout << "        Encoding Mode " << ++i << " " << mode.ToString() << endl;
                cout << "             Key-Value Encoded: " << boolalpha << mode.IsKeyValueEncoded() << endl;
                cout << "            Fixed Size Encoded: " << boolalpha << mode.IsFixedSizeEncoding() << endl << endl;
            }
        }

        if (initialInstance.empty())
        {
            cout << "No openHistorian instances detected!" << endl;
            return 0;
        }

        cout << "Opening \"" << initialInstance << "\" database instance..." << endl;
        instance = historian.OpenInstance(initialInstance);
        cout << "Using encoding " << instance->GetEncodingDefinition().ToString() << endl;

        // Execute a test write and verify read
        TestWriteAndVerify(instance);

        // Get a reference to the openHistorian metadata cache
        const MetadataCachePtr& metadata = historian.Metadata();

        // Execute a test read for recent data
        const datetime_t endTime = UtcNow() - seconds(10);
        const datetime_t startTime = endTime - milliseconds(33 + 1); // Add one ms to ensure end time is inclusive

        // Lookup measurements for frequency signals
        vector<MeasurementRecordPtr> records = metadata->GetMeasurementsBySignalType(SignalType::FREQ, instance->Name());

        // Lookup measurements for voltage phase magnitudes
        const vector<MeasurementRecordPtr> voltageRecords = metadata->GetMeasurementsBySignalType(SignalType::VPHM, instance->Name());
        records.insert(records.end(), voltageRecords.begin(), voltageRecords.end());

        // Lookup devices by matching text fields
        for (const DeviceRecordPtr& device : metadata->GetDevicesByTextSearch(deviceSearch))
            records.insert(records.end(), device->Measurements.begin(), device->Measurements.end());

        cout << "Queried " << records.size() << " metadata records associated with \"" << instance->Name() << "\" database instance." << endl;

        if (!records.empty())
        {
            const vector<uint64_t> pointIDs = MetadataCache::ToPointIDList(records);
            cout << "Starting read for " << pointIDs.size() << " points from " << ToString(startTime) << " to " << ToString(endTime) << "..." << endl << endl;
            TestRead(instance, *metadata, startTime, endTime, pointIDs);
        }
    }
    catch (const std::exception& ex)
    {
        cerr << "Failed: " << ex.what() << endl;
        return 1;
    }

    if (instance != nullptr)
        instance->Dispose();

    if (historian.IsConnected())
        cout << "Disconnecting from openHistorian" << endl;

    historian.Disconnect();

    return 0;
}

void TestRead(const HistorianInstancePtr& instance, const MetadataCache& metadata, const datetime_t& startTime, const datetime_t& endTime, const vector<uint64_t>& pointIDs)
{
    const auto opStart = chrono::steady_clock::now();
    const HistorianReaderPtr reader = instance->Read(startTime, endTime, pointIDs);
    HistorianKey key;
    HistorianValue value;
    uint64_t count = 0;

    while (reader->Read(key, value))
    {
        count++;
        cout << "    Point " << key.ToString(metadata) << " = " << value.ToString() << endl;
    }

    const float64_t elapsed = chrono::duration<float64_t>(chrono::steady_clock::now() - opStart).count();
    cout << endl << "Read complete for " << count << " points in " << elapsed << " seconds." << endl << endl;
}

void TestWriteAndVerify(const HistorianInstancePtr& instance)
{
    cout << endl << "Executing test write and read verification..." << endl << endl;

    const datetime_t utcTime = UtcNow();
    constexpr uint64_t pointID = 1;
    const uint64_t pointTime = static_cast<uint64_t>(ToTicks(utcTime));
    constexpr float32_t pointValue = 1000.98F;
    constexpr QualityFlags pointQuality = QualityFlags::WarningHigh;

    HistorianKey key;
    HistorianValue value;

    key.PointID = pointID;
    key.Timestamp = pointTime;
    value.SetSingle(pointValue);
    value.SetQuality(pointQuality);

    cout << "Source values compared to those assigned to key/value pair:" << endl;
    cout << boolalpha;
    cout << "    Point ID Match      = " << key.PointID << ", match: " << (pointID == key.PointID) << endl;
    cout << "    Point Time Match    = " << ToString(key.GetDateTime()) << ", match: " << (pointTime == key.Timestamp) << endl;
    cout << "    Point Value Match   = " << value.AsSingle() << ", match: " << (pointValue == value.AsSingle()) << endl;
    cout << "    Point Quality Match = " << ToString(value.AsQuality()) << ", match: " << (pointQuality == value.AsQuality()) << endl;

    cout << endl << "Writing a test point..." << endl;
    instance->Write(key, value);

    ThreadSleep(500); // Wait a moment before read

    const HistorianReaderPtr reader = instance->Read(utcTime - milliseconds(1), utcTime, { pointID });
    uint64_t count = 0;

    cout << endl << "Reading test point..." << endl << endl;

    while (reader->Read(key, value))
    {
        count++;

        cout << "Source values compared to those read from historian into key/value pair:" << endl;
        cout << "    Point ID Match      = " << key.PointID << ", match: " << (pointID == key.PointID) << endl;
        cout << "    Point Time Match    = " << ToString(key.GetDateTime()) << ", match: " << (pointTime == key.Timestamp) << endl;
        cout << "    Point Value Match   = " << value.AsSingle() << ", match: " << (pointValue == value.AsSingle()) << endl;
        cout << "    Point Quality Match = " << ToString(value.AsQuality()) << ", match: " << (pointQuality == value.AsQuality()) << endl;
    }

    cout << "    Point Count Match   = " << (count == 1) << endl << endl;
}
