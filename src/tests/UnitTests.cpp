//******************************************************************************************************
//  UnitTests.cpp - Gbtc
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

#include "../lib/openhistorian/HistorianConnection.h"
#include "../lib/io/BufferStream.h"
#include "../lib/Convert.h"
#include "../lib/Version.h"
#include <iostream>
#include <iomanip>

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::snap;
using namespace snapdb::openhistorian;

// Unit tests for the SNAPdb C++ API that do not require a running openHistorian. Expected
// encodings were generated using the openHistorian Python API as a reference implementation.

namespace
{
    int32_t Failures = 0;
    int32_t Passes = 0;

    void Check(const bool condition, const string& description)
    {
        if (condition)
        {
            Passes++;
            return;
        }

        Failures++;
        cout << "    FAIL: " << description << endl;
    }

    void BeginTest(const string& name)
    {
        cout << name << "..." << endl;
    }

    string ToHexString(const vector<uint8_t>& buffer)
    {
        stringstream stream;

        for (const uint8_t value : buffer)
            stream << hex << setw(2) << setfill('0') << static_cast<int32_t>(value);

        return stream.str();
    }

    vector<uint8_t> FromHexString(const string& value)
    {
        vector<uint8_t> buffer;

        for (size_t i = 0; i + 1 < value.size(); i += 2)
            buffer.push_back(static_cast<uint8_t>(stoi(value.substr(i, 2), nullptr, 16)));

        return buffer;
    }

    // Captures bytes written by the provided function
    template<class TWriter>
    string Capture(TWriter writer)
    {
        const BufferStreamPtr buffer = NewSharedPtr<BufferStream>();
        BinaryStream stream(buffer);
        writer(stream);
        stream.Flush();
        return ToHexString(buffer->Buffer());
    }

    // Defines a stream with separate input, e.g., simulated server responses, and output, e.g., client requests
    class DuplexStream final : public Stream
    {
    public:
        BufferStream Input;
        BufferStream Output;

        DuplexStream(vector<uint8_t> input) : Input(std::move(input))
        {
        }

        uint32_t Read(uint8_t* buffer, const uint32_t offset, const uint32_t count) override
        {
            return Input.Read(buffer, offset, count);
        }

        void Write(const uint8_t* buffer, const uint32_t offset, const uint32_t count) override
        {
            Output.Write(buffer, offset, count);
        }
    };

    struct TestPoint
    {
        uint64_t Timestamp, PointID, EntryNumber, Value1, Value2, Value3;
    };

    const vector<TestPoint> TestPoints =
    {
        { 638640000000000000ULL, 1ULL, 0ULL, 0x447A3EB8ULL, 0ULL, 0ULL },
        { 638640000000000000ULL, 2ULL, 0ULL, 0x447A3EB8ULL, 0ULL, 0ULL },
        { 638640000000000000ULL, 3ULL, 0ULL, 0ULL, 0ULL, 0ULL },
        { 638640000000000000ULL, 200ULL, 0ULL, 5ULL, 0ULL, 64ULL },
        { 638640000003333333ULL, 200ULL, 7ULL, 0x400921FB54442D18ULL, 0ULL, 0x100000001ULL },
        { 638640000003333333ULL, 9000000001ULL, 0ULL, 1ULL, 0xFFFFFFFFFFFFFFFFULL, 0ULL },
        { 638640000006666666ULL, 1ULL, 0ULL, 0ULL, 0ULL, 0ULL }
    };

    const string HistorianEncodingHex = "c88080ec9aa4f6b9ee0801b83e7a4443b83e7a440189cb010500000040000000f2d5b9db060007182d4454fb21094001000000010000008cc9b5c4c32101000000ffffffffffffffffc0ffcab50280b4c4c321ff";

    void ToKeyValue(const TestPoint& point, HistorianKey& key, HistorianValue& value)
    {
        key.Timestamp = point.Timestamp;
        key.PointID = point.PointID;
        key.EntryNumber = point.EntryNumber;
        value.Value1 = point.Value1;
        value.Value2 = point.Value2;
        value.Value3 = point.Value3;
    }

