# UniversalCodeGrep

UniversalCodeGrep (ucg) is another [Ack](http://beyondgrep.com/) clone.  It is a grep-like tool specialized for searching large bodies of source code.

`ucg` is written in C++ and takes advantage of the C++11 and newer facilities of the language to reduce reliance on non-standard libraries, increase portability, and increase scanning speed.

## License

[GPL (Version 3 only)](https://github.com/gvansickle/ucg/blob/master/COPYING)

## Installation

UniversalCodeGrep installs from the source tarball in the standard autotools manner:

```sh
tar -xaf <tarball-name>
cd <tarball-name>
./configure
make
make install
```

This will install the `ucg` executable in `/usr/local/bin`.  If you wish to install it elsewhere or don't have permissions on `/usr/local/bin`, specify an installation prefix on the `./configure` command line:

```sh
./configure --prefix=~/<install-root-dir>
```

### Prerequisites

- `gcc` version 4.9 or greater.

Versions of `gcc` prior to 4.9 are known to ship with an incomplete implementation of the standard `<regex>` library.  Since `ucg` depends on this C++11 feature, `configure` attempts to detect a broken `<regex>` at configure-time.

### Supported OSes and Distributions

UniversalCodeGrep should build and function anywhere there's a `gcc` 4.9 or greater available.  It has been tested on the following OSes/distros:

- Linux
  - Ubuntu 15.04 (with gcc 4.9.2, the current default compiler on this distro)
- Windows 7 + Cygwin 64-bit (with gcc 4.9.3, the current default compiler on this distro)

## Usage



## Author

Gary R. Van Sickle
