//******************************************************************************************************
//  SocketStream.h - Gbtc
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
    // Defines a blocking TCP socket stream with optional timeouts.
    class SocketStream final : public Stream
    {
    private:
        IOContext m_ioContext;
        TcpSocket m_socket;
        TcpEndPoint m_endPoint;
        int32_t m_ioTimeout;
        bool m_connected;

        void RunUntilComplete(const char* operation, int32_t timeout);

    public:
        SocketStream();
        ~SocketStream() override;

        // Connects socket to the specified host address and port. Timeout is in milliseconds,
        // a value of zero or less means wait indefinitely.
        void Connect(const std::string& hostAddress, uint16_t port, int32_t timeout = 0);

        // Gets the end point of the connected host.
        const TcpEndPoint& EndPoint() const;

        // Gets the socket read/write timeout, in milliseconds. A value of zero or less means wait indefinitely.
        int32_t GetIOTimeout() const;

        // Sets the socket read/write timeout, in milliseconds. A value of zero or less means wait indefinitely.
        void SetIOTimeout(int32_t timeout);

        uint32_t Read(uint8_t* buffer, uint32_t offset, uint32_t count) override;
        void Write(const uint8_t* buffer, uint32_t offset, uint32_t count) override;
        void Close() override;
        bool IsOpen() const override;
    };

    typedef SharedPtr<SocketStream> SocketStreamPtr;
}