    bool Matches(const TestPoint& point, const HistorianKey& key, const HistorianValue& value)
    {
        return key.Timestamp == point.Timestamp && key.PointID == point.PointID && key.EntryNumber == point.EntryNumber &&
            value.Value1 == point.Value1 && value.Value2 == point.Value2 && value.Value3 == point.Value3;
    }
}

void TestEncoding7Bit()
{
    BeginTest("7-bit encoding");

    Check(Capture([](BinaryStream& stream)
    {
        for (const uint64_t value : initializer_list<uint64_t>{ 0ULL, 127ULL, 128ULL, 16383ULL, 16384ULL, 4294967295ULL, 1ULL << 56, UInt64::MaxValue })
            stream.Write7BitUInt64(value);
    }) == "007f8001ff7f808001ffffffff0f808080808080808001ffffffffffffffffff", "7-bit 64-bit encoding matches reference");

    Check(Capture([](BinaryStream& stream)
    {
        for (const uint32_t value : initializer_list<uint32_t>{ 0U, 127U, 128U, 1U << 28, UInt32::MaxValue })
            stream.Write7BitUInt32(value);
    }) == "007f80018080808001ffffffff0f", "7-bit 32-bit encoding matches reference");

    // Round trip a range of values across all encoded sizes
    const BufferStreamPtr buffer = NewSharedPtr<BufferStream>();
    BinaryStream stream(buffer);
    vector<uint64_t> values;

    for (int32_t shift = 0; shift < 64; shift++)
    {
        values.push_back(1ULL << shift);
        values.push_back((1ULL << shift) - 1);
        values.push_back(~0ULL >> shift);
    }

    for (const uint64_t value : values)
    {
        stream.Write7BitUInt64(value);
        stream.Write7BitUInt32(static_cast<uint32_t>(value));
    }

    stream.Flush();

    bool roundTrip = true;

    for (const uint64_t value : values)
    {
        if (stream.Read7BitUInt64() != value || stream.Read7BitUInt32() != static_cast<uint32_t>(value))
            roundTrip = false;
    }

    Check(roundTrip, "7-bit values round trip");

    for (const uint64_t value : values)
    {
        uint8_t bytes[Encoding7Bit::MaxUInt64Size];

        if (Encoding7Bit::GetSize(value) != Encoding7Bit::WriteUInt64(bytes, value))
            roundTrip = false;
    }

    Check(roundTrip, "7-bit encoded size calculation matches written size");
}

void TestBinaryStream()
{
    BeginTest("Binary stream");

    Check(Capture([](BinaryStream& stream) { stream.WriteGuid(HistorianKey::TypeID); }) == "1bd42765049dfa4b813305273d521d46", "Guid uses .NET byte order");
    Check(Capture([](BinaryStream& stream) { stream.WriteString("PPA"); }) == "03505041", "string is 7-bit length prefixed UTF-8");
    Check(Capture([](BinaryStream& stream) { stream.WriteInt32(0x01020304); }) == "04030201", "integers are little-endian by default");

    const BufferStreamPtr bigEndianBuffer = NewSharedPtr<BufferStream>();
    BinaryStream bigEndian(bigEndianBuffer, false);
    bigEndian.WriteInt32(0x01020304);
    bigEndian.Flush();
    Check(ToHexString(bigEndianBuffer->Buffer()) == "01020304", "integers are big-endian when requested");

    // Round trip values, including writes that exceed the I/O buffer size
    const BufferStreamPtr buffer = NewSharedPtr<BufferStream>();
    BinaryStream stream(buffer);
    const Guid guid = NewGuid();
    const string text = "SNAPdb C++ API \xE2\x9C\x93";
    const vector<uint8_t> large(BinaryStream::IOBufferSize * 2 + 17, 0xA5);

    stream.WriteByte(0xFE);
    stream.WriteBoolean(true);
    stream.WriteInt16(-2);
    stream.WriteUInt16(65000);
    stream.WriteInt32(-123456789);
    stream.WriteUInt32(4000000000U);
    stream.WriteInt64(-1234567890123LL);
    stream.WriteUInt64(UInt64::MaxValue);
    stream.WriteGuid(guid);
    stream.WriteString(text);
    stream.WriteBuffer(large);

    for (int32_t i = 0; i < 20000; i++)
        stream.WriteUInt64(static_cast<uint64_t>(i) * 31ULL);

    stream.Flush();

    Check(stream.ReadByte() == 0xFE, "byte round trip");
    Check(stream.ReadBoolean(), "boolean round trip");
    Check(stream.ReadInt16() == -2, "int16 round trip");
    Check(stream.ReadUInt16() == 65000, "uint16 round trip");
    Check(stream.ReadInt32() == -123456789, "int32 round trip");
    Check(stream.ReadUInt32() == 4000000000U, "uint32 round trip");
    Check(stream.ReadInt64() == -1234567890123LL, "int64 round trip");
    Check(stream.ReadUInt64() == UInt64::MaxValue, "uint64 round trip");
    Check(stream.ReadGuid() == guid, "Guid round trip");
    Check(stream.ReadString() == text, "string round trip");
    Check(stream.ReadBuffer() == large, "large buffer round trip");

    bool valuesMatch = true;

    for (int32_t i = 0; i < 20000; i++)
    {
        if (stream.ReadUInt64() != static_cast<uint64_t>(i) * 31ULL)
            valuesMatch = false;
    }

    Check(valuesMatch, "values spanning buffer boundaries round trip");

    bool endOfStream = false;

    try
    {
        stream.ReadByte();
    }
    catch (const SnapException&)
    {
        endOfStream = true;
    }

    Check(endOfStream, "reading past end of stream throws");
}

