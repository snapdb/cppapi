//******************************************************************************************************
//  SnapClientDatabase.h - Gbtc
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

#include "DatabaseInfo.h"
#include "Filters.h"
#include "Library.h"
#include "ReaderOptions.h"
#include "TreeStream.h"

namespace snapdb::snap
{
    // Represents the non-generic base of a SNAPdb client database instance.
    class ClientDatabaseBase
    {
    public:
        virtual ~ClientDatabaseBase() = default;

        // Gets basic information about the database instance.
        virtual const DatabaseInfoPtr& Info() const = 0;

        // Gets flag that determines if the client database instance is disposed.
        virtual bool IsDisposed() const = 0;

        // Disconnects from the client database instance.
        virtual void Dispose() = 0;
    };

    typedef SharedPtr<ClientDatabaseBase> ClientDatabaseBasePtr;

    // Reads SNAPdb key/value pairs streamed from server in response to a read request.
    template<SnapType TKey, SnapType TValue>
    class PointReader final : public TreeStream<TKey, TValue>
    {
    private:
        KeyValueEncoderPtr<TKey, TValue> m_encoder;
        io::BinaryStreamPtr m_stream;
        bool m_completed;

        void Complete()
        {
            if (m_completed)
                return;

            m_completed = true;

            const ServerResponse response = ReadResponse(*m_stream);

            if (response == ServerResponse::ErrorWhileReading)
            {
                const std::string message = m_stream->ReadString();

                // Server closes connection after a read error
                m_stream->Close();

                throw SnapException("SNAPdb server exception encountered while reading: " + message);
            }

            ValidateExpectedResponses(response, { ServerResponse::ReadComplete, ServerResponse::CanceledRead });
        }

    protected:
        bool ReadNext(TKey& key, TValue& value) override
        {
            if (!m_completed && m_encoder->TryStreamDecode(*m_stream, key, value))
                return true;

            Complete();
            return false;
        }

        void Disposing() override
        {
            Cancel();
        }

    public:
        PointReader(KeyValueEncoderPtr<TKey, TValue> encoder, io::BinaryStreamPtr stream) :
            m_encoder(std::move(encoder)),
            m_stream(std::move(stream)),
            m_completed(false)
        {
            m_encoder->ResetEncoder();
        }

        ~PointReader() override
        {
            try
            {
                this->Dispose();
            }
            catch (...)
            {
                // Exceptions are not propagated from destructor
            }
        }

        // Gets flag that determines if the read operation has completed, i.e., all
        // points for the read request have been received from the server.
        bool IsCompleted() const
        {
            return m_completed;
        }

        // Cancels the read operation. Note that the SNAPdb protocol does not currently support
        // early termination of a read, so any remaining points are read from the stream and
        // discarded. To limit points returned by a read, use narrower seek and match filters.
        void Cancel()
        {
            if (m_completed)
                return;

            TKey key;
            TValue value;

            // Flush the remainder of the data off of the receive queue
            while (m_encoder->TryStreamDecode(*m_stream, key, value))
            {
            }

            Complete();
        }
    };

    template<SnapType TKey, SnapType TValue>
    using PointReaderPtr = SharedPtr<PointReader<TKey, TValue>>;

    // Writes a stream of SNAPdb key/value pairs to the server as a single write operation. Writing
    // remains active, i.e., other database operations are blocked, until the writer is disposed.
    template<SnapType TKey, SnapType TValue>
    class BulkWriter final
    {
    private:
        KeyValueEncoderPtr<TKey, TValue> m_encoder;
        io::BinaryStreamPtr m_stream;
        uint64_t m_count;
        bool m_disposed;

