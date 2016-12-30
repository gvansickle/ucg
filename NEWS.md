# NEWS file for the UniversalCodeGrep project.

## [0.3.2] - 2016-12-29

UniversalCodeGrep (ucg) 0.3.2 is a minor bugfix release with some major under-the-hood changes.

### New Features
- Added '--[no]follow' option.  Default is now to not follow symlinks.

### Changed
- Major overhaul of directory tree traversal code to allow more opportunities for increasing performance.
- Performance: Literal string search now bypasses `libpcre2` in certain cases and uses vectorized search instead.

### Fixed
- Fix to multiversioning to allow building under Homebrew without `env :std`.

## [0.3.1] - 2016-11-28

UniversalCodeGrep (ucg) 0.3.1 is a minor feature and bugfix release.

### New Features
- Full `man` page, courtesy @larryhynes.

### Changed
- Checks for and makes use of some C++14 and C++1y (C++17) features if available.
- Refactoring of the benchmark section of the testsuite.
- Added ".deps" to the default directory ignore list.

### Fixed
- Fixed compile breakage on FreeBSD due to undefined macro.
- Builds on TrueOS (nee PC-BSD) with gcc 6.  Configure with `../configure CC=gcc6 CXX=g++6`.
- Builds on Mac OS X in three configurations:
  - No special `configure` options
  - With `clang` plus the GNU `libstdc++`
  - With `clang` plus its own `libc++`

## [0.3.0] - 2016-10-23

Major feature/bugfix release of UniversalCodeGrep (ucg).

### New Features
- 20-30% faster than ucg 0.2.2 on most benchmarks.
- New file inclusion/exclusion options:
	- `ack`-style `--ignore-file=FILTER:FILTERARGS`: Files matching FILTER:FILTERARGS (e.g. "ext:txt,cpp") will be ignored.
	- `grep`-style `--include=GLOB`: Only files matching GLOB will be searched.
	- `grep`-style `--exclude=GLOB`: Files matching GLOB will be ignored.
	- `ag`-style `--ignore=GLOB`: Files matching GLOB will be ignored.  Note that unlike `ag`'s option, this does not apply to directories).
- Files and directories specified on the command line (including hidden files) are now scanned regardless of ignore settings, and in the case of files, whether they are recognized as text files.
- `--TYPE`- and `--noTYPE`-style options now support unique-prefix matching.  E.g., `--py`, `--pyth`, and `--python` all select the Python file type.
- New file type filter: `glob`.  E.g. '--type-set=mk:glob:?akefile*'.
- OS X and some *BSDs now supported.  Builds and runs on Xcode 6.1/OS X 10.9 through Xcode 8gm/OS X 10.11.
- Now compiles and links with either or both of libpcre and libpcre2, if available.  Defaults to using libpcre2 for matching.
- Directory tree traversal now uses more than one thread (two by default).  Can be overridden with new "--dirjobs" command-line parameter.  Overall performance improvement on all platforms vs. 0.2.2 (e.g., ~25% on Fedora 23 with hot cache).
- New portable function multiversioning infrastructure.  Currently used by the following features:
	- Line number counting is now done by either a generic or a vectorized (sse2/sse4.2 with/without popcount) version of FileScanner::CountLinesSinceLastMatch(), depending on the host system's ISA extension support.  Measurable performance increase; line number counting went from ~6.5% of total cycles to less than 1% on one workload per valgrind/callgrind.
- Added ".awk" as a builtin file type.

### Changed
- Improved error reporting when directory traversal runs into problems.
- A number of portability improvements related to OSX and PC-BSD support.
- Reduce unnecessary mutex contention, spurious thread wakes in sync_queue<>.
- Scanner threads now use a reusable buffer when reading in files, reducing memory allocations by ~10% (and ~40% fewer bytes allocated) compared to version 0.2.2.
- Refactored FileScanner to be a base class with derived classes handling the particulars of using libpcre or libpcre2 to do the scanning.
- Added a basic diagnostic/debug logging facility.
- ResizableArray now takes an alignment parameter.  File buffer allocations are now done on max(ST_BLKSIZE,128k)-byte boundaries.
- Testing/Benchmarking infrastructure expansion and improvements.

### Fixed
- Cygwin now requires AC_USE_SYSTEM_EXTENSIONS for access to get_current_dir_name().  Resolves #76.
- Resolved issue with highlights on wrapped lines sometimes extending the full width of the terminal.  Resolves #80.
- Resolved issue where matches spanning an eol (e.g. 'a\s+b' matching 'a\nb') would cause the program to throw an exception and terminate.  Resolves #92.
- Resolved segfaults on some systems due to dirname() modifying its parameter.  Resolves #96.
- No longer treating PCRE2 reporting no JIT support as an error.  Resolves #100.


## [0.2.2] - 2016-04-09

Minor feature/bugfix release of UniversalCodeGrep (ucg).

### New Features
- Added --[no]smart-case option, which is on by default.  With this feature enabled, matching is done case-insensitively if the given PATTERN is all lower-case.
- Added --[no]column option which prints the column number of a match after the line number.  Default is --nocolumn.  Note that tabs count as only one column, consistent with the behavior of ack and ag.
- Added -k/--known-types option for compatibility with ack.

### Changed
- Now checking for and rejecting non-option arguments and double-dash in .ucgrc files.
- File finding stage now prints an error message but continues if it runs into an unreadable directory.
- Don't try to read a file if fstat() indicates it isn't a regular file.
- Improved codebase support for C++11's move semantics.  Reduces memory allocation/deallocation traffic by about 20%.
- Removed Boost configuration cruft from earlier development.

### Fixed
- Now properly returning an exit status of 1 if no matches found.  Resolves #60.
- Improved error checking when looking for project .ucgrc file.  Resolves #47 / Coverity CID 53716, 53717.
- Hidden files were incorrectly being ignored even if they had recognized extensions.  Resolves #63.
- Now checking for libpcre > 8.21 at configure-time.  Resolves #45.
- Added handling of fstat() errors.  Resolves #48 / Coverity CID 53715.
- If project rc file can't be opened, error message now reports its name instead of $HOME/.ucgrc's name.

## [0.2.1] - 2016-02-08

Minor bugfix/feature release of UniversalCodeGrep (ucg).

### Added
- Added auto-versioning support, improved --version output to display built-from vcs/tarball info, compiler version, libpcre version and info.  Resolves #4, #56.
- Added performance test vs. grep on Boost --cpp files with regex 'BOOST.*HPP' to testsuite.
- Added color-vs-file and color-vs-tty tests to the testsuite.
- Performance test suite now captures version info of the programs that are being compared.  Resolves #22.

### Changed
- Updated color logic so that --color forces color regardless of output device, and tty will get color unless --nocolor is specified.  Resolves #52.
- Merged pull request #36 from SilverNexus/master: Add CMakeFiles to automatically excluded dirs.

### Fixed
- Files with thousands of matches no longer take anywhere near as long to output.  Should help performance in the average case as well.  Changed some naive O(n^2) algorithms to O(n) ones.  Resolves #35.
- Refactored Globber's bad-start-path detection logic to eliminate a file descriptor leak.  Resolves #46 / Coverity CID 53718.
- Fixed extra newline at the start of tty output.  Resolves #50.
- Merged pull request #54 from ismail/clang-fix: Add sstream include to fix compilation with clang with libc++.

### Security
- Now compiling with -Wformat, -Wformat-security, -Werror=format-security if compiler supports it.  Resolves #57.

## [0.2.0] - 2015-12-28
- No news yet.