void TestHistorianTypes()
{
    BeginTest("Historian key and value types");

    static_assert(TimestampPointIDType<HistorianKey>);
    static_assert(SnapType<HistorianValue>);

    HistorianKey key;
    key.SetDateTime(ParseTimestamp("2026-10-07 12:34:56.789"));
    key.PointID = 1234;

    Check(key.Timestamp == static_cast<uint64_t>(ToTicks(ParseTimestamp("2026-10-07 12:34:56.789"))), "key timestamp assigned from date/time");
    Check(key.ToString() == "1234 @ 2026-10-07 12:34:56.789", "key string format: " + key.ToString());
    Check(key.GetDateTime() == ParseTimestamp("2026-10-07 12:34:56.789"), "key date/time round trip");

    HistorianKey other(1234, key.Timestamp + 1);
    Check(key.CompareTo(other) < 0 && other.CompareTo(key) > 0 && key.CompareTo(key) == 0, "key comparison orders by timestamp");

    HistorianValue value(1000.98F, QualityFlags::WarningHigh);
    Check(value.Value1 == 0x447A3EB8ULL, "single value stored in Value1");
    Check(value.AsSingle() == 1000.98F, "single value round trip");
    Check(value.AsQuality() == QualityFlags::WarningHigh, "quality round trip");
    Check(value.ToString() == "1000.980 [WarningHigh]", "value string format: " + value.ToString());
    Check(ToString(QualityFlags::WarningHigh | QualityFlags::CalculatedValue) == "WarningHigh|CalculatedValue", "quality flags string format");
    Check(ToString(QualityFlags::Normal) == "Normal", "normal quality string format");

    value.SetDouble(3.14159265358979);
    Check(value.AsDouble() == 3.14159265358979 && value.AsSingle() != 3.14159265358979F, "double value round trip");

    value.SetString("Hello SNAPdb!");
    Check(value.AsString() == "Hello SNAPdb!", "string value round trip");

    bool overflow = false;

    try
    {
        value.SetString("This string is too long");
    }
    catch (const overflow_error&)
    {
        overflow = true;
    }

    Check(overflow, "string value longer than 16 characters throws");

    // .NET Guid "1bd42765-..." bytes are stored little-endian in Value1/Value2
    value.SetAlarm(true, HistorianKey::TypeID, QualityFlags::AlarmHigh);
    Check(value.Value1 == 0x4BFA9D046527D41BULL && value.Value2 == 0x461D523D27053381ULL, "alarm event ID uses .NET Guid byte order");
    Check(value.Value3 == (1ULL << 32 | 0x10ULL), "alarm flag and quality stored in Value3");

    bool alarmed;
    Guid eventID;
    QualityFlags flags;
    value.GetAlarm(alarmed, eventID, flags);
    Check(alarmed && eventID == HistorianKey::TypeID && flags == QualityFlags::AlarmHigh, "alarm round trip");

    Check(ParseSignalType("freq") == SignalType::FREQ && ParseSignalType("XYZ") == SignalType::UNKN, "signal type parsing");
}

