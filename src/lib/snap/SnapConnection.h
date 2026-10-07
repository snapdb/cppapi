//******************************************************************************************************
//  SnapConnection.h - Gbtc
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

#include "SnapClientDatabase.h"
#include "../io/SocketStream.h"

namespace snapdb::snap
{
    // Represents a connection to a SNAPdb server. Once connected, one client database
    // instance can be opened at a time to read and write key/value data. Note that the
    // connection and its client database instances are not thread-safe.
    class SnapConnection
    {
    private:
        std::string m_hostName;
        uint16_t m_port;
        int32_t m_connectTimeout;
        int32_t m_ioTimeout;
        io::SocketStreamPtr m_socket;
        io::BinaryStreamPtr m_stream;
        std::vector<DatabaseInfoPtr> m_instances;
        ClientDatabaseBasePtr m_instance;

        void Authenticate(bool useSsl) const;
        DatabaseInfoPtr ValidateOpenInstance(const std::string& instanceName, const Guid& keyTypeID, const Guid& valueTypeID);
        void ConnectToDatabase(const std::string& instanceName, const Guid& keyTypeID, const Guid& valueTypeID);

    public:
        // Creates a new SNAPdb connection. Host address can include port, e.g., "localhost:38402",
        // in which case the port parameter is ignored.
        SnapConnection(const std::string& hostAddress = "localhost", uint16_t port = DefaultPort);
        virtual ~SnapConnection();

        SnapConnection(const SnapConnection&) = delete;
        SnapConnection& operator=(const SnapConnection&) = delete;

        // Gets SNAPdb server host address and port, e.g., "localhost:38402".
        std::string HostAddress() const;

        // Gets SNAPdb server host name, e.g., "localhost".
        const std::string& HostName() const;

        // Gets SNAPdb server IP address, e.g., "127.0.0.1". Returns host name when not connected.
        std::string HostIPAddress() const;

        // Gets SNAPdb server port, e.g., 38402.
        uint16_t HostPort() const;

        // Gets flag that determines if connected to SNAPdb server.
        bool IsConnected() const;

        // Gets the connection timeout, in milliseconds. A value of zero or less means wait indefinitely.
        int32_t GetConnectTimeout() const;

        // Sets the connection timeout, in milliseconds. A value of zero or less means wait indefinitely.
        void SetConnectTimeout(int32_t timeout);

        // Gets the socket read/write timeout, in milliseconds. A value of zero or less means wait indefinitely.
        int32_t GetIOTimeout() const;

        // Sets the socket read/write timeout, in milliseconds. A value of zero or less means wait indefinitely.
        // Note that a timeout closes the connection since stream state is unknown after a partial read or write.
        void SetIOTimeout(int32_t timeout);

        // Gets the names of the client database instances available on the SNAPdb server.
        std::vector<std::string> InstanceNames() const;

        // Gets details for all client database instances available on the SNAPdb server.
        const std::vector<DatabaseInfoPtr>& Instances() const;

        // Determines if the named client database instance exists on the SNAPdb server.
        bool InstanceExists(const std::string& instanceName) const;

        // Gets details about a SNAPdb client database instance, or nullptr if instance does not exist.
        DatabaseInfoPtr GetInstanceInfo(const std::string& instanceName) const;

        // Gets currently open client database instance, if any.
        const ClientDatabaseBasePtr& OpenedInstance() const;

        // Connects to the SNAPdb server.
        virtual void Connect();

        // Disconnects from the SNAPdb server.
        virtual void Disconnect();

        // Opens a connection to a SNAPdb client database instance. If successful, client database instance
        // is returned and can be used to read and write key/value data. Only one instance can be open at a
        // time. When no encoding definition is specified, the first encoding supported by both the server
        // and the client is used.
        template<SnapType TKey, SnapType TValue>
        SnapClientDatabasePtr<TKey, TValue> OpenInstance(const std::string& instanceName, const EncodingDefinitionPtr& definition = nullptr)
        {
            const DatabaseInfoPtr info = ValidateOpenInstance(instanceName, TKey::TypeID, TValue::TypeID);
            const EncodingDefinition* selectedDefinition = definition.get();

            if (selectedDefinition == nullptr)
            {
                // Select first encoding supported by both server and client
                for (const EncodingDefinition& supportedEncoding : info->SupportedEncodings())
                {
                    if (EncoderLibrary<TKey, TValue>::IsSupported(supportedEncoding))
                    {
                        selectedDefinition = &supportedEncoding;
                        break;
                    }
                }

                // Fixed size encoding is always supported by server
                if (selectedDefinition == nullptr)
                    selectedDefinition = &EncodingDefinition::FixedSizeCombinedEncoding;
            }
            else if (!EncoderLibrary<TKey, TValue>::IsSupported(*selectedDefinition))
            {
                throw SnapException("Encoding method " + selectedDefinition->ToString() + " is not registered for SNAPdb instance key/value types");
            }

            ConnectToDatabase(info->DatabaseName(), TKey::TypeID, TValue::TypeID);

            SnapClientDatabasePtr<TKey, TValue> instance = NewSharedPtr<SnapClientDatabase<TKey, TValue>>(m_stream, info);
            m_instance = instance;

            instance->SetEncodingDefinition(*selectedDefinition);

            return instance;
        }

        // Closes currently open client database instance, if any.
        void CloseInstance();
    };

    typedef SharedPtr<SnapConnection> SnapConnectionPtr;
}
