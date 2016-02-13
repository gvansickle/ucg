# NEWS file for the UniversalCodeGrep project.

## [UNRELEASED]

### Changed
- Removed Boost configuration cruft from earlier development.
- File() now throws a FileException if fstat() indicates the open()ed file isn't a regular file.
- Now checking fstat() call's return value.
- Globber now prints an error message but continues if it runs into an unreadable directory.

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