void TestHistorianEncoder()
{
    BeginTest("Historian key/value encoder");

    HistorianKeyValueEncoder encoder;
    HistorianKey key;
    HistorianValue value;

    const string encoded = Capture([&](BinaryStream& stream)
    {
        encoder.ResetEncoder();

        for (const TestPoint& point : TestPoints)
        {
            ToKeyValue(point, key, value);
            encoder.StreamEncode(stream, key, value);
        }

        encoder.WriteEndOfStream(stream);
    });

    Check(encoded == HistorianEncodingHex, "historian encoding matches reference: " + encoded);

    // Decode reference encoding
    BinaryStream stream(NewSharedPtr<BufferStream>(FromHexString(HistorianEncodingHex)));
    encoder.ResetEncoder();

    bool decoded = true;
    size_t count = 0;

    while (encoder.TryStreamDecode(stream, key, value))
    {
        if (count >= TestPoints.size() || !Matches(TestPoints[count], key, value))
            decoded = false;

        count++;
    }

    Check(decoded && count == TestPoints.size(), "historian decoding of reference matches source points");

    // Round trip fixed size encoding
    FixedSizeKeyValueEncoder<HistorianKey, HistorianValue> fixedEncoder;
    const BufferStreamPtr buffer = NewSharedPtr<BufferStream>();
    BinaryStream fixedStream(buffer);

    fixedEncoder.ResetEncoder();

    for (const TestPoint& point : TestPoints)
    {
        ToKeyValue(point, key, value);
        fixedEncoder.StreamEncode(fixedStream, key, value);
    }

    fixedEncoder.WriteEndOfStream(fixedStream);
    fixedStream.Flush();

    Check(buffer->Buffer().size() == TestPoints.size() * 49 + 1, "fixed size encoding uses 49 bytes per point plus end of stream");

    fixedEncoder.ResetEncoder();
    decoded = true;
    count = 0;

    while (fixedEncoder.TryStreamDecode(fixedStream, key, value))
    {
        if (count >= TestPoints.size() || !Matches(TestPoints[count], key, value))
            decoded = false;

        count++;
    }

    Check(decoded && count == TestPoints.size(), "fixed size encoding round trip");
}

void TestFilters()
{
    BeginTest("Filters and reader options");

    Check(Capture([](BinaryStream& stream) { TimestampSeekFilter(638640000000000000ULL, 638640000006666666ULL).Save(stream); }) == "0100005b43b2e7dc08aab9c043b2e7dc08", "fixed range timestamp filter matches reference");
    Check(Capture([](BinaryStream& stream) { TimestampSeekFilter(100, 200000000, 10000000, 333333, 10000).Save(stream); }) == "02640000000000000000c2eb0b00000000809698000000000015160500000000001027000000000000", "interval timestamp filter matches reference");
    Check(Capture([](BinaryStream& stream) { PointIDMatchFilter({ 5, 1, 900000001, 5 }).Save(stream); }) == "0101e9a4350000000003000000010000000500000001e9a435", "32-bit point ID filter matches reference");
    Check(Capture([](BinaryStream& stream) { PointIDMatchFilter({ 9000000001ULL, 2 }).Save(stream); }) == "02" "011a711802000000" "02000000" "0200000000000000" "011a711802000000", "64-bit point ID filter is sorted");
    Check(Capture([](BinaryStream& stream) { PointIDMatchFilter({}).Save(stream); }) == "01000000000000000000000000", "empty point ID filter");
    Check(Capture([](BinaryStream& stream) { ReaderOptions(TimeSpan(0, 0, 1, 500000), 100, 200, 300).Save(stream); }) == "00c0e1e400000000006400000000000000c8000000000000002c01000000000000", "reader options match reference");
    Check(Capture([](BinaryStream& stream) { TimestampPointIDSeekFilter(1, 2).Save(stream); }) == "01000000000000000200000000000000", "timestamp/point ID filter serialization");

    bool invalid = false;

    try
    {
        TimestampSeekFilter(200, 100);
    }
    catch (const out_of_range&)
    {
        invalid = true;
    }

    Check(invalid, "timestamp filter with first time after last time throws");

    invalid = false;

    try
    {
        TimestampSeekFilter(100, 200, 10, 10, 20);
    }
    catch (const out_of_range&)
    {
        invalid = true;
    }

    Check(invalid, "timestamp filter with tolerance larger than sub-interval throws");
}

