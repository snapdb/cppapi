//******************************************************************************************************
//  SnapTypes.h - Gbtc
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
#include "../SnapException.h"
#include "../io/BinaryStream.h"
#include <concepts>

namespace snapdb::snap
{
    // Defines the default SNAPdb server port.
    constexpr uint16_t DefaultPort = 38402;

    // Defines the code sent by a client to identify the SNAPdb protocol.
    constexpr int64_t ProtocolCode = 0x2BA517361121LL;

    // Defines SNAPdb server commands.
    enum class ServerCommand : uint8_t
    {
        ConnectToDatabase = 0,
        DisconnectDatabase = 1,
        Disconnect = 2,
        Read = 3,
        CancelRead = 4,
        Write = 5,
        SetEncodingMethod = 6,
        GetAllDatabases = 7
    };

    // Defines SNAPdb server responses.
    enum class ServerResponse : uint8_t
    {
        UnhandledException = 0,
        UnknownProtocol = 1,
        ConnectedToRoot = 2,
        ListOfDatabases = 3,
        DatabaseDoesNotExist = 4,
        DatabaseKeyUnknown = 5,
        DatabaseValueUnknown = 6,
        SuccessfullyConnectedToDatabase = 7,
        GoodBye = 8,
        UnknownCommand = 9,
        UnknownEncodingMethod = 10,
        EncodingMethodAccepted = 11,
        DatabaseDisconnected = 12,
        UnknownDatabaseCommand = 13,
        UnknownOrCorruptSeekFilter = 14,
        UnknownOrCorruptMatchFilter = 15,
        UnknownOrCorruptReaderOptions = 16,
        SerializingPoints = 17,
        ErrorWhileReading = 18,
        CanceledRead = 19,
        ReadComplete = 20,
        ServerNameTooLong = 21,
        ServerNameDoesNotMatch = 22,
        RequiresLogin = 23,
        KnownProtocol = 24,
        AuthenticationFailed = 25
    };

    // Defines SNAPdb authentication modes.
    enum class AuthenticationMode : uint8_t
    {
        None = 1,
        Srp = 2,
        Scram = 3,
        Integrated = 4,
        Certificate = 5,
        ResumeSession = 255
    };

    std::string ToString(ServerCommand command);
    std::string ToString(ServerResponse response);

    // Reads next server response from stream. Throws when server reports an unhandled exception.
    ServerResponse ReadResponse(io::BinaryStream& stream);

    // Validates that a server response is the expected response. Throws when it is not.
    void ValidateExpectedResponse(ServerResponse response, ServerResponse expectedResponse);

    // Validates that a server response is one of the expected responses. Throws when it is not.
    void ValidateExpectedResponses(ServerResponse response, std::initializer_list<ServerResponse> expectedResponses);

    // Reads next server response from stream and validates that it is the expected response.
    void ValidateExpectedReadResponse(io::BinaryStream& stream, ServerResponse expectedResponse);

    // Defines the requirements for a type to be used as a key or value in a SNAPdb sorted tree.
    // Types are expected to define a static `TypeID` Guid and `Size` constant along with methods
    // to set min/max values, clear, read, write, copy and compare. See `HistorianKey` for example.
    template<class T>
    concept SnapType = std::default_initializable<T> && requires(T value, const T constValue, T& destination, io::BinaryStream& stream)
    {
        { T::TypeID } -> std::convertible_to<Guid>;
        { T::Size } -> std::convertible_to<uint32_t>;
        value.SetMin();
        value.SetMax();
        value.Clear();
        value.Read(stream);
        constValue.Write(stream);
        constValue.CopyTo(destination);
        { constValue.CompareTo(constValue) } -> std::convertible_to<int32_t>;
    };

    // Defines the requirements for a SNAPdb key type that includes a timestamp and point ID,
    // i.e., a type that can be used with timestamp and point ID based filters.
    template<class T>
    concept TimestampPointIDType = SnapType<T> && requires(T value)
    {
        { value.Timestamp } -> std::convertible_to<uint64_t>;
        { value.PointID } -> std::convertible_to<uint64_t>;
    };
}
