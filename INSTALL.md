
## General information
`fuseadf` requires [the ADFlib](https://github.com/lclevy/ADFlib) for building
and operation (if the shared library is used).

Testing requires also [the Check testing framework for C](https://libcheck.github.io/check/).

There are 2 buildsystems that can be used: [CMake](https://cmake.org/)
or [autotools](https://en.wikipedia.org/wiki/GNU_Autotools).
Obviously, a requirement for building is installation of the chosen one.


## Building with CMake
There are helper scripts in the `util/` directory, which should help doing quick
builds. For instance, to build the release version:
1. `$ util/cmake_release_configure`
2. `$ util/cmake_release_build`

Debug version (with `gdb` and address sanitizer) can be built in the same way,
just replace `release` with `debug` in the commands above.

To (eventually) remove the build subdirectory use:
   `$ util/cmake_clean`

(all have to be configured again for building).

Configuration options can be passed to configure script, for instance:
   `util/cmake_debug_configure -DFUSEADF_ALLOW_USE_AS_ROOT:BOOL=ON`


## Building with Autotools
Standard way:
1. `$ ./autogen.sh`
2. `$ ./configure`
3. `$ make`

For configuration options, check `./configure --help`.
One worth notifying is the option for allowing or disallowing (default) using
the built `fuseadf` as root (`--enable-use-as-root` / `--disable...`).


## Testing
Some tests require presence of test images. They are not stored in
the repository and have to be downloaded using a script in `tests/testdata/`.
The images are downloaded from:
- [ADFlib's repository](https://github.com/lclevy/ADFlib/raw/master/regtests/Dumps)
- [a collection of the Public Domain disks](https://ftp.grandis.nu/turran/FTP/TOSEC/Collections/Commodore%20Amiga%20-%20Collections%20-%20Fred%20Fish/) (one of the Fred Fish disks)


## Testing with CMake
After successful building (see above), automatic tests can be started with:
-  `$ util/cmake_release_test`   (for the release version)
-  `$ util/cmake_debug_test`     (for the debug version)


## Testing with Autotools
After successful building (see above), automatic tests can be started with:
  `$ make check`


## Installation with CMake
To default location: `$ util/cmake_release_install`

To a custom location: `$ util/cmake_release_install [custom_prefix]`


## Installation with autotools
To the configured location: `$ make install`

To change default location, do: `$ ./configure --help` and look for prefix options.


## Building Debian packages
To build a `.deb` packages for Debian (or any derivative), use the helper script:
  `$ util/deb_build.sh`

In the parent(!) directory of the project, it will create:
- a source package (`.tar.gz`)
- binary packages (`.deb`)

The source package alone can be created with:
  `$ util/deb_create_quilt3_archive.sh`

