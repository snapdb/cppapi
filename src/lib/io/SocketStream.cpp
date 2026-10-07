//******************************************************************************************************
//  SocketStream.cpp - Gbtc
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

#include "SocketStream.h"
#include "../SnapException.h"

using namespace std;
using namespace boost::asio;
using namespace snapdb;
using namespace snapdb::io;

SocketStream::SocketStream() :
    m_socket(m_ioContext),
    m_ioTimeout(0),
    m_connected(false)
{
}

SocketStream::~SocketStream()
{
    Close();
}

// Runs pending asynchronous operation until it completes or times out. This is the standard
// Boost.Asio pattern for blocking operations with a timeout, see blocking_tcp_client example.
void SocketStream::RunUntilComplete(const char* operation, const int32_t timeout)
{
    m_ioContext.restart();
    m_ioContext.run_for(std::chrono::milliseconds(timeout));

    if (m_ioContext.stopped())
        return;

    // Operation timed out, close socket to cancel outstanding operation and run until its handler completes.
    // Socket must be closed since stream state is unknown after a partial read or write.
    Close();
    m_ioContext.run();

    throw SnapTimeoutException("Socket " + string(operation) + " operation timed out after " + to_string(timeout) + "ms");
}

void SocketStream::Connect(const string& hostAddress, const uint16_t port, const int32_t timeout)
{
    Close();

    DnsResolver resolver(m_ioContext);
    const DnsResolver::results_type endPoints = resolver.resolve(hostAddress, to_string(port));
    ErrorCode errorCode = boost::asio::error::host_not_found;

    if (timeout > 0)
    {
        async_connect(m_socket, endPoints, [&](const ErrorCode& connectError, const TcpEndPoint& endPoint)
        {
            errorCode = connectError;
            m_endPoint = endPoint;
        });

        RunUntilComplete("connect", timeout);
    }
    else
    {
        m_endPoint = boost::asio::connect(m_socket, endPoints, errorCode);
    }

    if (errorCode)
    {
        Close();
        throw SystemError(errorCode);
    }

    m_socket.set_option(ip::tcp::no_delay(true));
    m_connected = true;
}

const TcpEndPoint& SocketStream::EndPoint() const
{
    return m_endPoint;
}

int32_t SocketStream::GetIOTimeout() const
{
    return m_ioTimeout;
}

void SocketStream::SetIOTimeout(const int32_t timeout)
{
    m_ioTimeout = timeout;
}

uint32_t SocketStream::Read(uint8_t* buffer, const uint32_t offset, const uint32_t count)
{
    if (!m_connected)
        throw SnapException("Cannot read from socket: socket is not connected");

    if (count == 0)
        return 0U;

    ErrorCode errorCode;
    size_t bytesRead = 0;

    if (m_ioTimeout > 0)
    {
        m_socket.async_read_some(boost::asio::buffer(buffer + offset, count), [&](const ErrorCode& readError, const size_t length)
        {
            errorCode = readError;
            bytesRead = length;
        });

        RunUntilComplete("read", m_ioTimeout);
    }
    else
    {
        bytesRead = m_socket.read_some(boost::asio::buffer(buffer + offset, count), errorCode);
    }

    if (errorCode == boost::asio::error::eof)
    {
        Close();
        return 0U;
    }

    if (errorCode)
    {
        Close();
        throw SystemError(errorCode);
    }

    return static_cast<uint32_t>(bytesRead);
}

void SocketStream::Write(const uint8_t* buffer, const uint32_t offset, const uint32_t count)
{
    if (!m_connected)
        throw SnapException("Cannot write to socket: socket is not connected");

    if (count == 0)
        return;

    ErrorCode errorCode;

    if (m_ioTimeout > 0)
    {
        async_write(m_socket, boost::asio::buffer(buffer + offset, count), [&](const ErrorCode& writeError, size_t)
        {
            errorCode = writeError;
        });

        RunUntilComplete("write", m_ioTimeout);
    }
    else
    {
        boost::asio::write(m_socket, boost::asio::buffer(buffer + offset, count), errorCode);
    }

    if (errorCode)
    {
        Close();
        throw SystemError(errorCode);
    }
}

void SocketStream::Close()
{
    m_connected = false;

    if (!m_socket.is_open())
        return;

    ErrorCode errorCode;
    m_socket.shutdown(socket_base::shutdown_both, errorCode);
    m_socket.close(errorCode);
}

bool SocketStream::IsOpen() const
{
    return m_connected;
}
