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


# Determine the number of elements in an array.
function alen (a, i, i_max)
{
	i_max = 0
	for (i in a)
	{
		###print("i=", i)
		if (i+0 > i_max+0) i_max=i
	}
	return i_max+0
}

function adelete(a,    i)
{
	# Delete all entries in array a.
	for (i in a)
	{
    	delete a[i]
    }
}

# Copy a numerically-indexed array
function acopy(ain, aout,    i)
{
	# Make sure aout is empty.
	adelete(aout)
    
	for (i=1; i <= alen(ain); i++ )
	{
		###print("acopy: ", i, ain[i])
	    aout[i] = ain[i];
    }
    
    return 0
}

function join_val_range(in_array, out_array, join_on_regex, regex_to_replace, prefix, max_val,   temp_entry, in_i, in_len, out_i, scan_jobs)
{
	in_len = alen(in_array)
	out_i = 1
	for(in_i = 1; in_i <= in_len; in_i++)
	{
		temp_entry=in_array[in_i]
		
		if(match(temp_entry, join_on_regex) == 0)
		{
			# No match, just pass this line through.
			###print("No join match, passthrough: ", temp_entry)
			out_array[out_i]=temp_entry
			out_i++
		}
		else
		{
			# For the first outgoing "joined" entry, simply replace with the empty string.
			sub(regex_to_replace, "", temp_entry)
			out_array[out_i]=temp_entry
			out_i++
	
			if (prefix != "")
			{
				# There is a non-empty prefix (command-line option), so do a join with the values 1-max_val.
				for (scan_jobs=1; scan_jobs <= max_val; scan_jobs++ )
				{
					###print("here", out_i, scan_jobs)
					temp_entry=in_array[in_i]
					sub(regex_to_replace, prefix scan_jobs, temp_entry)
					out_array[out_i]=temp_entry
					out_i++
				}
			}
		}
	}
	
	return 0
} 

BEGIN {

	if(ARGC != 3)
	{
		print("Incorrect number of args: ", ARGC) | "cat 1>&2"
		exit 1
	}
	
	# Get command line args.
	REGEX=ARGV[1]
	TEST_DATA_DIR=ARGV[2]
	
		
	# Pick up a usable "time" program from the environment, the testsuite should have put it there.
	PROG_TIME=ENVIRON["PROG_TIME"]
	if(PROG_TIME == "")
	{
		print("ERROR: env var $PROG_TIME is not set.");
		exit 1;
	}
	### @todo
	###PROG_TIME=("/usr/bin/time -f 'real %e\\npct_cpu %P\\ntimed_cmd_line %C\\n'");
	

	TEST_GROUPS[1]="built_ucg"
	TEST_GROUPS[2]="system_grep"
	
	TEST_GROUP_TO_PROGS["built_ucg"]="ucg"
	TEST_GROUP_TO_PROGS["system_grep"]="grep"
	
	TEST_GROUP_TO_PARAMS_PRE["built_ucg"]="--noenv --cpp"
	TEST_GROUP_TO_PARAMS_PRE["system_grep"]="-Ern --color --include=\\*.cpp --include=\\*.hpp --include=\\*.h --include=\\*.cc --include=\\*.cxx"
	
	PROG_TO_PARAMS_JOBS["ucg"]="-j"
	PROG_TO_PARAMS_JOBS["grep"]=""
	
	PROG_TO_PARAMS_DIRJOBS["ucg"]="--dirjobs="
	PROG_TO_PARAMS_DIRJOBS["grep"]=""

	### print("Num test groups: ", alen(TEST_GROUPS))
	
	COMMAND_LINE=""
	cla_index=1
	
	for (i=1; i <= alen(TEST_GROUPS); i++ )
	{
		j = TEST_GROUPS[i]
		PROG = TEST_GROUP_TO_PROGS[j]
		dirjobs_option = PROG_TO_PARAMS_DIRJOBS[PROG]
		scanjobs_option = PROG_TO_PARAMS_JOBS[PROG]
		COMMAND_LINE=("{ " PROG_TIME " " PROG " " TEST_GROUP_TO_PARAMS_PRE[j] " DIRJOBS_PLACEHOLDER SCANJOBS_PLACEHOLDER '" REGEX "' '" TEST_DATA_DIR "'; 1>&3 2>&4; }")
		
		# Output the default "number of threads to use for scanning" command-line option (i.e. empty).
		CL_COPY=COMMAND_LINE
		sub(/SCANJOBS_PLACEHOLDER/, "", COMMAND_LINE)
		CMD_LINE_ARRAY[cla_index]=COMMAND_LINE
		cla_index++
		COMMAND_LINE=CL_COPY

		if (PROG_TO_PARAMS_JOBS[PROG] != "")
		{
			# This program has a "number of threads to use for scanning" command-line option.
			for (scan_jobs=1; scan_jobs <= 4; scan_jobs++ )
			{
				COMMAND_LINE=CL_COPY
				sub(/SCANJOBS_PLACEHOLDER/, PROG_TO_PARAMS_JOBS[PROG] scan_jobs, COMMAND_LINE)
				CMD_LINE_ARRAY[cla_index]=COMMAND_LINE
				cla_index++
			}
		}
	}
	
	acopy(CMD_LINE_ARRAY, CLA_COPY)
	join_val_range(CLA_COPY, CMD_LINE_ARRAY, " ucg ", "DIRJOBS_PLACEHOLDER", PROG_TO_PARAMS_DIRJOBS["ucg"], 4)
	acopy(CMD_LINE_ARRAY, CLA_COPY)
	join_val_range(CLA_COPY, CMD_LINE_ARRAY, " grep ", "DIRJOBS_PLACEHOLDER", PROG_TO_PARAMS_DIRJOBS["grep"], 4)
	
	###print("Printing command line array:")
	cla_alen = alen(CMD_LINE_ARRAY)
	for ( i = 1; i <= cla_alen; i++ )
	{
		print(CMD_LINE_ARRAY[i])
	}
}
