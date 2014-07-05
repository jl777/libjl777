# CMake generated Testfile for 
# Source directory: /Users/jimbolaptop/pNXT/tests
# Build directory: /Users/jimbolaptop/pNXT/src/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
ADD_TEST(coretests "coretests" "--generate_and_play_test_data")
ADD_TEST(crypto "crypto-tests" "/Users/jimbolaptop/pNXT/tests/crypto/tests.txt")
ADD_TEST(difficulty "difficulty-tests" "/Users/jimbolaptop/pNXT/tests/difficulty/data.txt")
ADD_TEST(hash-fast "hash-tests" "fast" "/Users/jimbolaptop/pNXT/tests/hash/tests-fast.txt")
ADD_TEST(hash-slow "hash-tests" "slow" "/Users/jimbolaptop/pNXT/tests/hash/tests-slow.txt")
ADD_TEST(hash-tree "hash-tests" "tree" "/Users/jimbolaptop/pNXT/tests/hash/tests-tree.txt")
ADD_TEST(hash-extra-blake "hash-tests" "extra-blake" "/Users/jimbolaptop/pNXT/tests/hash/tests-extra-blake.txt")
ADD_TEST(hash-extra-groestl "hash-tests" "extra-groestl" "/Users/jimbolaptop/pNXT/tests/hash/tests-extra-groestl.txt")
ADD_TEST(hash-extra-jh "hash-tests" "extra-jh" "/Users/jimbolaptop/pNXT/tests/hash/tests-extra-jh.txt")
ADD_TEST(hash-extra-skein "hash-tests" "extra-skein" "/Users/jimbolaptop/pNXT/tests/hash/tests-extra-skein.txt")
ADD_TEST(hash-target "hash-target-tests")
ADD_TEST(unit_tests "unit_tests")
SUBDIRS(gtest)
