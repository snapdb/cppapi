//******************************************************************************************************
//  SnapTypes.cpp - Gbtc
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

#include "SnapTypes.h"
#include "../Convert.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::snap;

string snap::ToString(const ServerCommand command)
{
    switch (command)
    {
        case ServerCommand::ConnectToDatabase:
            return "ConnectToDatabase";
        case ServerCommand::DisconnectDatabase:
            return "DisconnectDatabase";
        case ServerCommand::Disconnect:
            return "Disconnect";
        case ServerCommand::Read:
            return "Read";
        case ServerCommand::CancelRead:
            return "CancelRead";
        case ServerCommand::Write:
            return "Write";
        case ServerCommand::SetEncodingMethod:
            return "SetEncodingMethod";
        case ServerCommand::GetAllDatabases:
            return "GetAllDatabases";
        default:
            return ToHex(static_cast<uint8_t>(command));
    }
}

string snap::ToString(const ServerResponse response)
{
    switch (response)
    {
        case ServerResponse::UnhandledException:
            return "UnhandledException";
        case ServerResponse::UnknownProtocol:
            return "UnknownProtocol";
        case ServerResponse::ConnectedToRoot:
            return "ConnectedToRoot";
        case ServerResponse::ListOfDatabases:
            return "ListOfDatabases";
        case ServerResponse::DatabaseDoesNotExist:
            return "DatabaseDoesNotExist";
        case ServerResponse::DatabaseKeyUnknown:
            return "DatabaseKeyUnknown";
        case ServerResponse::DatabaseValueUnknown:
            return "DatabaseValueUnknown";
        case ServerResponse::SuccessfullyConnectedToDatabase:
            return "SuccessfullyConnectedToDatabase";
        case ServerResponse::GoodBye:
            return "GoodBye";
        case ServerResponse::UnknownCommand:
            return "UnknownCommand";
        case ServerResponse::UnknownEncodingMethod:
            return "UnknownEncodingMethod";
        case ServerResponse::EncodingMethodAccepted:
            return "EncodingMethodAccepted";
        case ServerResponse::DatabaseDisconnected:
            return "DatabaseDisconnected";
        case ServerResponse::UnknownDatabaseCommand:
            return "UnknownDatabaseCommand";
        case ServerResponse::UnknownOrCorruptSeekFilter:
            return "UnknownOrCorruptSeekFilter";
        case ServerResponse::UnknownOrCorruptMatchFilter:
            return "UnknownOrCorruptMatchFilter";
        case ServerResponse::UnknownOrCorruptReaderOptions:
            return "UnknownOrCorruptReaderOptions";
        case ServerResponse::SerializingPoints:
            return "SerializingPoints";
        case ServerResponse::ErrorWhileReading:
            return "ErrorWhileReading";
        case ServerResponse::CanceledRead:
            return "CanceledRead";
        case ServerResponse::ReadComplete:
            return "ReadComplete";
        case ServerResponse::ServerNameTooLong:
            return "ServerNameTooLong";
        case ServerResponse::ServerNameDoesNotMatch:
            return "ServerNameDoesNotMatch";
        case ServerResponse::RequiresLogin:
            return "RequiresLogin";
        case ServerResponse::KnownProtocol:
            return "KnownProtocol";
        case ServerResponse::AuthenticationFailed:
            return "AuthenticationFailed";
        default:
            return ToHex(static_cast<uint8_t>(response));
    }
}

ServerResponse snap::ReadResponse(BinaryStream& stream)
{
    const ServerResponse response = static_cast<ServerResponse>(stream.ReadByte());

    if (response == ServerResponse::UnhandledException)
        throw SnapException("SNAPdb server unhandled exception: " + stream.ReadString());

    return response;
}

void snap::ValidateExpectedResponse(const ServerResponse response, const ServerResponse expectedResponse)
{
    if (response != expectedResponse)
        throw SnapException("Unexpected SNAPdb server response: " + ToString(response) + ", expected " + ToString(expectedResponse));
}

void snap::ValidateExpectedResponses(const ServerResponse response, const initializer_list<ServerResponse> expectedResponses)
{
    for (const ServerResponse expectedResponse : expectedResponses)
    {
        if (response == expectedResponse)
            return;
    }

    throw SnapException("Unexpected SNAPdb server response: " + ToString(response));
}

void snap::ValidateExpectedReadResponse(BinaryStream& stream, const ServerResponse expectedResponse)
{
    ValidateExpectedResponse(ReadResponse(stream), expectedResponse);
}