void TestEncodingDefinitions()
{
    BeginTest("Encoding definitions and library");

    RegisterTypes();

    Check(EncodingDefinition::FixedSizeCombinedEncoding.IsFixedSizeEncoding(), "fixed size combined encoding is fixed size");
    Check(!HistorianKeyValueEncoder::Encoding.IsFixedSizeEncoding() && HistorianKeyValueEncoder::Encoding.IsKeyValueEncoded(), "historian encoding is combined key/value encoding");
    Check(HistorianKeyValueEncoder::Encoding.ToString() == "{0418b3a7-f631-47af-bbfa-8b9bc0378328}", "encoding definition string format");

    const string saved = Capture([](BinaryStream& stream) { HistorianKeyValueEncoder::Encoding.Save(stream); });
    BinaryStream stream(NewSharedPtr<BufferStream>(FromHexString(saved)));
    Check(EncodingDefinition(stream) == HistorianKeyValueEncoder::Encoding, "encoding definition round trip");

    const EncodingDefinition individual = EncodingDefinition::CreateIndividualEncoding(EncodingDefinition::FixedSizeIndividualGuid, EncodingDefinition::FixedSizeIndividualGuid);
    Check(individual.IsFixedSizeEncoding() && !individual.IsKeyValueEncoded(), "individual fixed size encoding definition");

    Check(Library::LookupTypeName(HistorianKey::TypeID) == "HistorianKey", "historian key type is registered");
    Check(Library::LookupTypeName(HistorianValue::TypeID) == "HistorianValue", "historian value type is registered");
    Check(EncoderLibrary<HistorianKey, HistorianValue>::IsSupported(HistorianKeyValueEncoder::Encoding), "historian encoding is supported");
    Check(EncoderLibrary<HistorianKey, HistorianValue>::IsSupported(EncodingDefinition::FixedSizeCombinedEncoding), "fixed size encoding is supported");
    Check(!EncoderLibrary<HistorianKey, HistorianValue>::IsSupported(individual), "individual encodings are not supported");
    Check(EncoderLibrary<HistorianKey, HistorianValue>::CreateEncoder(HistorianKeyValueEncoder::Encoding) != nullptr, "historian encoder can be created");
}

