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


BEGIN {
	if(ARGC != 4)
	{
		print("Incorrect number of args: ", ARGC)
		exit 1
	}

	NUM_ITERATIONS=10;
	RESULTS_FILE=ARGV[2];
	BOOST_PATH=ARGV[3];
	
	cur_line=0
	while(getline < ARGV[1])
	{
		cur_line++;
		CMD_LINE_ARRAY[cur_line]=$0;
		#print "cur_line=", cur_line, $0
	}
	
	print("Starting performance tests, results file is", RESULTS_FILE);
	
	for(i=1; i<=alen(CMD_LINE_ARRAY); ++i)
	{
		COMMAND_LINE=CMD_LINE_ARRAY[i];
		
		###TEMP
		sub(/\${BOOST_PATH}/, BOOST_PATH, COMMAND_LINE);
		#COMMAND_LINE="\" COMMAND_LINE \"";
		
		
		print("Timing: ", COMMAND_LINE) >> RESULTS_FILE;
		# "Prep" run, to eliminate disk cache variability and capture the matches.
		# We pipe the results through sort so we can diff these later. 
		PREP_RUN_FILES[i]=("SearchResults_" i ".txt");
		print("Prep run for command line: '", COMMAND_LINE, "'") > PREP_RUN_FILES[i]; 
		wrapped_cmd_line=("{ ( eval \"" COMMAND_LINE "\" 2>&1 ); } 3>&1 4>&2 | sort >> " PREP_RUN_FILES[i] ";");
		system(wrapped_cmd_line);
		print(wrapped_cmd_line);
	
		# Timing runs.
		for(ITER=1; ITER <= NUM_ITERATIONS; ++ITER)
		{
			#{ REAL_TIME[$ITER]=$( { time ${PROG} ${PARAM_LIST[@]} ${REGEX} ${TEST_DATA_DIR} 1>&3- 2>&4-; } 2>&1 ); } 3>&1 4>&2;
			wrapped_cmd_line=("{ REAL_TIME[" ITER "]=$( eval \"" COMMAND_LINE "\" 2>&1 ); } 3>&1 4>&2; echo \"${REAL_TIME[" ITER "]}\" >> " RESULTS_FILE ";");
			system(wrapped_cmd_line);
			print(wrapped_cmd_line);
			# 
			#echo "${REAL_TIME[$ITER]}" >> "${RESULTS_FILE}";
			REAL_TIME[ITER]=0.5;
		}
	
		# Determine the average.
		AVG_TIME[i]=0;
		for (j=1; j <= alen(REAL_TIME); ++j)
		{
			ELAPSED=REAL_TIME[j];
			AVG_TIME[i]=(AVG_TIME[i] + ELAPSED);
		}
		AVG_TIME[i]=(AVG_TIME[i] / NUM_ITERATIONS);
		print("Average elapsed time:", AVG_TIME[i]) >> RESULTS_FILE;
	}
	
	# Output the results.
	print("| Program | Avg of", NUM_ITERATIONS, "runs |") >> RESULTS_FILE;
	print("|---------|---------------|") >> RESULTS_FILE;
	for(i=1; i<=alen(CMD_LINE_ARRAY); ++i)
	{
		print("|", CMD_LINE_ARRAY[i], "|", AVG_TIME[i], "|") >> RESULTS_FILE;
	}
}
