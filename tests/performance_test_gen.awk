
# Determine the number of elements in an array.
function alen (a, i)
{
  for (i in a);
	return i
}

BEGIN {

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
	
	REGEX="'BOOST.*HPP'"
	#REGEX="TEST_BOOST_NO_INTRINSIC_WCHAR_T"
	TEST_DATA_DIR="${BOOST_PATH}"

	print("Num test groups: ", alen(TEST_GROUPS))
	
	COMMAND_LINE=""
	cla_index=1
	
	for (i=1; i <= alen(TEST_GROUPS); i++ )
	{
		j = TEST_GROUPS[i]
		PROG = TEST_GROUP_TO_PROGS[j]
		dirjobs_option = PROG_TO_PARAMS_DIRJOBS[PROG]
		COMMAND_LINE = "{ time " PROG " " TEST_GROUP_TO_PARAMS_PRE[j] " DIRJOBS_PLACEHOLDER SCANJOBS_PLACEHOLDER " REGEX " " TEST_DATA_DIR " 1>&3- 2>&4-; }"
		if (PROG_TO_PARAMS_JOBS[PROG] == "")
		{
			sub(/SCANJOBS_PLACEHOLDER/, "", COMMAND_LINE)
		}
		else
		{
			sub(/SCANJOBS_PLACEHOLDER/, PROG_TO_PARAMS_JOBS[PROG], COMMAND_LINE)
		}
		print(COMMAND_LINE)
		CMD_LINE_ARRAY[cla_index]=COMMAND_LINE
		cla_index++

	}
}

