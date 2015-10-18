# UniversalCodeGrep

UniversalCodeGrep (ucg) is another [Ack](http://beyondgrep.com/) clone.  It is a grep-like tool specialized for searching large bodies of source code.

`ucg` is written in C++ and takes advantage of the C++11 and newer facilities of the language to reduce reliance on non-standard libraries, increase portability, and increase scanning speed.

As a consequence of `ucg`'s use of these facilities and its overall design for maximum concurrency and speed, `ucg` is extremely fast.  Under Ubuntu 15.04, scanning the Boost 1.58.0 source tree with `ucg` 0.1.0, [`ag`](http://geoff.greer.fm/ag/) 0.28.0, and `ack` 2.14 produces the following results:

| Command | Approximate Real Time |
|---------|-----------------------|
| `time ucg '#endif' ~/src/boost_1_58_0` | ~ 3 seconds |
| `time ag '#endif' ~/src/boost_1_58_0` | ~ 10 seconds |
| `time ack '#endif' ~/src/boost_1_58_0` | ~ 19 seconds |

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

Invoking `ucg` is the same as with `ack`:

```sh
ucg [OPTION...] PATTERN [FILES OR DIRECTORIES]
```

...where `PATTERN` is an ECMAScript-compatible regular expression.

If no `FILES OR DIRECTORIES` are specified, searching starts in the current directory.

### Options

Version 0.1.0 of `ucg` only supports a small subset of the options supported by `ack`.  Future releases will have support for more options.

#### Searching

| Option | Description |
|----------------------|------------------------------------------|
| `-i, --ignore-case`    |      Ignore case distinctions in PATTERN |


#### File presentation

| Option | Description |
|----------------------|------------------------------------------|
| `--color, --colour`   |   Render the output with ANSI color codes. |
| `--nocolor, --nocolour` | Render the output without ANSI color codes. |

## Author

Gary R. Van Sickle
