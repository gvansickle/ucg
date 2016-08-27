#!/usr/bin/python2.7
# encoding: utf-8

from __future__ import print_function

copyright_notice=\
'''
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
'''

import sys
import os
from argparse import ArgumentParser
from argparse import RawDescriptionHelpFormatter

import sqlite3
import csv
from string import Template

test_script_template_1 = Template("""\
#!/bin/sh
# GENERATED FILE, DO NOT EDIT
NUM_ITERATIONS=${num_iterations};
echo "Starting performance tests, results file is '${results_file}'";

${test_cases}

""")

prep_run_template = Template("""\
# Prep run.
# We do a prep run before each group of timing runs to eliminate disk cache variability and capture the matches.
# We pipe the results through sort so we can diff these later.
echo "Timing: \\"${cmd_line}\\"" >> ${results_file}
echo "Prep run for wrapped command line: ${wrapped_cmd_line}" > ${search_results_file}
${wrapped_cmd_line}

""")

timing_run_template = Template("""\
# Timing runs.
TIME_RESULTS_FILE=("./time_results_" i ".txt");
wrapped_cmd_line=("{{ " COMMAND_LINE " 2>> " TIME_RESULTS_FILE " ; }} 3>&1 4>&2;");
echo \"Timing run for wrapped command line: '" wrapped_cmd_line "'\" > " TIME_RESULTS_FILE);
echo \"TEST_PROG_ID: " PROG_ID "\" >> " TIME_RESULTS_FILE);
echo \"TEST_PROG_PATH: " PROG_PATH "\" >> " TIME_RESULTS_FILE);
for ITER in $$(seq 0 $$(expr $$NUM_ITERATIONS - 1));
do
    # Do a single run.
    wrapped_cmd_line
done;
""")

