//******************************************************************************************************
//  Library.h - Gbtc
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

#include "FixedSizeKeyValueEncoder.h"
#include <functional>

namespace snapdb::snap
{
    // Registered library of SNAPdb type names.
    class Library final
    {
    public:
        // Registers a SNAPdb type ID with its type name.
        static void RegisterType(const Guid& typeID, const std::string& typeName);

        // Registers a SNAPdb type with its type name.
        template<SnapType T>
        static void RegisterType(const std::string& typeName)
        {
            RegisterType(T::TypeID, typeName);
        }

        // Attempts to lookup type name for SNAPdb type ID.
        static bool TryLookupTypeName(const Guid& typeID, std::string& typeName);

        // Gets type name for SNAPdb type ID, or empty string if type ID is not registered.
        static std::string LookupTypeName(const Guid& typeID);

        // Attempts to lookup SNAPdb type ID for type name.
        static bool TryLookupTypeID(const std::string& typeName, Guid& typeID);
    };

    // Registered library of SNAPdb key/value encoders for a specific key and value type.
    template<SnapType TKey, SnapType TValue>
    class EncoderLibrary final
    {
    public:
        typedef std::function<KeyValueEncoderPtr<TKey, TValue>()> EncoderFactory;

    private:
        static std::unordered_map<Guid, EncoderFactory>& Registry()
        {
            static std::unordered_map<Guid, EncoderFactory> registry;
            return registry;
        }

        static Mutex& RegistryLock()
        {
            static Mutex lock;
            return lock;
        }

    public:
        // Registers a factory for a combined key/value encoding method.
        static void RegisterEncoder(const Guid& keyValueEncodingMethod, EncoderFactory factory)
        {
            ScopeLock lock(RegistryLock());
            Registry()[keyValueEncodingMethod] = std::move(factory);
        }

        // Determines if an encoder can be created for the specified encoding definition.
        static bool IsSupported(const EncodingDefinition& definition)
        {
            if (!definition.IsKeyValueEncoded())
                return false;

            if (definition.IsFixedSizeEncoding())
                return true;

            ScopeLock lock(RegistryLock());
            return Registry().contains(definition.KeyValueEncodingMethod());
        }

        // Creates a new encoder for the specified encoding definition, or nullptr if no encoder is
        // registered. Fixed size encoding is always supported. Note that separate key and value
        // encodings are not currently supported.
        static KeyValueEncoderPtr<TKey, TValue> CreateEncoder(const EncodingDefinition& definition)
        {
            if (!definition.IsKeyValueEncoded())
                return nullptr;

            EncoderFactory factory;

            {
                ScopeLock lock(RegistryLock());
                const auto iterator = Registry().find(definition.KeyValueEncodingMethod());

                if (iterator != Registry().end())
                    factory = iterator->second;
            }

            if (factory)
                return factory();

            if (definition.IsFixedSizeEncoding())
                return NewSharedPtr<FixedSizeKeyValueEncoder<TKey, TValue>>();

            return nullptr;
        }
    };
}
