//******************************************************************************************************
//  EncodingDefinition.h - Gbtc
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
    // Represents an immutable definition of the compression method used by the SNAPdb
    // sorted tree store. Encodings are either combined key/value encodings or separate
    // individual key and value encodings.
    class EncodingDefinition final
    {
    private:
        Guid m_keyEncodingMethod;
        Guid m_valueEncodingMethod;
        Guid m_keyValueEncodingMethod;
        bool m_isKeyValueEncoded;
        bool m_isFixedSizeEncoding;

        EncodingDefinition(const Guid& keyEncoding, const Guid& valueEncoding, const Guid& keyValueEncoding, bool isKeyValueEncoded);

    public:
        // Guid that represents the fixed size, i.e., no compression, encoding method.
        static const Guid FixedSizeIndividualGuid;

        // Defines the fixed size combined key/value encoding definition.
        static const EncodingDefinition FixedSizeCombinedEncoding;

        // Creates an encoding definition from a stream.
        EncodingDefinition(io::BinaryStream& stream);

        // Creates an encoding definition for a combined key/value encoding method.
        static EncodingDefinition CreateKeyValueEncoding(const Guid& keyValueEncoding);

        // Creates an encoding definition for an encoding method that compresses the key and the value independently.
        static EncodingDefinition CreateIndividualEncoding(const Guid& keyEncoding, const Guid& valueEncoding);

        // Gets flag that determines if encoding is a combined key/value encoding method.
        bool IsKeyValueEncoded() const;

        // Gets flag that determines if encoding is fixed size, i.e., uncompressed.
        bool IsFixedSizeEncoding() const;

        // Gets the key encoding method. Throws when encoding is a combined key/value encoding.
        const Guid& KeyEncodingMethod() const;

        // Gets the value encoding method. Throws when encoding is a combined key/value encoding.
        const Guid& ValueEncodingMethod() const;

        // Gets the combined key/value encoding method. Throws when encoding is not a combined key/value encoding.
        const Guid& KeyValueEncodingMethod() const;

        // Gets encoding definition as a string.
        std::string ToString() const;

        // Serializes encoding definition to a stream.
        void Save(io::BinaryStream& stream) const;

        bool operator==(const EncodingDefinition& other) const;
        bool operator!=(const EncodingDefinition& other) const;
    };

    typedef SharedPtr<EncodingDefinition> EncodingDefinitionPtr;
}
