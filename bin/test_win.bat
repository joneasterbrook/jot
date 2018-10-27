
@REM This script checks the output from test.bat in a genuine windows session (not wine).
@REM This started life as one script named test.bat but it proved impossible to make this
@REM work under wine so the testing functions were separed from the status-checking functions.

echo on
 
@REM sample batch script:  /home/jone/ed/wine/MVS10.0/VC/bin/vcvars32.bat 
 
@REM N.B. GetDefaultPath assignment may need changing to suit your environment - see Test 11
  
@cd %JOT_HOME%
  
@REM Tests demonstrating various modes of operation for jot
@REM
@REM History
@REM 29/06/13 Adapted from test.csh

@rm -f test1.txt test2.ref test2.txt test3.txt test3a.txt test4.txt test5.txt
@rm -f test6.ref test6.txt test7.txt test7.ref test7a.txt test7a.ref test8.jot test8.ref test8.txt test9.txt test10.txt
@rm -rf test8.txt.jnl
    
#Test8 setup
@chmod -R u+wx test8.txt.jnl
@cp t.t test8.txt
@chmod ugo+w test8.txt
@echo %%%%A few simple edits to start. > test8.jot
@echo m-0 (i/zzz /r0i/ zzz/m2m-)0 >> test8.jot
@echo m-0f/jon/ >> test8.jot
@echo %%ea=ls >> test8.jot
@echo z.mha >> test8.jot
@echo %%ib=test8.txt >> test8.jot
@echo z.mhb >> test8.jot
@echo m-0f/jon/i/.../ >> test8.jot
@echo %%%%%Remove Bill's folly from text. >> test8.jot
@echo n.a#z#ol13oo/%c/ z.m-0(f'#e)0 >> test8.jot
@echo %%o=test8.txt; >> test8.jot
@echo %%qa=pid; k(mk-,) i.%%e=kill . 'a >> test8.jot

:Test11 setup
@echo %%s=setenv GetDefaultPath z:\usr\include > test11.jot
@echo %%s=setenv HOME C:\Users\jone >> test11.jot
@echo %%  >> test11.jot
@echo z.n.aszs %b=pathname test11.txt; i/Init/b %%o; >> test11.jot
  
@echo %%m=********************************************** Starting Test 11, part 1 >> test11.jot
@echo %%m=Test that it can pick up a partly-defined given file spec            - should return a list of files in test_get >> test11.jot
@echo z. >> test11.jot
@echo getX171     >> test11.jot
@echo 'k-0 %s=sort; ((v/./m)0v"another_dir1/" mv"another_dir2/" mv"hello.c" mv"t.t" (mv"t.t.jnl"m,) v"test.tar" mv"test.tz"' \
    'mv"test_get.tar" mv"test_get.zip" m\, p0%a1=Failure test11 part 1;)' >> test11.jot
@echo zsi/Completed part 1/b %%o; >> test11.jot
 
@echo %%m=********************************************** Starting Test 11, part 2 >> test11.jot
@echo %%m=Test that it can pick up from a given subdirectory of the file path  - should offer all files in test_get/another_dir1 >> test11.jot
@echo z. >> test11.jot
@echo get anotherX171      >> test11.jot
@echo 'm-k-0 %s=sort; (v"another_dir1/" mv"another_dir2/" m\, p0%a1=Failure test11 part 2;)' >> test11.jot
@echo zsi/Completed part 2/b %%o; >> test11.jot
  
@echo %%m=********************************************** Starting Test 11, part 3 >> test11.jot
@echo %%m=Test pickup of a file that echos in in the current-files dir            - should offer four *s* files in test_get (test.tar, test.tz, test_get.tar and test_get.zip) >> test11.jot
@echo z. >> test11.jot
@echo get sX171      >> test11.jot
@echo m-k-0 %s=sort; (v"test.tar" mv"test.tz" mv"test_get.tar" mv"test_get.zip" m\, p0%a1=Failure test11 part 3;) >> test11.jot
@echo zsi/Completed part 3/b %%o; >> test11.jot
  
@echo %%m=********************************************** Starting Test 11, part 4 >> test11.jot
@echo %%m=Test pickup of a local file                                          - should prompt for buffer key then pick up test_get/hello.c >> test11.jot
@echo z. >> test11.jot
@echo get lloX171      >> test11.jot
@echo a >> test11.jot
@echo (%q~=buffer; f"pathName = "f1"test_get/hello.c" z.ok, p0%a1=Failure test11 part 4;) >> test11.jot
@echo zsi/Completed part 4/b %%o; >> test11.jot
 
