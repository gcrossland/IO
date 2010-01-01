# I/O Library

This library provides assorted input and output streams, plus TCP utilities.

Interface documentation can be directly found in the library header files, [libraries/io_file.hpp](../libraries/io_file.hpp) and [libraries/io_socket.hpp](../libraries/io_socket.hpp), in Javadoc-esque documentation comments.

## Licence

The content of the IO repository is free software; you can redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl-2.0.txt) as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

The content is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

## Quick Start

*   Building requires [SCons 2](http://scons.org/) and [buildtools](https://github.com/gcrossland/buildtools). Ensure that the contents of buildtools is available on PYTHONPATH e.g. `export PYTHONPATH=/path/to/buildtools` (or is in the SCons site_scons dir).
*   The library depends on [Core](https://github.com/gcrossland/Core) and [Iterators](https://github.com/gcrossland/Iterators). Build these first.
*   From the working directory (or archive) root, run SCons to make a release build, specifying the compiler to use and where to find it e.g. `scons CONFIG=release TOOL_GCC=/usr/bin`.
    *   The library files are deployed to the library cache dir, which is (by default) under buildtools.
