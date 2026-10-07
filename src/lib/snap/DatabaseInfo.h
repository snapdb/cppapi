//******************************************************************************************************
//  DatabaseInfo.h - Gbtc
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
    // Defines details for a SNAPdb client database. From the perspective of
    // the openHistorian API, client databases are known as "instances".
    class DatabaseInfo final
    {
    private:
        uint8_t m_version;
        std::string m_databaseName;
        Guid m_keyTypeID;
        Guid m_valueTypeID;
        std::string m_keyTypeName;
        std::string m_valueTypeName;
        std::vector<EncodingDefinition> m_supportedEncodings;

    public:
        // Creates database information from a stream.
        DatabaseInfo(io::BinaryStream& stream);

        // Gets the database information serialization version.
        uint8_t Version() const;

        // Gets the database name, always upper case.
        const std::string& DatabaseName() const;

        // Gets the SNAPdb type ID of the database key.
        const Guid& KeyTypeID() const;

        // Gets the SNAPdb type ID of the database value.
        const Guid& ValueTypeID() const;

        // Gets the registered type name of the database key, or empty string if the type is not registered.
        const std::string& KeyTypeName() const;

        // Gets the registered type name of the database value, or empty string if the type is not registered.
        const std::string& ValueTypeName() const;

        // Gets the encoding methods supported by the database.
        const std::vector<EncodingDefinition>& SupportedEncodings() const;
    };

    typedef SharedPtr<DatabaseInfo> DatabaseInfoPtr;
}
