#Make file to build a jot executable.

WD = $(shell pwd)
NOX11_OPT = -DnoX11
DEBUG_OPT = -g -Wall
FILE_OFFSET_BITS = -D_FILE_OFFSET_BITS=64
VERSION_STRING = -DVERSION_STRING="\"jot_dev, built `\date +%d/%m/%y\ %H:%M:%S` `\uname -nspr` \""

ARCH = $(shell uname -p)
ifeq ($(ARCH), x86_64)
  OS = -DLINUX
  CURSES_LIB = -lncursesw -lncurses 
else
  OS = -DCHROME
  CURSES_LIB = /usr/lib/arm-linux-gnueabihf/libncursesw.so
  endif
  
lin \
jot_dev: ${JOT_HOME}/source/jot.c
	echo "VERSION_STRING = $(VERSION_STRING)"
	gcc $(OS) $(NOX11_OPT) $(DEBUG_OPT) $(FILE_OFFSET_BITS) $(VERSION_STRING) ${JOT_HOME}/source/jot.c $(CURSES_LIB) -o jot_dev

win \
jot_dev.exe: ${JOT_HOME}/source/jot.c
	echo "VERSION_STRING = $(VERSION_STRING)"
	wineconsole $(WD)/compile.bat "\"$(VERSION_STRING)\""
