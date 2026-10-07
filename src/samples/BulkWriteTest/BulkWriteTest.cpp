//******************************************************************************************************
//  BulkWriteTest.cpp - Gbtc
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
#include <iomanip>

using namespace std;
using namespace boost::posix_time;
using namespace snapdb;
using namespace snapdb::snap;
using namespace snapdb::openhistorian;

typedef vector<pair<HistorianKey, HistorianValue>> PointList;

namespace
{
    // Point IDs in 32-bit range and beyond 32-bit range to exercise both point ID match filter encodings
    constexpr uint64_t PointCount = 10;
    constexpr uint64_t BasePointID32 = 900000000ULL;
    constexpr uint64_t BasePointID64 = 9000000000ULL;
    constexpr int32_t SamplesPerSecond = 30;
    constexpr int32_t TotalSeconds = 60;

    int32_t Failures = 0;

    void Check(const bool condition, const string& description)
    {
        cout << (condition ? "    PASS: " : "    FAIL: ") << description << endl;

        if (!condition)
            Failures++;
    }

    float64_t Elapsed(const chrono::steady_clock::time_point& startTime)
    {
        return chrono::duration<float64_t>(chrono::steady_clock::now() - startTime).count();
    }

    // Generates deterministic test data sorted by timestamp then point ID
    PointList GenerateTestData(const uint64_t startTime)
    {
        PointList points;
        constexpr int32_t sampleCount = SamplesPerSecond * TotalSeconds;

        for (int32_t i = 0; i < sampleCount; i++)
        {
            // Timestamps are aligned to sample rate, starting at top of second
            const uint64_t timestamp = startTime + static_cast<uint64_t>(i) * Ticks::PerSecond / SamplesPerSecond;

            // 32-bit point IDs use standard single-precision values and alternating quality, which
            // exercises compact historian encoding paths
            for (uint64_t p = 1; p <= PointCount; p++)
            {
                HistorianKey key(BasePointID32 + p, timestamp);
                HistorianValue value(static_cast<float32_t>(p) * 100.0F + static_cast<float32_t>(i) * 0.25F, i % 2 == 0 ? QualityFlags::WarningHigh : QualityFlags::Normal);
                points.emplace_back(key, value);
            }

            // 64-bit point IDs use double-precision values and 64-bit Value3 data, which exercises
            // full historian encoding paths
            for (uint64_t p = 1; p <= PointCount; p++)
            {
                HistorianKey key(BasePointID64 + p, timestamp);
                HistorianValue value;
                value.SetDouble(static_cast<float64_t>(p) * 1.0E9 + static_cast<float64_t>(i) * 0.001);
                value.Value3 = i % 3 == 0 ? 0x100000001ULL : 0ULL;
                points.emplace_back(key, value);
            }
        }

        return points;
    }

    vector<uint64_t> GetPointIDs(const uint64_t basePointID)
    {
        vector<uint64_t> pointIDs;

        for (uint64_t p = 1; p <= PointCount; p++)
            pointIDs.push_back(basePointID + p);

        return pointIDs;
    }

    PointList ReadAll(const HistorianReaderPtr& reader)
    {
        PointList points;
        HistorianKey key;
        HistorianValue value;

        while (reader->Read(key, value))
            points.emplace_back(key, value);

        return points;
    }

    bool PointsMatch(const PointList& actual, const PointList& expected, string& mismatch)
    {
        if (actual.size() != expected.size())
        {
            mismatch = "expected " + to_string(expected.size()) + " points, received " + to_string(actual.size());
            return false;
        }

        for (size_t i = 0; i < actual.size(); i++)
        {
            if (!(actual[i].first == expected[i].first) || !(actual[i].second == expected[i].second))
            {
                mismatch = "point " + to_string(i) + " mismatch: expected " + expected[i].first.ToString() + " = " + expected[i].second.ToString() +
                    ", received " + actual[i].first.ToString() + " = " + actual[i].second.ToString();
                return false;
            }
        }

        return true;
    }

    PointList Filter(const PointList& points, const uint64_t basePointID)
    {
        PointList filtered;

        for (const auto& point : points)
        {
            if (point.first.PointID > basePointID && point.first.PointID <= basePointID + PointCount)
                filtered.push_back(point);
        }

        return filtered;
    }
}

