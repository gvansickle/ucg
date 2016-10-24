# UniversalCodeGrep

[![License](https://img.shields.io/badge/License-GPL3-red.svg)](LICENSE)
[![Travis-CI Build Status](https://travis-ci.org/gvansickle/ucg.svg?branch=master)](https://travis-ci.org/gvansickle/ucg)
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
  * [Arch Linux User Repository](#arch-linux-user-repository)
  * [openSUSE Binary RPMs](#opensuse-binary-rpms)
  * [Building the Source Tarball](#building-the-source-tarball)
    * [Build Prerequisites](#build-prerequisites)
      * [gcc version 4\.8 or greater\.](#gcc-version-48-or-greater)
      * [pcre version 8\.21 or greater\.](#pcre-version-821-or-greater)
  * [Supported OSes and Distributions](#supported-oses-and-distributions)
* [Usage](#usage)
  * [Command Line Options](#command-line-options)
    * [Searching](#searching)
    * [Search Output](#search-output)
    * [File presentation](#file-presentation)
    * [File inclusion/exclusion:](#file-inclusionexclusion)
    * [File type specification:](#file-type-specification)
    * [Miscellaneous:](#miscellaneous)
    * [Informational options:](#informational-options)
* [Configuration (\.ucgrc) Files](#configuration-ucgrc-files)
  * [Format](#format)
  * [Location and Read Order](#location-and-read-order)
* [User\-Defined File Types](#user-defined-file-types)
  * [Extension List Filter](#extension-list-filter)
  * [Literal Filename Filter](#literal-filename-filter)
* [Author](#author)


## Introduction

UniversalCodeGrep (`ucg`) is an extremely fast grep-like tool specialized for searching large bodies of source code.  It is intended to be largely command-line compatible with [`Ack`](http://beyondgrep.com/), to some extent with [`ag`](http://geoff.greer.fm/ag/), and where appropriate with `grep`.  Search patterns are specified as PCRE regexes. 

### Speed
`ucg` is intended to address the impatient programmer's code searching needs.  `ucg` is written in C++11 and takes advantage of the concurrency (and other) support of the language to increase scanning speed while reducing reliance on third-party libraries and increasing portability.  Regex scanning is provided by the [PCRE2 library](http://www.pcre.org/), with its [JIT compilation feature](http://www.pcre.org/current/doc/html/pcre2jit.html) providing a huge performance gain on most platforms.  Directory tree traversal is performed by multiple threads, reducing the impact of waiting for I/O completions.  Critical functions are implemented with hand-rolled vectorized (SSE2/4.2/etc.) versions selected at program load-time based on what the system supports, with non-vectorized fallbacks.  

As a consequence of its overall design for maximum concurrency and speed, `ucg` is extremely fast.  As an example, under Fedora 24, one of the benchmarks in the test suite which scans the Boost 1.58.0 source tree with `ucg` and a selection of similar utilities yields the following results:

#### Benchmark: '#include\s+".*"' on Boost source

| Command | Program Version | Elapsed Real Time, Average of 10 Runs | Num Matched Lines | Num Diff Chars |
|---------|-----------------|---------------------------------------|-------------------|----------------|
| `ucg --noenv --cpp '#include\s+.*' ~/src/boost_1_58_0` | 0.3.0 | 0.212767 | 9511 | 189 |
| `/usr/bin/ucg --noenv --cpp '#include\s+.*' ~/src/boost_1_58_0` | 0.2.2 | 0.262368 | 9511 | 189 |
| `/usr/bin/rg -n -t cpp '#include\s+.*' ~/src/boost_1_58_0` | 0.2.3 | 0.262967 | 9509 | 0 |
| `grep -Ern --color --include=\*.cpp --include=\*.hpp --include=\*.h --include=\*.cc --include=\*.cxx '#include\s+.*' ~/src/boost_1_58_0` | grep (GNU grep) 2.25 | 0.366634 | 9509 | 0 |
| `/usr/bin/pcre2grep -rn --color '--exclude=^.*(?<!\.cpp|\.hpp|\.h|\.cc|\.cxx)$' '#include\s+.*' ~/src/boost_1_58_0` | 10.21 2016-01-12 | 0.818627 | 9527 | 1386 |
| `/usr/bin/ag  --cpp '#include\s+.*' ~/src/boost_1_58_0` | 0.32.0 | 1.90161 | 9511 | 189 |

Note that UniversalCodeGrep is in fact somewhat faster than `grep` itself, even when `grep` is only using [Extended Regular Expressions](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap09.html#tag_09_04).  And `ucg` certainly wins the ease-of-use contest.

## License

[GPL (Version 3 only)](https://github.com/gvansickle/ucg/blob/master/COPYING)

## Installation

UniversalCodeGrep binaries are currently available for Fedora 23/24/25/rawhide and Centos 7.  Binaries for other platforms (Ubuntu, Arch, openSUSE) are coming soon.

<!-- COMING SOON
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
-->

### Fedora/CentOS Copr Repository

If you are a Fedora or CentOS user, the easiest way to install UniversalCodeGrep is from the Fedora Copr-hosted dnf/yum repository [here](https://copr.fedoraproject.org/coprs/grvs/UniversalCodeGrep).  Installation is as simple as:

```sh
# Add the Copr repo to your system:
sudo dnf copr enable grvs/UniversalCodeGrep
# Install UniversalCodeGrep:
sudo dnf install universalcodegrep
```

<!-- COMING SOON

### Arch Linux User Repository

If you are a Arch Linux user, the easiest way to install UniversalCodeGrep is from the Arch Linux User Repository (AUR) [here](https://aur.archlinux.org/packages/ucg/).  Installation is as simple as:

```sh
# Install using yaourt:
yaourt -S ucg
# Install manually:
cd /tmp/
curl -L -O https://aur.archlinux.org/cgit/aur.git/snapshot/ucg.tar.gz
tar -xvf ucg.tar.gz
cd ycg
makepkg -sri
```

### openSUSE Binary RPMs

Binary RPMs for openSUSE are available [here](https://github.com/gvansickle/ucg/releases/tag/0.3.0).

-->

### Building the Source Tarball

UniversalCodeGrep can be built and installed from the distribution tarball (available [here](https://github.com/gvansickle/ucg/releases/download/0.3.0/universalcodegrep-0.3.0.tar.gz)) in the standard autotools manner:

```sh
tar -xaf universalcodegrep-0.3.0.tar.gz
cd universalcodegrep-0.3.0.tar.gz
./configure
make
make install
```

This will install the `ucg` executable in `/usr/local/bin`.  If you wish to install it elsewhere or don't have permissions on `/usr/local/bin`, specify an installation prefix on the `./configure` command line:

```sh
./configure --prefix=~/<install-root-dir>
```

> #### *BSD Note
>
> On at least PC-BSD 10.3, g++48 can't find its own libstdc++ without a little help.  Configure the package like this:
> ```sh
> ./configure LDFLAGS='-Wl,-rpath=/usr/local/lib/gcc48'
> ```

#### Build Prerequisites

##### `gcc` and `g++` versions 4.8 or greater.

Versions of `gcc` prior to 4.8 do not have sufficiently complete C++11 support to build `ucg`.  `clang`/`clang++` is also known to work, but is not the primary development compiler.

##### PCRE: `libpcre2-8` version 10.20 or greater, or `libpcre` version 8.21 or greater.

One or both of these should be available from your Linux/OS X/*BSD distro's package manager. You'll need the `-devel` versions if they're separate.  Prefer `libpcre2-8`; while `ucg` will currently work with either PCRE2 or PCRE, you'll get better performance with PCRE2.

> #### OS X Prerequisites
>
> OS X additionally requires the installation of `argp-standalone`, which is normally part of the `glibc` library on Linux systems.  This can
> be installed along with a pcre library from Homebrew:
> ```sh
> $ brew update
> $ brew install pcre argp-standalone
> ```  

### Supported OSes and Distributions

UniversalCodeGrep 0.3.0 should build and run anywhere the prerequisites are available.  It has been built and tested on the following OSes/distros:

- Linux
  - Fedora 22, 23, 24, rawhide
  - CentOS 7
  - Ubuntu 16.04 (Xenial), 15.04, 14.04 (Trusty Tahr)
<!-- @todo VERIFY
  - Arch Linux
  - SLE 12
  - openSUSE 13.2
  - openSUSE Leap 42.1
-->
- Windows 7 + Cygwin 64-bit
- OS X
  - Xcode 8gm/OS X 10.11
  - Xcode 7.1.1 GM/OSX 10.10
  - Xcode 6.1/OS X 10.9
- PC-BSD 10.3


Note that at this time, only x86-64/amd64 architectures are supported.

## Usage

Invoking `ucg` is the same as with `ack` or `ag`:

```sh
ucg [OPTION...] PATTERN [FILES OR DIRECTORIES]
```

...where `PATTERN` is an PCRE-compatible regular expression.

If no `FILES OR DIRECTORIES` are specified, searching starts in the current directory.

### Command Line Options

Version 0.3.0 of `ucg` supports a significant subset of the options supported by `ack`.  In general, options specified later
on the command line override options specified earlier on the command line.

#### Searching
| Option | Description |
|----------------------|------------------------------------------|
| `--[no]smart-case`   | Ignore case if PATTERN is all lowercase (default: enabled). |
| `-i, --ignore-case`  | Ignore case distinctions in PATTERN.                        |
| `-Q, --literal`      | Treat all characters in PATTERN as literal.                 |
| `-w, --word-regexp`  | PATTERN must match a complete word.                         |

####  Search Output
| Option | Description |
|----------------------|------------------------------------------|
| `--column`   | Print column of first match after line number. |
| `--nocolumn` | Don't print column of first match (default).   |

#### File presentation
| Option | Description |
|----------------------|------------------------------------------|
| `--color, --colour`     | Render the output with ANSI color codes.    |
| `--nocolor, --nocolour` | Render the output without ANSI color codes. |

#### File/directory inclusion/exclusion:
| Option | Description |
|----------------------|------------------------------------------|
| `--[no]ignore-dir=name, --[no]ignore-directory=name`     | [Do not] exclude directories with this name.        |
| `--exclude=GLOB, --ignore=GLOB` | Files matching GLOB will be ignored. |
| `--ignore-file=FILTER:FILTERARGS` |  Files matching FILTER:FILTERARGS (e.g. ext:txt,cpp) will be ignored. |
| `--include=GLOB`                       | Only files matching GLOB will be searched. |
| `-k, --known-types`                              | Only search in files of recognized types (default: on). |
| `-n, --no-recurse`                               | Do not recurse into subdirectories.        |
| `-r, -R, --recurse`                              | Recurse into subdirectories (default: on). |
| `--type=[no]TYPE`                                | Include only [exclude all] TYPE files.  Types may also be specified as `--[no]TYPE`: e.g., `--cpp` is equivalent to `--type=cpp`.  May be specified multiple times. |

#### File type specification:
| Option | Description |
|----------------------|------------------------------------------|
| `--type-add=TYPE:FILTER:FILTERARGS` | Files FILTERed with the given FILTERARGS are treated as belonging to type TYPE.  Any existing definition of type TYPE is appended to. |
| `--type-del=TYPE`                   | Remove any existing definition of type TYPE. |
| `--type-set=TYPE:FILTER:FILTERARGS` | Files FILTERed with the given FILTERARGS are treated as belonging to type TYPE.  Any existing definition of type TYPE is replaced. |

#### Performance Tuning:
| Option | Description |
|----------------------|------------------------------------------|
| `--dirjobs=NUM_JOBS`   |  Number of directory traversal jobs (std::thread<>s) to use.  Default is 2. |
| `-j, --jobs=NUM_JOBS`       | Number of scanner jobs (std::thread<>s) to use.  Default is the number of cores on the system. |

#### Miscellaneous:
| Option | Description |
|----------------------|------------------------------------------|
| `--noenv`         | Ignore .ucgrc files.                            |

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

`ucg` supports user-defined file types with the `--type-set=TYPE:FILTER:FILTERARGS` and `--type-add=TYPE:FILTER:FILTERARGS` command-line options.  Three FILTERs are currently supported, `ext` (extension list), `is` (literal filename), and `glob` (glob pattern).

### Extension List Filter

The extension list filter allows you to specify a comma-separated list of file extensions which are to be considered as belonging to file type TYPE.
Example:
`--type-set=type1:ext:abc,xqz,def`

### Literal Filename Filter

The literal filename filter simply specifies a single literal filename which is to be considered as belonging to file type TYPE.
Example:
`--type-add=autoconf:is:configure.ac`

### Glob filter

The glob filter allows you to specify a glob pattern to match against filenames.  If the glob matches, the file is considered as belonging to the file type TYPE.
Example:
`--type-set=mk:glob:?akefile*`

## Author

[Gary R. Van Sickle](https://github.com/gvansickle)
