//******************************************************************************************************
//  Stream.h - Gbtc
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

#include "../CommonTypes.h"

namespace snapdb::io
{
    // Defines an abstract base stream that reads and writes bytes, e.g., a socket.
    class Stream
    {
    public:
        virtual ~Stream() = default;

        // Reads up to count bytes into buffer at offset. Returns number of bytes
        // read, which may be less than count; returns zero at end of stream.
        virtual uint32_t Read(uint8_t* buffer, uint32_t offset, uint32_t count) = 0;

        // Writes count bytes from buffer at offset to the stream.
        virtual void Write(const uint8_t* buffer, uint32_t offset, uint32_t count) = 0;

        // Flushes any pending writes to the base stream.
        virtual void Flush() { }

        // Closes the stream.
        virtual void Close() { }

        // Determines if the stream is open.
        virtual bool IsOpen() const { return true; }
    };

    typedef SharedPtr<Stream> StreamPtr;
}
