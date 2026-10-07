# SNAPdb / openHistorian C++ API

[![Build](https://github.com/snapdb/cppapi/actions/workflows/build.yml/badge.svg)](https://github.com/snapdb/cppapi/actions/workflows/build.yml)
[![CodeQL](https://github.com/snapdb/cppapi/actions/workflows/codeql.yml/badge.svg)](https://github.com/snapdb/cppapi/actions/workflows/codeql.yml)

C++ API for high-speed reading and writing of time-series data with the [openHistorian](https://github.com/GridProtectionAlliance/openHistorian) using the [SNAPdb](https://github.com/snapdb/SnapDB) socket protocol.

Requires [boost](https://www.boost.org/). See [build steps](src) for Windows and Linux.

## Overview

The openHistorian is built using the SNAPdb Engine, a key/value pair archiving technology designed to handle extremely large volumes of real-time streaming data and directly serve the data to consuming applications. The openHistorian is a time-series implementation of SNAPdb where the "key" is a tuple of timestamp and measurement point ID, and the "value" is the stored data, normally a 32-bit float and quality flags.

This API is a socket-based client that talks directly to the SNAPdb server hosted by the openHistorian (default port `38402`), including its in-memory cache for very high-speed access to near real-time data. Metadata, i.e., the measurements, devices and phasors defined in the openHistorian, is queried using the openHistorian [STTP](https://sttp.info/) publisher (default port `7175`).

The API is a C++ port of the [openHistorian Python API](https://github.com/GridProtectionAlliance/openhistorian-python) and follows the authoritative C# [SNAPdb](https://github.com/snapdb/SnapDB) implementation. Code structure, Boost usage and helper functions follow the [STTP C++ API](https://github.com/sttp/cppapi).

### Features

* Connect to a SNAPdb server, list client database instances and their supported encodings
* Read with server-side filters:
  * `TimestampSeekFilter`: time range, or time range with interval sampling
  * `PointIDMatchFilter`: set of point IDs (32-bit and 64-bit point IDs)
  * `TimestampPointIDSeekFilter`: exact key lookup (newer SNAPdb servers)
* Write individual points, lists of points, tree streams, or stream large blocks with a bulk writer
* openHistorian compressed stream encoding, plus fixed-size (uncompressed) encoding
* Metadata cache with lookups by point ID, signal ID, point tag, signal reference, signal type, device and text search, including device/phasor/measurement relationships
* Optional connect and socket I/O timeouts
* Generic SNAPdb core: any key/value types satisfying the `snap::SnapType` concept can be used with `SnapConnection::OpenInstance<TKey, TValue>()`

### Namespaces

| Namespace | Folder | Description |
|---|---|---|
| `snapdb` | `src/lib` | Common types and helper functions (shared pattern with STTP C++ API) |
| `snapdb::io` | `src/lib/io` | Socket, buffered binary stream and 7-bit encoding |
| `snapdb::snap` | `src/lib/snap` | SNAPdb protocol: connection, client database, filters, encoders |
| `snapdb::openhistorian` | `src/lib/openhistorian` | openHistorian key/value types, encoder, metadata and connection |

## Example Usage

Full source for the examples can be found in the [samples](src/samples):

* Reading data ([ReadTest.cpp](src/samples/ReadTest/ReadTest.cpp))
* Writing data ([WriteTest.cpp](src/samples/WriteTest/WriteTest.cpp))
* Querying metadata ([MetadataTest.cpp](src/samples/MetadataTest/MetadataTest.cpp))
* Full API walk-through ([HistorianTest.cpp](src/samples/HistorianTest/HistorianTest.cpp))
* Bulk write with read verification ([BulkWriteTest.cpp](src/samples/BulkWriteTest/BulkWriteTest.cpp))

### Reading Data

The following example connects to the openHistorian, opens the first client database instance, refreshes metadata, looks up the measurements for a device, then reads the last minute of data for those measurements as historian key/value pairs:

```cpp
#include "lib/openhistorian/HistorianConnection.h"
#include "lib/Convert.h"
#include <iostream>

using namespace std;
using namespace snapdb;
using namespace snapdb::openhistorian;

int main()
{
    // Create historian connection (the root API object)
    HistorianConnection historian("localhost");

    try
    {
        historian.Connect();

        // Open first historian instance, e.g., "PPA"
        const HistorianInstancePtr instance = historian.OpenInstance(historian.InstanceNames()[0]);

        // Query metadata from openHistorian
        historian.RefreshMetadata();
        const MetadataCachePtr& metadata = historian.Metadata();

        // Lookup measurements for a device
        const DeviceRecordPtr device = metadata->LookupDeviceByAcronym("TEST_STATION");
        const vector<uint64_t> pointIDs = MetadataCache::ToPointIDList(device->Measurements);

        // Read last minute of data
        const datetime_t endTime = UtcNow();
        const datetime_t startTime = endTime - boost::posix_time::minutes(1);

        const HistorianReaderPtr reader = instance->Read(startTime, endTime, pointIDs);
        HistorianKey key;
        HistorianValue value;

        while (reader->Read(key, value))
            cout << key.ToString(*metadata) << " = " << value.ToString() << endl;
    }
    catch (const std::exception& ex)
    {
        cerr << "Failed to read data: " << ex.what() << endl;
    }

    historian.Disconnect();
}
```

Example output:

```
109: TEST_STATION-FQ [FREQ] @ 2026-10-07 12:00:00.033 = 60.001 [Normal]
112: TEST_STATION-PM1 [VPHM] @ 2026-10-07 12:00:00.033 = 299850.125 [Normal]
113: TEST_STATION-PA1 [VPHA] @ 2026-10-07 12:00:00.033 = 12.345 [Normal]
```

Filters can also be constructed directly, e.g., to down-sample to one value per second:

```cpp
const HistorianReaderPtr reader = instance->Read(
    TimestampSeekFilter::CreateFromIntervalData(startTime, endTime, seconds(1), milliseconds(1)),
    PointIDMatchFilter::CreateFromList(pointIDs));
```

Only one reader can be active at a time for an instance. Read a stream to completion, or dispose of the reader, before starting another operation. Note that the SNAPdb protocol does not support early termination of a read: disposing a reader before the end of the stream reads and discards the remaining points, so use narrow filters for large archives.

### Writing Data

```cpp
HistorianKey key;
key.PointID = 1;
key.SetDateTime(UtcNow());

HistorianValue value;
value.SetSingle(1000.98F);
value.SetQuality(QualityFlags::WarningHigh);

instance->Write(key, value);
```

For large blocks of data, use a bulk writer, which streams all points as a single write operation:

```cpp
const HistorianBulkWriterPtr writer = instance->StartBulkWriting();

for (const auto& [key, value] : points)
    writer->Write(key, value);

writer->Dispose(); // Completes write operation
```

Writes are sent to the server without acknowledgement; data is typically available for reading within a moment of being written.

## Python to C++ API Mapping

| Python API | C++ API |
|---|---|
| `historianConnection("localhost")` | `HistorianConnection historian("localhost")` |
| `historian.Connect()` / `Disconnect()` | `historian.Connect()` / `Disconnect()` |
| `historian.InstanceNames` | `historian.InstanceNames()` |
| `historian.GetInstanceInfo(name)` | `historian.GetInstanceInfo(name)` |
| `historian.OpenInstance(name)` | `historian.OpenInstance(name)` |
| `historian.RefreshMetadata()` | `historian.RefreshMetadata()` |
| `historian.Metadata` | `historian.Metadata()` |
| `instance.Read(timeFilter, pointFilter)` | `instance->Read(timeFilter, pointFilter)` or `instance->Read(startTime, endTime, pointIDs)` |
| `instance.Write(key, value)` | `instance->Write(key, value)` |
| `instance.WriteTreeStream(stream)` | `instance->Write(stream)` or `instance->StartBulkWriting()` |
| `reader.Read(key, value)` | `reader->Read(key, value)` |
| `timestampSeekFilter.CreateFromRange(...)` | `TimestampSeekFilter::CreateFromRange(...)` |
| `pointIDMatchFilter.CreateFromList(...)` | `PointIDMatchFilter::CreateFromList(...)` |
| `key.AsDateTime` | `key.GetDateTime()` / `key.SetDateTime()` |
| `value.AsSingle` / `value.AsQuality` | `value.AsSingle()` / `value.SetSingle()`, `value.AsQuality()` / `value.SetQuality()` |
| `metadataCache.GetMeasurementsBySignalType(...)` | `metadata->GetMeasurementsBySignalType(...)` |

## Limitations

* SNAPdb servers configured to require SSL or integrated security are not currently supported (same as the Python API)
* Separate key and value encodings are not currently supported, combined key/value encodings are used
* The connection and its client database instances are not thread-safe, use one connection per thread
