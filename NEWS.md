# NEWS file for the UniversalCodeGrep project.

## [UNRELEASED]

### New Features
- Now supports --[no]column option which prints the column number of a match after the line number.  Default is --nocolumn.  Note that tabs count as only one column, consistent with the behavior of ack and ag.

### Changed
- File finding stage now prints an error message but continues if it runs into an unreadable directory.
- Don't try to read a file if fstat() indicates it isn't a regular file.
- Removed Boost configuration cruft from earlier development.

### Fixed
- Now properly returning an exit status of 1 if no matches found.  Resolves #60.
- Improved error checking when looking for project .ucgrc file.  Resolves #47 / Coverity CID 53716, 53717.
- Hidden files were incorrectly being ignored even if they had recognized extensions.  Resolves #63.
- Now checking for libpcre > 8.21 at configure-time.  Resolves #45.
- Added handling of fstat() errors.  Resolves #48 / Coverity CID 53715.

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