class TestRunResultsDatabase(object):
    '''
    classdocs
    '''

    # AWK extract column number from name.
    # awk -v col=exename 'BEGIN{ FS="[[:space:]]*,[[:space:]]*"; } NR==1 { for(i=1;i<=NF;i++) { if($i==col) print("i=" i); } }' ../tests/benchmark_progs.csv

    def __init__(self):
        '''
        Constructor
        '''
        self.dbconnection = sqlite3.connect(":memory:")
        # Turn on foreign key support.
        self.dbconnection.execute("PRAGMA foreign_keys = ON")
        
        # Register a suitable csv dialect.
        csv.register_dialect('ucg_nonstrict', delimiter=',', doublequote=True, escapechar="\\", quotechar=r'"',
                             quoting=csv.QUOTE_MINIMAL, skipinitialspace=True)
        
    def _placeholders(self, num):
        """
        Helper function which generates a string of num placeholders (e.g. for num==5, returns "?,?,?,?,?").
        :param num: Number of placeholders to generate.
        """
        qmarks = "?"
        for col in range(1,num):
            qmarks += ",?"
        return qmarks
        
    def _create_tables(self):
        c = self.dbconnection.cursor()
        
        c.execute('''CREATE TABLE prog_info (prog_id,
            version_line,
             name,
              version,
               package_name,
               pathname,
               libpcre_major_ver,
               libpcre_version,
               libpcre_jit,
               isa_exts_in_use)''')
        
        self.dbconnection.commit()
        
    def _select_data(self, output_table_name=None):
        
        # Use a Row object.
        self.dbconnection.row_factory = sqlite3.Row
        c = self.dbconnection.cursor()
                
        # Do a cartesian join.
        c.execute("""CREATE TABLE {} AS SELECT test_cases.test_case_id, progsundertest.prog_id, progsundertest.exename, progsundertest.pre_options,
                coalesce(progsundertest.opt_dirjobs, '') as opt_dirjobs, coalesce(test_cases.regex,'') || "   " || coalesce(test_cases.corpus,'') AS CombinedColumnsTest
            FROM test_cases
            CROSS JOIN progsundertest
            """.format(output_table_name))
        
        # Print the column headers.
        #r = c.fetchall()
        #print(", ".join(r[0].keys()))
        # Print the rows.
        #for row in r:
        #    print("{}".format(", ".join(row)))
        
    def generate_tests_type_1(self, output_table_name=None):
        # Use a Row object.
        self.dbconnection.row_factory = sqlite3.Row
        c = self.dbconnection.cursor()
        c.execute("""CREATE TABLE {} AS
        SELECT t.test_case_id, p.prog_id, p.exename, p.pre_options, o.opt_expansion, t.regex, t.corpus
        FROM test_cases AS t CROSS JOIN progsundertest as p
        INNER JOIN opts_defs as o ON (o.opt_id = p.opt_only_cpp)
        """.format(output_table_name))
            
    def read_csv_into_table(self, table_name=None, filename=None, prim_key=None, foreign_key_tuples=None):
        c = self.dbconnection.cursor()
        if not foreign_key_tuples: foreign_key_tuples = []
        with open(filename) as csvfile:
            reader = csv.DictReader(csvfile, dialect='ucg_nonstrict')
            headers = [fn for fn in reader.fieldnames]
            decorated_headers = headers
            if prim_key:
                if prim_key not in headers:
                    raise Exception("Primary key '{}' not in csv file.")
                else:
                    decorated_headers = [ h.replace(prim_key, prim_key + " PRIMARY KEY ") for h in decorated_headers]
            qmarks = self._placeholders(len(headers))
            foreign_key_strs = []
            for (col, other_col) in foreign_key_tuples:
                foreign_key_strs.append("FOREIGN KEY({}) REFERENCES {}".format(col, other_col))
            sql_str = "CREATE TABLE {} ({})".format(table_name, ", ".join(decorated_headers+foreign_key_strs))
            print(sql_str)
            c.execute(sql_str)
            self.dbconnection.commit()
            for row in reader:
                print("row: {}".format(row))
                to_db = []
                for h in headers:
                    to_db.append(row[h])
                c.execute('''INSERT INTO {} VALUES ({})'''.format(table_name, qmarks), to_db)
        self.dbconnection.commit()
        #c.execute('SELECT * from csv_test')
        #print(c.fetchall())
       
    def PrintTable(self, table_name=None):
        # Use a Row object.
        self.dbconnection.row_factory = sqlite3.Row
        c = self.dbconnection.cursor()
        c.execute('SELECT * from {}'.format(table_name))
        rows = c.fetchall()
        #print("Type: {}".format(type(rows[0])))
        print("TABLE NAME : {}".format(table_name))
        print("Header     : " + ", ".join(rows[0].keys()))
        for row in rows:
            print("Row        : " + ", ".join(row))
    
    def GenerateTestScript(self):
        ###
        ### Output the test script.
        ###
        test_cases = ""
        for tc in range(6): ### @todo Num test cases.
            cmd_line="gfadsgfajkgkajsgfjgs"
            wrapped_cmd_line='''"{{ {cmd_line} 2>> {search_results_file} ; }} 3>&1 4>&2 | sort >> {search_results_file} ;"'''.format(cmd_line=cmd_line, search_results_file="SearchResults_{}.txt".format(tc))
            test_case = prep_run_template.substitute(
                results_file='/dev/null',
                search_results_file="SearchResults_{}.txt".format(tc),
                cmd_line=cmd_line,
                wrapped_cmd_line=wrapped_cmd_line
                ) + "\n" +\
                timing_run_template.substitute(
                    )
            test_cases += test_case + "\n"
        script = test_script_template_1.substitute(
            num_iterations=3,
            results_file='/dev/null',
            test_cases=test_cases
            )
        print(script)
        #with open(dir_out + "/report.html", "w") as f: 
        #    print(page, file=f)
            
        
