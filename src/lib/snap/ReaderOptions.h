//******************************************************************************************************
//  ReaderOptions.h - Gbtc
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
    // Contains the options to use for executing an individual read request. Note that current
    // SNAPdb servers only enforce the timeout option, the maximum count options are serialized
    // for protocol compatibility but are not enforced by the server.
    class ReaderOptions final
    {
    private:
        TimeSpan m_timeout;
        int64_t m_maxReturnedCount;
        int64_t m_maxScanCount;
        int64_t m_maxSeekCount;

    public:
        // Creates new reader options. Zero values mean no limit.
        ReaderOptions(const TimeSpan& timeout = TimeSpan(0, 0, 0), int64_t maxReturnedCount = 0, int64_t maxScanCount = 0, int64_t maxSeekCount = 0);

        // Gets the time before the query times out. Zero means no timeout.
        const TimeSpan& Timeout() const;

        // Gets the maximum number of points to return. Zero means no limit.
        int64_t MaxReturnedCount() const;

        // Gets the maximum number of points to scan to get the result set, this includes
        // any point that was filtered. Zero means no limit.
        int64_t MaxScanCount() const;

        // Gets the maximum number of seeks permitted. Zero means no limit.
        int64_t MaxSeekCount() const;

        // Serializes reader options to a stream.
        void Save(io::BinaryStream& stream) const;
    };

    typedef SharedPtr<ReaderOptions> ReaderOptionsPtr;
}
