# NEWS file for the UniversalCodeGrep project.

## [UNRELEASED]

### New Features
- New file inclusion/exclusion options:
	- `ack`-style `--ignore-file=FILTER:FILTERARGS`: Files matching FILTER:FILTERARGS (e.g. "ext:txt,cpp") will be ignored.
	- `grep`-style `--include=GLOB`: Only files matching GLOB will be searched.
	- `grep`-style `--exclude=GLOB`: Files matching GLOB will be ignored.
	- `ag`-style `--ignore=GLOB`: Files matching GLOB will be ignored.  Note that unlike `ag`'s option, this does not apply to directories).
- Experimental support for OSX and PC-BSD.
- Now compiles and links with either or both of libpcre and libpcre2, if available.  Defaults to using libpcre2 for matching.
- Directory tree traversal now uses more than one thread (two by default).  Can be overridden with new "--dirjobs" command-line parameter.  Overall performance improvement on all platforms vs. 0.2.2 (e.g., ~25% on Fedora 23 with hot cache).
- Line number counting is now done by either a generic or a vectorized (sse4.2 with/without popcount) version of FileScanner::CountLinesSinceLastMatch(), depending on the host system's ISA extension support.  Measurable performance increase; line number counting went from ~6.5% of total cycles to less than 1% on one workload per valgrind/callgrind.

### Changed
- Improved error reporting when directory traversal runs into problems.  
- A number of portability improvements related to OSX and PC-BSD support.
- Reduce unnecessary mutex contention, spurious thread wakes in sync_queue<>.
- Scanner threads now use a reuseable buffer when reading in files, reducing memory allocations by ~10% (and ~40% fewer bytes allocated) compared to version 0.2.2.
- Refactored FileScanner to be a base class with derived classes handling the particulars of using libpcre or libpcre2 to do the scanning. 
- Added a basic diagnostic/debug logging facility.
- ResizableArray now aligns allocations to 64-byte boundaries to match Core i7 cache line sizes in an effort to prevent false sharing.

### Fixed
- Cygwin now requires AC_USE_SYSTEM_EXTENSIONS for access to get_current_dir_name().  Resolves #76.


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
