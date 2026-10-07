//******************************************************************************************************
//  SnapConnection.cpp - Gbtc
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

#include "SnapConnection.h"
#include "../Convert.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::snap;

SnapConnection::SnapConnection(const string& hostAddress, const uint16_t port) :
    m_hostName(Trim(hostAddress)),
    m_port(port),
    m_connectTimeout(10000),
    m_ioTimeout(0)
{
    // Parse optional port from host address, e.g., "localhost:38402" -- IPv6 addresses
    // with a port must use bracket notation, e.g., "[::1]:38402"
    const size_t portIndex = m_hostName.rfind(':');

    if (portIndex != string::npos && (m_hostName.find(':') == portIndex || m_hostName.starts_with('[')))
    {
        uint16_t parsedPort;

        if (!TryParseUInt16(m_hostName.substr(portIndex + 1), parsedPort) || parsedPort == 0)
            throw invalid_argument("Invalid port specified in SNAPdb host address \"" + hostAddress + "\"");

        m_hostName = m_hostName.substr(0, portIndex);
        m_port = parsedPort;
    }

    if (m_hostName.starts_with('[') && m_hostName.ends_with(']'))
        m_hostName = m_hostName.substr(1, m_hostName.size() - 2);

    if (m_hostName.empty())
        throw invalid_argument("SNAPdb host address cannot be empty");
}

SnapConnection::~SnapConnection()
{
    try
    {
        SnapConnection::Disconnect();
    }
    catch (...)
    {
        // Exceptions are not propagated from destructor
    }
}

string SnapConnection::HostAddress() const
{
    if (Contains(m_hostName, ":", false))
        return "[" + m_hostName + "]:" + to_string(m_port);

    return m_hostName + ":" + to_string(m_port);
}

const string& SnapConnection::HostName() const
{
    return m_hostName;
}

string SnapConnection::HostIPAddress() const
{
    if (m_socket == nullptr || !m_socket->IsOpen())
        return m_hostName;

    return m_socket->EndPoint().address().to_string();
}

uint16_t SnapConnection::HostPort() const
{
    return m_port;
}

bool SnapConnection::IsConnected() const
{
    return m_stream != nullptr && m_stream->IsOpen();
}

int32_t SnapConnection::GetConnectTimeout() const
{
    return m_connectTimeout;
}

void SnapConnection::SetConnectTimeout(const int32_t timeout)
{
    m_connectTimeout = timeout;
}

int32_t SnapConnection::GetIOTimeout() const
{
    return m_ioTimeout;
}

void SnapConnection::SetIOTimeout(const int32_t timeout)
{
    m_ioTimeout = timeout;

    if (m_socket != nullptr)
        m_socket->SetIOTimeout(timeout);
}

vector<string> SnapConnection::InstanceNames() const
{
    vector<string> instanceNames;
    instanceNames.reserve(m_instances.size());

    for (const DatabaseInfoPtr& info : m_instances)
        instanceNames.push_back(info->DatabaseName());

    return instanceNames;
}

const vector<DatabaseInfoPtr>& SnapConnection::Instances() const
{
    return m_instances;
}

bool SnapConnection::InstanceExists(const string& instanceName) const
{
    return GetInstanceInfo(instanceName) != nullptr;
}

DatabaseInfoPtr SnapConnection::GetInstanceInfo(const string& instanceName) const
{
    const string databaseName = ToUpper(Trim(instanceName));

    for (const DatabaseInfoPtr& info : m_instances)
    {
        if (info->DatabaseName() == databaseName)
            return info;
    }

    return nullptr;
}

const ClientDatabaseBasePtr& SnapConnection::OpenedInstance() const
{
    return m_instance;
}

void SnapConnection::Connect()
{
    Disconnect();

    try
    {
        m_socket = NewSharedPtr<SocketStream>();
        m_socket->Connect(m_hostName, m_port, m_connectTimeout);
        m_socket->SetIOTimeout(m_ioTimeout);

        m_stream = NewSharedPtr<BinaryStream>(m_socket);

        // Initialize SNAPdb client stream
        m_stream->WriteInt64(ProtocolCode);
        m_stream->WriteBoolean(false); // Use SSL
        m_stream->Flush();

        ServerResponse response = static_cast<ServerResponse>(m_stream->ReadByte());

        if (response == ServerResponse::UnknownProtocol)
            throw SnapException("SNAPdb client and server cannot agree on a protocol, this is commonly because they are running incompatible versions");

        ValidateExpectedResponse(response, ServerResponse::KnownProtocol);

        Authenticate(m_stream->ReadBoolean());

        response = ReadResponse(*m_stream);

        if (response == ServerResponse::UnknownProtocol)
            throw SnapException("SNAPdb client and server cannot agree on a protocol, this is commonly because they are running incompatible versions");

        ValidateExpectedResponse(response, ServerResponse::ConnectedToRoot);

        // Request list of available client database instances
        m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::GetAllDatabases));
        m_stream->Flush();

        ValidateExpectedReadResponse(*m_stream, ServerResponse::ListOfDatabases);

        const int32_t count = m_stream->ReadInt32();
        vector<DatabaseInfoPtr> instances;

        if (count < 0)
            throw SnapException("Invalid SNAPdb database count: " + to_string(count));

        instances.reserve(count);

        for (int32_t i = 0; i < count; i++)
            instances.push_back(boost::make_shared<DatabaseInfo>(*m_stream));

        m_instances = std::move(instances);
    }
    catch (...)
    {
        if (m_socket != nullptr)
            m_socket->Close();

        m_stream.reset();
        m_socket.reset();

        throw;
    }
}