void TestClientDatabaseProtocol()
{
    BeginTest("Client database protocol");

    RegisterTypes();

    // Build simulated server responses
    const BufferStreamPtr responses = NewSharedPtr<BufferStream>();
    BinaryStream server(responses);

    // Database info
    server.WriteByte(1);
    server.WriteString("ppa ");
    server.WriteGuid(HistorianKey::TypeID);
    server.WriteGuid(HistorianValue::TypeID);
    server.WriteInt32(2);
    HistorianKeyValueEncoder::Encoding.Save(server);
    EncodingDefinition::FixedSizeCombinedEncoding.Save(server);

    // Encoding accepted
    server.WriteByte(static_cast<uint8_t>(ServerResponse::EncodingMethodAccepted));

    // Read response with points
    server.WriteByte(static_cast<uint8_t>(ServerResponse::SerializingPoints));
    server.Write(FromHexString(HistorianEncodingHex));
    server.WriteByte(static_cast<uint8_t>(ServerResponse::ReadComplete));

    // Second read response, canceled by client after first point
    server.WriteByte(static_cast<uint8_t>(ServerResponse::SerializingPoints));
    server.Write(FromHexString(HistorianEncodingHex));
    server.WriteByte(static_cast<uint8_t>(ServerResponse::ReadComplete));

    // Disconnect response
    server.WriteByte(static_cast<uint8_t>(ServerResponse::DatabaseDisconnected));
    server.Flush();

    const SharedPtr<DuplexStream> duplex = NewSharedPtr<DuplexStream>(responses->Buffer());
    const BinaryStreamPtr stream = NewSharedPtr<BinaryStream>(duplex);
    const DatabaseInfoPtr info = boost::make_shared<DatabaseInfo>(*stream);

    Check(info->DatabaseName() == "PPA", "database name is trimmed and upper case");
    Check(info->KeyTypeName() == "HistorianKey" && info->ValueTypeName() == "HistorianValue", "database key/value type names resolved");
    Check(info->SupportedEncodings().size() == 2, "database supported encodings parsed");

    HistorianInstance instance(stream, info);
    instance.SetEncodingDefinition(HistorianKeyValueEncoder::Encoding);
    Check(instance.GetEncodingDefinition() == HistorianKeyValueEncoder::Encoding, "encoding definition assigned");

    // Read all points
    const HistorianReaderPtr reader = instance.Read(TimestampSeekFilter::CreateFromRange(1, 2), PointIDMatchFilter::CreateFromList({ 1, 2 }));
    HistorianKey key;
    HistorianValue value;
    size_t count = 0;
    bool matches = true;

    while (reader->Read(key, value))
    {
        if (count >= TestPoints.size() || !Matches(TestPoints[count], key, value))
            matches = false;

        count++;
    }

    Check(matches && count == TestPoints.size(), "read decodes all streamed points");
    Check(reader->IsEOS() && reader->IsDisposed(), "reader is disposed at end of stream");

    // Read and cancel after first point
    const HistorianReaderPtr canceledReader = instance.Read();
    Check(canceledReader->Read(key, value) && Matches(TestPoints[0], key, value), "second read returns first point");

    bool blocked = false;

    try
    {
        instance.Write(key, value);
    }
    catch (const SnapException&)
    {
        blocked = true;
    }

    Check(blocked, "write is blocked while reader is active");

    canceledReader->Dispose();

    // Write a point
    const size_t writeOffset = duplex->Output.Buffer().size();
    ToKeyValue(TestPoints[0], key, value);
    instance.Write(key, value);

    const vector<uint8_t>& output = duplex->Output.Buffer();
    const vector<uint8_t> writeRequest(output.begin() + static_cast<ptrdiff_t>(writeOffset), output.end());
    Check(ToHexString(writeRequest) == "05" + HistorianEncodingHex.substr(0, 30) + "ff", "write request encoding: " + ToHexString(writeRequest));

    instance.Dispose();
    Check(instance.IsDisposed(), "instance disposed");
    Check(duplex->Input.Remaining() == 0, "all simulated server responses consumed");
    Check(duplex->Output.Buffer().back() == static_cast<uint8_t>(ServerCommand::DisconnectDatabase), "disconnect database command sent");

    // Verify first request: set encoding method
    const string requests = ToHexString(duplex->Output.Buffer());
    Check(requests.starts_with("0601a7b31804" "31f6af47bbfa8b9bc0378328"), "set encoding method request");

    // Verify read request: command, seek filter, match filter, no reader options
    const string readRequest = "03" "01" "78940f0f42dcef4e9f26231a942ef1fa" "01" "0100000000000000" "0200000000000000"
        "01" "e3a33420" "2ef9" "4947" "9306b04dc36fd743" "01" "0200000000000000" "02000000" "01000000" "02000000" "00";
    Check(Contains(requests, readRequest, false), "read request serialization");
}

