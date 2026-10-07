//******************************************************************************************************
//  MetadataTest.cpp - Gbtc
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
#include <map>

using namespace std;
using namespace snapdb;
using namespace snapdb::openhistorian;

// Sample application to demonstrate querying openHistorian metadata.
//
// Application connects to the openHistorian, refreshes metadata using the openHistorian STTP
// publisher, displays a summary of available signal types, then displays the measurements and
// phasors for the specified device, or the first device when no device is specified.
int main(int argc, char* argv[])
{
    // Get optional host address, e.g., "localhost:38402", device acronym and STTP port
    const string hostAddress = argc > 1 ? argv[1] : "localhost";
    const string deviceAcronym = argc > 2 ? argv[2] : "";
    uint16_t sttpPort = HistorianConnection::DefaultSttpPort;

    if (argc > 3 && !TryParseUInt16(argv[3], sttpPort))
    {
        cerr << "Invalid STTP port: " << argv[3] << endl;
        return 1;
    }

    HistorianConnection historian(hostAddress);

    try
    {
        // Metadata can be refreshed without connecting to SNAPdb server, only STTP connection is used
        historian.RefreshMetadata(sttpPort, [](const string& message) { cout << message << endl; });
        const MetadataCachePtr& metadata = historian.Metadata();

        // Display signal type summary
        map<string, int32_t> signalTypeCounts;

        for (const MeasurementRecordPtr& measurement : metadata->MeasurementRecords())
            signalTypeCounts[measurement->SignalTypeName]++;

        cout << endl << "Signal type summary:" << endl;

        for (const auto& [signalType, count] : signalTypeCounts)
            cout << "    " << signalType << ": " << count << endl;

        if (metadata->DeviceRecords().empty())
        {
            cout << "No devices found in metadata." << endl;
            return 0;
        }

        const DeviceRecordPtr device = deviceAcronym.empty() ? metadata->DeviceRecords()[0] : metadata->LookupDeviceByAcronym(deviceAcronym);

        if (device == nullptr)
        {
            cout << "Device \"" << deviceAcronym << "\" not found in metadata." << endl;
            return 1;
        }

        cout << endl << "Device \"" << device->Acronym << "\":" << endl;
        cout << "    Name: " << device->Name << endl;
        cout << "    Device ID: " << ToString(device->DeviceID) << endl;
        cout << "    Protocol: " << device->ProtocolName << endl;
        cout << "    Frames per Second: " << device->FramesPerSecond << endl;
        cout << "    Longitude, Latitude: " << ToString(device->Longitude) << ", " << ToString(device->Latitude) << endl;
        cout << "    Updated On: " << ToString(device->UpdatedOn) << endl;

        cout << endl << "    Measurements (" << device->Measurements.size() << "):" << endl;

        for (const MeasurementRecordPtr& measurement : device->Measurements)
        {
            cout << "        " << measurement->InstanceName << ":" << measurement->PointID << " [" << measurement->SignalTypeName << "] "
                 << measurement->PointTag << " (" << measurement->SignalReference << ")" << endl;
        }

        cout << endl << "    Phasors (" << device->Phasors.size() << "):" << endl;

        for (const PhasorRecordPtr& phasor : device->Phasors)
        {
            const MeasurementRecordPtr angle = phasor->AngleMeasurement();
            const MeasurementRecordPtr magnitude = phasor->MagnitudeMeasurement();

            cout << "        " << phasor->SourceIndex << ": " << phasor->Label << " [" << phasor->Type << phasor->Phase << "]";

            if (angle != nullptr && magnitude != nullptr)
                cout << " angle point ID " << angle->PointID << ", magnitude point ID " << magnitude->PointID;

            cout << endl;
        }
    }
    catch (const std::exception& ex)
    {
        cerr << "Failed to query openHistorian metadata: " << ex.what() << endl;
        return 1;
    }

    return 0;
}
