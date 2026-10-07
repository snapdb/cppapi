//******************************************************************************************************
//  FixedSizeKeyValueEncoder.h - Gbtc
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

#include "KeyValueEncoderBase.h"

namespace snapdb::snap
{
    // An encoding method that is fixed in size and calls the native read/write functions
    // of the key and value types, i.e., no compression is applied.
    template<SnapType TKey, SnapType TValue>
    class FixedSizeKeyValueEncoder final : public KeyValueEncoderBase<TKey, TValue>
    {
    public:
        const EncodingDefinition& Definition() const override
        {
            return EncodingDefinition::FixedSizeCombinedEncoding;
        }

        bool UsesPreviousKey() const override
        {
            return false;
        }

        bool UsesPreviousValue() const override
        {
            return false;
        }

        uint32_t MaxCompressionSize() const override
        {
            return TKey::Size + TValue::Size;
        }

        bool ContainsEndOfStreamSymbol() const override
        {
            return false;
        }

        uint8_t EndOfStreamSymbol() const override
        {
            throw SnapException("Fixed size encoding does not define an end of stream symbol");
        }

        void Encode(io::BinaryStream& stream, const TKey&, const TValue&, const TKey& key, const TValue& value) override
        {
            key.Write(stream);
            value.Write(stream);
        }

        bool Decode(io::BinaryStream& stream, const TKey&, const TValue&, TKey& key, TValue& value) override
        {
            key.Read(stream);
            value.Read(stream);
            return false;
        }
    };
}