void TestMetadataCache()
{
    BeginTest("Metadata cache");

    const string metadataXml = R"(<?xml version="1.0" standalone="yes"?>
<DataSet>
  <DeviceDetail>
    <NodeID>e7a5235d-fb6b-4d92-a4b1-1a1d3d6e2c5e</NodeID>
    <UniqueID>7b5e8a43-4b1e-4c1f-9f1a-2a0e6f5d3c21</UniqueID>
    <Acronym>TEST_STATION</Acronym>
    <Name>Test Station</Name>
    <AccessID>235</AccessID>
    <ProtocolName>IEEE C37.118-2005</ProtocolName>
    <FramesPerSecond>30</FramesPerSecond>
    <CompanyAcronym>GPA</CompanyAcronym>
    <Longitude>-84.32</Longitude>
    <Latitude>35.05</Latitude>
    <UpdatedOn>2026-10-07T12:00:00.12-04:00</UpdatedOn>
  </DeviceDetail>
  <MeasurementDetail>
    <DeviceAcronym>TEST_STATION</DeviceAcronym>
    <ID>PPA:10</ID>
    <SignalID>a3c0a2b2-10ee-4a7c-8b2a-2a3e1c9d5f10</SignalID>
    <PointTag>GPA_TEST:FREQ</PointTag>
    <SignalReference>TEST_STATION-FQ</SignalReference>
    <SignalAcronym>FREQ</SignalAcronym>
    <Description>Test Station Frequency</Description>
  </MeasurementDetail>
  <MeasurementDetail>
    <DeviceAcronym>TEST_STATION</DeviceAcronym>
    <ID>PPA:11</ID>
    <SignalID>b3c0a2b2-10ee-4a7c-8b2a-2a3e1c9d5f11</SignalID>
    <PointTag>GPA_TEST:VA</PointTag>
    <SignalReference>TEST_STATION-PA1</SignalReference>
    <SignalAcronym>VPHA</SignalAcronym>
    <Description>Voltage A Angle</Description>
  </MeasurementDetail>
  <MeasurementDetail>
    <DeviceAcronym>TEST_STATION</DeviceAcronym>
    <ID>PPA:12</ID>
    <SignalID>c3c0a2b2-10ee-4a7c-8b2a-2a3e1c9d5f12</SignalID>
    <PointTag>GPA_TEST:VM</PointTag>
    <SignalReference>TEST_STATION-PM1</SignalReference>
    <SignalAcronym>VPHM</SignalAcronym>
    <Description>Voltage A Magnitude</Description>
  </MeasurementDetail>
  <MeasurementDetail>
    <ID>STAT:5</ID>
    <PointTag>SYSTEM!STAT</PointTag>
    <SignalAcronym>STAT</SignalAcronym>
  </MeasurementDetail>
  <MeasurementDetail>
    <ID>Invalid</ID>
  </MeasurementDetail>
  <PhasorDetail>
    <ID>1</ID>
    <DeviceAcronym>TEST_STATION</DeviceAcronym>
    <Label>Voltage A</Label>
    <Type>V</Type>
    <Phase>A</Phase>
    <SourceIndex>1</SourceIndex>
    <BaseKV>230</BaseKV>
  </PhasorDetail>
</DataSet>)";

    const MetadataCache metadata(metadataXml);

    Check(metadata.MeasurementRecords().size() == 4, "measurement records parsed, invalid IDs skipped");
    Check(metadata.DeviceRecords().size() == 1 && metadata.PhasorRecords().size() == 1, "device and phasor records parsed");

    const DeviceRecordPtr device = metadata.LookupDeviceByAcronym("test_station");
    Check(device != nullptr && device->Measurements.size() == 3 && device->Phasors.size() == 1, "device lookup is case-insensitive with associated records");
    Check(device != nullptr && device->AccessID == 235 && ToString(device->Longitude) == "-84.32", "device fields parsed");

    const MeasurementRecordPtr frequency = metadata.LookupMeasurementByPointID(10);
    Check(frequency != nullptr && frequency->InstanceName == "PPA" && frequency->AsSignalType == SignalType::FREQ && frequency->Device == device, "measurement lookup by point ID");
    Check(metadata.LookupMeasurementByPointTag("GPA_TEST:FREQ") == frequency, "measurement lookup by point tag");
    Check(metadata.LookupMeasurementBySignalReference("TEST_STATION-FQ") == frequency, "measurement lookup by signal reference");
    Check(metadata.LookupMeasurementBySignalID(ParseGuid("a3c0a2b2-10ee-4a7c-8b2a-2a3e1c9d5f10")) == frequency, "measurement lookup by signal ID");

    const PhasorRecordPtr phasor = metadata.PhasorRecords()[0];
    Check(phasor->Type == 'V' && phasor->Phase == 'A' && phasor->BaseKV == 230, "phasor fields parsed");
    Check(phasor->AngleMeasurement() == metadata.LookupMeasurementByPointID(11) && phasor->MagnitudeMeasurement() == metadata.LookupMeasurementByPointID(12), "phasor angle and magnitude associated");

    Check(metadata.GetMeasurementsBySignalType(SignalType::STAT).size() == 1, "measurements by signal type");
    Check(metadata.GetMeasurementsBySignalType(SignalType::STAT, "PPA").empty(), "measurements by signal type filtered by instance");
    Check(metadata.GetMeasurementsByTextSearch("voltage").size() == 2, "measurements by text search");
    Check(metadata.GetDevicesByTextSearch("station", "PPA").size() == 1 && metadata.GetDevicesByTextSearch("station", "STAT").empty(), "devices by text search filtered by instance");
    Check(MetadataCache::ToPointIDList(device->Measurements) == vector<uint64_t>{ 10, 11, 12 }, "point ID list");

    HistorianKey key(10, ParseTimestamp("2026-10-07 12:00:00"));
    Check(key.ToString(metadata) == "10: TEST_STATION-FQ [FREQ] @ 2026-10-07 12:00:00.000", "key string with metadata: " + key.ToString(metadata));
}

