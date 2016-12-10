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

  const char* retval = (const char*)memmem_short_pattern<16>("abcde", 5, "cd", 2);
  std::string rs(retval, 2);

  EXPECT_EQ("cd", rs);

  retval = (const char*)memmem_short_pattern<16>(M_STRCLEN("abcdefghijklmnopqrstuvwxyz"), "cd", 2);
  rs = std::string(retval, 2);

  EXPECT_EQ("cd", rs);
}

// Tests that Foo does Xyz.
TEST_F(OptimizationsTest, SomeOtherTest) {
  // Exercises the Xyz feature of Foo.
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
