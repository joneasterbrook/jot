#! /bin/csh -f
#Checks output from tests demonstrating various modes of operation for jot under wine.
#
#History
#23/07/14 Adapted from test.bat
 
#This script launches test.bat in linux then checks the output.
#These scripts started life as one script named test.bat but it proved too difficult to make this
#work under wine so the testing functions were separated from the status-checking functions.
   
cd ${JOT_HOME}
  
\rm -f test1.txt test2.ref test2.txt test2a.ref test2a.txt test3.txt test3a.txt test4.txt test4a.txt test5.txt \
    test6.ref test6.txt test7.txt test7.ref test7a.txt test7a.ref test8.jot test8.ref test8.txt test9.txt test10.txt

##Redirection of echo output seems a bit flaky in wine - try doing it now, in linuxland.
##Test8 setup
#\chmod -R u+wx test8.txt.jnl
#\rm -rf test8.txt.jnl
#\rm -f test8.txt test8.ref
#\cp t.t test8.txt
#\chmod ugo+w test8.txt
#echo "%%A few simple edits to start." > test8.jot
#echo "m-0 (i/zzz /r0i/ zzz/m2m-)0" >> test8.jot
#echo "m-0f/jon/" >> test8.jot
#echo "%ea=ls" >> test8.jot
#echo "z.mha" >> test8.jot
#echo "%ib=test8.txt" >> test8.jot
#echo "z.mhb" >> test8.jot
#echo "m-0f/jon/i/.../" >> test8.jot
#echo "%%Remove Bill's folly from text." >> test8.jot
#echo "n.a#z#ol13oo/%c/ z.m-0(f'#e)0" >> test8.jot
#echo "%o=test8.txt;" >> test8.jot
#echo "%qa=pid; ki.%e=kill . 'a" >> test8.jot

#Test11 setup
echo %s=setenv GetDefaultPath z:\\usr\\include > test11.jot
echo "%s=setenv HOME C:\Users\jone" >> test11.jot
echo "%% " >> test11.jot
echo "z.n.aszs %b=pathname test11.txt; i/Init/b %o;" >> test11.jot
  
echo "%m=********************************************** Starting Test 11, part 1" >> test11.jot
echo "%m=Test that it can pick up a partly-defined given file spec            - should return a list of files in test_get" >> test11.jot
echo "z." >> test11.jot
echo "getX171    " >> test11.jot
echo 'k-0 %s=sort; ((v/./m)0v"another_dir1/" mv"another_dir2/" mv"hello.c" mv"t.t" (mv"t.t.jnl"m,) v"test.tar" mv"test.tz"' \
    'mv"test_get.tar" mv"test_get.zip" m\, p0%a1=Failure test11 part 1;)' >> test11.jot
echo "zsi/Completed part 1/b %o;" >> test11.jot
 
echo "%m=********************************************** Starting Test 11, part 2" >> test11.jot
echo "%m=Test that it can pick up from a given subdirectory of the file path  - should offer all files in test_get/another_dir1" >> test11.jot
echo "z." >> test11.jot
echo "get anotherX171     " >> test11.jot
echo 'm-k-0 %s=sort; (v"another_dir1/" mv"another_dir2/" m\, p0%a1=Failure test11 part 2;)' >> test11.jot
echo "zsi/Completed part 2/b %o;" >> test11.jot
  
echo "%m=********************************************** Starting Test 11, part 3" >> test11.jot
echo "%m=Test pickup of a file that echos in in the current-files dir            - should offer four *s* files in test_get (test.tar, test.tz, test_get.tar and test_get.zip)" >> test11.jot
echo "z." >> test11.jot
echo "get sX171     " >> test11.jot
echo "m-k-0 %s=sort; (v"test.tar" mv"test.tz" mv"test_get.tar" mv"test_get.zip" m\, p0%a1=Failure test11 part 3;)" >> test11.jot
echo "zsi/Completed part 3/b %o;" >> test11.jot
  
echo "%m=********************************************** Starting Test 11, part 4" >> test11.jot
echo "%m=Test pickup of a local file                                          - should prompt for buffer key then pick up test_get/hello.c" >> test11.jot
echo "z." >> test11.jot
echo "get lloX171     " >> test11.jot
echo "a" >> test11.jot
echo "(%q~=buffer; f"pathName = "f1"test_get/hello.c" z.ok, p0%a1=Failure test11 part 4;)" >> test11.jot
echo "zsi/Completed part 4/b %o;" >> test11.jot
 
echo "%m=********************************************** Starting Test 11, part 5" >> test11.jot
echo "%m=Test pickup of a file in a local subdirectory                        - should prompt and pick up ./another_dir1/hello.c ." >> test11.jot
echo "z." >> test11.jot
echo "get another_dir1/.cX171     " >> test11.jot
echo "a" >> test11.jot
echo "(%q~=buffer;  f"pathName = "f1"test_get/another_dir1/hello.c" z.ok, p0%a1=Failure test11 part 5;)" >> test11.jot
echo "zsi/Completed part 5/b %o;" >> test11.jot
 
echo "%m=********************************************** Starting Test 11, part 6" >> test11.jot
echo "%m=Test similar but path incomplete                                     - should offer a selection of items in ./test_get/ ." >> test11.jot
echo "z." >> test11.jot
echo "get testX171     " >> test11.jot
echo "m-k-0 %s=sort; (v"test.tar" mv"test.tz" mv"test_get.tar" mv"test_get.zip" m\, p0%a1=Failure test11 part 6;)" >> test11.jot
echo "zsi/Completed part 6/b %o;" >> test11.jot
 