#         cla_alen = alen(CMD_LINE_ARRAY)
#         for ( i = 1; i <= cla_alen; i++ )
#         {
#             acopy(CMD_LINE_ARRAY, CLA_COPY)
#             #print(">>1 :", CMD_LINE_ARRAY[i]);
#             split(CMD_LINE_ARRAY[i], CMD_LINE_ARRAY_2, "[[:space:]]+![[:space:]]+");
#             #print(">>1 :", CMD_LINE_ARRAY_2[1], CMD_LINE_ARRAY_2[2], CMD_LINE_ARRAY_2[3])
#             PROG_ID=CMD_LINE_ARRAY_2[1]
#             PROG_PATH=CMD_LINE_ARRAY_2[2]
#             COMMAND_LINE=CMD_LINE_ARRAY_2[3]
#             
#             print("")
#             print("# Prep run, to eliminate disk cache variability and capture the matches.")
#             print("# We pipe the results through sort so we can diff these later.")
#             print("echo \"Timing: " COMMAND_LINE "\" >>", RESULTS_FILE);
#             PREP_RUN_FILES[i]=("SearchResults_" i ".txt");
#             wrapped_cmd_line=("{ " COMMAND_LINE " 2>> " PREP_RUN_FILES[i] " ; } 3>&1 4>&2 | sort >> " PREP_RUN_FILES[i] ";");
#             print("echo \"Prep run for wrapped command line: '" wrapped_cmd_line "'\" > " PREP_RUN_FILES[i]);
#             print(wrapped_cmd_line);
#             print("");
#             print("# Timing runs.");
#             TIME_RESULTS_FILE=("./time_results_" i ".txt");
#             wrapped_cmd_line=("{ " COMMAND_LINE " 2>> " TIME_RESULTS_FILE " ; } 3>&1 4>&2;");
#             print("echo \"Timing run for wrapped command line: '" wrapped_cmd_line "'\" > " TIME_RESULTS_FILE);
#             print("echo \"TEST_PROG_ID: " PROG_ID "\" >> " TIME_RESULTS_FILE);
#             print("echo \"TEST_PROG_PATH: " PROG_PATH "\" >> " TIME_RESULTS_FILE);
#             print("for ITER in $(seq 0 $(expr $NUM_ITERATIONS - 1));\ndo")
#             print("    # Do a single run.")
#             print("    " wrapped_cmd_line);
#             print("done;");
#         }
        
    def test(self):
        print("sqlite3 lib version: {}".format(sqlite3.sqlite_version))
        self._create_tables()
        self.read_csv_into_table(table_name="opts_defs", filename='opts_defs.csv', prim_key='opt_id')
        self.read_csv_into_table(table_name="progsundertest", filename='benchmark_progs.csv',
                                 foreign_key_tuples=[("opt_only_cpp", "opts_defs(opt_id)")])
        self.PrintTable("progsundertest")
        self.read_csv_into_table(table_name="test_cases", filename='test_cases.csv')
        self._select_data("benchmark_1")
        self.PrintTable("benchmark_1")
        self.generate_tests_type_1("benchmark1")
        self.PrintTable("benchmark1")
        
        self.GenerateTestScript()
        
        self.dbconnection.close()
        pass


def main(argv=None): # IGNORE:C0111
    '''Command line options.'''

    if argv is None:
        argv = sys.argv
    else:
        sys.argv.extend(argv)

    program_name = os.path.basename(sys.argv[0])
    #program_version = "v%s" % __version__
    #program_build_date = str(__updated__)
    #program_version_message = '%%(prog)s %s (%s)' % (program_version, program_build_date)
    program_version_message = "<unknown>"
    #program_shortdesc = __import__('__main__').__doc__.split("\n")[1]
    program_license = copyright_notice # % (program_shortdesc)

    try:
        # Setup argument parser
        parser = ArgumentParser(description=program_license, formatter_class=RawDescriptionHelpFormatter)
        parser.add_argument("-v", "--verbose", dest="verbose", action="count", help="set verbosity level [default: %(default)s]")
        parser.add_argument("-i", "--include", dest="include", help="only include paths matching this regex pattern. Note: exclude is given preference over include. [default: %(default)s]", metavar="RE" )
        parser.add_argument("-e", "--exclude", dest="exclude", help="exclude paths matching this regex pattern. [default: %(default)s]", metavar="RE" )
        parser.add_argument('-V', '--version', action='version', version=program_version_message)
        parser.add_argument(dest="paths", help="paths to folder(s) with source file(s) [default: %(default)s]", metavar="path", nargs='?')

        # Process arguments
        args = parser.parse_args()

        paths = args.paths
        verbose = args.verbose
        inpat = args.include
        expat = args.exclude

        if verbose > 0:
            print("Verbose mode on")

        if inpat and expat and inpat == expat:
            raise CLIError("include and exclude pattern are equal! Nothing will be processed.")

        #inpath = paths[0]
        #outdir = paths[1] 
        
        results_db = TestRunResultsDatabase()
        results_db.test()
            
        return 0
    except KeyboardInterrupt:
        ### handle keyboard interrupt ###
        return 0

if __name__ == "__main__":
    sys.exit(main())