void SnapConnection::Authenticate(const bool useSsl) const
{
    if (useSsl)
        throw SnapException("SNAPdb server is configured to require SSL. This version of the SNAPdb C++ API does not support SSL");

    m_stream->WriteByte(static_cast<uint8_t>(AuthenticationMode::None));
    m_stream->Flush();

    if (!m_stream->ReadBoolean())
        throw SnapException("SNAPdb authentication failed");

    // Server may provide a session resume ticket and secret, these are not currently used
    if (m_stream->ReadBoolean())
    {
        m_stream->ReadBytes(m_stream->ReadByte()); // Resume ticket
        m_stream->ReadBytes(m_stream->ReadByte()); // Session secret
    }
}

void SnapConnection::Disconnect()
{
    exception_ptr closeException = nullptr;

    try
    {
        CloseInstance();
    }
    catch (...)
    {
        closeException = current_exception();
    }

    if (m_stream != nullptr && m_stream->IsOpen())
    {
        try
        {
            m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::Disconnect));
            m_stream->Flush();
        }
        catch (...)
        {
            // Connection is closing, failures to send disconnect command are not reported
        }
    }

    if (m_socket != nullptr)
        m_socket->Close();

    m_stream.reset();
    m_socket.reset();

    if (closeException != nullptr)
        rethrow_exception(closeException);
}

DatabaseInfoPtr SnapConnection::ValidateOpenInstance(const string& instanceName, const Guid& keyTypeID, const Guid& valueTypeID)
{
    if (!IsConnected())
        throw SnapException("Cannot open SNAPdb instance \"" + instanceName + "\": not connected to SNAPdb server");

    if (m_instance != nullptr && !m_instance->IsDisposed())
        throw SnapException("SNAPdb instance \"" + m_instance->Info()->DatabaseName() + "\" is currently open. Only one SNAPdb instance can be open at once, call CloseInstance() first");

    DatabaseInfoPtr info = GetInstanceInfo(instanceName);

    if (info == nullptr)
        throw SnapException("Failed to find SNAPdb instance \"" + instanceName + "\"");

    if (info->KeyTypeID() != keyTypeID)
        throw SnapException("SNAPdb instance \"" + info->DatabaseName() + "\" key type {" + snapdb::ToString(info->KeyTypeID()) + "} does not match requested key type {" + snapdb::ToString(keyTypeID) + "}");

    if (info->ValueTypeID() != valueTypeID)
        throw SnapException("SNAPdb instance \"" + info->DatabaseName() + "\" value type {" + snapdb::ToString(info->ValueTypeID()) + "} does not match requested value type {" + snapdb::ToString(valueTypeID) + "}");

    return info;
}

void SnapConnection::ConnectToDatabase(const string& instanceName, const Guid& keyTypeID, const Guid& valueTypeID)
{
    m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::ConnectToDatabase));
    m_stream->WriteString(instanceName);
    m_stream->WriteGuid(keyTypeID);
    m_stream->WriteGuid(valueTypeID);
    m_stream->Flush();

    const ServerResponse response = ReadResponse(*m_stream);
    string message;

    switch (response)
    {
        case ServerResponse::DatabaseDoesNotExist:
            message = "SNAPdb server reports instance \"" + instanceName + "\" does not exist";
            break;
        case ServerResponse::DatabaseKeyUnknown:
            message = "SNAPdb server reports key type {" + snapdb::ToString(keyTypeID) + "} does not match type defined for instance \"" + instanceName + "\"";
            break;
        case ServerResponse::DatabaseValueUnknown:
            message = "SNAPdb server reports value type {" + snapdb::ToString(valueTypeID) + "} does not match type defined for instance \"" + instanceName + "\"";
            break;
        default:
            break;
    }

    if (!message.empty())
    {
        // Server closes connection after a failed database connection
        Disconnect();
        throw SnapException(message);
    }

    ValidateExpectedResponse(response, ServerResponse::SuccessfullyConnectedToDatabase);
}

void SnapConnection::CloseInstance()
{
    if (m_instance == nullptr)
        return;

    const ClientDatabaseBasePtr instance = m_instance;
    m_instance.reset();
    instance->Dispose();
}
