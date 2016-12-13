/*
 * Copyright 2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of UniversalCodeGrep.
 *
 * UniversalCodeGrep is free software: you can redistribute it and/or modify it under the
 * terms of version 3 of the GNU General Public License as published by the Free
 * Software Foundation.
 *
 * UniversalCodeGrep is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * UniversalCodeGrep.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file */


#include "../src/libext/memory.hpp"
#include "gtest/gtest.h"

namespace {

// The fixture for testing class Foo.
class OptimizationsTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

	OptimizationsTest() {
    // You can do set-up work for each test here.
  }

  virtual ~OptimizationsTest() {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  virtual void SetUp() {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  virtual void TearDown() {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test case for Foo.
};

// Tests that the Foo::Bar() method does Abc.
TEST_F(OptimizationsTest, memmem_short_pattern_works) {
  const std::string input_filepath = "this/package/testdata/myinputfile.dat";
  const std::string output_filepath = "this/package/testdata/myoutputfile.dat";

#define M_STRCLEN(str) (str), strlen(str)

  const char* retval = (const char*)MV_USE(memmem_short_pattern,ISA_x86_64::SSE4_2)("abcde", 5, "cd", 2);
  std::string rs(retval, 2);

  EXPECT_EQ("cd", rs);

  retval = (const char*)MV_USE(memmem_short_pattern,ISA_x86_64::SSE4_2)(M_STRCLEN("abcdefghijklmnopqrstuvwxyz"), "cd", 2);
  rs = std::string(retval, 2);

  EXPECT_EQ("cd", rs);
}

TEST_F(OptimizationsTest, memmem_short_pattern_vs_32_bytes)
{
  std::string str = "0123456789ABCDEFfedcba9876543210";
  std::string rs = "10";
  std::string matchstr {(const char*)MV_USE(memmem_short_pattern,ISA_x86_64::SSE4_2)(str.c_str(), 32, rs.c_str(), 2), rs.length()};
  EXPECT_EQ(rs, matchstr);
}

TEST_F(OptimizationsTest, memmem_short_pattern_vs_38_bytes)
{
  std::string str = "0123456789ABCDEFfedcba9876543210qwerty";
  std::string rs = "0qw";
  std::string matchstr {(const char*)MV_USE(memmem_short_pattern,ISA_x86_64::SSE4_2)(str.c_str(), str.length(), rs.c_str(), rs.length()), rs.length()};
  EXPECT_EQ(rs, matchstr);
}

// Tests that Foo does Xyz.
TEST_F(OptimizationsTest, Renamed) {
  // Exercises the Xyz feature of Foo.
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
