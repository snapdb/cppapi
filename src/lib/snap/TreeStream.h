//******************************************************************************************************
//  TreeStream.h - Gbtc
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

#include "SnapTypes.h"

namespace snapdb::snap
{
    // Represents a stream of SNAPdb key/value pairs.
    template<SnapType TKey, SnapType TValue>
    class TreeStream
    {
    private:
        bool m_eos = false;
        bool m_disposed = false;

    protected:
        // Advances the stream to the next value. If before the beginning of the stream, advances to the first
        // value. Returns true if the advance was successful; otherwise, false if end of the stream was reached.
        virtual bool ReadNext(TKey& key, TValue& value) = 0;

        // Derived classes should override this method for custom shutdown operations.
        virtual void Disposing() { }

        // Occurs when the end of the stream has been reached. The default behavior is to call `Dispose`.
        virtual void EndOfStreamReached()
        {
            Dispose();
        }

    public:
        virtual ~TreeStream() = default;

        // Gets flag that determines if the end of the stream has been read or stream has been disposed.
        bool IsEOS() const
        {
            return m_eos;
        }

        // Gets flag that determines if stream has been disposed.
        bool IsDisposed() const
        {
            return m_disposed;
        }

        // Advances the stream to the next key/value pair. Returns true if a pair was read;
        // otherwise, false if end of the stream was reached.
        bool Read(TKey& key, TValue& value)
        {
            if (m_eos || !ReadNext(key, value))
            {
                EndOfStreamReached();
                return false;
            }

            return true;
        }

        // Disposes of the stream.
        void Dispose()
        {
            if (m_disposed)
                return;

            try
            {
                Disposing();
            }
            catch (...)
            {
                m_eos = true;
                m_disposed = true;
                throw;
            }

            m_eos = true;
            m_disposed = true;
        }
    };

    template<SnapType TKey, SnapType TValue>
    using TreeStreamPtr = SharedPtr<TreeStream<TKey, TValue>>;

    // Represents a tree stream over a list of key/value pairs, e.g., for writing.
    template<SnapType TKey, SnapType TValue>
    class VectorTreeStream final : public TreeStream<TKey, TValue>
    {
    private:
        const std::vector<std::pair<TKey, TValue>>& m_values;
        size_t m_index = 0;

    protected:
        bool ReadNext(TKey& key, TValue& value) override
        {
            if (m_index >= m_values.size())
                return false;

            const auto& [nextKey, nextValue] = m_values[m_index++];
            nextKey.CopyTo(key);
            nextValue.CopyTo(value);

            return true;
        }

    public:
        // Creates a tree stream over the provided key/value pairs. The source list must outlive the stream.
        VectorTreeStream(const std::vector<std::pair<TKey, TValue>>& values) :
            m_values(values)
        {
        }
    };
}
