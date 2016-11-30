# Makefile.am for UniversalCodeGrep, ./doc directory.
#
# Copyright 2015-2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

dist_man1_MANS = ucg.1

# Built source for checking command-line params against the hand-rolled man page.
#nodist_man1_MANS = mancheck.man

mancheck.man: $(top_builddir)/ucg$(EXEEXT) Makefile
	export ARGP_HELP_FMT=header-col=0,short-opt-col=2,long-opt-col=2 && help2man --no-info --section=1 "$<" > "$@"

