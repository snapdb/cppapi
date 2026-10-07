//******************************************************************************************************
//  HistorianConnection.h - Gbtc
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

#include "../snap/SnapConnection.h"
#include "HistorianKey.h"
#include "HistorianValue.h"
#include "HistorianKeyValueEncoder.h"
#include "MetadataCache.h"

namespace snapdb::openhistorian
{
    // Represents a SNAPdb client database instance for the openHistorian, i.e.,
    // a database with `HistorianKey` keys and `HistorianValue` values.
    typedef snap::SnapClientDatabase<HistorianKey, HistorianValue> HistorianInstance;
    typedef SharedPtr<HistorianInstance> HistorianInstancePtr;

    // Represents a stream of historian key/value pairs, e.g., from a read.
    typedef snap::TreeStream<HistorianKey, HistorianValue> HistorianReader;
    typedef SharedPtr<HistorianReader> HistorianReaderPtr;

    // Represents a bulk writer for historian key/value pairs.
    typedef snap::BulkWriter<HistorianKey, HistorianValue> HistorianBulkWriter;
    typedef SharedPtr<HistorianBulkWriter> HistorianBulkWriterPtr;

    // Registers openHistorian SNAPdb key/value types and encoders. This is called automatically
    // by `HistorianConnection`, but should be called before using a generic `SnapConnection`
    // with openHistorian types. Function is safe to call multiple times.
    void RegisterTypes();

    // Defines API functionality for connecting to an openHistorian instance, then reading and
    // writing measurement data from the instance. Metadata can be queried from the openHistorian
    // using its STTP publisher. Note that the connection is not thread-safe.
    class HistorianConnection final : public snap::SnapConnection
    {
    private:
        MetadataCachePtr m_metadata;
        int32_t m_metadataTimeout;

    public:
        // Defines the default openHistorian STTP port used for metadata queries.
        static constexpr uint16_t DefaultSttpPort = 7175;

        // Creates a new openHistorian connection. Host address can include port, e.g.,
        // "localhost:38402", in which case the port parameter is ignored.
        HistorianConnection(const std::string& hostAddress = "localhost", uint16_t port = snap::DefaultPort);

        // Opens a connection to an openHistorian client database instance. If successful, client database
        // instance is returned and can be used to read and write measurement data. Only one instance can
        // be open at a time.
        HistorianInstancePtr OpenInstance(const std::string& instanceName, const snap::EncodingDefinitionPtr& definition = nullptr);

        // Gets the openHistorian metadata cache from last metadata refresh, or nullptr if metadata
        // has not been refreshed.
        const MetadataCachePtr& Metadata() const;

        // Gets the metadata refresh timeout, in milliseconds. A value of zero or less means wait indefinitely.
        int32_t GetMetadataTimeout() const;

        // Sets the metadata refresh timeout, in milliseconds. A value of zero or less means wait indefinitely.
        void SetMetadataTimeout(int32_t timeout);

        // Requests updated metadata from openHistorian using its STTP publisher, then caches the parsed metadata.
        // Optional log output function receives progress messages. Returns total number of metadata records.
        uint64_t RefreshMetadata(uint16_t sttpPort = DefaultSttpPort, const std::function<void(const std::string&)>& logOutput = nullptr);
    };

    typedef SharedPtr<HistorianConnection> HistorianConnectionPtr;
}
