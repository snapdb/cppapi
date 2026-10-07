//******************************************************************************************************
//  KeyValueEncoderBase.h - Gbtc
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

#include "EncodingDefinition.h"

namespace snapdb::snap
{
    // Represents an encoding method that takes both a key and a value to encode. Encoders
    // maintain streaming state, i.e., previous key and value, so an encoder instance should
    // only be used for one stream at a time.
    template<SnapType TKey, SnapType TValue>
    class KeyValueEncoderBase
    {
    private:
        TKey m_prevKey;
        TValue m_prevValue;

    public:
        virtual ~KeyValueEncoderBase() = default;

        // Gets the encoding definition that this class implements.
        virtual const EncodingDefinition& Definition() const = 0;

        // Gets flag that determines if the previous key will need to be presented to the
        // encoding algorithms to properly encode the next sample.
        virtual bool UsesPreviousKey() const = 0;

        // Gets flag that determines if the previous value will need to be presented to the
        // encoding algorithms to properly encode the next sample.
        virtual bool UsesPreviousValue() const = 0;

        // Gets the maximum amount of space that is required for the compression algorithm.
        virtual uint32_t MaxCompressionSize() const = 0;

        // Gets flag that determines if the stream supports a symbol that represents that the end
        // of the stream has been encountered. If the encoding does not reserve a symbol for the end
        // of the stream, each streamed point will be prefixed with an extra byte.
        virtual bool ContainsEndOfStreamSymbol() const = 0;

        // Gets the byte code to use as the end of stream symbol. Only valid
        // when `ContainsEndOfStreamSymbol` is true.
        virtual uint8_t EndOfStreamSymbol() const = 0;

        // Encodes key and value to the provided stream.
        virtual void Encode(io::BinaryStream& stream, const TKey& prevKey, const TValue& prevValue, const TKey& key, const TValue& value) = 0;

        // Decodes key and value from the provided stream. When `ContainsEndOfStreamSymbol` is true, returns
        // true if the end of stream symbol was encountered; otherwise, returns false.
        virtual bool Decode(io::BinaryStream& stream, const TKey& prevKey, const TValue& prevValue, TKey& key, TValue& value) = 0;

        // Writes the end of stream symbol to the stream.
        void WriteEndOfStream(io::BinaryStream& stream)
        {
            stream.WriteByte(ContainsEndOfStreamSymbol() ? EndOfStreamSymbol() : 0);
        }

        // Encodes key and value to the stream, tracking previous key and value.
        void StreamEncode(io::BinaryStream& stream, const TKey& key, const TValue& value)
        {
            if (!ContainsEndOfStreamSymbol())
                stream.WriteByte(1);

            Encode(stream, m_prevKey, m_prevValue, key, value);

            key.CopyTo(m_prevKey);
            value.CopyTo(m_prevValue);
        }

        // Attempts to decode next key and value from the stream. Returns false at end of stream.
        bool TryStreamDecode(io::BinaryStream& stream, TKey& key, TValue& value)
        {
            if (!ContainsEndOfStreamSymbol() && stream.ReadByte() == 0)
                return false;

            if (Decode(stream, m_prevKey, m_prevValue, key, value))
                return false;

            key.CopyTo(m_prevKey);
            value.CopyTo(m_prevValue);

            return true;
        }

        // Resets the encoder. Encoders maintain streaming state that
        // should be reset when reading from or writing to a new stream.
        void ResetEncoder()
        {
            m_prevKey.Clear();
            m_prevValue.Clear();
        }
    };

    template<SnapType TKey, SnapType TValue>
    using KeyValueEncoderPtr = SharedPtr<KeyValueEncoderBase<TKey, TValue>>;
}
