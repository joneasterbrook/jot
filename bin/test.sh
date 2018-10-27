#! /bin/sh
cd ${JOT_HOME}
  
#Tests demonstrating various modes of operation for jot
#Examples:
#   $ ./test.csh                - runs tests using jot_dev - the default name for the jot executable.
#   $ ./test.csh <exePathName>  - runs tests using ${exeName}
#History
#16/09/12 Created using test.ecc
  
failedTests=""
jotExe=jot_dev
if [ "$1" != "" ]; then
  jotExe=$1
  fi
echo "Testing $jotExe"

echo "Test  1  test.jot - The jot-command regression test"
${jotExe} ${JOT_RESOURCES}/t.t -quiet -in="%r=test; ((v/Successfully completed all basic tests/, v/Pass/) %a0=Successfully completed all basic tests;, %a1=Fail;)"
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 1"
  fi
 
echo "Test  2  Streaming out to stdout"
result=`${jotExe} ${JOT_RESOURCES}/t.t -st -tty -quiet -init="p0 %a" | \grep -c '1'`
if [ "$result" != "3" ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 2"
  fi
 
echo "Test  3  Accepting input from stdin stream - in -tty mode."
\rm -f test.txt
#${jotExe} < ${JOT_RESOURCES}/t.t -quiet -tty -init='%o=test.txt; %a'
${jotExe} < ${JOT_RESOURCES}/t.t -st -quiet -tty -init='%o=test.txt; %a'
\diff test.txt ${JOT_RESOURCES}/t.t
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 3"
  fi
  
echo "Test  3a Accepting input from stdin stream - in non-tty mode."
\rm -f test.txt
#${jotExe} < ${JOT_RESOURCES}/t.t -quiet -init='%o=test.txt; %a'
${jotExe} < ${JOT_RESOURCES}/t.t -st -quiet -init='%o=test.txt; %a'
\diff test.txt ${JOT_RESOURCES}/t.t
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 3a"
  fi
  
\rm -f test4.txt
echo "Test  4  Accepting input from stream and piping to stdout - in -tty mode."
#cat ${JOT_RESOURCES}/t.t | ${jotExe} -quiet -tty -init='p10 %a' > test4.txt
cat ${JOT_RESOURCES}/t.t | ${jotExe} -st -quiet -tty -init="p10 %a" > test4.txt
\diff test4.txt ${JOT_RESOURCES}/t.t
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 4"
  fi

echo "Test  5  Read from command."
\rm -f test.txt
#${jotExe} ${JOT_RESOURCES}/t.t -tty -quiet -in="%eq=cat ${JOT_RESOURCES}/l99.t; %o=test.txt; %a"
${jotExe} ${JOT_RESOURCES}/t.t -st -tty -quiet -in="%eq=cat ${JOT_RESOURCES}/l99.t; %o=test.txt; %a"
\diff test.txt ${JOT_RESOURCES}/l99.t
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 5"
  fi

echo "Test  6 Read from command - in non-tty mode."
\rm -f test.txt
#${jotExe} ${JOT_RESOURCES}/t.t -quiet -in="%eq=cat ${JOT_RESOURCES}/l99.t; %o=test.txt; %a"
${jotExe} ${JOT_RESOURCES}/t.t -st -quiet -in="%eq=cat ${JOT_RESOURCES}/l99.t; %o=test.txt; %a"
\diff test.txt ${JOT_RESOURCES}/l99.t
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 6"
  fi

echo "Test  7  streamed-in commands."
\rm -f test7.txt
${jotExe} ${JOT_RESOURCES}/t.t -st -to=test7.txt -tty -quiet -obey << EndOfTest7Commands
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
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 7"
  fi

echo "Test  7a streamed-in commands - in non-tty mode."
\rm -f test7a.txt
${jotExe} ${JOT_RESOURCES}/t.t -st -to=test7a.txt -quiet -obey << EndOfTest7aCommands
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
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 7a"
  fi

echo Test  8: piping stdout in -tty mode.
${jotExe} ${JOT_RESOURCES}/t.t -tty -quiet -init="f/1234567890/p %a" | grep "3 :1234567890" > /dev/null
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 8"
  fi

echo "Test  9  journal-keeping and recovery test."
#
#First - launch an edit session, do a few little edits then kill the session.
#The test of %E and %I are important because these use a complicated bit of wriggling to read filenames from the correct stream.
\rm -f test9.ref test9.txt
\rm -Rf test9.txt.jnl
\cp ${JOT_RESOURCES}/t.t test9.txt
chmod u+w test9.txt
${jotExe} test9.txt -journal -obey << EndOfTest9Part1Commands
%%A few simple edits.
m-0 (i/zzz /r0i/ zzz/m2m-)0
m-0f/jon/ p
%ea=ls -l *.t; z.ha
z.mha
%ib=test9.txt;
z.mhb
%o=test9.txt
%%Get PID and kill this session.
%q@=pid; k i/%e=\kill / '@
EndOfTest9Part1Commands
\stty sane
  
#Rename the original result for later comparison with the recovered version.
\cp -p test9.txt test9.ref
#Nobble the killer line of history.
jot test9.txt.jnl/history.txt -quiet -tty -in="f/\kill/r-0i/%%/ %c" -startup
  
#Now recover the session and output another version of the result. 
${jotExe} test9.txt -st=recover -in="z. %o=test9.txt; %a"
#Run debugger with this (after running recover.jot to create a ./recover_now.jot):
#  gdb -ex="confirm off" --args ./jot_dev x.lis -st -in="%r=./recover_now.jot -asConsole"
#  gdb -ex="confirm off" --args ./jot_dev test9.txt -st -in="%r=./recover_now.jot -asConsole ;"
#  gdb -ex="confirm off" --args ./jot_dev test9.txt -init="%r=recover; z. %o=test9.txt; %a"
  
#Finally - compare reference and recovered versions.
\diff test9.ref test9.txt
  
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 9"
  fi

echo "Test  10  -asConsole."
#
#Set up the command script.
echo "%%Test 10 command script." > test10.jot
echo "%d^=^[Xd0       m-0f/jon/i/##/" >> test10.jot
echo " " >> test10.jot
echo "%m=******************************************** Test10;" >> test10.jot
echo "%m=Test -asConsole is working correctly" >> test10.jot
echo "z." >> test10.jot
echo "Congratulations Xd0     " >> test10.jot
echo "%c;" >> test10.jot
 
${jotExe} ${JOT_RESOURCES}/t.t -to=test10.txt -st -init="%w 15; %w 0; %r=./test10.jot -asConsole;"
\grep "4 :Test file for Congratulations jonathans wonderful editor." test10.txt > /dev/null
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 10"
  fi

echo "Test 11  get.jot."
#Use the get.jot self-test routine.
${jotExe} ${JOT_RESOURCES}/test_get/hello.c -init="%r=get; %h'=call GetSelfTest;"
if [ $? -ne 0 ]; then
  stty sane
  echo "**** Fail ****"
  failedTests="${failedTests} 11"
  fi

echo "Test 12 type-to-screen mode tests."
#
#Set up the command script.
\rm test12.jot
  
echo "f/jon/" > test12.jot
#Set up ^ key-translation buffer.
echo "%g^" >> test12.jot
echo "X0      %s=commandmode +2" >> test12.jot
echo "ins     i/##/" >> test12.jot
echo ":" >> test12.jot
#Enter type-into-screen mode and insert "zzz".
echo "X0      .xyz" >> test12.jot
#In a new line, insert a string then rubout last three characters **** NB in linuxland rubout is ^? not ^[ (windows)
echo "123999" >> test12.jot
#Return to command mode.
echo "abcdefghiX0       " >> test12.jot
#Verify.
echo "%g@" >> test12.jot
echo "%%Verification for test 12." >> test12.jot
echo "m-0" >> test12.jot
echo "( v/1 :abcdefghijklmnopqrstuvwxyz/" >> test12.jot
echo "  mv/2 :ABCDEFGHIJKLMNOPQRSTUVWXYZ/" >> test12.jot
echo "  mv/3 :1234567890/" >> test12.jot
echo "  mv/4 :Test file for xyz/" >> test12.jot
echo "  mv/123/r0v-//" >> test12.jot
echo "  mv/abcdefghijonathans wonderful editor./" >> test12.jot
echo "  mv/5 :aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/" >> test12.jot
echo "  n.aqzqi/Type-into-screen mode seems OK;/b %o=test12.rpt; %a0; )" >> test12.jot
#echo "  %a0=Type-into-screen mode seems OK; )" >> test12.jot
echo ":" >> test12.jot
echo "'@" >> test12.jot

echo "Test 13  recover.jot"
#
#  Tests the recover.jot script. The  interaction  between  debugger,  {Ctrl+c}
#  interrupt and G command are explored  here.  These  tests  are  important  -
#  seemingly innocuous tweaks can cause recover.jot to  fail.  Then  the  first
#  anyone knows about this failure mode is when they're trying to recover  from
#  a crash - not an ideal time to be debugging this kind of code.
#
rm -rf test13.txt.jnl;
rm -f test13.txt;
touch test13.txt
#Create a history-file stub.
${jotExe} test13.txt -journal -in="%q@=pid; k i/%e=\kill / '@"
#Remove the kill line.
jot test13.txt.jnl/history.txt -in="f/<<Init commands follow>>/mi/%%/ %c"
#
#---------------------------History setup.
cat - >> test13.txt.jnl/history.txt << EndOfHistory

%w; %w 15; %w 0;
%%Clear the failure log.
%%
%%To enter the debugger in this file, insert the line "%s=trace B001;".

%%------------------------------------------------------------------------------------------
%m=Test 13, part 1: A {Ctrl+c} interrupt of a loop - the original command was separated from the <<INT ...>> line by an OP report.
zfi/13-1 /
%i.=test13.txt;
<<Recovery pathname: ${JOT_RESOURCES}/t.t to buffer . >>
op (r, m, m-0)0
0 0 0MOUSE
<<INT 1538530>> 2
i/[/r?i/]/r-0(v/1 :abcdefghijklmnopqrstuvwxy[z]/zfi/Pass  /, pzfi/Fail  /)z.

%%------------------------------------------------------------------------------------------
%m=Test13, part 2: The <<Debugger on> line is separated from the t command line by some other gumph (in this case an OP report) in the history file.
zfi/13-2 /
%i.=test13.txt;
<<Recovery pathname: ${JOT_RESOURCES}/t.t to buffer . >>
me      
me      
me      
opt
0 0 0MOUSE
<<Debugger on>> 2
me      





me      

<<INT 2>> 1
<<Debugger off>>
i/[/r?i/]/r-0(v/1 :ab[c]defghijklmnopqrstuvwxyz/zfi/Pass  /, pzfi/Fail  /)z.

%%------------------------------------------------------------------------------------------
%m=Test13, part 3: The interrupt is delivered mid-debug session.
zfi/13-3 /
%i.=test13.txt;
<<Recovery pathname: ${JOT_RESOURCES}/t.t to buffer . >>
opt
0 0 0MOUSE
<<Debugger on>> 2
(r, m, m-0)0










<<INT 11>> 2
<<Debugger off>>
i/[/r?i/]/r-0(v/1 :abcdefg[h]ijklmnopqrstuvwxyz/zfi/Pass  /, pzfi/Fail  /)z.


%%------------------------------------------------------------------------------------------
%m=Test13, part 4: The interrupt terminates a G command.
zfi/13-4 /
%i.=test13.txt;
<<Recovery pathname: ${JOT_RESOURCES}/t.t to buffer . >>
m4
g0
abc
def
<<INT 1>> 3
ghi
<<G-command terminated>>
i/[/r?i/]/m(v/5 :aaaaaaaaaaaaaa/ m-v/[g]hi/ m-v/def/ m-v/abc/ m-v/4 :Test file/zfi/Pass  /, pzfi/Fail  /)z. 

%%------------------------------------------------------------------------------------------
%m=Test13, part 5: The interrupt terminates a G command in a blank line.
zfi/13-5 /
%i.=test13.txt;
<<Recovery pathname: ${JOT_RESOURCES}/t.t to buffer . >>
m4
g0
abc
def
<<INT 1>> 3

<<G-command terminated>>
i/[/r?i/]/m(v/5 :aaaaaaaaaaaaaa/ m-v/[]/ m-v/def/ m-v/abc/ m-v/4 :Test file/zfi/Pass  /, pzfi/Fail  /)z. 

%%------------------------------------------------------------------------------------------
%m=Test13, part 6: The debugger session contains typed-in commands.
zfi/13-6 /
%i.=test13.txt;
<<Recovery pathname: ${JOT_RESOURCES}/t.t to buffer . >>
t
<<Debugger on>> 1
me      

y

me      


me      


q       
q       
<<Debugger off>>
i/[/r?i/]/r-0(v/2 :[A]BCDEFGHIJKLMNOPQRSTUVWXYZ/zfi/Pass  /, pzfi/Fail  /)z.

EndOfHistory
#---------------------------History setup ends.
# 
#Go
${jotExe} test13.txt -st=recover -in="zfr-0(f/Fail/\ %a0=Pass;, %a1=Fail: 'f;)"
if [ $? -ne 0 ]; then
  echo "**** Fail ****"
  failedTests="${failedTests} 13"
  fi

stty sane
if [ "${failedTests}" = "" ]; then
  echo
  echo "=====================  The linux version passed all tests  ====================="
  echo
else
  echo
  echo "===The linux version failed the following tests: ${failedTests} ===" 
  echo
  fi