@echo %%m=********************************************** Starting Test 11, part 5 >> test11.jot
@echo %%m=Test pickup of a file in a local subdirectory                        - should prompt and pick up ./another_dir1/hello.c . >> test11.jot
@echo z. >> test11.jot
@echo get another_dir1/.cX171      >> test11.jot
@echo a >> test11.jot
@echo (%q~=buffer;  f"pathName = "f1"test_get/another_dir1/hello.c" z.ok, p0%a1=Failure test11 part 5;) >> test11.jot
@echo zsi/Completed part 5/b %%o; >> test11.jot
 
@echo %%m=********************************************** Starting Test 11, part 6 >> test11.jot
@echo %%m=Test similar but path incomplete                                     - should offer a selection of items in ./test_get/ . >> test11.jot
@echo z. >> test11.jot
@echo get testX171      >> test11.jot
@echo m-k-0 %s=sort; (v"test.tar" mv"test.tz" mv"test_get.tar" mv"test_get.zip" m\, p0%a1=Failure test11 part 6;) >> test11.jot
@echo zsi/Completed part 6/b %%o; >> test11.jot
 
@echo %%m=********************************************** Starting Test 11, part 7 >> test11.jot
@echo %%m=Test pick up a simple name from text when file local                 - should prompt and pick up .../stdio.h . >> test11.jot
@echo z. f/stdio/ >> test11.jot
@echo get X171      >> test11.jot
@echo a >> test11.jot
@echo %%q~=buffer; (f"pathName = "f1"stdio.h" z.ok, p0%a1=Failure test11 part 7;) >> test11.jot
@echo zsi/Completed part 7/b %%o; >> test11.jot
 
@echo %%m=********************************************** Starting Test 11, part 8 >> test11.jot
@echo %%m=Test pick up a pathname from text when file in GetDefaultPath env    - should prompt and pick up /usr/include/sys/stat.h . >> test11.jot
@echo 'z. f"sys/stat.h"' >> test11.jot
@echo get X171     >> test11.jot
@echo a >> test11.jot
@echo '%q~=buffer; (f"pathName = "f1"sys/stat.h" z.ok, p0%a1=Failure test11 part 8;)' >> test11.jot
@echo zsi/Completed part 8/b %%o; >> test11.jot
 
@echo %%m=********************************************** Starting Test 11, part 9 >> test11.jot
@echo %%m=An uncompressed tarball                                              - should return hello.c from the another_dir1 dir in archive.  >> test11.jot
@echo z. >> test11.jot
@echo get .X171      >> test11.jot
@echo f/test.tar/ >> test11.jot
@echo '0 >> test11.jot
@echo a >> test11.jot
@echo m4 >> test11.jot
@echo '0 >> test11.jot
@echo %%q~=buffer; (f"pathName = [ From CLI command tar -Oxf "f1"test_get/test.tar test_get/t.t ]" z.ok, p0%a1=Failure test11 part 9;) >> test11.jot
@echo zsi/Completed part 9/b %%o; >> test11.jot
 
@echo %%m=********************************************** Starting Test 11, part 10 >> test11.jot
@echo %%m=A compressed tarball                                                 - should return t.t from the another_dir1 dir in archive.  >> test11.jot
@echo z. >> test11.jot
@echo get .X171      >> test11.jot
@echo z+m-0f/test.tz/ >> test11.jot
@echo '0 >> test11.jot
@echo a >> test11.jot
@echo m4f/t.t/ >> test11.jot
@echo '0 >> test11.jot
@echo %%q~=buffer; (f"pathName = [ From CLI command cat "f1"| gunzip | tar -Oxf - test_get/t.t ]" z.ok, p0%a1=Failure test11 part 10;) >> test11.jot
@echo zsi/Completed part 10/b %%o; >> test11.jot
  
@echo %%a0=get.jot seems OK         >> test11.jot

@test.bat

:Test1
@echo Test  1: test.jot - The basic jot-command regression test - test.jot
@grep "Successfully completed all tests" test1.txt > NULL
@if not errorlevel 1 goto Test2
  @echo **** Fail ****
  @set failedTests=%failedTests% 1

:Test2
@diff test2.ref test2.txt
@if not errorlevel 1 goto Test3
  @echo **** Fail ****
  @set failedTests=%failedTests% 2

:Test3
@diff test3.txt t.t
@if not errorlevel 1 goto Test3a
  @echo **** Fail ****
  @set failedTests=%failedTests% 3

:Test3a
@diff test3a.txt t.t
@if not errorlevel 1 goto Test4
  @echo **** Fail ****
  @set failedTests=%failedTests% 3a

:Test4
@diff test4.txt t.t
@if not errorlevel 1 goto Test5
  @echo **** Fail ****
  @set failedTests=%failedTests% 4

:Test5
@diff test5.txt l99.t
@if not errorlevel 1 goto Test6
  @echo **** Fail ****
  @set failedTests=%failedTests% 5

