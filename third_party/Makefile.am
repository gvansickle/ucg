# Second-level Makefile.am for UniversalCodeGrep, ./third_party directory.
#
# Copyright 2015 Gary R. Van Sickle (grvs@users.sourceforge.net).
#
# This file is part of UniversalCodeGrep.
#
# UniversalCodeGrep is free software: you can redistribute it and/or modify it under the
# terms of version 3 of the GNU General Public License as published by the Free
# Software Foundation.
#
# UniversalCodeGrep is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with
# UniversalCodeGrep.  If not, see <http://www.gnu.org/licenses/>.

# This is a proxy Makefile.am which dispatches the Autotools rules to the real makefiles of
# the third-party libraries/tools we may need.


## Much of this directory is conditionalized at configure time, so we define some vars at the top
## which we'll append to later in the sections which are not conditionalized out.

noinst_SCRIPTS =

# Directory trees we need to delete on a "make clean".
CLEAN_LOCAL_DIRS = 

# The one file we explicitly "build" is this stampfile.
# This stampfile depends on all files added to $(third_party_complete_deps), which will in turn be the result of
# the various sub-builds.  We do it this way (instead of listing all the sub-build stamps here) so we can still
# benefit from parallel builds.
BUILT_SOURCES = stampfile-all-third-party-builds-complete

# Append any stamp (or other) files which indicate that a sub-build is complete to this variable.
third_party_complete_deps =

# Extra files we'll need to distribute.
EXTRA_DIST =

# Files to clean for "make mostlyclean" and "make clean".
MOSTLYCLEANFILES =
CLEANFILES =


###
### Include third-party Makefile.am's here.
###


# The stamp file which will depend on all active third-party builds.
# We do this this way so as to maximize parallel build potential.
stampfile-all-third-party-builds-complete: $(third_party_complete_deps)
	echo "All third-party builds complete, $$(date)" > "$@-temp"
	mv "$@-temp" "$@"

###
### MAINTAINER-ONLY TARGETS
### These are strictly of use for the maintainer, e.g. to update the version of Boost.
###



###
### Clean rules
###

# Delete all the stamps except for the untarring ones for all clean targets.
# Also delete the generated .tar files, they're cheap to rebuild.
MOSTLYCLEANFILES +=  $(CLEANFILES_REPORT_BOILERPLATE) stampfile-all-third-party-builds-complete
	 
# We need to hook 'clean' here because we need to rm the entire untarred source trees, not just individual files.	
clean-local:
	@:; # Strip any spaces off the ends of the directory list.
	DIRLIST="$$(echo "$(CLEAN_LOCAL_DIRS)" | $(SED) -r 's/[ ]*(.*)[ ]*/\1\n/')"; \
	if [ -n "$${DIRLIST}" ]; then \
		for DIRNAME in $${DIRLIST}; \
		do \
			echo "Removing directory $${DIRNAME}...";\
			test ! -d "$${DIRNAME}" || rm -rf "$${DIRNAME}"; \
		done; \
	fi;

# Nothing in here to install or check, so no other *-local hooks.
