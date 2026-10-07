//******************************************************************************************************
//  DatabaseInfo.cpp - Gbtc
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

#include "DatabaseInfo.h"
#include "Library.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::snap;

DatabaseInfo::DatabaseInfo(BinaryStream& stream)
{
    m_version = stream.ReadByte();

    if (m_version != 1)
        throw SnapException("Unknown SNAPdb database info version: " + to_string(m_version));

    m_databaseName = ToUpper(Trim(stream.ReadString()));
    m_keyTypeID = stream.ReadGuid();
    m_valueTypeID = stream.ReadGuid();

    const int32_t count = stream.ReadInt32();

    if (count < 0)
        throw SnapException("Invalid SNAPdb database info encoding count: " + to_string(count));

    m_supportedEncodings.reserve(count);

    for (int32_t i = 0; i < count; i++)
        m_supportedEncodings.emplace_back(stream);

    m_keyTypeName = Library::LookupTypeName(m_keyTypeID);
    m_valueTypeName = Library::LookupTypeName(m_valueTypeID);
}

uint8_t DatabaseInfo::Version() const
{
    return m_version;
}

const string& DatabaseInfo::DatabaseName() const
{
    return m_databaseName;
}

const Guid& DatabaseInfo::KeyTypeID() const
{
    return m_keyTypeID;
}

const Guid& DatabaseInfo::ValueTypeID() const
{
    return m_valueTypeID;
}

const string& DatabaseInfo::KeyTypeName() const
{
    return m_keyTypeName;
}

const string& DatabaseInfo::ValueTypeName() const
{
    return m_valueTypeName;
}

const vector<EncodingDefinition>& DatabaseInfo::SupportedEncodings() const
{
    return m_supportedEncodings;
}
