//******************************************************************************************************
//  Library.cpp - Gbtc
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

#include "Library.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::snap;

namespace
{
    unordered_map<Guid, string>& TypeIDNameMap()
    {
        static unordered_map<Guid, string> typeIDNameMap;
        return typeIDNameMap;
    }

    StringMap<Guid>& TypeNameIDMap()
    {
        static StringMap<Guid> typeNameIDMap;
        return typeNameIDMap;
    }

    SharedMutex& TypeMapLock()
    {
        static SharedMutex lock;
        return lock;
    }
}

void Library::RegisterType(const Guid& typeID, const string& typeName)
{
    WriterLock lock(TypeMapLock());
    TypeIDNameMap()[typeID] = typeName;
    TypeNameIDMap()[typeName] = typeID;
}

bool Library::TryLookupTypeName(const Guid& typeID, string& typeName)
{
    ReaderLock lock(TypeMapLock());
    const auto iterator = TypeIDNameMap().find(typeID);

    if (iterator == TypeIDNameMap().end())
    {
        typeName.clear();
        return false;
    }

    typeName = iterator->second;
    return true;
}

string Library::LookupTypeName(const Guid& typeID)
{
    string typeName;
    TryLookupTypeName(typeID, typeName);
    return typeName;
}

bool Library::TryLookupTypeID(const string& typeName, Guid& typeID)
{
    ReaderLock lock(TypeMapLock());
    const auto iterator = TypeNameIDMap().find(typeName);

    if (iterator == TypeNameIDMap().end())
    {
        typeID = Guid{};
        return false;
    }

    typeID = iterator->second;
    return true;
}
