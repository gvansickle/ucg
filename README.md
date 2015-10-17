# UniversalCodeGrep

UniversalCodeGrep (ucg) is another [Ack](http://beyondgrep.com/) clone.  It is a grep-like tool specialized for searching large bodies of source code.

Ucg is written in C++ and takes advantage of the C++11 and newer facilities of the language to reduce reliance on non-standard libraries, increase portability, and increase scanning speed.

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

- gcc version 4.9 or greater.

Versions of gcc prior to 4.9 are known to ship with an incomplete implementation of the standard <regex> library.

## Author

Gary R. Van Sickle
