#! /bin/csh -f
cd ${JOT_HOME}
  
#Tests demonstrating various modes of operation for jot
#Examples:
#   $ ./test.csh                - runs tests using jot_dev - the default name for the jot executable.
#   $ ./test.csh <exePathName>  - runs tests using ${exeName}
#History
#16/09/12 Created using test.ecc
  
set failedTests=""
set jotExe=./jot_dev
if("$1" != "") set jotExe=$1
echo "Testing $jotExe"

echo "Test  1  test.jot - The jot-command regression test"
${jotExe} t.t -st=test.jot -quiet -in="(v/Successfully completed all tests/ %a0=Successfully completed all tests;, %a1=Fail;)"
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 1"
endif
 
echo "Test  2  Streaming out to stdout"
#set result=`${jotExe} t.t -tty -quiet -init="p0 %a" | \grep -c '1'`
set result=`${jotExe} t.t -st -tty -quiet -init="p0 %a" | \grep -c '1'`
if("$result" != "3") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 2"
  endif
 
echo "Test  3  Accepting input from stdin stream - in -tty mode."
\rm -f test.txt
#${jotExe} < t.t -quiet -tty -init='%o=test.txt; %a'
${jotExe} < t.t -st -quiet -tty -init='%o=test.txt; %a'
\diff test.txt t.t
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 3"
endif
  
echo "Test  3a Accepting input from stdin stream - in non-tty mode."
\rm -f test.txt
#${jotExe} < t.t -quiet -init='%o=test.txt; %a'
${jotExe} < t.t -st -quiet -init='%o=test.txt; %a'
\diff test.txt t.t
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 3a"
endif
  
\rm -f test4.txt
echo "Test  4  Accepting input from stream and piping to stdout - in -tty mode."
#cat t.t | ${jotExe} -quiet -tty -init='p10 %a' > test4.txt
cat t.t | ${jotExe} -st -quiet -tty -init='p10 %a' > test4.txt
\diff test4.txt t.t
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 4"
endif

echo "Test  5  Read from command."
\rm -f test.txt
#${jotExe} t.t -tty -quiet -in="%eq=cat l99.t; %o=test.txt; %a"
${jotExe} t.t -st -tty -quiet -in="%eq=cat l99.t; %o=test.txt; %a"
\diff test.txt l99.t
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 5"
endif

echo "Test  5a Read from command - in non-tty mode."
\rm -f test.txt
#${jotExe} t.t -quiet -in="%eq=cat l99.t; %o=test.txt; %a"
${jotExe} t.t -st -quiet -in="%eq=cat l99.t; %o=test.txt; %a"
\diff test.txt l99.t
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 5a"
endif

echo "Test  6 Command-line filter command tests."
#
#${jotExe} ${JOT_RESOURCES}/l99.t \
${jotExe} ${JOT_RESOURCES}/l99.t -st \
  -filter=f1/9:/,k -init="v/__09: /m v/__19: /m v/__29: /m v/__39: /m v/__49: /m v/__59: /m v/__69: /m v/__79: /m v/__89: /m v/__99: /mm\ %a0=Pass;, %a1=Fail;" 
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 6"
endif

echo "Test  7  streamed-in commands."
\rm -f test7.txt
#${jotExe} t.t -to=test7.txt -tty -quiet -obey << EndOfTest7Commands
${jotExe} t.t -st -to=test7.txt -st -tty -quiet -obey << EndOfTest7Commands
m-0 (i/zzz /m2m-)0
%c
EndOfTest7Commands
 
\diff test7.txt - << EndOfRef
zzz 1 :abcdefghijklmnopqrstuvwxyz
zzz 2 :ABCDEFGHIJKLMNOPQRSTUVWXYZ
zzz 3 :1234567890
zzz 4 :Test file for jonathans wonderful editor.
zzz 5 :aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
zzz 6 :bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
zzz 7 :qwertyuiop[]
zzz 8 :asdgfghjkl;'
zzz 9 :zxcvbnm,./
zzz 10:<>\|~!@#\$%^&*()_+-=
EndOfRef
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 7"
endif

echo "Test  7a streamed-in commands - in non-tty mode."
\rm -f test7a.txt
#${jotExe} t.t -to=test7a.txt -quiet -obey << EndOfTest7aCommands
${jotExe} t.t -st -to=test7a.txt -quiet -obey << EndOfTest7aCommands
m-0 (i/zzz /m2m-)0
%c
EndOfTest7aCommands
 
\diff test7a.txt - << EndOfRef
zzz 1 :abcdefghijklmnopqrstuvwxyz
zzz 2 :ABCDEFGHIJKLMNOPQRSTUVWXYZ
zzz 3 :1234567890
zzz 4 :Test file for jonathans wonderful editor.
zzz 5 :aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
zzz 6 :bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
zzz 7 :qwertyuiop[]
zzz 8 :asdgfghjkl;'
zzz 9 :zxcvbnm,./
zzz 10:<>\|~!@#\$%^&*()_+-=
EndOfRef
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 7a"
endif

