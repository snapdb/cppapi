//******************************************************************************************************
//  HistorianKeyValueEncoder.h - Gbtc
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

#include "../snap/KeyValueEncoderBase.h"
#include "HistorianKey.h"
#include "HistorianValue.h"

namespace snapdb::openhistorian
{
    // Represents an openHistorian specific SNAPdb stream encoder, i.e., an
    // encoder for the `HistorianKey` and `HistorianValue` types.
    class HistorianKeyValueEncoder final : public snap::KeyValueEncoderBase<HistorianKey, HistorianValue>
    {
    public:
        // Guid uniquely defining this SNAPdb encoding method.
        static const Guid SnapTypeID;

        // Defines the end of stream symbol for this encoding method.
        static constexpr uint8_t EndOfStream = 0xFF;

        // Defines the encoding definition for this encoding method.
        static const snap::EncodingDefinition Encoding;

        const snap::EncodingDefinition& Definition() const override;
        bool UsesPreviousKey() const override;
        bool UsesPreviousValue() const override;
        uint32_t MaxCompressionSize() const override;
        bool ContainsEndOfStreamSymbol() const override;
        uint8_t EndOfStreamSymbol() const override;
        void Encode(io::BinaryStream& stream, const HistorianKey& prevKey, const HistorianValue& prevValue, const HistorianKey& key, const HistorianValue& value) override;
        bool Decode(io::BinaryStream& stream, const HistorianKey& prevKey, const HistorianValue& prevValue, HistorianKey& key, HistorianValue& value) override;
    };
}
