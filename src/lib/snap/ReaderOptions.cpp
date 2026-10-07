//******************************************************************************************************
//  ReaderOptions.cpp - Gbtc
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

#include "ReaderOptions.h"
#include "../Convert.h"

using namespace std;
using namespace snapdb;
using namespace snapdb::io;
using namespace snapdb::snap;

ReaderOptions::ReaderOptions(const TimeSpan& timeout, const int64_t maxReturnedCount, const int64_t maxScanCount, const int64_t maxSeekCount) :
    m_timeout(timeout),
    m_maxReturnedCount(maxReturnedCount),
    m_maxScanCount(maxScanCount),
    m_maxSeekCount(maxSeekCount)
{
}

const TimeSpan& ReaderOptions::Timeout() const
{
    return m_timeout;
}

int64_t ReaderOptions::MaxReturnedCount() const
{
    return m_maxReturnedCount;
}

int64_t ReaderOptions::MaxScanCount() const
{
    return m_maxScanCount;
}

int64_t ReaderOptions::MaxSeekCount() const
{
    return m_maxSeekCount;
}

void ReaderOptions::Save(BinaryStream& stream) const
{
    stream.WriteByte(0); // Version
    stream.WriteInt64(ToTicks(m_timeout));
    stream.WriteInt64(m_maxReturnedCount);
    stream.WriteInt64(m_maxScanCount);
    stream.WriteInt64(m_maxSeekCount);
}