echo "Test  8  journal-keeping and recovery test."
#
#First - launch an edit session, do a few little edits then kill the session.
#The test of %E and %I are important because these use a complicated bit of wriggling to read filenames from the correct stream.
\rm -f test8.ref test8.txt
\rm -Rf test8.txt.jnl
\cp t.t test8.txt
date > test8_date.txt
chmod u+w test8.txt
#${jotExe} test8.txt -tty -journal -obey << EndOfTest8Part1Commands
#${jotExe} test8.txt -journal -obey << EndOfTest8Part1Commands
${jotExe} test8.txt -st -journal -obey << EndOfTest8Part1Commands
%%A few simple edits.
m-0 (i/zzz /r0i/ zzz/m2m-)0
m-0f/jon/ p
%ea=ls; z.ha
z.mha
%ib=test8.txt;
z.mhb
%o=test8.txt
%%Get PID and kill this session.
%q@=pid; k i/%e=\kill / p '@
EndOfTest8Part1Commands
\stty sane
  
#Rename the original result for later comparison with the recovered version.
\cp -p test8.txt test8.ref
#Nobble the killer line of history.
jot test8.txt.jnl/history.txt -quiet -tty -in="f/\kill/r-0i/%%/ %c" -startup
  
#Now recover the session and output another version of the result. 
${jotExe} test8.txt -init="%r=journal_recover; z. %o=test8.txt; %a"
#Run debugger with this (after running journal_recover.jot to create a ./recover_now.jot):
#  gdb -ex="set confirm off" --args ./jot_dev x.lis -st -in="%r=./recover_now.jot -asConsole"
#  gdb -ex="set confirm off" --args ./jot_dev test8.txt -st -in="%r=./recover_now.jot -asConsole ;"
#  gdb -ex="set confirm off" --args ./jot_dev test8.txt -init="%r=journal_recover; z. %o=test8.txt; %a"
  
#Finally - compare reference and recovered versions.
\diff test8.ref test8.txt
  
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 8"
endif

echo "Test  9  -asConsole."
#
#Set up the command script.
echo "%%Test 9 command script." > test9.jot
echo "%d^=Xd0     m-0f/jon/i/##/" >> test9.jot
echo " " >> test9.jot
echo "%m=******************************************** Test9;" >> test9.jot
echo "%m=Test -asConsole is working correctly" >> test9.jot
echo "z." >> test9.jot
echo "Congratulations Xd0     " >> test9.jot
#echo 'l0v/4 :Test file for Congratulations jonathans wonderful editor./ %a0=-asConsole seems OK"' >> test9.jot
echo 'r-0v/4 :Test file for Congratulations jonathans wonderful editor./ %a0=-asConsole seems OK"' >> test9.jot
 
${jotExe} ${JOT_RESOURCES}/t.t -st -init="%w 15; %w 0; %r=./test9.jot -asConsole;"
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 9"
endif

echo "Test 10  get.jot."
#Use the get.jot self-test routine.
${jotExe} ${JOT_RESOURCES}/test_get/hello.c -init="%r=get; %h'=call GetSelfTest;"
if("$status" != "0") then
  stty sane
  echo "**** Fail ****"
  set failedTests="${failedTests} 10"
endif

echo "Test 11 type-to-screen mode tests."
#
#Set up the command script.
\rm test11.jot
  
echo "f/jon/" > test11.jot
#Set up ^ key-translation buffer.
echo "%d^=X0      %s=commandmode +2" >> test11.jot
#Enter type-into-screen mode and insert "zzz".
echo "X0      .xyz" >> test11.jot
#In a new line, insert a string then rubout last three characters
echo "123999" >> test11.jot
#Return to command mode.
echo "abcdefghiX0       " >> test11.jot
#Verify.
echo "%g@" >> test11.jot
echo "%%Verification for test 11." >> test11.jot
echo "m-0" >> test11.jot
echo "( v/1 :abcdefghijklmnopqrstuvwxyz/" >> test11.jot
echo "  mv/2 :ABCDEFGHIJKLMNOPQRSTUVWXYZ/" >> test11.jot
echo "  mv/3 :1234567890/" >> test11.jot
echo "  mv/4 :Test file for xyz/" >> test11.jot
echo "  mv/123/" >> test11.jot
echo "  mv/abcdefghijonathans wonderful editor./" >> test11.jot
echo "  mv/5 :aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/" >> test11.jot
echo "  %a0=Type-into-screen mode seems OK; )" >> test11.jot
#echo "%a=Failed type-into-screen tests" >> test11.jot
echo ":" >> test11.jot
echo "'@" >> test11.jot
  
#${jotExe} ${JOT_RESOURCES}/t.t -st -init="%r=./test11.jot -asConsole; %a=Failed type-into-screen test"
${jotExe} ${JOT_RESOURCES}/t.t -st -init="%r=./test11.jot -asConsole; %a=Failed type-into-screen test"
if("$status" != "0") then
  echo "**** Fail ****"
  set failedTests="${failedTests} 11"
endif

stty sane
if("${failedTests}" == "") then
  echo
  echo "*****************************  Passed all linux tests  *****************************"
  echo
else
  echo
  echo "***The following tests failed: ${failedTests} ***" 
  echo
endif

