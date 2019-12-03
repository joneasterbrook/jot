#!/bin/sh
#
#Checks operation of deeply nested hashtables.

#jot_dev t.t -in="%w; %w 9; %w 0; \
#gdb -ex="set confirm off" --args jot_dev t.t -in="%w; %w 9; %w 0; \
 valgrind --leak-check=full --track-origins=yes --log-file=valg.log jot_dev t.t -in="
  %h=create 10; %h=data a; %q~=date; z.o#ov/a/z~ \
  %h=create 10; %h=data b; %q~=system; osz~os o#ov/b/z~ \
  %h=create 10; %h=data c; %q~=system; osz~os o#ov/c/z~ \
  %h=create 10; %h=data d; %q~=system; osz~os o#ov/d/z~ \
  %h=create 10; %h=data e; %q~=system; osz~os o#ov/e/z~ \
  %h=create 10; %h=data f; %q~=system; osz~os o#ov/f/z~ \
  %h=create 10; %h=data g; %q~=system; osz~os o#ov/g/z~ \
  %h=create 10; %h=data h; %q~=system; osz~os o#ov/h/z~ \
  %h=create 10; %h=data i; %q~=system; osz~os o#ov/i/z~ \
  %h=create 10; %h=data j; %q~=system; osz~os o#ov/j/z~ \
  %h=create 10; %h=data k; %q~=system; osz~os o#ov/k/z~ \
  %h=create 10; %h=data l; %d~=Final element; osz~os o#ov/l/z~ \
%qz=system; z. %qx=system; \
oq/.=a|b|c|d|e|f|g|h|i|j|k|l/z~ %b=writeifchanged; i/zzz/ %d~=A redefined version of sub-buffer e; \
zqn.a. %a

#%r=freeall; "
#ov/.=a|b|c|d|e/"
#oq/.=a|b|c|d|e|f|g|h|i|j|k|l/z~v-/zzz/v/Final sub-buffer L/ %b=unrestricted; \
#  z. t o@ oq/.|a|b|c|d|e|f|g|h|i|j|k|l/ "

