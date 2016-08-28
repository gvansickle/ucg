#! /usr/bin/awk -f
#
# Copyright 2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

BEGIN {
	TEST_PROG_ID="unknown";
	TEST_PROG_PATH="unknown";
	NUM_TIME_ENTRIES=0;
	AVG_TIME=0;
	SAMPLE_STD_DEV=0;
	SEM=0;
	NUM_MATCHED_LINES=0;
	NUM_DIFF_CHARS=0;
}

#FNR==1 { print("FN=" FILENAME); };
$1 ~ /^TEST_PROG_ID:/ { TEST_PROG_ID=$2; }
$1 ~ /^TEST_PROG_PATH:/ { TEST_PROG_PATH=$2; }
$1 ~ /^real/ {
	REAL_TIME[NUM_TIME_ENTRIES]=$2;
	NUM_TIME_ENTRIES++;
	AVG_TIME+=$2;
	}

END {
	# Protect from divide by zero.
	if(NUM_TIME_ENTRIES==0) { print("ERROR: No time entries in file " FILENAME "."); exit 1; };
	
	AVG_TIME /= NUM_TIME_ENTRIES;
	# Calculate the sample std deviation and the standard error of the mean (SEM).
	# https://en.wikipedia.org/wiki/Standard_error#Standard_error_of_the_mean
	for(j=0; j < NUM_TIME_ENTRIES; ++j)
	{
		# sample std dev is sqrt((sum of squared deviations from mean)/(N-1))
		SAMPLE_STD_DEV+=(AVG_TIME - REAL_TIME[j])^2;
	}
	SAMPLE_STD_DEV=sqrt(SAMPLE_STD_DEV/(NUM_TIME_ENTRIES-1));
	SEM=SAMPLE_STD_DEV/sqrt(NUM_TIME_ENTRIES);
	
	OFS=" | ";
	
	print("| " TEST_PROG_ID, AVG_TIME/NUM_TIME_ENTRIES, SAMPLE_STD_DEV, SEM, NUM_MATCHED_LINES, NUM_DIFF_CHARS " |");
}