echo "%m=********************************************** Starting Test 11, part 7" >> test11.jot
echo "%m=Test pick up a simple name from text when file local                 - should prompt and pick up .../stdio.h ." >> test11.jot
echo "z. f/stdio/" >> test11.jot
echo "get X171     " >> test11.jot
echo "a" >> test11.jot
echo "%q~=buffer; (f"pathName = "f1"stdio.h" z.ok, p0%a1=Failure test11 part 7;)" >> test11.jot
echo "zsi/Completed part 7/b %o;" >> test11.jot
 
echo "%m=********************************************** Starting Test 11, part 8" >> test11.jot
echo "%m=Test pick up a pathname from text when file in GetDefaultPath env    - should prompt and pick up /usr/include/sys/stat.h ." >> test11.jot
echo 'z. f"sys/stat.h"' >> test11.jot
echo "get X171    " >> test11.jot
echo "a" >> test11.jot
echo '%q~=buffer; (f"pathName = "f1"sys/stat.h" z.ok, p0%a1=Failure test11 part 8;)' >> test11.jot
echo "zsi/Completed part 8/b %o;" >> test11.jot
 
echo "%m=********************************************** Starting Test 11, part 9" >> test11.jot
echo "%m=An uncompressed tarball                                              - should return hello.c from the another_dir1 dir in archive. " >> test11.jot
echo "z." >> test11.jot
echo "get .X171     " >> test11.jot
echo "f/test.tar/" >> test11.jot
echo "'0" >> test11.jot
echo "a" >> test11.jot
echo "m4" >> test11.jot
echo "'0" >> test11.jot
echo "%q~=buffer; (f"pathName = [ From CLI command tar -Oxf "f1"test_get/test.tar test_get/t.t ]" z.ok, p0%a1=Failure test11 part 9;)" >> test11.jot
echo "zsi/Completed part 9/b %o;" >> test11.jot
 
echo "%m=********************************************** Starting Test 11, part 10" >> test11.jot
echo "%m=A compressed tarball                                                 - should return t.t from the another_dir1 dir in archive. " >> test11.jot
echo "z." >> test11.jot
echo "get .X171     " >> test11.jot
echo "z+m-0f/test.tz/" >> test11.jot
echo "'0" >> test11.jot
echo "a" >> test11.jot
echo "m4f/t.t/" >> test11.jot
echo "'0" >> test11.jot
echo "%q~=buffer; (f"pathName = [ From CLI command cat "f1"| gunzip | tar -Oxf - test_get/t.t ]" z.ok, p0%a1=Failure test11 part 10;)" >> test11.jot
echo "zsi/Completed part 10/b %o;" >> test11.jot
  
echo "%a0=get.jot seems OK        " >> test11.jot
  
\wineconsole test.bat
  
set failedTests=""

##Test1
#\echo Test  1: test.jot - The basic jot-command regression test - test.jot
#\grep "Successfully completed all tests" test1.txt > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 1"
#  endif
#
##Test2
#\echo Test  2: streaming out to stdout in -tty mode.
#\diff test2.ref test2.txt --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 2"
#  endif
#
##Test3
#\echo Test  3: Accepting input from stream in -tty mode.
#\diff test3.txt t.t --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 3"
#  endif
#
##Test3a
#\echo Test  3a: Accepting input from stream - non-tty mode.
#\diff test3a.txt t.t --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 3a"
#  endif
#
##Test4
#\echo Test  4: Accepting input from stream and piping to stdout in -tty mode.
#\diff test4.txt t.t --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 4"
#  endif
#
##Test5
#\echo Test  5: Read from CLI command.
#\diff test5.txt l99.t --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 5"
#  endif
#
##Test6
#\echo Test  6: Filter mode.
#\diff test6.ref test6.txt --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 6"
#  endif
#
##Test7
#\echo Test  7: streamed-in commands in -tty mode.
#\diff -q test7.txt test7.ref --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 7"
#  endif
#
##Test7a
#\echo Test  7a: streamed-in commands in non-tty mode.
#\diff -q test7a.txt test7a.ref --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 7a"
#  endif
#
##Test8
#echo Test  8: journal-keeping/recovery test.
#\diff test8.ref test8.txt --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 8"
#  endif
#
##Test9
#echo Test  9: piping stdout in -tty mode.
#echo "3 :1234567890" | \diff - test9.txt --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 9"
#  endif
#
##Test10
#\echo Test  10: piping stdout non-tty mode
#echo "3 :1234567890" | \diff - test10.txt --strip-trailing-cr > /dev/null
#if ("${status}" != "0") then
#  echo "**** Fail ****"
#  set failedTests="${failedTests} 10"
#  endif

echo Test 11: get.jot basic operations
\grep "Completed part 10" test11.txt > /dev/null
if ("${status}" != "0") then
  echo "**** Fail ****"
  \tail -1 test11.txt
  set failedTests="${failedTests} 11"
  endif

if ("${failedTests}" != "") then
  \echo "*********The following tests failed: ${failedTests}"
  exit -1
  else
  \echo "Passed all tests."
  exit 0
  endif
