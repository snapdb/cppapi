//******************************************************************************************************
//  WriteTest.cpp - Gbtc
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
using namespace snapdb::openhistorian;

// Sample application to demonstrate writing data to the openHistorian.
//
// Application connects to the openHistorian, opens the first client database
// instance, then writes a single test measurement value for point ID 1, or the
// specified point ID, using the current time.
int main(int argc, char* argv[])
{
    // Get optional host address, e.g., "localhost:38402", and point ID
    const string hostAddress = argc > 1 ? argv[1] : "localhost";
    uint64_t pointID = 1;

    if (argc > 2 && !TryParseUInt64(argv[2], pointID))
    {
        cerr << "Invalid point ID: " << argv[2] << endl;
        return 1;
    }

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

        HistorianKey key;
        key.PointID = pointID;
        key.SetDateTime(UtcNow());

        HistorianValue value;
        value.SetSingle(1000.98F);
        value.SetQuality(QualityFlags::WarningHigh);

        cout << "Writing test point " << key.ToString() << " = " << value.ToString() << "..." << endl;
        instance->Write(key, value);
    }
    catch (const std::exception& ex)
    {
        cerr << "Failed to write to openHistorian: " << ex.what() << endl;
        return 1;
    }

    if (historian.IsConnected())
        cout << "Disconnecting from openHistorian" << endl;

    historian.Disconnect();

    return 0;
}