void TestConnectionArguments()
{
    BeginTest("Connection arguments and sockets");

    const HistorianConnection connection1("localhost");
    Check(connection1.HostName() == "localhost" && connection1.HostPort() == DefaultPort, "default port");

    const HistorianConnection connection2("server1:38500");
    Check(connection2.HostName() == "server1" && connection2.HostPort() == 38500 && connection2.HostAddress() == "server1:38500", "host address with port");

    const HistorianConnection connection3("[::1]:38403");
    Check(connection3.HostName() == "::1" && connection3.HostPort() == 38403 && connection3.HostAddress() == "[::1]:38403", "IPv6 host address with port");

    const HistorianConnection connection4("::1");
    Check(connection4.HostName() == "::1" && connection4.HostPort() == DefaultPort, "IPv6 host address without port");

    Check(!connection1.IsConnected() && connection1.InstanceNames().empty(), "not connected before connect");

    bool invalid = false;

    try
    {
        HistorianConnection connection5("server1:abc");
    }
    catch (const invalid_argument&)
    {
        invalid = true;
    }

    Check(invalid, "invalid port throws");

    // Connection to closed port fails
    HistorianConnection connection6("127.0.0.1:1");
    connection6.SetConnectTimeout(2000);
    bool failed = false;

    try
    {
        connection6.Connect();
    }
    catch (const std::exception&)
    {
        failed = true;
    }

    Check(failed && !connection6.IsConnected(), "connect to closed port fails");

    // Socket read times out when server does not respond
    IOContext ioContext;
    TcpAcceptor acceptor(ioContext, TcpEndPoint(boost::asio::ip::make_address("127.0.0.1"), 0));
    const SocketStreamPtr socket = NewSharedPtr<SocketStream>();

    socket->Connect("127.0.0.1", acceptor.local_endpoint().port(), 2000);
    socket->SetIOTimeout(250);

    bool timedOut = false;
    uint8_t buffer[16];
    const auto startTime = chrono::steady_clock::now();

    try
    {
        socket->Read(buffer, 0, sizeof(buffer));
    }
    catch (const SnapTimeoutException&)
    {
        timedOut = true;
    }

    const auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - startTime).count();

    Check(timedOut && elapsed >= 200 && elapsed < 2000, "socket read times out, elapsed " + to_string(elapsed) + "ms");
    Check(!socket->IsOpen(), "socket is closed after timeout");
}

int main()
{
    cout << SNAPDB_TITLE << " v" << SNAPDB_VERSION << " unit tests" << endl << endl;

    const vector<void(*)()> tests =
    {
        TestEncoding7Bit,
        TestBinaryStream,
        TestHistorianTypes,
        TestHistorianEncoder,
        TestFilters,
        TestEncodingDefinitions,
        TestClientDatabaseProtocol,
        TestMetadataCache,
        TestConnectionArguments
    };

    for (const auto test : tests)
    {
        try
        {
            test();
        }
        catch (const std::exception& ex)
        {
            Failures++;
            cout << "    FAIL: unexpected exception: " << ex.what() << endl;
        }
    }

    cout << endl << Passes << " checks passed, " << Failures << " failed." << endl;

    return Failures == 0 ? 0 : 1;
}
