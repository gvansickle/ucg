# UniversalCodeGrep

<a href="https://scan.coverity.com/projects/gvansickle-ucg">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/7451/badge.svg"/>
</a>

UniversalCodeGrep (ucg) is an extremely fast grep-like tool specialized for searching large bodies of source code.

## Table of Contents

* [Introduction](#introduction)
  * [Speed](#speed)
* [License](#license)
* [Installation](#installation)
  * [Ubuntu PPA](#ubuntu-ppa)
  * [Red Hat/Fedora/CentOS dnf/yum Repository](#red-hatfedoracentos-dnfyum-repository)
  * [Binary RPMs](#binary-rpms)
  * [Building the Source Tarball](#building-the-source-tarball)
	* [Build Prerequisites](#build-prerequisites)
	  * [gcc version 4.8 or greater.](#gcc-version-48-or-greater)
	  * [pcre version 8.21 or greater.](#pcre-version-821-or-greater)
  * [Supported OSes and Distributions](#supported-oses-and-distributions)
* [Usage](#usage)
  * [Command Line Options](#command-line-options)
	* [Searching](#searching)
	* [File presentation](#file-presentation)
	* [File inclusion/exclusion:](#file-inclusionexclusion)
	* [File type specification:](#file-type-specification)
	* [Miscellaneous:](#miscellaneous)
	* [Informational options:](#informational-options)
* [Configuration (.ucgrc) Files](#configuration-ucgrc-files)
  * [Format](#format)
  * [Location and Read Order](#location-and-read-order)
* [User-Defined File Types](#user-defined-file-types)
  * [Extension List Filter](#extension-list-filter)
  * [Literal Filename Filter](#literal-filename-filter)
* [Author](#author)

## Introduction

UniversalCodeGrep (ucg) is an extremely fast grep-like tool specialized for searching large bodies of source code.  It is intended to be largely command-line compatible with [Ack](http://beyondgrep.com/), to some extent with [`ag`](http://geoff.greer.fm/ag/), and where appropriate with `grep`.  Search patterns are specified as PCRE regexes. 

### Speed
`ucg` is intended to address the impatient programmer's code searching needs.  `ucg` is written in C++11 and takes advantage of the concurrency (and other) support of the language to increase scanning speed while reducing reliance on third-party libraries and increasing portability.  Regex scanning is provided by the [PCRE library](http://www.pcre.org/), with its [JIT compilation feature](http://www.pcre.org/original/doc/html/pcrejit.html) providing a huge performance gain on most platforms.

As a consequence of its use of these facilities and its overall design for maximum concurrency and speed, `ucg` is extremely fast.  Under Fedora 23, scanning the Boost 1.58.0 source tree with `ucg` 0.2.1, [`ag`](http://geoff.greer.fm/ag/) 0.31.0, and `ack` 2.14 produces the following results:

| Command | Approximate Real Time |
|---------|-----------------------|
| `time ucg 'BOOST.*HPP' ~/src/boost_1_58_0` | ~ 0.509 seconds |
| `time ag 'BOOST.*HPP' ~/src/boost_1_58_0`  | ~ 10.66 seconds |
| `time ack 'BOOST.*HPP' ~/src/boost_1_58_0` | ~ 17.19 seconds |

UniversalCodeGrep is in fact somewhat faster than `grep` itself.  Again under Fedora 23 and searching the Boost 1.58.0 source tree, `ucg` bests grep 2.22 not only in ease-of-use but in raw speed:

| Command | Approximate Real Time |
|---------|-----------------------|
| `time grep -Ern --include=\*.cpp --include=\*.hpp --include=\*.h --include=\*.cc --include=\*.cxx 'BOOST.*HPP' ~/src/boost_1_58_0/ | sort > grepout.txt` | ~ 0.570 seconds |
| `time ucg --cpp 'BOOST.*HPP' ~/src/boost_1_58_0/ | sort > ucgout.txt`  | ~ 0.498 seconds |

The resulting match files (*out.txt) are identical.

## License

[GPL (Version 3 only)](https://github.com/gvansickle/ucg/blob/master/COPYING)

## Installation

### Ubuntu PPA

If you are a Ubuntu user, the easiest way to install UniversalCodeGrep is from the Launchpad PPA [here](https://launchpad.net/~grvs/+archive/ubuntu/ucg).  To install from the command line:

```sh
# Add the PPA to your system:
sudo add-apt-repository ppa:grvs/ucg
# Pull down the latest lists of software from all archives:
sudo apt-get update
# Install ucg:
sudo apt-get install universalcodegrep
```

### Red Hat/Fedora/CentOS dnf/yum Repository

If you are a Red Hat, Fedora, or CentOS user, the easiest way to install UniversalCodeGrep is from the Fedora Copr-hosted dnf/yum repository [here](https://copr.fedoraproject.org/coprs/grvs/UniversalCodeGrep).  Installation is as simple as:

```sh
# Add the Copr repo to your system:
sudo dnf copr enable grvs/UniversalCodeGrep
# Install UniversalCodeGrep:
sudo dnf install universalcodegrep
```

### Binary RPMs

Binary RPMs for openSUSE are available [here](https://github.com/gvansickle/ucg/releases/latest).  Note that when installing via `zypper`, `yum`, `dnf`, or whatever's appropriate for your system, you will probably get the same sort of security warnings you get if you install Google Chrome from its RPM.

### Building the Source Tarball

UniversalCodeGrep can be built and installed from the distribution tarball (available [here](https://github.com/gvansickle/ucg/releases/download/0.2.1/universalcodegrep-0.2.1.tar.gz)) in the standard autotools manner:

```sh
tar -xaf universalcodegrep-0.2.1.tar.gz
cd universalcodegrep-0.2.1.tar.gz
./configure
make
make install
```

This will install the `ucg` executable in `/usr/local/bin`.  If you wish to install it elsewhere or don't have permissions on `/usr/local/bin`, specify an installation prefix on the `./configure` command line:

```sh
./configure --prefix=~/<install-root-dir>
```

#### Build Prerequisites

##### `gcc` version 4.8 or greater.

Versions of `gcc` prior to 4.8 do not have sufficiently complete C++11 support to build `ucg`.

##### `pcre` version 8.21 or greater.

This should be available from your Linux distro.

### Supported OSes and Distributions

UniversalCodeGrep should build and function anywhere the prerequisites are available.  It has been built and tested on the following OSes/distros:

- Linux
  - Ubuntu 15.04
  - CentOS 7
  - Fedora 22
  - Fedora 23
  - RHEL 7
  - SLE 12
  - openSUSE 13.2
  - openSUSE Leap 42.1
- Windows 7 + Cygwin 64-bit (Note however that speed here is comparable to `ag`)

## Usage

Invoking `ucg` is the same as with `ack` or `ag`:

```sh
ucg [OPTION...] PATTERN [FILES OR DIRECTORIES]
```

...where `PATTERN` is an PCRE-compatible regular expression.

If no `FILES OR DIRECTORIES` are specified, searching starts in the current directory.

### Command Line Options

Version 0.2.1 of `ucg` supports a significant subset of the options supported by `ack`.  Future releases will have support for more options.

#### Searching

| Option | Description |
|----------------------|------------------------------------------|
| `-i, --ignore-case`  |      Ignore case distinctions in PATTERN        |
| `-Q, --literal`      |     Treat all characters in PATTERN as literal. |
| `-w, --word-regexp`  |      PATTERN must match a complete word.        |

#### File presentation

| Option | Description |
|----------------------|------------------------------------------|
| `--color, --colour`     | Render the output with ANSI color codes.    |
| `--nocolor, --nocolour` | Render the output without ANSI color codes. |

#### File inclusion/exclusion:
| Option | Description |
|----------------------|------------------------------------------|
| `--ignore-dir=name, --ignore-directory=name`     | Exclude directories with this name.        |
| `--noignore-dir=name, --noignore-directory=name` | Do not exclude directories with this name. |
| `-n, --no-recurse`                               | Do not recurse into subdirectories.        |
| `-r, -R, --recurse`                              | Recurse into subdirectories (default: on). |
| `--type=[no]TYPE`                                | Include only [exclude all] TYPE files.  Types may also be specified as `--[no]TYPE`.     |

#### File type specification:
| Option | Description |
|----------------------|------------------------------------------|
| `--type-add=TYPE:FILTER:FILTERARGS` | Files FILTERed with the given FILTERARGS are treated as belonging to type TYPE.  Any existing definition of type TYPE is appended to. |
| `--type-del=TYPE`                   | Remove any existing definition of type TYPE. |
| `--type-set=TYPE:FILTER:FILTERARGS` | Files FILTERed with the given FILTERARGS are treated as belonging to type TYPE.  Any existing definition of type TYPE is replaced. |

#### Miscellaneous:
| Option | Description |
|----------------------|------------------------------------------|
| `-j, --jobs=NUM_JOBS` | Number of scanner jobs (std::thread<>s) to use. |
| `--noenv`             | Ignore .ucgrc files.                            |

#### Informational options:
| Option | Description |
|----------------------|------------------------------------------|
| `-?, --help`                      | give this help list                 |
| `--help-types, --list-file-types` | Print list of supported file types. |
| `--usage`                         | give a short usage message          |
| `-V, --version`                   | print program version               |

## Configuration (.ucgrc) Files

UniversalCodeGrep supports configuration files with the name `.ucgrc`, in which command-line options can be stored on a per-user and per-directory-hierarchy basis.

### Format

`.ucgrc` files are text files with a simple format.  Each line of text can be either:

1. A single-line comment.  The line must start with a `#` and the comment continues for the rest of the line.
2. A command-line parameter.  This must be exactly as if it was given on the command line.

### Location and Read Order

When `ucg` is invoked, it looks for command-line options from the following locations in the following order:

1. The `.ucgrc` file in the user's `$HOME` directory, if any.
2. The first `.ucgrc` file found, if any, by walking up the component directories of the current working directory.  This traversal stops at either the user's `$HOME` directory or the root directory.  This is called the project config file, and is intended to live in the top-level directory of a project directory hierarchy.
3. The command line itself.

Options read later will override earlier options.

## User-Defined File Types

`ucg` supports user-defined file types with the `--type-set=TYPE:FILTER:FILTERARGS` and `--type-add=TYPE:FILTER:FILTERARGS` command-line options.  Only two FILTERs are currently supported, `ext` (extension list) and `is` (literal filename).

### Extension List Filter

The extension list filter allows you to specify a comma-separated list of file extensions which are to be considered as belonging to file type TYPE.
Example:
`--type-set=type1:ext:abc,xqz,def`

### Literal Filename Filter

The literal filename filter simply specifies a single literal filename which is to be considered as belonging to file type TYPE.
Example:
`--type-add=autoconf:is:configure.ac`

## Author

[Gary R. Van Sickle](https://github.com/gvansickle)