    public:
        BulkWriter(KeyValueEncoderPtr<TKey, TValue> encoder, io::BinaryStreamPtr stream) :
            m_encoder(std::move(encoder)),
            m_stream(std::move(stream)),
            m_count(0ULL),
            m_disposed(false)
        {
            m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::Write));
            m_encoder->ResetEncoder();
        }

        ~BulkWriter()
        {
            try
            {
                Dispose();
            }
            catch (...)
            {
                // Exceptions are not propagated from destructor
            }
        }

        BulkWriter(const BulkWriter&) = delete;
        BulkWriter& operator=(const BulkWriter&) = delete;

        // Gets flag that determines if bulk writer has been disposed.
        bool IsDisposed() const
        {
            return m_disposed;
        }

        // Gets the number of key/value pairs written.
        uint64_t Count() const
        {
            return m_count;
        }

        // Writes a key/value pair to the stream.
        void Write(const TKey& key, const TValue& value)
        {
            if (m_disposed)
                throw SnapException("Cannot write to SNAPdb bulk writer: writer has been disposed");

            m_encoder->StreamEncode(*m_stream, key, value);
            m_count++;
        }

        // Completes the write operation, sending any remaining buffered data to the server.
        void Dispose()
        {
            if (m_disposed)
                return;

            m_disposed = true;

            m_encoder->WriteEndOfStream(*m_stream);
            m_stream->Flush();
        }
    };

    template<SnapType TKey, SnapType TValue>
    using BulkWriterPtr = SharedPtr<BulkWriter<TKey, TValue>>;

    // Represents a single SNAPdb client database, a.k.a., an "instance",
    // that reads and writes data from an underlying stream, e.g., a socket.
    template<SnapType TKey, SnapType TValue>
    class SnapClientDatabase : public ClientDatabaseBase
    {
    private:
        io::BinaryStreamPtr m_stream;
        DatabaseInfoPtr m_info;
        KeyValueEncoderPtr<TKey, TValue> m_encoder;
        WeakPtr<PointReader<TKey, TValue>> m_reader;
        WeakPtr<BulkWriter<TKey, TValue>> m_writer;
        bool m_disposed;

        bool ReaderIsActive() const
        {
            const PointReaderPtr<TKey, TValue> reader = m_reader.lock();
            return reader != nullptr && !reader->IsCompleted();
        }

        bool WriterIsActive() const
        {
            const BulkWriterPtr<TKey, TValue> writer = m_writer.lock();
            return writer != nullptr && !writer->IsDisposed();
        }

        void ValidateState(const std::string& operation) const
        {
            if (m_disposed)
                throw SnapException("Cannot " + operation + ": SNAPdb instance \"" + Name() + "\" has been disposed");

            if (!m_stream->IsOpen())
                throw SnapException("Cannot " + operation + ": SNAPdb connection is closed");

            if (m_encoder == nullptr)
                throw SnapException("Cannot " + operation + ": no encoding definition has been assigned");

            if (ReaderIsActive())
                throw SnapException("Cannot " + operation + ": concurrent operations while reading are not supported, dispose of active reader first");

            if (WriterIsActive())
                throw SnapException("Cannot " + operation + ": concurrent operations while bulk writing are not supported, dispose of active bulk writer first");
        }

    public:
        SnapClientDatabase(io::BinaryStreamPtr stream, DatabaseInfoPtr info) :
            m_stream(std::move(stream)),
            m_info(std::move(info)),
            m_disposed(false)
        {
        }

        ~SnapClientDatabase() override
        {
            try
            {
                SnapClientDatabase::Dispose();
            }
            catch (...)
            {
                // Exceptions are not propagated from destructor
            }
        }

        SnapClientDatabase(const SnapClientDatabase&) = delete;
        SnapClientDatabase& operator=(const SnapClientDatabase&) = delete;

        // Gets basic information about the database instance.
        const DatabaseInfoPtr& Info() const override
        {
            return m_info;
        }

        // Gets the name of the database instance.
        const std::string& Name() const
        {
            return m_info->DatabaseName();
        }

        // Gets flag that determines if the client database instance is disposed.
        bool IsDisposed() const override
        {
            return m_disposed;
        }

        // Gets the assigned encoding definition for this client database instance.
        const EncodingDefinition& GetEncodingDefinition() const
        {
            if (m_encoder == nullptr)
                throw SnapException("No encoding definition has been assigned");

            return m_encoder->Definition();
        }

        // Assigns an encoder based on encoding definition for this client database instance.
        // This is normally only called once after database is opened.
        void SetEncodingDefinition(const EncodingDefinition& definition)
        {
            if (m_disposed)
                throw SnapException("Cannot set encoding definition: SNAPdb instance \"" + Name() + "\" has been disposed");

            if (ReaderIsActive() || WriterIsActive())
                throw SnapException("Cannot set encoding definition while a read or bulk write operation is active");

            KeyValueEncoderPtr<TKey, TValue> encoder = EncoderLibrary<TKey, TValue>::CreateEncoder(definition);

            if (encoder == nullptr)
                throw SnapException("Encoding method " + definition.ToString() + " is not registered for SNAPdb instance key/value types");

            m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::SetEncodingMethod));
            definition.Save(*m_stream);
            m_stream->Flush();

            const ServerResponse response = ReadResponse(*m_stream);

            if (response == ServerResponse::UnknownEncodingMethod)
            {
                // Server closes connection after an unknown encoding method
                m_stream->Close();
                throw SnapException("SNAPdb server reports encoding method " + definition.ToString() + " is unrecognized, hence it is unsupported");
            }

            ValidateExpectedResponse(response, ServerResponse::EncodingMethodAccepted);
            m_encoder = encoder;
        }

        // Reads data from the SNAPdb client database instance with the provided server-side filters
        // and read options. Any parameter can be nullptr. Returns a stream that will read the specified
        // data. Only one reader can be active at a time, read stream to completion or dispose of reader
        // before executing another operation.
        TreeStreamPtr<TKey, TValue> Read(const SeekFilterBasePtr& keySeekFilter = nullptr, const MatchFilterBasePtr& keyMatchFilter = nullptr, const ReaderOptionsPtr& options = nullptr)
        {
            ValidateState("read");

            m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::Read));

            if (keySeekFilter == nullptr)
            {
                m_stream->WriteBoolean(false);
            }
            else
            {
                m_stream->WriteBoolean(true);
                m_stream->WriteGuid(keySeekFilter->TypeID());
                keySeekFilter->Save(*m_stream);
            }

            if (keyMatchFilter == nullptr)
            {
                m_stream->WriteBoolean(false);
            }
            else
            {
                m_stream->WriteBoolean(true);
                m_stream->WriteGuid(keyMatchFilter->TypeID());
                keyMatchFilter->Save(*m_stream);
            }

            if (options == nullptr)
            {
                m_stream->WriteBoolean(false);
            }
            else
            {
                m_stream->WriteBoolean(true);
                options->Save(*m_stream);
            }

            m_stream->Flush();

            const ServerResponse response = ReadResponse(*m_stream);
            std::string message;

            switch (response)
            {
                case ServerResponse::UnknownOrCorruptSeekFilter:
                    message = "SNAPdb server reports key seek filter is unrecognized or corrupted";
                    break;
                case ServerResponse::UnknownOrCorruptMatchFilter:
                    message = "SNAPdb server reports key match filter is unrecognized or corrupted";
                    break;
                case ServerResponse::UnknownOrCorruptReaderOptions:
                    message = "SNAPdb server reports reader options are unrecognized or corrupted";
                    break;
                case ServerResponse::ErrorWhileReading:
                    message = "SNAPdb server reported an exception while reading: " + m_stream->ReadString();
                    break;
                default:
                    break;
            }

            if (!message.empty())
            {
                // Server closes connection after a failed read request
                m_stream->Close();
                throw SnapException(message);
            }

            ValidateExpectedResponse(response, ServerResponse::SerializingPoints);

            PointReaderPtr<TKey, TValue> reader = NewSharedPtr<PointReader<TKey, TValue>>(m_encoder, m_stream);
            m_reader = reader;

            return reader;
        }

        // Reads data for the specified time range (inclusive), with times specified in ticks,
        // and point IDs. An empty point ID list reads all points.
        TreeStreamPtr<TKey, TValue> Read(uint64_t startTime, uint64_t endTime, const std::vector<uint64_t>& pointIDs = {}, const ReaderOptionsPtr& options = nullptr) requires TimestampPointIDType<TKey>
        {
            return Read(
                TimestampSeekFilter::CreateFromRange(startTime, endTime),
                pointIDs.empty() ? nullptr : PointIDMatchFilter::CreateFromList(pointIDs),
                options);
        }

        // Reads data for the specified time range (inclusive) and point IDs. An empty point ID list reads all points.
        TreeStreamPtr<TKey, TValue> Read(const datetime_t& startTime, const datetime_t& endTime, const std::vector<uint64_t>& pointIDs = {}, const ReaderOptionsPtr& options = nullptr) requires TimestampPointIDType<TKey>
        {
            return Read(
                TimestampSeekFilter::CreateFromRange(startTime, endTime),
                pointIDs.empty() ? nullptr : PointIDMatchFilter::CreateFromList(pointIDs),
                options);
        }

        // Reads data with the provided filters and options, invoking the callback for each key/value pair.
        // Callback should return true to continue reading, or false to cancel the read. Returns the number
        // of key/value pairs that were provided to the callback.
        uint64_t Read(const SeekFilterBasePtr& keySeekFilter, const MatchFilterBasePtr& keyMatchFilter, const ReaderOptionsPtr& options, const std::function<bool(const TKey&, const TValue&)>& callback)
        {
            const TreeStreamPtr<TKey, TValue> reader = Read(keySeekFilter, keyMatchFilter, options);
            TKey key;
            TValue value;
            uint64_t count = 0ULL;

            while (reader->Read(key, value))
            {
                count++;

                if (!callback(key, value))
                    break;
            }

            reader->Dispose();

            return count;
        }

        // Writes an individual key/value pair to the SNAPdb client database instance.
        void Write(const TKey& key, const TValue& value)
        {
            ValidateState("write");

            m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::Write));
            m_encoder->ResetEncoder();
            m_encoder->StreamEncode(*m_stream, key, value);
            m_encoder->WriteEndOfStream(*m_stream);
            m_stream->Flush();
        }

        // Writes all key/value pairs of the tree stream to the SNAPdb client database instance.
        void Write(TreeStream<TKey, TValue>& stream)
        {
            ValidateState("write");

            TKey key;
            TValue value;

            m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::Write));
            m_encoder->ResetEncoder();

            while (stream.Read(key, value))
                m_encoder->StreamEncode(*m_stream, key, value);

            m_encoder->WriteEndOfStream(*m_stream);
            m_stream->Flush();
        }

        // Writes all key/value pairs of the list to the SNAPdb client database instance.
        void Write(const std::vector<std::pair<TKey, TValue>>& values)
        {
            ValidateState("write");

            m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::Write));
            m_encoder->ResetEncoder();

            for (const auto& [key, value] : values)
                m_encoder->StreamEncode(*m_stream, key, value);

            m_encoder->WriteEndOfStream(*m_stream);
            m_stream->Flush();
        }

        // Starts a bulk write operation. Key/value pairs written to the returned writer are streamed to
        // the server as a single write operation. Dispose of writer to complete the write operation.
        BulkWriterPtr<TKey, TValue> StartBulkWriting()
        {
            ValidateState("start bulk writing");

            BulkWriterPtr<TKey, TValue> writer = NewSharedPtr<BulkWriter<TKey, TValue>>(m_encoder, m_stream);
            m_writer = writer;

            return writer;
        }

        // Disconnects from the SNAPdb client database instance. Any active
        // reader is canceled and any active bulk writer is completed.
        void Dispose() override
        {
            if (m_disposed)
                return;

            m_disposed = true;

            if (const PointReaderPtr<TKey, TValue> reader = m_reader.lock())
                reader->Dispose();

            if (const BulkWriterPtr<TKey, TValue> writer = m_writer.lock())
                writer->Dispose();

            if (!m_stream->IsOpen())
                return;

            m_stream->WriteByte(static_cast<uint8_t>(ServerCommand::DisconnectDatabase));
            m_stream->Flush();

            ValidateExpectedReadResponse(*m_stream, ServerResponse::DatabaseDisconnected);
        }
    };

    template<SnapType TKey, SnapType TValue>
    using SnapClientDatabasePtr = SharedPtr<SnapClientDatabase<TKey, TValue>>;
}