:Test6
@diff test6.ref test6.txt
@if not errorlevel 1 goto Test7
  @echo **** Fail ****
  @set failedTests=%failedTests% 6

:Test7
@diff -q test7.txt test7.ref
@if not errorlevel 1 goto Test7a
  @echo **** Fail ****
  @set failedTests=%failedTests% 7

:Test7a
@diff -q test7a.txt test7a.ref
@if not errorlevel 1 goto Test8
  @echo **** Fail ****
  @set failedTests=%failedTests% 7a

:Test8
@diff test8.ref test8.txt
@Rem 
@if not errorlevel 1 goto Test9
  @echo **** Fail ****
  @set failedTests=%failedTests% 8

:Test9
@for /f "usebackq delims=" %%i in (`type Test9.txt`) do set result=%%i
@if "%result%" == "3 :1234567890" goto Test10
  @echo **** Fail ****
  @set failedTests=%failedTests% 9

:Test10
@for /f "usebackq delims=" %%i in (type Test10.txt) do set result=%%i
echo Test10: %result%
@if "%result%" == "3 :1234567890" goto Test11
  @echo **** Fail ****
  @set failedTests=%failedTests% 10

@REM :Test11
@REM @echo %%%%Test 11 command script. > test11.jot
@REM @echo %%s=setenv GetDefaultPath z:\usr\include >> test11.jot
@REM @if not exist "C:\Program Files\Microsoft Visual Studio 10.0\VC\INCLUDE" goto Test11_Wine
@REM    @echo %%s=setenv GetDefaultPath C:\Program Files\Microsoft Visual Studio 10.0\VC\INCLUDE >> test11.jot
@REM @echo %%s=setenv HOME C:\Users\jone >> test11.jot
@REM @echo %%d^^=Xd0     obz!n.a$**z$i-`%%r=##`(f1/ /bi/%%%%/r0b,)oz'$ >> test11.jot
@REM @echo z^^l0ol38 (f/**/o#oo/%%c/)0 okp >> test11.jot
@REM @REM @echo z^^l0ol38 (f/**/o#oo/%%)0 okp >> test11.jot
@REM @echo %%%%  >> test11.jot
@REM @echo z.n.aszs %%b=pathname test11.txt; i/Init/b %%o; >> test11.jot
@REM   
@REM @echo %%m=********************************************** Starting Test 11, part 1 >> test11.jot
@REM @echo %%m=Test that it can pick up a partly-defined given file spec            - should return a list of files in test_get >> test11.jot
@REM @echo z. >> test11.jot
@REM @echo getXd0     >> test11.jot
@REM @echo k-0 %%s=sort; ((v/./m)0v"another_dir1/" mv"another_dir2/" mv"hello.c" mv"t.t" (mv"t.t.jnl"m,) v"test.tar" mv"test.tz" ^
@REM     mv"test_get.tar" mv"test_get.zip" m\, p0%%a1=Failure test11 part 1;) >> test11.jot
@REM @echo zsi/Completed part 1/b %%o; >> test11.jot
@REM  
@REM @echo %%m=********************************************** Starting Test 11, part 2 >> test11.jot
@REM @echo %%m=Test that it can pick up from a given subdirectory of the file path  - should offer all files in test_get/another_dir1 >> test11.jot
@REM @echo z. >> test11.jot
@REM @echo get anotherXd0      >> test11.jot
@REM @echo m-k-0 %%s=sort; (v"another_dir1/" mv"another_dir2/" m\, p0%%a1=Failure test11 part 2;) >> test11.jot
@REM @echo zsi/Completed part 2/b %%o; >> test11.jot
@REM   
@REM @echo %%m=********************************************** Starting Test 11, part 3 >> test11.jot
@REM @echo %%m=Test pickup of a file thatecho s in in the current-files dir            - should offer four *s* files in test_get (test.tar, test.tz, test_get.tar and test_get.zip) >> test11.jot
@REM @echo z. >> test11.jot
@REM @echo get sXd0      >> test11.jot
@REM @echo m-k-0 %%s=sort; (v"test.tar" mv"test.tz" mv"test_get.tar" mv"test_get.zip" m\, p0%%a1=Failure test11 part 3;) >> test11.jot
@REM @echo zsi/Completed part 3/b %%o; >> test11.jot
@REM   
@REM @echo %%m=********************************************** Starting Test 11, part 4 >> test11.jot
@REM @echo %%m=Test pickup of a local file                                          - should prompt for buffer key then pick up test_get/hello.c >> test11.jot
@REM @echo z. >> test11.jot
@REM @echo get lloXd0      >> test11.jot
@REM @echo a>> test11.jot
@REM @echo (%%q~=buffer; f"pathName = "f1"test_get/hello.c" z.ok, p0%%a1=Failure test11 part 4;) >> test11.jot
@REM @echo zsi/Completed part 4/b %%o; >> test11.jot
@REM  
@REM @echo %%m=********************************************** Starting Test 11, part 5 >> test11.jot
@REM @echo %%m=Test pickup of a file in a local subdirectory                        - should prompt and pick up ./another_dir1/hello.c . >> test11.jot
@REM @echo z. >> test11.jot
@REM @echo get another_dir1/.cXd0      >> test11.jot
@REM @echo a>> test11.jot
@REM @echo (%%q~=buffer;  f"pathName = "f1"test_get/another_dir1/hello.c" z.ok, p0%%a1=Failure test11 part 5;) >> test11.jot
@REM @echo zsi/Completed part 5/b %%o; >> test11.jot
@REM  
@REM @echo %%m=********************************************** Starting Test 11, part 6 >> test11.jot
@REM @echo %%m=Test similar but path incomplete                                     - should offer a selection of items in ./test_get/ . >> test11.jot
@REM @echo z. >> test11.jot
@REM @echo get testXd0      >> test11.jot
@REM @echo m-k-0 %%s=sort; (v"test.tar" mv"test.tz" mv"test_get.tar" mv"test_get.zip" m\, p0%%a1=Failure test11 part 6;) >> test11.jot
@REM @echo zsi/Completed part 6/b %%o; >> test11.jot
@REM  
@REM @echo %%m=********************************************** Starting Test 11, part 7 >> test11.jot
@REM @echo %%m=Test pick up a simple name from text when file local                 - should prompt and pick up .../stdio.h . >> test11.jot
@REM @echo z. f/stdio/ >> test11.jot
@REM @echo get Xd0      >> test11.jot
@REM @echo a>> test11.jot
@REM @echo %%q~=buffer; (f"pathName = "f1"stdio.h" z.ok, p0%%a1=Failure test11 part 7;) >> test11.jot
@REM @echo zsi/Completed part 7/b %%o; >> test11.jot
@REM  
@REM @echo %%m=********************************************** Starting Test 11, part 8 >> test11.jot
@REM @echo %%m=Test pick up a pathname from text when file in GetDefaultPath env    - should prompt and pick up /usr/include/sys/stat.h . >> test11.jot
@REM @echo z. f"sys/stat.h" >> test11.jot
@REM @echo get Xd0      >> test11.jot
@REM @echo a>> test11.jot
@REM @echo %%q~=buffer; (f"pathName = "f1"sys/stat.h" z.ok, p0%%a1=Failure test11 part 8;) >> test11.jot
@REM @echo zsi/Completed part 8/b %%o; >> test11.jot
@REM  
@REM @echo %%m=********************************************** Starting Test 11, part 9 >> test11.jot
@REM @echo %%m=An uncompressed tarball                                              - should return hello.c from the another_dir1 dir in archive.  >> test11.jot
@REM @echo z. >> test11.jot
@REM @echo get .Xd0      >> test11.jot
@REM @echo f/test.tar/ >> test11.jot
@REM @echo '0 >> test11.jot
@REM @echo a>> test11.jot
@REM @echo m4 >> test11.jot
@REM @echo '0 >> test11.jot
@REM @echo %%q~=buffer; (f"pathName = [ From CLI command tar -Oxf "f1"test_get/test.tar test_get/t.t ]" z.ok, p0%%a1=Failure test11 part 9;) >> test11.jot
@REM @echo zsi/Completed part 9/b %%o; >> test11.jot
@REM  
@REM @echo %%m=********************************************** Starting Test 11, part 10 >> test11.jot
@REM @echo %%m=A compressed tarball                                                 - should return t.t from the another_dir1 dir in archive.  >> test11.jot
@REM @echo z+m-0f/test.tz/ >> test11.jot
@REM @echo '0 >> test11.jot
@REM @echo a>> test11.jot
@REM @echo m4f/t.t/ >> test11.jot
@REM @echo '0 >> test11.jot
@REM @echo %%q~=buffer; (f"pathName = [ From CLI command cat "f1"| gunzip | tar -Oxf - test_get/t.t ]" z.ok, p0%%a1=Failure test11 part 10;) >> test11.jot
@REM @REM @echo zsi/Completed part 10/b \%\%o; >> test11.jot
  
@echo %%a0=get.jot seems OK         >> test11.jot
@grep "Completed part 10" test11.txt
  
@if not errorlevel 1 goto CheckFails
  @echo **** Fail ****
  @set failedTests=%failedTests% 11

:CheckFails
@echo "Failed tests: %failedTests%
@if "%failedTests%" == "" goto Pass
  @echo *********The following tests failed: %failedTests%
@goto AllDone

:Pass
@echo Passed all tests. 

:AllDone
%