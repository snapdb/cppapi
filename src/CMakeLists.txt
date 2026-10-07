#******************************************************************************************************
#  CMakeLists.txt - Gbtc
#
#  Copyright © 2026, Grid Protection Alliance.  All Rights Reserved.
#
#  Licensed to the Grid Protection Alliance (GPA) under one or more contributor license agreements. See
#  the NOTICE file distributed with this work for additional information regarding copyright ownership.
#  The GPA licenses this file to you under the MIT License (MIT), the "License"; you may not use this
#  file except in compliance with the License. You may obtain a copy of the License at:
#
#      http://opensource.org/licenses/MIT
#
#  Unless agreed to in writing, the subject software distributed under the License is distributed on an
#  "AS-IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. Refer to the
#  License for the specific language governing permissions and limitations.
#
#  Code Modification History:
#  ----------------------------------------------------------------------------------------------------
#  10/07/2026 - J. Ritchie Carroll
#       Generated original version of source code.
#
#******************************************************************************************************

cmake_minimum_required (VERSION 3.16)
project (snapdb)

set (HEADER_OUTPUT_DIRECTORY Include/snapdb)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY Output)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY Libraries)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY Libraries)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

set (SNAPDB_HEADER_FILES
    "lib/CommonTypes.h"
    "lib/Convert.h"
    "lib/EndianConverter.h"
    "lib/SnapException.h"
    "lib/Version.h"
    "lib/pugiconfig.hpp"
    "lib/pugixml.hpp"
    "lib/io/BinaryStream.h"
    "lib/io/BufferStream.h"
    "lib/io/Encoding7Bit.h"
    "lib/io/SocketStream.h"
    "lib/io/Stream.h"
    "lib/snap/DatabaseInfo.h"
    "lib/snap/EncodingDefinition.h"
    "lib/snap/Filters.h"
    "lib/snap/FixedSizeKeyValueEncoder.h"
    "lib/snap/KeyValueEncoderBase.h"
    "lib/snap/Library.h"
    "lib/snap/ReaderOptions.h"
    "lib/snap/SnapClientDatabase.h"
    "lib/snap/SnapConnection.h"
    "lib/snap/SnapTypes.h"
    "lib/snap/TreeStream.h"
    "lib/openhistorian/HistorianConnection.h"
    "lib/openhistorian/HistorianKey.h"
    "lib/openhistorian/HistorianKeyValueEncoder.h"
    "lib/openhistorian/HistorianTypes.h"
    "lib/openhistorian/HistorianValue.h"
    "lib/openhistorian/MetadataCache.h"
)

set (SNAPDB_SOURCE_FILES
    "lib/CommonTypes.cpp"
    "lib/Convert.cpp"
    "lib/EndianConverter.cpp"
    "lib/SnapException.cpp"
    "lib/pugixml.cpp"
    "lib/io/BinaryStream.cpp"
    "lib/io/BufferStream.cpp"
    "lib/io/SocketStream.cpp"
    "lib/snap/DatabaseInfo.cpp"
    "lib/snap/EncodingDefinition.cpp"
    "lib/snap/Filters.cpp"
    "lib/snap/Library.cpp"
    "lib/snap/ReaderOptions.cpp"
    "lib/snap/SnapConnection.cpp"
    "lib/snap/SnapTypes.cpp"
    "lib/openhistorian/HistorianConnection.cpp"
    "lib/openhistorian/HistorianKey.cpp"
    "lib/openhistorian/HistorianKeyValueEncoder.cpp"
    "lib/openhistorian/HistorianTypes.cpp"
    "lib/openhistorian/HistorianValue.cpp"
    "lib/openhistorian/MetadataCache.cpp"
)

# Option to choose whether to build static or shared libraries
option (BUILD_SHARED_LIBS "Build SNAPdb using shared libraries" OFF)

# Copy header files
foreach (headerFile ${SNAPDB_HEADER_FILES})
    string (REGEX MATCH "(.*)[/\\]" DIR ${headerFile})
    file (COPY ${headerFile} DESTINATION ${HEADER_OUTPUT_DIRECTORY}/${DIR})
endforeach (headerFile)

# Discover the Boost headers and version used by this build.
find_package (Boost REQUIRED)

# Define include directories
include_directories(
    ${Boost_INCLUDE_DIRS}
)

# Define macros
add_definitions(
   -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
   -DBOOST_ASIO_ENABLE_CANCELIO
)

# Define compile flags
if (MSVC)
    add_definitions(-D_WIN32_WINNT=0x0601 -D_SILENCE_FPOS_SEEKPOS_DEPRECATION_WARNING)
    add_compile_options(/utf-8 /bigobj /permissive-)

    # Boost libraries are auto-linked on Windows, define library search path
    if (Boost_LIBRARY_DIRS)
        link_directories(${Boost_LIBRARY_DIRS})
    else ()
        link_directories(${Boost_INCLUDE_DIRS}/stage/lib)
    endif ()
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftabstop=4 -finput-charset=UTF-8 -fextended-identifiers -Wno-attributes -Wno-unknown-pragmas")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif ()

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
endif ()

# Build SNAPdb library
add_library (snapdb ${SNAPDB_SOURCE_FILES})

if (NOT MSVC)
    target_link_libraries (snapdb pthread m z bz2 boost_thread boost_date_time boost_iostreams boost_chrono)

    # Boost.System is header-only from 1.69; its compatibility library was removed in 1.89.
    # Use the major/minor variables, which are available in both old FindBoost and BoostConfig.
    if ("${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}" VERSION_LESS "1.69")
        target_link_libraries (snapdb boost_system)
    endif ()
endif ()

# Install headers and library
install (DIRECTORY ${PROJECT_BINARY_DIR}/${HEADER_OUTPUT_DIRECTORY} DESTINATION include)
install (TARGETS snapdb DESTINATION lib)

######################
# SAMPLE EXECUTABLES #
######################

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY Samples)

# ReadTest sample
add_executable (ReadTest EXCLUDE_FROM_ALL samples/ReadTest/ReadTest.cpp)
target_link_libraries (ReadTest snapdb)

# WriteTest sample
add_executable (WriteTest EXCLUDE_FROM_ALL samples/WriteTest/WriteTest.cpp)
target_link_libraries (WriteTest snapdb)

# HistorianTest sample
add_executable (HistorianTest EXCLUDE_FROM_ALL samples/HistorianTest/HistorianTest.cpp)
target_link_libraries (HistorianTest snapdb)

# BulkWriteTest sample
add_executable (BulkWriteTest EXCLUDE_FROM_ALL samples/BulkWriteTest/BulkWriteTest.cpp)
target_link_libraries (BulkWriteTest snapdb)

# MetadataTest sample
add_executable (MetadataTest EXCLUDE_FROM_ALL samples/MetadataTest/MetadataTest.cpp)
target_link_libraries (MetadataTest snapdb)

# Build with 'make samples'
add_custom_target (samples DEPENDS
    ReadTest
    WriteTest
    HistorianTest
    BulkWriteTest
    MetadataTest
)

##############
# UNIT TESTS #
##############

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY Tests)

# UnitTests do not require a running openHistorian
add_executable (UnitTests EXCLUDE_FROM_ALL tests/UnitTests.cpp)
target_link_libraries (UnitTests snapdb)

# Build with 'make tests', run with 'ctest' or 'make check'
add_custom_target (tests DEPENDS UnitTests)
add_custom_target (check COMMAND UnitTests DEPENDS UnitTests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

enable_testing ()
add_test (NAME UnitTests COMMAND UnitTests)
