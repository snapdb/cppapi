//******************************************************************************************************
//  ReadTest.cpp - Gbtc
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
using namespace snapdb;
using namespace snapdb::snap;
using namespace snapdb::openhistorian;

void TestRead(const HistorianInstancePtr& instance, const MetadataCache& metadata, const datetime_t& startTime, const datetime_t& endTime, const vector<uint64_t>& pointIDs);

// Sample application to demonstrate reading data from the openHistorian.
//
// Application connects to the openHistorian, opens the first client database instance, refreshes
// available metadata, filters metadata to the frequency measurements, or measurements of a specified
// device, establishes a start and stop time for the read, then reads each time-series value as a
// historian key/value pair.
int main(int argc, char* argv[])
{
    // Get optional host address, e.g., "localhost:38402", and device acronym
    const string hostAddress = argc > 1 ? argv[1] : "localhost";
    const string deviceAcronym = argc > 2 ? argv[2] : "";

    // Create historian connection (the root API object)
    HistorianConnection historian(hostAddress);

    try
    {
        cout << "Connecting to openHistorian at \"" << historian.HostAddress() << "\"..." << endl;
        historian.Connect();

        if (!historian.IsConnected() || historian.InstanceNames().empty())
        {
            cout << "No openHistorian instances detected!" << endl;
            return 0;
        }

        // Get first historian instance
        const string initialInstance = historian.InstanceNames()[0];

        cout << "Opening \"" << initialInstance << "\" database instance..." << endl;
        const HistorianInstancePtr instance = historian.OpenInstance(initialInstance);

        // Get a reference to the openHistorian metadata cache
        historian.RefreshMetadata(HistorianConnection::DefaultSttpPort, [](const string& message) { cout << message << endl; });
        const MetadataCachePtr& metadata = historian.Metadata();

        vector<MeasurementRecordPtr> records;

        if (deviceAcronym.empty())
        {
            // Lookup measurements that represent frequency values
            records = metadata->GetMeasurementsBySignalType(SignalType::FREQ, instance->Name());
        }
        else
        {
            // Lookup measurements for specified device
            const DeviceRecordPtr device = metadata->LookupDeviceByAcronym(deviceAcronym);

            if (device == nullptr)
                cout << "Device \"" << deviceAcronym << "\" not found in metadata." << endl;
            else
                records = device->Measurements;
        }

        cout << "Queried " << records.size() << " metadata records associated with \"" << instance->Name() << "\" database instance." << endl;

        if (!records.empty())
        {
            const vector<uint64_t> pointIDs = MetadataCache::ToPointIDList(records);

            // Execute a test read for data archived ten seconds ago
            const datetime_t endTime = UtcNow() - boost::posix_time::seconds(10);
            const datetime_t startTime = endTime - boost::posix_time::milliseconds(33);

            cout << "Starting read for " << pointIDs.size() << " points from " << ToString(startTime) << " to " << ToString(endTime) << "..." << endl << endl;

            TestRead(instance, *metadata, startTime, endTime, pointIDs);
        }
    }
    catch (const std::exception& ex)
    {
        cerr << "Failed to read from openHistorian: " << ex.what() << endl;
        return 1;
    }

    if (historian.IsConnected())
        cout << "Disconnecting from openHistorian" << endl;

    historian.Disconnect();

    return 0;
}

void TestRead(const HistorianInstancePtr& instance, const MetadataCache& metadata, const datetime_t& startTime, const datetime_t& endTime, const vector<uint64_t>& pointIDs)
{
    const SeekFilterBasePtr timeFilter = TimestampSeekFilter::CreateFromRange(startTime, endTime);
    const MatchFilterBasePtr pointFilter = PointIDMatchFilter::CreateFromList(pointIDs);
    const auto opStart = chrono::steady_clock::now();

    const HistorianReaderPtr reader = instance->Read(timeFilter, pointFilter);
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
