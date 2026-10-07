//******************************************************************************************************
//  BufferStream.h - Gbtc
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

#include "Stream.h"

namespace snapdb::io
{
    // Defines an in-memory stream where writes append to the buffer
    // and reads advance from the current position, e.g., for testing.
    class BufferStream final : public Stream
    {
    private:
        std::vector<uint8_t> m_buffer;
        uint32_t m_position;

    public:
        BufferStream();
        BufferStream(std::vector<uint8_t> buffer);

        uint32_t Read(uint8_t* buffer, uint32_t offset, uint32_t count) override;
        void Write(const uint8_t* buffer, uint32_t offset, uint32_t count) override;

        // Gets the underlying buffer.
        const std::vector<uint8_t>& Buffer() const;

        // Gets current read position.
        uint32_t Position() const;

        // Sets current read position.
        void SetPosition(uint32_t position);

        // Gets the number of bytes remaining to be read.
        uint32_t Remaining() const;

        // Clears the buffer and resets the read position.
        void Clear();
    };

    typedef SharedPtr<BufferStream> BufferStreamPtr;
}
