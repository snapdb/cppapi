# Implementation of the SNAPdb / openHistorian API in C++.

Code includes SNAPdb client functionality for reading and writing openHistorian data, plus openHistorian metadata queries.

Build instructions follow:

* [Windows](#compiling-in-visual-studio) (using [Visual Studio 2026](https://visualstudio.microsoft.com/vs/community/))
* [Unix Variants](#compiling-in-linux) (using [CMake](https://cmake.org/))

## Compiling in Visual Studio

To properly compile in Visual Studio, you will need to download Boost:
    http://www.boost.org/users/download/

By default, the C++ SNAPdb API project configuration adds an additional include
directory for the Boost libraries in a parallel location to the API project in
a folder called _boost_ regardless of version, for example:

SNAPdb API project files:
```
    C:\projects\snapdb\cppapi
                       \src
                       \build
                       etc...
```
Boost library files:
```
    C:\projects\snapdb\boost
                       \doc
                       \libs
                       etc...
```

If you have an existing Boost installation you can simply create a symbolic
link to the folder, e.g.:
```cmd
mklink /D C:\projects\snapdb\boost C:\boost_1_92_0
```

Alternately you can specify the Boost location with the `BoostRoot` property,
for example, when building from the command line:
```cmd
msbuild snapdb.cpp.sln /p:Configuration=Release /p:Platform=x64 /p:BoostRoot=C:\boost_1_92_0
```

The code has been tested with v1.92 of Boost.

Note that you will need to compile Boost in order to link the library and samples. The SNAPdb
API library requires the zlib features of Boost for decompressing openHistorian metadata, as a
result it is necessary to compile boost with access to zlib source code that can be downloaded
separately: https://zlib.net/

After unzipping the zlib source code and running the Boost `bootstrap.bat` script,
run  the `.\b2` build application with the following zlib parameters, adjusting
the paths to the directory where the zlib source code was unzipped:
```cmd
b2 -s ZLIB_SOURCE="C:\zlib-1.3.2" -s ZLIB_INCLUDE="C:\zlib-1.3.2"
```

Build output is written to `build\output\<platform>\<configuration>`, with the library in `lib`,
sample applications in `samples` and unit tests in `tests`.

The project can also be built with CMake on Windows, for example:
```cmd
cmake -S . -B build\cmake -A x64 -DBOOST_ROOT=C:\boost_1_92_0
cmake --build build\cmake --config Release --target snapdb samples UnitTests
```

## Compiling in Linux

The following information is intended to help developers build the SNAPdb API
library on Linux platforms. Similar instructions may apply to other platforms.

### Dependencies

The SNAPdb API library depends on the following libraries in order to build.
Earlier versions of the libraries listed may not work properly.

* CMake v3.16 (http://www.cmake.org/)

* GNU Make (http://www.gnu.org/software/make/)

* gcc v10.2 (for C++20 support)

* zlib Library, e.g.: `sudo apt install zlib1g-dev`

* bzip2 Library, e.g.: `sudo apt install libbz2-dev`

* Boost C++ Libraries v1.92.0 (http://www.boost.org/)
    - Boost.Asio
    - Boost.Iostreams
    - Boost.System
    - Boost.Thread
    - Boost.Uuid

Boost will need to be compiled:
https://www.boost.org/doc/libs/latest/more/getting_started/unix-variants.html

For Ubuntu, here are some common steps:

```bash
sudo apt update
sudo apt install build-essential
sudo apt install cmake
sudo apt install gcc-10 g++-10
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 --slave /usr/bin/g++ g++ /usr/bin/g++-10 --slave /usr/bin/gcov gcov /usr/bin/gcov-10

sudo apt install zlib1g-dev
sudo apt install libbz2-dev

sudo mkdir /usr/local/boost_1_92_0
cd /usr/local/
sudo wget https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.bz2
sudo tar -xvjf boost_1_92_0.tar.bz2
```

Start a new terminal session before building Boost:

```bash
cd /usr/local/boost_1_92_0
sudo ./bootstrap.sh
sudo ./b2 install
```

It may be necessary to add `/usr/local/lib`, the default path for boost libraries,
to the system library path before running any samples:

```bash
sudo ldconfig /usr/local/lib
```

Alternately, distribution Boost packages can be used, e.g., `sudo apt install libboost-all-dev`.

### Configuration

From the command terminal, enter the source directory containing this
README file and type the following command:

```bash
cmake .
```

Alternatively, you can create a build directory separate from the
source code you downloaded. Enter the build directory you created
and type the following command:

```bash
cmake -S path/to/source -B .
```

Using the CMake GUI, you can modify configuration options, such as
building as a shared library or changing the installation directory.

To make a debug build, use the following:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug .
```

### Build

At the top level of the build directory, type the following command.

```bash
make -j6
```

In addition to the library itself, there are sample applications which
demonstrate the proper use of the SNAPdb library API. To build all samples,
type the following command:

```bash
make -j6 samples
```
> Hint: You can start with samples and this will auto-build SNAPdb library dependency.

Individual sample applications can be built as follows:

```bash
make ReadTest
make WriteTest
make HistorianTest
make BulkWriteTest
make MetadataTest
```

### Samples

Samples accept an optional host address as the first argument, e.g., `localhost` or
`localhost:38402`, defaulting to `localhost`:

| Sample | Arguments | Description |
|---|---|---|
| `ReadTest` | `[host[:port]] [device]` | Reads recent frequency data, or data for a device |
| `WriteTest` | `[host[:port]] [pointID]` | Writes a test value for point ID 1, or the specified point ID |
| `MetadataTest` | `[host] [device] [sttpPort]` | Displays metadata summary and details for a device |
| `HistorianTest` | `[host[:port]] [deviceSearch]` | Port of the Python API walk-through test |
| `BulkWriteTest` | `[host[:port]] [instance]` | Writes test data one hour in the past to point IDs 900000001+ and 9000000001+, then verifies reads, filters, encodings and write APIs. Exit code is number of failed checks |

### Unit Tests

Unit tests do not require a running openHistorian. To build and run unit tests:

```bash
make tests
ctest --output-on-failure
```

### Installation

At the top level of the build directory, type the following command.

```bash
make install
```

This will move the header files and the library file to the location
specified during configuration. Header files go under the 'include/'
subdirectory, and the library file goes under the 'lib/' subdirectory.