// Sample application that verifies openHistorian read and write operations. Application writes a block of
// test data using a bulk writer, reads the data back and verifies it, then tests various read options,
// filters and encodings. Test data is written one hour in the past using point IDs that are not expected
// to be defined in openHistorian metadata. Returns the number of failed checks.
int main(int argc, char* argv[])
{
    // Get optional host address, e.g., "localhost:38402", and instance name
    const string hostAddress = argc > 1 ? argv[1] : "localhost";
    string instanceName = argc > 2 ? argv[2] : "";

    HistorianConnection historian(hostAddress);

    try
    {
        cout << "Connecting to openHistorian at \"" << historian.HostAddress() << "\"..." << endl;
        historian.Connect();

        if (instanceName.empty())
        {
            if (historian.InstanceNames().empty())
            {
                cout << "No openHistorian instances detected!" << endl;
                return 1;
            }

            instanceName = historian.InstanceNames()[0];
        }

        cout << "Opening \"" << instanceName << "\" database instance..." << endl;
        HistorianInstancePtr instance = historian.OpenInstance(instanceName);
        cout << "Using encoding " << instance->GetEncodingDefinition().ToString() << endl << endl;

        // Generate test data, starting one hour ago at top of second
        const uint64_t now = static_cast<uint64_t>(ToTicks(UtcNow()));
        const uint64_t startTime = (now - static_cast<uint64_t>(Ticks::PerHour)) / Ticks::PerSecond * Ticks::PerSecond;
        const PointList expected = GenerateTestData(startTime);
        const uint64_t endTime = expected.back().first.Timestamp;
        const vector<uint64_t> pointIDs32 = GetPointIDs(BasePointID32);
        const vector<uint64_t> pointIDs64 = GetPointIDs(BasePointID64);
        vector<uint64_t> allPointIDs = pointIDs32;
        allPointIDs.insert(allPointIDs.end(), pointIDs64.begin(), pointIDs64.end());

        cout << "Test data: " << expected.size() << " points from " << HistorianKey(0, startTime).ToString().substr(4) << " to " << HistorianKey(0, endTime).ToString().substr(4) << endl << endl;

        // Bulk write test data
        cout << "Bulk write test:" << endl;
        auto opStart = chrono::steady_clock::now();

        {
            const HistorianBulkWriterPtr writer = instance->StartBulkWriting();

            for (const auto& [key, value] : expected)
                writer->Write(key, value);

            Check(writer->Count() == expected.size(), "bulk writer wrote " + to_string(writer->Count()) + " points");

            // Operations are blocked while bulk writing is active
            bool blocked = false;

            try
            {
                instance->Read();
            }
            catch (const SnapException&)
            {
                blocked = true;
            }

            Check(blocked, "read is blocked while bulk writer is active");
            writer->Dispose();
        }

        float64_t elapsed = Elapsed(opStart);
        cout << "    Sent " << expected.size() << " points in " << fixed << setprecision(3) << elapsed << " seconds (" << setprecision(0) << static_cast<float64_t>(expected.size()) / elapsed << " points/second)" << endl << endl;

        // Read back all test data, retrying briefly while server commits written data
        cout << "Read verification test:" << endl;
        PointList actual;
        string mismatch;

        for (int32_t attempt = 0; attempt < 20; attempt++)
        {
            opStart = chrono::steady_clock::now();
            actual = ReadAll(instance->Read(startTime, endTime, allPointIDs));
            elapsed = Elapsed(opStart);

            if (actual.size() >= expected.size())
                break;

            ThreadSleep(500);
        }

        cout << "    Read " << actual.size() << " points in " << setprecision(3) << elapsed << " seconds (" << setprecision(0) << static_cast<float64_t>(actual.size()) / elapsed << " points/second)" << endl;
        Check(PointsMatch(actual, expected, mismatch), "all points read back match written points" + (mismatch.empty() ? "" : ": " + mismatch));

        // Verify 32-bit point ID filter
        mismatch.clear();
        actual = ReadAll(instance->Read(startTime, endTime, pointIDs32));
        Check(PointsMatch(actual, Filter(expected, BasePointID32), mismatch), "32-bit point ID filter returns expected points" + (mismatch.empty() ? "" : ": " + mismatch));

        // Verify 64-bit point ID filter
        mismatch.clear();
        actual = ReadAll(instance->Read(startTime, endTime, pointIDs64));
        Check(PointsMatch(actual, Filter(expected, BasePointID64), mismatch), "64-bit point ID filter returns expected points" + (mismatch.empty() ? "" : ": " + mismatch));

        // Verify point ID filter with single point
        actual = ReadAll(instance->Read(TimestampSeekFilter::CreateFromRange(startTime, endTime), PointIDMatchFilter::CreateFromPointID(BasePointID32 + 1)));
        Check(actual.size() == static_cast<size_t>(SamplesPerSecond * TotalSeconds), "single point ID filter returns " + to_string(actual.size()) + " points");
        cout << endl;

        // Verify reader options are accepted -- note that current SNAPdb servers only enforce the
        // timeout option, maximum count options are serialized but not enforced by server
        cout << "Reader options test:" << endl;
        const ReaderOptionsPtr options = NewSharedPtr<ReaderOptions>(TimeSpan(0, 0, 30), 100);
        actual = ReadAll(instance->Read(startTime, endTime, allPointIDs, options));
        Check(actual.size() == expected.size() || actual.size() == 100, "reader options accepted by server, read returned " + to_string(actual.size()) + " points");
        cout << endl;

        // Verify early reader cancel leaves connection in a usable state
        cout << "Reader cancel test:" << endl;

        {
            const HistorianReaderPtr reader = instance->Read(startTime, endTime, allPointIDs);
            HistorianKey key;
            HistorianValue value;

            for (int32_t i = 0; i < 10 && reader->Read(key, value); i++)
            {
            }

            // Concurrent reads are not supported
            bool blocked = false;

            try
            {
                instance->Read();
            }
            catch (const SnapException&)
            {
                blocked = true;
            }

            Check(blocked, "concurrent read is blocked while reader is active");
            reader->Dispose();
        }

        actual = ReadAll(instance->Read(startTime, startTime, { BasePointID32 + 1 }));
        Check(actual.size() == 1 && actual[0] == expected[0], "read after canceled reader returns expected point");

        // Dropped readers are automatically canceled
        {
            const HistorianReaderPtr reader = instance->Read(startTime, endTime, allPointIDs);
            HistorianKey key;
            HistorianValue value;
            reader->Read(key, value);
        }

        actual = ReadAll(instance->Read(startTime, startTime, { BasePointID32 + 1 }));
        Check(actual.size() == 1 && actual[0] == expected[0], "read after dropped reader returns expected point");
        cout << endl;

        // Verify callback read with early termination
        cout << "Callback read test:" << endl;
        int32_t callbackLimit = 0;

        const uint64_t callbackCount = instance->Read(TimestampSeekFilter::CreateFromRange(startTime, endTime), PointIDMatchFilter::CreateFromList(allPointIDs), nullptr,
            [&callbackLimit](const HistorianKey&, const HistorianValue&) { return ++callbackLimit < 50; });
        Check(callbackCount == 50, "callback read stopped after " + to_string(callbackCount) + " points");
        cout << endl;

        // Verify interval seek filter, test data has a sample at the top of each second
        cout << "Interval filter test:" << endl;
        actual = ReadAll(instance->Read(TimestampSeekFilter::CreateFromIntervalData(startTime, endTime, Ticks::PerSecond, Ticks::PerSecond, Ticks::PerMillisecond), PointIDMatchFilter::CreateFromList(pointIDs32)));
        bool onInterval = !actual.empty() && ranges::all_of(actual, [](const auto& point) { return point.first.Timestamp % Ticks::PerSecond == 0; });
        Check(actual.size() == PointCount * TotalSeconds && onInterval, "1-second interval filter returns " + to_string(actual.size()) + " points, all on interval: " + (onInterval ? "true" : "false"));
        cout << endl;

        // Verify write APIs for a list of points and a tree stream
        cout << "List and stream write test:" << endl;
        PointList extraPoints;

        for (uint64_t p = 1; p <= 5; p++)
            extraPoints.emplace_back(HistorianKey(BasePointID32 + p, endTime + Ticks::PerSecond), HistorianValue(static_cast<float32_t>(p), QualityFlags::CalculatedValue));

        instance->Write(extraPoints);

        PointList streamPoints;

        for (uint64_t p = 1; p <= 5; p++)
            streamPoints.emplace_back(HistorianKey(BasePointID32 + p, endTime + 2 * Ticks::PerSecond), HistorianValue(static_cast<float32_t>(p) * 2.0F, QualityFlags::Normal));

        VectorTreeStream<HistorianKey, HistorianValue> treeStream(streamPoints);
        instance->Write(treeStream);

        PointList expectedExtra = extraPoints;
        expectedExtra.insert(expectedExtra.end(), streamPoints.begin(), streamPoints.end());

        for (int32_t attempt = 0; attempt < 20; attempt++)
        {
            actual = ReadAll(instance->Read(endTime + Ticks::PerSecond, endTime + 2 * Ticks::PerSecond, pointIDs32));

            if (actual.size() >= expectedExtra.size())
                break;

            ThreadSleep(500);
        }

        mismatch.clear();
        Check(PointsMatch(actual, expectedExtra, mismatch), "list and tree stream writes read back as expected" + (mismatch.empty() ? "" : ": " + mismatch));
        cout << endl;

        // Verify fixed size encoding returns same data as historian encoding
        cout << "Fixed size encoding test:" << endl;
        historian.CloseInstance();
        instance = historian.OpenInstance(instanceName, NewSharedPtr<EncodingDefinition>(EncodingDefinition::FixedSizeCombinedEncoding));
        Check(instance->GetEncodingDefinition().IsFixedSizeEncoding(), "instance opened with fixed size encoding " + instance->GetEncodingDefinition().ToString());

        opStart = chrono::steady_clock::now();
        actual = ReadAll(instance->Read(startTime, endTime, allPointIDs));
        elapsed = Elapsed(opStart);
        cout << "    Read " << actual.size() << " points in " << setprecision(3) << elapsed << " seconds" << endl;

        mismatch.clear();
        Check(PointsMatch(actual, expected, mismatch), "fixed size encoding read matches written points" + (mismatch.empty() ? "" : ": " + mismatch));

        // Write with fixed size encoding and verify with historian encoding
        const HistorianKey fixedKey(BasePointID64 + 1, endTime + 3 * Ticks::PerSecond);
        HistorianValue fixedValue;
        fixedValue.SetString("FixedSize Value");
        instance->Write(fixedKey, fixedValue);

        historian.CloseInstance();
        instance = historian.OpenInstance(instanceName);

        for (int32_t attempt = 0; attempt < 20; attempt++)
        {
            actual = ReadAll(instance->Read(fixedKey.Timestamp, fixedKey.Timestamp, { fixedKey.PointID }));

            if (!actual.empty())
                break;

            ThreadSleep(500);
        }

        Check(actual.size() == 1 && actual[0].second.AsString() == "FixedSize Value", "point written with fixed size encoding reads back with historian encoding");
        cout << endl;

        // Verify timestamp/point ID seek filter -- older servers do not support this filter and will close the
        // connection, so test is executed last
        cout << "Timestamp and point ID seek filter test:" << endl;

        try
        {
            actual = ReadAll(instance->Read(TimestampPointIDSeekFilter::FindKey(expected[5].first.Timestamp, expected[5].first.PointID)));
            Check(actual.size() == 1 && actual[0] == expected[5], "find key filter returns expected point");
        }
        catch (const SnapException& ex)
        {
            cout << "    SKIP: server does not support timestamp/point ID seek filter: " << ex.what() << endl;
        }

        cout << endl;
    }
    catch (const std::exception& ex)
    {
        cerr << "Test failed with exception: " << ex.what() << endl;
        Failures++;
    }

    historian.Disconnect();

    cout << (Failures == 0 ? "All tests passed." : to_string(Failures) + " test(s) failed.") << endl;

    return Failures;
}
