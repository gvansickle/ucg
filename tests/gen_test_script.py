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

import sqlite3
import csv

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
        
        c.execute('''CREATE TABLE progsundertest (prog_id,
            exename,
            pre_options,
            dirjobs_option,
            scanjobs_option)''')
        
        c.execute('''CREATE TABLE test_cases(test_case_id,
            short_description,
            description,
            regex,
            test_path)''')
        
        self.dbconnection.commit()
        
    def _insert_data(self):
        c = self.dbconnection.cursor()
        
        progsundertest = [('built_ucg', 'ucg', '--noenv', '--dirjobs=N', '-jN'),
                          ('system_ucg', '/usr/bin/ucg', '--noenv', '--dirjobs=N', '-jN'),
                          ('gnu_grep', 'ggrep', '-Ern --color', '', '')
                          ]
        
        c.executemany('INSERT INTO progsundertest VALUES (?,?,?,?,?)', progsundertest)
        
        test_cases = [('TC1', 'Test case 1', 'This is the long description of Test Case 1', 'BOOST.*HPP', '../../boost_1_58_0'),
                      ('TC2', 'Test Case 2', 'This is the long description of Test Case 2', 'BOOST.*?HPP', '../../boost_1_58_0')
                      ]
        
        c.executemany('INSERT INTO test_cases VALUES (?,?,?,?,?)', test_cases)

    def _select_data(self):
        
        # Use a Row object.
        self.dbconnection.row_factory = sqlite3.Row
        c = self.dbconnection.cursor()
                
        # Do a cartesian join.
        c.execute("""SELECT test_cases.test_case_id, progsundertest.prog_id, progsundertest.exename, progsundertest.pre_options,
                coalesce(progsundertest.dirjobs_option, '') as dirjobs_option, coalesce(test_cases.regex,'') || "   " || coalesce(test_cases.test_path,'') AS CombinedColumnsTest
            FROM test_cases
            CROSS JOIN progsundertest
            """)
        r = c.fetchall()
        
        # Print the column headers.
        print ", ".join(r[0].keys())
        # Print the rows.
        for row in r:
            print("{}".format(", ".join(row)))
            
    def _read_csv(self, filename=None):
        c = self.dbconnection.cursor()
        with open(filename) as csvfile:
            reader = csv.DictReader(csvfile)
            headers = [fn for fn in reader.fieldnames]
            qmark = "?"
            for col in range(1,len(headers)):
                qmark += ",?"
            print(headers)
            print(qmark)
            sql_str = "CREATE TABLE csv_test ({})".format(", ".join(headers))
            print(sql_str)
            c.execute(sql_str)
            self.dbconnection.commit()
            for row in reader:
                print("row: {}".format(row))
                to_db = []
                for h in headers:
                    to_db.append(row[h])
                c.execute('''INSERT INTO csv_test VALUES ({})'''.format(qmark), to_db)
        self.dbconnection.commit()
        c.execute('SELECT * from csv_test')
        print(c.fetchall())
       
    def PrintTable(self, table_name=None):
        # Use a Row object.
        self.dbconnection.row_factory = sqlite3.Row
        c = self.dbconnection.cursor()
        c.execute('SELECT * from csv_test')
        rows = c.fetchall()
        #print("Type: {}".format(type(rows[0])))
        print("TABLE NAME : {}".format(table_name))
        print("Header     : " + ", ".join(rows[0].keys()))
        for row in rows:
            print("Row        : " + ", ".join(row))
        
        
    def test(self):
        self._create_tables()
        self._insert_data()
        self._select_data()
        self._read_csv('benchmark_progs.csv')
        self.PrintTable("csv_test")
        self.dbconnection.close()
        pass
