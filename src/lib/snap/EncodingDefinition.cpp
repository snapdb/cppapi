//******************************************************************************************************
//  EncodingDefinition.cpp - Gbtc
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

#include "EncodingDefinition.h"
#include "../Convert.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::snap;

// {1DEA326D-A63A-4F73-B51C-7B3125C6DA55}
const Guid EncodingDefinition::FixedSizeIndividualGuid = ParseGuid("1dea326d-a63a-4f73-b51c-7b3125c6da55");

const EncodingDefinition EncodingDefinition::FixedSizeCombinedEncoding = CreateKeyValueEncoding(FixedSizeIndividualGuid);

EncodingDefinition::EncodingDefinition(const Guid& keyEncoding, const Guid& valueEncoding, const Guid& keyValueEncoding, const bool isKeyValueEncoded) :
    m_keyEncodingMethod(keyEncoding),
    m_valueEncodingMethod(valueEncoding),
    m_keyValueEncodingMethod(keyValueEncoding),
    m_isKeyValueEncoded(isKeyValueEncoded)
{
    m_isFixedSizeEncoding = isKeyValueEncoded ?
        m_keyValueEncodingMethod == FixedSizeIndividualGuid :
        m_keyEncodingMethod == FixedSizeIndividualGuid && m_valueEncodingMethod == FixedSizeIndividualGuid;
}

EncodingDefinition::EncodingDefinition(BinaryStream& stream) :
    m_keyEncodingMethod(Empty::Guid),
    m_valueEncodingMethod(Empty::Guid),
    m_keyValueEncodingMethod(Empty::Guid),
    m_isKeyValueEncoded(false)
{
    const uint8_t code = stream.ReadByte();

    switch (code)
    {
        case 1:
            m_keyValueEncodingMethod = stream.ReadGuid();
            m_isKeyValueEncoded = true;
            break;
        case 2:
            m_keyEncodingMethod = stream.ReadGuid();
            m_valueEncodingMethod = stream.ReadGuid();
            break;
        default:
            throw SnapException("Unknown SNAPdb encoding definition version: " + to_string(code));
    }

    m_isFixedSizeEncoding = m_isKeyValueEncoded ?
        m_keyValueEncodingMethod == FixedSizeIndividualGuid :
        m_keyEncodingMethod == FixedSizeIndividualGuid && m_valueEncodingMethod == FixedSizeIndividualGuid;
}

EncodingDefinition EncodingDefinition::CreateKeyValueEncoding(const Guid& keyValueEncoding)
{
    return { Guid{}, Guid{}, keyValueEncoding, true };
}

EncodingDefinition EncodingDefinition::CreateIndividualEncoding(const Guid& keyEncoding, const Guid& valueEncoding)
{
    return { keyEncoding, valueEncoding, Guid{}, false };
}

bool EncodingDefinition::IsKeyValueEncoded() const
{
    return m_isKeyValueEncoded;
}

bool EncodingDefinition::IsFixedSizeEncoding() const
{
    return m_isFixedSizeEncoding;
}

const Guid& EncodingDefinition::KeyEncodingMethod() const
{
    if (m_isKeyValueEncoded)
        throw SnapException("Key encoding method is not valid for a combined key/value encoding definition");

    return m_keyEncodingMethod;
}

const Guid& EncodingDefinition::ValueEncodingMethod() const
{
    if (m_isKeyValueEncoded)
        throw SnapException("Value encoding method is not valid for a combined key/value encoding definition");

    return m_valueEncodingMethod;
}

const Guid& EncodingDefinition::KeyValueEncodingMethod() const
{
    if (!m_isKeyValueEncoded)
        throw SnapException("Key/value encoding method is not valid for an individual encoding definition");

    return m_keyValueEncodingMethod;
}

string EncodingDefinition::ToString() const
{
    if (m_isKeyValueEncoded)
        return "{" + snapdb::ToString(m_keyValueEncodingMethod) + "}";

    return "{" + snapdb::ToString(m_keyEncodingMethod) + "} / {" + snapdb::ToString(m_valueEncodingMethod) + "}";
}

void EncodingDefinition::Save(BinaryStream& stream) const
{
    if (m_isKeyValueEncoded)
    {
        stream.WriteByte(1);
        stream.WriteGuid(m_keyValueEncodingMethod);
    }
    else
    {
        stream.WriteByte(2);
        stream.WriteGuid(m_keyEncodingMethod);
        stream.WriteGuid(m_valueEncodingMethod);
    }
}

bool EncodingDefinition::operator==(const EncodingDefinition& other) const
{
    if (m_isKeyValueEncoded != other.m_isKeyValueEncoded)
        return false;

    if (m_isKeyValueEncoded)
        return m_keyValueEncodingMethod == other.m_keyValueEncodingMethod;

    return m_keyEncodingMethod == other.m_keyEncodingMethod && m_valueEncodingMethod == other.m_valueEncodingMethod;
}

bool EncodingDefinition::operator!=(const EncodingDefinition& other) const
{
    return !(*this == other);
}
