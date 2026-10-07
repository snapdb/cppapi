//******************************************************************************************************
//  HistorianConnection.cpp - Gbtc
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

#include "HistorianConnection.h"
#include "../Convert.h"
#include <iomanip>
#include <mutex>

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::snap;
using namespace snapdb::openhistorian;

namespace
{
    // STTP server commands used for metadata queries
    constexpr uint8_t MetadataRefreshCommand = 0x01;
    constexpr uint8_t DefineOperationalModesCommand = 0x06;

    // STTP server responses
    constexpr uint8_t SucceededResponse = 0x80;

    // STTP connection is only used to get metadata, i.e., no subscription, so operational modes are:
    // OperationalModes.CompressMetadata | CompressionModes.GZip | OperationalEncoding.UTF8 | (OperationalModes.VersionMask & 1)
    constexpr uint32_t OperationalModes = 0x80000221U;

    // STTP response header size: response code (1 byte), command code (1 byte), payload length (4 bytes)
    constexpr uint32_t ResponseHeaderSize = 6U;

    // Maximum allowed STTP response packet size
    constexpr uint32_t MaxPacketSize = 512U * 1024U * 1024U;

    string FormatElapsed(const chrono::steady_clock::time_point& startTime)
    {
        const float64_t elapsed = chrono::duration<float64_t>(chrono::steady_clock::now() - startTime).count();
        stringstream stream;
        stream << fixed << setprecision(2) << elapsed;
        return stream.str();
    }

    string FormatCount(const uint64_t value)
    {
        string digits = to_string(value);

        for (int32_t i = static_cast<int32_t>(digits.size()) - 3; i > 0; i -= 3)
            digits.insert(i, ",");

        return digits;
    }

    vector<uint8_t> Decompress(const vector<uint8_t>& buffer)
    {
        const MemoryStream memoryStream(buffer);
        StreamBuffer streamBuffer;
        vector<uint8_t> result;

        streamBuffer.push(GZipDecompressor(), StreamFilterBufferSize);
        streamBuffer.push(memoryStream);

        CopyStream(&streamBuffer, result);

        return result;
    }
}

void openhistorian::RegisterTypes()
{
    static once_flag registered;

    call_once(registered, []
    {
        Library::RegisterType<HistorianKey>("HistorianKey");
        Library::RegisterType<HistorianValue>("HistorianValue");

        EncoderLibrary<HistorianKey, HistorianValue>::RegisterEncoder(HistorianKeyValueEncoder::SnapTypeID, []
        {
            return NewSharedPtr<HistorianKeyValueEncoder>();
        });
    });
}

HistorianConnection::HistorianConnection(const string& hostAddress, const uint16_t port) :
    SnapConnection(hostAddress, port),
    m_metadataTimeout(30000)
{
    RegisterTypes();
}

HistorianInstancePtr HistorianConnection::OpenInstance(const string& instanceName, const EncodingDefinitionPtr& definition)
{
    return SnapConnection::OpenInstance<HistorianKey, HistorianValue>(instanceName, definition);
}

const MetadataCachePtr& HistorianConnection::Metadata() const
{
    return m_metadata;
}

int32_t HistorianConnection::GetMetadataTimeout() const
{
    return m_metadataTimeout;
}

void HistorianConnection::SetMetadataTimeout(const int32_t timeout)
{
    m_metadataTimeout = timeout;
}

uint64_t HistorianConnection::RefreshMetadata(const uint16_t sttpPort, const function<void(const string&)>& logOutput)
{
    const auto log = [&logOutput](const string& message)
    {
        if (logOutput)
            logOutput(message);
    };

    const SocketStreamPtr socket = NewSharedPtr<SocketStream>();
    BinaryStream stream(socket, false); // STTP uses big-endian encoding

    log("Requesting metadata from openHistorian...");
    chrono::steady_clock::time_point startTime = chrono::steady_clock::now();

    socket->Connect(HostIPAddress(), sttpPort, GetConnectTimeout());
    socket->SetIOTimeout(m_metadataTimeout);

    // Establish operational modes for STTP connection
    stream.WriteInt32(5); // Payload aware buffer length
    stream.WriteByte(DefineOperationalModesCommand);
    stream.WriteUInt32(OperationalModes);

    // Request metadata refresh
    stream.WriteInt32(1); // Payload aware buffer length
    stream.WriteByte(MetadataRefreshCommand);
    stream.Flush();

    while (true)
    {
        // Read payload aware buffer length
        const uint32_t packetSize = stream.ReadUInt32();

        if (packetSize > MaxPacketSize)
            throw SnapException("STTP response packet size " + to_string(packetSize) + " exceeds maximum allowed size");

        const vector<uint8_t> packet = stream.ReadBytes(packetSize);

        if (packetSize < 2)
            continue;

        if (packetSize < ResponseHeaderSize)
            throw SnapException("Invalid STTP response packet size: " + to_string(packetSize));

        const uint8_t responseCode = packet[0];
        const uint8_t commandCode = packet[1];

        // Other commands can come spontaneously, like NoOp, only interested in metadata refresh response
        if (commandCode != MetadataRefreshCommand)
        {
            log("Ignoring response " + ToHex(responseCode) + " received for command " + ToHex(commandCode) + "...");
            continue;
        }

        const uint32_t length = std::min(EndianConverter::ToBigEndian<uint32_t>(packet.data(), 2), packetSize - ResponseHeaderSize);

        const vector<uint8_t> payload(packet.begin() + ResponseHeaderSize, packet.begin() + ResponseHeaderSize + length);

        if (responseCode != SucceededResponse)
            throw SnapException("Failure code received in response to STTP metadata refresh request: " + string(payload.begin(), payload.end()));

        socket->Close();

        log("Received " + FormatCount(length) + " bytes of metadata in " + FormatElapsed(startTime) + " seconds. Decompressing...");
        startTime = chrono::steady_clock::now();

        vector<uint8_t> metadataXml;

        try
        {
            metadataXml = Decompress(payload);
        }
        catch (const std::exception& ex)
        {
            throw SnapException("Failed to decompress metadata: " + string(ex.what()));
        }

        log("Decompressed " + FormatCount(metadataXml.size()) + " bytes of metadata in " + FormatElapsed(startTime) + " seconds. Parsing...");
        startTime = chrono::steady_clock::now();

        m_metadata = NewSharedPtr<MetadataCache>(string(metadataXml.begin(), metadataXml.end()));

        const uint64_t measurementRecordCount = m_metadata->MeasurementRecords().size();
        const uint64_t deviceRecordCount = m_metadata->DeviceRecords().size();
        const uint64_t phasorRecordCount = m_metadata->PhasorRecords().size();
        const uint64_t recordCount = measurementRecordCount + deviceRecordCount + phasorRecordCount;

        log("Parsed " + FormatCount(recordCount) + " metadata records in " + FormatElapsed(startTime) + " seconds.");
        log("    Discovered:");
        log("        " + FormatCount(measurementRecordCount) + " measurement records");
        log("        " + FormatCount(deviceRecordCount) + " device records, and");
        log("        " + FormatCount(phasorRecordCount) + " phasor records");

        return recordCount;
    }
}
