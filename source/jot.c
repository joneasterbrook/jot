//This file contains all of the source code for jot (Joy Of Text - a text editor).
//
//
//    Copyright (C) 2013-2018 Jon. Easterbrook
//
//    This file is part of jot (Joy Of Text - a text editor).
//
//    jot is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    jot is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with jot.  If not, see <https://www.gnu.org/licenses/>.
//
//

#define _GNU_SOURCE
#define GNU_SOURCE
#if defined(CHROME)
#define LINUX
#endif
#if defined(PI)
#define LINUX
#endif

#if defined(VC)
#define _UNICODE
#define nolstat
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <malloc.h>
#include <windows.h> 
#include <tchar.h>
#include <strsafe.h>
#include <locale.h>
#include <direct.h>
#include <uchar.h>
#include <io.h>
#include <WinBase.h>
#include <glibc/search.h>
#include <setjmp.h>
//
//Dreadful tack. This is defined in unistd.h and not in winsock.h, but winsock.h is incompatible with unistd.h ... I give up.
#define W_OK 2
#define off64_t long
 
#elif defined(LINUX)
#if defined(CHROME)
#include <ncursesw/curses.h>
#elif defined(CYGWIN)
#include <ncurses6/curses.h>
#elif defined(PI)
#include <ncursesw/curses.h>
#else
#include <ncurses/curses.h>
#endif
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <dirent.h>
#include <search.h>
#include <unistd.h>
#include <uchar.h>
#include <wchar.h>
#include <locale.h>
#include <ctype.h>
#include <setjmp.h>
#include <termios.h>
#include <grp.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
 
#elif defined(ANDROID)
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <wchar.h>
#include <ctype.h>
#include <locale.h>
#endif 

#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/timeb.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
 
#if !defined(NoRegEx)
#include <regex.h>
#endif
 
#if defined(VC)
#define KBD_NAME "CON"
#define NULL_STREAM "NUL"
#define RubOut     0x08
#define Shift_key  0x10
#define Ctrl_key   0x11
#define Alt_key    0x12
 
#elif defined(LINUX)
#define KBD_NAME "/dev/tty"
#define NULL_STREAM "/dev/null"
#define RubOut 0x7F

#elif defined(ANDROID)
static int KBD_Descriptor = -1;
#define KBD_NAME "/dev/tty"
#define NULL_STREAM "/dev/null"
#define RubOut 0x7F
#endif
  
//GetBuffer() CreateOrDelete arg.
#define AlwaysNew 0      //If there was a preexisting buffer GetBuffer will check for edit locks then clear out the old buffer.
#define OptionallyNew 1  //If there is a preexisting buffer GetBuffer will return it as is.
#define NeverNew 2       //If there is no preexisting buffer GetBuffer will return NULL.
 
//Command-mode bits:
#define CommandMode_ToggleScreen      0x00000001      //Reverts to command mode temporarily for each new hotkey.
#define CommandMode_ScreenMode        0x00000002      //Currently in screen mode.
#define CommandMode_OverTypeMode      0x00000004      //Typing into screen overwrites preexisting text.
#define CommandMode_EscapeAll         0x00000008      //Any character can initiate an escape string.
#define CommandMode_Pass_Ctrl_C       0x00000010      //{Ctrl+C} paases through like any other character.

#if !defined(VERSION_STRING)
#define VERSION_STRING "Test version"
#endif
#define TRUE 1
#define FALSE 0
#if defined(VC)
#define MB_CUR_MAX 8
#endif
#define StringMaxChr 1024         //The largest string that can be accommodated by most static and local strings.
#define BucketSize 1000           //Size of temporary buffer used by ReadUnbufferedStreamRec and ReadNewRecord.
//N.B. For safe handling of UTF-32 BucketSize *must* be divisible by 4
#define StackSize 100             //The default stack maximum.
#define MaxAutoTabStops 1000      //The maximum no. of tabstops that may be automatically assigned.
#define EscapeSequenceLength 12   //Maximum no. of characters allowed in an escape sequence.
#define ASCII_Escape 0x1B  
#define Control_D 'D'-64  
#define Control_U 'U'-64  
#define UNTRANSLATED_CHR 1        //Returned by TransEscapeSequence() when a control character is not recognized
                                  //behaviour now is to assume it's unicode and enter it into the command string.
#define CTRL_C_INTERRUPT 2        //Returned by TransEscapeSequence() when a Ctrl+C interrupt is received.
  
#define FirstRecord_Flag 0X1      //This record is the first received from the input stream.
#define LastRecord_Flag  0X2      //This record was followed by an EOF from the input stream.

//Trace-vector settings (s_TraceMode)
#define Trace_AllCommands  0x0001 //Traces all commands.
#define Trace_CommandLines 0x0002 //Traces at start of new command line.
#define Trace_Functions    0x0004 //Traces at start of each function call.
#define Trace_SubSequence  0x0008 //Traces at start of new subsequence (new block or else clause).
#define Trace_Failures     0x0010 //Traces all failing commands.
#define Trace_FailNoElse   0x0020 //Traces failing commands with no local else sequence.
#define Trace_FailMacros   0x0040 //Traces on exit from failed macro call.
#define Trace_Interrupt    0x0080 //Traces command following a Ctrl+C interrupt.
#define Trace_Recovery     0x0100 //Allows stepping through recovery files.
#define Trace_CmdCount     0x0400 //Reports command counter at each trace point..
#define Trace_Backtrace    0x0800 //Backtraces to console.
#define Trace_Stack        0x1000 //Dumps stack at each trace event.
#define Trace_Print        0x2000 //Prints current line at each trace event.
#define Trace_Source       0x4000 //Displays line of source code at each trace event.
#define Trace_Break        0x8000 //Break before command triggering a trace event.
  
#define PathDelimChr '/'          //Path-element delimiter.
#define ExtnDelimChr '.'          //file-name extention delimiter.

//Tag type codes.
#define TagType_Text      1       //This tag is a text string - Tag->Attr points to the string.
#define TagType_Target    2       //The Tag->Attr points to a JumpObj.
#define TagType_Colour    3       //The text is to be displayed in the forground/background - Tag->Attr points to the text string.
#define TagType_Font      4       //When output to a relevant file type, the text is to be written in the font specified in the FontTag - Tag->Attr points to FontTag.
  
//Values for AdjustMode in FreeRec() - determines where any adjustable target points end up.
#define DeleteNotAdjust 0         //As it's name suggests - the TargetTag gets deleted.
#define AdjustForwards 1          //Any adjustable target tag gets shifted to the next record except for last record.
#define AdjustBack 2              //Any adjustable target tag gets shifted to the previous record except for first record.
  
//Tags are held in a StartPoint-ordered chain belonging to the record.
//For ColourTags, FontTags and TextTags, the Attr element points to one of the tag objects below,
//for target tags Attr points to the hash-table object.
struct AnyTag {                   //A member of a records tag chain - attributes (readonly, for/background colour and font attributes).
  struct AnyTag *next;            //The next member in the ordered list of tags on this Rec.
  unsigned char type;             //The tag type see above for type codes.
  int StartPoint;                 //The first (or only, in the case of target-types) character that this tag applies to.
  void *Attr;                     //Points to attribute descriptor, where applicible - a target point of a hash-table, a Font or a text string.
  int EndPoint;                   //The end point for colour, text or font tags.
  };
  
//Target tags are endpoints for JumpObj hashtable objects.
struct TargetTag {                //A member of a records tag chain - attributes (readonly, for/background colour and font attributes).
  struct Tag *next;               //The next member in the ordered list of tags on this Rec.
  unsigned char type;             //The tag type see above for type codes.
  int StartPoint;                 //The first (or only, in the case of target-types) character that this tag applies to.
  void *Attr;                     //Points to target point of a hash-table.
  };
  
//ColourObjs are stored in a chain belonging to the parent buffer the Attr pointer of a Record tag points to the relevant master ColourTag. 
//These are read by JotUpdateWindow - this structure optimizes screen updates.
struct ColourObj {                //This object defines the forground, background and selected-substring colours.
  struct ColourObj *next;         //The next colour object in the chain.
  unsigned char type;             //The tag type see above for type codes.
  int StartPoint;                 //The first (or only, in the case of target-types) character that this tag applies to.
  int ColourPair;                 //In linux, the curses colour-pair id, in windows a colour pair.
  short Foreground;               //The foreground colour.
  short Background;               //The background colour.
  char TagName[12];               //The name of the colour pair
  };
  
//If ever FontTags get properly implemented the actual structure may be more like the ColourTags - e.g. no EndPoint but a strt tag and a stop tag.
struct FontTag {                  //This tag defines font name and size for the tagged text.
  int FontSize;                   //The font size (Pt).
  unsigned char type;             //The tag type see above for type codes.
  char *FontName;                 //The name of the font.
  };
  
//Valid object types:
#define HTabObj_Jump        0     //Defines a jump object.
#define HTabObj_Data        1     //Defines a pointer to a data-object frame (see stack-frame-type.
#define HTabObj_Setsect     2     //Defines section-start and bytecount in an already-opened file. 
#define HTabObj_Setfilesect 3     //Defines file pathname as well as section-start and bytecount.
#define HTabObj_Code        4     //Defines a entry point for a macro or a h_Call.
#define HTabObj_Zombie      5     //hashtable-object has been deleted.
  
//The following objects are referenced by hashtables.
struct AnyHTabObj {               //Object is one of the following: JumpObj, DataObj, SetsectObj, SetfsectObj or ZombiObj (a deleted object).
  unsigned char type;             //The object type see above for type codes.
  struct AnyHTabObj *next;        //Points to next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  };

struct ZombieHTabObj {            //Object was deleted but there's still an entry in the hashtable.
  unsigned char type;             //Set to HTabObj_Zombie, see above for type codes.
  struct AnyHTabObj *next;        //Points to next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  struct AnyHTabObj *NewObj;      //The replacement object (if one is defined).
  short int WasSetFSectHTabObj;   //The original object was a Setfsect Obj - there may be a pathname to be freed.
  };

struct JumpHTabObj {              //Htab entry points to a jump object - points to a record, character and linenumber etc.
  unsigned char type;             //Set to HTabObj_Jump, see above for type codes.
  struct AnyHTabObj *next;        //Points to next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  struct Rec *TargetRec;          //The jump destination record, if NULL the object is deemed to have been deleted.
  int LineNumber;                 //The jump destination line no. - only valid when buffer is unchanged since JumpHTabObj was created.
  struct Buf *TargetBuf;          //The jump destination buffer.
  struct Buf *HashBuf;            //The hash-table buffer key.
  struct TargetTag *Target;       //The target tag for the jump.
  };

struct CodeHTabObj {              //Htab entry points to a code object - points to a record - code starts at the first character in record.
  unsigned char type;             //Set to HTabObj_Code, see above for type codes.
  struct AnyHTabObj *next;        //Points to next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  struct CodeHeader *CodeHeader;  //The function entry point object, if NULL the object is deemed to have been deleted.
  };

struct DataHTabObj {              //Htab entry points to a seek/bytes to fetch a selected part of a file.
  unsigned char type;             //Set to HTabObj_Data, see above for type codes.
  struct AnyHTabObj *next;        //The next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  struct intFrame *Frame;         //Generic stack-frame object.
  struct Buf *ParentBuf;          //Points back to parent buffer, used to determine buffer-path.
  };

struct SetsectHTabObj {           //Htab entry points to a seek/bytes to fetch a selected part of a file.
  unsigned char type;             //Set to HTabObj_Setsect, see above for type codes.
  struct AnyHTabObj *next;        //The next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  long long Seek;                 //Byte offset from start of file to selected section.
  long long Bytes;                //Byte count of selected section of the file.
  };

struct SetfsectHTabObj {          //Htab entry points to a seek/bytes to fetch a selected part of a file.
  unsigned char type;             //Set to HTabObj_Setfsect, see above for type codes.
  struct AnyHTabObj *next;        //The next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  long long Seek;                 //Byte offset from start of file to selected section.
  long long Bytes;                //Byte count of selected section of the file.
  char *PathName;                 //A string holding the pathname of the file.
  };

//Values for DisplayFlag in Rec struct.
#define Redraw_None 0             //This line is accurately represented on the screen already.
#define Remove_Cursor 1           //This line previously contained the cursor/current substring and these characters need redrawing.
#define Redraw_Line 2             //This line has been redefined and needs completely rewriting.

struct Rec {                      //A record belonging to a Buf's record ring.
  struct Rec *prev;               //The previous Rec in this buffer's record chain - n.b. the chain is circular, FirstRec->prev points to the last Rec in the chain.
  struct Rec *next;               //The next Rec in this buffer's record chain.
  int MaxLength;                  //The maximum allowable length for this record (the original malloc allocation).
  struct AnyTag *TagChain;        //Chain of Tag elements in text order.
  char *text;                     //The text string for this record.
  char DisplayFlag;               //Indicates to JotUpdateWindow that this line needs redrawing.
  };

//Edit permissions (for the EditLock field):
//#define Unrestricted 0            //Like it says - no restrictions.
#define ReadOnly 1                //No changes to this buffer allowed.
#define WriteIfChanged 2          //If changed then ensure these are written before exiting the editor session.

//Modification status bits (for the UnchangedStatus field) - the UnchangedStatus byte is set to 0X00 every time there is a change to the buffer. 
#define SameSinceIO       0x01    //Set to 1 when originally read (or defined by %Q, %E etc.) set again by a write.
#define SameSinceIndexed  0x02    //Set to 1 After a hash-table entry is made.
#define SameSinceCompiled 0x04    //Set to 1 When macro is compiled.
#define SameFlag1         0x08    //Set by user %q[<key>]=UserFlag1 [0|1]
#define MacroRun          0x10    //Set to 1 When macro exits - used to detect redefinition of a macro buffer.
                                  //This, combined with SameSinceCompiled, detects the situation when some child frame redefines then recompiles the macro text.
 
//FileType tag values, any positive values indicate binary-mode record length.
#define ASCII 0
    
//Buf->HashtableMode has these possible states:
#define NoHashtable 0             //No hashtable in this buffer.
#define ProtectEntries 1          //May change or delete any text except the hash-table target strings.
#define AdjustEntries 2           //If a hash-table target is removed, the entry is silently adjusted to point to somewhere nearby.
#define DeleteEntries 3           //If a hash-table target is removed, the entry is silently removed from the hashtable.
#define DestroyHashtables 4       //If a hash-table target is removed, the whole hashtable is silently destroyed.
  
//UCEncoding values:
#define UCEncoding_Plain 1
#define UCEncoding_UTF_8 2
#define UCEncoding_UTF_16LE 3
#define UCEncoding_UTF_16BE 4
#define UCEncoding_UTF_32LE 5
#define UCEncoding_UTF_32BE 6
#define UCEncoding_LE 1           //Distinguish big-endian encoding from little-endien.
  
struct Buf {
  struct Rec *FirstRec;           //The first Rec in the buffer.
  struct Rec *CurrentRec;         //The Rec that contains the current character.
  int CurrentByte;                //The byte offset to the start of the current character.
  int SubstringLength;            //The length of the currently-selected substring.
  int LineNumber;                 //The line number of the current Rec in this buffer.
  int OldFirstLineNo;             //The line number of the first line in the open window (i.e. the window that follows the current buffer).
  struct Buf *NextBuf;            //The next buffer in the s_BufferChain.
  int *TabStops;                  //Tabstop array - TabStops[0] = no. of tabstops.
  int LeftOffset;                 //View left offset when displaying long lines, normally 0, any nagative value means auto LeftOffset.
  struct Com *Predecessor;        //The buffer was called as a macro by Predecessor (or NULL).
  char *PathName;                 //The original pathName.
  char *Header;                   //Header to be displayed above window.
  char *Footer;                   //Footer to be displayed in window-separator line instead of file name.
  struct DataHTabObj *ParentObj;  //Points to object in parent-buffers hashtable.
  struct ColourObj *FirstColObj;  //A list of colour-pair objects associated with this buffer.
  struct AnyHTabObj *FirstObj;    //Chain of Htab/Seek/DataHTabObj structs.
  struct hsearch_data *htab;      //Hash table associated with this buffer.
  struct IOBlock *IOBlock;        //Holds I/O/data while reading, also interactive I/O data for interactive I/O (%e=&... operations pending).
  struct CodeHeader *CodeChain;   //Points to start of code-header chain for this bufffer.
#if defined(VC)
  int CodePage;                   //Code page to be used for this buffer.
#endif
  unsigned char FrameCount;       //Used by Backtrace() to detect the redefinition of a macro-buffer.
  char AbstractWholeRecordFlag;   //When inserting this text  (i.e. Here command) into some other buffer, the inserted text goes at the start of the current word.
  char UnchangedStatus;           //If zero, the buffer has not been changed since it was read/indexed/compiled.
  char EditLock;                  //The requested edit permission for buffer. One of Unrestricted (the default), WriteIfChanged and ReadOnly.
  char HashtableMode;             //Defines behaviour when targets of the hashtable are to be deleted (see above for values).
  char NewPathName;               //When TRUE, the PathName has been changed - triggers an update in the display.
  char NoUnicode;                 //When TRUE, turns off unicode support.
  char FileType;                  //File type identified from FileType or defined explicitly to ReadUnbufferedChan()
  char AutoTabStops;              //Assign tabstops automatically, when TRUE.
  char ExistsInData;              //When true the buffer is in a DataHTabObj (or a clone of one) and the buffer should not be deleted while the DataHTabObj exists.
  char BufferKey;                 //The key associated with this buffer.
  char TabCells;                  //The indicates that text between tabs must be treated as cells not simple tabular form.
  char UCEncoding;                //Defaults to Plain, set to other unicode types by BOM as the file is read or by %b=UCEncoding assignment.
  };

//Types of arguments in an Arg struct:
#define IntArgType 1              //The arg is an integer literal.
#define IntArgDeferredType 2      //An integer, value to be determined by evaluating an expression at execution time.
#define FloatArgType 3            //The arg is a FP literal.
#define StringArgType 4           //The arg is a string literal.
#define IndirectStringArgType 5   //The arg is an integer containing a buffer key indicating the required buffer.
#define BlockArgType 6            //The arg is a code block.

struct AnyArg   {                 //A generic arg.
  char           type;            //Must be one of these: IntArgType, FloatArgType, StringArgType, IndirectStringArgType or BlockArgType.
  struct AnyArg  *next;           //Points to next arg in chain.
  };

struct IntArg {                   //An integer arg fully specified in the source coding.
  char           type;            //Must be IntArgType
  struct AnyArg  *next;           //Points to next arg in chain.
  long long      IntArg;          //Integer value
  };

struct IntArgDeferred {           //An integer arg value references the stack or nominated buffer.
  char           type;            //Must be IntArgDeferredType
  struct AnyArg  *next;           //Points to next arg in chain.
  int            Radix;           //The conversion radix to be used.
  char *Expr;                     //The indirect-reference expression (N.B. *not* an arithmetic expression).
  };

struct StringArg     {            //An arg that points to a simple string.
  char           type;            //Must be one of these: StringArgType, IndirectStringArgType or BlockArgType.
  struct AnyArg *next;            //Points to next arg in chain.
  char *       String;            //Points to string
  int    StringLength;            //The length of the string.
  struct StringArg *Continuation; //Points to a continuation substring for implementation of ampersand syntax.
  };

struct PtrArg {                   //An arg that points to either a string or a block.
  char          type;             //Must be one of these: StringArgType, IndirectStringArgType or BlockArgType.
  struct AnyArg *next;            //Points to next arg in chain.
  void *        pointer;          //Points to string/data-struct arg or VOID.
  char          Key;              //Indicates a buffer key for some flavours of pointer.
  unsigned char Offset;           //The length of the command string - percent commands may be truncated
  };

struct FloatArg {                 //A floating-point arg.
  char           type;            //Must be FloatArgType.
  struct AnyArg  *next;           //Points to next arg in chain.
  double         FloatArg;        //Floating-point value
  };

struct Com {                      //Stores one command and pointer to argument list. 
  int          CommandKey;        //Identifies command type.
  struct AnyArg  *ArgList;        //Points to first arg in list.
  struct Com  *NextCommand;       //Points to next command in sequence.
  struct Buf  *CommandBuf;        //Points to buffer holding the source.
  struct Rec  *CommandRec;        //Points to record with command line.
  int          CommandByteNo;     //Byte no. of start of command in line.
  int          ComChrs;           //No. of characters in command source, including arguments.
  int          CommandLineNo;     //Line no. of command in macro.
  };

struct CodeHeader {               //Introduces a block of either source and compiled code.
  struct Rec *SourceRec;          //The first record of the function source.
  struct Seq *CompiledCode;       //If already compiled, points to compiled code sequence.
  struct CodeHeader *next;        //The next CodeHeader block in the chain.
  };
  
struct Seq {                      //Sequence of instructions with possible ELSE sequence. 
  struct Com  *FirstCommand;      //First in a sequence of commands.
  struct Seq  *ElseSequence;      //Sequence to be obeyed in event of failure.
  struct Seq  *NextSequence;      //next sequence to be obeyed.
  };

struct Block {                    //Allows implementation of repeated block.
  struct Seq  *BlockSequence;     //The sequence associated with this block.
  int          Repeats;           //Number of times to repeat this sequence.
  };

struct BacktraceFrame {           //These are chained to provide a simple backtrace diagnostic.
  struct BacktraceFrame  *prev;   //Points back to predecessor frame.
  struct Com  *LastCommand;       //The command triggering the context change.
  struct CommandFile *EditorInput;//The active command file  where appropriate.
  struct Rec *FirstRec;           //The first record of the function for a CallFrame - by convention contains it's name.
  char type;                      //See below for valid type codes.
  };
#define InitializationCommand 1   //The command is from the -init=... sequence.
#define ScriptFrame 2             //The command is from a %r=<scriptPath>
#define MacroFrame 3              //The command is from a '<key> macro call.
#define CallFrame 4               //The command is from a %h'=call <functionName>
#define ConsoleCommand 5          //The command was read from the console or from an -asConsole script.
#define DebuggerCommand 6         //The command was entered to the debugger.
  
static struct BacktraceFrame  *s_BacktraceFrame = NULL; //Points to first member of backtrace chain.

#define FrameType_Void 0
#define FrameType_Int 1
#define FrameType_Float 2
#define FrameType_Buf 3

struct intFrame {                   //One frame in the stack - (long long) is assumed to be the buggest of (long long), (double) and (struct Buf *).
  long long Value;                  //The int value.
  char type;                        //One of FrameType_Int, FrameType_Float or FrameType_Buf - always FrameType_Int for intFrame.
  };
struct floatFrame {                 //One frame in the stack - used only to cast the value field as (double).
  double fValue;                    //The floating-point value object - same size as long long.
  char type;                        //Always FrameType_Float
  };
struct bufFrame {                   //One frame in the stack - used only to cast the value field as (struct Buf *).
  struct Buf  *BufPtr;              //Points to buffer - normally 4 bytes ... but no one's making any promises.
#ifdef VC
  char Padding[sizeof(long long)-4];
#else
  char Padding[sizeof(long long)-sizeof(struct Buf * )];
#endif
  char type;                        //Always FrameType_Buf
  };

//Window types for Window->WindowType:
#define WinType_Normal 1            //The window is an ordinary horizontal slice of the terminal screen.
#define WinType_Slice 2             //The window is a vertical slice.
#define WinType_Popup 3             //The window is a popup.
#define WinType_Mask 3              //Mask used to separate type codes from Frozen bit.
#define WinType_Leftmost 4          //The window left edge is aligned to the screen left edge (either an ordinary horizontal window or the first vertical slice).
#define WinType_LeftmostSlice 6     //The window is the first (leftmost) slice in a group.
#define WinType_Frozen 8            //The window has been forced to the frozen state.
#define WinType_Void 16             //The window is void - probably because one of the term sizes has been reduced.
struct Window {
  struct Window *next;              //The next window on the screen (in %w order).
  int OldFirstLineNo;               //The line number of the first record displayed in this window.
  int LastSubstringStart;           //The start point of the selected substring last displayed in this window.
  int LastSubstringLength;          //The length of the selected substring last displayed in this window.
  short unsigned int Top;           //Assigned top screen-line of window.
  short unsigned int Bot;           //Assigned bottom screen-line of window.
  short unsigned int Left;          //Assigned left-margin on screen.
  short unsigned int Right;         //Assigned right-margin on screen.
  short unsigned int PopupWidth;    //The current width of the popup - which might be less than Right-Left.
  short int DisplayFooter;          //When true, indicates that JotUpdateWindow should follow the window with the buffer details in reverse video.
  char WindowKey;                   //Key for the buffer associated with this window (if '\0' then JotUpdateWindow takes the s_CurrentBuf).
  char LastKey;                     //Key for the last buffer displayed in window.
  unsigned short int WindowType;    //The type of window - see above tor valid types.
  };

struct CommandFile {
  FILE *FileHandle;                 //The filehandle from open( ...).
  struct CommandFile *ReturnFile;   //The CommandFile that initiated this command file, at the bottom of the chain is s_EditorInput.
  struct Buf *CommandBuf;           //The command buffer associated with this command file.
  struct Buf *OldCommandBuf;        //The command buf that launched this  command file.
  int LineNo;                       //Counts the line numbers for error reporting.
  char *FileName;                   //The pathName of the command file.
  struct CommandFile *Pred;         //The next command file in the chain or NULL;
  char asConsole;                   //From the -asConsole qualifier.
  };
 
struct IOBlock {                    //Used by %E= ... -interactive and -%i= ... -hold I/O 
  struct Buf *NxInteractiveBuf;     //Points to next interactive I/O buffer.
  struct BucketBlock *BucketBlock;  //Temporary storage for buffers input operations.
  int HoldDesc;                     //File descriptor for normal (synchronous) reads with -hold qualifier.
  char *ExitCommand;                //The user-nominated command that causes the child to exit.
  struct Buf *WaitforSequence;      //The command sequence to be called run each new readback.
  struct Seq *Code;                 //The compiled WaitforSequence code.
  int IoFd;                         //Bidirectional fileDesc for the pty or InPipe file descriptor in windowsland.
  int StartTime;                    //Seconds since start of epoch.
  int Timeout;                      //Timeout (in seconds) for current read operation.
#if defined(VC)
  HANDLE OutPipe;                   //Interactive output stream.
  HANDLE ProcessId;                 //The child-process ID.
#else
  pid_t ChildPid;                   //Used to kill the child.
#endif
  };
  
struct BucketBlock {                //Used to buffer reads from unbuffered streams.
  int RecStartByte;                 //The first byte of the next record in this bucket.
  int RecEndByte;                   //The last byte of the next record in this bucket.
  int BufBytes;                     //The total number of bytes available in the bucket.
  struct BucketBlock *Next;         //Next bucket in chain.
  char Bucket[BucketSize];          //The read bucket.
  };

#define Attr_Normal    0
#define Attr_SelSubStr 1
#define Attr_CurrChr   2
#define Attr_WinDelim  3
#define Attr_CurrCmd   4
static int s_Attrs[5];                  //Translates various jot display attributes to system display attributes.
  
#if defined(VC)
static int s_OrigConsoleMode;           //Used to restore console mode.
static int s_DefaultColourPair = Attr_Normal;
static int s_CodePage = 65001;          //Changed from 65001 to avoid corrupting wine session.
DWORD fdwMode, fdwOldMode; 
HANDLE hStdout, hStdin;                 //The handles used by low-level I/O routines read() and write() to access stdin/stdout.
CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
WORD s_OrigColorAttrs; 
int OpenWindowsCommand(HANDLE *, HANDLE *, HANDLE *, int, int, char *);
BOOL Ctrl_C_Interrupt(DWORD);
int ReadBuffer(FILE *, HANDLE *, int, int, char *, struct Buf *, int, long long, long long *, int, int);
int ReadBucketFromHandle(HANDLE *, char *, int);
int ReadUnbufferedStreamRec(struct Buf *, int, HANDLE *, int, long long *);
#else

volatile int PropagateSigwinch = 0;
void SigwinchHandler(int);
int RegexSearchBuffer(struct Buf *, char, char *, int , int, int);
static int s_DefaultColourPair;
static char *s_Locale;
void Ctrl_C_Interrupt(int);
int ReadBuffer(FILE *,           int, int, char *, struct Buf *, int, long long, long long *, int, int);
int ReadUnbufferedStreamRec(struct Buf *, int, int, long long *);
int DoublePopen(int *, int *, pid_t *, struct Buf *, char *);
#endif

char *Jot_strcasestr(char *, char *);
int SimpleSearchBuffer(struct Buf *, char *, int, int);
int GeneralSearchBuffer(struct Buf *, char *, char, int, int, int, int, int);
int GetBufferPath(struct Buf *, char * , int);
void TextViaJournal(struct Buf *, char *);
void Zombify(struct Buf *, struct AnyHTabObj *);
void FreeTarget(struct Buf *, struct JumpHTabObj *);
void FreeTag(struct Rec *, struct AnyTag *);
int ReadNewRecord(struct Buf *, FILE *, int);
int MoveDown(struct Buf *, int, char *);
int Backtrace(struct Buf *);
int Run_Sequence(struct Seq *);
int Run_Block(struct Block *);
int GetChrNo(struct Buf *);
char CheckNextCommand(struct Buf *, char *);
int CopyToken(char * , struct Buf * , int);
int TrimCommand(char * , char * );
int CheckToken(char *, char *, int, int, struct Com *, int);
struct Block *JOT_Block(struct Block *, struct Buf *, int);
void StartEdit();
void ExpandEnv(char *, int);
void DestroyHtab(struct Buf *);
int AddFormattedRecord(int, struct Buf *, char *String, ...);
int JotGetCh(FILE *);

void DumpCommand(struct Com *, int *, char *);
struct Buf *QueryKey(char *, struct Buf *);
struct CodeHeader *QueryCode(char *, struct Buf *);
struct Buf *QueryPath(char *, struct Buf *, char **, struct intFrame **, struct DataHTabObj **);
struct DataHTabObj * QueryData(char *, struct Buf *);
struct SetsectHTabObj * QuerySeek(char *, struct Buf *);
struct SetsectHTabObj * QuerySection(char *, struct Buf *);
struct SetfsectHTabObj * QueryFSection(char *, struct Buf *);
void AddTag(struct Rec *, struct AnyTag *);
int SearchStackForBuf(struct Buf *); 
void DumpStack(struct Buf * );
int KillTop();
char *BinFind(char *, int, char *, int);
int BinCmp(char *, char *, int );
int JotMkdir(char * , int );
void UpdateJournal(char *, char *);
void JotDeleteChr(int, int);
void JotInsertChr(char);
void JotSetAttr(int);
void JotClrToEndOfSlice(int, int, int, int);
void JotClearToEOL();
void JotAddBoundedString(char *, int, int, int *, int *, int);
int JotStrlenBytes(char *, int );
int JotStrlenChrs(char *, int );
int JotRecLenChrs(struct Buf * , int );
int JotBytesInChEndr(char *, int);
int JotAddChr(signed char);
void JotGotoXY(int, int);
int InitTermTable();
void ForceLineRefresh();
void JotScroll(struct Window *, int);
int PushInt(long long);
int PushFloat(double );
int PushBuf(struct Buf *);
int InteractiveEditor(void);
int GetBreakArg(struct Buf *, int);
void DisplaySource(struct Com *, char *);
int AdvanceRecord(struct Buf *, int);
int BreakRecord(struct Buf *);
void ClearRecord(struct Buf *);
int DuplicateRecord(struct Buf *, struct Buf *);
char *ExtractKey(struct Buf *, struct Buf *);
int CheckCircularHier(struct Buf *);
int CheckChildBuffers(struct Buf *);
int FreeBuffer(struct Buf *);
int FreeRecord(struct Buf *, int);
long long GetDec(struct Buf *);
int GetHex(struct Buf *);
int GetOct(struct Buf *);
int GetRecord(struct Buf *, int, char *);
int CheckBufferKey(char );
void *JotMalloc(int);
struct Buf *GetBuffer(int, int);
int FreeBufferThings(struct Buf *);
void FreeIOBlock(struct Buf *);
void ClearBuffer(struct Buf *);
void CopyBuffer(struct Buf *, struct Buf *);
char ImmediateCharacter(struct Buf *);
int JoinRecords(struct Buf *);
void MoveRecord(struct Buf *, struct Buf *);
int Abstract(struct Buf *, struct Buf *, char );
char NextCommand(void);
char NextNonBlank(struct Buf *);
void ResetBuffer(struct Buf *);
int SearchRecord(struct Buf *, char *, int );
void SetPointer(struct Buf *, int);
char StealCharacter(struct Buf *);
int SubstituteString(struct Buf *, const char *, int);
int VerifyAlpha(struct Buf *);
int VerifyCharacter(struct Buf *, int);
int VerifyDigits(struct Buf *);
int VerifyNonBlank(struct Buf *);
int DoWindows(char, struct AnyArg **);
void JotBreak(char *);
void JotTrace(char *);
int TidyUp(int, char *);
int SynError(struct Buf *, char *, ...);
int Fail(char *, ...);
int RunError(char *, ...);
void Disaster(char *, ...);
void Message(struct Buf *, char *, ...);
void PriorityPrompt(char *, ...);
void CommandPrompt(char *, ...);
void Prompt(char *, ...);
void InitializeEditor(int, char *[]);
int Noise(void);
int OpenJournalFile(int *, FILE **, char *);
int SwitchToComFile(char *, int, char *, struct Com *);
char ReadCommand(struct Buf *, FILE *, char *, ...);
int TransEscapeSequence(struct Buf *, FILE *, signed char, char *);
int AnalyzeTabs(struct Buf *, struct Rec *, int);
int WriteString(char *, int, int, int, int, int, int, int [], int, struct Window *);
int JotUpdateWindow(void); 
void ScrollConsole(int, int);
char VerifyKey(int);
int NewCommandFile(FILE *, char *, struct Com *);
int FreeCommandFile();
int ChrUpper(char);
void ExitNormally(char *);
int JotWrite(FILE *, int, struct Buf *);
void FreeCommand(struct Com **);
void FreeSequence(struct Seq **);
void FreeBlock(struct Block **);
long long GetArg(struct Buf *, long long);
void AddBlock(struct Com *, struct Block *);
int AddIntArgFromText(struct Com *, struct Buf *, int, char *);
void AddIntArg(struct Com *, long long);
void AddFloatArg(struct Com *, double);
void AddStringArgPercent(struct Com *, struct Buf *, int);
void AddStringArg(struct Com *, struct Buf *);
int FetchIntArg(long long *, struct AnyArg **);
int FetchIntArgFunc(struct AnyArg **);
int FetchIntArgDeferred(long long *, int, char *);
long long FetchLLIntArgFunc(struct AnyArg **);
long long FetchLLHexArgFunc(struct AnyArg **);
int FetchHexArgFunc(struct AnyArg **);
double FetchFloatArg(struct FloatArg **);
struct Block *FetchBlock(struct PtrArg **);
int FetchStringArgPercent(char *, int *, struct AnyArg **);
char * FetchStringArg(struct PtrArg **, char *);
int DoHash(char, struct AnyArg **, struct Com *);
int SetDataObject(char *);
void JOT_Sequence(struct Buf *, struct Seq *, int);
int ReadString(char *, int, FILE *);
int CrossmatchPathnames(int *, FILE **, char [], char *, char *, char *, char *);
void DisplayDiag(struct Buf *, int, char *);
long long PopInt(int *);
double PopFloat(int *);
int DoSort(int , struct AnyArg **);
  
static char *s_ProgName;                              //From ArgV[0]
static long Original_nsec;
static int Original_sec;
static char *s_SetfsectPathName = NULL;               //The last-used pathname for setfsect.
static struct Buf *s_SetfsectBuffer = NULL;           //The buffer that's hashtable was last assigned a setfsect object.
static unsigned long long int s_CommandCounter;       //Counts commands for journal recovery following a Ctrl+C
static unsigned long long int s_CommandCounterInit;   //Initialization value for s_CommandCounter (defaults to 0).
static int s_MouseMask = 0;                           //Controls mouse event handling.
static int s_DebugLevel = 0;                          //Shows nesting of debugger calls.
static int s_GuardBandSize = 0;                       //Scrolling guardbands at top and bottom of window.
static char *s_ModifiedCommandString = NULL;          //Used to redefine the keyboard input line from %s=commandstring
static int s_NewFile = FALSE;                         //Indicates that the -new arg was given - creates a new file without reading.
static int s_NoUnicode = FALSE;                       //Indicates state of NoUnicode flag in buffer currently being displayed.
//There's also s_TTYMode - Disables curses and uses normal characters to indicate cursor position in line-editing mode.
static int s_StreamOut = FALSE;                       //Indicates that stdout is a stream.
static int s_StreamIn = FALSE;                        //Indicates that stdin is stream to read into the primary buffer.
static int s_ObeyStdin = FALSE;                       //Set by -Obey CLI qualifier - the stdin stream is commands.
                                                      //For console or -asConsole scripts unwinds current line and continues from next command.
static int s_Interrupt;                               //An Ctrl+C interrupt has been received - always calls JotBreak() or  unwinds call stacks right back to the real console.
static int s_BombOut;                                 //A an error or an interrupt has occured - causes all call frames to unwind tidily and silently.
static char *s_JournalPath = NULL;                    //Directory holding journal files.
static FILE *s_JournalHand = NULL;                    //File handle for journal file.
static int s_JournalUniquifier = 0;                   //Appended to some filenames in the journal area.
static int s_JournalDataLines;                        //Counts data lines written to journal, only used for <<Debugger on>> events.
static int s_RecoveryMode = FALSE;                    //Disables %O writing and %I reads path from recovery script when set.
static int s_OriginalSession = TRUE;                  //Reset on entry to recovery mode, prevents deletion of journal files on exit from recovery session.
static struct Buf *s_InitCommands = NULL;             //Buffer holding -init command sequence.
static int s_HoldScreen = FALSE;                      //Set by -Hold - holds the screen on exit.

static struct intFrame *s_Stack;                      //The operand stack.
static int s_StackSize = 100;                         //The maximum size of the main operand stack.
static int s_StackPtr;                                //Points to next item on the operand stack.
static struct Buf *s_LastFloatingBuf;                 //The last stack-based buffer that was the subject of a Z~ command.
static int s_LastFloatingBufPtr;                      //The stack pointer when s_LastFloatingBuf was set.
static struct Window *s_FirstWindow;                  //The first member of the window chain.
static int s_MaxSlices;                               //The maximum number of slaces in any window.
static int s_ChrNo;                                   //Real character no. in  current record (ie. ignoring LeftOffset) of next character to be written by WriteString()
static char **s_TermTable = NULL;                     //The first n elements point to text string in line/slice at the start of line n on the screen.

//Bitmasks for s_Verbose:
#define Verbose_NonSilent 1                           //Normal reporting of errors I/O etc. #define PromptUser
#define Verbose_Prompt 2                              //Issues a command prompt on completion of each command line.
#define Verbose_QuiteChatty 4                         //Reports more events and explains some failures.
#define Verbose_PrintLine 8                           //Writes current line before message.
#define Verbose_ReportCounter 16                      //Includes command count in message.
#define Verbose_AnnoyingBleep 32                      //Emits an annoying bleep on errors.
static int s_Verbose = 3;                             //Current verbosity level.
static char s_TableSeparator = '\t';                  //Used to identify tabular-text boundaries.
static int s_CommandMode = 0;                         //Goes non-zero for insert-mode.
static int s_CaseSensitivity;                         //Set to TRUE for case-sensitive searches.
static struct Buf *s_BufferChain;                     //First member of a linked-list of all buffers.
static struct Buf *s_CommandBuf;                      //The source-code buffer currently being scanned for commands.
static struct Com *s_CurrentCommand = NULL;           //The currently-executed command object, this global is only for error reporting.
static struct Rec *s_PrevCommandRec;                  //For implementation of Trace_CommandLines.
static int s_MaxConsoleLines = 0;                     //Sets maximum size of console area - including borrowable lines from lower windows.
static int s_NewConsoleLines = 0;                     //Counts console lines inserted since last command.
static int s_OldConsoleLines = 0;                     //The previous s_NewConsoleLines count - JotUpdateWindow displays messages in borrowed console lines once then redraws.
static int s_ConsoleTop = 0;                          //The screen line no. (top line is line 0) of the first line of the normal console area.
static int s_DisplayedConsole = FALSE;                //The screen has been updated and borrowed lines have been displayed.
struct Buf *s_CurrentBuf;                             //The buffer that's currently the focus of the editor.
static struct CommandFile *s_EditorInput;             //The current command stream - could be s_EditorConsole or some jot script initiated by %r
static struct CommandFile *s_asConsole;               //If set, this is a command file being run as -asConsole.
static struct CommandFile *s_EditorConsole;           //The console stream - usually stdin.
static FILE *s_EditorOutput;                          //Destination stream for p commands.
static int s_TraceMode;                               //Controls the editor debugger.
static int s_DefaultTraceMode;                        //Value for s_TraceMode that's assigned by the T command.
static int s_TraceLevel;                              //Counts macro/command-file/function depth
static int s_TraceSkipped;                            //Indicates that s_MaxTraceLevel has been exceeded at least once.
static int s_MaxTraceLevel;                           //When +ve, suppresses tracing when s_TraceLevel exceeds this level.
static int s_TTYMode;                                 //Set TRUE when in teletype (non-window) mode.
static int s_TermHeight = 0;                          //The current-terminal height.
static int s_TermWidth = 80;                          //The current-terminal width - defaults to 80.
static int s_PredefinedScreenSize = FALSE;            //Set true by -SCreensize CLI modifier.
static jmp_buf *s_RestartPoint;                       //For longjmp when {Ctrl+c} fails to exit current activity.
static char *s_StartupFileName;                       //Pathname of startup script specified in -startup CLI qualifier.
static char *s_DefaultComFileName = "${JOT_HOME}/coms/.jot"; 
static int s_FileFlags = 0;
static int s_x, s_y;
static int s_MouseBufKey, s_MouseLine, s_Mouse_x, s_Mouse_y;
static char s_CommandChr;
static int s_SystemMode = 0;
static char s_InsertString[StringMaxChr];
static char s_FindString[StringMaxChr];
static char s_GenFindString[StringMaxChr];
static int s_StartLineSave = -1;
static int s_StartChrSave = -1;
static int s_EndLineSave = -1;
static int s_EndChrSave = -1;
static char s_QualifyString[StringMaxChr];
static char s_PromptString[StringMaxChr];
struct Buf *s_OnKeyCommandBuf = NULL;
struct Buf *s_AfterKeyCommandBuf = NULL;
struct Buf *s_OnIntCommandBuf = NULL;
static char s_ArgString[StringMaxChr];
static int s_Y_ColumnNo = -1;                        //Column no. used by Y command, -1 indicates it may be set.
static int s_NoteLine;                               //Line no. of latest note point.
static int s_NoteByte;                               //Byte offset to latest note point.
static struct Buf *s_NoteBuffer = NULL;
static struct Buf *s_DebugBuf;                       //Command buffer for the jot debugger.
static int s_AbstractWholeRecordFlag = TRUE;
static int s_ExitBlocks;                             //Counts no. of blocks to be exited following an X command.
static int s_Return;                                 //Implementation of X0 command - early RETURN from outermost Macro, Function or Script.
 
static int s_InView;
struct Buf *s_FirstInteractiveBuf;                   //Points to first buffer using interactive I/O.
int InteractiveReadRecs();

#if defined(VC)
void *memmem(const void *, size_t , const void * const, const size_t);
#else
static WINDOW *mainWin = NULL;                       //In non-curses implementations this is set to -1 - assumed to be NULL in s_TTYMode.
short s_NextColourPair;                              //The next-available colour-pair ID number.
int TestInteractivePipe(struct Buf *);
#endif
fd_set s_InteractiveFDs;                             //List of active interactive handles.
int s_TopInteractiveFD = 0;                          //Highest-numbered member of s_InteractiveFDs - used by select().

//Definitions for %I
#define I_section 1
#define I_fsection 2
#define I_seek 3
#define I_bytes 4
#define I_block 5
#define I_records 6
#define I_binary 7
#define I_insert 8
#define I_append 9
#define I_hold 10

//definitions for %S
#define S_case 1
#define S_commandcounter 2
#define S_commandmode 3
#define S_commandstring 4
#define S_console 5
#define S_guardband 6
#define S_mousemask 7
#define S_on_key 8
#define S_after 9
#define S_prompt 10
#define S_recoverymode 11
#define S_setenv 12
#define S_setmouse 13
#define S_system 14
#define S_tab 15
#define S_trace 16
#define S_tracedefault 17
#define S_traceskip 18
#define S_verbose 19
#define S_on_int 20
#define S_sleep 21

//Definitions for %W
#define W_new 1
#define W_modify 2
#define W_clear 3
#define W_refresh 4
#define W_key 10
#define W_popupQual 11
#define W_delimQual 12
#define W_heightQual 13
#define W_widthQual 14
#define W_guardQual 15
#define W_deleteQual 16
#define W_insertQual 17
#define W_freezeQual 18
//#define W_winnoQual 19
#define W_keyQual 20

//Definitions for %B
#define B_readonly 1                     //Prohibits changes to this buffer.
#define B_writeifchanged 2               //Insists that changes are written out.
#define B_sameflag1 3                    //Sets the user change flag.
#define B_codepage 4                     //Define CodePage for this buffer.
#define B_header 5                       //Define a header string for display above window.
#define B_footer 6                       //Define a string to replace pathname in window-separator line.
#define B_leftoffset 7                   //Define LeftOffset for this buffer (leftmost character in view).
#define B_tabstops 8                     //Define TabStops - tabular-text stops for this buffer.
#define B_tabcells 9                     //Define tabcells - cell or column widths for this buffer. 
#define B_tagtype 10                     //tagtype - define a user tag, currently only colour tags supported.
#define B_encoding 11                    //encoding - set the unicode encoding scheme for writing this buffer.
#define B_addtag 12                      //addtag - applies a tag defined by %b=tagtype to the current text.
#define B_notTextQual 13                 //%b=addtag -text=... qualifier.
#define B_remove_tag 14                  //remove_tag - identifies a tag and removes it from the record structure.
#define B_colourQual 15                  //%b=remove_tag ... -colour;
#define B_textQual 16                    //%b=addtag -text=...; and %b=remove_tag ... -text; qualifiers
#define B_targetQual 17                  //%b=remove_tag ... -target;
#define B_sort 18                        //sort - perfom a qsort on the records in this buffer.
#define B_tabsort 19                     //tabsort - perfom a qsort on the records in this buffer.
#define B_unicode 20                     //Controls unicode support for this buffer.
#define B_pathname 21                    //Sets the pathName for when the buffer is written.
#define B_unrestricted 22                //As it says - no restrictions.
#define B_sortPlusQual 23
#define B_sortMinusQual 24
#if defined(VC)
#define B_colourBlack 0                  //Colours in windowsland.
#define B_colourWhite 7
#define B_colourRed 4
#define B_colourBlue 1
#define B_colourGreen 2
#define B_colourYellow 6
#define B_colourMagenta 5
#define B_colourCyan 3
#else
#define B_colourBlack 0                  //Colours in linuxland.
#define B_colourWhite 7
#define B_colourRed 1
#define B_colourBlue 4
#define B_colourGreen 2
#define B_colourYellow 3
#define B_colourMagenta 5
#define B_colourCyan 6
#endif

//Definitions for %Q
#define Q_version  1                     //version - print version or send it to a buffer.
#define Q_windows  2                     //windows - fails if not operating under windows.
#define Q_linux  3                       //linux fails if not operating under linux.
#define Q_case  4                        //case - fails if not case sensitive.
#define Q_system  5                      //System settings etc.
//
#define Q_wd  7                          //wd - return current directory only.
#define Q_heap  8                        //heap - report heap stats to stdout.
#define Q_backtrace  9                   //Writes diagnostic backtrace to nominated buffer.
#define Q_history 10                     //history - dumps history to specified buffer.
#define Q_tabstops 11                    //tabstops - pushes TRUE if tabstops or tabcells are set otherwise FALSE.
#define Q_tabcells 12                    //tabcells - pushes TRUE if tabcells are set otherwise FALSE.
#define Q_keys 13                        //keys - sends a list of keys and truncated endpoint strings to nominated buffer.
//#define Q_key 14                         //key - reports as above but only for selected key.
#define Q_tags 15                        //tags - sends a list of record tags in current buffer to nominated buffer.
#define Q_hereQual 1                     //Report only tags active at current character position.
#define Q_buffer 16                      //buffer - returns info on buffer.
#define Q_window 17                      //window - returns window size and allocation.
#define Q_time 18                        //time - return seconds since start of unix epoch.
#define Q_cputime 33                     //cpu time - time since system startup.
#define Q_date 19                        //date - return todays date only.
#define Q_inview 20                      //Is current character visible with current LeftOffset setting.
#define Q_commandmode 21                 //Query in command mode
#define Q_samesinceio 22                 //Query SameSinceIO in this buffer.
#define Q_samesinceindexed 23            //Query SameSinceIndexed in this buffer.
#define Q_samesincecompiled 24           //Query SameSinceCompiled in this buffer.
#define Q_sameflag1 25                   //Query or set SameFlag1 in this buffer.
#define Q_pid 26                         //pid - Process ID of current process.
#define Q_env 27                         //env - return translation of env variable.
#define Q_dir 28                         //dir <path> - return directory contents.
#define Q_file 29                        //file <pathName> - a selection of info on the selected file.
#define Q_stack 30                       //stack - dumps stack to selected buffer.
#define Q_verify 31                      //Verifies the basic integrity of the buffer and the sequence.
#define Q_atimeQual 32                   //Qualifiers to Q_dir query.
#define Q_ctimeQual 33
#define Q_gidQual 34
#define Q_inodeQual 35
#define Q_mtimeQual 36
#define Q_modeQual 37
#define Q_sizeQual 38
#define Q_uidQual 39
#define Q_appendQual 40
#define Q_nofollowlinksQual 41
#define Q_keyQual 42

//Definitions for %H
#define H_create 1          //create - initialize the table with specified no. of elements.
#define H_adjustQual 0      //The -adjust qualifier to %h=create
#define H_destroyQual 1     //The -destroy qualifier to %h=create
#define H_protectQual 2     //The -protect qualifier to %h=create
#define H_deleteQual 3      //The -delete qualifier to %h=create
#define H_newjump 2         //newjump - adds a new jump point to the hashtable.
#define H_addjump 3         //addjump - adds or replaces a jump point.
#define H_setfsect 4        //setfsect - adds FileHand, seek offset and byte count for %i ... -section=...; N.B. does not consume the stack values.
#define H_setsect 5         //setsect - adds seek offset and byte count fo %i ... -section=...; N.B. does not consume the stack values.
#define H_data 6            //data - creates an entry for stack-frame object.
#define H_setdata 7         //setdata - assigns the specified data object a value from the stack.
#define H_newQual 1         //The -new qualifier to setdata - creates the specified data object.
#define H_getdata 8         //setdata - copies the value of the specified data object to the stack.
#define H_jump 9            //jump - looks up item in table and switches context.
#define H_code 10           //code - adds entry point for a function.
#define H_call 11           //call - compiles and runs specified routine.
#define H_fix 12            //fix - corrects line numbers in hashtable entries.
#define H_delete 13         //delete - removes item from the table.
#define H_testkey 14        //testkey - verify that there is at least one key matching the given string,
#define H_typeQual 1        //The -type qualifier to h_testkey.
#define H_destroy 15        //destroy - frees the hashtable and unprotects records.
#define H_allQual 1         //Qualifier to H_destroy.
#define H_onelineQual 1     //Qualifier to H_call - compiles only the first line.

//Definitions for %D
#define D_append 1
#define D_break 2
#define D_literal 3
//Definitions for %G - n.b - this must not collide with any D_<...> value.
#define G_copyNotRead 4
  
//Definitions for %F
#define F_rex 1
#define F_back 2
#define F_reverse 3
#define F_setaperture 4
#define F_aperture 5
#define F_restart 6
#define F_startline 7
#define F_startchr 8
#define F_endline 9
#define F_endChr 10
#define F_any 11
#define F_all 12
#define F_first 13
#define F_substitute 14
#define F_tab 15

//Odds and sods.
#define O_append 1
#define O_binary 2
#define E_pipe 1
#define E_interactive 2
#define E_exit 3
#define P_waitfor 1
#define P_timeout 2
#define P_nocr 3
#define R_asconsole 1

//static int s_DbgCtr = 0;
//static FILE *Dbg;
//----------------------------------------------main
int main(int ArgC, char *ArgV[])
  { // Outer procedure - calls editor. 
//Monitor file debug.lis with tail -f debug.lis - debugging only.
//#if defined(VC)
//{ Dbg = fopen("win_debug.lis", "a"); fprintf(Dbg, "\n\n\nNew windows session -------------------------------------------------------\n");  fflush(Dbg); }
//#else
//{ Dbg = fopen("lin_debug.lis", "a"); fprintf(Dbg, "\n\n\nNew linux session -------------------------------------------------------\n");  fflush(Dbg); }
//#endif
  s_ProgName = ArgV[0];
  InitializeEditor(ArgC, ArgV); 
  
  StartEdit();
   
  //It only gets here if something has exited abnormally.
  FreeBuffer(s_InitCommands);
  s_InitCommands = NULL;
  TidyUp(0, "Abnormal exit");
  return 0; }

//----------------------------------------------InitializeEditor
void InitializeEditor(int ArgC, char *ArgV[])
  { // Editor initialization for editing and application use. 
  int Arg = 0;
  int CommandBufSize = 20;
  char *FileName = NULL;
  char *ToFile = NULL;
  char ActualFile[StringMaxChr];
#if defined(VC)
  SMALL_RECT SrWindow;
  
  //Take a look at WinCon.h to make sense of this.
  //
  //Attr_Normal = 0, Attr_SelSubStr = 1, Attr_CurrChr = 2, Attr_WinDelim = 3, Attr_CurrCmd = 4.
  //s_Attrs[Attr_Normal] = 0xF,  s_Attrs[Attr_SelSubStr] = 0xC,  s_Attrs[Attr_CurrChr] = 0x7F,  s_Attrs[Attr_WinDelim] = 0xF0,  s_Attrs[Attr_CurrCmd] = 0x7F
  //
  s_Attrs[Attr_Normal] =     ( FOREGROUND_INTENSITY | FOREGROUND_GREEN     | FOREGROUND_RED  | FOREGROUND_BLUE );
  s_Attrs[Attr_SelSubStr] =  ( FOREGROUND_INTENSITY | FOREGROUND_RED );
  s_Attrs[Attr_CurrChr] =    ( FOREGROUND_INTENSITY | BACKGROUND_INTENSITY | BACKGROUND_RED  | BACKGROUND_GREEN | BACKGROUND_BLUE);
  s_Attrs[Attr_WinDelim] =   ( BACKGROUND_INTENSITY | BACKGROUND_GREEN     | BACKGROUND_RED  | BACKGROUND_BLUE );
  s_Attrs[Attr_CurrCmd] =    (                        BACKGROUND_GREEN     | BACKGROUND_RED  | BACKGROUND_BLUE );
#else
  if ( ! (s_Locale = getenv("XTERM_LOCALE")))
    s_Locale = "en_US.utf-8";
  setlocale(LC_CTYPE, s_Locale);
  s_Attrs[Attr_Normal] =     A_NORMAL;       //0 in gdb.
  s_Attrs[Attr_SelSubStr] =  A_UNDERLINE;    //262144 in gdb.
  s_Attrs[Attr_CurrChr] =    A_STANDOUT;     //65536 in gdb.
  s_Attrs[Attr_WinDelim] =   A_REVERSE;      //
  s_Attrs[Attr_CurrCmd] =    A_STANDOUT;     //65536 in gdb.
#endif
  s_RestartPoint = (jmp_buf *)JotMalloc(sizeof(jmp_buf));
  s_StartupFileName = JotMalloc(12);
  strcpy(s_StartupFileName, "startup.jot");
  s_TTYMode = FALSE;
  s_StreamOut = ! isatty(fileno(stdout));
  s_StreamIn = ! isatty(fileno(stdin));
  s_FileFlags = 0;
  s_EditorInput = NULL;
  strcpy(s_PromptString, "> ");
  s_DebugBuf = GetBuffer('d', AlwaysNew);
  GetRecord(s_DebugBuf, StringMaxChr, NULL);
  s_InsertString[0] = '\0';
  s_FindString[0] = '\0';
  s_GenFindString[0] = '\0';
  s_QualifyString[0] = '\0';
  s_StackSize = StackSize;
  s_StackPtr = 0;
  s_asConsole = NULL;
  s_ExitBlocks = 0;
  s_LastFloatingBuf = NULL;
  
  while (++Arg < ArgC) // Argument extraction loop.
    if (ArgV[Arg][0] != '-') { //Argument looks like a filespec.
      if (FileName != NULL)
        Disaster("Multiple filenames \"%s\" and \"%s\"\n", FileName, ArgV[Arg]);
      else
        FileName = ArgV[Arg]; }
  
    else {
      char *EqChr = ArgV[Arg]+1;
      
      while (EqChr[0] && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ", toupper(EqChr[0])))
        EqChr++;
      if (EqChr[0] == '=')
        EqChr++;
      
      switch (toupper(ArgV[Arg][1])) { // Qualifier switch block.
        //  -                               - edit stdin stream.
        //  -Codepage                       - Sets windows code page for display (Windows only).
        //  -Init=<commandSequence>         - Execute these commands before starting interactive session.
        //  -History=<n>                    - Lines of history to be held in s_CommandBuf.
        //  -Hold                           - holds the screen on exit from curses.
        //  -Journal                        - requests journal files, these are saved in <primaryPathname>.jnl
        //  -Locale=<locale>                - Sets the linux locale for display (Linux only).
        //  -New                            - Creating new file
        //  -Obey                           - commands from stdin stream. 
        //  -Quiet                          - no messages except from P commands etc.
        //  -SCreensize=<width>[x<height>]  - Screen size follows.
        //  -STartup=<startupfilename>      - use specified startup file   N.B. Text file not read until after startup file has been run.
        //  -STAcksize=<stackSize>          - sets the stack size, defaults to StackSize.
        //  -TO=<filename>                  - On exit write the main file to specified pathName.
        //  -TTy                            - simple, non-curses,  teletype mode - no screen attributes to be used.      
         
        case 'N': // -New - create new file
          s_NewFile = TRUE;
          break;
           
        case 'I': // -init=<JOT commands>  - Initialization before starting interactive session.
          if ( ! EqChr)
            SynError(NULL, "No \"=\" found in -init option.");
          s_InitCommands = GetBuffer('i', OptionallyNew);
          if (s_InitCommands->CurrentRec == NULL)
            GetRecord(s_InitCommands, 1, "");
          for ( ; ; ) { //Identify any further args that form part of the init string.
            int ArgLength;
            s_InitCommands->SubstringLength = 0;
            s_InitCommands->CurrentByte = strlen(s_InitCommands->CurrentRec->text);
            SubstituteString(s_InitCommands, EqChr, -1);
            ArgLength = strlen(s_InitCommands->CurrentRec->text)-1;
            if ( (s_InitCommands->CurrentRec->text[ArgLength] == '\\') && (++Arg <= ArgC) ) {
              s_InitCommands->CurrentRec->text[ArgLength] = ' ';
              s_InitCommands->CurrentByte = ArgLength+1; }
            else
              break;
            EqChr = NULL; }
          s_InitCommands->CurrentByte = 0;
          break;
           
        case 'S': // 'S' commands switch.
          switch (toupper(ArgV[Arg][2]))
            {
          case 'C': // -SCreensize size follows.
            sscanf(EqChr, "%ix%i", &s_TermWidth, &s_TermHeight);
            s_PredefinedScreenSize = TRUE;
            break;
          case 'T':
            if (toupper(ArgV[Arg][3]) == 'A' && toupper(ArgV[Arg][4]) == 'C') //-STACksize=<n> - set the operand stack size.
              sscanf(EqChr, "%d", &s_StackSize);
            else { // -STartup file name follows, optionally, followed by args.
              if ( ! EqChr[0]) {
                free(s_StartupFileName);
                s_StartupFileName = NULL; }
              else { //Startup pathname specified.
                free(s_StartupFileName);
                s_StartupFileName = (char *)JotMalloc(strlen(EqChr)+1);
                strcpy(s_StartupFileName, EqChr); }
              break; } }
          break;
      
        case 'T':
          switch (toupper(ArgV[Arg][2])) { // 'T' commands switch.
            case 'O': // -To = <O/P filespec.
              ToFile = EqChr;
              break;
            case 'T': // -tty mode
              s_TTYMode = TRUE;
              break; }
               
          break;
      
        case 'Q': // -Quiet mode - no messages except from P commands etc.
          s_Verbose = 0;
          break;
          
        case 'J': //-Journal - requests journal files, these are saved in <primaryPathname>.jnl
          s_JournalPath = "";
          break;
        
#if defined(VC)
        case 'C': //-Codepage=<codepage> - for a specific codepage.
          sscanf(EqChr, "%ix", &s_CodePage);
          break;
#else
        case 'L': //-Locale=<locale> - for a specific locale setting.
          s_Locale = EqChr;
          setlocale(LC_CTYPE, s_Locale);
          break;
#endif
        
        case '\0': //Arg is a simple '-' - this indicates a pipe in from stdin.
          s_StreamIn = TRUE;
          break;
        
        case 'O': //-Obey - jot commands from stdin.
          s_StreamIn = FALSE;
          s_ObeyStdin = TRUE;
          break;
         
        default:
          fprintf(stderr, "Unknown qualifier \"%s\"\n", ArgV[Arg]);
           
        case 'H': //-Help, -HOld, -HIstory
          if (toupper(ArgV[Arg][2]) == 'O') { // -HOld - hold the screen on exit from curses.
            s_HoldScreen = TRUE;
            break; }
          else if (toupper(ArgV[Arg][2]) == 'I') { // -History=<n> - no. of command lines to hold in s_CommandBuf.
            sscanf(EqChr, "%d", &CommandBufSize); 
            break; }
             
        fprintf(stderr, "\nDocumentation guide at at %s/docs/README.html\n", getenv("JOT_HOME"));
        fprintf(stderr, "Valid qualifiers follow:\n");
        fprintf(stderr, "  -    - edit stdin stream.\n");
        fprintf(stderr, "  -New - Creating new file\n");
        fprintf(stderr, "  -History=<n> - Lines of history to be held in s_CommandBuf.\n");
        fprintf(stderr, "  -Hold - holds the screen on exit from curses.\n");
        fprintf(stderr, "  -Init=<commandSequence> - Execute these commands before starting interactive session.\n");
        fprintf(stderr, "  -Journal - requests journal files, these are saved in ./<primaryPathname>.jnl/\n");
        fprintf(stderr, "  -Obey - obey commands from stdin stream\n");
        fprintf(stderr, "  -Quiet - no messages except from P commands etc.\n");
        fprintf(stderr, "  -SCreensize=<width>[x<height>] - Screen size follows.\n");
        fprintf(stderr, "  -STAcksize=<stackSize> - set the operand-stack size [defaults to %d].\n", StackSize);
        fprintf(stderr, "  -STartup=<startupfilename> - use specified startup file.\n");
        fprintf(stderr, "  -TO=<filename> - On exit write the main file to specified pathName.\n");
        fprintf(stderr, "  -TTy - simple, non-curses,  teletype mode - no screen attributes to be used.\n");
#if defined(VC)
        fprintf(stderr, "  -Codepage - sets codepage (windows only).\n");
#else
        fprintf(stderr, "  -Locale - sets locale (linux only).\n");
#endif
        fprintf(stderr, "Bye now.\n");
        exit(1); } }
    
  if (getenv("JOT_JOURNAL")) //Journalling requested via the JOT_JOURNAL env.
    s_JournalPath = "";
  if (s_JournalPath) {
    char FullPath[StringMaxChr], Actual[StringMaxChr], *NameElem, LockFilePathname[StringMaxChr];
    struct stat Stat;
    if ( ! FileName)
      Disaster("Cannot use -journal in stream-in mode");
    s_JournalPath = (char *)JotMalloc(strlen(FileName)+8);
    strcpy(s_JournalPath, (NameElem = strrchr(FileName, '/')) ? NameElem+1 : FileName);
    strcat(s_JournalPath, ".jnl/");
    strcpy(LockFilePathname, s_JournalPath);
    strcat(LockFilePathname, "LOCK");
    
    if ( ! stat(LockFilePathname, &Stat)) //LOCK exists - exit now.
      Disaster("Detected a lock file ( %s ) - previous session did not close down normally", LockFilePathname);
    else { //Delete the journal files.
#if defined(VC)
      HANDLE DirHand = INVALID_HANDLE_VALUE;
      WIN32_FIND_DATA FileData;
      DWORD dwRet;
      char Path[StringMaxChr];
      
      strcpy(Path, s_JournalPath);
      strcat(Path, "*");
      DirHand = FindFirstFile(Path, &FileData);
      if (DirHand != INVALID_HANDLE_VALUE) {
        do {
          strcpy(Path, s_JournalPath);
          strcat(Path, FileData.cFileName);
          stat(Path, &Stat);
          if ((Stat.st_mode & S_IFMT) == S_IFDIR)
            continue;
          if(unlink(Path))
            printf("Failed to delete journal/backup file %s\n", Path); }
        while (FindNextFile(DirHand, &FileData) != 0);
        rmdir(s_JournalPath);
        FindClose(DirHand); }
#else
      DIR *PathElemDIR = opendir(s_JournalPath);
      struct dirent *Entry;
      
      if (PathElemDIR) { //PathElemDIR is NULL if some recovery session has already deleted the journal files.
        while ( (Entry = readdir(PathElemDIR)) ) {
          char temp[StringMaxChr];
          strcpy(temp, s_JournalPath);
          strcat(temp, Entry->d_name);
          stat(temp, &Stat);
          if ((Stat.st_mode & S_IFMT) == S_IFDIR)
            continue;
          if(unlink(temp))
            printf("Failed to delete journal/backup file %s\n", temp); }
        closedir(PathElemDIR);
        if(rmdir(s_JournalPath))
          printf("Failed to delete journal directory %s\n", s_JournalPath); }
    mousemask(0, NULL);
#endif
    }

    if (JotMkdir(s_JournalPath, 00777))
      Disaster("Can't create journal directory %s\n", s_JournalPath);
    strcpy(FullPath, s_JournalPath);
    strcat(FullPath, "history.txt");
    Actual[0] = '\0';
    CrossmatchPathnames(0, &s_JournalHand, "w", FullPath, NULL, NULL, Actual);
    if ( s_JournalHand == NULL)
      Disaster("Failed to open journal file \"%s\"", Actual);
    close(open(LockFilePathname, 0700)); }

#if defined(VC)
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if ( ! GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) 
      Fail("GetConsoleScreenBufferInfo: stdout Console Error"); 
    SrWindow = csbiInfo.srWindow;
    if ( ! s_TTYMode && ! s_PredefinedScreenSize) {
      s_TermWidth = SrWindow.Right-SrWindow.Left+1;
      s_TermHeight = SrWindow.Bottom-SrWindow.Top+1; }
#else
  if ( ! s_TTYMode) { //Using screen attributes - curses/windows screen initialization happens here.
    mainWin = initscr();
    refresh();
    start_color();
    use_default_colors();
    init_pair(1, -1, -1);
    s_NextColourPair = 2;
    s_DefaultColourPair = COLOR_PAIR(1);
    raw();
    keypad(mainWin, TRUE);
    noecho();
    mousemask(s_MouseMask, NULL);
    halfdelay(1);
    if ( ! s_PredefinedScreenSize) {
      s_TermWidth = getmaxx(mainWin);
      s_TermHeight = getmaxy(mainWin); } }
      
  s_FirstInteractiveBuf = NULL;
#endif
  //Read file/stdin or create an empty file image.
  if (s_NewFile) { //Create a new file.
    if ( ! FileName) { //No file name given.
      if ( ! s_StreamIn)
        Disaster("No filename specified."); }
    s_CurrentBuf = GetBuffer('.', AlwaysNew);
    s_CurrentBuf->LineNumber = 1;
    GetRecord(s_CurrentBuf, StringMaxChr, NULL);
    GetRecord(s_CurrentBuf, StringMaxChr, NULL);
    s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec;
    ToFile = FileName; }
  else {
    if (s_StreamIn) { // Reading from stdin stream.
      s_CurrentBuf = GetBuffer('.', AlwaysNew);
#if defined(VC)
      ReadBuffer(stdin, NULL, 0, FALSE, "-", s_CurrentBuf, 0, 0ll, NULL, FALSE, 0);
#else
      ReadBuffer(stdin,       0, FALSE, "-", s_CurrentBuf, 0, 0ll, NULL, FALSE, 0);
#endif
      }
    else { //Reading from a real file.
      int FileDesc = 0;
      
      ActualFile[0] = '\0';
#if defined(VC)
      CrossmatchPathnames(&FileDesc, NULL, "r", FileName, "./", NULL, ActualFile);
#else
      CrossmatchPathnames(&FileDesc, NULL, "r", FileName, NULL, NULL, ActualFile);
#endif
      s_CurrentBuf = GetBuffer('.', AlwaysNew);
      
      if (s_JournalHand) {
        char Temp[StringMaxChr], TimeStamp[30];
        time_t DateTime;
        
        DateTime = time(NULL);
        strftime(TimeStamp, 100, "%d/%m/%y, %H:%M:%S", localtime(&DateTime));
        sprintf(Temp, "%%%%Primary session pid %d, at %s", getpid(), TimeStamp);
        UpdateJournal(Temp, NULL);
        sprintf(Temp, "%%%%jot version %s", VERSION_STRING);
        UpdateJournal(Temp, NULL);
        sprintf(Temp, "%%%%Primary file %s", FileName);
        UpdateJournal(Temp, NULL);
        sprintf(Temp, "%%%%Recovery terminal size: %d %d", s_TermWidth, s_TermHeight);
        UpdateJournal(Temp, NULL); }
      if ( ! FileDesc) {
        if ( ! FileName )
          FileName = "";
        Fail("Can't open file \"%s\"", FileName);
        GetRecord(s_CurrentBuf, 1, "");
        s_CurrentBuf->PathName = (char *)JotMalloc(strlen(FileName)+25);
        strcpy(s_CurrentBuf->PathName, "[ Nonexistent file: ");
        strcat(s_CurrentBuf->PathName, FileName);
        strcat(s_CurrentBuf->PathName, " ]"); }
      else {
#if defined(VC)
        ReadBuffer(NULL, NULL, FileDesc, FALSE, ActualFile, s_CurrentBuf, 0, 0ll, NULL, FALSE, 0);
#else
        ReadBuffer(NULL,       FileDesc, FALSE, ActualFile, s_CurrentBuf, 0, 0ll, NULL, FALSE, 0);
#endif
        close(FileDesc); } } }
      
  if (ToFile) { //Rename the file.
    free(s_CurrentBuf->PathName);
    s_CurrentBuf->PathName = (char *)JotMalloc(strlen(ToFile)+1);
    s_CurrentBuf->NewPathName = TRUE;
    strcpy(s_CurrentBuf->PathName, ToFile); }
    
  if (s_StreamIn) { //Primary text from stdin stream - duplicate and switch so that stdin can be used for console reads.
    freopen(KBD_NAME, "r", stdin); }
  //Reorganize stdin and stdout as appropriate for required operating mode.
#if defined(VC)
  if (s_StreamOut) //Piped output - in windows just force -tty mode and have done with it.
    s_TTYMode = TRUE;
  if (s_StreamOut) //No output pipe.
    AllocConsole();
  if ( ! s_ObeyStdin) //No stdin switchover required.
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(hStdin, &s_OrigConsoleMode);
  if ( ! s_TTYMode) { //Normal screen mode - use console screen-text routines.
    s_OrigColorAttrs = csbiInfo.wAttributes; 
    if ( ! SetConsoleTextAttribute(hStdout, Attr_Normal))
      Fail("SetConsoleTextAttribute: Console Error");
  if (s_CodePage) {  
    if ( ! SetConsoleOutputCP(s_CodePage))
      puts("SetConsoleOutputCP failed"); } }
#endif

  InitTermTable();
  NewCommandFile(fopen(KBD_NAME, "r"), "<The Keyboard>", NULL);
  s_EditorConsole = s_EditorInput;
  s_EditorOutput = stdout;
  s_CommandBuf = s_EditorConsole->CommandBuf;
  if (CommandBufSize <= 0)
    Disaster("Invalid history-buffer size");
  while (CommandBufSize--)
    GetRecord(s_CommandBuf, StringMaxChr, NULL);
  s_CommandBuf->CurrentByte = 0;
  s_FirstWindow = NULL;
  s_CaseSensitivity = FALSE;
  JotSetAttr(Attr_Normal);
  
  s_TraceMode = 0;
  s_DefaultTraceMode = Trace_Stack | Trace_Backtrace | Trace_Print | Trace_Break | Trace_Source | Trace_AllCommands;
  
#if defined(VC)
  if( ! SetConsoleCtrlHandler( (PHANDLER_ROUTINE) Ctrl_C_Interrupt, TRUE ) ) 
    Disaster("Can't set up Ctrl+C handler");
  if ( (s_Verbose & Verbose_NonSilent) && ( ! s_StreamOut) )
    system("cls");
#elif defined(LINUX)
  if (signal(SIGINT, SIG_IGN) != SIG_IGN)
    sigset(SIGINT, Ctrl_C_Interrupt);
#endif
  
  s_Stack = (struct intFrame *)JotMalloc((sizeof(struct intFrame)+1)*s_StackSize);
  s_Stack[0].type = FrameType_Void; }

//----------------------------------------------StartEdit
void StartEdit()
  { //The initial text file is already read, renames and calls the main editor as requested.
  struct Seq *InitSequence = NULL;
  struct BacktraceFrame ThisBacktraceFrame;
  
  ThisBacktraceFrame.LastCommand = NULL;
  ThisBacktraceFrame.prev = s_BacktraceFrame;
  ThisBacktraceFrame.type = InitializationCommand;
  ThisBacktraceFrame.EditorInput = s_EditorInput;
  s_BacktraceFrame = &ThisBacktraceFrame;
  if (s_JournalHand && s_StartupFileName)
    UpdateJournal("%%Startup script", NULL);
  
  if (setjmp(*s_RestartPoint)) //Error detected in editor setup.
    Disaster("Error detected in startup sequence - session abandoned");
  if (s_StartupFileName) {
    char * Args = strchr(s_StartupFileName, ' ');
    char PathName[StringMaxChr];
    if (Args) {
      strncpy(PathName, s_StartupFileName, Args-s_StartupFileName);
      PathName[Args-s_StartupFileName] = '\0';
      SwitchToComFile(PathName, 0, Args, NULL); }
    else
      SwitchToComFile(s_StartupFileName, 0, "", NULL); }
 
  if (setjmp(*s_RestartPoint)) //Error detected in editor setup.
    Disaster("Error detected in -init commands sequence - session abandoned");
  if (s_JournalHand) {
    char Temp[StringMaxChr];
    sprintf(Temp, "%%%%Startup Sequence ends, buffer %c", s_CurrentBuf->BufferKey);
    UpdateJournal(Temp, NULL); }
  if (s_InitCommands) {
    s_BombOut = FALSE;
    s_CommandCounter = 0ll;
    if (s_JournalHand) {
      UpdateJournal("%%Init commands follow", NULL);
      UpdateJournal(s_InitCommands->FirstRec->text, NULL); }
    InitSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
    JOT_Sequence(s_InitCommands, InitSequence, FALSE);
    s_TraceLevel++;
    if (Run_Sequence(InitSequence)) {
      s_CurrentCommand = NULL;//At rhis point the command structures will have been free'd so s_CurrentCommand is now pointing off into hyperspace.
      RunError("-init sequence failed."); }
    if (s_JournalHand)
      UpdateJournal("%%Interactive session starts.", NULL);
    s_TraceLevel--; }
  
  FreeSequence(&InitSequence); 
  s_BacktraceFrame = NULL; 
  ThisBacktraceFrame.LastCommand = NULL;
  ThisBacktraceFrame.prev = s_BacktraceFrame;
  ThisBacktraceFrame.type = ConsoleCommand;
  ThisBacktraceFrame.EditorInput = s_EditorInput;
  s_BacktraceFrame = &ThisBacktraceFrame;
  s_Return = FALSE;
  while (InteractiveEditor())
    if (s_Return) {
      s_Return = FALSE;
      continue; }
  Message(NULL, "Abnormal exit");
  FreeBuffer(s_CurrentBuf); }

//----------------------------------------------InteractiveEditor
int InteractiveEditor()
  { //Main entry point for the C version of jot, called after reading the initial text-file 
    //image to buffer '.' and all function-key mappings have been defined by the startup file.
  struct Seq *MainSequence = NULL;
  int Status, LocalFail = 0;
  
  if (setjmp(*s_RestartPoint)) { //Returning from detected error. 
    s_CommandBuf->SubstringLength = 1;
    s_CurrentCommand = NULL; }
      
  for (;;) { //User command loop. 
    int InteractiveInput = (s_EditorInput == s_EditorConsole) || (s_RecoveryMode && s_EditorInput == s_asConsole);
    s_BombOut = FALSE;
    s_Interrupt = FALSE;
    s_MaxTraceLevel = s_TraceLevel = 0;
    JotUpdateWindow();
    if (s_Return) //X0 exits here.
      break;
    
    if (s_AfterKeyCommandBuf && InteractiveInput) { //%s=on_key -after command sequence set and we're interactive.
      //In fact, as we can see here, the -after command sequence is done *before* the next command - same difference.
      struct Seq *AfterCommandSequence = NULL;
      s_CommandBuf->CurrentByte = 0;
      AfterCommandSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
      JOT_Sequence(s_AfterKeyCommandBuf, AfterCommandSequence, FALSE);
      if (s_CommandChr == ')')
        SynError(s_CommandBuf, "Surplus \')\' in AfterCommand sequence");
      s_TraceLevel++;
      Status = Run_Sequence(AfterCommandSequence);
      s_TraceLevel--;
      if (Status && ! s_BombOut)
        RunError("Command-sequence failed.");
      FreeSequence(&AfterCommandSequence);
      JotUpdateWindow(); }
  
    if (s_CommandMode & CommandMode_ScreenMode)
      s_NewConsoleLines = 0;
    if ( ( ! s_InView) && (s_Verbose & Verbose_NonSilent) )
      DisplayDiag(s_CurrentBuf, TRUE, "");
    s_CurrentCommand = NULL;
    s_ExitBlocks = 0;
    //Detecting input from the console is not enough for the recovery-mode case - in recovery mode all  recovery activity 
    //is initiated by an -init command - normally -startup=recover or -init="%r=./recover_now -asConsole"
    //So, reset command counter in recovery mode if command source is -asConsole?
    if (InteractiveInput) {
      s_CommandCounter = s_CommandCounterInit;
      s_CommandCounterInit = 0ll; }
     
#if defined LINUX
    if (s_FirstInteractiveBuf) { //Set up to check for I/O-ready interactive subprocesses.
      struct Buf **ThisInteractiveBufPtr = &s_FirstInteractiveBuf;
      s_TopInteractiveFD = 0;
      while (*ThisInteractiveBufPtr) { //This loop checks for dead children.
        FD_SET((*ThisInteractiveBufPtr)->IOBlock->IoFd, &s_InteractiveFDs);
        if (s_TopInteractiveFD < (*ThisInteractiveBufPtr)->IOBlock->IoFd)
          s_TopInteractiveFD = (*ThisInteractiveBufPtr)->IOBlock->IoFd;
        ThisInteractiveBufPtr = &((*ThisInteractiveBufPtr)->IOBlock->NxInteractiveBuf); } }
#endif
          
    Status = ReadCommand(
      s_CommandBuf,
      s_EditorInput->FileHandle,
      ((s_CurrentBuf->CurrentRec) == (s_CurrentBuf->FirstRec->prev) ? "****End****%d %c> " : "%d %c> "),
      s_CurrentBuf->LineNumber,
      s_CurrentBuf->BufferKey);
    s_EditorInput->LineNo++;
    if (s_JournalHand)
      s_JournalDataLines = 0;
#if defined(LINUX)
    refresh();
#endif
    if (Status) {
      if ( s_asConsole && (s_Interrupt || Status == (char)EOF) ) //Status is set to EOF at the end of an -asConsole script.
        break;
      else { //An error of some kind
        continue; } }
    
    if (s_OnKeyCommandBuf && InteractiveInput) { //on_key command sequence set and we're interactive.
      struct Seq *OnCommandSequence = NULL;
      s_CommandBuf->CurrentByte = 0;
      OnCommandSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
      JOT_Sequence(s_OnKeyCommandBuf, OnCommandSequence, FALSE);
      if (s_CommandChr == ')')
        SynError(s_CommandBuf, "Surplus \')\' in OnCommand sequence");
      s_TraceLevel++;
      Status = Run_Sequence(OnCommandSequence);
      s_TraceLevel--;
      if (Status && ! s_BombOut)
        RunError("Command-sequence failed.");
      FreeSequence(&OnCommandSequence); }
      
    if (VerifyDigits(s_CommandBuf)) { //Repeat command.
      long long Repeat = GetDec(s_CommandBuf);
      int Count = 0;
      
      if ( ! MainSequence) {
        RunError("Last command failed.");
        continue; }
      if (VerifyNonBlank(s_CommandBuf)) {
        SynError(s_CommandBuf, "Unexpected characters");
        continue; }
       
      s_TraceLevel++;
      for ( ; ; )
        if (Run_Sequence(MainSequence)) { // Failure detected.
          if (0 < Repeat) {
            Message(NULL, "Repeated command failed no. %d of %d", Count, Repeat);
            break; }
          else //That's OK it was an infinite repeat.
            break; }
        else
          if (++Count ==  Repeat) {
            if (s_Verbose & Verbose_QuiteChatty)
              Message(NULL, "Completed all reiterations without error");
            break; }
      s_TraceLevel--;
             
      continue; }
    
    FreeSequence(&MainSequence);
    s_CommandBuf->CurrentByte = 0;
    s_Return = FALSE;
    MainSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
    JOT_Sequence(s_CommandBuf, MainSequence, FALSE);
    if (s_CommandChr == ')')
      SynError(s_CommandBuf, "Surplus \')\'");
    s_TraceLevel++;
    LocalFail = Run_Sequence(MainSequence);
    s_TraceLevel--;
    if (LocalFail && ! s_BombOut) {
      RunError("Command-sequence failed.");
      if (s_Interrupt)
        break; } }
        
  FreeSequence(&MainSequence);
  return LocalFail; }

//---------------------------------------------JOT_Sequence
void JOT_Sequence(struct Buf *CommandBuf, struct Seq *ThisSequence, int StopOnEOL)
  { //The main entry point for the JOT compiler.
    //Compiles a sequence of JOT commands, terminated by one of the following:
    // ',' - an JOT-style else clause follows - calls itself recursively,
    // ')' - end of JOT-style block - pass it back to calling JOT_Block
    // '\0' - the end of a command line. */
    //When TRUE, StopOnEOL stops compilation at the end of the initial line.
  struct Com *ThisCommand = NULL;
  struct Com *PrevCommand = NULL;
  struct Rec *InitialRec = CommandBuf->CurrentRec;
  int LocalFail = FALSE;
  
  ThisSequence->FirstCommand = NULL;
  ThisSequence->ElseSequence = NULL;
  ThisSequence->NextSequence = NULL;
  CommandBuf->UnchangedStatus |= SameSinceCompiled;
  s_PrevCommandRec = NULL;
   
  for ( ; ; ) { // Command-sequence loop.
    int CommandByteNo;
    int CommandLineNo;
    
    s_CommandChr = ChrUpper(NextNonBlank(CommandBuf));
    if (StopOnEOL && CommandBuf->CurrentRec != InitialRec)
      return;
    CommandByteNo = (CommandBuf->CurrentByte)-1;
    CommandLineNo = CommandBuf->LineNumber;
    if (s_CommandChr == ',') { //,  ELSE Clause follows. 
      ThisSequence->ElseSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
      JOT_Sequence(CommandBuf, ThisSequence->ElseSequence, StopOnEOL);
      break; }
    else if (s_CommandChr == '\0') { // End of command line reached.
      if (StopOnEOL)
        return;
      if ( ! AdvanceRecord(CommandBuf, 1))
        continue;
      return; }
  
    if (CommandBuf->CurrentByte == 1 && CommandBuf->CurrentRec->text[0] == '<' && CommandBuf->CurrentRec->text[1] == '<') { //Looks like a function header?
      if (CommandLineNo == 1) { //This is the first line of the function - skip forwards to next line.
        AdvanceRecord(CommandBuf, 1);
        continue; }
      else { //This is the start of the next function - exit now.
        CommandBuf->CurrentRec = CommandBuf->FirstRec->prev;
        CommandBuf->CurrentByte = strlen(CommandBuf->CurrentRec->text);
        break; } }
    ThisCommand = (struct Com *)JotMalloc(sizeof(struct Com));
//    if (ThisCommand == NULL) {
//      SynError(NULL, "Failed to allocate space for command.");
//      return; }
    ThisCommand->ArgList = NULL;
    ThisCommand->NextCommand = NULL;
    if (PrevCommand == NULL)
      ThisSequence->FirstCommand = ThisCommand;
    else
      PrevCommand->NextCommand = ThisCommand;
    ThisCommand->CommandKey = s_CommandChr;
    ThisCommand->ArgList = NULL;
    ThisCommand->NextCommand = NULL;
    ThisCommand->CommandBuf = CommandBuf;
    ThisCommand->CommandRec = CommandBuf->CurrentRec;
    ThisCommand->CommandByteNo = CommandByteNo;
    ThisCommand->ComChrs = 0;
    ThisCommand->CommandLineNo = CommandLineNo;
       
    switch (s_CommandChr) {
    case '\'':  //' - Deferred command specification.
      AddIntArg(ThisCommand, ChrUpper(ImmediateCharacter(CommandBuf)));
      break;
  
    case '(': { //( - Open a new Block. 
      struct Block *NewBlock = (struct Block *)JotMalloc(sizeof(struct Block));
      NewBlock->BlockSequence = NULL;
      AddBlock(ThisCommand, NewBlock);
      if ( ! JOT_Block(NewBlock, CommandBuf, StopOnEOL))
        return;
      break; }
  
    case ')':  // End of Block. 
      ThisCommand->ComChrs = (CommandBuf->CurrentByte)-ThisCommand->CommandByteNo;
      return;
  
    case '?':  //force Pass/fail state to TRUE.
    case '\\':  //reverse pass/fail state.
      break;
  
    case 'A': { //A - Abstract using note record pointer. 
      char Chr, Qual1 = '\0', Qual2 = '\0';
      Chr = ImmediateCharacter(CommandBuf);
      if ( ! CheckBufferKey(Chr)) {
        SynError(CommandBuf, "Syntax invalid buffer identifier %c", Chr);
        continue; }
      AddIntArg(ThisCommand, ChrUpper(Chr));
      Chr = StealCharacter(CommandBuf);
      if (Chr == '&') {
        Qual2 = ImmediateCharacter(CommandBuf);
        Chr = StealCharacter(CommandBuf);
        if (strchr("+.-", Chr))
          Qual1 = ImmediateCharacter(CommandBuf); }
      else if (strchr("+.-", Chr)) {
        Qual1 = ImmediateCharacter(CommandBuf);
        Chr = StealCharacter(CommandBuf);
        if (Chr == '&')
          Qual2 = ImmediateCharacter(CommandBuf); }
      AddIntArg(ThisCommand, Qual1);
      AddIntArg(ThisCommand, Qual2);
      break; }
  
    case 'Q':  //Q - Qualify.
      AddIntArg(ThisCommand, (VerifyCharacter(CommandBuf, '-') ? -1 : 1));
      AddStringArg(ThisCommand, CommandBuf);
      break;
  
    case 'V':  //V - Verify text string
      AddIntArg(ThisCommand, (VerifyCharacter(CommandBuf, '-') ? -1 : 1));
      AddStringArg(ThisCommand, CommandBuf);
      break;
  
    case 'M':  //M - Move command.
    case 'Y':  //Y - move in same column.
      {
      char Chr, Arg1 = '\0';
      int Arg2;
      
      Chr = StealCharacter(CommandBuf);
      if (Chr == '=' || Chr == '+') {
        Arg1 = '=';
        ImmediateCharacter(CommandBuf);
        Arg2 = GetArg(CommandBuf, 1); }
      else if (Chr == '*') {
        Arg1 = ImmediateCharacter(CommandBuf);
        Arg2 = GetArg(CommandBuf, 1); }
      else {
        Arg2 = GetArg(CommandBuf, 1);
        Chr = StealCharacter(CommandBuf);
        if (Chr == '=' || Chr == '+') {
          Arg1 = '=';
          ImmediateCharacter(CommandBuf); }
        else if (Chr == '*')
          Arg1 = ImmediateCharacter(CommandBuf); }
      AddIntArg(ThisCommand, Arg1);
      AddIntArg(ThisCommand, Arg2);
      break; }
  
    case 'X': { //X - eXit (or repeat) current (or outer) block, macro or function.
      int Arg1;
      Arg1 = GetArg(CommandBuf, 1);
      AddIntArg(ThisCommand, Arg1);
      break; }
    
    case 'T':  //T - Trace i.e. debug
      break;
  
    case 'N':  //N - Note current Record for later abstract.
      AddIntArg(ThisCommand, VerifyCharacter(CommandBuf,'.'));
      break;
  
    case 'F':  //F - Find Command.
      AddIntArg(ThisCommand, (VerifyCharacter(CommandBuf, '-') ? -1 : 1));
      AddIntArg(ThisCommand, ((s_CommandChr == 'F') ? GetArg(CommandBuf, 0) : GetArg(CommandBuf, 1)));
      AddStringArg(ThisCommand, CommandBuf);
      AddIntArg(ThisCommand, GetArg(CommandBuf, 1));
      break;
  
    case 'H': { //H - Here command.
      char Chr;
      int Repeats = 1;
      Chr = ChrUpper(ImmediateCharacter(CommandBuf));
      AddIntArg(ThisCommand, Chr);
      if ( ! CheckBufferKey(Chr)) {
        SynError(CommandBuf, "Invalid buffer.");
        continue; }
      Repeats = GetArg(CommandBuf, 1);
      if (StealCharacter(CommandBuf) == '*')
        SynError(CommandBuf, "\"*\" not allowed in this context.");
      AddIntArg(ThisCommand, Repeats);
      break; }
  
    case 'S':  //S - Substitute command.
      AddIntArg(ThisCommand, (VerifyCharacter(CommandBuf, '-') ? -1 : 1));
      AddStringArg(ThisCommand, CommandBuf);
      break;
  
    case 'I':  //I - Insert command.
      AddIntArg(ThisCommand, (VerifyCharacter(CommandBuf, '-') ? -1 : 1));
      AddStringArg(ThisCommand, CommandBuf);
      AddIntArg(ThisCommand, (VerifyDigits(CommandBuf) ? GetDec(CommandBuf) : 1));
      break;
  
    case 'K':  //K - delete (Kill) some lines.
    case 'P':  //P - Print lines.
    case 'J':  //J - Join line(s).
    case 'C':  //C - Change case of characters.
    case 'E':  //E - Erase bytes or characters.
      AddIntArg(ThisCommand, ((VerifyCharacter(CommandBuf, '-') ? -1 : 1)));
      AddIntArg(ThisCommand, GetArg(CommandBuf, 1));
      break;
      
    case 'B': { //B - break line.
      AddIntArg(ThisCommand, ((VerifyCharacter(CommandBuf, '-') ? -1 : 1)));
      AddIntArg(ThisCommand, GetBreakArg(CommandBuf, 1));
      break; }
      
    case 'R':  //R - Move current character pointer right n bytes.
    case 'G':  //G - Get - Input text from console.
      AddIntArg(ThisCommand, GetArg(CommandBuf, 1));
      break;

    case 'O': {  //O - stack-Operation command.
      char Arg1 = ChrUpper(ImmediateCharacter(CommandBuf));
      char Arg2;
      AddIntArg(ThisCommand, Arg1);
      switch (Arg1) {//Stack operation switch block.
      case 'L': { //OL - OL<nnn>[.<mmm>] - Literal, integer of floating, number to be pushed onto stack.
        int Chr, Chrs = 0;
        int InitialCharacter = CommandBuf->CurrentByte;
        long long int IntLit;
        double FloatLit;
        
        Chr = StealCharacter(CommandBuf);
        if (Chr == 'x') {
          CommandBuf->CurrentByte++;
          IntLit = GetHex(CommandBuf); }
        else if (Chr == 'c') {
          CommandBuf->CurrentByte++;
          IntLit = CommandBuf->CurrentRec->text[CommandBuf->CurrentByte++]; }
        else if (Chr == 'o') {
          CommandBuf->CurrentByte++;
          IntLit = GetOct(CommandBuf); }
        else
          IntLit = GetDec(CommandBuf);
        if (StealCharacter(CommandBuf) != '.') {
          AddIntArg(ThisCommand, 0);
          AddIntArg(ThisCommand, IntLit); }
        else {
          sscanf(CommandBuf->CurrentRec->text+InitialCharacter, "%lf%n", &FloatLit, &Chrs);
          CommandBuf->CurrentByte = InitialCharacter+Chrs;
          AddIntArg(ThisCommand, 1);
          AddFloatArg(ThisCommand, FloatLit); }
        break; }
      case 'O':  //OO<string> - formatted Output using string as sprintf format string.
      case 'V':  //OV<htabPath> - Defines the value of a hashtable data entry.
      case 'Q':  //OQ<htabPath> - Retreives the value of a hashtable data entry.
        AddStringArg(ThisCommand, CommandBuf);
        break;
      case 'I':  //OI<string> - formatted Input - Convert string at CurentChr using string as sscanf format.
//      case '^':  //O^ Test state
        AddIntArg(ThisCommand, ChrUpper(ImmediateCharacter(CommandBuf)));
        break;
      case 'G':  //OG prompts and reads one character from console stream.
      case 'W':  //OW Scrolls up by lines set in top of stack.
      case '@':  //O@ reset stack
      case '?':  //O? Dump contents of stack
      case 'S':  //OS Swap 1st and 2nd items of stack.
      case '#':  //O# Duplicate top of stack.
      case '=':  //O= Fail if top two items on stack not equal.
      case '0':  //O0 Fail if intergerized top item on stack is not 0.
      case '1':  //O1 Fail if intergerized top item on stack is not 1.
      case '<':  //O< Test that item at top greater than (top-1).
      case '>':  //O> Test that item at top less than (top-1).
      case 'K':  //OK Kill - delete top of stack.
      case 'D':  //OD Decimal conversion - pop stack, decimal to current chr.
      case '~':  //O~ Increment - add 1 to item at top, fail if result=0.
      case 'A':  //OA rAndom - push a random number onto the stack.
      case '!':
      case '&':
      case '|':  //Bitwise NOT, AND and OR
      case '+':
      case '-':
      case '*':
      case '/':  //Add, Subtract, Multiply and Divide.
      case '%':  //O% Remainder
      case '.':  //O. Set current line number to <top of stack>.
      case 'B':  //OB Push pointer to current buffer onto stack.
      case 'X':  //OX Push eXtent (i.e. length of current substring) taking unicode characters into account.
      case 'F':  //OF Push line no. of first line in window.
      case 'P':  //OP Push mouse Position - last detected mouse-click position
      case 'N':  //ON Push current line no. onto stack.
      case 'U':  //OU set current sUbstring length from value on stack.
      case 'C':  //OC Push current chr. no. onto stack.
      case 'Z':  //OZ Zoom (change to buffer) pointer popped off stack.
      case 'R':  //OR shift Right (left if -ve) no. of bytes popped off stack.
      case 'E':  //OE Erase no. of characters specified by top of stack.
      case 'M':  //OM Move (backwards if -ve) no. of lines popped off stack.
        break;
      default:
        LocalFail = SynError(CommandBuf, "Unknown stack operator ( %c )", Arg1);
      };//Stack-operation switch block ends.
    break;
  
    case 'Z':  //Z - zoom into another buffer.
      Arg2 = ChrUpper(ImmediateCharacter(CommandBuf));
      if ( ! CheckBufferKey(Arg2))
        SynError(CommandBuf, "Invalid buffer tag.");
      AddIntArg(ThisCommand, Arg2);
      break; }

    case '%': { //% - Percent-command-argument block.
      char Token[30], BufferKey;
      int BufferKeyChr, QualLen;
//      int BufferKeyChr, QualLen, LocalFail = FALSE;
      
      s_CommandChr = ChrUpper(NextNonBlank(CommandBuf));
      AddIntArg(ThisCommand, s_CommandChr);
      BufferKeyChr = CommandBuf->CurrentByte;
      BufferKey = ChrUpper(CommandBuf->CurrentRec->text[BufferKeyChr]);
      if (BufferKey == ';') { //This is a very short command terminated with a semicolon - like "%w; m" - ensures that the following command is parsed correctly.
        BufferKey = '\0';
        CommandBuf->CurrentByte = BufferKeyChr-1; }
      if (BufferKey == '=' && CommandBuf->CurrentRec->text[BufferKeyChr+1] != '=') //No buffer key given.
        BufferKey = '\0';
      else if (CommandBuf->CurrentRec->text[BufferKeyChr] != '\0')
        CommandBuf->CurrentByte++;
      if (s_CommandChr == '%') { //%% - Comment, skip all the comment text.
        CommandBuf->CurrentByte = strlen(CommandBuf->CurrentRec->text);
        AddIntArg(ThisCommand, BufferKey); }
        
      else if (s_CommandChr == 'A') { //%A<status>[=<message>] - pick up status and message
        if ('0' <= BufferKey && BufferKey <= '9') {
          CommandBuf->CurrentByte -= 1;
          AddIntArgFromText(ThisCommand, CommandBuf, 10, "=");
          CommandBuf->CurrentByte += 1; }
        else
          AddIntArg(ThisCommand, 1);
        if (CommandBuf->CurrentRec->text[CommandBuf->CurrentByte] == '=')
          CommandBuf->CurrentByte += 1;
        AddStringArgPercent(ThisCommand, CommandBuf, TRUE); }
        
      else //Any other percent command.
        AddIntArg(ThisCommand, BufferKey);
        
      if (CommandBuf->CurrentRec->text[CommandBuf->CurrentByte] == '=')
        CommandBuf->CurrentByte += 1;

      switch (s_CommandChr) { // Percent-command switch block.
        case 'A':  //%A - Exit without writing file.
        case 'U':  //%U - Undo last substitution.
        case '%':  //%% - Comment line.
          break;

        case 'G': { //%G - Get - but from current command file or console (macro case dealt with above).
          if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) && CheckToken("APPEND", Token, 1, QualLen, ThisCommand, D_append))
            { }
          if (CommandBuf != s_CommandBuf) { //Reading commands from a macro or function.
            struct StringArg *StringArg, *PrevArg;
            char *TermStart = CommandBuf->CurrentRec->text + CommandBuf->CurrentByte, *TermEnd  = strchr(TermStart, ';');
            int TermLength = TermEnd ? TermEnd-TermStart : strlen(TermStart);
            
            AddIntArg(ThisCommand, G_copyNotRead);
            
            PrevArg = (struct StringArg *)ThisCommand->ArgList->next;
            while (PrevArg->next)
              PrevArg = (struct StringArg *)PrevArg->next;
        
            while ( ! AdvanceRecord(CommandBuf, 1)) { //Copy records to arg list loop.
              char *Text = CommandBuf->CurrentRec->text;
              int TextLength = strlen(Text);
              if ( (memmem(Text, TextLength, TermStart, TermLength) == Text) && TextLength == TermLength+1 && Text[TextLength-1] == ':') //A valid termination label found.
                break;
              StringArg = (struct StringArg *)JotMalloc(sizeof(struct StringArg));
              PrevArg->next = (struct AnyArg *)StringArg;
              StringArg->next = NULL;
              StringArg->type = StringArgType;
              StringArg->Continuation = NULL;
              StringArg->String = JotMalloc(TextLength+1);
              strcpy(StringArg->String, Text);
              StringArg->StringLength = TextLength;
              PrevArg = StringArg; }
            AdvanceRecord(CommandBuf, 1); }
        else //Reading commands from console  or a command file.
          AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
        break; }

        case '~':  //%~ - Insert or display control character.
          AddIntArgFromText(ThisCommand, CommandBuf, 16, ";");
          break;

        case 'L':  //%L - set terminal line Length and, optionally, terminal height.
          if ( ! AddIntArgFromText(ThisCommand, CommandBuf, 10, "x"))
            AddIntArg(ThisCommand, -1);
          if ( ! AddIntArgFromText(ThisCommand, CommandBuf, 10, ";"))
            AddIntArg(ThisCommand, -1);
          break;

        case 'D': { //%D - Define a buffer from console or last console command.
          if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
            if ((strstr(Token, "LITERAL_DEFINITION") == Token) ) {
              char *CommandStart = CommandBuf->CurrentRec->text + CommandBuf->CurrentByte + 1;
              char * Next = CommandBuf->CurrentRec->text + CommandBuf->CurrentByte;
              char * Endpoint = strstr(CommandStart, " -BUFFERDEFINITIONENDSHERE;");
              
              if ( ! Endpoint || Endpoint <= Next) {
                //The user has probablyeithr not specified -BUFFERDEFINITIONENDSHERE or only put one blank before the terminator string e.g: -LITERAL_DEFINITION -BUFFERDEFINITIONENDSHERE;
                CommandBuf->SubstringLength = 0;
                SynError(CommandBuf, "Syntax error in specification of a literal string.");
                break; }
              else
                CommandBuf->CurrentByte++;
              AddIntArg(ThisCommand, D_literal);
              struct StringArg * NewArg = (struct StringArg *)JotMalloc(sizeof(struct StringArg));
              NewArg->type = StringArgType;
              NewArg->Continuation = NULL;
              NewArg->String = (char *)JotMalloc(Endpoint-CommandStart+1);
              memcpy(NewArg->String, CommandStart, Endpoint-CommandStart);
              NewArg->String[Endpoint-CommandStart] = '\0';
              NewArg->StringLength = Endpoint-CommandStart;
              NewArg->next = NULL;
              ThisCommand->ArgList->next->next->next = (struct AnyArg *)NewArg;
              CommandBuf->CurrentByte += Endpoint-CommandStart+27; }
            else {
              do {
                if (CheckToken("APPEND", Token, 1, QualLen, ThisCommand, D_append))
                  { }
                else if (CheckToken("BREAK", Token, 1, QualLen, ThisCommand, D_break))
                  { }
                else
                  LocalFail = SynError(CommandBuf, "Invalid qualifier to %%D command"); }
                while ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) );
              
              if (LocalFail)
                break;
              AddStringArgPercent(ThisCommand, CommandBuf, TRUE); } }
          else
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          break; }

        case 'F': { //%F - General find
          while ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
            if (CheckToken("STARTLINE", Token, 6, QualLen, ThisCommand, F_startline)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%F -startline=<n> ");
                break; }
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else if (CheckToken("STARTCHR", Token, 6, QualLen, ThisCommand, F_startchr)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%F -startchr=<n> ");
                break; }
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else if (CheckToken("ENDLINE", Token, 4, QualLen, ThisCommand, F_endline)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%F -endline=<n> ");
                break; }
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else if (CheckToken("ENDCHR", Token, 4, QualLen, ThisCommand, F_endChr)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%F -endchr=<n> ");
                break; }
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else if (CheckToken("REX", Token, 1, QualLen, ThisCommand, F_rex))
              { }
            else if (CheckToken("SETAPERTURE", Token, 2, QualLen, ThisCommand, F_setaperture))
              { }
            else if (CheckToken("APERTURE", Token, 1, QualLen, ThisCommand, F_aperture))
              { }
            else if (CheckToken("RESTART", Token, 3, QualLen, ThisCommand, F_restart))
              { }
            else if (CheckToken("ANY", Token, 2, QualLen, ThisCommand, F_any))
              { }
            else if (CheckToken("ALL", Token, 2, QualLen, ThisCommand, F_all))
              { }
            else if (CheckToken("FIRST", Token, 2, QualLen, ThisCommand, F_first))
              { }
            else if (CheckToken("BACK", Token, 1, QualLen, ThisCommand, F_back))
              { }
            else if (CheckToken("REVERSE", Token, 1, QualLen, ThisCommand, F_reverse))
              { }
            else if (CheckToken("TAB", Token, 1, QualLen, ThisCommand, F_tab))
              AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
            else if (CheckToken("SUBSTITUTE", Token, 2, QualLen, ThisCommand, F_substitute))
              AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
            else
              LocalFail = SynError(CommandBuf, "Invalid %%F qualifier ( -%s )", Token); }
          if (LocalFail)
            break;
          AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          break; }

        case 'C':  //%C - Exit writing new file.
        case 'M':  //%M - Message.
        case 'X':  //%X - Exit via a run-time error message.
          AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          break;

        case 'E': { //%E - Execute following CLI command line.
          while ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
            if (CheckToken("PIPE", Token, 2, QualLen, ThisCommand, E_pipe))
              { }
            else if (CheckToken("INTERACTIVE", Token, 1, QualLen, ThisCommand, E_interactive))
              { }
            else if (CheckToken("EXIT", Token, 2, QualLen, ThisCommand, E_exit))
              AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
            else
              LocalFail = SynError(CommandBuf, "Invalid %%E qualifier ( -%s )", Token); }
          //Deal with legacy syntax ( | for -pipe and & for -interactive).
          if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '|') {
            CommandBuf->CurrentByte += 1;
             AddIntArg(ThisCommand, E_pipe); }
          else if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '&') {
            CommandBuf->CurrentByte += 1;
             AddIntArg(ThisCommand, E_interactive); }
          if (LocalFail)
            break;
          AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          break; }

        case 'P':  //%P - Pipe text to subprocess (pipe set up by %E with -interactive qualifier).
          while ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
            if (CheckToken("NOCR", Token, 1, QualLen, ThisCommand, P_nocr))
              { }
            else if (CheckToken("WAITFOR", Token, 1, QualLen, ThisCommand, P_waitfor)) {
              CommandBuf->CurrentByte += 1;
              AddStringArgPercent(ThisCommand, CommandBuf, FALSE); }
            else if (CheckToken("TIMEOUT", Token, 1, QualLen, ThisCommand, P_timeout)) {
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else
              LocalFail = SynError(CommandBuf, "Invalid %%P qualifier ( -%s )", Token); }
          if (LocalFail)
            break;
          AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          break;

        case 'W': { //%W - Set up a screen window.
          int Modify = 0;
          QualLen = CopyToken(Token, CommandBuf, FALSE);
          if (CheckToken("NEW", Token, 1, QualLen, ThisCommand, W_new))
            { }
          else if (CheckToken("MODIFY", Token, 1, QualLen, ThisCommand, W_modify)) {
            Modify = 1;
            if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '=') {
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else {
              SynError(CommandBuf, "Missing or invalid value to %%W= -winno=<n>; command.");
              break; } }
          else if (CheckToken("REFRESH", Token, 1, QualLen, ThisCommand, W_refresh))
            { }
          else if (CheckToken("CLEAR", Token, 1, QualLen, ThisCommand, W_clear))
            { }
          else
            LocalFail = SynError(CommandBuf, "Unrecognized %%W command %S", Token);
          while ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
            if (CheckToken("HEIGHT", Token, 1, QualLen, ThisCommand, W_heightQual)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '=') {
                CommandBuf->CurrentByte += 1;
                AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
              else {
                SynError(CommandBuf, "Missing or invalid value to %%W= -height=<n>; command.");
                break; }
              continue; }
            else if (CheckToken("WIDTH", Token, 3, QualLen, ThisCommand, W_widthQual)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '=') {
                CommandBuf->CurrentByte += 1;
                AddIntArgFromText(ThisCommand, CommandBuf, 10, " +;");
                if ((CommandBuf->CurrentRec->text+CommandBuf->CurrentByte)[0] == '+') {
                  CommandBuf->CurrentByte++;
                  AddIntArg(ThisCommand, W_guardQual);
                  AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); } }
              else {
                SynError(CommandBuf, "Missing or invalid value to %%W= -width=<n>; command.");
                break; }
              continue; }
            else if (CheckToken("POPUP", Token, 1, QualLen, ThisCommand, W_popupQual)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '=') {
                CommandBuf->CurrentByte += 1;
                AddIntArgFromText(ThisCommand, CommandBuf, 10, " ,;");
                if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == ',') {
                  CommandBuf->CurrentByte += 1;
                  AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
                else 
                  AddIntArg(ThisCommand, 0); }
              else {
                AddIntArg(ThisCommand, 0);
                AddIntArg(ThisCommand, 0); }
              continue; }
            else if (CheckToken("KEY", Token, 1, QualLen, ThisCommand, W_keyQual)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '=') {
                CommandBuf->CurrentByte += 1;
                AddStringArgPercent(ThisCommand, CommandBuf, FALSE); }
              else {
                SynError(CommandBuf, "Missing or invalid value to %%W= -winno=<n>; command.");
                break; }
              continue; }
            else if (CheckToken("FREEZE", Token, 1, QualLen, ThisCommand, W_freezeQual)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '=') {
                CommandBuf->CurrentByte += 1;
                AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
              else {
                SynError(CommandBuf, "Missing or invalid value to %%W= -freeze=<n>; command.");
                break; }
              continue; }
            else if (CheckToken("DELETE", Token, 4, QualLen, ThisCommand, W_deleteQual))
              { }
            else if (CheckToken("INSERT", Token, 1, QualLen, ThisCommand, W_insertQual)) {
              if (Modify) {
                SynError(CommandBuf, "The -insert option cannot be used with %%W=modify");
                break; }
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '=') {
                CommandBuf->CurrentByte += 1;
                AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
              else {
                SynError(CommandBuf, "Missing or invalid value to %%W= -insert=<n>; command.");
                break; }
              continue; }
            else if (CheckToken("FREEZE", Token, 1, QualLen, ThisCommand, W_freezeQual))
              { }
            else if (CheckToken("DELIM", Token, 1, QualLen, ThisCommand, W_delimQual))
              { }
            else 
              SynError(CommandBuf, "Invalid qualifier to %%W "); }
        break; }

        case 'I': { //%I - Secondary input file.
          while ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
            if (CheckToken("SECTION", Token, 2, QualLen, ThisCommand, I_section)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%I= -section=<name> ");
                break; }
              CommandBuf->CurrentByte += 1;
              AddStringArgPercent(ThisCommand, CommandBuf, FALSE); }
            else if (CheckToken("FSECTION", Token, 1, QualLen, ThisCommand, I_fsection)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%I= -fsection=<name> ");
                break; }
              CommandBuf->CurrentByte += 1;
              AddStringArgPercent(ThisCommand, CommandBuf, FALSE); }
            else if (CheckToken("SEEK", Token, 1, QualLen, ThisCommand, I_seek)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%I= -seek=<n> ");
                break; }
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else if (CheckToken("BYTES", Token, 1, QualLen, ThisCommand, I_bytes)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%I= -bytes=<n> ");
                break; }
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else if (CheckToken("BLOCK", Token, 1, QualLen, ThisCommand, I_block)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%I= -block=<n>");
                break; }
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else if (CheckToken("RECORDS", Token, 1, QualLen, ThisCommand, I_records)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%I= -records=<n>");
                break; }
              CommandBuf->CurrentByte += 1;
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            else if (CheckToken("BINARY", Token, 1, QualLen, ThisCommand, I_binary)) {
              if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] == '=') {
                CommandBuf->CurrentByte += 1;
                if ( ! AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"))
                  SynError(CommandBuf, "Missing or invalid value to %%I= -binary=<n>;"); }
              else
                AddIntArg(ThisCommand, 16); }
            else if (CheckToken("INSERT", Token, 1, QualLen, ThisCommand, I_insert))
              { }
            else if (CheckToken("APPEND", Token, 1, QualLen, ThisCommand, I_append))
              { }
            else if (CheckToken("HOLD", Token, 1, QualLen, ThisCommand, I_hold))
              { }
            else 
              LocalFail = SynError(CommandBuf, "Invalid %%I qualifier ( -%s )", Token); }
          if (LocalFail)
            break;
          AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          break; }

        case 'R': {  //%R - Run a command file.
          int Qual = 0;
          if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
            if ( ! CheckToken("ASCONSOLE", Token, 1, QualLen, ThisCommand, R_asconsole)) {
              LocalFail = SynError(CommandBuf, "Invalid qualifier to %%R command.");
              break; } }
          else
            AddIntArg(ThisCommand, Qual);
          if (LocalFail)
            break;
          AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
          AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          break; }

        case 'O': { //%O - Output current buffer as specified file.
          while ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
            if (CheckToken("APPEND", Token, 1, QualLen, ThisCommand, O_append))
              { }
            else if (CheckToken("BINARY", Token, 1, QualLen, ThisCommand, O_binary)) {
              if ((CommandBuf->CurrentRec->text+CommandBuf->CurrentByte)[0] == '=') {
                CommandBuf->CurrentByte += 1;
                AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
              else
                AddIntArg(ThisCommand, 16); }
            else
              LocalFail = SynError(CommandBuf, "Invalid qualifier to %%O command."); }
          if (LocalFail)
            break;
          AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
          break; }

        case 'B': { //%B - Set buffer attributes.
          QualLen = CopyToken(Token, CommandBuf, FALSE);
          if (CheckToken("READONLY", Token, 2, QualLen, ThisCommand, B_readonly)) {
            if ( ! AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;") )
              AddIntArg(ThisCommand, 1);
            break; }
          else if (CheckToken("WRITEIFCHANGED", Token, 2, QualLen, ThisCommand, B_writeifchanged)) {
            if ( ! AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;") )
              AddIntArg(ThisCommand, 1);
            break; }
          else if (CheckToken("SAMEFLAG1", Token, 2,QualLen, ThisCommand, B_sameflag1)) {
            if ( ! AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;") )
              AddIntArg(ThisCommand, 1);
            break; }
          else if (CheckToken("CODEPAGE", Token, 2, QualLen, ThisCommand, B_codepage)) {
            if ( ! AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;") )
              AddIntArg(ThisCommand, 65001);
            break; }
          else if (CheckToken("HEADER", Token, 2, QualLen, ThisCommand, B_header)) {
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
            break; }
          else if (CheckToken("FOOTER", Token, 2, QualLen, ThisCommand, B_footer)) {
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
            break; }
          else if (CheckToken("LEFTOFFSET", Token, 2, QualLen, ThisCommand, B_leftoffset)) {
            if ( ! AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;") )
              AddIntArg(ThisCommand, 0);
            break; }
          else if (CheckToken("TABSTOPS", Token, 4, QualLen, ThisCommand, B_tabstops)) {
            while (AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"))
              { };
            break; }
          else if (CheckToken("TABCELLS", Token, 4, QualLen, ThisCommand, B_tabcells)) {
            while (AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"))
              { };
            break; }
          else if (CheckToken("SORT", Token, 2, QualLen, ThisCommand, B_sort)) {
            while ( AddIntArgFromText(ThisCommand, CommandBuf, 10, " +-;") ) {
              int Delim = CommandBuf->CurrentRec->text[CommandBuf->CurrentByte++];
              if (Delim == '+')
                AddIntArg(ThisCommand, B_sortPlusQual);
              else if (Delim == '-')
                AddIntArg(ThisCommand, B_sortMinusQual);
              AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;"); }
            break; }
          else if (CheckToken("TABSORT", Token, 2, QualLen, ThisCommand, B_tabsort)) {
            while ( AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;") )
              { } }
          else if (CheckToken("TAGTYPE", Token, 3, QualLen, ThisCommand, B_tagtype)) {
            QualLen = CopyToken(Token, CommandBuf, TRUE);
            if (CheckToken("COLOUR", Token, 1, QualLen, ThisCommand, B_colourQual) || CheckToken("COLOR", Token, 1, QualLen, ThisCommand, B_colourQual)) {
              int Chr, Pass = 1;
              if ((CommandBuf->CurrentRec->text+CommandBuf->CurrentByte)[0] != '=') {
                SynError(CommandBuf, "Missing or invalid value to %%B=tagtype -colour=<fg>:<bg> <name>; command.");
                break; }
              CommandBuf->CurrentByte += 1;
              for ( ; ; ) { //A two-pass loop, foreground first then background colour.
                Chr = StealCharacter(CommandBuf);
                if ('0' <= Chr && Chr <= '9') //Colour pair defined numerically.
                  AddIntArgFromText(ThisCommand, CommandBuf, 10, " :");
                else { //Colour pair defined symbolically.
                  int ColourLen = CopyToken(Token, CommandBuf, FALSE);
                  if (CheckToken("BLACK", Token, 3, ColourLen, ThisCommand, B_colourBlack))
                    { }
                  else if (CheckToken("WHITE", Token, 1, ColourLen, ThisCommand, B_colourWhite))
                    { }
                  else if (CheckToken("RED", Token, 1, ColourLen, ThisCommand, B_colourRed))
                    { }
                  else if (CheckToken("BLUE", Token, 3, ColourLen, ThisCommand, B_colourBlue))
                    { }
                  else if (CheckToken("GREEN", Token, 1, ColourLen, ThisCommand, B_colourGreen))
                    { }
                  else if (CheckToken("YELLOW", Token, 1, ColourLen, ThisCommand, B_colourYellow))
                    { }
                  else if (CheckToken("MAGENTA", Token, 1, ColourLen, ThisCommand, B_colourMagenta))
                    { }
                  else if (CheckToken("CYAN", Token, 1, ColourLen, ThisCommand, B_colourCyan))
                    { }
                  else
                    SynError(CommandBuf, "Unrecognized colour \"%s\".", Token); }
                if (2 <= Pass++)
                  break;
                if ((CommandBuf->CurrentRec->text+CommandBuf->CurrentByte)[0] != ':') {
                  SynError(CommandBuf, "Missing or invalid value to %%B=tagtype -colour=<fg>:<bg> <name>; command.");
                  break; }
                else
                  (CommandBuf->CurrentByte)++; }
              AddStringArgPercent(ThisCommand, CommandBuf, FALSE); }
            else
              SynError(CommandBuf, "Error in colour-pair specification in tagtype definition.");
            break; }
          else if (CheckToken("ENCODING", Token, 2, QualLen, ThisCommand, B_encoding)) {
            AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
            break; }
          else if (CheckToken("ADDTAG", Token, 2, QualLen, ThisCommand, B_addtag)) {
            if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
              if (CheckToken("TEXT", Token, 1, QualLen, ThisCommand, B_textQual)) {
                if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                  SynError(CommandBuf, "Missing or invalid value to %%B=addtag -text=<textString>; command.");
                  break; }
                CommandBuf->CurrentByte += 1;
                AddStringArgPercent(ThisCommand, CommandBuf, TRUE); }
              else
                SynError(CommandBuf, "Invalid qualifier to %%B=addtag"); }
            else {
              AddIntArg(ThisCommand, B_notTextQual);
              AddStringArgPercent(ThisCommand, CommandBuf, FALSE); }
            break; }
          else if (CheckToken("REMOVE_TAG", Token, 2, QualLen, ThisCommand, B_remove_tag)) {
            if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
              if (CheckToken("COLOUR", Token, 1, QualLen, ThisCommand, B_colourQual)) {
                AddStringArgPercent(ThisCommand, CommandBuf, FALSE); }
              else if (CheckToken("TEXT", Token, 2, QualLen, ThisCommand, B_textQual)) {
                if ((CommandBuf->CurrentRec->text + CommandBuf->CurrentByte)[0] != '=') {
                  SynError(CommandBuf, "Missing or invalid value to %%B=addtag -text=<textString>; command.");
                  break; }
                CommandBuf->CurrentByte += 1;
                AddStringArgPercent(ThisCommand, CommandBuf, TRUE); }
              else if (CheckToken("TARGET", Token, 2, QualLen, ThisCommand, B_targetQual)) {
                AddStringArgPercent(ThisCommand, CommandBuf, FALSE); }
              else {
                SynError(CommandBuf, "Syntax error in remove_tag command");
                break; } }
            break; }
          else if (CheckToken("UNICODE", Token, 3, QualLen, ThisCommand, B_unicode)) {
            AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;");
            break; }
          else if (CheckToken("PATHNAME", Token, 2, QualLen, ThisCommand, B_pathname)) {
            AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
            break; }
          else if (CheckToken("UNRESTRICTED", Token, 3, QualLen, ThisCommand, B_unrestricted)) {
            break; }
          break; }

        case 'S': { //%S - System settings 
          QualLen = CopyToken(Token, CommandBuf, FALSE);
          if (CheckToken("CASE", Token, 2, QualLen, ThisCommand, S_case)) //%s=case {0|1} -  controls case sensitivity for F command.
            AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;");
          else if ( (CheckToken("COMMANDCOUNTER", Token, 8, QualLen, ThisCommand, S_commandcounter)) || (CheckToken("CC", Token, 2, QualLen, ThisCommand, S_commandcounter)) )
            //%s=commandcounter <n> - sets the debug counter to some positive value.
            AddIntArgFromText(ThisCommand, CommandBuf, 10, ";");
          else if (CheckToken("CONSOLE", Token, 3, QualLen, ThisCommand, S_console)) //%s=console <n> - Set the overall console-area size.
            AddIntArgFromText(ThisCommand, CommandBuf, 10, ";");
          else if (CheckToken("GUARDBAND", Token, 1, QualLen, ThisCommand, S_guardband)) //%s=guardband <n> - Set window guardband.
            AddIntArgFromText(ThisCommand, CommandBuf, 10, ";");
          else if (CheckToken("SLEEP", Token, 2, QualLen, ThisCommand, S_sleep)) //%s=sleep <n> - sleep for that many microseconds.
            AddIntArgFromText(ThisCommand, CommandBuf, 10, ";");
          else if (CheckToken("RECOVERYMODE", Token, 1, QualLen, ThisCommand, S_recoverymode)) //%s=recoverymode <n> -  disables %O command, %I prompts and reads filename from console (the recovery script).
            AddIntArgFromText(ThisCommand, CommandBuf, 10, ";");
          else if (CheckToken("SYSTEM", Token, 1, QualLen, ThisCommand, S_system)) //%s=system {0|1} - preserves current value of default strings.
            AddIntArgFromText(ThisCommand, CommandBuf, 10, ";");
          else if (CheckToken("TRACESKIP", Token, 6, QualLen, ThisCommand, S_traceskip)) //%s=traceskip - skip-over blocks, macros, functions, scripts etc.
            AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;");
            
          else if ( (CheckToken("COMMANDSTRING", Token, 8, QualLen, ThisCommand, S_commandstring)) || (CheckToken("CS", Token, 2, QualLen, ThisCommand, S_commandstring)) )
            //%s=commandstring <string> - return modified string back to command-line input.
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          else if (CheckToken("ON_KEY", Token, 4, QualLen, ThisCommand, S_on_key)) { //%s=on_key <CommandString> - specifies a command string to be executed after a command
            if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
              if ( ! (CheckToken("AFTER", Token, 1, QualLen, ThisCommand, S_commandstring)) ) {
                SynError(CommandBuf, "Invalid qualifier to %s=On_Key command.");
                break; } }
            else
              AddIntArg(ThisCommand, 0);
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE); }
          else if (CheckToken("ON_INT", Token, 4, QualLen, ThisCommand, S_on_int))
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          else if (CheckToken("PROMPT", Token, 1, QualLen, ThisCommand, S_prompt)) //%s=prompt <string> - sets the prompt string used by the G and OG commands.
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
            
          else if (CheckToken("SETENV", Token, 4, QualLen, ThisCommand, S_setenv)) { //%s=setenv <name> <value> - set env variable for the session.
            AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE); }
            
          else if (CheckToken("TAB", Token, 2, QualLen, ThisCommand, S_tab)) //%s=tab <n> - table-column separator character.
            AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
            
          else if (CheckToken("SETMOUSE", Token, 4, QualLen, ThisCommand, S_setmouse)) //%s=setmouse <value> - sets the mouse coordinates returned by OP and %q=window commands.
            { }
              
          else {  //The following take hex bit patterns which may eithr replace, set specified bit or reset specified bits.
            int ModifierChr;
            while(CommandBuf->CurrentRec->text[CommandBuf->CurrentByte] == ' ')
              CommandBuf->CurrentByte++;
            ModifierChr = CommandBuf->CurrentRec->text[CommandBuf->CurrentByte];
            if (ModifierChr == '+' || ModifierChr == '-')
              CommandBuf->CurrentByte++;
            else
              ModifierChr = '\0';
            
            if (CheckToken("COMMANDMODE", Token, 8, QualLen, ThisCommand, S_commandmode) || CheckToken("CM", Token, 2, QualLen, ThisCommand, S_commandmode)) //%s=commandmode - controls command/type-into-screen mode.
              AddIntArgFromText(ThisCommand, CommandBuf, 16, ";");
            else if (CheckToken("MOUSEMASK", Token, 1, QualLen, ThisCommand, S_mousemask)) //%s=mousemask <x> - Define the mouse mask
              AddIntArgFromText(ThisCommand, CommandBuf, 16, ";");
            else if (CheckToken("TRACE", Token, 2, QualLen, ThisCommand, S_trace)) //%s=trace <x> - set Trace mode. Prefix argument with '+' to XOR with current value.
              AddIntArgFromText(ThisCommand, CommandBuf, 16, ";");
            else if (CheckToken("TRACEDEFAULT", Token, 6, QualLen, ThisCommand, S_tracedefault)) //%s=tracedefsult <x> - Set default trace vector set by t command.
              AddIntArgFromText(ThisCommand, CommandBuf, 16, ";");
            else if (CheckToken("VERBOSE", Token, 1, QualLen, ThisCommand, S_verbose)) //%s=verbose <n> - Set/reset verbose mode.
              AddIntArgFromText(ThisCommand, CommandBuf, 16, ";");
              
            AddIntArg(ThisCommand, ModifierChr); }
          break; }

        case 'H': { //%H - Hashtable maintenance
          
          QualLen = CopyToken(Token, CommandBuf, FALSE);
          if (CheckToken("CALL", Token, 2, QualLen, ThisCommand, H_call)) {
            if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
              if (CheckToken("ONELINE", Token, 1, QualLen, ThisCommand, H_onelineQual))
                { }
              else {
                SynError(CommandBuf, "Invalid qualifier to %%H=call");
                break; } }
            else
              AddIntArg(ThisCommand, 0);
            //%h=call is a special case - it adds the functions's pathname followed by the function's arguments.
            AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
            break; }
          //All other cases only add the pathname.
          else if (CheckToken("CODE", Token, 2, QualLen, ThisCommand, H_code))
            { }
          else if (CheckToken("CREATE", Token, 2, QualLen, ThisCommand, H_create)) {
            if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
              if (CheckToken("DESTROY", Token, 3, QualLen, ThisCommand, H_destroyQual))
                { }
              else if (CheckToken("PROTECT", Token, 1, QualLen, ThisCommand, H_protectQual))
                { }
              else if (CheckToken("ADJUST", Token, 1, QualLen, ThisCommand, H_adjustQual))
                { }
              else if (CheckToken("DELETE", Token, 1, QualLen, ThisCommand, H_deleteQual))
                { }
              else {
                SynError(CommandBuf, "Invalid qualifier to %%H=create");
                break; } }
            else {
              AddIntArg(ThisCommand, H_adjustQual); }
            AddIntArgFromText(ThisCommand, CommandBuf, 10, " ;");
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
            break; }
          else if (CheckToken("DATA", Token, 2, QualLen, ThisCommand, H_data))
            { }
          else if (CheckToken("SETDATA", Token, 4, QualLen, ThisCommand, H_setdata)) {
            if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
              if (CheckToken("NEW", Token, 1, QualLen, ThisCommand, H_newQual))
                { }
              else {
                SynError(CommandBuf, "Invalid qualifier to %%H=create");
                break; } } }
          else if (CheckToken("GETDATA", Token, 1, QualLen, ThisCommand, H_getdata))
            { }
          else if (CheckToken("DELETE", Token, 2, QualLen, ThisCommand, H_delete))
            { }
          else if (CheckToken("DESTROY", Token, 1, QualLen, ThisCommand, H_destroy)) { 
            if ( (QualLen = CopyToken(Token, CommandBuf, FALSE)) ) {
              if (CheckToken("ALL", Token, 1, QualLen, ThisCommand, H_allQual))
                AddIntArg(ThisCommand, H_allQual); }
            else
              AddIntArg(ThisCommand, 0);
            AddStringArgPercent(ThisCommand, CommandBuf, FALSE);
            break; }
          else if (CheckToken("FIX", Token, 1, QualLen, ThisCommand, H_fix)) {
            break; }
          else if (CheckToken("NEWJUMP", Token, 1, QualLen, ThisCommand, H_newjump))
            { }
          else if (CheckToken("ADDJUMP", Token, 1, QualLen, ThisCommand, H_addjump))
            { }
          else if (CheckToken("SETSECT", Token, 4, QualLen, ThisCommand, H_setsect))
            { }
          else if (CheckToken("SETFSECT", Token, 4, QualLen, ThisCommand, H_setfsect))
            { }
          else if (CheckToken("JUMP", Token, 1, QualLen, ThisCommand, H_jump))
            { }
          else if (CheckToken("TESTKEY", Token, 1, QualLen, ThisCommand, H_testkey)) {
            if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
              if (CheckToken("TYPE", Token, 1, QualLen, ThisCommand, H_typeQual))
                { }
              else {
                SynError(CommandBuf, "Invalid qualifier to %%H=testkey");
                break; } }
            else
              AddIntArg(ThisCommand, 0); }
          AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          break; }

        case 'Q': { //%Q - system Query.
          int FirstCurrentByte = CommandBuf->CurrentByte;
          int Qual = 0;
          if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
            if ( ! CheckToken("APPEND", Token, 1, QualLen, ThisCommand, Q_appendQual)) {
              SynError(CommandBuf, "Invalid qualifier to %%Q query.");
              break; } }
          else
            AddIntArg(ThisCommand, Qual);
          
          QualLen = CopyToken(Token, CommandBuf, FALSE);
          
          if (CheckToken("BACKTRACE", Token, 2, QualLen, ThisCommand, Q_backtrace)) //Writes diagnostic backtrace to nominated buffer.
                { }
          else if (CheckToken("BUFFER", Token, 1, QualLen, ThisCommand, Q_buffer)) //buffer - returns info on buffer.
                { }
          else if (CheckToken("CASE", Token, 1, QualLen, ThisCommand, Q_case)) //case - fails if not case sensitive.
                { }
          else if (CheckToken("CPUTIME", Token, 2, QualLen, ThisCommand, Q_cputime)) //Pushes time counted since system startup time.
                { }
          else if (CheckToken("DATE", Token, 2, QualLen, ThisCommand, Q_date)) //date - return todays date only.
                { }
          else if (CheckToken("DIR", Token, 2, QualLen, ThisCommand, Q_dir)) { //dir <path> - return directory contents.
            QualLen = 0;
            
            while (TRUE) {
              if ( ! (QualLen = CopyToken(Token, CommandBuf, TRUE)) )
                break; 
              if (CheckToken("ATIME", Token, 1, QualLen, ThisCommand, Q_atimeQual))
                { }
              else if (CheckToken("CTIME", Token, 1, QualLen, ThisCommand, Q_ctimeQual))
                { }
              else if (CheckToken("GID", Token, 1, QualLen, ThisCommand, Q_gidQual))
                { }
              else if (CheckToken("INODE", Token, 1, QualLen, ThisCommand, Q_inodeQual))
                { }
              else if (CheckToken("MTIME", Token, 2, QualLen, ThisCommand, Q_mtimeQual))
                { }
              else if (CheckToken("MODE", Token, 2, QualLen, ThisCommand, Q_modeQual))
                { }
              else if (CheckToken("SIZE", Token, 1, QualLen, ThisCommand, Q_sizeQual))
                { }
              else if (CheckToken("UID", Token, 1, QualLen, ThisCommand, Q_uidQual))
                { }
              else {
                SynError(CommandBuf, "Syntax error in directory query.");
                break; } }
            
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
            break; }
          else if (CheckToken("ENV", Token, 1, QualLen, ThisCommand, Q_env)) {
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
            break; }
          else if (CheckToken("FILE", Token, 1, QualLen, ThisCommand, Q_file)) {
            if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
              if ( ! CheckToken("NOFOLLOWLINKS", Token, 1, QualLen, ThisCommand, Q_nofollowlinksQual)) {
                SynError(CommandBuf, "Invalid qualifier to query dir command.");
                break; } }
              else
                AddIntArg(ThisCommand, 0);
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
            break; }
          else if (CheckToken("HEAP", Token, 2, QualLen, ThisCommand, Q_heap))
            { }
          else if (CheckToken("HISTORY", Token, 1, QualLen, ThisCommand, Q_history))
            { }
          else if (CheckToken("INVIEW", Token, 1, QualLen, ThisCommand, Q_inview))
            { }
          else if (CheckToken("KEYS", Token, 1, QualLen, ThisCommand, Q_keys)) {
            if ( (QualLen = CopyToken(Token, CommandBuf, TRUE)) ) {
              if (CheckToken("KEY", Token, 1, QualLen, ThisCommand, Q_keyQual)) {
                CommandBuf->CurrentByte++;
                AddStringArgPercent(ThisCommand, CommandBuf, FALSE); }
              else
                LocalFail = SynError(CommandBuf, "Invalid %%Q=keys qualifier ( -%s )", Token); }
            AddStringArgPercent(ThisCommand, CommandBuf, TRUE); }
          else if (CheckToken("LINUX", Token, 1, QualLen, ThisCommand, Q_linux))
            { }
          else if (CheckToken("PID", Token, 1, QualLen, ThisCommand, Q_pid))
            { }
          else if (CheckToken("SYSTEM", Token, 2, QualLen, ThisCommand, Q_system))
            { }
          else if (CheckToken("SAMESINCEIO", Token, 11, QualLen, ThisCommand, Q_samesinceio))
            { }
          else if (CheckToken("SAMESINCEINDEXED", Token, 11, QualLen, ThisCommand, Q_samesinceindexed))
            { }
          else if (CheckToken("SAMESINCECOMPILED", Token, 10, QualLen, ThisCommand, Q_samesincecompiled))
            { }
          else if (CheckToken("SAMEFLAG1", Token, 5, QualLen, ThisCommand, Q_sameflag1))
            { }
          else if (CheckToken("STACK", Token, 2, QualLen, ThisCommand, Q_stack))
            { }
          else if (CheckToken("TABSTOPS", Token, 4, QualLen, ThisCommand, Q_tabstops))
            { }
          else if (CheckToken("TABCELLS", Token, 4, QualLen, ThisCommand, Q_tabcells))
            { }
          else if (CheckToken("TAGS", Token, 2, QualLen, ThisCommand, Q_tags)) {
            QualLen = CopyToken(Token, CommandBuf, TRUE);
            if ( ! CheckToken("HERE", Token, 1, QualLen, ThisCommand, Q_hereQual))
              AddIntArg(ThisCommand, 0); }
          else if (CheckToken("TIME", Token, 2, QualLen, ThisCommand, Q_time))
            { }
          else if (CheckToken("VERIFY", Token, 4, QualLen, ThisCommand, Q_verify))
            { }
          else if (CheckToken("VERSION", Token, 1, QualLen, ThisCommand, Q_version))
            { }
          else if (CheckToken("WINDOWS", Token, 7, QualLen, ThisCommand, Q_windows))
            { }
          else if (CheckToken("WD", Token, 2, QualLen, ThisCommand, Q_wd))
            { }
          else if (CheckToken("WINDOW", Token, 2, QualLen, ThisCommand, Q_window))
            { }
          CommandBuf->CurrentByte = FirstCurrentByte;
          AddStringArgPercent(ThisCommand, CommandBuf, TRUE);
          break; }

        default: { //Default case for percent-command block.
          CommandBuf->SubstringLength = 1;
          LocalFail = SynError(CommandBuf, "Unknown %% command"); }  //percent switch block ends.
          return; }
          
        while (CommandBuf->CurrentRec->text[CommandBuf->CurrentByte] == ' ')
          CommandBuf->CurrentByte++;
        if (CommandBuf->CurrentRec->text[CommandBuf->CurrentByte] == ';')
          CommandBuf->CurrentByte++;
        if (LocalFail) {
          if (ThisCommand)
            FreeCommand(&ThisCommand);
          return; }
        break; } //% command case ends.

    default: //Default case for main command switch.
      CommandBuf->CurrentByte--;
      if (s_JournalHand) {
        fputs("<Invalid command>\n", s_JournalHand);
        fflush(s_JournalHand); }
      SynError(CommandBuf, "Invalid command character.");
      return; } // Command case block ends.
  
    PrevCommand = ThisCommand;
    ThisCommand->ComChrs = (CommandBuf->CurrentByte)-(ThisCommand->CommandByteNo)-1; } }

#if defined(VC)
//---------------------------------------------memmem
void *memmem(const void *haystack, size_t haystack_len, const void * const needle, size_t needle_len) {
  //Windowsland replacement for memmem - adapted from https://stackoverflow.com/questions/52988769/writing-own-memmem-for-windows
  if (haystack == NULL || haystack_len == 0 || needle == NULL)
    return NULL;
  if ( ! needle_len) 
    return haystack;
  
  for (const char *h = haystack; needle_len < haystack_len; ++h, --haystack_len) {
    if ( ! memcmp(h, needle, needle_len))
      return h; }
      
  return NULL; }
#endif

//---------------------------------------------CopyToken
int CopyToken(char * Token, struct Buf * CommandBuf, int IsQualifier)
  { //Copies next Token (A-Z, 0-9 with a-z converted to upper case) from buffer to Token.
    //The IsQualifier arg tells it to expect a leading hyphen.
  int From = CommandBuf->CurrentByte, To = 0;
  while (CommandBuf->CurrentRec->text[From] == ' ')
    From += 1;
  if (IsQualifier && CommandBuf->CurrentRec->text[From++] != '-')
    return 0;
      
  while (TRUE) {
    Token[To] = toupper(CommandBuf->CurrentRec->text[From]);
    if ( (Token[To] == '_') || ( ('A' <= Token[To]) && (Token[To]  <= 'Z') ) || ( ('0' <= Token[To]) && (Token[To] <= '9') ) ) {
      To++;
      From++;
      continue; }
    else
      break; }
  Token[To] = '\0';
    
  if (To)
    CommandBuf->CurrentByte = From;
  return To; }

//---------------------------------------------TrimCommand
int TrimCommand(char * Token, char * Text)
  { //Copies command (A-Z, 0-9 with a-z converted to upper case) from Text to Token.
  int i;
  for (i=0; ; i++) {
    Token[i] = Text[i];
    if ( ('a' <= Token[i]) && (Token[i] <='z') )
      Token[i] -= 'a'-'A';
    if (Token[i] == '_') 
      continue;
    if ( ('A' <= Token[i]) && (Token[i]  <= 'Z') ) 
      continue;
    if ( ('0' <= Token[i]) && (Token[i] <= '9') )
      continue;
    Token[i] = '\0';
    break; }
    
  return i; }

//---------------------------------------------CheckToken
int CheckToken(char * Text, char * MatchString, int ChrsRequired, int ChrsGiven, struct Com * ThisCommand, int TokenKey)
  { //Just like strstr except that it also verifies that the MatchString is at the start of the Text.
  char * Match = strstr(Text, MatchString);
  if ( Match && (Match == Text) && (ChrsRequired <= ChrsGiven) ) {
    AddIntArg(ThisCommand, TokenKey);
    return TRUE; }
  else {
    ThisCommand->CommandBuf->SubstringLength = -strlen(MatchString);
    return FALSE; } }

//---------------------------------------------JOT_Block
struct Block *JOT_Block(struct Block *ThisBlock, struct Buf *CommandBuf, int StopOnEOL)
  { //Compiles a Block of JOT commands, terminated by a ')'. 
  ThisBlock->BlockSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
  JOT_Sequence(CommandBuf, ThisBlock->BlockSequence, StopOnEOL);
  if (s_CommandChr != ')') {
    SynError(CommandBuf, "Missing \')\'");
    return NULL; }
  ThisBlock->Repeats = GetArg(CommandBuf, 1);
  return ThisBlock;  }

//---------------------------------------------Run_Sequence
int Run_Sequence(struct Seq *Sequence)
  { //Executes a sequence of compiled commands, returns 0 on success, 1 if something fails.
  int LocalFail = 0;
  struct Com *NextCommand = NULL;
  struct Seq *ElseCommands = Sequence->ElseSequence;
  struct AnyArg *ThisArg;
  
  s_CurrentCommand = Sequence->FirstCommand;
  s_ExitBlocks = 0;
  
  for (;;) { // Command-key loop. 
    if (s_CurrentCommand) {
      s_CommandChr = s_CurrentCommand->CommandKey;
      ThisArg = s_CurrentCommand->ArgList;
      NextCommand = s_CurrentCommand->NextCommand; }
    else
      break;
    if ( ! ++s_CommandCounter) { //Reached terminal count.
      if (s_RecoveryMode) { //In recovery mode, execution is abruptly terminated.
        s_TraceMode = 0;
        LocalFail = RunError("Command sequence aborted by simulated Ctrl+C"); }
      else {
        Message(s_CommandBuf, "Reached specified command count");
        s_TraceMode = s_DefaultTraceMode; } }
    
    if (s_BombOut)
      break;
    if ( (s_TraceMode & Trace_CommandLines) && (s_CurrentCommand->CommandRec != s_PrevCommandRec) ) {
      s_PrevCommandRec = s_CurrentCommand->CommandRec;
      JotTrace("New command line."); }
    if ( (s_TraceMode & Trace_SubSequence) && (Sequence->FirstCommand == s_CurrentCommand) )
      JotTrace("New subsequence.");
    if (s_TraceMode & Trace_AllCommands)
      JotTrace("New command.");
    switch (s_CommandChr) {

    case '(': { //( - Block.
      struct Com *ParentCommand = s_CurrentCommand;
      LocalFail = Run_Block(FetchBlock((struct PtrArg **)&ThisArg));
      s_CurrentCommand = ParentCommand;
      if (s_ExitBlocks)
        return LocalFail;
      break; }

    case ')':  //) - End of current Block.
      break;

    case '\'': { //' - Macro call - deferred sequence block.
      char Key = FetchIntArgFunc(&ThisArg);
      struct Buf *CommandBuf = GetBuffer(Key, NeverNew);
      struct Seq *DeferredSeq;
      struct Com *ParentCommand = s_CurrentCommand;
      struct BacktraceFrame ThisBacktraceFrame;
      struct BacktraceFrame *ThisFrame;
      
      if (CommandBuf == NULL) {
        LocalFail = Fail("Undefined buffer");
        break; }
      if (CommandBuf->Predecessor) {
        LocalFail = RunError("Recursive use of macro in buffer %c", Key);
        break; }
      ResetBuffer(CommandBuf);
      CommandBuf->CurrentRec = CommandBuf->FirstRec;
      CommandBuf->CurrentByte = 0;
      CommandBuf->SubstringLength = 0;
      CommandBuf->Predecessor = s_CurrentCommand;
      DeferredSeq = (struct Seq *)JotMalloc(sizeof(struct Seq));
      
      JOT_Sequence(CommandBuf, DeferredSeq, FALSE);
      if (s_BombOut) { 
        struct Buf *TempBuf = GetBuffer(CommandBuf->BufferKey, NeverNew);
        if (TempBuf->FirstRec == CommandBuf->FirstRec)
          Message(CommandBuf, "Exiting macro %c, line no %d", Key, CommandBuf->LineNumber);
        else //The failing sequence changed the command buffer - this is allowable but the original command string is no longer available.
          Message(CommandBuf, "Exiting macro %c, line no %d (<original command-line text has been deleted>)", Key, CommandBuf->LineNumber); }
      s_BacktraceFrame->LastCommand = s_CurrentCommand;
      ThisBacktraceFrame.LastCommand = NULL;
      ThisBacktraceFrame.prev = s_BacktraceFrame;
      ThisBacktraceFrame.type = MacroFrame;
      s_BacktraceFrame = &ThisBacktraceFrame;
      CommandBuf->UnchangedStatus &= ~MacroRun;
      
      ThisFrame = s_BacktraceFrame;
      CommandBuf->FrameCount = 0;
      while (ThisFrame) { //Check loop - resets the Backtraced status of the buffer.
        CommandBuf->FrameCount++;
        ThisFrame = ThisFrame->prev; }
      
      s_TraceLevel++;
      LocalFail = Run_Sequence(DeferredSeq);
      s_TraceLevel--;
      s_CurrentCommand = ParentCommand;
      CommandBuf->UnchangedStatus |= MacroRun;
      if (LocalFail)
        Fail("Command-sequence failed.");
      if (CommandBuf == GetBuffer(Key, NeverNew)) //The command buffer may have been re-initialized as part of the command sequence - we allow this to happen.
        CommandBuf->Predecessor = NULL;
      FreeSequence(&DeferredSeq);
      if (LocalFail && (s_TraceMode & Trace_FailMacros) ) {
        s_BacktraceFrame->LastCommand = s_CurrentCommand;
        JotTrace("Macro failure."); }
      s_BacktraceFrame = ThisBacktraceFrame.prev;
      if (s_Return) {
        s_Return = FALSE;
        s_ExitBlocks = 0; }
      break; }

    case '\\': //\ - Reverse PASS/FAIL status of previous command.
    case '?':  //? - Set status of previous command to PASS.
      //These two are done immediately after the command - see the end of the main case block.
      break;

    case 'X': //X - eXit (or repeat) current (or specified) block.
      s_ExitBlocks = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      if (s_ExitBlocks == INT_MAX)
        s_Return = TRUE;
      break;

    case 'N':  //N - Note current Record for later abstract.
      s_AbstractWholeRecordFlag = ! FetchIntArgFunc((struct AnyArg **)&ThisArg);
      s_NoteLine = s_CurrentBuf->LineNumber;
      s_NoteBuffer = s_CurrentBuf;
      s_NoteByte = s_AbstractWholeRecordFlag ? 0 : s_CurrentBuf->CurrentByte;
      break;

    case 'A': { //A - Abstract using note record pointer.
      char BufferKey = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      char ModeQualifier = FetchIntArgFunc((struct AnyArg **)&ThisArg);  // NULL, +, - or .
      char FillQualifier = FetchIntArgFunc((struct AnyArg **)&ThisArg);  // &.
      struct Buf *DestBuf;
       
      if ( (BufferKey <= ' ') || (('z' < BufferKey) && BufferKey != '~')) {
        LocalFail = Fail("Invalid buffer key");
        break; }
      if (FillQualifier != '&') {
        if (s_CurrentBuf->EditLock & ReadOnly) {
          LocalFail = RunError("Attempt to modify a readonly buffer");
          break; }
        s_CurrentBuf->UnchangedStatus = 0; }
        
      if ( ! s_NoteBuffer || (s_NoteBuffer != s_CurrentBuf) ) {
        LocalFail = Fail("The note point is in a different buffer");
        break; }
      s_NoteBuffer = NULL;
      if (s_CurrentBuf->BufferKey == BufferKey) {
        LocalFail = Fail("Abstract source and destination is the same buffer");
        break; }
      DestBuf = GetBuffer(BufferKey, ModeQualifier ? OptionallyNew : AlwaysNew);
      if (DestBuf == NULL) {
        LocalFail = Fail("Invalid buffer");
        break; }
         
      if ( ! DestBuf->FirstRec) //A newly-minted buffer with no records.
        GetRecord(DestBuf, 1, "");
      DestBuf->UnchangedStatus = 0;
       
      switch (ModeQualifier) {;
        int i;
        case '+': //Append
          AdvanceRecord(DestBuf, INT_MAX);
          DestBuf->CurrentByte = 0;
          for (i = DestBuf->CurrentRec->MaxLength-1; 0 <= i; i--)
            if (DestBuf->CurrentRec->text[i]) {
              DestBuf->CurrentByte = i+1;
              break; }
          break;
        
        case '-': //Prepend
          DestBuf->CurrentRec = DestBuf->FirstRec;
          DestBuf->LineNumber = 1;
          DestBuf->CurrentByte = 0;
          DestBuf->SubstringLength = 0;
          break;
        
        case '.': //Insert
          break;
          
        case '\0':
          break;
          
        default: //Unrecognized qualifier character.
          LocalFail = RunError("Invalid qualifier to abstract command - must be one of +, - or .");
          break; }
      
      if (s_AbstractWholeRecordFlag) {
        s_CurrentBuf->CurrentByte = 0;
        s_CurrentBuf->SubstringLength = 0;
        if (s_NoteLine < s_CurrentBuf->LineNumber) {
          LocalFail = AdvanceRecord(s_CurrentBuf, 1); }
        else 
          s_NoteLine++; }
      if (LocalFail)
        break;
        
      LocalFail |= Abstract(DestBuf, s_CurrentBuf, FillQualifier);    
      DestBuf->AbstractWholeRecordFlag = s_AbstractWholeRecordFlag;
      break; }

    case 'Q': { //Q - Qualify.
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      char *StringArgument = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_QualifyString);
      char *RecordText = s_CurrentBuf->CurrentRec->text;
      int CurrentByte = s_CurrentBuf->CurrentByte;
      int ChrFirstByte = CurrentByte;
      char Chr;
      int Length, w_Length, Index;
      
      if ( ! StringArgument || mblen(StringArgument, MB_CUR_MAX) < 0) {
        LocalFail = Fail("Qualify arg missing or contains corrupt unicode.");
        break; }
      
      s_CurrentBuf->SubstringLength = 0;
      LocalFail = 1;
      //Extract match character, set to 0 if already at end stop. 
      if (Direction < 0)
        Chr = (CurrentByte == 0) ? '\0' : RecordText[CurrentByte-1];
      else
        Chr = RecordText[CurrentByte];
      if (Direction < 0)
        ChrFirstByte = s_CurrentBuf->CurrentByte + JotRecLenChrs(s_CurrentBuf, -1);
      if (Chr == '\0') 
        break;
      else if ( ((signed char)Chr < 0) && ! (s_CurrentBuf->NoUnicode) ) { //Assume we're matching a unicode character.
        int BytesConverted;
#ifdef VC
        wchar_t w_Chr, w_StringArgument[StringMaxChr];
        Length = strlen(StringArgument);
        MultiByteToWideChar(s_CodePage, MB_ERR_INVALID_CHARS, StringArgument, Length, w_StringArgument, StringMaxChr-1);
        BytesConverted = MultiByteToWideChar(s_CodePage, MB_ERR_INVALID_CHARS, s_CurrentBuf->CurrentRec->text + ChrFirstByte, -1, &w_Chr, 1);
        if (BytesConverted < 0) {
          LocalFail = Fail("Error detected while converting the current character to wide-character format for qualify.");
          break; }
#else
        Length = JotStrlenBytes(StringArgument, strlen(StringArgument));
        wchar_t w_Chr, w_StringArgument[Length+1];
        if (mbstowcs(w_StringArgument, StringArgument, Length+1) == (size_t)-1) {
          LocalFail = Fail("Error converting multi-byte to wide string");
          break; }
        BytesConverted = mbstowcs(&w_Chr, s_CurrentBuf->CurrentRec->text + ChrFirstByte, 1);
        if (BytesConverted <= 0) {
          LocalFail = Fail("Error detected while converting the current character to wide-character format for qualify.");
          break; }
#endif
        w_Length = wcslen(w_StringArgument);
          
        for (Index = 0; Index < w_Length; Index +=1) { // Search loop. 
          if (w_StringArgument[Index] == '-' && (0 < Index && Index < w_Length-1) ) { // A range of characters.
            if ((w_StringArgument[Index-1] <= w_Chr) && (w_Chr <= w_StringArgument[Index+1])) { // Chr in range. 
              LocalFail = 0;
              break; } }
          else if (w_StringArgument[Index] == w_Chr) { // Chr match. 
            if (w_Chr == '-' && (Index != 0 && Index != w_Length-1) ) //It's a character-range hyphen.
              continue;
            LocalFail = 0;
            break; } }
          if (LocalFail)
            Fail("Unicode character qualify failed."); }
      else { //Plain old ASCII match.
        Length = (StringArgument == NULL) ? 0 : strlen(StringArgument);
        for (Index = 0; Index < Length; Index +=1) { // Search loop. 
          if (StringArgument[Index] == '-' && (0 < Index && Index < Length-1) ) { // A range of characters.
            if (((signed char)StringArgument[Index-1] <= Chr) && (Chr <= (signed char)StringArgument[Index+1])) { // Chr in range. 
              LocalFail = 0;
              break; } }
          else if (StringArgument[Index] == Chr) { // Chr match. 
            if (Chr == '-' && (Index != 0 && Index != Length-1) ) //It's a character-range hyphen.
              continue;
            LocalFail = 0;
            break; } }
        if (LocalFail)
          Fail("ASCII character qualify failed."); }
      break; }

    case 'V': { //V - Verify text string.
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      char *StringArgument = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_SystemMode ? NULL : s_FindString);
      int InitialPointer = s_CurrentBuf->CurrentByte;
      
      s_CurrentBuf->SubstringLength = 0; 
      if (0 < Direction) { // Forward verify.
        if (SearchRecord(s_CurrentBuf, StringArgument, 1) )
          LocalFail = ((s_CurrentBuf->CurrentByte) != InitialPointer);
        else
          LocalFail = 1; }
      else { // Backwards verify. 
        int StringArgLength = strlen(StringArgument);
        s_CurrentBuf->CurrentByte = InitialPointer-StringArgLength;
        if (SearchRecord(s_CurrentBuf, StringArgument, -1) )
          LocalFail = (s_CurrentBuf->CurrentByte) != (InitialPointer-StringArgLength);
        else
          LocalFail = 1;
        s_CurrentBuf->SubstringLength = -StringArgLength; }
      s_CurrentBuf->CurrentByte = InitialPointer;
       
    if (LocalFail) { // Failure. 
      s_CurrentBuf->SubstringLength = 0; }
    break; }

    case 'M':  //M - Move command.
    case 'Y':  //Y - Move in same column.
      {
      char Arg1 = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      long long Arg2 = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int *TabStops = s_CurrentBuf->TabStops;
      int TabCount;
       
      //Column no., used by Y command, is reset by an M command.
      if (s_CommandChr == 'Y' && Arg2 == INT_MAX) {
        if (s_CurrentBuf->NoUnicode)
          s_Y_ColumnNo = s_CurrentBuf->CurrentByte;
        else
          s_Y_ColumnNo = JotStrlenBytes(s_CurrentBuf->CurrentRec->text, s_CurrentBuf->CurrentByte);
        break; }
      if (Arg1 == '=')
        Arg2 = Arg2-(s_CurrentBuf->LineNumber);
      else { // Not '=' try '*' 
        struct Window *Win = s_FirstWindow;
        
        if (Arg1 == '*') { // Window-relative move. 
          while (Win != NULL) // Window search loop. 
            if ( ((Win->Top != Win->Bot) && ((Win->WindowKey) == (s_CurrentBuf->BufferKey)) ) || (Win->WindowKey == '\0')) { // Window found. 
              int WindowHeight = (Win->Bot-Win->Top+1) - (s_CurrentBuf->Header ? 1 : 0) - (Win->DisplayFooter ? 1 : 0);
              int OldFirstLineNo = Win->OldFirstLineNo;
              int CurrentGuard = (s_GuardBandSize*2 <= WindowHeight) ? s_GuardBandSize : 0;
            
              Arg2 = OldFirstLineNo+WindowHeight-(s_CurrentBuf->LineNumber) + ( (Arg2 < 0) ? CurrentGuard-(WindowHeight*2) : WindowHeight-(CurrentGuard*2)-1 );
              break; }
            else
              Win = Win->next; } }
      
      if (s_CommandChr == 'Y' && TabStops) { //Identify the TabStop in the original line.
        char *SubString = s_CurrentBuf->CurrentRec->text;
        TabCount = 0;
        while (SubString-s_CurrentBuf->CurrentRec->text <= s_CurrentBuf->CurrentByte)   {
          char *Pos = strchr(SubString, s_TableSeparator);
          if ( ! Pos) { //Current character is in the last cell.
            TabCount++;
            break; }
          if ( ! (SubString = Pos+1))
            break;
          TabCount++; } }
      LocalFail = AdvanceRecord(s_CurrentBuf, Arg2);
      if (s_CommandChr == 'Y') { //Y Move in text column.
        int Bytes;
        if (TabStops) { //Y move in table column. 
          char *SubString = s_CurrentBuf->CurrentRec->text;
          s_Y_ColumnNo = 0;
          while (--TabCount) {
            char *Pos = strchr(SubString, s_TableSeparator);
            if ( ! Pos) {
              LocalFail = 1;
              break; }
            SubString = strchr(SubString, s_TableSeparator)+1;
            s_Y_ColumnNo = SubString-s_CurrentBuf->CurrentRec->text; } }
        s_CurrentBuf->CurrentByte = 0;
        s_CurrentBuf->SubstringLength = 0;
        Bytes = JotRecLenChrs(s_CurrentBuf, s_Y_ColumnNo);
        s_CurrentBuf->CurrentByte += (s_Y_ColumnNo && ! Bytes) ? strlen(s_CurrentBuf->CurrentRec->text) : Bytes; }
      LocalFail &= ( (Arg2 != INT_MIN) && (Arg2 != INT_MAX) );
      break; }

    case 'B': { //B - Break line.
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      long long Count = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int Index;
    
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
      s_CurrentBuf->UnchangedStatus = 0;
       
      for (Index = 1; Index <= Count; Index +=1) { // Record-loop. 
        if ( (LocalFail = BreakRecord(s_CurrentBuf)) )
          break;
        if (Direction < 0) { // Backwards.
          int i;
          s_CurrentBuf->CurrentRec = s_CurrentBuf->CurrentRec->prev;
          s_CurrentBuf->CurrentByte = 0;
          for (i = s_CurrentBuf->CurrentRec->MaxLength-1; 0 <= i; i--)
            if (s_CurrentBuf->CurrentRec->text[i]) {
              s_CurrentBuf->CurrentByte = i+1;
              break; }
          s_CurrentBuf->LineNumber -= 2; }
        else
          s_CurrentBuf->LineNumber -= 1; }
    
      break; }

    case 'T':  //T - Trace i.e. debug
      s_TraceMode = s_DefaultTraceMode;
      s_CommandMode = 0;
      if (s_JournalHand) {
        if (s_TraceMode & Trace_AllCommands ) {
          char Temp[20];
          sprintf(Temp, "%%%%Debugger on %d", s_JournalDataLines);
          UpdateJournal(Temp, NULL); }
        else
          UpdateJournal("%%%%Debugger off", NULL); }
      break;

    case 'J': { //J - Join line(s).
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      long long Count = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int Joins = 0;
    
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
      s_CurrentBuf->UnchangedStatus = 0;
      
      if (1 <= Direction)
        while (s_CurrentBuf->CurrentRec->next != s_CurrentBuf->FirstRec && Joins++ < Count)
          JoinRecords(s_CurrentBuf);
      else
        while (s_CurrentBuf->CurrentRec->prev != s_CurrentBuf->FirstRec->prev && Joins++ < Count) {
          AdvanceRecord(s_CurrentBuf, -1);
          JoinRecords(s_CurrentBuf); }
          
      LocalFail = ( (Joins < Count) && (Count != INT_MAX) );
      break; }

    case 'F': { //F - Find Command.
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      long long SearchSpan = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      char *StringArgument = FetchStringArg((struct PtrArg **)&ThisArg, s_SystemMode ? NULL : s_FindString);
      long long Count = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int OrigByte = s_CurrentBuf->CurrentByte, Reverse = FALSE;
        
      if (Count < 0) { 
        Reverse = TRUE;
        Count = -Count; }
  
      for ( ; (0 < Count)  && ( ! LocalFail); Count--) {
        if (SearchSpan != INT_MAX && SearchSpan != INT_MIN)
          LocalFail = ! GeneralSearchBuffer(s_CurrentBuf, StringArgument, s_TableSeparator, 0, Direction, SearchSpan-1, -1, Reverse);
        else
          LocalFail = ! SimpleSearchBuffer(s_CurrentBuf, StringArgument, Direction, Reverse); }
      if (LocalFail && SearchSpan == 1)
        s_CurrentBuf->CurrentByte = OrigByte;
          
      break; }

    case 'G':  //G - Get - Input text from console.
      { //Get block. 
      struct Rec *PrevRecordSave = s_CurrentBuf->CurrentRec->prev;
      long long Count = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int ExitNow = FALSE, Infinate = (Count == INT_MAX);
      
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
      if (s_TermWidth < strlen(s_PromptString)) {
        Message(NULL, "Prompt string truncated to %d characters", s_TermWidth-1);
        s_PromptString[s_TermWidth-1] = '\0'; }
      s_CurrentBuf->UnchangedStatus = 0;
    
      for ( ; Count; Count--) { // Insert-records loop.
        struct Rec *Rec;
        if (s_PromptString[0])
          Prompt("%s", s_PromptString);
        s_CurrentBuf->CurrentRec = s_CurrentBuf->CurrentRec->prev;
        if ( ! ReadNewRecord(s_CurrentBuf, s_asConsole ? s_asConsole->FileHandle : s_EditorConsole->FileHandle, FALSE))
          break;
        if (s_BombOut) {
          s_CurrentBuf->LineNumber--;
          if (s_JournalHand)
            UpdateJournal("%%G-command terminated", NULL);
          break; }
        Rec = s_CurrentBuf->CurrentRec;
        if ((strlen(Rec->text) == 1) && ((Rec->text)[0] == ':')) { // Last record. 
          FreeRecord(s_CurrentBuf, AdjustForwards);
          ExitNow = TRUE;
          s_CurrentBuf->LineNumber--;
          break; }
        s_CurrentBuf->CurrentRec = Rec->next;
        s_EditorInput->LineNo++;
        JotUpdateWindow();
        if (ExitNow || LocalFail)
          break; }
  
      if ((s_CurrentBuf->CurrentRec) == (s_CurrentBuf->FirstRec))
        s_CurrentBuf->FirstRec = PrevRecordSave->next;
      LocalFail |= ( ! Infinate && (0 < Count) );
      break; }

    case 'H': { //H - Here command.
      char BufferKey = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      long long Count = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int OriginalCurrentByte = s_CurrentBuf->CurrentByte;
      int OriginalLineNumber = s_CurrentBuf->LineNumber;
      int Index, Reverse = FALSE;
      int OrigSourceLineNumber, OrigSourceCurrentByte, OrigSourceSubstrLen;
      struct Rec *OrigSourceCurrentRec;
      struct Buf *SourceBuf = GetBuffer(BufferKey, NeverNew);
      
      if (  ( ! SourceBuf) || ! CheckBufferKey(BufferKey) || (SourceBuf == s_CurrentBuf)) {
        LocalFail = Fail("Invalid buffer.");
        break; }
      if ( ! (OrigSourceCurrentRec = SourceBuf->CurrentRec) ) //The source buffer is empty - do nothing.
        break;
      OrigSourceLineNumber = SourceBuf->LineNumber;
      OrigSourceCurrentByte = SourceBuf->CurrentByte;
      OrigSourceSubstrLen = SourceBuf->SubstringLength;
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
      s_CurrentBuf->UnchangedStatus = 0;
      ResetBuffer(SourceBuf);
      if (Count < 0) {
        Count = -Count;
        Reverse = TRUE; }
      
      for (Index = 1; Index <= Count; Index +=1) { // Copy-buffer loop. 
        if (SourceBuf->AbstractWholeRecordFlag )
          s_CurrentBuf->CurrentByte = 0;
        LocalFail |= BreakRecord(s_CurrentBuf);
        LocalFail |= AdvanceRecord(s_CurrentBuf, -1);
        LocalFail |= DuplicateRecord(s_CurrentBuf, SourceBuf);
        LocalFail |= AdvanceRecord(s_CurrentBuf, -1);
        LocalFail |= JoinRecords(s_CurrentBuf);
  
        for (;;) { // Record loop. 
          if (AdvanceRecord(SourceBuf, 1) )
            break;
          DuplicateRecord(s_CurrentBuf, SourceBuf); }
          
        LocalFail |= JoinRecords(s_CurrentBuf);
        s_CurrentBuf->LineNumber -= 1; 
      ResetBuffer(SourceBuf); } // Copy-buffer loop ends.
           
      if (Reverse) {
        MoveDown(s_CurrentBuf, s_CurrentBuf->LineNumber-OriginalLineNumber, NULL);
        s_CurrentBuf->CurrentByte = OriginalCurrentByte; }
      SourceBuf->CurrentRec = OrigSourceCurrentRec;
      SourceBuf->LineNumber = OrigSourceLineNumber;
      SourceBuf->CurrentByte = OrigSourceCurrentByte;
      SourceBuf->SubstringLength = OrigSourceSubstrLen;
      break; }

    case 'S': { //S - Substitute command.
      long long Arg1 = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      char *StringArgument = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_SystemMode ? NULL : s_InsertString);
       
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
      s_CurrentBuf->UnchangedStatus = 0;
       
      if ((s_CurrentBuf->SubstringLength) == 0) {
        LocalFail = 1;
        break; }
      else { // Non-null substitute string. 
        LocalFail = SubstituteString(s_CurrentBuf, StringArgument, -1);
        if (0 < Arg1) {
          s_CurrentBuf->SubstringLength = -s_CurrentBuf->SubstringLength;
          s_CurrentBuf->CurrentByte = (s_CurrentBuf->CurrentByte) - (s_CurrentBuf->SubstringLength); } }
      break; }

    case 'I': { //I - Insert command.
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      char *StringArgument = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_SystemMode ? NULL : s_InsertString);
      long long Arg2 = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int StringLenBytes = strlen(StringArgument);
      int RepCount;
        
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
      s_CurrentBuf->UnchangedStatus = 0;
      
      if (Arg2 <= 0)
        break;
       
      for (RepCount = 1; RepCount <= Arg2; RepCount +=1) { // Repeat loop. 
        s_CurrentBuf->SubstringLength = 0;
        LocalFail = SubstituteString(s_CurrentBuf, StringArgument, -1); }
    
      if (0 < Direction) {
        s_CurrentBuf->SubstringLength = - StringLenBytes;
        s_CurrentBuf->CurrentByte = s_CurrentBuf->CurrentByte + StringLenBytes*(RepCount-1); }
      else
        s_CurrentBuf->SubstringLength = StringLenBytes;
      break; }

    case 'E': { //E - Erase characters (bytes if unicode is disables).
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      long long Chrs = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int Bytes, To, From;
      
      if (0 <= Direction) {
        if (Chrs == INT_MAX) { //Erase to end of line.
          Bytes = s_CurrentBuf->CurrentRec->MaxLength - s_CurrentBuf->CurrentByte;
          memset(s_CurrentBuf->CurrentRec->text + s_CurrentBuf->CurrentByte, '\0', Bytes);
          s_CurrentBuf->SubstringLength = 0;
          break; }
        else { //The number of bytes needs to be calculated properly.
          Bytes = JotRecLenChrs(s_CurrentBuf, Chrs);
          To = s_CurrentBuf->CurrentByte;
          From = To + Bytes; } }
      if (0 <= Direction) {
        Bytes = (Chrs == INT_MAX) ? (strlen(s_CurrentBuf->CurrentRec->text) - s_CurrentBuf->CurrentByte) : JotRecLenChrs(s_CurrentBuf, Chrs);
        To = s_CurrentBuf->CurrentByte;
        From = To + Bytes; }
      else {
        Bytes = (Chrs == INT_MAX) ? -s_CurrentBuf->CurrentByte : JotRecLenChrs(s_CurrentBuf, -Chrs);
        From = s_CurrentBuf->CurrentByte;
        To = From + Bytes;
        s_CurrentBuf->CurrentByte += Bytes; }
        
      if (From == To) {
        if (Chrs != INT_MAX) 
          LocalFail = 1;
        break; }
        
      s_CurrentBuf->SubstringLength = From-To;
      LocalFail = SubstituteString(s_CurrentBuf, NULL, -1);
      break; }

    case 'C':  //C - Change case of characters.
      { // Change case of alpha characters. 
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      long long ChrsToChange = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      char *RecordText = s_CurrentBuf->CurrentRec->text;
      int StartByte = s_CurrentBuf->CurrentByte, ByteNo = StartByte;
      int ByteCount = JotStrlenChrs(RecordText+ByteNo, ChrsToChange);
      
      if (ChrsToChange == INT_MAX)
        ByteCount = (0 < Direction) ? strlen(RecordText+StartByte) : StartByte;
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
       
      if (0 < Direction) { //Forwrds change.
        int Bytes = JotStrlenChrs(RecordText+ByteNo, ChrsToChange);
        if (JotStrlenBytes(RecordText+ByteNo, Bytes) < ChrsToChange) {
          if (ChrsToChange != INT_MAX)
            LocalFail = Fail("Over-run the end of line."); }
        while ( ByteNo < StartByte+ByteCount ) {
          int Chr = RecordText[ByteNo];
          RecordText[ByteNo++] = (('a' <=  Chr && Chr <= 'z') ? Chr-'a'+'A' : (('A' <=  Chr && Chr <= 'Z') ? Chr-'A'+'a' : Chr)); } }
      else { //Backwards change.
        while ( 1 ) {
          int Chr;
          if  (--ByteNo < 0) {
            if (ChrsToChange != INT_MAX)
              LocalFail = Fail("Over-run the end of line.");
            ByteNo = 0;
            break; }
          Chr = RecordText[ByteNo];
          RecordText[ByteNo] = (('a' <=  Chr && Chr <= 'z') ? Chr-'a'+'A' : (('A' <=  Chr && Chr <= 'Z') ? Chr-'A'+'a' : Chr));
          if ( ChrsToChange <= JotStrlenBytes(RecordText+ByteNo, StartByte-ByteNo) )
            break; } }
      
      s_CurrentBuf->SubstringLength = 0;
      s_CurrentBuf->CurrentByte = ByteNo;
      s_CurrentBuf->CurrentRec->DisplayFlag = Redraw_Line;
      s_CurrentBuf->UnchangedStatus = 0;
      break; }

    case 'R': { //R - move current character pointer Right characters.
      int Displacement = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      
      s_CurrentBuf->SubstringLength = 0; 
      if (Displacement == INT_MAX) {
        int i;
        s_CurrentBuf->CurrentByte = 0;
        for (i = s_CurrentBuf->CurrentRec->MaxLength-1; 0 <= i; i--)
          if (s_CurrentBuf->CurrentRec->text[i]) {
            s_CurrentBuf->CurrentByte = i+1;
            break; } }
      else if (Displacement == INT_MIN)
        s_CurrentBuf->CurrentByte = 0;
      else { //A finite displacement.
        int ByteShift = JotRecLenChrs(s_CurrentBuf, Displacement);
        if (ByteShift)
          s_CurrentBuf->CurrentByte += ByteShift;
        else { //Specified displacement sends it past the start/end of the string.
          s_CurrentBuf->CurrentByte = 0;
          if (0 < Displacement) {
            int TextEnd = 0;
            for (TextEnd = s_CurrentBuf->CurrentRec->MaxLength-1; 0 <= TextEnd; TextEnd--)
              if (s_CurrentBuf->CurrentRec->text[TextEnd])
                break;
            s_CurrentBuf->CurrentByte = TextEnd+1; }
          else 
            s_CurrentBuf->CurrentByte = 0;
          LocalFail = 1; } }
      break; }

    case 'K': { //K - delete (Kill) some lines.
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      long long Count = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int Index, LastRec = FALSE, FirstRec = FALSE;
    
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
       
      for (Index = 1; ! LocalFail && (Index <= Count); Index+=1) { // Record Loop. 
        if (s_CurrentBuf->CurrentRec->next == s_CurrentBuf->FirstRec)
          LastRec = TRUE;
        if (s_CurrentBuf->CurrentRec == s_CurrentBuf->FirstRec)
          FirstRec = TRUE;
          
        if (FirstRec && LastRec) {
          struct AnyTag * ThisTag = s_CurrentBuf->CurrentRec->TagChain;
          memset(s_CurrentBuf->CurrentRec->text, '\0', s_CurrentBuf->CurrentRec->MaxLength);
          s_CurrentBuf->CurrentByte = 0;
          s_CurrentBuf->SubstringLength = 0;
          while (ThisTag) { //Remove all but htab target tags.
            struct AnyTag * NextTag = ThisTag->next;
            if (ThisTag->type != TagType_Target)
              FreeTag(s_CurrentBuf->CurrentRec, ThisTag);
            else //It is a target tag.
              ThisTag->StartPoint = 0;
            ThisTag = NextTag; }
          LocalFail = 1;
          break; }
        else 
          LocalFail |= FreeRecord(s_CurrentBuf, (Direction == 1) ? AdjustForwards : AdjustBack);
        if (LocalFail)
          break;
        s_CurrentBuf->UnchangedStatus = 0;
        if (Direction < 0) {
          if (FirstRec) {
            LocalFail = 1;
            break; }
          if (LastRec) {
            s_CurrentBuf->LineNumber -= 1;
            s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec->prev; }
          else if (AdvanceRecord(s_CurrentBuf, -1))
            break; }
        else if (LastRec) {
          s_CurrentBuf->LineNumber -= 1;
          s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec->prev;
          LocalFail = 1; } }
          
      s_CurrentBuf->SubstringLength = 0;
      s_CurrentBuf->CurrentByte = 0;
      if (Count == INT_MAX)
        LocalFail = 0;
      break; }

    case 'P': { //P - Print line(s).
      long long Direction = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      long long Count = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      int Index;
      
      for (Index = 1; Index <= Count; Index++) { // Print loop. 
        if (Index == 1) { //First line.
          if (s_TTYMode && ! (s_Verbose & Verbose_NonSilent) ) //Print lines exactly as-is, in -tty && -silent mode
            fprintf(stdout, "%s\n", s_CurrentBuf->CurrentRec->text);
          else
            DisplayDiag(s_CurrentBuf, TRUE, ""); }
        else {
          if ( (LocalFail = AdvanceRecord(s_CurrentBuf, Direction)) ) {
            break; }
          if (s_TTYMode && ! (s_Verbose & Verbose_NonSilent) )
            fprintf(stdout, "%s\n", s_CurrentBuf->CurrentRec->text);
          else
            DisplayDiag(s_CurrentBuf, FALSE, ""); } }
         
#if defined(LINUX)
      refresh();
#endif
      LocalFail = (LocalFail && (Count != INT_MAX));
      break; }

    case 'O': { //O - Stack Operation.
      char Chr = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      switch (Chr) {//Stack operation switch block.
      case '@': { //O@ reset stack
        long long Seed = (0 < s_StackPtr) && ((s_Stack[s_StackPtr-1].type == FrameType_Int)) ? PopInt(&LocalFail) : 1;
        srand(Seed);
        while (s_StackPtr && ! KillTop())
          { }
        break; }
         
      case '?': //O? Dump contents of stack
        DumpStack(NULL);
        break;
         
      case 'A': //OA rAndom - push a random number to top of stack.
        LocalFail = PushInt(rand());
        break;

      case 'G': { //OG prompts and reads one character from console stream.
        char Buf[10];
        wchar_t WChr;
#ifdef VC
        char Bytes[20];
#else
        char Bytes[MB_CUR_MAX];
#endif
        int i = 0;
        
        Bytes[0] = '\0';
        if (s_PromptString[0])
          Prompt("%s", s_PromptString);
        do {
          int Chr = JotGetCh(s_asConsole ? s_asConsole->FileHandle : s_EditorConsole->FileHandle);
          Bytes[i++] = Chr;
          if (Chr == EOF || s_BombOut) {
            LocalFail = 1;
            break; } }
          while (mblen(Bytes, i) < 0);
        Bytes[i] = '\0';
          
        mbtowc(&WChr, Bytes, i);
        if (s_RecoveryMode) { //In recovery mode pick up the character from the journal file.
          ReadString(Buf, 100, s_asConsole->FileHandle);
          sscanf(Buf, "%lc", &WChr); }
        else if (s_JournalHand) {
          sprintf(Buf, "%lc", WChr);
          UpdateJournal(Buf, NULL); }
        LocalFail |= PushInt(WChr);
        break; }

      case 'W': { //OW Scrolls up by lines set in top of stack.
        struct Window *Win = s_FirstWindow;
        int WindowHeight, Offset;
        
        //First find the current window.
        while (Win ) {
          if (Win->LastKey == s_CurrentBuf->BufferKey)
            break;
          Win = Win->next; }
        if ( ! Win ) {
          LocalFail = Fail("Failed to find current buffer in display.");
          break; }
         
        Offset = PopInt(&LocalFail);
        if (LocalFail)
          break;
        
        //Apply sane limits.
        WindowHeight = Win->Bot - Win->Top + 1;
        if (Offset+WindowHeight < 0)
          Offset = -WindowHeight;
        if (0 < Offset-WindowHeight)
          Offset = WindowHeight;
        Win->OldFirstLineNo += Offset;
        s_CurrentBuf->OldFirstLineNo += Offset;
        JotScroll(Win, Offset);
        JotUpdateWindow();
        break; }

      case 'S': { //OS Swap 1st and 2nd items of stack.
        struct intFrame TopCopy;
         
        if (s_StackPtr <= 1) {
          LocalFail = 1;
          break; }
        TopCopy.type = s_Stack[s_StackPtr-1].type;
        TopCopy.Value = s_Stack[s_StackPtr-1].Value;
        s_Stack[s_StackPtr-1].type = s_Stack[s_StackPtr-2].type;
        s_Stack[s_StackPtr-1].Value = s_Stack[s_StackPtr-2].Value;
        s_Stack[s_StackPtr-2].type = TopCopy.type;
        s_Stack[s_StackPtr-2].Value = TopCopy.Value; }
      break;

      case '#': { //O# Duplicate top of stack.
        if (s_StackPtr < 1)
          LocalFail = Fail("Stack empty");
        else if (s_StackSize <= s_StackPtr)
          LocalFail = Fail("Stack overflow");
        if (LocalFail)
          break;
        s_Stack[s_StackPtr].Value = s_Stack[s_StackPtr-1].Value;
        s_Stack[s_StackPtr].type = s_Stack[s_StackPtr-1].type;
        s_StackPtr += 1;
        break; }

      case 'L': {  //OL Literal number to be pushed onto stack.
        int Type = FetchIntArgFunc((struct AnyArg **)&ThisArg);
        if (Type == 0)
          LocalFail = PushInt(FetchLLIntArgFunc((struct AnyArg **)&ThisArg));
        else
          LocalFail = PushFloat(FetchFloatArg((struct FloatArg **)&ThisArg));
        break; }

      case '=': { //O= Fail if intergerized top two items on stack are unequal.
        long long TopValue, NewTopValue;
        if (s_StackPtr <= 1) {
          LocalFail = Fail("Less than two items on stack");
          break; }
        if ((s_Stack[s_StackPtr-1].type == FrameType_Buf) || (s_Stack[s_StackPtr-2].type == FrameType_Buf)) {
          LocalFail = Fail("Buffer types not allowed in arithmetic operations.");
          break; }
        TopValue = (s_Stack[s_StackPtr-1].type == FrameType_Int) ? PopInt(&LocalFail) : (int)PopFloat(&LocalFail);
        NewTopValue = (s_Stack[s_StackPtr-1].type == FrameType_Int) ? s_Stack[s_StackPtr-1].Value  : ((struct floatFrame *)(&s_Stack[s_StackPtr-1]))->fValue;
        LocalFail |= (TopValue != NewTopValue);
        break; }

      case '1': //O1 Fail if intergerized top item on stack is not 1.
      case '0': //O0 Fail if intergerized top item on stack is not 0.
        {
        long long TopValue;
        if (s_StackPtr < 1) {
          LocalFail = Fail("Stack empty");
          break; }
        if (s_Stack[s_StackPtr-1].type == FrameType_Buf) {
          LocalFail = Fail("Buffer types not allowed in arithmetic operations.");
          break; }
        TopValue = (s_Stack[s_StackPtr-1].type == FrameType_Int) ? PopInt(&LocalFail) : (int)PopFloat(&LocalFail);
        LocalFail |= (Chr == '1') ? (TopValue != 1) : (TopValue != 0);
        break; }

      case '<': { //O< Test that item at top greater than (top-1).
        if (s_StackPtr <= 1) {
          LocalFail = Fail("Less than two items on stack");
          break; }
        if (s_Stack[s_StackPtr-1].type == FrameType_Buf) {
          LocalFail = Fail("Attempt to compare a buffer object on the stack");
          break; }
        if ( (s_Stack[s_StackPtr-1].type == FrameType_Int) && (s_Stack[s_StackPtr-2].type == FrameType_Int) ) { //Both items are integer - no conversion required.
          long long OldTopValue = PopInt(&LocalFail);
          LocalFail |= (OldTopValue <= s_Stack[s_StackPtr-1].Value); }
        else if ((s_Stack[s_StackPtr-1].type == FrameType_Buf) || (s_Stack[s_StackPtr-2].type == FrameType_Buf))
          LocalFail = Fail("Buffer types not allowed in arithmetic operations.");
        else { 
          double OldTopValue = (s_Stack[s_StackPtr-1].type == FrameType_Int) ? (double)PopInt(&LocalFail) : PopFloat(&LocalFail);
          double NewTopValue = (s_Stack[s_StackPtr-1].type == FrameType_Int) ? (double)s_Stack[s_StackPtr-1].Value : ((struct floatFrame *)(&s_Stack[s_StackPtr-1]))->fValue;
          LocalFail |= (OldTopValue <= NewTopValue); } }
        break;

      case '>': { //O> Test that item at top less than (top-1).
        if (s_StackPtr <= 1) {
          LocalFail = Fail("Less than two items on stack");
          break; }
        if ( (s_Stack[s_StackPtr-1].type == FrameType_Int) && (s_Stack[s_StackPtr-2].type == FrameType_Int) ) { //Both items are integer - no conversion required.
          long long OldTopValue = PopInt(&LocalFail);
          LocalFail |= ((long long)s_Stack[s_StackPtr-1].Value <= OldTopValue); }
        else if ((s_Stack[s_StackPtr-1].type == FrameType_Buf) || (s_Stack[s_StackPtr-2].type == FrameType_Buf))
          LocalFail |= Fail("Buffer types not allowed in arithmetic operations.");
        else { 
          double OldTopValue = (s_Stack[s_StackPtr-1].type == FrameType_Int) ? (double)PopInt(&LocalFail) : PopFloat(&LocalFail);
          double NewTopValue = (s_Stack[s_StackPtr-1].type == FrameType_Int) ? (double)s_Stack[s_StackPtr-1].Value : ((struct floatFrame *)(&s_Stack[s_StackPtr-1]))->fValue;
          LocalFail |= (NewTopValue <= OldTopValue); } }
        break;

      case 'K':  //OK Kill - delete top of stack.
        LocalFail = KillTop();
        break;

      case 'I': { //OI formatted Input - Convert chr(s) at CurentChr using format key.
        char FormatChr = ChrUpper(FetchIntArgFunc((struct AnyArg **)&ThisArg));
        int Length;
        
        if (FormatChr == 'C') { //OIC
          int Chr, Bytes = 1;
          if (strlen(s_CurrentBuf->CurrentRec->text) <= s_CurrentBuf->CurrentByte) {
            LocalFail = 1;
            break; }
          Chr = (signed char)s_CurrentBuf->CurrentRec->text[s_CurrentBuf->CurrentByte];
          if (Chr < 0) {
            if (s_CurrentBuf->NoUnicode)
              LocalFail = PushInt(Chr);
            else {
              wchar_t w_Chr = '?';
              int Size = -1;
              if ( ! (Bytes = JotStrlenChrs(s_CurrentBuf->CurrentRec->text + s_CurrentBuf->CurrentByte, 1) ) ) {
                LocalFail = Fail("Invalid character in IOC");
                break; }
#ifdef VC
              Size = MultiByteToWideChar(s_CodePage, MB_ERR_INVALID_CHARS, s_CurrentBuf->CurrentRec->text + s_CurrentBuf->CurrentByte, -1, &w_Chr, 1);
              if (Size < 0)
                LocalFail = 1;
#else
              Size = mbstowcs(&w_Chr, s_CurrentBuf->CurrentRec->text + s_CurrentBuf->CurrentByte, 1);
              if (Size <= 0)
                LocalFail = 1;
#endif
              LocalFail |= PushInt(w_Chr); } }
          else
            LocalFail |= PushInt(Chr);
            
          s_CurrentBuf->CurrentByte += Bytes;
          s_CurrentBuf->SubstringLength = -Bytes; }
          
        else if (FormatChr == 'D') { //OID
          long long IntValue;
          if (sscanf((s_CurrentBuf->CurrentRec->text)+(s_CurrentBuf->CurrentByte), "%lld%n", &IntValue, &Length) <= 0) {
            LocalFail = 1;
            break; }
          else {
            LocalFail = PushInt(IntValue);
            s_CurrentBuf->CurrentByte += Length;
            s_CurrentBuf->SubstringLength = -Length;
            break; } }
            
        else if (FormatChr == 'X') { //OIX
          long long IntValue;
          if (sscanf((s_CurrentBuf->CurrentRec->text)+(s_CurrentBuf->CurrentByte), "%llX%n", &IntValue, &Length) <= 0) {
            LocalFail = 1;
            break; }
          else {
            LocalFail = PushInt(IntValue);
            s_CurrentBuf->CurrentByte += Length;
            s_CurrentBuf->SubstringLength = -Length;
            break; } }
            
        else if (FormatChr == 'O') { //OIO
          long long IntValue;
          if (sscanf((s_CurrentBuf->CurrentRec->text)+(s_CurrentBuf->CurrentByte), "%llo%n", &IntValue, &Length) <= 0) {
            LocalFail = 1;
            break; }
          else {
            LocalFail = PushInt(IntValue);
            s_CurrentBuf->CurrentByte += Length;
            s_CurrentBuf->SubstringLength = -Length;
            break; } }
            
#if defined(VC)
        else if (FormatChr == 'F') { //OIF
          float Float;
          if (sscanf((s_CurrentBuf->CurrentRec->text)+(s_CurrentBuf->CurrentByte), "%f%n", &Float, &Length) <= 0) {
            LocalFail = PushFloat(1.0);
            LocalFail |= Fail("Error converting floating-point number");
            break; }
          else {
            LocalFail = PushFloat((double)Float);
            s_CurrentBuf->CurrentByte += Length;
            s_CurrentBuf->SubstringLength = -Length;
            break; } }
#else

        else if (FormatChr == 'F') { //OIF
          char *EndPtr;
          float Float = strtof((s_CurrentBuf->CurrentRec->text)+(s_CurrentBuf->CurrentByte), &EndPtr);
          errno = 0;
          Length = EndPtr-(s_CurrentBuf->CurrentRec->text);
          if (errno) {
            LocalFail = PushFloat(1.0);
            LocalFail |= Fail("Error converting floating-point number");
            break; }
          else {
            LocalFail |= PushFloat(Float);
            s_CurrentBuf->CurrentByte += Length;
            s_CurrentBuf->SubstringLength = -Length;
            break; } }
#endif
        else 
          LocalFail = 1;
        break; }

      case 'V': { //OV define Value of hashtable data from top of stack.
        //After checking, attach the top stack frame to the DataHTabObj located by the QueryPath() call.
        char Temp[StringMaxChr];
        char *Path = (char *)FetchStringArg((struct PtrArg **)&ThisArg, Temp);
        LocalFail = SetDataObject(Path);
        break; }

      case 'Q': { //OQ Query hashtable, copy previously-defined stack frame to top of stack.
        struct Buf * ThisBuf;
        char Temp[StringMaxChr];
        char *LastPathElem, *Path = (char *)FetchStringArg((struct PtrArg **)&ThisArg, Temp);
        struct intFrame *FinalFrame = NULL;
        
        if (s_StackSize <= s_StackPtr) {
          LocalFail = Fail("Stack overflow");
          break; }
        if ( ! (ThisBuf = QueryPath(Path, s_CurrentBuf, &LastPathElem, &FinalFrame, NULL)) || ! FinalFrame ) {
          LocalFail = Fail("Error in path %s near %s", Path, LastPathElem);
          break; }
        memcpy(&s_Stack[s_StackPtr++], FinalFrame, sizeof(struct intFrame));
        break; }

      case 'O': { //OO Output to current text, using given string as format spec.
        char Temp[StringMaxChr];
        char *FormatString = (char *)FetchStringArg((struct PtrArg **)&ThisArg, Temp);
        char TempString[StringMaxChr];
        int Chrs;
         
        if (s_CurrentBuf->EditLock & ReadOnly) {
          LocalFail = RunError("Attempt to modify a readonly buffer");
          break; }
        s_CurrentBuf->UnchangedStatus = 0;
       
        if (s_StackPtr < 1) {
          LocalFail = Fail("Stack underflow");
          break; }
        if (s_Stack[s_StackPtr-1].type == FrameType_Buf) {
          struct Buf *Buf = ((struct bufFrame *)(&s_Stack[s_StackPtr-1]))->BufPtr;
          Chrs = sprintf(TempString, FormatString, Buf->CurrentRec->text);
          KillTop(); }
        else if (s_Stack[s_StackPtr-1].type == FrameType_Float) {
          Chrs = sprintf(TempString, FormatString, ((struct floatFrame *)(&s_Stack[s_StackPtr-1]))->fValue);
          KillTop(); }
        else { //An integer.
          //Outputting any interger in any valid format is OK in linux.
          //Outputtng wide a character using either (%lc or %C) in windowsland is problematic.
          Chrs = sprintf(TempString, FormatString, s_Stack[s_StackPtr-1].Value);
          KillTop(); }
        if ( ( ! s_CurrentBuf->NoUnicode) && (JotStrlenBytes(TempString, Chrs) < 0) ) {
          LocalFail = Fail("The OO operation would have introduced invalid unicode into the buffer.");
          break; }
        SubstituteString(s_CurrentBuf, TempString, -1);
        s_CurrentBuf->SubstringLength = -strlen(TempString);
        s_CurrentBuf->CurrentByte = s_CurrentBuf->CurrentByte - s_CurrentBuf->SubstringLength;
        break; }

      case '~': { //O~ Increment - (item at top of stack)++, fail if result=0.
        int ActualType;
        if (s_StackPtr < 1) {
          LocalFail = 1;
          break; }
        if ( (ActualType = s_Stack[s_StackPtr-1].type) != FrameType_Int) {
          LocalFail = RunError("Incorrect stack-frame type, expecting integer frame, found frame type %d", ActualType);
          break; }
        LocalFail |= ! (++s_Stack[s_StackPtr-1].Value);
        break; }

      case '!': { //O! bitwise NOT
        int ActualType;
        if (s_StackPtr < 1) {
          LocalFail = Fail("No items on stack");
          break; }
        if ( (ActualType = s_Stack[s_StackPtr-1].type) != FrameType_Int) {
          LocalFail = RunError("Incorrect stack-frame type, expecting integer frame, found frame type %d", ActualType);
          break; }
        s_Stack[s_StackPtr-1].Value = s_Stack[s_StackPtr-1].Value ^ -1;
        break; }

      case '&':   //O& - bitwise OR
      case '|': { //O| - bitwise AND
        if (s_StackPtr <= 1) {
          LocalFail = Fail("Less than two items on stack");
          break; }
        if ((s_Stack[s_StackPtr-1].type != FrameType_Int) && (s_Stack[s_StackPtr-2].type != FrameType_Int)) {
          LocalFail = RunError("Non integer types not allowed in bitwise AND/OR operations");
          break; }
        LocalFail |= PushInt(Chr == '&' ? PopInt(&LocalFail)&PopInt(&LocalFail) : PopInt(&LocalFail)|PopInt(&LocalFail));
        break; }

      case '+':   //O+ Add
      case '-':   //O- Subtract
      case '*':   //O* Multiply
      case '/':   //O/ Divide
      case '%': { //O% Remainder
        if (s_StackPtr <= 1) {
          LocalFail = Fail("Less than two items on stack");
          break; }
        if ((s_Stack[s_StackPtr-1].type == FrameType_Int) && (s_Stack[s_StackPtr-2].type == FrameType_Int)) {
          long long Arg2 = PopInt(&LocalFail);
          long long Arg1 = PopInt(&LocalFail);
          switch (Chr) {//Two-operand-stack-operations switch block.
            case '+':
              LocalFail |= PushInt(Arg1+Arg2);
              break;
            case '-':
              LocalFail |= PushInt(Arg1-Arg2);
              break;
            case '*':
              LocalFail |= PushInt(Arg1*Arg2);
              break;
            case '/':
              if (Arg2 == 0) { // Divide by zero.
                s_StackPtr = s_StackPtr+2;
                LocalFail = 1;
                break; }
              LocalFail |= PushInt(Arg1/Arg2);
              break;
            case '%':  //Remainder
              if (Arg2 == 0) { // Divide by zero.
                s_StackPtr = s_StackPtr+2;
                LocalFail = 1;
                break; }
              LocalFail |= PushInt(Arg1%Arg2);
              break; } }
        else if ((s_Stack[s_StackPtr-1].type == FrameType_Buf) || (s_Stack[s_StackPtr-2].type == FrameType_Buf)) {
          LocalFail = Fail("Buffer types not allowed in arithmetic operations.");
          break; }
        else if (Chr == '%') { //O% - convert both items to integer.
          long long Arg2 = (s_Stack[s_StackPtr-1].type == FrameType_Float) ? (int)PopFloat(&LocalFail) : PopInt(&LocalFail);
          long long Arg1 = (s_Stack[s_StackPtr-1].type == FrameType_Float) ? (int)PopFloat(&LocalFail) : PopInt(&LocalFail);
          if (Arg2 == 0) { // Divide by zero.
            s_StackPtr = s_StackPtr+2;
            LocalFail = 1;
            break; }
          LocalFail |= PushInt(Arg1%Arg2);
          break; }
        else {
          double Arg2 = (s_Stack[s_StackPtr-1].type == FrameType_Int) ? (double)PopInt(&LocalFail) : PopFloat(&LocalFail);
          double Arg1 = (s_Stack[s_StackPtr-1].type == FrameType_Int) ? (double)PopInt(&LocalFail) : PopFloat(&LocalFail);
          switch (Chr) {//Two-operand-stack-operations switch block.
            case '+':
              LocalFail |= PushFloat(Arg1+Arg2);
              break;
            case '-':
              LocalFail |= PushFloat(Arg1-Arg2);
              break;
            case '*':
              LocalFail |= PushFloat(Arg1*Arg2);
              break;
            case '/':
              if (Arg2 == 0.0) { // Divide by zero.
                s_StackPtr = s_StackPtr+1;
                LocalFail = 1;
                break; }
              LocalFail |= PushFloat(Arg1/Arg2); } } }
          break;

      case '.': { //O. Set current line number to <top of stack>.
        int NewLineNo = 0;
        int Offset;
        
        NewLineNo = (int)PopInt(&LocalFail);
        Offset = s_CurrentBuf->LineNumber - NewLineNo;
        s_CurrentBuf->LineNumber = NewLineNo;
        s_CurrentBuf->OldFirstLineNo -= Offset;
        break; }

      case 'B':  //OB Push current buffer key onto stack.
        LocalFail = PushInt(s_CurrentBuf->BufferKey);
        break;

      case 'X': { //OX Push eXtent (i.e. length of current substring) taking unicode characters into account.
        
        if (s_CurrentBuf->NoUnicode) //Unicode is disabled - just duplicate the byte-count entry.
          LocalFail = PushInt(s_CurrentBuf->SubstringLength ? s_CurrentBuf->SubstringLength : strlen(s_CurrentBuf->CurrentRec->text));
        else {  //Unicode is enabled - add true character count.
          if (s_CurrentBuf->SubstringLength) { //Length of substring in display characters.
            if (s_CurrentBuf->SubstringLength < 0)
              LocalFail |= PushInt( - JotStrlenBytes(s_CurrentBuf->CurrentRec->text+s_CurrentBuf->CurrentByte+s_CurrentBuf->SubstringLength, -s_CurrentBuf->SubstringLength));
            else 
              LocalFail |= PushInt(JotStrlenBytes(s_CurrentBuf->CurrentRec->text+s_CurrentBuf->CurrentByte, s_CurrentBuf->SubstringLength)); }
          else //Length of entire record in display characters.
            LocalFail |= PushInt(JotStrlenBytes(s_CurrentBuf->CurrentRec->text, strlen(s_CurrentBuf->CurrentRec->text))); }
        break; }

      case 'F': { //OF Push line no. of first line in window.
        LocalFail = PushInt(s_CurrentBuf->OldFirstLineNo);
        break; }

      case 'P': { //OP push mouse Position - last detected mouse-click position
        char Buf[100];
        struct Buf *MouseBuf;
        if (s_RecoveryMode) { //In recovery mode pick up the mouse-click point from the journal file.
          ReadString(Buf, 100, s_asConsole->FileHandle);
          sscanf(Buf, "%d %d %d\033MOUSE", &s_MouseBufKey, &s_MouseLine, &s_Mouse_x); }
        else if (s_JournalHand) {
          sprintf(Buf, "%d %d %d", s_MouseBufKey, s_MouseLine, s_Mouse_x);
          UpdateJournal(Buf, "<<MOUSE>>"); }
        MouseBuf = GetBuffer(s_MouseBufKey, NeverNew);
        LocalFail = PushInt(s_Mouse_x + (MouseBuf ? MouseBuf : s_CurrentBuf)->LeftOffset);
        LocalFail |= PushInt(s_MouseLine);
        LocalFail |= PushInt(s_MouseBufKey);
        break; }

      case 'N':  //ON Push current line no. onto stack.
        LocalFail = PushInt(s_CurrentBuf->LineNumber);
        break;

      case 'U': //OU set currently-selected sUbstring length from value on stack.
        if (s_CurrentBuf->NoUnicode) {
          int SubstringLength = PopInt(&LocalFail);
          int Len = strlen(s_CurrentBuf->CurrentRec->text);
          if ( (s_CurrentBuf->CurrentByte + SubstringLength) < 0 ) {
             SubstringLength = -s_CurrentBuf->CurrentByte;
             LocalFail = 1; }
           else if (Len < (s_CurrentBuf->CurrentByte + SubstringLength) ) {
             SubstringLength = Len - s_CurrentBuf->CurrentByte;
             LocalFail = 1; }
           s_CurrentBuf->SubstringLength = SubstringLength; }
        else {
          int Chrs = PopInt(&LocalFail);
          int Bytes = JotRecLenChrs(s_CurrentBuf, Chrs);
          s_CurrentBuf->SubstringLength = Bytes; }
        break;

      case 'C': { //OC Push current chr. no. onto stack.
        int CurrentByte = s_CurrentBuf->CurrentByte;
        LocalFail = PushInt(CurrentByte ? (s_CurrentBuf->NoUnicode ? CurrentByte : JotStrlenBytes(s_CurrentBuf->CurrentRec->text, CurrentByte)) : 0);
        break; }

      case 'Z': { //OZ Zoom (change to buffer) pointer popped off stack.
        long long Arg1;
        struct Buf *DestBuf;
         
        Arg1 = PopInt(&LocalFail);
        if (LocalFail)
          break;
        if (Arg1 == '~') {
          if ( ((struct bufFrame *)(&s_Stack[s_LastFloatingBufPtr]))->BufPtr == s_LastFloatingBuf )
            DestBuf = s_LastFloatingBuf;
          else {
            LocalFail = Fail("Invalid stack item for OZ command.");
            break; } }
        else { //Switching to a primary buffer.
          if (Arg1 <= ' ' || 'a' <= Arg1) {
            LocalFail = Fail("Invalid buffer key 0x%X", Arg1);
            break; }
          DestBuf = GetBuffer(Arg1, OptionallyNew); }
        if (DestBuf == NULL) {
          LocalFail = Fail("Uninitialized buffer");
          break; }
        if ((DestBuf->CurrentRec) == NULL)
          GetRecord(DestBuf, 1, "");
        s_CurrentBuf = DestBuf;
        break; }

      case 'R': { //OR shift Right (left if -ve) no. of bytes popped off stack.
        long long Chrs =  PopInt(&LocalFail);
        int Bytes;
        if (LocalFail)
          break;
        Bytes = JotRecLenChrs(s_CurrentBuf, Chrs);
        if ( ! Bytes && Chrs)
          LocalFail = Fail("OR failed, shift offset greater than availabl characters.");
        s_CurrentBuf->CurrentByte += Bytes;
        s_CurrentBuf->SubstringLength = 0;
        break; }

      case 'E': { //OE Erase no. of characters specified by top of stack.
        long long Chrs =  PopInt(&LocalFail);
        int Bytes, To, From;
        
        if (LocalFail)
          break;
        if (0 <= Chrs) {
          Bytes = JotRecLenChrs(s_CurrentBuf, Chrs);
          To = s_CurrentBuf->CurrentByte;
          From = To + Bytes; }
        else {
          Bytes = JotRecLenChrs(s_CurrentBuf, Chrs);
          To = s_CurrentBuf->CurrentByte + Bytes;
          From = s_CurrentBuf->CurrentByte;
          s_CurrentBuf->CurrentByte += Bytes; }
          
        if (From == To) {
          if (Chrs != INT_MAX) 
            LocalFail = 1;
          break; }
          
        s_CurrentBuf->SubstringLength = From-To;
        LocalFail = SubstituteString(s_CurrentBuf, NULL, -1);
        break; }

      case 'M': { //OM Move (backwards if -ve) no. of lines popped off stack.
        long long Arg1 =  PopInt(&LocalFail);
        if (LocalFail)
          break;
        LocalFail |= AdvanceRecord(s_CurrentBuf, Arg1);
        break; }
         
      default:
        LocalFail = RunError("Unknown stack operator");
        break; } }
         
      break;

    case 'Z': { //Z - Zoom to another buffer.
      char Chr = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      struct Buf *DestBuf;
      
      if (Chr == '~') {
        if ( (DestBuf = GetBuffer(Chr, NeverNew)) ) {
          s_LastFloatingBufPtr = s_StackPtr-1;
          s_LastFloatingBuf = ((struct bufFrame *)(&s_Stack[s_LastFloatingBufPtr]))->BufPtr; } }
      else {
        if ( (Chr <= ' ') || ('z' < Chr) ) {
          LocalFail = Fail("Invalid buffer key");
          break; }
        DestBuf = GetBuffer(Chr, OptionallyNew); }
      if ( ! DestBuf) {
        LocalFail = 1;
        break; }
      if ( ! DestBuf->CurrentRec)
        GetRecord(DestBuf, 1, "");
      s_CurrentBuf = DestBuf;
      break; }

    case '%': { //% - Percent-commands block.
      char BufferKey = '\0';
       
      s_CommandChr = FetchIntArgFunc((struct AnyArg **)&ThisArg);
      BufferKey = FetchIntArgFunc((struct AnyArg **)&ThisArg);

      switch (s_CommandChr) {
      case 'O': { //%O - Write buffer to file.
        char ResolvedPathName[StringMaxChr], OpenMode[3];
        FILE *File = NULL;
        char Path[StringMaxChr];
        char *Append = "w";
        int Binary = 0;
        
        while (ThisArg->next) { //The last arg being the pathname string.
          int Qual = FetchIntArgFunc(&ThisArg);
          switch (Qual) {
          case O_append:
            Append = "a";
            break;
            
          case O_binary:
            Binary = FetchIntArgFunc(&ThisArg);
            break; } }
          
        FetchStringArgPercent(Path, NULL, (struct AnyArg**)&ThisArg);
        ResolvedPathName[0] = '\0';
        if (UCEncoding_Plain < s_CurrentBuf->UCEncoding)
          Binary = 1;
        strcpy(OpenMode, Append);
        strcat(OpenMode, Binary ? "b" : "");
        
        if (s_RecoveryMode) {
          if (s_Verbose & Verbose_NonSilent)
            Message(NULL, "Writing to \"%s\" was disabled", s_CurrentBuf->PathName); }
        else { //Really do the write.
          CrossmatchPathnames(0, &File, OpenMode, Path, s_CurrentBuf->PathName, "./", ResolvedPathName);
          if ( ! File) {
            LocalFail = Fail("Failed to open file \"%s\" for writing.", ResolvedPathName);
            break; }
          if (s_Verbose & Verbose_NonSilent) {
            Prompt("Writing to \"%s\"", ResolvedPathName);
            if (s_TTYMode)
              ScrollConsole(FALSE, FALSE); }
          LocalFail |= JotWrite(File, Binary, s_CurrentBuf);
          if (fclose(File))
            LocalFail = 1; }
        s_CurrentBuf->UnchangedStatus |= SameSinceIO;
        break; }

      case 'C': { //%C - Exit writing new file.
        char MessageText[StringMaxChr];
        FetchStringArgPercent(MessageText, NULL, &ThisArg);
        if (s_CurrentBuf->BufferKey != '.') {
          LocalFail = RunError("Can only %%C from main buffer.");
          break; }
        if (s_RecoveryMode) {
          Message(NULL, "Writing to \"%s\" was disabled", s_CurrentBuf->PathName);
          break; }
        if (MessageText[0])
          ExitNormally("Normal exit");
        else {
          ExitNormally(MessageText); }
        break; }

      case 'A': { //%A - Exit without writing file.
        int ExitStatus = BufferKey;
        char MessageText[StringMaxChr];
        //Don't call FetchStringArgPercent() as it's important to pass the indirect reference to TidyUp().
        char *GivenString = ((ThisArg->type == StringArgType) && (((struct StringArg *)ThisArg)->String)[0] ? ((struct StringArg *)ThisArg)->String : "Edit abandoned");
        
        if (TidyUp(ExitStatus, GivenString)) //TidyUp() fails if there's an error or buffers to be written.
          LocalFail = TRUE;
        break; }

      case 'R': { //%R - Run a command file.
        int Arg1Len = 0, QualType = FetchIntArgFunc((struct AnyArg **)&ThisArg);
        char Args[StringMaxChr], RawPath[StringMaxChr];
        struct BacktraceFrame ThisBacktraceFrame;
        
        s_BacktraceFrame->LastCommand = s_CurrentCommand;
        ThisBacktraceFrame.LastCommand = NULL;
        ThisBacktraceFrame.prev = s_BacktraceFrame;
        ThisBacktraceFrame.type = ScriptFrame;
        s_BacktraceFrame = &ThisBacktraceFrame;
        FetchStringArgPercent(RawPath, NULL, (struct AnyArg**)&ThisArg);
        FetchStringArgPercent(Args+Arg1Len, NULL, (struct AnyArg**)&ThisArg);
        
        LocalFail = SwitchToComFile(RawPath, QualType, Args, s_CurrentCommand);
        
        s_CurrentCommand = s_BacktraceFrame->LastCommand;
        s_BacktraceFrame = ThisBacktraceFrame.prev;
        s_BacktraceFrame->LastCommand = NULL;
        if (s_Return) {
          s_Return = FALSE;
          s_ExitBlocks = 0; }
        break; }

      case 'I': { //%I - secondary input.
        char *FileName = s_CurrentBuf->PathName;
        char Actual[StringMaxChr], NameString[StringMaxChr], *PathName = NameString;
        int AppendQual = 0, InsertQual = 0, HoldQual = 0, SectionQual = 0, FSectionQual = 0;
        int SeekQual = 0, BlockQual = 0, BytesQual = 0, RecordsQual = 0;
        long long SeekOffset = 0;
        long long Bytes = 0ll;
        int Records = 0;
        char SectionKey[30], FSectionKey[30];
        struct Rec *BreakLine;    //This record marks the point where the original record was broken to insert the new text.
        int BinaryRecordSize = 0, NewRecCount;
        int LineNumber, CurrentByte = -1;
        struct Buf *DestBuf;
        int FileDesc = 0;
        
        if (BufferKey == '\0')
          BufferKey = s_CurrentBuf->BufferKey;
        while (ThisArg->next) { //The last arg being the pathname string.
          int Qual = FetchIntArgFunc(&ThisArg);
          switch (Qual) {
          case I_section:
            SectionQual = TRUE;
            FetchStringArgPercent(SectionKey, NULL, &ThisArg);
            break;
            
          case I_fsection:
            FSectionQual = TRUE;
            FetchStringArgPercent(FSectionKey, NULL, &ThisArg);
            break;
            
          case I_seek:
            SeekQual = TRUE;
            FetchIntArg(&SeekOffset, &ThisArg);
            break;
            
          case I_bytes:
            BytesQual = TRUE;
            FetchIntArg(&Bytes, &ThisArg);
            break;
            
          case I_block:
            BlockQual = TRUE;
            FetchIntArg(&Bytes, &ThisArg);
            break;
            
          case I_records:
            RecordsQual = TRUE;
            FetchIntArg((long long *)&Records, &ThisArg);
            break;
            
          case I_binary:
            FetchIntArg((long long *)&BinaryRecordSize, &ThisArg);
            break;
            
          case I_insert:
            InsertQual = TRUE;
            break;
            
          case I_append:
            AppendQual = TRUE;
            break;
            
          case I_hold:
            HoldQual = TRUE;
            break; } }
            
        if (SectionQual || FSectionQual) {
          if (SeekQual || BytesQual || BlockQual || RecordsQual) {
            LocalFail = SynError(s_CommandBuf, "%%%%I -section cannot be used with -seek, -bytes, -records or -block");
            break; } }
            
        FetchStringArgPercent(PathName, NULL, &ThisArg);

        Actual[0] = '\0';
        if (SectionQual) {
          struct SetsectHTabObj *Object;
          SeekQual = BytesQual = TRUE;
          if ( ! (Object = QuerySection(SectionKey, s_CurrentBuf)) ) {
            LocalFail = Fail("Missing or invalid entry for key \"%s\"", SectionKey);
            break; }
          SeekOffset = (off64_t)Object->Seek;
          Bytes = Object->Bytes; }
        if (FSectionQual) {
          struct SetfsectHTabObj *Object;
          SeekQual = BytesQual = TRUE;
          if ( ! (Object = QueryFSection(FSectionKey, s_CurrentBuf)) ) {
            LocalFail = Fail("Missing or invalid entry for key \"%s\"", FSectionKey);
            break; }
          PathName = Object->PathName;
          SeekOffset = (off64_t)Object->Seek;
          Bytes = Object->Bytes; }
        if ( ! CheckBufferKey(BufferKey)) {
          LocalFail = Fail("Invalid buffer key \'%c\'", BufferKey);
          break; }
        if (HoldQual) {
          if ( (DestBuf = GetBuffer(BufferKey, NeverNew)) ) {
            if (DestBuf->IOBlock)
              FileDesc = DestBuf->IOBlock->HoldDesc; } }
        if ( ! FileDesc)
          CrossmatchPathnames(&FileDesc, NULL, "r", PathName[0] ? PathName : FileName, FileName, "./", Actual);
        if ( ! FileDesc) {
          LocalFail = Fail("Failed to crossmatch file, Path: %s, name: %s", PathName, FileName);;
          if (s_JournalHand)
            UpdateJournal("%%File open failed", NULL);
          break; }
        if (HoldQual && ! (AppendQual || InsertQual)) {
          if ( ! (DestBuf = GetBuffer(BufferKey, OptionallyNew)) ) {
            LocalFail = 1;
            break; }
          if (DestBuf->CurrentRec) {
            DestBuf->CurrentRec = DestBuf->FirstRec;
            while (DestBuf->FirstRec)
              FreeRecord(DestBuf, DeleteNotAdjust); } }
        else {
          if ( ! (DestBuf = GetBuffer(BufferKey, (InsertQual || AppendQual) ? OptionallyNew : AlwaysNew)) ) {
            LocalFail = 1;
            break; } }
        s_CurrentBuf = DestBuf;
        if ( ! BlockQual) {
          if (AppendQual || InsertQual) {
            if ( ! s_CurrentBuf->CurrentRec)
              GetRecord(s_CurrentBuf, 1, "");
            if ( (s_CurrentBuf->FirstRec == s_CurrentBuf->FirstRec->next) && ( ! (s_CurrentBuf->FirstRec->text)[0]) ) {
              //Empty file - just read it in, -append and -insert are irrelevant to this read.
              FreeRecord(s_CurrentBuf, DeleteNotAdjust);
              AppendQual = InsertQual = FALSE; }
            else {
              LineNumber = DestBuf->LineNumber;
              CurrentByte = DestBuf->CurrentByte;
              if (AppendQual) {
                s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec->prev;
                CurrentByte = strlen(s_CurrentBuf->CurrentRec->text);
                s_CurrentBuf->CurrentByte = CurrentByte; }
              BreakRecord(s_CurrentBuf);
              s_CurrentBuf->CurrentRec = s_CurrentBuf->CurrentRec->prev;
              BreakLine = s_CurrentBuf->CurrentRec->next->next; } } }
        if (s_RecoveryMode) { //In recovery mode prompt then read the journal-area pathname.
          if (OpenJournalFile(&FileDesc, NULL, Actual)) {
            GetRecord(s_CurrentBuf, 1, "");
            LocalFail = 1;
            break; } }
            
        //N.B. ReadNewRecord() adds the new record *after* the current record.
        if (FileDesc) {
#if defined(VC)
          NewRecCount = ReadBuffer(NULL, NULL, FileDesc, FALSE, Actual, s_CurrentBuf, BinaryRecordSize, SeekOffset, (BytesQual||BlockQual ? &Bytes : NULL), BlockQual ? TRUE : FALSE, Records);
#else
          NewRecCount = ReadBuffer(NULL,       FileDesc, FALSE, Actual, s_CurrentBuf, BinaryRecordSize, SeekOffset, (BytesQual||BlockQual ? &Bytes : NULL), BlockQual ? TRUE : FALSE, Records);
#endif
          if (HoldQual)
            DestBuf->IOBlock->HoldDesc = FileDesc;
          else
            close(FileDesc); }
        else {
          if (s_JournalHand)
            UpdateJournal("%%Read file failed", NULL);
          LocalFail = 1;
          break; }
          
        if ( ! BlockQual) {
          if (AppendQual || InsertQual) {
            if ( ! s_CurrentBuf->FirstRec)
              GetRecord(s_CurrentBuf, 1, "");
            if ( ! NewRecCount) {
              s_CurrentBuf->CurrentRec = s_CurrentBuf->CurrentRec->prev;
              JoinRecords(s_CurrentBuf);
              s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec;
              FreeRecord(s_CurrentBuf, DeleteNotAdjust); }
            else if (AppendQual || InsertQual) {
              s_CurrentBuf->CurrentRec = BreakLine->prev->prev;
              JoinRecords(s_CurrentBuf);
              AdvanceRecord(s_CurrentBuf, -NewRecCount);
              JoinRecords(s_CurrentBuf);
              if (AppendQual) {
                s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec;
                s_CurrentBuf->LineNumber = 1;
                s_CurrentBuf->CurrentByte = 0;
                s_CurrentBuf->SubstringLength = 0; }
              else { //InsertQual
                s_CurrentBuf->LineNumber = LineNumber;
                s_CurrentBuf->CurrentByte = CurrentByte;
                s_CurrentBuf->UnchangedStatus = 0; } } }
          else
            s_CurrentBuf->UnchangedStatus |= SameSinceIO; }
        if (BytesQual) {
          if (Bytes == 0ll)
            break;
          else
            LocalFail = 1; }
        if (NewRecCount < 0)
          LocalFail = 1;
        break; }

      case 'D':   //%D - Define a buffer from console or last console command.
      case 'G': { //%G - Get - but from current command file (or console).
        int AppendQual = FALSE, BreakQual = FALSE, IsCurrentBuf = (BufferKey == s_CurrentBuf->BufferKey);
        char Label[StringMaxChr];
        struct Buf *DestBuf = NULL;
        int LabelLength, Qual = -1, CopyNotRead = FALSE;
         
        while (ThisArg && (ThisArg->type == IntArgType) ) {
          Qual = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          if (Qual == D_append)
            AppendQual = TRUE;
          else if (Qual == D_break)
            BreakQual = TRUE;
          else if (Qual == G_copyNotRead)
            CopyNotRead = TRUE; }
       
        if ( ! CopyNotRead && s_CommandChr == 'G') {
          FetchStringArgPercent(Label, &LabelLength, &ThisArg);
          Label[LabelLength] = ':';
          Label[LabelLength+1] = '\0'; }
        if ( ( ! CheckBufferKey(BufferKey)) || ( ( ! (DestBuf = GetBuffer(BufferKey, AppendQual ? OptionallyNew : AlwaysNew))) ) ) {
          LocalFail = 1;
          break; }
        if (IsCurrentBuf && BufferKey != '~')
          s_CurrentBuf = DestBuf;
        if (DestBuf->EditLock & ReadOnly) {
          LocalFail = Fail("Attempt to redefine a readonly buffer");
          break; }
        DestBuf->UnchangedStatus = 0;
        DestBuf->PathName = (char *)JotMalloc(30);
        sprintf(DestBuf->PathName, "[ Set by %%%c command ]", s_CommandChr);
          
        if (Qual == D_literal) { //Literal string just copy it over and exit.
          char * String = ((struct StringArg *)ThisArg)->String;;
          ThisArg = ThisArg->next;
          LocalFail |= GetRecord(DestBuf, strlen(String)+1, String);
          break; }
          
        if (s_CommandChr == 'D') { //%D
          char String[StringMaxChr];
          
          String[0] = '\0';
          LocalFail |= FetchStringArgPercent(String, NULL, &ThisArg);
          LocalFail |= GetRecord(DestBuf, strlen(String)+1, String);
          if (AppendQual) {
            AdvanceRecord(DestBuf, -1);
            JoinRecords(DestBuf); }
          if (BreakQual) {
            DestBuf->CurrentByte = strlen(DestBuf->CurrentRec->text);
            BreakRecord(DestBuf); } }
        else if (s_CommandChr == 'G') { //%G
          if (AppendQual)
            DestBuf->CurrentRec = DestBuf->FirstRec->prev;
          if (CopyNotRead) { //Copying records picked up from macro or function.
            while (ThisArg) { //Line copying loop - note we just want the raw untranslated text for %G hence we don't call FetchStringArgPercent.
              GetRecord(DestBuf, ((struct StringArg *)ThisArg)->StringLength+1, ((struct StringArg *)ThisArg)->String);
              ThisArg = ThisArg->next; } }
          else { //Reading from command file.
            for ( ; ; ) { // Insert records loop. 
              char *Record;
              if (s_EditorInput == s_EditorConsole) {
                Prompt("%s", s_PromptString[0] ? s_PromptString : "> "); }
              else
                s_EditorInput->LineNo++;
              if ( ! ReadNewRecord(DestBuf, s_EditorInput->FileHandle, FALSE)) {
                if ( s_JournalHand && (s_EditorInput == s_EditorConsole) )
                  UpdateJournal("%%G-command terminated", NULL);
                break; }
              Record = (DestBuf->CurrentRec->text);
              if ( (strstr(Record, Label) == Record) && LabelLength == strlen(Record)-1 && Record[LabelLength] == ':') { //A valid termination label found.
                if (DestBuf->FirstRec == DestBuf->FirstRec->next)
                  DestBuf->CurrentRec->text[0] = '\0';
                else
                  LocalFail |= FreeRecord(DestBuf, AdjustForwards);
                break; }
              if (LocalFail)
                break; } }
          ResetBuffer(DestBuf); }
        break; }

      case 'E': { //%E - Execute following CLI command line.
        int Status = 0, EQual, PipeOpt = FALSE, InteractiveOpt = FALSE, ExitCommandLen = 0, Length;
        char RawCommandString[StringMaxChr], ExitCommand[StringMaxChr], *CommandString = RawCommandString;
        
        while (ThisArg->next) { 
          EQual = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          if (EQual == E_exit)
            FetchStringArgPercent(ExitCommand, &ExitCommandLen, &ThisArg);
          else if (EQual == E_pipe)
            PipeOpt = TRUE;
          else if (EQual == E_interactive)
            InteractiveOpt = TRUE;
          else
            break; }
        FetchStringArgPercent(RawCommandString, &Length, &ThisArg);
        
#if defined(VC)
        
        if (BufferKey != '\0') {
          char *NameString;
          struct Buf *DestBuf;
          FILE *fd = NULL;
          int StreamInBuf = FALSE, ReadOK;
          HANDLE InHand = NULL, OutHand = NULL;
          HANDLE ProcessId;
          int OutDesc = 0;
           
          DestBuf = GetBuffer(BufferKey, AlwaysNew);
          if (DestBuf == NULL) {
            LocalFail = Fail("Invalid buffer key");
            break; }
          
          NameString = (char *)JotMalloc(Length+25);
          sprintf(NameString, "[ From CLI command %s ]", RawCommandString);
          
          if (s_RecoveryMode) {
            OpenJournalFile(&OutDesc, NULL, NameString); }
          else { //Not a recovery session.
            if (PipeOpt)
              StreamInBuf = TRUE;
            else if (InteractiveOpt) {
              if (s_JournalHand) {
                LocalFail = Fail("Interactive I/O not available  with journal files.");
                free(NameString);
                break; }
              
              if (DestBuf->IOBlock) {
                if (DestBuf->IOBlock->OutPipe) {
                  LocalFail = Fail("Buffer ( %c ) already has interactive I/O set up.", DestBuf->BufferKey);
                  free(NameString);
                  break; } }
              else {
                DestBuf->IOBlock = (struct IOBlock *)JotMalloc(sizeof(struct IOBlock));
                DestBuf->IOBlock->HoldDesc = 0;
                DestBuf->IOBlock->ExitCommand = NULL;
                DestBuf->IOBlock->BucketBlock = NULL;
                DestBuf->IOBlock->WaitforSequence = NULL;
                DestBuf->IOBlock->Code = NULL;
                if (ExitCommandLen) {
                  if (DestBuf->IOBlock->ExitCommand)
                    free(DestBuf->IOBlock->ExitCommand);
                  DestBuf->IOBlock->ExitCommand = strdup(ExitCommand+1); }
                else
                  DestBuf->IOBlock->ExitCommand = NULL; } }
            
            if (InteractiveOpt)
              LocalFail = OpenWindowsCommand(&InHand, &OutHand, &ProcessId, StreamInBuf, (InteractiveOpt), CommandString);
            else { 
              ExpandEnv(CommandString, StringMaxChr);
              LocalFail = OpenWindowsCommand(NULL, &OutHand, NULL, StreamInBuf, (InteractiveOpt), CommandString); }
            if (LocalFail)
              break; }
            
          if ( ! CheckBufferKey(BufferKey)) {
            LocalFail = Fail("Invalid buffer key \'%c\'", BufferKey);
            free(NameString);
            break; }
          s_CurrentBuf = DestBuf;
          ReadOK = (0 < ReadBuffer(fd, OutHand, OutDesc, TRUE, NameString, s_CurrentBuf, 0, 0ll, NULL, FALSE, 0));
          
          if (InteractiveOpt) {
            DestBuf->PathName = NameString;
            DestBuf->IOBlock->NxInteractiveBuf = s_FirstInteractiveBuf;
            s_FirstInteractiveBuf = DestBuf;
            DestBuf->IOBlock->IoFd = InHand;
            DestBuf->IOBlock->OutPipe = OutHand; 
            DestBuf->IOBlock->ProcessId = ProcessId; 
            if (InteractiveReadRecs())
              JotUpdateWindow();
            break; }
            
          free(NameString);
          if ( ! ReadOK) {
            LocalFail = 1;
            break; }
          if (DestBuf->IOBlock->IoFd)
            CloseHandle(DestBuf->IOBlock->IoFd);
          if (DestBuf->IOBlock->OutPipe)
            CloseHandle(DestBuf->IOBlock->OutPipe);
          if (s_RecoveryMode)
            close(OutDesc); }
        else { //Just run and, if it writes to it's stdout, let it mess up the screen.
          Status = system(CommandString); }

#else
        if (BufferKey != '\0') {
          char NameString[StringMaxChr+23];
          struct Buf *DestBuf;
          FILE *FileDesc = NULL;
          int StreamInBuf = FALSE; 
          int OutDesc = 0;
          pid_t ChildPid;
          
          DestBuf = GetBuffer(BufferKey, AlwaysNew);
          if (DestBuf == NULL) {
            LocalFail = Fail("Invalid buffer key");
            break; }
          
          sprintf(NameString, "[ From CLI command %s ]", RawCommandString);
            
          if ( ! CheckBufferKey(BufferKey)) {
            LocalFail = Fail("Invalid buffer key \'%c\'", BufferKey);
            break; }
          
          if (s_RecoveryMode)
            OpenJournalFile(&OutDesc, NULL, NameString);
          else { //Not a recovery session.
            int i;
            char *ArgV[20];
            
            if (PipeOpt) {
              FILE *fd = NULL;
              LocalFail = DoublePopen(NULL, &OutDesc, &ChildPid, s_CurrentBuf, CommandString);
              if (OutDesc < 0) {
                LocalFail = Fail("Could not open reply stream");
                break; }
              s_CurrentBuf = DestBuf;
              if (ReadBuffer(fd, OutDesc, TRUE, NameString, DestBuf, 0, 0ll, NULL, FALSE, 0) < 0) {
                LocalFail = RunError("Failed to read back from the command.");
                break; } }
                
            else if (InteractiveOpt) {
              int MasterFd;
              struct winsize ws;
              struct sigaction act;
              int get_master_pty(char **);
              int get_slave_pty(char *);
              char PtyName[StringMaxChr];
              if (s_JournalHand) {
                LocalFail = Fail("Interactive I/O not available  with journal files.");
                break; }
              
              ArgV[0] = strtok(CommandString, " ");
              for ( i=1; i<20; i++) {
                ArgV[i] = strtok(NULL, " "); 
                if ( ! ArgV[i])
                  break; }
                  
              if (DestBuf->IOBlock) {
                if (DestBuf->IOBlock->IoFd) {
                  LocalFail = Fail("Buffer ( %c ) already has interactive I/O set up.", DestBuf->BufferKey);
                  break; } }
              else {
                DestBuf->IOBlock = (struct IOBlock *)JotMalloc(sizeof(struct IOBlock));
                DestBuf->IOBlock->ChildPid = 0;
                DestBuf->IOBlock->HoldDesc = 0;
                DestBuf->IOBlock->ExitCommand = NULL;
                DestBuf->IOBlock->BucketBlock = NULL;
                DestBuf->IOBlock->WaitforSequence = NULL;
                DestBuf->IOBlock->Code = NULL;
                if (ExitCommandLen) {
                  if (DestBuf->IOBlock->ExitCommand)
                    free(DestBuf->IOBlock->ExitCommand);
                  DestBuf->IOBlock->ExitCommand = strdup(ExitCommand+1); }
                else
                  DestBuf->IOBlock->ExitCommand = NULL; }
                
              if ( ((MasterFd = posix_openpt(O_RDWR)) <= 0) || (0 < grantpt(MasterFd)) || (0 < unlockpt(MasterFd)) ) {
                LocalFail = RunError("Something went wrong opening the pty.");
                break; }
              else
                strcpy(PtyName, ptsname(MasterFd));
                
              act.sa_handler = SigwinchHandler;
              sigemptyset(&(act.sa_mask));
              act.sa_flags = 0;
              if ( (sigaction(SIGWINCH, &act, NULL) < 0) ||  ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0 || ((ChildPid = fork()) < 0) ) {
                LocalFail = RunError("Something went wrong Getting terminal characteristics and forking");
                break; }
              if (ChildPid == 0) { //We're in the child process.
                int slave = -1;
 
                close(MasterFd);
                if ((slave = open(PtyName, O_RDWR)) < 0) {
                  LocalFail = RunError("Something went wrong opening the slave pty (1).");
                  break; }
                if ( (setsid() < 0) || (ioctl(slave, TIOCSCTTY, NULL)) ) {
                  LocalFail = RunError("Something went wrong opening the slave pty (2).");
                  break; }
                dup2(slave, STDIN_FILENO);
                dup2(slave, STDOUT_FILENO);
                dup2(slave, STDERR_FILENO);
 
                if (2 <= slave)
                  close(slave);
   
                if (ioctl(STDOUT_FILENO, TIOCSWINSZ, &ws) < 0) {
                  LocalFail = RunError("could not restore window size");
                  break; }
  
                execvp(CommandString, ArgV); 
                exit(1); }
 
              //In the parent process.
              GetRecord(DestBuf, 1, "");
              s_CurrentBuf = DestBuf;
              DestBuf->IOBlock->NxInteractiveBuf = s_FirstInteractiveBuf;
              s_FirstInteractiveBuf = DestBuf;
              DestBuf->IOBlock->IoFd = MasterFd;
              DestBuf->IOBlock->ChildPid = ChildPid;
              if ( ! s_CurrentBuf->CurrentRec)
                GetRecord(s_CurrentBuf, 1, "");
              break; }
            else {  //Simple case run the command and pick up the result in the specified buffer.
              if ( (FileDesc = popen(CommandString, "r")) < 0 ) {
                LocalFail = RunError("Failed to open the command.");
                GetRecord(DestBuf, 1, "");
                break; }
              s_CurrentBuf = DestBuf; } }
          if (ReadBuffer(FileDesc, OutDesc, TRUE, NameString, DestBuf, 0, 0ll, NULL, FALSE, 0) < 0) {
            LocalFail = RunError("Failed to read back from the command.");
            break; }
          if (FileDesc)
            Status = pclose(FileDesc);
          if (s_RecoveryMode)
            close(OutDesc);
          if (StreamInBuf)
            waitpid(ChildPid, NULL, 0); }
        else { //Just run and, if it writes to it's stdout, let it mess up the screen.
          Status = system(CommandString);
          clearok(mainWin, TRUE); }
#endif
        if (Status != 0)
          LocalFail = 1;
        break; }

      case 'P': { //%P - Pipe text to subprocess (pipe set up by %E with & modifier).
        char OnReplyString[StringMaxChr], CLICommand[StringMaxChr];
        struct Buf *DestBuf;
        int Qual, OnReplyLength = 0, Length, Timeout = 0, nocr = FALSE;
#if defined(VC)
        struct timeb TimeBlock;
        int Bytes;
        struct _timeb tb;
#else
        struct timespec TimeSpec;
#endif
        
        if (BufferKey != '\0')
          DestBuf = GetBuffer(BufferKey, NeverNew);
        else
          DestBuf = s_CurrentBuf;
        if ( ! DestBuf || ! DestBuf->IOBlock || ! DestBuf->IOBlock->IoFd) {
          LocalFail = Fail("Buffer ( %c ) has not been set up a %%E... -interactive ...  command.", DestBuf ? DestBuf->BufferKey : BufferKey);
          break; }
        if (DestBuf->IOBlock->WaitforSequence)
          FreeBuffer(DestBuf->IOBlock->WaitforSequence);
        DestBuf->IOBlock->WaitforSequence = NULL;
        FreeSequence(&DestBuf->IOBlock->Code);
        DestBuf->IOBlock->Code = NULL;
          
        while (ThisArg->next) {
          Qual = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          if (Qual == P_waitfor) {
            struct Buf *ReplyBuf = GetBuffer('r', AlwaysNew);
            FetchStringArgPercent(OnReplyString, &OnReplyLength, &ThisArg);
            GetRecord(ReplyBuf, OnReplyLength+1, OnReplyString);
            DestBuf->IOBlock->WaitforSequence = ReplyBuf; }
          else if (Qual == P_nocr)
            nocr = TRUE;
          else if (Qual == P_timeout)
            Timeout = FetchIntArgFunc((struct AnyArg **)&ThisArg); }
        FetchStringArgPercent(CLICommand, &Length, &ThisArg);
#if defined(VC)
        _ftime64_s(&tb);
        DestBuf->IOBlock->StartTime  = tb.time;
        if ( ! InteractiveReadRecs())
          LocalFail = TRUE;
        else {
          DestBuf->SubstringLength = 0;
          SubstituteString(DestBuf, CLICommand, -1);
          JotUpdateWindow();
          WriteFile(DestBuf->IOBlock->IoFd, CLICommand, Length, &Bytes, NULL);
          if ( ! nocr)
            WriteFile(DestBuf->IOBlock->IoFd, "\n", 1, &Bytes, NULL);
          DestBuf->CurrentRec = DestBuf->FirstRec->prev;
          DestBuf->CurrentByte = strlen(DestBuf->CurrentRec->text); }
#else
        clock_gettime(CLOCK_REALTIME, &TimeSpec);
        DestBuf->IOBlock->StartTime  = (long long)TimeSpec.tv_sec;
        DestBuf->IOBlock->Timeout = Timeout;
        if ( ! InteractiveReadRecs())
          LocalFail = TRUE;
        else {
          JotUpdateWindow();
          write(DestBuf->IOBlock->IoFd, CLICommand, Length);
          if ( ! nocr)
            write(DestBuf->IOBlock->IoFd, "\n", 1);
          DestBuf->CurrentRec = DestBuf->FirstRec->prev;
          DestBuf->CurrentByte = strlen(DestBuf->CurrentRec->text); }
#endif
        break; }

      case 'F': { //%F - General find
        int Direction = 1, StartLine = 0, StartChr = 0, EndLine, EndChr, EndByte = -1, LineCount = -1;
        char BackQual = FALSE, ReverseQual = FALSE, QuickSearch = TRUE, SubstituteQual = FALSE;
        char StartLineQual = FALSE, StartChrQual = FALSE, EndLineQual = FALSE, EndChrQual = FALSE;
        char MultiSearchArg = FALSE, RexQual = FALSE, SetapertureQual = FALSE, ApertureQual = FALSE, RestartQual = FALSE;
        char LocalTab = s_TableSeparator;
        char Expr[StringMaxChr], SubsString[StringMaxChr];
        if (BufferKey == s_CurrentBuf->BufferKey) {
          LocalFail = SynError(s_CommandBuf, "Destination buffer for %%F is current buffer");
          break; }
        while (ThisArg->next) { //The last arg being the pathname string.
          int Qual = FetchIntArgFunc(&ThisArg);
          switch (Qual) {
          case F_substitute:
            QuickSearch = FALSE;
            SubstituteQual = TRUE;
            FetchStringArgPercent(SubsString, NULL, &ThisArg);
            break;
          case F_rex:
            RexQual = TRUE;
            QuickSearch = FALSE;
            break;
          case F_setaperture:
            SetapertureQual = TRUE;
            QuickSearch = FALSE;
            break;
          case F_tab:
            LocalTab = FALSE;
            FetchStringArgPercent(Expr, NULL, &ThisArg);
            LocalTab = Expr[1];
            break;
          case F_aperture:
            ApertureQual = TRUE;
            QuickSearch = FALSE;
            break;
          case F_restart:
            RestartQual = TRUE;
            break;
          case F_back:
            BackQual = TRUE;
            Direction = -1;
            break;
          case F_reverse:
            ReverseQual = TRUE;
            break;
          case F_startline:
            StartLineQual = TRUE;
            StartLine = FetchIntArgFunc(&ThisArg);
            break;
          case F_startchr:
            StartChrQual = TRUE;
            StartChr = FetchIntArgFunc(&ThisArg);
            break;
          case F_endline:
            EndLineQual = TRUE;
            QuickSearch = FALSE;
            EndLine = FetchIntArgFunc(&ThisArg);
            break;
          case F_endChr:
            EndChrQual = TRUE;
            QuickSearch = FALSE;
            EndChr = FetchIntArgFunc(&ThisArg);
            break;
          case F_any:
            QuickSearch = FALSE;
            MultiSearchArg = F_any;
            break;
          case F_first:
            QuickSearch = FALSE;
            MultiSearchArg = F_first;
            break;
          case F_all:
            QuickSearch = FALSE;
            MultiSearchArg = F_all;
            break; } }
        if (LocalFail)
          continue;
        FetchStringArgPercent(Expr, NULL, &ThisArg);
        if (Expr[0] != '\0')
          strncpy(s_GenFindString, Expr, StringMaxChr);
        else
          strncpy(Expr, s_GenFindString, StringMaxChr);
        if (SetapertureQual) {
          if (StartLineQual)
            s_StartLineSave = StartLine;
          if (StartChrQual)
            s_StartChrSave = StartChr;
          if (EndLineQual)
            s_EndLineSave = EndLine;
          if (EndChrQual)
            s_EndChrSave = EndChr;
          break; }
        else if (ApertureQual) {
          if (RestartQual)
            EndLineQual = EndChrQual = TRUE;
          else {
            StartLineQual = StartChrQual = EndLineQual = EndChrQual = TRUE;
            StartLine = s_StartLineSave;
            StartChr = s_StartChrSave; }
          EndLine = s_EndLineSave;
          EndChr = s_EndChrSave;
          if (StartLine<0 || StartChr<0 || EndLine<0 || EndChr<0) {
            LocalFail = Fail("Aperture not completely defined.");
            break; } }
          
        if (StartLineQual)     //Start line is specified.
          AdvanceRecord(s_CurrentBuf, StartLine-s_CurrentBuf->LineNumber);
        else
          StartLine = s_CurrentBuf->LineNumber;
        if (EndLineQual) {
          int CurrentLine = s_CurrentBuf->LineNumber;
          if (CurrentLine < EndLine )
            LineCount = EndLine-CurrentLine;
          else if ( EndLine < CurrentLine ) {
            Direction = -1;
            LineCount = CurrentLine-EndLine; }
          else
            LineCount = 0;
          if (EndLineQual && EndChrQual) { //The end-line character no. was specified - go there and do the Chrs to Bytes conversion.
            int OrigLine = s_CurrentBuf->LineNumber, OrigByte = s_CurrentBuf->CurrentByte;
            AdvanceRecord(s_CurrentBuf, EndLine-s_CurrentBuf->LineNumber);
            EndByte = JotStrlenChrs(s_CurrentBuf->CurrentRec->text, EndChr);
            AdvanceRecord(s_CurrentBuf, OrigLine-s_CurrentBuf->LineNumber);
            s_CurrentBuf->CurrentByte = OrigByte; } }
        else if (BackQual) {
          Direction = -1;
          LineCount = -1; }
        if (StartChrQual)
          s_CurrentBuf->CurrentByte = JotStrlenChrs(s_CurrentBuf->CurrentRec->text, StartChr);
        else if (StartLineQual && BackQual) //For backwards searches StartChr defaults to the end of the line.
          s_CurrentBuf->CurrentByte = strlen(s_CurrentBuf->CurrentRec->text);
          //
#if defined(NoRegEx)
        RexQual = FALSE;
#endif
        if (RexQual && Expr[1] == '\0') { //This is bollox but single-character RegEx expressions can result in a valgrind failure.
          LocalFail = Fail("Single-character RegEx searches are not allowed.");
          break; }
        if ( QuickSearch ) { //No end point - use the simplest, speediest search function.
          if (SimpleSearchBuffer(s_CurrentBuf, s_GenFindString, Direction, ReverseQual) == 0 )
            LocalFail = 1; }

        else if (SubstituteQual) {
          int SubsStringLength = strlen(SubsString+1), InstanceCount, TotalInstancesLen, PrevMatchEnd, PrevSubsEnd;
          int OrigTextLen, NewTextLen, Global = FALSE, ConditionalLineCount = -1, ConditionalEndByte = -1;
          char *OrigText, *NewText;
          
          MultiSearchArg = F_first;
          if ( ! StartLineQual && ! StartChrQual && ! EndLineQual && ! EndChrQual) { //It's a global substitution.
            s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec;
            StartLine = 1;
            s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec;
            s_CurrentBuf->LineNumber = 1;
            StartChr = 0;
            EndLine = -1;
            EndByte = -1;
            Global = TRUE; }
          s_CurrentBuf->CurrentByte = StartChr;
          
          for ( ; ; ) { //Initial instances search loop.
            if ( ! Global) {
              if ( (ConditionalLineCount = EndLine-s_CurrentBuf->LineNumber) < 0 )  //Have we reached the last line in the range?
                break;
              ConditionalEndByte = ConditionalLineCount==0 ? EndByte : -1; }        //Only set the ConditionalEndByte on the last line.
            if( ! (RexQual ?
                RegexSearchBuffer(s_CurrentBuf, BufferKey, s_GenFindString, Direction, ConditionalLineCount, EndByte) :
                GeneralSearchBuffer(s_CurrentBuf, s_GenFindString, LocalTab, MultiSearchArg, Direction, ConditionalLineCount, EndByte, ReverseQual)) )
              break;
            
            TotalInstancesLen = s_CurrentBuf->SubstringLength;
            InstanceCount = 1;
            OrigText = s_CurrentBuf->CurrentRec->text;
            while (RexQual ?
                RegexSearchBuffer(s_CurrentBuf, BufferKey, s_GenFindString, Direction, 0, ConditionalEndByte) :
                GeneralSearchBuffer(s_CurrentBuf, s_GenFindString, LocalTab, MultiSearchArg, Direction, 0, ConditionalEndByte, ReverseQual) ) {
              InstanceCount++;
              TotalInstancesLen += s_CurrentBuf->SubstringLength; }
                  
            PrevMatchEnd = 0;
            PrevSubsEnd = 0;
            OrigTextLen = s_CurrentBuf->CurrentRec->MaxLength-2;
            while ((0 < OrigTextLen) && ! OrigText[OrigTextLen] )
              OrigTextLen--;
            NewTextLen = OrigTextLen - TotalInstancesLen + SubsStringLength*InstanceCount + 1;
            NewText = (char *)JotMalloc(NewTextLen+1);
            s_CurrentBuf->CurrentByte = (s_CurrentBuf->LineNumber == StartLine) ? StartChr : 0;
            
            while (RexQual ?
              RegexSearchBuffer(s_CurrentBuf, BufferKey, s_GenFindString, Direction, 0, ConditionalEndByte) :
              GeneralSearchBuffer(s_CurrentBuf, s_GenFindString, LocalTab, MultiSearchArg, Direction, 0, ConditionalEndByte, ReverseQual) )
            { //Text-copying loop.
              memcpy(NewText + PrevSubsEnd, OrigText + PrevMatchEnd, s_CurrentBuf->CurrentByte - PrevMatchEnd + 1);
              memcpy(NewText + PrevSubsEnd + s_CurrentBuf->CurrentByte - PrevMatchEnd, SubsString + 1, SubsStringLength);
              PrevSubsEnd += s_CurrentBuf->CurrentByte - PrevMatchEnd + SubsStringLength;
              PrevMatchEnd = s_CurrentBuf->CurrentByte + s_CurrentBuf->SubstringLength; }
                
            //Found all matching strings in this record - copy tail end text and replace the text string.
            strcpy(NewText + PrevSubsEnd, OrigText + PrevMatchEnd);
            free(OrigText);
            s_CurrentBuf->CurrentRec->text = NewText;
            s_CurrentBuf->CurrentRec->MaxLength = NewTextLen;
            s_CurrentBuf->CurrentByte = 0;
            AdvanceRecord(s_CurrentBuf, 1); }
              
          //Substitution done.
          s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec; 
          s_CurrentBuf->CurrentByte = 0; 
          s_CurrentBuf->LineNumber = 1; }
        else { //No global substitution - plod through with a general search
          if ( ! RexQual) { //Plod through with a general search
            if (GeneralSearchBuffer(s_CurrentBuf, s_GenFindString, LocalTab, MultiSearchArg, Direction, LineCount, EndByte, ReverseQual) == 0)
              LocalFail = 1; }
          else { //It's a regular expression.
            if (RegexSearchBuffer(s_CurrentBuf, BufferKey, s_GenFindString, Direction, LineCount, EndByte) == 0)
              LocalFail = 1;
            break; } }
        break; }

      case 'U':  //%U - Undo last substitution.
        if (s_CurrentBuf->EditLock & ReadOnly) {
          LocalFail = RunError("Attempt to modify a readonly buffer");
          break; }
        s_CurrentBuf->UnchangedStatus = 0;
         
        LocalFail = SubstituteString(s_CurrentBuf, s_FindString, -1);
        break;

      case 'L': { //%L - set terminal line Length and, optionally, terminal height.
        int NewWidth, NewHeight, RealTermHeight, RealTermWidth;
         
        if (s_TTYMode)
          break;
#if defined(VC)
        GetConsoleScreenBufferInfo(hStdout, &csbiInfo);
        {
          SMALL_RECT SrWindow = csbiInfo.srWindow;
          RealTermWidth = SrWindow.Right-SrWindow.Left+1;
          RealTermHeight = SrWindow.Bottom-SrWindow.Top+1; }
#elif defined(LINUX)
        RealTermHeight = getmaxy(mainWin);
        RealTermWidth = getmaxx(mainWin);
#endif
        NewWidth = FetchIntArgFunc((struct AnyArg **)&ThisArg);
        NewHeight = FetchIntArgFunc((struct AnyArg **)&ThisArg);
        
        if (0 < NewWidth) {
          if (RealTermWidth < NewWidth) {
            LocalFail = Fail("Width given in %%L expression exceedes corresponding terminal dimension - reverting to %d", s_TermWidth);
            break; }
          s_TermWidth = NewWidth; }
        else
          s_TermWidth = RealTermWidth;
          
        if (0 < NewHeight) {
          if (RealTermHeight < NewHeight) {
            LocalFail = Fail("Height given in %%L expression exceedes corresponding terminal dimension - reverting to %d", s_TermHeight);
            break; }
          if (NewHeight <= s_ConsoleTop) {
            LocalFail = Fail("Given height (%d) is insufficient for current window configuration.", NewHeight);
            break; }
          s_TermHeight = NewHeight; }
        else
          s_TermHeight = RealTermHeight;
          
        if (s_RecoveryMode) { //In recovery mode, read the term dimensions from recovery script.
          int Width, Height;
          char Buf[StringMaxChr];
          s_asConsole->LineNo++;
          fgets(Buf, StringMaxChr, s_asConsole->FileHandle);
          if (sscanf(Buf, "%%%%Recovery terminal size: %d %d\n", &Width, &Height)) {
            Message(NULL, "Read term size %dx%d", Width, Height);
            s_TermWidth = Width;
            s_TermHeight = Height; } }
        else if (s_JournalHand) {
          char Buf[100];
          sprintf(Buf, "%%%%Recovery terminal size: %d %d", s_TermWidth, s_TermHeight);
          UpdateJournal(Buf, NULL); }
        LocalFail |= InitTermTable();
#if defined(LINUX)
        clear();
        refresh(); 
#endif
        break; }

      case '%':  //%% - Comment line.
        break;

      case '~': { //%~ - Insert or display control character.
        wchar_t Chr;
        char String[MB_CUR_MAX];
        
        if (ThisArg) {
          Chr = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          sprintf(String, "%lc", Chr);
          if (s_CurrentBuf->EditLock & ReadOnly) {
            LocalFail = RunError("Attempt to modify a readonly buffer");
            break; }
           
          if ( ( ! s_CurrentBuf->NoUnicode) && (mblen(String, 1) <= 0) )
            LocalFail = Fail("The %%~ operation would have introduced invalid unicode into the buffer.");
          else {
            LocalFail = SubstituteString(s_CurrentBuf, String, -1);
            s_CurrentBuf->UnchangedStatus = 0; } }
        else
          Message(NULL, "\"%X\"", (int)(s_CurrentBuf->CurrentRec->text)[s_CurrentBuf->CurrentByte]);
        break; }

      case 'W': //%W - Set up a screen window.
        LocalFail = DoWindows(BufferKey, &ThisArg);
        break;

      case 'X': {  //%X - Exit with a run-time error message.
        char MessageText[StringMaxChr];
        FetchStringArgPercent(MessageText, NULL, &ThisArg);
        Message(NULL, "%s", MessageText);
        LocalFail = 1;
        s_BombOut = TRUE;
        break; }

     case 'S': { //%S - System settings and utilities.
        int SettingType = FetchIntArgFunc((struct AnyArg **)&ThisArg);
        
        switch (SettingType) {
        case S_commandmode: { //%s=commandmode - controls command/type-into-screen mode.
          int OrigValue = s_CommandMode;
          int Value = FetchHexArgFunc((struct AnyArg **)&ThisArg);
          int Modifier = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          
          if (Modifier == '+')
            s_CommandMode = s_CommandMode ^ Value; 
          else if (Modifier == '-')
            s_CommandMode &= ~Value;
          else
            s_CommandMode = Value;
          
          Message(NULL, "Command mode was %X, now set to %X", OrigValue, s_CommandMode);
          break; }

        case S_recoverymode: { //%s=recoverymode <n> -  disables %O command, %I prompts and reads filename from console (the recovery script).
          int Value = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          struct Buf *PrimaryBuffer = GetBuffer('.', NeverNew);
          
          s_RecoveryMode = Value;
          Message(NULL, "%%O and %%I %sabled", s_RecoveryMode ? "dis" : "en");
          if ( ! PrimaryBuffer->PathName)
            Disaster("Primary buffer pathname is undefined");
          if (Value != 0) {
            s_OriginalSession = FALSE;
            if ( ! s_asConsole) {
              LocalFail = RunError("Recovery script must be run with -asConsole set");
              break; }
            s_JournalPath = (char *)JotMalloc(strlen(PrimaryBuffer->PathName)+8);
            strcpy(s_JournalPath, PrimaryBuffer->PathName);
            strcat(s_JournalPath, ".jnl/"); }
          else {
            char FullPath[StringMaxChr], Actual[StringMaxChr], Temp[StringMaxChr], TimeStamp[30];
            time_t DateTime;
            struct stat Stat;
            
            Actual[0] = '\0';
            if (stat(s_JournalPath, &Stat) ) {
              LocalFail = RunError("Journal directory \"%s\" does not exist, continuing without journal.", s_JournalPath);
              free(s_JournalPath);
              s_JournalPath = NULL; }
            else {
              strcpy(Actual, s_JournalPath);
              strcat(Actual, "history.txt");
              CrossmatchPathnames(0, &s_JournalHand, "a", s_JournalPath, "history.txt", NULL, Actual);
              if ( s_JournalHand == NULL)
                Disaster("Failed to open journal file \"%s\"", FullPath);
              DateTime = time(NULL);
              strftime(TimeStamp, 100, "%d/%m/%y, %H:%M:%S", localtime(&DateTime));
              sprintf(Temp, "%%%%Recovery session pid %d, at %s", getpid(), TimeStamp);
              UpdateJournal(Temp, NULL); } }
          break; }

        case S_guardband: { //%s=guardband <n> - Set window guardband.
          int Value = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          s_GuardBandSize = Value;
          if (s_Verbose & Verbose_NonSilent)
            Message(NULL, "Window guardband set to %d", s_GuardBandSize);
          break; }

        case S_console: { //%s=console <n> - Set the overall console-area size.
          int Value = (int)FetchIntArgFunc((struct AnyArg **)&ThisArg);
          if (s_TermHeight < Value) {
            Fail("Console size cannot be greater than the terminal height.");
            Value = 0; }
          s_MaxConsoleLines = Value;
          break; }

        case S_verbose: { //%s=verbose <n> - redefine verbose mode.
          int Value = FetchHexArgFunc((struct AnyArg **)&ThisArg);
          int Sign = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          
          if (Sign == '+')
            s_Verbose |= Value;
          else if (Sign == '-')
            s_Verbose &= ~Value;
          else
            s_Verbose = Value;
            
          break; }

        case S_mousemask: { //%s=mousemask <x> - Define the mouse mask
          int Value = FetchHexArgFunc((struct AnyArg **)&ThisArg);
          int Sign = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          
#if defined(VC)
          if (Value)
            s_MouseMask = -1;
          else
            s_MouseMask = 0;
          SetConsoleMode(hStdin, s_MouseMask ? ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT : 0);
#else
          if (Sign == '+')
            s_MouseMask |= Value;
          else if (Sign == '-')
            s_MouseMask &= ~Value;
          else
            s_MouseMask = Value;
          mousemask(s_MouseMask, NULL);
#endif
          break; }

        case S_setenv: { //%s=setenv <name> <value> - set env variable for the session.
          char EnvName[StringMaxChr], EnvValue[StringMaxChr];
          FetchStringArgPercent(EnvName, NULL, &ThisArg);
          FetchStringArgPercent(EnvValue, NULL, &ThisArg);
          if (setenv(EnvName, EnvValue, -1))
            LocalFail = 1;
          break; }

        case S_case: { //%s=case {0|1} -  controls case sensitivity for F command.
          int Value = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          s_CaseSensitivity = Value;
          if (s_Verbose & Verbose_QuiteChatty) {
            if (s_CaseSensitivity)
              Message(NULL, "Case sensitivity on"); 
            else
              Message(NULL, "Case sensitivity off"); }
          break; }

        case S_system: //%s=system {0|1} - preserves current value of default strings.
          s_SystemMode = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          break;

        case S_traceskip: { //%s=traceskip - skip-over blocks, macros, functions, scripts etc.
          int RelativeSkipLevel = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          s_MaxTraceLevel = s_TraceLevel-RelativeSkipLevel;
          s_TraceSkipped = FALSE;
          break; }

        case S_tracedefault: { //%s=tracedefault <x> - Set default trace vector set by t command.
          int Value = FetchHexArgFunc((struct AnyArg **)&ThisArg);
          int Sign = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          int OrigValue = s_DefaultTraceMode;
          
          if (Sign == '+')
            s_DefaultTraceMode |= Value;
          else if (Sign == '-')
            s_DefaultTraceMode &= ~Value;
          else
            s_DefaultTraceMode = Value;
          if (s_Verbose & Verbose_NonSilent)
            Message(NULL, "Trace-default mode was %X, now set to %X", OrigValue, s_DefaultTraceMode);
          break; }

        case S_trace: { //%s=trace <x> - set Trace mode. Prefix argument with '+' to XOR with current value.
          int Value = FetchHexArgFunc((struct AnyArg **)&ThisArg);
          int Sign = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          int OrigValue = s_TraceMode;
          
          if (Sign == '+')
            s_TraceMode |= Value;
          else if (Sign == '-')
            s_TraceMode &= ~Value;
          else
            s_TraceMode = Value;
          if (s_Verbose & Verbose_NonSilent)
            Message(NULL, "Trace mode was %X, now set to %X", OrigValue, s_TraceMode);
          if (s_JournalHand && (s_TraceMode != OrigValue) ) {
            if (s_TraceMode & Trace_AllCommands ) {
              char Temp[20];
              sprintf(Temp, "%%%%Debugger on %d", s_JournalDataLines);
              UpdateJournal(Temp, NULL); }
            else
              UpdateJournal("%%Debugger off", NULL); }
          break; }

        case S_commandcounter: { //%s=commandcounter <n> - sets the debug counter to some positive value.
          long long Value;
          FetchIntArg(&Value, (struct AnyArg **)&ThisArg);
          s_CommandCounter = s_CommandCounterInit = 0ll-Value;
          if (s_Verbose & Verbose_NonSilent)
            Message(NULL, "Command sequence will be interrupted after %lld commands", Value);
          break; }

        case S_tab: { //%s=tab <n> - table-column separator character.
          char TempString[StringMaxChr];
          FetchStringArgPercent(TempString, NULL, &ThisArg);
          s_TableSeparator = TempString[0] ? TempString[0] : '\t';
          break; }

        case S_sleep: { //%s=sleep <n> - sleeps for n miliseconds.
          long long Value;
          FetchIntArg(&Value, (struct AnyArg **)&ThisArg);
          usleep(Value*1000);
          break; }

        case S_commandstring: { //%s=commandstring <string> - return modified string back to command-line input.
          char TempString[StringMaxChr];
          FetchStringArgPercent(TempString, NULL, &ThisArg);
          if (s_ModifiedCommandString) {
            LocalFail = RunError("Recursive use of %s=commandstring is not supported.");
            break; }
          s_ModifiedCommandString = (char *)JotMalloc(strlen(TempString)+1);
          strcpy(s_ModifiedCommandString, TempString);
          if (s_JournalHand)
            UpdateJournal("%% %s=commandstring", NULL);
          break; }

        case S_prompt: { //%s=prompt <string> - sets the prompt string used by G command.
          char TempString[StringMaxChr];
          int Length;
          FetchStringArgPercent(TempString, &Length, &ThisArg);
          if (TempString[0]) {
            strncpy(s_PromptString, TempString, Length);
            s_PromptString[Length] = '\0'; }
          else
            strcpy(s_PromptString, "> ");
          break; }

        case S_on_key: { //%s=on_key <CommandString> - specifies a command string to be executed after a command
          char TempString[StringMaxChr];
          int Length, AfterQual = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          FetchStringArgPercent(TempString, &Length, &ThisArg);
          if ( ! AfterQual) {  //Command string to run before command.
            if (s_OnKeyCommandBuf)
              FreeBuffer(s_OnKeyCommandBuf);
            if (Length) {
              s_OnKeyCommandBuf = GetBuffer('k', AlwaysNew);
              GetRecord(s_OnKeyCommandBuf, Length+1, TempString); }
            else
              s_OnKeyCommandBuf = NULL; }
          else {  //Command string to run after command.
            if (s_AfterKeyCommandBuf)
              FreeBuffer(s_AfterKeyCommandBuf);
            if (Length) {
              s_AfterKeyCommandBuf = GetBuffer('a', AlwaysNew);
              GetRecord(s_AfterKeyCommandBuf, Length+1, TempString); }
            else
              s_AfterKeyCommandBuf = NULL; }
          break; }

        case S_on_int: { //%s=on_int <CommandString> - specifies a command string to be executed on {Ctrl+C} interrupt
          char TempString[StringMaxChr];
          int Length;
          FetchStringArgPercent(TempString, &Length, &ThisArg);
          if (s_OnIntCommandBuf)
            FreeBuffer(s_OnIntCommandBuf);
          if (Length) {
            s_OnIntCommandBuf = GetBuffer('t', AlwaysNew);
            GetRecord(s_OnIntCommandBuf, Length+1, TempString); }
          else
            s_OnIntCommandBuf = NULL;
          break; }

        case S_setmouse: { //%s=setmouse <value> - sets the mouse coordinates returned by OP and %q=window commands.
          struct Window *Win = s_FirstWindow;
          
          s_MouseBufKey = s_CurrentBuf->BufferKey;
          s_MouseLine = s_CurrentBuf->LineNumber;
          s_Mouse_x = s_CurrentBuf->CurrentByte - s_CurrentBuf->LeftOffset;
          s_Mouse_y = -1;
          
          while (Win != NULL) { // Window loop.
            if (Win->WindowKey == '\0' || Win->WindowKey == s_CurrentBuf->BufferKey) {
              int FirstLineNo = Win->OldFirstLineNo;
              int FirstLineNumber = FirstLineNo - (Win->Bot-Win->Top+1);  //Tack alert!!! if the text is smaller than the window, this value is negative but does this matter?
              if (FirstLineNumber < s_CurrentBuf->LineNumber || s_CurrentBuf->LineNumber < FirstLineNo) { //Current line appears in this window.
                s_Mouse_y = s_CurrentBuf->LineNumber - FirstLineNumber;
                break; } }
            Win = Win->next; }
          break; }
           
        default: { 
          LocalFail = SynError(s_CommandBuf, "Invalid %%s command qualifier \"%s\"", SettingType);
          break; } }
          
        break; }

      case 'M': { //%M - Message
        struct Buf *DestBuf;
        char MessageText[StringMaxChr];
        FetchStringArgPercent(MessageText, NULL, &ThisArg);
        if (BufferKey != '\0') { //Direct message to buffer as well as console.
          if ( ! (DestBuf = GetBuffer(BufferKey, OptionallyNew))) {
            LocalFail = 1;
            break; }
          if ( ! DestBuf->FirstRec) //A newly-minted buffer with no records.
            GetRecord(DestBuf, 1, "");
          
          DestBuf->CurrentRec = DestBuf->FirstRec->prev;
          GetRecord(DestBuf, strlen(MessageText)+1, MessageText); }
        Message(NULL, "%s", MessageText);
#if defined(LINUX)
        refresh();
#endif
        break; }

      case 'B': { //%B - Buffer attributes and operations.
        char Qualifier[20];
        int SettingKey = FetchIntArgFunc((struct AnyArg **)&ThisArg);
         
        switch (SettingKey) {
        case B_unrestricted: { //As it says - no restrictions.
          s_CurrentBuf->EditLock = 0;
          break; }
        
        case B_readonly: { //Prohibits changes to this buffer.
          int NewReadonlyState = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          s_CurrentBuf->EditLock = NewReadonlyState;
          break; }
        
        case B_writeifchanged: { //Insists that changes are written out.
          int NewWriteIfChangedState = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          s_CurrentBuf->EditLock |= WriteIfChanged;
          if ( ! NewWriteIfChangedState)
            s_CurrentBuf->EditLock ^= WriteIfChanged;
          break; }
          
        case B_sameflag1: { //Sets the user change flag.
          int NewSameFlag1State = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          s_CurrentBuf->UnchangedStatus |= SameFlag1;
          if ( ! NewSameFlag1State)
            s_CurrentBuf->UnchangedStatus ^= SameFlag1;
          break; }
           
#if defined(VC)
        case B_codepage: { //Define CodePage for this buffer.
          s_CurrentBuf->CodePage = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          break; }
#endif

        case B_header: { //Define a header string for display above window.
          char RawHeader[StringMaxChr], *Header;
          int Length;
          FetchStringArgPercent(RawHeader, &Length, &ThisArg);
          if (Length) {
            Header = (char *)JotMalloc(Length+1);
            strcpy(Header, RawHeader); }
          else
            Header = NULL;
          if (s_CurrentBuf->Header) 
            free(s_CurrentBuf->Header);
          s_CurrentBuf->Header = Header;
          LocalFail = InitTermTable();
          JotUpdateWindow();
          break; }

        case B_footer: { //Define a string to replace pathname in window-separator line.
          char RawFooter[StringMaxChr], *Footer;
          int Length;
          FetchStringArgPercent(RawFooter, &Length, &ThisArg);
          if (Length) {
            Footer = (char *)JotMalloc(Length+1);
            strcpy(Footer, RawFooter); }
          else
            Footer = NULL;
          if (s_CurrentBuf->Footer) 
            free(s_CurrentBuf->Footer);
          s_CurrentBuf->Footer = Footer;
          JotUpdateWindow();
          break; }

        case B_leftoffset: { //Define LeftOffset for this buffer (leftmost character in view).
          int OrigLeftOffset = s_CurrentBuf->LeftOffset;
          int NewLeftOffset = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          if (1000000000 < NewLeftOffset) {
            LocalFail = Fail("Yikes!!! left offset setting too large (%d)", NewLeftOffset);
            break; }
          s_CurrentBuf->LeftOffset = (NewLeftOffset < 0) ? 0 : NewLeftOffset;
          if (OrigLeftOffset == s_CurrentBuf->LeftOffset) 
            break;
          LocalFail |= InitTermTable();
          JotUpdateWindow();
          break; }

        case B_tabstops:
        case B_tabcells: { //Define TabStops/TabCells - cell or column widths for this buffer.
          struct Chain { int TabStop; struct Chain *next; };
          struct Chain *FirstLink, *ThisLink, *PrevLink = NULL;
          int Index = 1, TabStop, LastTabStop = 0, *TabStops;
          
          s_CurrentBuf->TabCells = SettingKey == B_tabcells;
          if (s_CurrentBuf->TabStops) {
            free(s_CurrentBuf->TabStops);
            s_CurrentBuf->TabStops = NULL; }
          
          while (TRUE) {
            if ( ! ThisArg) {
              if ( ! PrevLink) //No tabstop setting supplied, buffer reverts to non-tabular behaviour.
                break;
              ThisLink->next = NULL;
              break; }
            TabStop = FetchIntArgFunc((struct AnyArg **)&ThisArg);
            if (PrevLink)
              ThisLink = PrevLink->next = (struct Chain *)JotMalloc(sizeof(struct Chain));
            else
              ThisLink = FirstLink = (struct Chain *)JotMalloc(sizeof(struct Chain));
            ThisLink->TabStop = TabStop;
            PrevLink = ThisLink;
            Index++; }
            
          if (Index == 1) //No TabStops. 
            break;
          s_CurrentBuf->AutoTabStops = (Index == 2) && (FirstLink->TabStop == -1);
          TabStops = (int *)JotMalloc(sizeof(int)*(s_CurrentBuf->AutoTabStops ? MaxAutoTabStops : Index));
          TabStops[0] = Index-1;
          ThisLink = FirstLink;
          Index = 1;
          
          //Copy the tabstops into the buffer, performing a sanity check.
          while(ThisLink) {
            int TabStop = ThisLink->TabStop;
            if (TabStop <= LastTabStop && 1 < Index) {
              free(TabStops);
              LocalFail = RunError("TabStops must be an ascending sequence");
              break; }
            TabStops[Index++] = LastTabStop = TabStop;
            ThisLink = (PrevLink = ThisLink)->next;
            free(PrevLink); }
             
          if (LocalFail)
            break;
          s_CurrentBuf->TabStops = TabStops; 
          LocalFail |= InitTermTable();
          JotUpdateWindow();
          break; }

        case B_tagtype: { //tagtype - define a user tag, currently only colour tags supported.
          int Foreground, Background, Length;
          struct ColourObj *NewColourObj;
          short ColourPair = 0;
          char TagName[StringMaxChr];
          
          FetchIntArgFunc((struct AnyArg **)&ThisArg);
          Foreground = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          Background = FetchIntArgFunc((struct AnyArg **)&ThisArg);
#if defined(VC)
          if (15 < Foreground || 15 < Background) {
            LocalFail = Fail("Invalid colour number for windows (must be in the range 0 to 15)");
            break; }
#else
          if (Foreground < 0 || Background < 0 || 9 < Foreground || 9 < Background) {
            LocalFail = Fail("Invalid colour number for unix (must be in the range 0 to 7)");
            break; }
#endif
          FetchStringArgPercent(TagName, &Length, &ThisArg);
          if (12 < Length) {
            LocalFail = Fail("Name string is longer than 12 characters");
            break; }
            
          NewColourObj = (struct ColourObj *)JotMalloc(sizeof(struct ColourObj));
          NewColourObj->Foreground = Foreground;
          NewColourObj->Background = Background;
          strcpy(NewColourObj->TagName, TagName);
          NewColourObj->next = s_CurrentBuf->FirstColObj;
          s_CurrentBuf->FirstColObj = NewColourObj;
#if defined(VC)
          NewColourObj->ColourPair = Foreground+(Background*16);
#else
          for (ColourPair = 0; ColourPair < s_NextColourPair; ColourPair++) { //Search the existing colour pairs for a matching pair.
            short fg, bg;
            pair_content(ColourPair, &fg, &bg);
            if (fg == Foreground && bg == Background)
              break; }
          if (ColourPair == s_NextColourPair) { //That colour pair has not yet been defined.
            if (COLOR_PAIRS-1 < s_NextColourPair) {
              LocalFail = Fail("System colour-pair limit has been reached");
              break; }
            s_NextColourPair++; }
          NewColourObj->ColourPair = ColourPair;
          init_pair(ColourPair, Foreground, Background);
#endif
          break; }

        case B_encoding: { //encoding - set the unicode encoding scheme for writing this buffer.
          char Code[StringMaxChr];
          int CodeLength;
          
          FetchStringArgPercent(Code, &CodeLength, &ThisArg);
          if (strstr(Code, "Plain") && CodeLength == 5)
            s_CurrentBuf->UCEncoding = UCEncoding_Plain;
          else if (strstr(Code, "UTF_8") && CodeLength == 5)
            s_CurrentBuf->UCEncoding = UCEncoding_UTF_8;
          else if (strstr(Code, "UTF_16LE") && CodeLength == 8)
            s_CurrentBuf->UCEncoding = UCEncoding_UTF_16LE;
          else if (strstr(Code, "UTF_16BE") && CodeLength == 8)
            s_CurrentBuf->UCEncoding = UCEncoding_UTF_16BE;
          else if (strstr(Code, "UTF_32LE") && CodeLength == 8)
            s_CurrentBuf->UCEncoding = UCEncoding_UTF_32LE;
          else if (strstr(Code, "UTF_32BE") && CodeLength == 8)
            s_CurrentBuf->UCEncoding = UCEncoding_UTF_32BE;
          else
            LocalFail = Fail("Invalid or unsupported unicode encoding tag \"%s\"", Code);
          break; }

        case B_addtag: { //addtag - applies a tag defined by %b=tagtype to the current text.
          char Name[StringMaxChr];
          struct ColourObj *ThisColourObj = s_CurrentBuf->FirstColObj;
          char OrigString[StringMaxChr], *String = NULL;
          int Length;
          struct AnyTag *ThisTag;
          int TagType = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          
          if (FetchStringArgPercent(OrigString, &Length, &ThisArg)) {
            LocalFail = 1;
            break; }
          
          ThisTag = (struct AnyTag *)JotMalloc(sizeof(struct AnyTag));
          if (TagType == B_textQual) { //It's the %b=addtag -string=<string> form
            String = (char *)JotMalloc(Length+1);
            strcpy(String, OrigString);
            ThisTag->Attr = String; }
          else { //A colour tag.
            strcpy(Name, OrigString);
            s_CurrentBuf->CurrentRec->DisplayFlag = Redraw_Line;
            while (ThisColourObj) { //Tagname search loop.
              if ((strstr(Name, ThisColourObj->TagName) == Name) && (strlen(Name) == strlen(ThisColourObj->TagName)) ) {
                ThisTag->Attr = ThisColourObj;
                break; }
              else
                ThisColourObj = ThisColourObj->next; }
            if ( ! ThisColourObj) {
              free(ThisTag);
              LocalFail = Fail("Colour tag \"%s\" does not exist in buffer %c", Name, s_CurrentBuf->BufferKey);
              break; } }
          if (s_CurrentBuf->SubstringLength < 0) {
            ThisTag->StartPoint = s_CurrentBuf->CurrentByte+s_CurrentBuf->SubstringLength;
            ThisTag->EndPoint = s_CurrentBuf->CurrentByte; }
          else {
            ThisTag->StartPoint = s_CurrentBuf->CurrentByte;
            ThisTag->EndPoint = (0<s_CurrentBuf->SubstringLength) ? s_CurrentBuf->CurrentByte+s_CurrentBuf->SubstringLength-1 : strlen(s_CurrentBuf->CurrentRec->text); }
          ThisTag->next = NULL;
          ThisTag->type = String ? TagType_Text : TagType_Colour;
          AddTag(s_CurrentBuf->CurrentRec, ThisTag);
          break; }

        case B_remove_tag: { //remove_tag - identifies a tag and removes it from the record structure.
          int TagType;
          char TagName[StringMaxChr];
          int TagStartPoint, TagEndPoint;
          struct Rec *CurrentRec = s_CurrentBuf->CurrentRec;
          struct AnyTag *ThisTag, *StartTag = NULL;
          int Qualifier = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          
          if (FetchStringArgPercent(TagName, NULL, &ThisArg)) {
            LocalFail = 1;
            break; }
          TagType = (Qualifier == B_colourQual) ? TagType_Colour : ((Qualifier == B_textQual) ? TagType_Text : TagType_Target);
          if (s_CurrentBuf->SubstringLength < 0) {
            TagStartPoint = s_CurrentBuf->CurrentByte+1;
            TagEndPoint = s_CurrentBuf->CurrentByte-s_CurrentBuf->SubstringLength-1; }
          else {
            TagStartPoint = s_CurrentBuf->CurrentByte;
            TagEndPoint = (0<s_CurrentBuf->SubstringLength) ? s_CurrentBuf->CurrentByte+s_CurrentBuf->SubstringLength-1 :  strlen(s_CurrentBuf->CurrentRec->text); }
            
          ThisTag = CurrentRec->TagChain;
          while (ThisTag) {
            if (ThisTag->type == TagType) {
              if (TagType == TagType_Colour || TagType == TagType_Text) {
                if (ThisTag->StartPoint == TagStartPoint && ThisTag->EndPoint == TagEndPoint) { 
                  if ( (TagType == TagType_Colour && ! strcmp(((struct ColourObj *)ThisTag->Attr)->TagName, TagName) ) || ( TagType == TagType_Text && ! strcmp((char *)ThisTag->Attr, TagName) ) )
                    StartTag = ThisTag; } }
              else if (TagType == TagType_Target) {
                struct JumpHTabObj *HasJumpHTabObj = ThisTag->Attr;
                if ( ! strcmp(HasJumpHTabObj->HashKey, TagName) && ThisTag->StartPoint == TagStartPoint) {
                  StartTag = ThisTag;
                  break; } } }
            ThisTag = ThisTag->next; }
            
          if ( ! StartTag) {
            LocalFail = Fail("Can't locate the tag");
            break; }
          FreeTag(CurrentRec, StartTag);
          ForceLineRefresh();
          break; }

        case B_sort: { //sort - perfom a qsort on the records in this buffer.
          if (DoSort(0, (struct AnyArg **)&ThisArg))
            LocalFail = 1;
          break; }
        
        case B_tabsort: { //tabsort - perfom a qsort on the records in this buffer.
          if (DoSort(1, (struct AnyArg **)&ThisArg))
            LocalFail = 1;
          break; }
             
        case B_unicode: { //Controls unicode support for this buffer.
          int Line, UnicodeMode = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          s_CurrentBuf->NoUnicode = (UnicodeMode == 0);
          if (s_TermTable) {
            for (Line = 0; Line <= s_TermHeight; Line +=1) {
              s_TermTable[Line] = NULL;
              JotGotoXY(0, Line);
              JotClearToEOL(); } }
#if defined(LINUX)
        refresh(); 
#endif
          break; }

        case B_pathname: { //Sets the pathName for when the buffer is written.
          int Line;
          char NewName[StringMaxChr];
           
          if (s_CurrentBuf->PathName) 
            free(s_CurrentBuf->PathName);
          FetchStringArgPercent(NewName, NULL, &ThisArg);
          s_CurrentBuf->PathName = strcpy((char *)JotMalloc(strlen(NewName)+1), NewName);
          if (s_TermTable) {
            for (Line = 0; Line <= s_TermHeight; Line +=1)
              s_TermTable[Line] = NULL; }
          JotUpdateWindow();
          break; }
          
        default:
          LocalFail = RunError("Unrecognized %%B qualifier \"%s\"", Qualifier); }
           
        break; }

      case 'H': { //%H - Hashtable maintenance.
        struct Com *ParentCommand = s_CurrentCommand;
        LocalFail = DoHash(BufferKey, &ThisArg, s_CurrentCommand);
        s_CurrentCommand = ParentCommand;
        break; }

      case 'Q': { //%Q - Query.
        int AppendQual = 0, QueryType = 0, QueryQual = 0;
        struct Buf *DestBuf = NULL, *SourceBuf = s_CurrentBuf;
        struct Rec * FirstRec = NULL;
        int SourceBufKey = SourceBuf->BufferKey, StringArgLen, NoFollowLinksQual;
        char OrigCommand[(StringMaxChr*2)+30], StringArg[StringMaxChr];
        int AddToJournal = FALSE;
        
        OrigCommand[0] = '\0';
        StringArg[0] = '\0';
        AppendQual = FetchIntArgFunc((struct AnyArg **)&ThisArg);
        QueryType = FetchIntArgFunc((struct AnyArg **)&ThisArg);
        
        if (QueryType==Q_dir || QueryType==Q_file || QueryType==Q_env) {
          //The pathnames in these three queries may contain references to the DestBuf - so we defer GetBuf until the path has been safely extracted.
          if (QueryType==Q_dir) {
            struct AnyArg *FirstQualifierArg = ThisArg;
            
            while (ThisArg && ThisArg->type == IntArgType)
              ThisArg = ThisArg->next;
            FetchStringArgPercent(StringArg, &StringArgLen, (struct AnyArg **)&ThisArg);
            ThisArg = FirstQualifierArg; }
          else if (QueryType==Q_file) {
            NoFollowLinksQual = FetchIntArgFunc((struct AnyArg **)&ThisArg);
#if defined(VC)
            NoFollowLinksQual = FALSE;
#endif
            FetchStringArgPercent(StringArg, &StringArgLen, (struct AnyArg **)&ThisArg); }
          else
            FetchStringArgPercent(StringArg, &StringArgLen, (struct AnyArg **)&ThisArg); }
        if (BufferKey != '\0') {
          if ( (BufferKey == s_CurrentBuf->BufferKey) && (QueryType==Q_buffer || QueryType==Q_system || QueryType==Q_tabstops || QueryType==Q_tabcells)) {
            LocalFail = Fail("Source and destination buffers are the same");
            break; }
          DestBuf = GetBuffer(BufferKey, AppendQual ? OptionallyNew : AlwaysNew);
          if (DestBuf) {
            s_CurrentBuf = DestBuf; }
          else {
            LocalFail = TRUE;
            break; }
          if (AppendQual && DestBuf->CurrentRec) {
            DestBuf->CurrentRec = DestBuf->FirstRec->prev;
            DestBuf->CurrentByte = strlen(DestBuf->CurrentRec->text);
            FirstRec = DestBuf->CurrentRec; } }
        else if (AppendQual) {
          LocalFail = Fail("No destination buffer specified for -append query.");
          break; }

        switch (QueryType) {
        case Q_version: { //version - print version or send it to a buffer.
          if ( ! DestBuf) {
#if defined(VC)
            Message(NULL, VERSION_STRING);
#else
            Message(NULL, "%s%s", VERSION_STRING, curses_version());
#endif
            break; }
          GetRecord(DestBuf, strlen(VERSION_STRING)+1, VERSION_STRING);
#if ! defined(VC)
          DestBuf->CurrentByte = strlen(VERSION_STRING);
          SubstituteString(DestBuf, curses_version(), -1);
#endif
          break; }

        case Q_windows: //windows - fails if not operating under windows.
#if defined(VC)
          break;
#else
          LocalFail = TRUE;
          break;
#endif

        case Q_linux: //linux fails if not operating under linux.
#if defined(VC)
          LocalFail = TRUE;
          break;
#else
          break;
#endif

        case Q_case: //case - fails if not case sensitive.
          LocalFail = ! s_CaseSensitivity;
          break;

        case Q_system: { //System settings etc.
          struct Buf *ThisBuf = s_BufferChain;
          int DataHTabObjs = 0, NoteChr, i;
          struct Rec * Record = SourceBuf->CurrentRec;
          
          //s_NoteByte indicates a byte offset - to get the character offset go back to that record.
          if (SourceBuf->LineNumber < s_NoteLine)
            for (i = SourceBuf->LineNumber; i < s_NoteLine; i++)
              Record = Record->next;
          else if (s_NoteLine < SourceBuf->LineNumber)
            for (i = SourceBuf->LineNumber; s_NoteLine < i; i--)
              Record = Record->prev;
          NoteChr = JotStrlenBytes(Record->text, s_NoteByte);
          
          if ( ! DestBuf) {
            LocalFail = TRUE;
            break; }
          AddFormattedRecord(FALSE, DestBuf, "            Trace vector = %4X", s_TraceMode);
          AddFormattedRecord(FALSE, DestBuf, "    Default trace vector = %4X", s_DefaultTraceMode);
          AddFormattedRecord(FALSE, DestBuf, "               Verbosity = %4x", s_Verbose);
          AddFormattedRecord(FALSE, DestBuf, "         Command counter = %4lld", s_CommandCounter);
          AddFormattedRecord(FALSE, DestBuf, "    Command counter init = %4lld", s_CommandCounterInit);
          AddFormattedRecord(FALSE, DestBuf, "        Case sensitivity = %4X", s_CaseSensitivity);
          AddFormattedRecord(FALSE, DestBuf, "            Command mode = %4X", s_CommandMode);
          AddFormattedRecord(FALSE, DestBuf, "              Mouse Mask = %4X", s_MouseMask);
          AddFormattedRecord(FALSE, DestBuf, "          Current buffer = %4c", SourceBuf ? SourceBuf->BufferKey : ' ');
          AddFormattedRecord(FALSE, DestBuf, "             Note buffer = %4c", s_NoteBuffer ? s_NoteBuffer->BufferKey : ' ');
          AddFormattedRecord(FALSE, DestBuf, "         Note linenumber = %4d", s_NoteLine);
          AddFormattedRecord(FALSE, DestBuf, "          Note character = %4d", NoteChr);
          AddFormattedRecord(FALSE, DestBuf, "  Abstract whole records = %4d", s_AbstractWholeRecordFlag);
          AddFormattedRecord(FALSE, DestBuf, "          Stream-in mode = %4d", s_StreamIn);
          AddFormattedRecord(FALSE, DestBuf, "         Stream-out mode = %4d", s_StreamOut);
          AddFormattedRecord(FALSE, DestBuf, "             System mode = %4d", s_SystemMode);
          AddFormattedRecord(FALSE, DestBuf, "     Exit-blocks counter = %4d", s_ExitBlocks);
          AddFormattedRecord(FALSE, DestBuf, "  Return from macro etc. = %4d", s_Return);
          AddFormattedRecord(FALSE, DestBuf, "             Find string = \"%s\"", s_FindString);
          AddFormattedRecord(FALSE, DestBuf, "          %%F find string = \"%s\"", s_GenFindString);
          AddFormattedRecord(FALSE, DestBuf, "             %%F aperture = %d:%d - %d:%d",  s_StartLineSave, s_StartChrSave, s_EndLineSave, s_EndChrSave);
          AddFormattedRecord(FALSE, DestBuf, "           Insert string = \"%s\"", s_InsertString);
          AddFormattedRecord(FALSE, DestBuf, "          Qualify string = \"%s\"", s_QualifyString);
          AddFormattedRecord(FALSE, DestBuf, "           Prompt string = \"%s\"", s_PromptString);
          if (s_OnKeyCommandBuf)
            AddFormattedRecord(FALSE, DestBuf, "                  On key = \"%s\"", s_OnKeyCommandBuf->CurrentRec->text);
          else
            AddFormattedRecord(FALSE, DestBuf, "                  On key = Not defined");
          if (s_AfterKeyCommandBuf)
            AddFormattedRecord(FALSE, DestBuf, "               After key = \"%s\"", s_AfterKeyCommandBuf->CurrentRec->text);
          else
            AddFormattedRecord(FALSE, DestBuf, "               After key = Not defined");
          if (s_OnIntCommandBuf)
            AddFormattedRecord(FALSE, DestBuf, "                  On int = \"%s\"", s_OnIntCommandBuf->CurrentRec->text);
          else
            AddFormattedRecord(FALSE, DestBuf, "                  On int = Not defined");
          AddFormattedRecord(FALSE, DestBuf, "         Invocation name = %s", s_ProgName);
          AddFormattedRecord(FALSE, DestBuf, " Initialization commands = %s", s_InitCommands ? s_InitCommands->CurrentRec->text : "");
          AddFormattedRecord(FALSE, DestBuf, "          Startup script = %s", s_StartupFileName ? s_StartupFileName : "");
#if defined(VC)
          AddFormattedRecord(FALSE, DestBuf, "                CodePage = %d", s_CodePage);
#else
          AddFormattedRecord(FALSE, DestBuf, "                  Locale = %s", s_Locale);
#endif
          AddFormattedRecord(FALSE, DestBuf, "   Table-entry separator = '0x%02X'", s_TableSeparator);
          AddFormattedRecord(FALSE, DestBuf, "     Hold screen on exit = %4s", s_HoldScreen ? "On" : "Off");
          AddFormattedRecord(FALSE, DestBuf, "            Journal path = %s", s_JournalPath);
          AddFormattedRecord(FALSE, DestBuf, "           Recovery mode = %4s", s_RecoveryMode ? "On" : "Off");
          AddFormattedRecord(FALSE, DestBuf, "Buffers:");
        
          while (ThisBuf != NULL) {
            char TruncatedRec[101];
            strncpy(TruncatedRec, ThisBuf->CurrentRec ? ThisBuf->CurrentRec->text : "(null)", 100);
            TruncatedRec[100] = '\0';
            if ( ! ThisBuf->ParentObj)
              AddFormattedRecord(FALSE, DestBuf, "  buffer %c: %4d \"%s\"", ThisBuf->BufferKey, ThisBuf->LineNumber, TruncatedRec);
            ThisBuf = ThisBuf->NextBuf; }
            
          ThisBuf = s_BufferChain;
          while (ThisBuf != NULL) {
            char TruncatedRec[101];
            strncpy(TruncatedRec, ThisBuf->CurrentRec ? ThisBuf->CurrentRec->text : "(null)", 100);
            TruncatedRec[100] = '\0';
            if (ThisBuf->ExistsInData && ThisBuf->ParentObj) { //It is possible that the original data object has been deleted.
              char Path[StringMaxChr];
              GetBufferPath(ThisBuf, Path, StringMaxChr);
              if ( ! DataHTabObjs++)
                AddFormattedRecord(FALSE, DestBuf, "Data-object buffers:");
              AddFormattedRecord(FALSE, DestBuf, "  data obj %20s: %4d \"%s\"", Path, ThisBuf->LineNumber, TruncatedRec); }
            ThisBuf = ThisBuf->NextBuf; }
          break; }

        case Q_wd: { //wd - return current directory only.
#if defined(VC)
          char trans[StringMaxChr];
#else
          char *trans;
#endif
           
          if ( ! DestBuf) {
            LocalFail = TRUE;
            break; }
#if defined(VC)
          if (getcwd(trans, StringMaxChr))
            GetRecord(DestBuf, strlen(trans)+1, trans);
#else
          if ( (trans = getenv("PWD")))
            GetRecord(DestBuf, strlen(trans)+1, trans);
#endif
          else {
            GetRecord(DestBuf, 17, " - <Undefined> - ");
            LocalFail = TRUE;
            break; }
        break; }

        case Q_heap: { //heap - report heap stats to stdout.
#if defined(LINUX)
          if ( ! DestBuf) {
            LocalFail = TRUE;
            break; }
          struct mallinfo MallInfo = mallinfo();
          AddFormattedRecord(FALSE, DestBuf, "    arena = %d", MallInfo.arena);
          AddFormattedRecord(FALSE, DestBuf, "  ordblks = %d", MallInfo.ordblks);
          AddFormattedRecord(FALSE, DestBuf, "   smblks = %d", MallInfo.smblks);
          AddFormattedRecord(FALSE, DestBuf, "    hblks = %d", MallInfo.hblks);
          AddFormattedRecord(FALSE, DestBuf, "   hblkhd = %d", MallInfo.hblkhd);
          AddFormattedRecord(FALSE, DestBuf, "  usmblks = %d", MallInfo.usmblks);
          AddFormattedRecord(FALSE, DestBuf, "  fsmblks = %d", MallInfo.fsmblks);
          AddFormattedRecord(FALSE, DestBuf, " uordblks = %d", MallInfo.uordblks);
          AddFormattedRecord(FALSE, DestBuf, " fordblks = %d", MallInfo.fordblks);
          AddFormattedRecord(FALSE, DestBuf, " keepcost = %d", MallInfo.keepcost);
#endif
        break; }

        case Q_backtrace: //Writes diagnostic backtrace to nominated buffer.
          LocalFail = Backtrace(DestBuf);
          break;

        case Q_history: { //history - dumps history to specified buffer.
          struct Rec *CommandRec = s_EditorConsole->CommandBuf->FirstRec->next;
          if ( ! DestBuf) {
            LocalFail = TRUE;
            break; }
          while (CommandRec != s_EditorConsole->CommandBuf->FirstRec) {
            AddFormattedRecord(FALSE, DestBuf, "%s", CommandRec->text);
            CommandRec = CommandRec->next; }
            
          DestBuf->CurrentRec = DestBuf->FirstRec->prev;
          break; }

        case Q_tabstops: //tabstops - pushes TRUE if tabstops are set otherwise FALSE.
          LocalFail = ! SourceBuf->TabStops;
          break;
          
        case Q_tabcells: //tabcells - pushes TRUE if tabcells are set otherwise FALSE.
          LocalFail = ! SourceBuf->TabCells;
          break;

        case Q_keys: { //keys - Sends a list of hashtable items and values to specified buffer.
          struct AnyHTabObj *ThisObj;
          char *LastPathElem, KeyName[StringMaxChr], Path[StringMaxChr];
          
          if (ThisArg && ThisArg->next->next) { //A -key=<name> qualifier was given.
            ThisArg = ThisArg->next;
            FetchStringArgPercent(KeyName, NULL, &ThisArg); }
          else
            KeyName[0] = '\0';
          FetchStringArgPercent(Path, NULL, &ThisArg); 
          if (Path[0])
            SourceBuf = QueryPath(Path, SourceBuf, &LastPathElem, NULL, NULL);
          FetchStringArgPercent(StringArg, NULL, &ThisArg); 
          
          if (KeyName[0]) { //Look up the specified hash-table key (the destination buffer key is optional for this case).
            ENTRY *FoundVal, SearchVal;
            char ParentPath[StringMaxChr];
            GetBufferPath(SourceBuf, ParentPath, StringMaxChr);
            sprintf(OrigCommand, "keys -key=%s, in buffer %s", KeyName, ParentPath);
            if ( ! SourceBuf->htab) {
              LocalFail = Fail("No hash-table associated with buffer %c", SourceBuf->BufferKey);
              break; }
              SearchVal.key = KeyName;
              SearchVal.data = NULL;
            
            if ( ! hsearch_r(SearchVal, FIND, &FoundVal, SourceBuf->htab) || ! FoundVal) {
              char ParentPath[StringMaxChr];
              GetBufferPath(SourceBuf, ParentPath, StringMaxChr);
              LocalFail = Fail("Failed to find item \"%s\" in hashtable of buffer %s", KeyName, ParentPath);
              break; }
            if ( ! FoundVal || ((struct AnyHTabObj *)FoundVal->data)->type == HTabObj_Zombie) { //A deleted entry.
              LocalFail = Fail("Entry for %s has been deleted.", KeyName);
              break; }
            if ( ! DestBuf || ! FoundVal)
              break;
            ThisObj = FoundVal->data; }
          else { //It's a request for a full report
            if ( ! DestBuf) { //The destination buffer is unspecified.
              LocalFail = (SourceBuf->htab == NULL);
              break; }
            sprintf(OrigCommand, "%s", StringArg);
            ThisObj = (struct AnyHTabObj *)SourceBuf->FirstObj; }
            
          if ( ! SourceBuf->htab) {
            AddFormattedRecord(FALSE, DestBuf, "Buffer %c has no hashtable.", SourceBuf->BufferKey);
            break; }
          else {
            if (SourceBuf->BufferKey == '~') { //Also print the pathname for this floating buffer.
              char ParentPath[StringMaxChr];
              GetBufferPath(SourceBuf, ParentPath, StringMaxChr);
              AddFormattedRecord(FALSE, DestBuf, "Keys for buffer %c (Pathname %s):", SourceBuf->BufferKey, ParentPath); }
            else
              AddFormattedRecord(FALSE, DestBuf, "Keys for buffer %c :", SourceBuf->BufferKey); }
          
          while (ThisObj) {
            if (ThisObj->type == HTabObj_Zombie) { //A deleted entry.
              struct AnyHTabObj * NewObj = ((struct ZombieHTabObj *)ThisObj)->NewObj;
              if (NewObj) {
                AddFormattedRecord(FALSE, DestBuf, "key %20s replaced by following item:", ThisObj->HashKey);
                ThisObj = NewObj;
                continue; }
              else
                AddFormattedRecord(FALSE, DestBuf, "key %20s deleted", ThisObj->HashKey); }
            else if (ThisObj->type == HTabObj_Data) { //A DataHTabObj
              struct DataHTabObj *ThisDataHTabObj = (struct DataHTabObj *)ThisObj;
              if (ThisDataHTabObj->Frame) {
                int Type = ((struct DataHTabObj *)ThisObj)->Frame->type;
                if (Type == FrameType_Int)
                  AddFormattedRecord(FALSE, DestBuf, "key %20s, (DataObj) Int %lld", ThisDataHTabObj->HashKey, ThisDataHTabObj->Frame->Value);
                else if (Type == FrameType_Float)
                  AddFormattedRecord(FALSE, DestBuf, "key %20s, (DataObj) Float %f", ThisDataHTabObj->HashKey, ((struct floatFrame *)ThisDataHTabObj->Frame)->fValue);
                else if (Type == FrameType_Buf)
                  AddFormattedRecord(FALSE, DestBuf, "key %20s, (DataObj) Buffer \"%s\"", 
                    ThisDataHTabObj->HashKey, ((struct bufFrame *)ThisDataHTabObj->Frame)->BufPtr->CurrentRec->text); }
              else //An undefined-value DataHTabObj.
                AddFormattedRecord(FALSE, DestBuf, "key %20s, (DataObj) Undefined type and value", ThisDataHTabObj->HashKey); }
            else if (ThisObj->type == HTabObj_Setsect) { //A SetsectHTabObj
              struct SetsectHTabObj *ThisSetsectHTabObj = (struct SetsectHTabObj *)ThisObj;
              AddFormattedRecord(FALSE, DestBuf, "key %20s, (SetsectObj) Seek %lld, Bytes %lld", ThisSetsectHTabObj->HashKey, ThisSetsectHTabObj->Seek, ThisSetsectHTabObj->Bytes); }
            else if (ThisObj->type == HTabObj_Setfilesect) { //A SetfsectHTabObj
              struct SetfsectHTabObj *ThisSetfsectHTabObj = (struct SetfsectHTabObj *)ThisObj;
              AddFormattedRecord(FALSE, DestBuf, "key %20s, (SetfsectObj) Section Pathname %s, Seek %lld, Bytes %lld", 
                ThisSetfsectHTabObj->HashKey, ThisSetfsectHTabObj->PathName ? ThisSetfsectHTabObj->PathName : "<Invalid pathname>", ThisSetfsectHTabObj->Seek, ThisSetfsectHTabObj->Bytes); }
            else if (ThisObj->type == HTabObj_Code) { //A code object
              struct CodeHTabObj *ThisCodeHTabObj = (struct CodeHTabObj *)ThisObj;
              AddFormattedRecord(FALSE, DestBuf, "key %25s, (CodeObj) \"%s\"", ThisCodeHTabObj->HashKey, ThisCodeHTabObj->CodeHeader->SourceRec->text);
              if (ThisCodeHTabObj->CodeHeader->CompiledCode) {
                char String[StringMaxChr];
                int i = 0, ChrNo = 0;
                struct Com *ThisCommand = ThisCodeHTabObj->CodeHeader->CompiledCode->FirstCommand;
                while ( i++ <= 5 && ThisCommand) {
                  DumpCommand(ThisCommand, &ChrNo, String);
                  ThisCommand = ThisCommand->NextCommand; }
                AddFormattedRecord(FALSE, DestBuf, "  %s", String); }
              else
                AddFormattedRecord(FALSE, DestBuf, "  No compiled code for %s", ThisCodeHTabObj->HashKey); }
            else { //It's a bog-standard JumpHTabObj
              char TruncText[50];
              memset(TruncText, '\0', 50);
              if (((struct JumpHTabObj *)ThisObj)->TargetRec) {
                strncpy(TruncText, ((struct JumpHTabObj *)ThisObj)->TargetRec->text, 30);
                TruncText[30] = '\0';
                if (((struct JumpHTabObj *)ThisObj)->Target)
                  AddFormattedRecord(FALSE, 
                    DestBuf, 
                    "key %20s, (JumpObj) buf %c, line no. %4d, chr no. %d, Rec:%s", 
                    ThisObj->HashKey, ((struct JumpHTabObj *)ThisObj)->TargetBuf->BufferKey, 
                    ((struct JumpHTabObj *)ThisObj)->LineNumber,
                    JotStrlenBytes(((struct JumpHTabObj *)ThisObj)->TargetRec->text, ((struct JumpHTabObj *)ThisObj)->Target->StartPoint),
                    TruncText);
                else
                  AddFormattedRecord(FALSE, 
                    DestBuf, 
                    "key %20s, (JumpObj) buf %c, line no. %4d, No target character, Rec:%s", 
                    ThisObj->HashKey, ((struct JumpHTabObj *)ThisObj)->TargetBuf->BufferKey, ((struct JumpHTabObj *)ThisObj)->LineNumber, TruncText); }
              else
                strcpy(TruncText, "deleted key"); }
            ThisObj = (KeyName[0]) ? NULL : ((struct JumpHTabObj *)ThisObj)->next; }
          break; }

        case Q_tags: { //tags - sends a list of record tags in current buffer to nominated buffer.
          int LineNo = 1, ErrorRecs = 0, ErrorTags = 0;
          int Here = FALSE;
          struct Rec *ThisRec;
          struct Rec *LastRec;
          char TruncText[51];
          
          if ( ! DestBuf) {
            LocalFail = TRUE;
            break; }
            
          QueryQual = FetchIntArgFunc((struct AnyArg **)&ThisArg);
          if (QueryQual == Q_hereQual) { //Report only tags active at current character position.
            ThisRec = LastRec = SourceBuf->CurrentRec;
            AddFormattedRecord(FALSE, DestBuf, "Reporting tags at Line no. %d Character no. %2d of Buffer %c", SourceBuf->LineNumber, SourceBuf->CurrentByte, SourceBuf->BufferKey);
            LineNo = SourceBuf->LineNumber;
            Here = TRUE; }
          else
            AddFormattedRecord(FALSE, DestBuf, "Reporting all tags in Buffer %c", SourceBuf->BufferKey);
            
          ThisRec = Here ? SourceBuf->CurrentRec : SourceBuf->FirstRec;
          LastRec = SourceBuf->FirstRec->prev;
                            
          while (TRUE) {    
            int ErrorRec = 0;
            struct AnyTag *ThisTag = ThisRec->TagChain;
            
            if ( Here || ThisTag ) {
              memset(TruncText, '\0', 50);
              strncpy(TruncText, ThisRec->text, 49);
              TruncText[50] = '\0';
              AddFormattedRecord(FALSE, DestBuf, "Rec %3d: \"%s\"", LineNo, TruncText); }
                
            while (ThisTag) {
              if ( ! Here || ( (ThisTag->StartPoint <= SourceBuf->CurrentByte) && ( (ThisTag->type == TagType_Target) || (SourceBuf->CurrentByte <= ThisTag->EndPoint) ) ) ) {
                int ChrFirst = JotStrlenBytes(ThisRec->text, ThisTag->StartPoint);
                if (ThisTag->type == TagType_Target) {
                  struct JumpHTabObj *Object = ThisTag->Attr;
                  int ErrorTag = (Object->TargetRec != ThisRec);
                  AddFormattedRecord(FALSE, DestBuf, "  Type target %c, Chr %2d, %s", 
                    Object->HashBuf->BufferKey, ChrFirst, ErrorTag ? "  **Error JumpHTabObj-record mismatch **" : Object->HashKey);
                  ErrorTags += ErrorTag;
                  ErrorRec |= ErrorTag; }
                else {
                  int ChrLast = JotStrlenBytes(ThisRec->text, ThisTag->EndPoint);
                  if (ThisTag->type == TagType_Text)
                    AddFormattedRecord(FALSE, DestBuf, "  Type text from chr %3d to %3d = \"%s\"", ChrFirst, ChrLast, ThisTag->Attr);
                  else if (ThisTag->type == TagType_Colour) {
                    AddFormattedRecord(FALSE, DestBuf, "  At rec %3d, chr %2d to %2d, active colour is %s",
                      LineNo, ChrFirst, ChrLast, ((struct ColourObj *)ThisTag->Attr)->TagName); }
                  else if (ThisTag->type == TagType_Font)
                    AddFormattedRecord(FALSE,  DestBuf, "  Type Font at chr %2d to %2d", ChrFirst, ChrLast); } }
              ThisTag = ThisTag->next; }
                
            if (Here)
              break;
            LineNo++;
            if (ErrorRec)
              ErrorRecs++;
            if (ThisRec == LastRec)
              break;
            ThisRec = ThisRec->next; }
          
          if (ErrorRecs)
             AddFormattedRecord(FALSE, DestBuf, "There were %d records containing inconsistent hashtable entries, %d tag/entries were in error", ErrorRecs, ErrorTags);
          break; }

        case Q_buffer: { //buffer - returns info on buffer.
          char temp[StringMaxChr];
          struct stat Stat;
          char *PathName;
          int HTabCount = 0, EndByte, Headroom = 0;
          struct AnyHTabObj *ThisAnyHTabObj;
          struct ColourObj *TagType;
          
          if ( ! DestBuf) {
            LocalFail = TRUE;
            break; }
          if (SourceBufKey == BufferKey) {
            LocalFail = TRUE;
            break; }
            
          AddToJournal = TRUE;
          TagType = SourceBuf->FirstColObj;
          PathName = SourceBuf->PathName;
          ThisAnyHTabObj = (struct AnyHTabObj *)SourceBuf->FirstObj;
          while (ThisAnyHTabObj) {
            HTabCount++;
            ThisAnyHTabObj = (struct AnyHTabObj *)ThisAnyHTabObj->next; }
          AddFormattedRecord(FALSE, DestBuf, "                  key = %c", SourceBuf->BufferKey); 
          if (SourceBuf->ParentObj) {
            char ParentPath[StringMaxChr];
            GetBufferPath(SourceBuf, ParentPath, StringMaxChr);
            AddFormattedRecord(FALSE, DestBuf, "          Parent path = %s", ParentPath); }
          AddFormattedRecord(FALSE, DestBuf, "             pathName = %s", SourceBuf->PathName); 
          if (SourceBuf->PathName) //This is the image of a real file.
            if ( ! stat(PathName, &Stat)) { 
              strftime(temp, StringMaxChr, "%Y/%m/%d, %H:%M:%S", localtime(&Stat.st_mtime));
              AddFormattedRecord(TRUE, DestBuf, "     currentDatestamp = %s", temp); }
          AddFormattedRecord(FALSE, DestBuf, "          SameSinceIO = %s", SourceBuf->UnchangedStatus & SameSinceIO ? "TRUE" : "FALSE"); 
          AddFormattedRecord(FALSE, DestBuf, "     SameSinceIndexed = %s", SourceBuf->UnchangedStatus & SameSinceIndexed ? "TRUE" : "FALSE"); 
          AddFormattedRecord(FALSE, DestBuf, "    SameSinceCompiled = %s", SourceBuf->UnchangedStatus & SameSinceCompiled ? "TRUE" : "FALSE"); 
          AddFormattedRecord(FALSE, DestBuf, "            SameFlag1 = %s", SourceBuf->UnchangedStatus & SameFlag1 ? "TRUE" : "FALSE"); 
          AddFormattedRecord(FALSE, DestBuf, "             MacroRun = %s", SourceBuf->UnchangedStatus &  MacroRun ? "TRUE" : "FALSE"); 
          AddFormattedRecord(FALSE, DestBuf, "            NoUnicode = %s", SourceBuf->NoUnicode ? "TRUE" : "FALSE"); 
          AddFormattedRecord(FALSE, DestBuf, "           LineNumber = %d", SourceBuf->LineNumber); 
          AddFormattedRecord(FALSE, DestBuf, "       OldFirstLineNo = %d", SourceBuf->OldFirstLineNo); 
          AddFormattedRecord(FALSE, DestBuf, "          CurrentByte = %d", SourceBuf->CurrentByte); 
          AddFormattedRecord(FALSE, DestBuf, "      SubstringLength = %d", SourceBuf->SubstringLength); 
          AddFormattedRecord(FALSE, DestBuf, "         wholeRecords = %s", SourceBuf->AbstractWholeRecordFlag ? "TRUE" : "FALSE"); 
          AddFormattedRecord(FALSE, DestBuf, "          predecessor = %c", (SourceBuf->Predecessor) ? SourceBuf->Predecessor->CommandBuf->BufferKey : ' '); 
          AddFormattedRecord(FALSE, DestBuf, "             editLock = %s %s", SourceBuf->EditLock & ReadOnly ? "ReadOnly" : "", SourceBuf->EditLock & WriteIfChanged ? "WriteIfChanged" : ""); 
          AddFormattedRecord(FALSE, DestBuf, "           LeftOffset = %d", SourceBuf->LeftOffset);
          AddFormattedRecord(FALSE, DestBuf, "CurrentRec max length = %d", SourceBuf->CurrentRec->MaxLength);
          EndByte = SourceBuf->CurrentRec->MaxLength-2;
          while ((0 < EndByte-Headroom) && ! SourceBuf->CurrentRec->text[EndByte-Headroom] )
            Headroom++;
          AddFormattedRecord(FALSE, DestBuf, "     Current headroom = %d", Headroom);
          AddFormattedRecord(FALSE, DestBuf, "               Header = %s", SourceBuf->Header);
          AddFormattedRecord(FALSE, DestBuf, "               Footer = %s", SourceBuf->Footer);
          AddFormattedRecord(FALSE, DestBuf, "           UCEncoding = %s", 
            (SourceBuf->UCEncoding == UCEncoding_Plain ? "Plain" :
            (SourceBuf->UCEncoding == UCEncoding_UTF_8 ? "UTF_8" :
            (SourceBuf->UCEncoding == UCEncoding_UTF_16LE ? "UTF_16LE" :
            (SourceBuf->UCEncoding == UCEncoding_UTF_16BE ? "UTF_16BE" :
            (SourceBuf->UCEncoding == UCEncoding_UTF_32LE ? "UTF_32LE" :
            (SourceBuf->UCEncoding == UCEncoding_UTF_32BE ? "UTF_32BE" : "Invalid text-encoding key" ) ) ) ) ) ) );
          AddFormattedRecord(FALSE, DestBuf, "   Pending characters = %s", SourceBuf->IOBlock ? "some" : "none"); 
          if (0 < SourceBuf->FileType)
            AddFormattedRecord(FALSE, DestBuf, "             FileType = binary, fixed length %d-byte records", SourceBuf->FileType);
          else
            AddFormattedRecord(FALSE, DestBuf, "             FileType = text");
#if defined(VC)
          AddFormattedRecord(FALSE, DestBuf, "             CodePage = %d", SourceBuf->CodePage);
#endif
          AddFormattedRecord(FALSE, DestBuf, "                        %s", TagType ? "Tag-types follow" : "No tag types");
          while (TagType) {
            AddFormattedRecord(FALSE, DestBuf, "              TagType = Name %12s, Forground Colour %d, Background colour %d", TagType->TagName, TagType->Foreground, TagType->Background);
            TagType = TagType->next; }
          AddFormattedRecord(FALSE, DestBuf, "        HashtableMode = %d", SourceBuf->HashtableMode);
          if (SourceBuf->TabStops) {
            int *TabStops = SourceBuf->TabStops;
            char Temp[StringMaxChr];
            int Chrs = 0, TabStopsIndex = 1;
            while (TabStopsIndex <= TabStops[0]) 
              Chrs += sprintf(Temp+Chrs, "%d ", TabStops[TabStopsIndex++]);
            AddFormattedRecord(FALSE, DestBuf, "             TabStops = %s", Temp); }
          else
            AddFormattedRecord(FALSE, DestBuf, "             TabStops = 0 entries");
          AddFormattedRecord(FALSE, DestBuf, "                            %s", SourceBuf->TabCells ? "Tabs treated as cell delimiters" : "Tabs treated as simple text-alignment stops");
          AddFormattedRecord(FALSE, DestBuf, "                 htab = %d entries", HTabCount);
          if (SourceBuf->IOBlock) {
            AddFormattedRecord(FALSE, DestBuf, "             HoldDesc = %d",  SourceBuf->IOBlock->HoldDesc);
#if defined(VC)
            AddFormattedRecord(FALSE, DestBuf, "                 IoFd = %d",  SourceBuf->IOBlock->IoFd);
            AddFormattedRecord(FALSE, DestBuf, "              OutPipe = %d",  SourceBuf->IOBlock->OutPipe);
            AddFormattedRecord(FALSE, DestBuf, "            ProcessId = %d",  SourceBuf->IOBlock->ProcessId);
#else
            AddFormattedRecord(FALSE, DestBuf, "                 IoFd = %d",  SourceBuf->IOBlock->IoFd);
            AddFormattedRecord(FALSE, DestBuf, "             ChildPid = %d",  SourceBuf->IOBlock->ChildPid);
#endif
            if (SourceBuf->IOBlock->ExitCommand)
              AddFormattedRecord(FALSE, DestBuf, "          ExitCommand = %s",  SourceBuf->IOBlock->ExitCommand);
            if (SourceBuf->IOBlock->WaitforSequence)
              AddFormattedRecord(FALSE, DestBuf, "     WaitFor Sequence = %s", SourceBuf->IOBlock->WaitforSequence->FirstRec->text);
            if ( ! SourceBuf->IOBlock->BucketBlock)
              AddFormattedRecord(FALSE, DestBuf, "  Buffer ( %c ) has finished reading", SourceBuf->BufferKey);
            else {
              AddFormattedRecord(FALSE, DestBuf, "  Buffer ( %c ) has pending IO operations",  SourceBuf->BufferKey);
              if (SourceBuf->IOBlock->BucketBlock->Bucket) {
                char TruncatedRec[101];
                AddFormattedRecord(FALSE, DestBuf, "Bucket 1st. byte %d", SourceBuf->IOBlock->BucketBlock->RecStartByte);
                AddFormattedRecord(FALSE, DestBuf, " Bucket end byte %d", SourceBuf->IOBlock->BucketBlock->RecEndByte);
                AddFormattedRecord(FALSE, DestBuf, "     Total bytes %d", SourceBuf->IOBlock->BucketBlock->BufBytes);
                strncpy(TruncatedRec, SourceBuf->IOBlock->BucketBlock->Bucket, 100);
                TruncatedRec[100] = '\0';
                AddFormattedRecord(FALSE, DestBuf, "      Pending text \"%s\"", TruncatedRec); } } }
          break; }

        case Q_window: { //window - returns window size and allocation.
          char String[StringMaxChr];
          int Chr_x = 0, Chr_y = 0;
          char AttrString[StringMaxChr];
          struct Window *Win = s_FirstWindow;
          int WinCount = 0;
          int ValidMousePoint = FALSE;
#if defined(VC)
          WORD AttrWords[StringMaxChr];
          int Chrs;
#endif
           
          if ( ! DestBuf) {
            LocalFail = TRUE;
            break; }
          AddFormattedRecord(FALSE, DestBuf, "  screenHeight = %d", s_TermHeight);
          AddFormattedRecord(FALSE, DestBuf, "  screenWidth = %d", s_TermWidth);
          
          while (Win != NULL) { // Window loop.
            int WindowHeight = Win->Bot-Win->Top+1;
            int SliceWidth = Win->Right-Win->Left+1;
            char Delim[100] = "";
            char Key = Win->WindowKey;
             
            if (Key == '\0') {
              if (Win->LastKey)
                sprintf(String, "currently buffer ( %c )", Win->LastKey);
              else
                sprintf(String, "currently blank       "); }
            else
              sprintf(String, "fixed on buffer  ( %c )", Key);
            if (Win->WindowType & WinType_Frozen)
              strcat(Delim, ", Frozen");
            if (Win->WindowType & WinType_Void)
              strcat(Delim, ", Void");
            if (Win->WindowType & WinType_Leftmost)
              strcat(Delim, ", Leftmost");
            if (Win->DisplayFooter)
              strcat(Delim, ", Height includes an end delimiter");
            if ((Win->WindowType & WinType_Mask) == WinType_Normal)
              AddFormattedRecord(FALSE, DestBuf, "      win:%-2d = %s, %2d lines (%-2d to %2d)%s.", WinCount++, String, WindowHeight, Win->Top, Win->Bot, Delim);
            else if ((Win->WindowType & WinType_Mask) == WinType_Slice)
              AddFormattedRecord(FALSE, DestBuf, "      win:%-2d = %s, %2d lines (%-2d to %2d), %2d column slice (%-2d to %2d)%s.", 
                WinCount++, String, WindowHeight, Win->Top, Win->Bot, SliceWidth, Win->Left, Win->Right, Delim);
            else
              AddFormattedRecord(FALSE, DestBuf, "      win:%-2d = %s, %2d lines (%-2d to %2d), %2d column popup (%-2d to %2d)%s.", 
                WinCount++, String, WindowHeight, Win->Top, Win->Bot, SliceWidth, Win->Left, Win->Right, Delim);
            Win = Win->next; }
            
          if (s_MaxConsoleLines)
            AddFormattedRecord(FALSE, DestBuf, "  Console area %d lines (%-2d to %d), extendable up to %d lines", s_TermHeight-s_ConsoleTop, s_ConsoleTop+1, s_TermHeight, s_MaxConsoleLines);
          else
            AddFormattedRecord(FALSE, DestBuf, "  Console area %d lines (%-2d to %d)", s_TermHeight-s_ConsoleTop, s_ConsoleTop+1, s_TermHeight);
          AddFormattedRecord(FALSE, DestBuf, "  Current buffer ( %c )", SourceBuf->BufferKey);
          AddFormattedRecord(FALSE, DestBuf, "");
          
          if (ValidMousePoint)
            AddFormattedRecord(FALSE, DestBuf, "Mouse cursor at: (%d, %d), Buffer %c, Line %d, Chr %d", s_Mouse_x, s_Mouse_y, s_MouseBufKey, s_MouseLine, s_Mouse_x);
          else
            AddFormattedRecord(FALSE, DestBuf, "No valid mouse point set.");
          AddFormattedRecord(FALSE, DestBuf, "");
            
          AddFormattedRecord(FALSE, DestBuf, "Screen dump follows:");
#if defined(VC)
          for (Chr_y = 0; Chr_y < s_TermHeight; Chr_y++) {
            COORD Pos = { (short)0, (short)Chr_y };
            int AttrChr = 0, Attrs, PrevAttr = s_Attrs[Attr_Normal];
            if (s_TTYMode)
              break;
            
            ReadConsoleOutputCharacter(hStdout, String, StringMaxChr, Pos, &Chrs);
            ReadConsoleOutputAttribute(hStdout, AttrWords, StringMaxChr, Pos, &Attrs);
            String[s_TermWidth] = '\0';
            AttrString[0] = '\0';
            for (Chr_x = 0; Chr_x < s_TermWidth; Chr_x++) {
              int Attr = AttrWords[Chr_x];
              if (Attr==0)
                Attr = s_Attrs[Attr_Normal];
              if (Attr != PrevAttr) {
                if (Attr==s_Attrs[Attr_Normal])
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d Normal_Text) ", Chr_x);
                else if (Attr==s_Attrs[Attr_SelSubStr])
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d Selected_Substring) ", Chr_x);
                else if (Attr==s_Attrs[Attr_WinDelim])
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d Reverse_Video) ", Chr_x);
                else if (Attr==s_Attrs[Attr_CurrChr])
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d Current_Chr) ", Chr_x);
                else if (Attr==s_Attrs[Attr_CurrCmd])
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d Current_Chr) ", Chr_x);
                else if (Attr != 0)
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d colour %X/%X) ", Chr_x, Attr&0xF, (Attr>>4)&0xF);
                PrevAttr = Attr; } }
            AddFormattedRecord(FALSE, DestBuf, "%s", String);
            if (AttrChr)
              AddFormattedRecord(FALSE, DestBuf, "Attrs: %s", AttrString); }
#else
          for (Chr_y = 0; Chr_y < s_TermHeight; Chr_y++) {
            int Byte = 0, PrevAttr = 0, AttrChr = 0;
            short PrevColourPair = 0;
            String[0] = '\0';
            AttrString[0] = '\0';
            for (Chr_x = 0; Chr_x < s_TermWidth; Chr_x++) {
              int PhysAttr = 0;
              cchar_t C_chr;
              wchar_t wch[20];
              attr_t attrs = 0;
              short ColourPair = 0, Forground, Background;
              
              mvin_wch(Chr_y, Chr_x, &C_chr);
              PhysAttr = C_chr.attr & 0xFFEFF;
              if (PhysAttr != PrevAttr) {
                getcchar(&C_chr, wch, &attrs, &ColourPair, NULL);
                if (PhysAttr==s_Attrs[Attr_Normal])
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d Normal_Text) ", Chr_x);
                else if (PhysAttr==s_Attrs[Attr_SelSubStr])
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d Selected_Substring) ", Chr_x);
                else if (PhysAttr==s_Attrs[Attr_WinDelim])
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d Reverse_Video) ", Chr_x);
                else if (PhysAttr==s_Attrs[Attr_CurrChr])
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d Current_Chr) ", Chr_x);
                else if (ColourPair != PrevColourPair) {
                  pair_content(ColourPair, &Forground, &Background);
                  AttrChr += sprintf(AttrString+AttrChr, "(X=%d colour %X/%X) ", Chr_x, Forground, Background);
                  PrevColourPair = ColourPair; }
                PrevAttr = PhysAttr; }
              Byte += wctomb(String+Byte, C_chr.chars[0]); }
            String[s_TermWidth] = '\0';
            AddFormattedRecord(FALSE, DestBuf, "%s", String);
            if (AttrChr)
              AddFormattedRecord(FALSE, DestBuf, "Attrs: %s", AttrString); }
#endif
          break; }

        case Q_time: { //time - return seconds since start of unix epoch.
          long long MSec;
#if defined(VC)
          struct timeb TimeBlock;
          _ftime64_s(&TimeBlock);
          MSec = (TimeBlock.time)*1000 + (int)(TimeBlock.millitm);
#else
          struct timespec TimeSpec;
          clock_gettime(CLOCK_REALTIME, &TimeSpec);
          MSec = ((long long)TimeSpec.tv_sec)*1000 + (((long long)TimeSpec.tv_nsec)/1000000)%1000;
#endif
          PushInt(MSec);
          break; }

        case Q_date: { //date - return todays date only.
          time_t DateTime;
          char temp[100];
           
          if ( ! DestBuf) {
            LocalFail = TRUE;
            break; }
          DateTime = time(NULL);
          AddToJournal = TRUE;
          strftime(temp, 100, "%d/%m/%y, %H:%M:%S", localtime(&DateTime));
          AddFormattedRecord(TRUE, DestBuf, "%s", temp);
          break; }

        case Q_inview: { //Is current character visible, in at least one window,  with current LeftOffset setting.
          int PerceivedChr = GetChrNo(SourceBuf);
          struct Window *Win = s_FirstWindow;
          int VisibleLimit = -1;
          
          while ( Win ) {
            if (Win->WindowKey == BufferKey || Win->WindowKey == '\0') {
              VisibleLimit = Win->Right-Win->Left;
              if ( (SourceBuf->LeftOffset <= PerceivedChr) && (PerceivedChr <= SourceBuf->LeftOffset+VisibleLimit) ) {
                PushInt(SourceBuf->LeftOffset);
                PushInt(VisibleLimit);
                PushInt(PerceivedChr);
                break; } }
              Win = Win->next; }
          if ( VisibleLimit == -1 ) {
            LocalFail = RunError("Buffer ( %c ) is not displayed in any window - %q=inview failed", SourceBuf->BufferKey);
            break; }
            
          if ( ! Win) {
            PushInt(SourceBuf->LeftOffset);
            PushInt(VisibleLimit);
            PushInt(PerceivedChr);
            LocalFail = TRUE; }
          break; }
             
        case Q_commandmode: //Query in command mode
          LocalFail = s_CommandMode;
          break;
        
        case Q_samesinceio: //Query SameSinceIO in this buffer.
          LocalFail = ((SourceBuf->UnchangedStatus) & SameSinceIO) == 0;
          break;
         
        case Q_samesinceindexed: //Query SameSinceIndexed in this buffer.
          LocalFail = ((SourceBuf->UnchangedStatus) & SameSinceIndexed) == 0;
          break;
         
        case Q_samesincecompiled: //Query SameSinceCompiled in this buffer.
          LocalFail = ((SourceBuf->UnchangedStatus) & SameSinceCompiled) == 0;
          break;
         
        case Q_sameflag1: //Query or set SameFlag1 in this buffer.
          LocalFail =  ((SourceBuf->UnchangedStatus) & SameFlag1) == 0;
          break;
         
        case Q_pid: //pid - Process ID of current process.
          AddFormattedRecord(FALSE, DestBuf, "%d", getpid());
          break;
        
        case Q_env: { //env - return translation of env variable.
          char *trans;
           
          sprintf(OrigCommand, "env %s", StringArg);
          if ( ! DestBuf) {
            LocalFail = TRUE;
            break; }
          trans = getenv(StringArg);
          AddToJournal = TRUE;
          AddFormattedRecord(TRUE, DestBuf, "%s", trans ? trans : " - <Undefined> - ");
          break; }

        case Q_dir: { //dir <path> - return directory contents.
#if defined(VC)
          char *Qualifier;
          struct __stat64 Stat;
          HANDLE DirHand = INVALID_HANDLE_VALUE;
          WIN32_FIND_DATA FileData;
          FILETIME ftCreate, ftAccess, ftWrite;
          SYSTEMTIME stUTC, stLocal;
          DWORD dwRet;
          WORD AttrWords[StringMaxChr];
#else
          DIR *PathElemDIR;
          struct dirent *Entry = NULL;
          struct stat Stat;
#endif
          char Temp[StringMaxChr];
          int PathLen = 0;
          struct AnyArg *FirstQualifierArg = ThisArg, *ThisQualifierArg;
          
          AddToJournal = TRUE;
          ExpandEnv(StringArg, StringMaxChr);
          PathLen = strlen(StringArg);
#ifdef VC
          if (__stat64(StringArg, &Stat)) {
            LocalFail = Fail("Nonexistent directory \"%s\"", StringArg); 
            break; }
          if ((Stat.st_mode & S_IFMT) != S_IFDIR) {
            LocalFail = Fail("Not a directory \"%s\"", StringArg); 
            break; }
#else
          if (stat(StringArg, &Stat)) {
            LocalFail = Fail("Nonexistent directory \"%s\"", StringArg); 
            break; }
          if ((Stat.st_mode & S_IFMT) != S_IFDIR) {
            LocalFail = Fail("Not a directory \"%s\"", StringArg); 
            break; }
#endif
          if ( ! DestBuf) {
            break; }
            
          if (s_RecoveryMode) { //Pick up records directly from recovery script.
            while ( AddFormattedRecord(TRUE, DestBuf, "") )
              { }
            s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec->prev;
            break; }
             
          Temp[0] = '\0';
          ThisQualifierArg = FirstQualifierArg;
          while (ThisQualifierArg && ThisQualifierArg->type == IntArgType) {  //Find the total length of qualifiers and construct the report's header line.
            int QualType;
            if (ThisQualifierArg && ThisQualifierArg->type != IntArgType)
              break;
            QualType = FetchIntArgFunc((struct AnyArg **)&ThisQualifierArg);
            switch (QualType) {
            case Q_atimeQual:
              strcat(Temp, "\t-atime");
              break;
            case Q_ctimeQual:
              strcat(Temp, "\t-ctime");
              break;
            case Q_gidQual:
              strcat(Temp, "\t-gid");
              break;
            case Q_inodeQual:
              strcat(Temp, "\t-inode");
              break;
            case Q_modeQual:
              strcat(Temp, "\t-mode");
              break;
            case Q_mtimeQual:
              strcat(Temp, "\t-mtime");
              break;
            case Q_sizeQual:
              strcat(Temp, "\t-size");
              break;
            case Q_uidQual:
              strcat(Temp, "\t-uid");
              break;
            default:
              break; } }
              
          if (Temp[0])
            AddFormattedRecord(TRUE, DestBuf, "%s", Temp);
          sprintf(OrigCommand, "dir%s %s", Temp, StringArg);
#ifdef VC
          strcpy(Temp, StringArg);
          strcat(Temp, "\\*");
          DirHand = FindFirstFile(Temp, &FileData);
          if (DirHand == INVALID_HANDLE_VALUE) {
            LocalFail = TRUE;
            break; }
            
          while ( 1 )
#else
          if ((Stat.st_mode & S_IFMT) != S_IFDIR) {
            LocalFail = Fail("Not a directory \"%s\"", StringArg); 
            break; }
          PathElemDIR = opendir(StringArg);
          if (PathElemDIR)
            Entry = readdir(PathElemDIR);
            
          while (Entry)
#endif
            {
            strcpy(Temp, StringArg);
            strcat(Temp, "/");
#ifdef VC
            strcpy(Temp, FileData.cFileName);
            __stat64(Temp, &Stat);
            if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
              strcat(Temp, "/");
#else
            strcat(Temp, Entry->d_name);
            stat(Temp, &Stat);
            if ((Stat.st_mode & S_IFMT) == S_IFDIR) 
              strcat(Temp, "/");
#endif
              
            ThisQualifierArg = FirstQualifierArg;
            while (ThisQualifierArg && ThisQualifierArg->type == IntArgType) {  //Some qualifiers to be processed.
              int QualType = FetchIntArgFunc((struct AnyArg **)&ThisQualifierArg);
              char FmtResult[StringMaxChr];
              switch (QualType) {
              case Q_atimeQual:
                strftime(FmtResult, 100, "\t%Y/%m/%d-%H:%M:%S", localtime(&Stat.st_atime));
                strcat(Temp, FmtResult);
                break;
              case Q_ctimeQual:
                strftime(FmtResult, 100, "\t%Y/%m/%d-%H:%M:%S", localtime(&Stat.st_ctime));
                strcat(Temp, FmtResult);
                break;
              case Q_gidQual:
                sprintf(FmtResult, "\t%4d", Stat.st_gid); 
                strcat(Temp, FmtResult);
                break;
              case Q_inodeQual:
                sprintf(FmtResult, "\t%9d", (int)Stat.st_ino); 
                strcat(Temp, FmtResult);
                break;
              case Q_modeQual:
                sprintf(FmtResult, "\t%6o", Stat.st_mode); 
                strcat(Temp, FmtResult);
                break;
              case Q_mtimeQual:
                strftime(FmtResult, 100, "\t%Y/%m/%d-%H:%M:%S", localtime(&Stat.st_mtime));
                strcat(Temp, FmtResult);
                break;
              case Q_sizeQual:
                sprintf(FmtResult, "\t%10d", (int)Stat.st_size); 
                strcat(Temp, FmtResult);
                break;
              case Q_uidQual:
                sprintf(FmtResult, "\t%4d", Stat.st_uid); 
                strcat(Temp, FmtResult);
                break;
              default:
                break; } }
                
#if defined(VC)
            AddFormattedRecord(TRUE, DestBuf, "%s", Temp);
            if (FindNextFile(DirHand, &FileData) == 0)
              break;
#else
            AddFormattedRecord(TRUE, DestBuf, "%s", Temp+1+PathLen);
            Entry = readdir(PathElemDIR);
#endif
            }
#if defined(VC)
          FindClose(DirHand);
#else
          closedir(PathElemDIR);
#endif
          if (s_JournalHand)
            UpdateJournal("%%Recovery open-ended report ends", NULL);
          ThisArg = NULL;
          break; }

        case Q_file: { //file <pathName> - a selection of info on the current or selected file.
#if defined(VC)
          struct __stat64 Stat;
#else
          struct stat Stat;
#endif
          
          AddToJournal = TRUE;
          if (SourceBufKey == BufferKey) {
            LocalFail = TRUE;
            break; }
          sprintf(OrigCommand, "file %s", StringArg);
          if ( ! StringArgLen) {
            if ( ! SourceBuf->PathName)
              strcpy(StringArg, "./");
            else
              strcpy(StringArg, SourceBuf->PathName); }
          ExpandEnv(StringArg, StringMaxChr);
#if defined(VC)
          if (NoFollowLinksQual ? lstat(StringArg, &Stat) : stat(StringArg, &Stat))
#else
          if (NoFollowLinksQual ? lstat(StringArg, &Stat) : stat(StringArg, &Stat))
#endif
            { //Error - the StringArg does not point to a valid file.
            if (DestBuf) {
              char Temp[StringMaxChr+20];
              sprintf(Temp, "Nonexistent file \"%s\"", StringArg); 
              GetRecord(DestBuf, strlen(Temp)+1, Temp); }
            LocalFail = TRUE;
            break; }
          else { //StringArg points to a valid file.
            char Temp[StringMaxChr];
            time_t Time;
            if ( ! DestBuf)
              break;
            AddFormattedRecord(TRUE, DestBuf, "                 Name = \"%s\"", StringArg);
#if defined(VC)
            AddFormattedRecord(TRUE, DestBuf, "                Drive = %c:", Stat.st_dev+'A');
#else
            AddFormattedRecord(TRUE, DestBuf, "                inode = %d", Stat.st_ino);
#endif
            AddFormattedRecord(TRUE, DestBuf, "                 Mode = %6o", Stat.st_mode);
            AddFormattedRecord(TRUE, DestBuf, "                  uid = %d", Stat.st_uid);
            AddFormattedRecord(TRUE, DestBuf, "                  gid = %d", Stat.st_gid);
#if defined(VC)
            AddFormattedRecord(TRUE, DestBuf, "                 size = %d", Stat.st_size);
#else
            AddFormattedRecord(TRUE, DestBuf, "                 size = %jd", Stat.st_size);
#endif
            AddFormattedRecord(TRUE, DestBuf, " writable by this UID = %s", (euidaccess(StringArg, W_OK) == 0) ? "yes" : "no");
            AddFormattedRecord(TRUE, DestBuf, "            directory = %d", (Stat.st_mode & S_IFMT) == S_IFDIR);
#if defined(LINUX)
            if ((Stat.st_mode & S_IFMT) == S_IFLNK) {
              char Temp[StringMaxChr];
              int Chrs;
              if ( ! (Chrs = readlink(StringArg, Temp, StringMaxChr))) {
                LocalFail = RunError("Failed to resolve link \"%s\"", StringArg);
                break; }
              Temp[Chrs] = '\0';
              AddFormattedRecord(TRUE, DestBuf, "              link to = %s", Temp); }
            else
              AddFormattedRecord(TRUE, DestBuf, "                 link = 0");
#endif
            Time = Stat.st_atime;
            strftime(Temp, 100, "%Y/%m/%d, %H:%M:%S", localtime(&Time));
            AddFormattedRecord(TRUE, DestBuf, "          Access time = %s (%ld)", Temp, Time);
            Time = Stat.st_mtime;
            strftime(Temp, 100, "%Y/%m/%d, %H:%M:%S", localtime(&Time));
            AddFormattedRecord(TRUE, DestBuf, "          Modify time = %s (%ld)", Temp, Time);
            Time = Stat.st_ctime;
            strftime(Temp, 100, "%Y/%m/%d, %H:%M:%S", localtime(&Time));
            AddFormattedRecord(TRUE, DestBuf, "        Creation time = %s (%ld)", Temp, Time); }
          break; }

        case Q_stack: //stack - dumps stack to selected buffer.
          DumpStack(DestBuf);
          break;

#if defined(VC)
        case Q_cputime: //Pushes 0.0 onto the stack.
          PushFloat(0.0);
          break;
#else
        case Q_cputime: { //Pushes time since system startup time.
          clockid_t clockid;
          struct timespec CurrentCPU;
          double DeltaTime;
          long Delta_nsec, Delta_sec;
          clock_getcpuclockid(getpid(), &clockid);
          clock_gettime(clockid, &CurrentCPU);
          Delta_sec = CurrentCPU.tv_sec-Original_sec;
          Delta_nsec = CurrentCPU.tv_nsec-Original_nsec;
          DeltaTime = Delta_sec-1+((Delta_nsec+1000000000)/1000000000.0);
          Original_sec = CurrentCPU.tv_sec;
          Original_nsec = CurrentCPU.tv_nsec;
          PushFloat(DeltaTime);
          break; }
#endif

        case Q_verify: { //Verifies the basic integrity of the buffer and the sequence.
          struct Rec *ThisRec = SourceBuf->FirstRec;
          int LineNo = 1, TagCount;
          struct AnyTag *ThisTag;
          struct Buf *ThisBuf = s_BufferChain;
          struct Buf *ActiveBuffers[256];
          int ErrorsDetected = 0;
          int TotalLen = strlen(SourceBuf->CurrentRec->text);
          
          if (DestBuf == SourceBuf) {
            LocalFail = TRUE;
            break; }
          if (DestBuf)
            AddFormattedRecord(FALSE, DestBuf, "Verifying records in buffer %c", SourceBuf->BufferKey);
          if (SourceBuf->CurrentByte < 0) {
            ErrorsDetected++;
            if (DestBuf)
              AddFormattedRecord(FALSE, DestBuf, "Current Character is negative (%d)",  SourceBuf->CurrentByte); }
          if (TotalLen < SourceBuf->CurrentByte || TotalLen < SourceBuf->CurrentByte+SourceBuf->SubstringLength) {
            ErrorsDetected++;
            if (DestBuf) {
              AddFormattedRecord(FALSE, DestBuf, "Overall string length %d, is exceeded by either (CurrentByte=%d) or CurrentByte+(SubstringLength=%d)",
                TotalLen, SourceBuf->CurrentByte, SourceBuf->SubstringLength);
              AddFormattedRecord(FALSE, DestBuf, "  for current record: \"%s\"", SourceBuf->CurrentRec->text); } }
             
          do { 
            int Length;
            if ( ! ThisRec->text) {
              if (DestBuf)
                AddFormattedRecord(FALSE, DestBuf, "Ooops! at line %d of buffer %c, the text is a null pointer.", LineNo, SourceBuf->BufferKey);
              else
                Message(NULL, "Ooops! at line %d of buffer %c, the text is a null pointer.", LineNo, SourceBuf->BufferKey);
              continue; }
            Length = strlen(ThisRec->text);
            if (ThisRec->next->prev != ThisRec) {
              ErrorsDetected++;
              if (DestBuf)
                AddFormattedRecord(FALSE, DestBuf, "Ooops! at line %d of buffer %c, the next record does not point back to this record.", LineNo, SourceBuf->BufferKey);
              else
                Message(NULL, "Ooops! at line %d of buffer %c, the next record does not point back to this record.", LineNo, SourceBuf->BufferKey);
              break; }
            if (ThisRec->MaxLength < Length) {
              ErrorsDetected++;
              if (DestBuf)
                AddFormattedRecord(FALSE, DestBuf, "Ooops! at line %d of buffer %c, the text (%d chrs.) is longer than the allocated size of the record (%d chrs).", 
                  LineNo, SourceBuf->BufferKey, Length, ThisRec->MaxLength);
              else
                Message(NULL, "Ooops! at line %d of buffer %c, the text (%d chrs.) is longer than the allocated size of the record (%d chrs).", 
                  LineNo, SourceBuf->BufferKey, Length, ThisRec->MaxLength);
              break; }
            if (DestBuf)
              AddFormattedRecord(FALSE, DestBuf, "Rec %4d:\"%s\"", LineNo++, ThisRec->text ? ThisRec->text : "(null)");
            else
              Message(NULL, "Rec %4d:\"%s\"", LineNo++, ThisRec->text ? ThisRec->text : "(null)");
            ThisTag = ThisRec->TagChain;
            TagCount = 0;
            while (ThisTag) {
              if (ThisTag->StartPoint < 0 || Length < ThisTag->StartPoint) {
                ErrorsDetected++;
                if (DestBuf)
                  AddFormattedRecord(FALSE, DestBuf, "Ooops: Tag has an out-of-range character pointer");
                else
                  Message(NULL, "Ooops: Tag has an out-of-range character pointer"); }
              if (ThisTag->type == TagType_Target) {
                struct JumpHTabObj *ThisHTEntry = (struct JumpHTabObj *)ThisTag->Attr;
                if (DestBuf)
                  AddFormattedRecord(FALSE, DestBuf, "  JumpHTabObj: Buffer %c, Key %s", ThisHTEntry->HashBuf->BufferKey, ThisHTEntry->HashKey);
                else
                  Message(NULL, "  JumpHTabObj: Buffer %c, Key %s", ThisHTEntry->HashBuf->BufferKey, ThisHTEntry->HashKey); }
              if(10 < ++TagCount) {
                ErrorsDetected++;
                if (DestBuf)
                  AddFormattedRecord(FALSE, DestBuf, "Ooops: Tag chain of more than 10 entries");
                else
                  Message(NULL, "Ooops: Tag chain of more than 10 entries");
                break; }
              ThisTag = ThisTag->next; }
            } while ( (ThisRec = ThisRec->next) != SourceBuf->FirstRec->prev);
            
          if (DestBuf)
            AddFormattedRecord(FALSE, DestBuf, "Rec %4d:\"%s\"", LineNo++, ThisRec->text ? ThisRec->text : "(null)");
          else
            Message(NULL, "Rec %4d:\"%s\"", LineNo++, ThisRec->text ? ThisRec->text : "(null)");
            
          memset(ActiveBuffers, 0, sizeof(struct Buf *)*256);
          while (ThisBuf != NULL) {
            if ( ThisBuf->BufferKey != '~' && ActiveBuffers[(int)ThisBuf->BufferKey]) {
              ErrorsDetected++;
              if (DestBuf)
                AddFormattedRecord(FALSE, DestBuf, "  Duplicate buffer found key=\'%c\', current line (this one) \"%s\" (last one) \"%s\"", 
                  ThisBuf->BufferKey, ThisBuf->CurrentRec->text, ActiveBuffers[(int)ThisBuf->BufferKey]->CurrentRec->text);
              else
                Message(NULL, "  Duplicate buffer found key=\'%c\', current line (this one) \"%s\" (last one) \"%s\"", 
                  ThisBuf->BufferKey, ThisBuf->CurrentRec->text, ActiveBuffers[(int)ThisBuf->BufferKey]->CurrentRec->text); }
            ActiveBuffers[(int)ThisBuf->BufferKey] = ThisBuf;
            ThisBuf = ThisBuf->NextBuf; }
          
          LocalFail = ErrorsDetected;
          break; }

        default: //In theory, this just can't happen.
          LocalFail = RunError("Unrecognized %%Q qualifier");
          break; }
          
        //This is where we add the first line of the %q report.
        if (OrigCommand[0] == '\0') //The query handler has not copied the full command string to the destination buffer.
          FetchStringArgPercent(OrigCommand, NULL, &ThisArg);
        if (DestBuf && BufferKey != '\0') {
          if (FirstRec)
            DestBuf->CurrentRec = FirstRec;
          AddFormattedRecord(FALSE, DestBuf, ""); 
          if (s_Verbose & Verbose_ReportCounter && QueryType != Q_dir)
            AddFormattedRecord(FALSE, DestBuf, "%s (%d)", OrigCommand, s_CommandCounter);
          else
            AddFormattedRecord(FALSE, DestBuf, "%s", OrigCommand);
          if ( ! FirstRec)
            DestBuf->FirstRec = DestBuf->CurrentRec;
          DestBuf->LineNumber = 1;
          s_CurrentBuf = DestBuf; }
        break; }

      default: { // Should never get here - this error should be detected at compile time.
        LocalFail = RunError("Unknown %% command"); 
        break; } } } // Percent-commands block ends.
      break;
    
    default:  //Invalid command character.
      LocalFail = RunError("Unknown command"); 
      return 1;
     } // Main s_CommandChr case block ends - destination-point for breaks - from successful commands. 

    if (ThisArg != NULL) {
      LocalFail = Fail("Argument(s) remain");
      return 1; }
    
    //On completion of one command check for failure and handle it appropriately.
    if (s_BombOut) {
      return 1; }
    if (LocalFail && (s_TraceMode & Trace_Failures) )
      JotTrace("Command failure.");
    if (NextCommand == NULL) { //Last command in sequence.
      if ( ! LocalFail)
        break;
      else if (ElseCommands == NULL)
        return LocalFail; }
    else { //At least one more command to follow.
      s_CommandChr = NextCommand->CommandKey;
      ThisArg = NextCommand->ArgList;
      if (s_CommandChr == '?')       //? - Unset failure flag
        LocalFail = 0;
      else if (s_CommandChr == '\\') //\ - Reverse failure flag.
        LocalFail = ! LocalFail; }
    
    if (s_ExitBlocks)
      return LocalFail;
      
    if (LocalFail) {
      if (ElseCommands) {
        Fail("Command failed, taking else sequence");
        s_TraceLevel++;
        LocalFail = Run_Sequence(ElseCommands);
        s_TraceLevel--;
        return LocalFail; }
      else {
        if (s_TraceMode & Trace_FailNoElse)
          JotTrace("Command failure with no local handler.");
        return 1; } }
      
    s_CurrentCommand = NextCommand;
    } // Command-key loop ends.
  return LocalFail; }

//---------------------------------------------Run_Block
int Run_Block(struct Block *ThisBlock)
  { // Calls Run_Sequence to execute embedded sequence, normally returns 0 or nonzero if something has failed,
    //     checks for ')' and repeat-block count.
  struct Seq *Sequence;
  int LocalFail = 0, RepeatCount;
  
  if ( ! ThisBlock) 
    return 1;
     
  Sequence = ThisBlock->BlockSequence;
  RepeatCount = ThisBlock->Repeats;
  
  while (TRUE) {
    s_TraceLevel++;
    LocalFail = Run_Sequence(Sequence);
    s_TraceLevel--;
    if (s_ExitBlocks) {        //Early exit from current block?
      if (s_ExitBlocks < 0) {
        if (++s_ExitBlocks)    //Not yet unwound to repeating block - simply exit this block.
          return 0;
        else {                 //Repeat current block.
          Sequence = ThisBlock->BlockSequence;
          continue; } }
      else {
        --s_ExitBlocks;        //Exit blocks until we reach requested block.
        return LocalFail; } }
    if (LocalFail) //Block failed - but forgive that failure if it's an infinite loop.
      //No this looks wrong! - if there's an else clause to the infinately-repeating block we should execute that code before exiting the loop.
      return (ThisBlock->Repeats != INT_MAX);
    if (s_BombOut)
      return 1;
    if ( ! --RepeatCount)
      break; }

  return 0; }

//----------------------------------------------ExitNormally
void ExitNormally(char *ExitMessage)
  { // Exits after writing file. 
  FILE *File = NULL;
  char TempString[StringMaxChr];
  
  TempString[0] = '\0';
  CrossmatchPathnames(0, &File, "w", s_CurrentBuf->PathName, "./", NULL, TempString);
  if (File == NULL) { // Denied write access.
    RunError("Error opening file \"%s\" for writing.", TempString);
    return; }
  else { // File opend OK. 
    
    if (s_Verbose & Verbose_NonSilent)
      Message(NULL, "Writing to \"%s\"", TempString);
    JotWrite(File, FALSE, s_CurrentBuf);
    if (fclose(File) == 0) { // File wriitten OK.
      s_CurrentBuf->UnchangedStatus |= SameSinceIO;
      if (TidyUp(0, ExitMessage))
        return; }
    Fail("Error Closing your file"); }
  return;  }

//----------------------------------------------JotWrite
int JotWrite(FILE *File, int Binary, struct Buf *TextBuf)
  { //Writes specified buffer, returns FALSE on success.
    //Binary=16 will assume the file is in the form of a hex dump, the original binary form is reconstituted and written.
    //Similarly Binary=9 an octal dump and Binary=2 a binary dump.
  int LocalFail = FALSE, LineCount = 1;
  int EncodingSize = TextBuf->UCEncoding <= UCEncoding_UTF_8 ? 8 : 
                    (TextBuf->UCEncoding == UCEncoding_UTF_16LE || TextBuf->UCEncoding == UCEncoding_UTF_16BE ? 16 : 32);
  int EncodingLE = TextBuf->UCEncoding & UCEncoding_LE;
  struct Rec *Record = TextBuf->FirstRec, *LastRec = TextBuf->FirstRec->prev;
  
  if (EncodingSize == 16) {
      mbstate_t State;
      memset(&State, '\0', sizeof(mbstate_t));
      fputs(EncodingLE ? "\xff\xfe" : "\xfe\xff", File);
          
      do { // Write out records loop. 
        char16_t *RealUChr = (char16_t *)JotMalloc(sizeof(char16_t) * (Record->MaxLength+1)), *U16Chr = RealUChr;
        size_t Bytes;
        int InChr, OutByte;
        
        for (InChr = 0; ; InChr++) {
          while ((Bytes = mbrtoc16(U16Chr, Record->text + InChr, MB_CUR_MAX, &State))) {
            if (Bytes == (size_t)-1) {
              LocalFail = Fail("Invalid UTF-8 character detected at line %d, Chr %d", LineCount, InChr);
              break; }
            else if (Bytes == (size_t)-2) { // truncated input
               InChr += 1;
               continue; }
            else if (Bytes == (size_t)-3) // UTF-16 high surrogate
              U16Chr += 1;
            else {
              InChr += Bytes;
              U16Chr += 1; } }
        mbrtoc16(U16Chr, "\n", 1, &State);
        for (OutByte = 0; OutByte <= (U16Chr-RealUChr)*2; OutByte+=2) {
          if (EncodingLE) {
            fputc( ((char *)RealUChr)[OutByte], File);
            fputc( ((char *)RealUChr)[OutByte+1], File); }
          else {
            fputc( ((char *)RealUChr)[OutByte+1], File);
            fputc( ((char *)RealUChr)[OutByte], File); } }
        if ( ! Record->text[InChr])
          break; }
        Record = Record->next;
        LineCount += 1;
        free(RealUChr); }
      while (Record != LastRec); }
  else if (EncodingSize == 32) {
      mbstate_t State;
      int i;
      memset(&State, '\0', sizeof(mbstate_t));
      for (i=0; i<4; i++)
        fputc( (EncodingLE ? "\xff\xfe\x00\x00" : "\x00\x00\xfe\xff")[i], File);
          
      do { // Write out records loop. 
        char32_t *RealUChr = JotMalloc(sizeof(char32_t) * (Record->MaxLength+1)), *U32Chr = RealUChr;
        size_t Bytes;
        int InChr, OutByte;
        
        for (InChr = 0; ; InChr++) {
          while ((Bytes = mbrtoc32(U32Chr, Record->text + InChr, Record->MaxLength-InChr, &State))) {
            if(Bytes == (size_t)-1) {
              LocalFail = Fail("Invalid UTF-8 character detected");
              break; }
            if(Bytes == (size_t)-2) //Truncated input
              break; 
            InChr += Bytes;
            U32Chr += 1; }
        mbrtoc32(U32Chr, "\n", 1, &State);
        for (OutByte = 0; OutByte <= (U32Chr-RealUChr)*4; OutByte+=4) {
          if (EncodingLE) {
            fputc( ((char *)RealUChr)[OutByte], File);
            fputc( ((char *)RealUChr)[OutByte+1], File);
            fputc( ((char *)RealUChr)[OutByte+2], File);
            fputc( ((char *)RealUChr)[OutByte+3], File); }
          else {
            fputc( ((char *)RealUChr)[OutByte+3], File);
            fputc( ((char *)RealUChr)[OutByte+2], File);
            fputc( ((char *)RealUChr)[OutByte+1], File);
            fputc( ((char *)RealUChr)[OutByte], File); } }
        if ( ! Record->text[InChr])
          break; }
        Record = Record->next;
        free(RealUChr); }
      while (Record != LastRec); }
  else if (Binary == 16) { 
    if (Binary == 16) {
      while (TRUE) {
        int InChr = 0, Chrs, Byte, NxChr;
        while (TRUE) {
          while ( (NxChr = Record->text[InChr]) == ' ')
            InChr += 1;
          if (NxChr == '#')
            break;
          if ( ! strchr("0123456789abcdefABCDEF", NxChr)) {
            LocalFail = Fail("Invalid hex digit encountered in hex dump");
            break; }
          if (sscanf(Record->text+InChr, "%x %n", &Byte, &Chrs) < 0)
            break;
          putc(Byte, File);
          InChr += Chrs; }
      if (Record == LastRec)
        break;
      Record = Record->next; } } }
          
  else { //Plain vanilla dump of records to ASCII/UTF-8 text file.
    while ( TRUE ) { // Write out records loop. 
      if (Record == LastRec) {
        if (Record->text[0])
          fprintf(File, "%s", Record->text);
        break; }
      fprintf(File, "%s\n", Record->text);
      Record = Record->next; } }
      
  return LocalFail; }

//----------------------------------------------PushInt
int PushInt(long long Value)
  { //Checks for stack overflow and Pushes Value onto stack.
  if (s_StackSize <= s_StackPtr) {
    Fail("Stack overflow");
    return 1; }
  s_Stack[s_StackPtr].Value = Value; 
  s_Stack[s_StackPtr++].type = FrameType_Int;
  s_Stack[s_StackPtr].type = FrameType_Void;
  return 0; }

//----------------------------------------------PushFloat
int PushFloat(double Value)
  { //Checks for stack overflow and Pushes Value onto stack.
  if (s_StackSize <= s_StackPtr) {
    Fail("Stack overflow");
    return 1; }
  ((struct floatFrame *)(&s_Stack[s_StackPtr]))->fValue = Value; 
  s_Stack[s_StackPtr++].type = FrameType_Float;
  s_Stack[s_StackPtr].type = FrameType_Void;
  return 0; }

//----------------------------------------------PushBuf
int PushBuf(struct Buf *Buffer)
  { //Checks for stack overflow and Pushes Buffer onto stack.
  if (s_StackSize <= s_StackPtr) {
    Fail("Stack overflow");
    return 1; }
  ((struct bufFrame *)(&s_Stack[s_StackPtr]))->BufPtr = Buffer; 
  s_Stack[s_StackPtr++].type = FrameType_Buf;
  s_Stack[s_StackPtr].type = FrameType_Void;
  return 0; }

//----------------------------------------------PopInt
long long PopInt(int *FailFlag)
  { //Checks for stack underflow and and frame type, then pops  Value from stack.
  int ActualType;
   
  if (s_StackPtr < 1) {
    Fail("Stack underflow");
    *FailFlag |= 1;
    return 0; }
  ActualType = (&s_Stack[--s_StackPtr])->type;
  if (ActualType != FrameType_Int) {
    s_StackPtr++;
    RunError("Incorrect stack-frame type, expecting integer frame, found frame type %d", ActualType);
    *FailFlag |= 1;
    return 0; }
     
  return s_Stack[s_StackPtr].Value; }

//----------------------------------------------PopFloat
double PopFloat(int *FailFlag)
  { //Checks for stack underflow and and frame tye, then pops  Value from stack.
  int ActualType;
   
  if (s_StackPtr < 1) {
    Fail("Stack underflow");
    *FailFlag |= 1;
    return 0; }
  ActualType = (&s_Stack[--s_StackPtr])->type;
  if (ActualType != FrameType_Float) {
    s_StackPtr++;
    RunError("Incorrect stack-frame type, expecting floating-point frame, found frame type %d", ActualType);
    *FailFlag |= 1;
    return 0; }
     
  return ((struct floatFrame *)(&s_Stack[s_StackPtr]))->fValue; }

//---------------------------------------------KillTop
int KillTop()
  { //Moves the stack pointer down one frame, returns 0 if there are no errors.
  if (s_StackPtr < 1) {
    Fail("Stack underflow");
    return 1; }
    
  s_StackPtr--;
  if (s_Stack[s_StackPtr].type == FrameType_Buf) { //It's a buffer, free the buffer - FreeBuffer() checks for clones and delete locks.
    struct Buf *StackTopBuf = ((struct bufFrame *)(&s_Stack[s_StackPtr]))->BufPtr;
    if (StackTopBuf == s_CurrentBuf) {
      s_StackPtr++;
      return Fail("Can't remove current buffer from stack."); }
    else {
      FreeBuffer(StackTopBuf); } }
      
  return 0; }

//---------------------------------------------SearchStackForBuf
int SearchStackForBuf(struct Buf *TargetBuf) 
  { //Searches stack for clones of TargetBuf, starting at current top item, returns number of hits.
  int Count = 0, SP;
  for (SP = s_StackPtr-1; 0 <= SP; SP--) { // Stack-dump loop.
    if (s_Stack[SP].type == FrameType_Buf) {
      if (((struct bufFrame *)(&s_Stack[SP]))->BufPtr == TargetBuf)
        Count++; } }
  return Count; }

//---------------------------------------------DumpStack
void DumpStack(struct Buf * DestBuf)
  { //Like it says on the tin - it dumps the stack to a nominated buffer or the editor console.
  int SP;
  char Line[StringMaxChr];
  
  if (DestBuf) {
    AddFormattedRecord(FALSE, DestBuf, "Stack currently holds %d item%s:", s_StackPtr, s_StackPtr == 1 ? "" : "s");
    if (s_StackPtr)
      AddFormattedRecord(FALSE, DestBuf, "%4s %8s %8s %8s", "Item", "Dec", "Hex", "Chr"); }
  else {
    Message(NULL, "Stack currently holds %d item%s:", s_StackPtr, s_StackPtr == 1 ? "" : "s");
    if (s_StackPtr)
      Message(NULL, "%4s %8s %8s %8s", "Item", "Dec", "Hex", "Chr"); }
  for (SP = s_StackPtr-1; 0 <= SP; SP--) { // Stack-dump loop.
    if (s_Stack[SP].type == FrameType_Int) {
      long long value = s_Stack[SP].Value;
      unsigned char Chr = value&0xFF;
      sprintf(Line, "%4d %8lld %8llX %8c", SP, value, value, (Chr < 32 || 127 < Chr) ? '~' : Chr); }
    else if ((&s_Stack[SP])->type == FrameType_Float) {
      double value = ((struct floatFrame *)(&s_Stack[SP]))->fValue;
      sprintf(Line, "%4d %f", SP, value); }
    else if ( (((struct bufFrame *)(&s_Stack[SP]))->BufPtr)->CurrentRec ) {
      struct Buf *Buf = ((struct bufFrame *)(&s_Stack[SP]))->BufPtr;
      char TruncatedText[101];
      strncpy(TruncatedText, Buf->CurrentRec->text, 100);
      TruncatedText[100] = '\0';
      sprintf(Line, "%4d      Buffer -> \"%s\"", SP, TruncatedText); }
    else
      sprintf(Line, "Invalid buffer record, buffer %c, line %d", 
        ((struct bufFrame *)(&s_Stack[SP]))->BufPtr->BufferKey, ((struct bufFrame *)(&s_Stack[SP]))->BufPtr->LineNumber);
    if (DestBuf)
      GetRecord(DestBuf, strlen(Line)+1, Line);
    else
      Message(NULL, "%s", Line); } }

//--------------------------------------------------FreeRecord
int FreeRecord(struct Buf *TextBuf, int AdjustMode)
  {
  //
  //Releases current record of Buffer.
  //If asked to delete first record, sets first record descriptor to next rec.
  //AdjustMode controls shifing of htab entries.
  //
  struct Rec *Record = TextBuf->CurrentRec;
  if (Record) {
    struct Rec *NextRecord = Record->next;
    int FirstRecord = (TextBuf->FirstRec) == Record;
    struct AnyTag *ThisTag = Record->TagChain;
    
    if (TextBuf->EditLock & ReadOnly) {
      Fail("Can't delete records from read-only buffer %c", TextBuf->BufferKey);
      return 1; }
      
    while (ThisTag) {
      struct AnyTag *NextTag = ThisTag->next;
      
      if (ThisTag->type == TagType_Target) {
        struct JumpHTabObj *ThisJumpHTabObj = (struct JumpHTabObj *)(ThisTag->Attr);
        struct Buf *RemoteBuf = ThisJumpHTabObj->HashBuf;
        
        if (ThisJumpHTabObj->TargetRec) {
          if (RemoteBuf) {
            if (RemoteBuf->HashtableMode == DestroyHashtables) {
              DestroyHtab(RemoteBuf);
              //N.B. DestroyHtab() also takes out tags pointing to the hashtable - including the one we're currently holding.
              NextTag = Record->TagChain; }
            else if (RemoteBuf->HashtableMode == DeleteEntries) {
              FreeTag(Record, ((struct AnyTag *)ThisJumpHTabObj->Target));
              ThisJumpHTabObj->Target = NULL;
              ThisJumpHTabObj->TargetRec = NULL; }
            else if (RemoteBuf->HashtableMode == AdjustEntries) {
              if (AdjustMode == DeleteNotAdjust)
                ThisJumpHTabObj->TargetRec = NULL;
              else { //Adjust tag by making a copy in the adjacent record.
                struct AnyTag *NewTag = (struct AnyTag *)JotMalloc(sizeof(struct AnyTag));
                ThisJumpHTabObj->TargetRec = (AdjustMode == AdjustForwards) ?
                  ((ThisJumpHTabObj->TargetRec->next == TextBuf->FirstRec) ? ThisJumpHTabObj->TargetRec->prev : ThisJumpHTabObj->TargetRec->next) :
                  ((ThisJumpHTabObj->TargetRec == TextBuf->FirstRec) ? ThisJumpHTabObj->TargetRec->next : ThisJumpHTabObj->TargetRec->prev);
                NewTag->next = NULL;
                NewTag->StartPoint = 0;
                ThisJumpHTabObj->Target = (struct TargetTag *)NewTag;
                NewTag->type = TagType_Target;
                NewTag->Attr = (void *)ThisJumpHTabObj;
                AddTag(ThisJumpHTabObj->TargetRec, NewTag); } }
            else if (RemoteBuf->HashtableMode == ProtectEntries) {
              Fail("Can't delete a protected record from buffer %c", RemoteBuf->BufferKey);
              return 1; } } } }
      else
        FreeTag(Record, ThisTag);
      ThisTag = NextTag; }
       
    ThisTag = Record->TagChain;
    while (ThisTag) {
      struct AnyTag *NextTag = ThisTag->next;
      free(ThisTag);
      ThisTag = NextTag; }
    Record->TagChain = NULL;
      
    if (NextRecord == Record) { //This is the one-and-only record in the buffer.
      NextRecord = TextBuf->FirstRec = NULL;
      TextBuf->SubstringLength = 0; }
    else { // Haven't deleted one and only record so relink other records.
      struct Rec *PrevRecord = Record->prev;
      PrevRecord->next = NextRecord;
      NextRecord->prev = PrevRecord; }
    free(Record->text);
    free(Record);
    if (FirstRecord)
      TextBuf->FirstRec = NextRecord;
    TextBuf->CurrentRec = NextRecord;
    TextBuf->CurrentByte = 0;
    TextBuf->SubstringLength = 0;
    
    return 0; }
  else
    return 1; }

//----------------------------------------------FreeBuffer
int FreeBuffer(struct Buf *TextBuf)
  { //Releases Buffer records returns 1 if error, otherwise 0.
  struct Buf **ThisBufPtr = &s_BufferChain;
  int LockedBuffers = CheckChildBuffers(TextBuf);
  
  if (TextBuf == s_NoteBuffer)
    s_NoteBuffer = NULL;
  if (LockedBuffers) {
    Fail("Attempt to delete a locked buffer or the parent of a locked buffer.");
    return 1; }
  if ( ! TextBuf)
    return 0;
  if (TextBuf->EditLock & ReadOnly) {
    Fail("Attempt to delete readonly buffer %c", TextBuf->BufferKey);
    return 1; }
  if (TextBuf->EditLock & WriteIfChanged && (TextBuf->UnchangedStatus & SameSinceIO) == 0) {
    Fail("Attempt to delete a changed WriteIfChanged buffer %c", TextBuf->BufferKey);
    return 1; }
  if (TextBuf->ExistsInData || SearchStackForBuf(TextBuf))
    //The buffer's been cloned to a DataHTabObject - the FreeBuffer operation is unnecessary and silently abandonded.
    return 0;
  FreeBufferThings(TextBuf);
  
  //Search for TextBuf and remove it from chain.
  while ( *ThisBufPtr != NULL) {
    struct Buf **NextBufPtr = &(( *ThisBufPtr)->NextBuf);
    if ( *ThisBufPtr == TextBuf) {
      *ThisBufPtr = *NextBufPtr;
      break; }
    else
      ThisBufPtr = NextBufPtr; }
     
  free(TextBuf);
  return 0; }

//----------------------------------------------FreeBufferThings
int FreeBufferThings(struct Buf *TextBuf)
  { //Frees Buffer records, hashtable and tags etc.
  struct ColourObj *ThisColourObj = TextBuf->FirstColObj;
  struct CodeHeader *CodeChain;
  
  if (TextBuf == s_NoteBuffer)
    s_NoteBuffer = NULL;
  TextBuf->UnchangedStatus = 0;
  
  while (ThisColourObj) {
    struct ColourObj *NextColourObj = ThisColourObj->next;
    free(ThisColourObj);
    ThisColourObj = NextColourObj; }
        
  if (TextBuf->PathName)
    free(TextBuf->PathName);
  FreeIOBlock(TextBuf);
  CodeChain = TextBuf->CodeChain;
  while (CodeChain) {
    struct CodeHeader *next = CodeChain->next;
    FreeSequence(&CodeChain->CompiledCode);
    free(CodeChain);
    CodeChain = next; }
  TextBuf->CodeChain = NULL;
  if (TextBuf->Header)
    free(TextBuf->Header);
  if (TextBuf->TabStops)
    free(TextBuf->TabStops);
  TextBuf->TabStops = NULL;
  
  DestroyHtab(TextBuf);
  //Free the records.
  if (TextBuf->CurrentRec) {
    TextBuf->CurrentRec = TextBuf->FirstRec;
    while (TextBuf->FirstRec)
      FreeRecord(TextBuf, DeleteNotAdjust); }
      
  return 0; }

//--------------------------------------------------FreeIOBlock
void FreeIOBlock(struct Buf *TextBuf) {
  //Frees the IOBlock associated with the nominated text buffer.
#if defined(VC)
  if (TextBuf->IOBlock) {
    int Bytes;
    if (TextBuf->IOBlock->HoldDesc)
      close(TextBuf->IOBlock->HoldDesc);
    if (TextBuf->IOBlock->BucketBlock)
      free(TextBuf->IOBlock->BucketBlock);
    if (TextBuf->IOBlock->IoFd) { //The buffer has interactive I/O set up.
      struct Buf **ThisInteractiveBufPtr = &s_FirstInteractiveBuf;
      while (*ThisInteractiveBufPtr) {
        if (*ThisInteractiveBufPtr == TextBuf) {
          *ThisInteractiveBufPtr = TextBuf->IOBlock->NxInteractiveBuf;
          break; }
        ThisInteractiveBufPtr = &(*ThisInteractiveBufPtr)->IOBlock->NxInteractiveBuf; }
        if (TextBuf->IOBlock->ExitCommand) { //An exit command has been defined.
          WriteFile(TextBuf->IOBlock->IoFd, TextBuf->IOBlock->ExitCommand, strlen(TextBuf->IOBlock->ExitCommand), &Bytes, NULL);
          WriteFile(TextBuf->IOBlock->IoFd, "\n", 1, &Bytes, NULL); } }
      else {
        if (TerminateProcess(TextBuf->IOBlock->ProcessId, 0))
          Fail("Failed to terminate the child process"); }
      CloseHandle(TextBuf->IOBlock->OutPipe);
      CloseHandle(TextBuf->IOBlock->IoFd);
      if (TextBuf->IOBlock->ExitCommand)
        free(TextBuf->IOBlock->ExitCommand);
      if (TextBuf->IOBlock->WaitforSequence)
        FreeBuffer(TextBuf->IOBlock->WaitforSequence);
      FreeSequence(&TextBuf->IOBlock->Code);
      free(TextBuf->IOBlock); }
#else
  if (TextBuf->IOBlock) {
    if (TextBuf->IOBlock->IoFd) { //The buffer has interactive I/O set up.
      struct Buf **ThisInteractiveBufPtr = &s_FirstInteractiveBuf;
      if (TextBuf->IOBlock->ExitCommand) { //An exit command has been defined.
        write(TextBuf->IOBlock->IoFd, TextBuf->IOBlock->ExitCommand, strlen(TextBuf->IOBlock->ExitCommand));
        write(TextBuf->IOBlock->IoFd, "\n", 1);
        free(TextBuf->IOBlock->ExitCommand); }
      else
        kill(TextBuf->IOBlock->ChildPid, SIGTERM);
      waitpid(TextBuf->IOBlock->ChildPid, NULL, 0);
      close(TextBuf->IOBlock->IoFd);
      while (*ThisInteractiveBufPtr) {
        if (*ThisInteractiveBufPtr == TextBuf) {
          *ThisInteractiveBufPtr = TextBuf->IOBlock->NxInteractiveBuf;
          break; }
        ThisInteractiveBufPtr = &(*ThisInteractiveBufPtr)->IOBlock->NxInteractiveBuf; } }
    if (TextBuf->IOBlock->HoldDesc)
      close(TextBuf->IOBlock->HoldDesc);
    if (TextBuf->IOBlock->BucketBlock)
      free(TextBuf->IOBlock->BucketBlock);
    if (TextBuf->IOBlock->WaitforSequence)
      FreeBuffer(TextBuf->IOBlock->WaitforSequence);
    FreeSequence(&TextBuf->IOBlock->Code);
    free(TextBuf->IOBlock); }
#endif
  TextBuf->IOBlock = NULL;
  }

//--------------------------------------------------CheckChildBuffers
int CheckChildBuffers(struct Buf *ThisBuf)
  { //Sniffs out all the sub buffers dangling off the current buffers hashtable, returns number of changed writeifchanged buffers.
  struct AnyHTabObj *ThisObj = (struct AnyHTabObj *)ThisBuf->FirstObj;
  int Count = 0;
  
  if (ThisBuf == s_CurrentBuf) {
    char ParentPath[StringMaxChr];
    GetBufferPath(ThisBuf, ParentPath, StringMaxChr);
    Fail("The current buffer belongs to the tree offered for removal \"%s\"", ParentPath);
    Count++; }
  
  if ( (ThisBuf->EditLock & WriteIfChanged) && ! (ThisBuf->UnchangedStatus & SameSinceIO) ) {
    char ParentPath[StringMaxChr];
    GetBufferPath(ThisBuf, ParentPath, StringMaxChr);
    Fail("Found a changed write-if-changed sub-buffer path \"%s\"", ParentPath);
    Count++; }
          
  while (ThisObj) {
    if (ThisObj->type == HTabObj_Data) { //A DataHTabObj
      struct DataHTabObj *ThisDataHTabObj = (struct DataHTabObj *)ThisObj;
      if (ThisDataHTabObj->Frame && ((struct DataHTabObj *)ThisObj)->Frame->type == FrameType_Buf) //It's a buffer - check it.
        Count += CheckChildBuffers(((struct bufFrame *)ThisDataHTabObj->Frame)->BufPtr); }
    ThisObj = ((struct JumpHTabObj *)ThisObj)->next; }
        
  return Count; }

//--------------------------------------------------CheckCircularHier
int CheckCircularHier(struct Buf *ThisBuf)
  { //Sniffs out all the sub buffers dangling off the specified buffer's hashtable, returns 1 if the buffer exists in the subtree, otherwise returns 0.
  struct AnyHTabObj *ThisObj = (struct AnyHTabObj *)ThisBuf->FirstObj;
  int Count = 0;
  
  if (ThisBuf == s_CurrentBuf) {
    char ParentPath[StringMaxChr];
    GetBufferPath(ThisBuf, ParentPath, StringMaxChr);
    Fail("The current buffer belongs to the tree offered for removal \"%s\"", ParentPath);
    Count++; }
  
  while (ThisObj) {
    if (ThisObj->type == HTabObj_Data) { //A DataHTabObj
      struct DataHTabObj *ThisDataHTabObj = (struct DataHTabObj *)ThisObj;
      if (ThisDataHTabObj->Frame && ((struct DataHTabObj *)ThisObj)->Frame->type == FrameType_Buf) //It's a buffer - check it.
        Count += CheckChildBuffers(((struct bufFrame *)ThisDataHTabObj->Frame)->BufPtr); }
    ThisObj = ((struct JumpHTabObj *)ThisObj)->next; }
        
  return Count; }

//--------------------------------------------------DuplicateRecord
int DuplicateRecord(struct Buf *OutBuf, struct Buf *InBuf)
  { //Gets a record of the right size then copies in text.
  char *InRecord = InBuf->CurrentRec->text;
  
  if (GetRecord(OutBuf, strlen(InRecord)+1, InRecord))
    return 1;
  return 0; }

//--------------------------------------------------CheckBufferKey
int CheckBufferKey(char Key)
  { //Returns 1 if the given key is in the valid range.
  return ( ( (' ' < Key) && (Key < '{') ) || (Key == '~') ); }

//--------------------------------------------------JotMalloc
void *JotMalloc(int Bytes)
  { //malloc with jot-style error messages.
  void *Result;
  if ( ! (Result = malloc(Bytes)) )
    Disaster("Memory allocation failed (%d bytes).", Bytes);
  return Result; }

//--------------------------------------------------GetBuffer
struct Buf *GetBuffer(int Key, int CreateOrDelete)
  { //Creates new buffer, if the buffer already exists then the existing buffer is returned unchanged.
    //If buffer is ~ creates a new buffer on stack if CreateInStack is true - even if top frame is a buffer.
    //CreateOrDelete controls creation of new buffers, three valid values are recognized:
    //  0 - AlwaysNew      - Deletes any preexisting buffer, returned buffer is always empty.
    //  1 - OptionallyNew  - Returns preexisting buffer or creates a new buffer not a stack buffer.
    //  2 - NeverNew       - Returns preexisting buffer or NULL.
    //
  struct Buf *NewBuf = NULL;
  struct Buf *ThisBuf = s_BufferChain;
  
  if (Key == '~') {
    if (CreateOrDelete == AlwaysNew) { //Creation of new buffers on the stack only allowed when tagged AlwaysNew.
      if (s_StackSize <= s_StackPtr) {
        Fail("Stack overflow");
        return NULL; }
      NewBuf = (struct Buf *)JotMalloc(sizeof(struct Buf));
      NewBuf->BufferKey = Key;
      ClearBuffer(NewBuf);
      PushBuf(NewBuf);
      return NewBuf; }
    else {
      if (s_StackPtr < 1) {
        Fail("Stack underflow");
        return NULL; }
      return (s_Stack[s_StackPtr-1].type == FrameType_Buf) ? ((struct bufFrame *)(&s_Stack[s_StackPtr-1]))->BufPtr : NULL; } }
  else {
    while (ThisBuf != NULL) {
      if ((ThisBuf->BufferKey) == Key) { //Found the buffer.
        if (CreateOrDelete == AlwaysNew) { //Check locks on preexisting buffer before clearing the existing version of the buffer.
          if (ThisBuf->EditLock & ReadOnly) {
            Fail("Attempt to redefine readonly buffer %c", ThisBuf->BufferKey);
            return NULL; }
          if (ThisBuf->EditLock & WriteIfChanged && (ThisBuf->UnchangedStatus & SameSinceIO) == 0) {
            Fail("Attempt to destroy a changed WriteIfChanged buffer %c", ThisBuf->BufferKey);
            return NULL; }
          FreeBufferThings(ThisBuf);
          ClearBuffer(ThisBuf);
          return ThisBuf; }
        else //Use the pre-existing buffer as-is.
          return ThisBuf; }
      else
        ThisBuf = ThisBuf->NextBuf; } }
        
  if ( ! NewBuf) {
    if (CreateOrDelete == NeverNew)
      return NULL;
    else {
      NewBuf = (struct Buf *)JotMalloc(sizeof(struct Buf));
      NewBuf->BufferKey = Key;
      ClearBuffer(NewBuf);
      NewBuf->NextBuf = s_BufferChain;
      s_BufferChain = NewBuf; } }
  return NewBuf;  }

//--------------------------------------------------ClearBuffer
void ClearBuffer(struct Buf *ThisBuf)
  { //(Re)sets all buffer pointers and data.
  ThisBuf->CurrentRec = NULL;
  ThisBuf->FirstRec = NULL;
  ThisBuf->SubstringLength = 0;
  ThisBuf->AbstractWholeRecordFlag = FALSE;
  ThisBuf->CurrentByte = 0;
  ThisBuf->TabStops = NULL;
  ThisBuf->LeftOffset = 0;
  ThisBuf->LineNumber = 0;
  ThisBuf->OldFirstLineNo = 1; 
  ThisBuf->ParentObj = NULL;
  ThisBuf->Predecessor = NULL;
  ThisBuf->Header = NULL;
  ThisBuf->Footer = NULL;
  ThisBuf->PathName = NULL;
  ThisBuf->IOBlock = NULL;
  ThisBuf->FrameCount = 0;
  ThisBuf->NewPathName = TRUE;
  ThisBuf->NoUnicode = FALSE;
  ThisBuf->UnchangedStatus = 0;
  ThisBuf->EditLock = 0;
  ThisBuf->AutoTabStops = FALSE; 
  ThisBuf->FirstColObj = NULL;
  ThisBuf->htab = NULL;
  ThisBuf->FirstObj = NULL;
  ThisBuf->FileType = 0;
  ThisBuf->UCEncoding = 0;
  ThisBuf->HashtableMode = NoHashtable;
  if (ThisBuf->IOBlock) {
    struct BucketBlock *ThisBucketBlock = ThisBuf->IOBlock->BucketBlock;
    while (ThisBucketBlock) {
      struct BucketBlock *NextBucketBlock = ThisBucketBlock->Next;
      free(ThisBucketBlock);
      ThisBucketBlock = NextBucketBlock; }
#if defined(VC)
    if (ThisBuf->IOBlock->OutPipe)
      CloseHandle(ThisBuf->IOBlock->OutPipe);
    ThisBuf->IOBlock->OutPipe = 0;
#else
    if (ThisBuf->IOBlock->IoFd)
      close(ThisBuf->IOBlock->IoFd);
    ThisBuf->IOBlock->IoFd = 0;
#endif
  }
  ThisBuf->CodeChain = NULL;
  ThisBuf->ExistsInData = FALSE;
  ThisBuf->TabCells = FALSE;
#if defined(VC)
  ThisBuf->CodePage = s_CodePage;
#endif
  }

//--------------------------------------------------CopyBuffer
void CopyBuffer(struct Buf *ToBuf, struct Buf *FromBuf)
  { //Makes an exact copy of FromBuf in ToBuf.
  struct Rec *FromRec = FromBuf->FirstRec, *PrevRec = NULL, *ToRec;
  
  do {
    ToRec = (struct Rec *)JotMalloc(sizeof(struct Rec));
    ToRec->MaxLength = strlen(FromRec->text)+1;
    ToRec->text = (char *)JotMalloc(ToRec->MaxLength);
    ToRec->TagChain = NULL;
    ToRec->DisplayFlag = Redraw_Line;
    strcpy(ToRec->text, FromRec->text);
    if ( ! PrevRec) {
      ToBuf->FirstRec = ToRec;
      PrevRec = ToRec; }
    else {
      PrevRec->next = ToRec;
      ToRec->prev = PrevRec;
      PrevRec = ToRec; }
    if (FromRec == FromBuf->CurrentRec)
      ToBuf->CurrentRec = ToRec;
    FromRec = FromRec->next; }
    while (FromRec != FromBuf->FirstRec);
    
  ToRec->next = ToBuf->FirstRec;
  ToBuf->FirstRec->prev = ToRec;
  ToBuf->CurrentByte = FromBuf->CurrentByte;
  ToBuf->SubstringLength = FromBuf->SubstringLength;
  ToBuf->LineNumber = FromBuf->LineNumber;
  ToBuf->OldFirstLineNo = 0;
  ToBuf->NextBuf = NULL;
  ToBuf->TabStops = NULL;
  ToBuf->LeftOffset = FromBuf->LeftOffset;
  ToBuf->Predecessor = NULL;
  ToBuf->PathName = NULL;
  ToBuf->Header = NULL;
  ToBuf->Footer = NULL;
  ToBuf->FirstColObj = NULL;
  ToBuf->FirstObj = NULL;
  ToBuf->htab = NULL;
  ToBuf->IOBlock = NULL;
  ToBuf->AbstractWholeRecordFlag = FALSE;
  ToBuf->NewPathName = 0;
  ToBuf->UnchangedStatus = FromBuf->UnchangedStatus;
  ToBuf->EditLock = 0;
  ToBuf->HashtableMode = 0;
  ToBuf->NewPathName = FALSE;
  ToBuf->NoUnicode = 0;
  ToBuf->FileType = FromBuf->FileType;
  ToBuf->AutoTabStops = FromBuf->AutoTabStops;
  ToBuf->ExistsInData = FALSE;
  ToBuf->BufferKey = '~'; }

//--------------------------------------------------GetRecord
int GetRecord(struct Buf *TextBuf, int Size, char * InitString)
  { //Extracts new record from heap to follow original current record,
    //  then sets it to current and initializes it.
    //If an empty buffer then create first record.
  struct Rec *Record = TextBuf->CurrentRec;
  struct Rec *NewRecord;
  char *NewText;
  
  if (Size < 0) {
    RunError("Negative record size in GetRecord()");
    return 1; }
  NewRecord = (struct Rec *)JotMalloc(sizeof(struct Rec));
  NewText = (char *)JotMalloc(Size);
  if (Record == NULL) { //Empty buffer - set up Buffer block descriptor.
    TextBuf->FirstRec = NewRecord;
    NewRecord->prev = NewRecord;
    NewRecord->next = NewRecord; }
  else { // Non-null Buffer, just link in new record.
    struct Rec *NextRecord = Record->next;
  
    NewRecord->prev = Record;
    NewRecord->next = NextRecord;
    NextRecord->prev = NewRecord;
    Record->next = NewRecord; }
  TextBuf->CurrentRec = NewRecord;
  TextBuf->CurrentByte = 0;
  TextBuf->SubstringLength = 0;
  NewRecord->text = NewText;
  if (InitString)
    strcpy(NewRecord->text, InitString);
  else
    NewRecord->text[0] = '\0';
  NewRecord->DisplayFlag = Redraw_Line;
  NewRecord->MaxLength = Size;
  NewRecord->TagChain = NULL;
  TextBuf->LineNumber += 1;
  return 0; }

//-------------------------------------------------AddTag
void AddTag(struct Rec *Record, struct AnyTag *NewTag)
  { //Adds the tag to the correct place in the record's tag chain.
  struct AnyTag **PrevTagPtr = &(Record->TagChain);
  
  while ( *PrevTagPtr && ( (*PrevTagPtr)->StartPoint <= NewTag->StartPoint ) )
    PrevTagPtr = &((*PrevTagPtr)->next);
    
  NewTag->next = *PrevTagPtr;
  *PrevTagPtr = NewTag; }

//-------------------------------------------------Abstract
int Abstract(struct Buf *DestBuf, struct Buf *SourceBuf, char FillQualifier)
  { //Moves records from one buf to another (i.e. abstraction.), copying starts at the current character of the source buffer and progresses to ChrNo of LineNo.
  int StartLine = s_NoteLine, StartByte = s_NoteByte;
  int EndLine = SourceBuf->LineNumber;
  int EndByte = SourceBuf->CurrentByte;
  int SourceBytes;
  int AbstractFailed = FALSE, Line, CopyLenBytes;
  int NoUnicode = SourceBuf->NoUnicode;
  
  SourceBuf->SubstringLength = 0;
  if ( (StartLine < EndLine) || ((StartLine == EndLine) && (StartByte < EndByte)) ) { //Abstracting forwards - swap start and end points.
    EndLine = StartLine;
    EndByte = StartByte; 
    StartLine = SourceBuf->LineNumber;
    StartByte = SourceBuf->CurrentByte;
    AbstractFailed |= AdvanceRecord(SourceBuf, EndLine-StartLine); }
    
  SourceBytes = strlen(SourceBuf->CurrentRec->text);
  if ( (EndLine == StartLine) && (SourceBytes < StartByte) )
    AbstractFailed = Fail("Invalid note point.");
  
  if (EndLine == StartLine) {
    CopyLenBytes = StartByte-EndByte;
    if ( ! CopyLenBytes)
      return 0; }
  else
    CopyLenBytes = SourceBytes-EndByte;
  if (AbstractFailed)
    return 1;
  if ( (StartLine == EndLine) && (SourceBytes < StartByte) ) {
    Fail("Invalid note point.");
    return 1; }
  
  if (SourceBytes < EndByte || SourceBytes < CopyLenBytes) {
    RunError("Garbled note point");
    return 1; }
  //Copy the first (sub)record.
  DestBuf->SubstringLength = 0;
  SubstituteString(DestBuf, SourceBuf->CurrentRec->text+EndByte, CopyLenBytes);
  if (FillQualifier != '&') { //Destroy.
    SourceBuf->CurrentByte = EndByte;
    if (SourceBytes < SourceBuf->CurrentByte+CopyLenBytes)
      CopyLenBytes = SourceBytes-SourceBuf->CurrentByte;
    SourceBuf->SubstringLength = CopyLenBytes;
    SubstituteString(SourceBuf, "", -1); }
  else {
    SourceBuf->CurrentByte = StartByte; }
  if (EndLine != StartLine) {
    AbstractFailed |= AdvanceRecord(SourceBuf, 1);
    DestBuf->CurrentByte += CopyLenBytes;
    BreakRecord(DestBuf);
    AbstractFailed |= AdvanceRecord(DestBuf, -1);
    if (AbstractFailed)
      return 1; }
    
  //Move any complete records between first and last.
  for (Line = 1; Line < StartLine-EndLine; Line++) { //Move records loop.
    SourceBytes = strlen(SourceBuf->CurrentRec->text);
    if (FillQualifier == '&') { //Preserve original.
      GetRecord(DestBuf, SourceBytes+1, SourceBuf->CurrentRec->text);
      AbstractFailed |= AdvanceRecord(SourceBuf, 1); }
    else //Destroy.
      MoveRecord(DestBuf, SourceBuf);
    if (AbstractFailed)
      return 1; }
    
  if (EndLine != StartLine) { //Copy the final (sub)record.
    int Chrs = NoUnicode ? StartByte : JotStrlenBytes(SourceBuf->CurrentRec->text, StartByte);
    if (Chrs < 0)
      return 1;
    DestBuf->LineNumber -= 1;
    AbstractFailed |= AdvanceRecord(DestBuf, 1);
    DestBuf->CurrentByte = 0;
    DestBuf->SubstringLength = 0;
    SourceBytes = strlen(SourceBuf->CurrentRec->text);
    if (SourceBytes < StartByte) {
      RunError("Garbled note point");
      return 1; }
    SubstituteString(DestBuf, SourceBuf->CurrentRec->text, StartByte);
    if (FillQualifier != '&') { //Remove the source substring.
      SourceBuf->CurrentByte = 0;
      SourceBuf->SubstringLength = StartByte;
      SubstituteString(SourceBuf, "", -1);
      AbstractFailed |= AdvanceRecord(SourceBuf, -1);
      JoinRecords(SourceBuf); }
    else
      SourceBuf->CurrentByte = Chrs; }
      
  return AbstractFailed; }

//-------------------------------------------------MoveRecord
void MoveRecord(struct Buf *ToBuf, struct Buf *FromBuf)
  {
  //Extracts record from FromBuf and places it to follow original current record in ToBuf it then becomes the current record.
  struct Rec *ToRecord = ToBuf->CurrentRec;
  struct Rec *FromRecord = FromBuf->CurrentRec;
  struct AnyTag *ThisTag = FromRecord->TagChain;
  
  while (ThisTag) {
    struct AnyTag *NextTag = ThisTag->next;
    
    if (ThisTag->type == TagType_Target) {
      struct JumpHTabObj *ThisJumpHTabObj = (struct JumpHTabObj *)(ThisTag->Attr);
      struct Buf *RemoteBuf = ThisJumpHTabObj->HashBuf;
      
      if (RemoteBuf->HashtableMode == DestroyHashtables) {
        DestroyHtab(RemoteBuf);
        //N.B. DestroyHtab() also takes out tags pointing to the hashtable - including the one we're currently holding.
        NextTag = FromRecord->TagChain; }
      else if (RemoteBuf->HashtableMode == DeleteEntries)
        ThisJumpHTabObj->TargetRec = NULL;
      else if (RemoteBuf->HashtableMode == AdjustEntries) { //Normally push the target to the next record
        FreeTarget(FromBuf, ThisJumpHTabObj);
        if (ThisJumpHTabObj->TargetRec) {
          struct AnyTag *NewTag = (struct AnyTag *)JotMalloc(sizeof(struct AnyTag));
          struct Rec *NewTargetRec = (ThisJumpHTabObj->TargetRec->next == FromBuf->FirstRec) ? ThisJumpHTabObj->TargetRec->prev : ThisJumpHTabObj->TargetRec->next;
          ThisJumpHTabObj->type = HTabObj_Jump;
          NewTag->next = NULL;
          NewTag->StartPoint = 0;
          NewTag->type = TagType_Target;
          NewTag->Attr = (void *)ThisJumpHTabObj;
          ThisJumpHTabObj->TargetRec = NewTargetRec;
          AddTag(NewTargetRec, NewTag); } }
      else if (RemoteBuf->HashtableMode == ProtectEntries) {
        Fail("Can't abstract a protected record from buffer %c", RemoteBuf->BufferKey);
        return; } }
    else {
      FreeTag(FromRecord, ThisTag); }
      
    ThisTag = NextTag; }
      
  if (FromRecord == FromBuf->FirstRec->prev) //No records in FromBuf.
    return;
  if (FromRecord == (FromRecord->next))
    FromBuf->CurrentRec = NULL;
  else { // Have not deleted one-and-only record so relink other records.
    FromRecord->prev->next = FromRecord->next;
    FromRecord->next->prev = FromRecord->prev; }
  //If moving first record, then put in a new first record descriptor.
  if ((FromBuf->FirstRec) == FromRecord)
     FromBuf->FirstRec = FromRecord->next;
  //Update Current record pointer.
  FromBuf->CurrentRec = FromRecord->next;
  FromBuf->CurrentByte = 0;
  if (ToRecord == ToBuf->FirstRec->prev) { //Null Buffer, move in first record.
    ToRecord = FromRecord;
    ToBuf->CurrentRec = ToRecord;
    ToBuf->FirstRec = ToRecord;
    ToRecord->next = ToRecord;
    ToRecord->prev = ToRecord; }
  else { // Add new record after current record.
    ToBuf->CurrentRec = FromRecord;
    ToBuf->CurrentRec->prev = ToRecord;
    ToBuf->CurrentRec->next = ToRecord->next;
    ToRecord->next->prev = ToBuf->CurrentRec;
    ToRecord->next = ToBuf->CurrentRec; }
  ToBuf->CurrentByte = 0;
  ToBuf->LineNumber += 1; }

//----------------------------------------------BreakRecord
int BreakRecord(struct Buf *TextBuf)
  { //Break current record at current character.
  int BreakPointer = TextBuf->CurrentByte;
  struct Rec *BreakRec = TextBuf->CurrentRec;
  struct Rec *FirstRec;
  struct Rec *SecondRec;
  struct AnyTag **ThisTagPtr = &(BreakRec->TagChain);
  struct AnyTag *FirstRecLastTag = NULL;
  
  GetRecord(TextBuf, BreakPointer+1, NULL);
  FirstRec = TextBuf->CurrentRec;
  strncpy(FirstRec->text, BreakRec->text, BreakPointer);
  if (GetRecord(TextBuf, (strlen(BreakRec->text))-BreakPointer+1, (BreakRec->text)+BreakPointer))
    return 1;
  SecondRec = TextBuf->CurrentRec;
  
  //Now split any tags between the two halves adjusting target points as appropriate.
  if (*ThisTagPtr) {
    if ((*ThisTagPtr)->StartPoint < BreakPointer)
      FirstRec->TagChain = BreakRec->TagChain;
    while ((*ThisTagPtr) && (*ThisTagPtr)->StartPoint < BreakPointer) {
      if ((*ThisTagPtr)->type == TagType_Target)
        ((struct JumpHTabObj *)(*ThisTagPtr)->Attr)->TargetRec = FirstRec;
      else if (BreakPointer < (*ThisTagPtr)->EndPoint)
        (*ThisTagPtr)->EndPoint = BreakPointer;
      FirstRecLastTag = *ThisTagPtr;
      (*ThisTagPtr) = (*ThisTagPtr)->next; }
    
    SecondRec->TagChain = *ThisTagPtr;
    while ((*ThisTagPtr)) {
      (*ThisTagPtr)->StartPoint -= BreakPointer;
      if ((*ThisTagPtr)->type == TagType_Target)
        ((struct JumpHTabObj *)(*ThisTagPtr)->Attr)->TargetRec = SecondRec;
      else
        (*ThisTagPtr)->EndPoint -= BreakPointer;
      (*ThisTagPtr) = (*ThisTagPtr)->next; }
      
    if (FirstRecLastTag)
      FirstRecLastTag->next = NULL; }
    
  BreakRec->TagChain = NULL;
  TextBuf->CurrentRec = BreakRec;
  FreeRecord(TextBuf, AdjustForwards);
  TextBuf->CurrentRec = SecondRec;
  *((TextBuf->CurrentRec->prev->text)+BreakPointer) = '\0';
  TextBuf->CurrentByte = 0;
  return 0; }

//----------------------------------------------JoinRecords
int JoinRecords(struct Buf *TextBuf)
  {  //Join current record to next, returns 0 on success, nonzero on failure.
  struct Rec *FirstRecord = TextBuf->CurrentRec;
  char *FirstText = FirstRecord->text;
  struct Rec *SecondRecord = FirstRecord->next;
  char *SecondText = SecondRecord->text;
  struct Rec *NewRecord;
  char *NewText;
  int FirstLength = strlen(FirstText);
  struct AnyTag **FirstTagsPtr = &(FirstRecord->TagChain);
  struct AnyTag *SecondTags = SecondRecord->TagChain;
  
  if (SecondRecord == (TextBuf->FirstRec)) // End-flag record.
    return 1;
  
  TextBuf->CurrentRec = SecondRecord;
  if (GetRecord(TextBuf, FirstLength+strlen(SecondText)+1, NULL))
    return 1;
  NewRecord = TextBuf->CurrentRec;
  NewText = NewRecord->text;
  
  strcpy(NewText, FirstText);
  strcat(NewText, SecondText);
  
  //Concatinate tags, adjusting tags, target points and, in 2nd. record, the character no.
  NewRecord->TagChain = *FirstTagsPtr ? *FirstTagsPtr : SecondTags;
  while (*FirstTagsPtr) {
    if ((*FirstTagsPtr)->type == TagType_Target)
      ((struct JumpHTabObj *)(*FirstTagsPtr)->Attr)->TargetRec = NewRecord;
    if ((*FirstTagsPtr)->next)
      *FirstTagsPtr = (*FirstTagsPtr)->next;
    else {
      (*FirstTagsPtr)->next = SecondTags;
      break; } }
  
  while (SecondTags) {
    SecondTags->StartPoint += FirstLength;
    if (SecondTags->type != TagType_Target)
      SecondTags->EndPoint += FirstLength;
    if (SecondTags->type == TagType_Target)
      ((struct JumpHTabObj *)SecondTags->Attr)->TargetRec = NewRecord;
    SecondTags = SecondTags->next; }
  FirstRecord->TagChain = NULL;
  SecondRecord->TagChain = NULL;
  
  TextBuf->CurrentRec = FirstRecord;
  FreeRecord(TextBuf, AdjustForwards);
  FreeRecord(TextBuf, AdjustBack);
  TextBuf->LineNumber -= 1;
  TextBuf->CurrentByte = FirstLength;
  return 0; }

//----------------------------------------------SearchRecord
int SearchRecord(struct Buf *TextBuf, char *SearchString, int Direction) {
  //
  //Function searches just one record for substring,
  //  if found then returns TRUE and adjusts substring start pointer,
  //  if not then returns FALSE, substring start descriptor points to
  //  end of current record.
  //If case sensitivity is off this function assumes the search string is upper case.
  //
  char Chr, RefChr = -1, *Text = TextBuf->CurrentRec->text;
  int SubstringStart = 0;
  int Character = SearchString[0];
  int SearchStringLength;
  int RefByte;
  int SubstringLength;
  int FirstByte;
  if (Character == '\0') //The search string is empty - this is deemed to match to an empty substring in the buffer.
    return (0<Direction) ? (strlen(Text)==TextBuf->CurrentByte) : (TextBuf->CurrentByte==0);
  SearchStringLength = strlen(SearchString);
  SubstringLength = TextBuf->SubstringLength;
  
  if (SubstringLength < 0)
    FirstByte = TextBuf->CurrentByte + SubstringLength + Direction;
  else if (SubstringLength)
    FirstByte = TextBuf->CurrentByte + Direction;
  else
    FirstByte = TextBuf->CurrentByte;
  RefByte = 0;
  SubstringStart = FirstByte;
  
  if (Direction == 1) { //Forwards search.
    if (s_CaseSensitivity) { //Case sensitive, forwards search.
      while ( (RefChr = SearchString[RefByte]) && (Chr = Text[SubstringStart+RefByte++]) )
        if (RefChr != Chr) {
          SubstringStart++;
          RefByte = 0; } }
    else { //Case insensitive, forwards search.
      while ( (RefChr = SearchString[RefByte]) && (Chr = Text[SubstringStart+RefByte++]) )
        if ((char)toupper(RefChr) != (char)toupper(Chr)) {
          SubstringStart++;
          RefByte = 0; } } }
  else { //Reverse search.
    if (s_CaseSensitivity) { //Case sensitive, reverse search.
      while ( (0 <= SubstringStart) && (RefChr = SearchString[RefByte]) )
        if (RefChr != Text[SubstringStart+RefByte++]) {
          SubstringStart--;
          RefByte = 0; } }
    else { //Case insensitive, reverse search.
      while ( (0 <= SubstringStart) && (RefChr = SearchString[RefByte]) )
        if ((char)toupper(RefChr) != (char)toupper(Text[SubstringStart+RefByte++])) {
          SubstringStart--;
          RefByte = 0; } } }
          
  if ( ! RefChr) { //Search was successful
    TextBuf->SubstringLength = SearchStringLength;
    TextBuf->CurrentByte = SubstringStart;
    return TRUE; }
  else {
    TextBuf->CurrentByte = 0;
    TextBuf->SubstringLength = 0;
    return FALSE; } }

//----------------------------------------------Jot_strcasestr
char *Jot_strcasestr(char *Text, char *OrigSearchString) {
  //
  //A less efficient replacement for gnu library function strcasestr (case insensitive strstr) for windows.
  //Heigh-ho - if windows users were at all botherered about search efficiency then they wouldn't be windows users?
  //
  char StringArgCopy[StringMaxChr], *SearchString = StringArgCopy;
  int Byte, TextLength = strlen(Text), SearchStringLength = strlen(OrigSearchString), i = 0;
  
  if (StringMaxChr <= SearchStringLength || TextLength < SearchStringLength)
    return NULL;
    
  for ( ; i <= SearchStringLength ; i++)
    SearchString[i] = s_CaseSensitivity ? OrigSearchString[i] : (char)toupper(OrigSearchString[i]);
  
  for (Byte = 0; Byte <= TextLength-SearchStringLength; Byte++) { //First-character-search loop.
    if ( (SearchString[(0)] == (char)toupper(Text[Byte]))) { //Found a first-character match - check the remaining characters.
      for (i = 0; ; i++) { //Remaining-character verification loop.
        if ( ! SearchString[i] )
          return Text+Byte;
        if ((char)toupper(Text[Byte+i]) != SearchString[i])
          break; } } }
    
 return NULL; }

 //----------------------------------------------GeneralSearchBuffer
int GeneralSearchBuffer(struct Buf *TextBuf, char *OrigSearchString, char TabChr, int MultiSearchArg, int Direction, int OrigLineCount, int LastByte, int Reverse)
  { //Search of buffer subsection defined by current chr position and LineCount & LastByte, reverse reverses the final substring with optional lists of search strings.
  int MatchedLineNo, MatchedByte, MatchedSearchStringLength, OrigSearchStringLength = strlen(OrigSearchString);
  int i, j, RefStartPoint = TextBuf->CurrentByte;
  char *SubString, StringArgCopy[StringMaxChr], *SearchString = StringArgCopy;
  //SearchString is a list of C null-terminated strings, 
  //each of these strings is followed by a status byte another null byte follows the last search string's status byte.
  //The status byte is Initialized to '\0', and set to '\1' when the correisponding search string has been matched.
  //Initially:
  //   <String1>, '\0', '\0', <string2>, '\0', '\0', <string3>, '\0', '\0', ... , <lastString>, '\0', '\0', '\0'
  //After successfully matching all search strings:
  //   <String1>, '\0', '\1', <string2>, '\0', '\1', <string3>, '\0', '\1', ... , <lastString>, '\0', '\1', '\0'
  //
  i = j = 0;
  if ( ! OrigSearchStringLength)
    return TRUE;
  if (StringMaxChr < OrigSearchStringLength) {
    Fail("Overlong search string");
    return FALSE; }
  while ( 1 ) {
    char Chr = OrigSearchString[i++];
    if ( (MultiSearchArg && (Chr == TabChr) ) || Chr == '\0') {
      SearchString[j++] = '\0';
      SearchString[j++] = '\0';
      if (Chr == '\0')
       break; }
    else {
      SearchString[j++] = s_CaseSensitivity ? Chr : toupper(Chr);
      if ( ! Chr)
        break; } }
  SearchString[j++] = '\0';
  SearchString[j] = '\0';

  if (0 < Direction) { //Forward searches.
    MatchedLineNo = INT_MAX;
    MatchedByte = INT_MAX;
    if (0 < TextBuf->SubstringLength)
      RefStartPoint += 1;
    TextBuf->SubstringLength = 0;
    if (s_CaseSensitivity) { //Case-sensitive forward search.
      int LineCount = OrigLineCount, SearchStringLength = strlen(SearchString), StartPoint = RefStartPoint;
      MatchedLineNo = INT_MAX;
      MatchedByte = INT_MAX;
      do { //Record loop - search for each of the search strings in turn before advancing to next record.
        int Byte = -1;
        SearchString = StringArgCopy;
        if (LineCount == 0 && LastByte < 0) //Search to end of last line.
          LastByte = INT_MAX;
        do { //Search-strings loop.
          while ( SearchString[0] ) { //Search for next unmatched search string.
            SearchStringLength = strlen(SearchString);
            if ( ! (SearchString+SearchStringLength)[1])
              break;
            SearchString += SearchStringLength+2; }
          if ( ! SearchString[0]) //Exhausted the list of search strings in this record.
            break;
          if ( (SubString = strstr((TextBuf->CurrentRec->text)+StartPoint, SearchString)) )
            Byte = SubString-(TextBuf->CurrentRec->text);
          if (SubString && ( LineCount || (Byte <= LastByte)) ) { //A valid match found - not only was there a match but it did not occur after the end byte of the last line.
            if ( (TextBuf->LineNumber < MatchedLineNo) || (TextBuf->LineNumber == MatchedLineNo && Byte < MatchedByte) ) { //This match is before other ones - note the match details.
              MatchedLineNo = TextBuf->LineNumber;
              MatchedByte = Byte;
              MatchedSearchStringLength = SearchStringLength; }
            (SearchString+SearchStringLength)[1] = '\1';
            if ( ! MultiSearchArg || MultiSearchArg == F_any ) { //For single search strings and F_any we can exit all of straightaway.
              LineCount = 0;
              break; } }
          if ( ! MultiSearchArg ) //For failed simple searches in this record exit and move to the next record.
            break; 
          SearchString += SearchStringLength+2;
          TextBuf->CurrentByte = StartPoint; }
          while ( SearchString[0] );
        if (0 <= Byte && (MultiSearchArg == F_first) )
          break;
        StartPoint = 0; }
        while ( LineCount-- && ! AdvanceRecord(TextBuf, 1)); }

    else { //Case-insensitive forward search.
      int LineCount = OrigLineCount, SearchStringLength = strlen(SearchString), StartPoint = RefStartPoint;
      MatchedLineNo = INT_MAX;
      MatchedByte = INT_MAX;
      do { //Record loop - search for each of the search strings in turn before advancing to next record.
        int Byte = -1;
        SearchString = StringArgCopy;
        if (LineCount == 0 && LastByte < 0) //Search to end of last line.
          LastByte = INT_MAX;
        do { //Search-strings loop.
          while ( SearchString[0] ) { //Search for next unmatched search string.
            SearchStringLength = strlen(SearchString);
            if ( ! (SearchString+SearchStringLength)[1])
              break;
            SearchString += SearchStringLength+2; }
          if ( ! SearchString[0]) //Exhausted the list of search strings in this record.
            break;
#if defined(VC)
          if ( (SubString = Jot_strcasestr((TextBuf->CurrentRec->text)+StartPoint, SearchString)) )
#else
          if ( (SubString = strcasestr((TextBuf->CurrentRec->text)+StartPoint, SearchString)) )
#endif
            Byte = SubString-(TextBuf->CurrentRec->text);
          if (SubString && ( LineCount || (Byte <= LastByte)) ) { //A valid match found - not only was there a match but it did not occur after the end byte of the last line.
            if ( (TextBuf->LineNumber < MatchedLineNo) || (TextBuf->LineNumber == MatchedLineNo && Byte < MatchedByte) ) { //This match is before other ones - note the match details.
              MatchedLineNo = TextBuf->LineNumber;
              MatchedByte = Byte;
              MatchedSearchStringLength = SearchStringLength; }
            (SearchString+SearchStringLength)[1] = '\1';
            if ( ! MultiSearchArg || MultiSearchArg == F_any ) { //For single search strings and F_any we can exit all of straightaway.
              LineCount = 0;
              break; } }
          if ( ! MultiSearchArg ) //For failed simple searches in this record exit and move to the next record.
            break; 
          SearchString += SearchStringLength+2;
          TextBuf->CurrentByte = StartPoint; }
          while ( SearchString[0] );
        if (0 <= Byte && (MultiSearchArg == F_first) )
          break;
        StartPoint = 0; }
        while ( LineCount-- && ! AdvanceRecord(TextBuf, 1)); } }

  else { //Backwards searches.
    MatchedLineNo = INT_MIN;
    MatchedByte = INT_MIN;
    if (0 < TextBuf->SubstringLength)
      RefStartPoint -= 1;
    else if (TextBuf->SubstringLength < 0)
      RefStartPoint -= -(TextBuf->SubstringLength);
    TextBuf->SubstringLength = 0;
    if (s_CaseSensitivity) { //Case-sensitive backwards search.
      int LineCount = OrigLineCount, StartPoint = RefStartPoint, SearchStringLength = strlen(SearchString);
      while ( 1 ) { //Record loop.
        if (LineCount == 0 && LastByte < 0) //Search to start of last line.
          LastByte = 0;
        while ( 1 ) { //Search-strings loop.
          int Byte = StartPoint-1, ThisRecMatchedByte = -1;
          if (0 <= Byte) {
            do { //First-character-search loop.
              if ( (SearchString[(i = 0)] == TextBuf->CurrentRec->text[Byte])) { //Found a first-character match - check the remaining characters.
                do { //Remaining-character verification loop.
                  if (SearchStringLength <= ++i) {
                    ThisRecMatchedByte = Byte;
                    break; } }
                  while ( SearchString[i] == TextBuf->CurrentRec->text[Byte+i]); } }
              while ( (ThisRecMatchedByte < 0) && (0 <= --Byte) );
            //It should only get down here after a matching substring has been confirmed.
            if (0 <= ThisRecMatchedByte) { //Match found in this record.
              if ( ( LineCount || (LastByte <= ThisRecMatchedByte)) ) { //A valid match found - not only was there a match but it did not occur before the end byte of the last line.
                if ( (MatchedLineNo < TextBuf->LineNumber) || (TextBuf->LineNumber == MatchedLineNo && MatchedByte < ThisRecMatchedByte) ) { //This match is before other ones - note the match details.
                  MatchedLineNo = TextBuf->LineNumber;
                  MatchedByte = ThisRecMatchedByte;
                  MatchedSearchStringLength = SearchStringLength; }
                (SearchString+SearchStringLength)[1] = '\1';
                if ( ! MultiSearchArg || MultiSearchArg == F_any ) { //For single search strings and F_any we can exit all of straightaway.
                  LineCount = 0;
                  break; } } } }
          //For F_all or an unsuccesful search, we need to try matching all the other unmatched search strings in this record.
          while ( 1 ) { //Search for next unmatched search string.
            SearchString += SearchStringLength+2;
            SearchStringLength = strlen(SearchString);
            if ( ! SearchString[0] || ! (SearchString+SearchStringLength)[1]) //No more search strings or the new search string has already been matched.
              break; }
          if ( ! MultiSearchArg ) //For failed simple searches in this record exit and move to the next record.
            break; 
          if ( ! SearchString[0] ) //Exhausted the list of search strings in this record.
            break; }
        if ( ! LineCount-- || AdvanceRecord(TextBuf, -1)) //Index to next record.
          break;
        if ( (MultiSearchArg == F_first) && (INT_MIN < MatchedByte) )
          break;
        SearchString = StringArgCopy;
        SearchStringLength = strlen(SearchString);
        StartPoint = 0;
        for (StartPoint = s_CurrentBuf->CurrentRec->MaxLength-1; 0 <= StartPoint; StartPoint--)
          if (TextBuf->CurrentRec->text[StartPoint])
            break; } }

    else {  //Case-insensitive backwards search
      int LineCount = OrigLineCount, StartPoint = RefStartPoint, SearchStringLength = strlen(SearchString);
      while ( 1 ) { //Record loop.
        if (LineCount == 0 && LastByte < 0) //Search to start of last line.
          LastByte = 0;
        while ( 1 ) { //Search-strings loop.
          int Byte = StartPoint-1, ThisRecMatchedByte = -1;
          if (0 <= Byte) {
            do { //First-character-search loop.
              if ( (SearchString[(i = 0)] == (char)toupper(TextBuf->CurrentRec->text[Byte]))) { //Found a first-character match - check the remaining characters.
                do { //Remaining-character verification loop.
                  if (SearchStringLength <= ++i) {
                    ThisRecMatchedByte = Byte;
                    break; } }
                  while ( SearchString[i] == (char)toupper(TextBuf->CurrentRec->text[Byte+i])); } }
              while ( (ThisRecMatchedByte < 0) && (0 <= --Byte) );
            //It should only get down here after a matching substring has been confirmed.
            if (0 <= ThisRecMatchedByte) { //Match found in this record.
              if ( ( LineCount || (LastByte <= ThisRecMatchedByte)) ) { //A valid match found - not only was there a match but it did not occur before the end byte of the last line.
                if ( (MatchedLineNo < TextBuf->LineNumber) || (TextBuf->LineNumber == MatchedLineNo && MatchedByte < ThisRecMatchedByte) ) { //This match is before other ones - note the match details.
                  MatchedLineNo = TextBuf->LineNumber;
                  MatchedByte = ThisRecMatchedByte;
                  MatchedSearchStringLength = SearchStringLength; }
                (SearchString+SearchStringLength)[1] = '\1';
                if ( ! MultiSearchArg || MultiSearchArg == F_any ) { //For single search strings and F_any we can exit all of straightaway.
                  LineCount = 0;
                  break; } } } }
          //For F_all or an unsuccesful search, we need to try matching all the other unmatched search strings in this record.
          while ( 1 ) { //Search for next unmatched search string.
            SearchString += SearchStringLength+2;
            SearchStringLength = strlen(SearchString);
            if ( ! SearchString[0] || ! (SearchString+SearchStringLength)[1]) //No more search strings or the new search string has already been matched.
              break; }
          if ( ! MultiSearchArg ) //For failed simple searches in this record exit and move to the next record.
            break; 
          if ( ! SearchString[0] ) //Exhausted the list of search strings in this record.
            break; }
        if ( ! LineCount-- || AdvanceRecord(TextBuf, -1)) //Index to next record.
          break;
        if ( (MultiSearchArg == F_first) && (INT_MIN < MatchedByte) )
          break;
        SearchString = StringArgCopy;
        SearchStringLength = strlen(SearchString);
        StartPoint = 0;
        for (StartPoint = TextBuf->CurrentRec->MaxLength-1; 0 <= StartPoint; StartPoint--)
          if (TextBuf->CurrentRec->text[StartPoint])
            break; } } }

      if (MatchedLineNo == INT_MAX || MatchedLineNo == INT_MIN)
        return FALSE;
      if (MultiSearchArg == F_all) { //Check that all search strings have been matched.
        int SearchStringLength;
        SearchString = StringArgCopy;
        while ( SearchString[0] ) {
          SearchStringLength = strlen(SearchString);
          if ( ! (SearchString+SearchStringLength)[1]) //This search string has not been matched.
            return FALSE;
          SearchString += SearchStringLength+2; } }
        
      AdvanceRecord(TextBuf, MatchedLineNo-TextBuf->LineNumber);
      if (Reverse) {
        TextBuf->CurrentByte = MatchedByte+MatchedSearchStringLength;
        TextBuf->SubstringLength = -MatchedSearchStringLength; }
      else {
        TextBuf->CurrentByte = MatchedByte;
        TextBuf->SubstringLength = MatchedSearchStringLength; }
      return TRUE; }

//----------------------------------------------SimpleSearchBuffer
int SimpleSearchBuffer(struct Buf *TextBuf, char *OrigSearchString, int Direction, int Reverse)
  { //Simple Search of remaining buffer from current record and byte, reverse reverses the final substring..
  int ThisRecMatchedByte = -1;
  int i, SearchStringLength = strlen(OrigSearchString);
  int StartPoint = TextBuf->CurrentByte;
  char *SearchString;
  
  if ( ! SearchStringLength)
    return TRUE;
  
  SearchString = OrigSearchString;
  
  if (0 < Direction) { //Forwards searches.
    if (0 < TextBuf->SubstringLength)
      StartPoint += 1;
    TextBuf->SubstringLength = 0;
      
    if (s_CaseSensitivity) { //Case-sensitive forward search.
      char * SubString;
      while ( 1 ) { //Record loop, if pattern not matched in this record, move record pointer.
        if ( (SubString = strstr((TextBuf->CurrentRec->text)+StartPoint, SearchString)) ) {
          ThisRecMatchedByte = SubString-TextBuf->CurrentRec->text;
          break; }
        if (AdvanceRecord(TextBuf, 1) )
          return FALSE;
        StartPoint = 0; } }

    else { //Case-insensitive forwards search.
      char * SubString;
      while ( 1 ) { //Record loop, if pattern not matched in this record, move record pointer.
#if defined(VC)
        if ( (SubString = Jot_strcasestr((TextBuf->CurrentRec->text)+StartPoint, SearchString)) )
#else
        if ( (SubString = strcasestr((TextBuf->CurrentRec->text)+StartPoint, SearchString)) )
#endif
        {
          ThisRecMatchedByte = SubString-TextBuf->CurrentRec->text;
          break; }
        if (AdvanceRecord(TextBuf, 1) )
          return FALSE;
        StartPoint = 0; } } }

  else {  //Backwards searches.
    if (0 < TextBuf->SubstringLength)
      StartPoint -= 1;
    else if (TextBuf->SubstringLength < 0)
      StartPoint -= 1-TextBuf->SubstringLength;
    TextBuf->SubstringLength = 0;
      
    if (s_CaseSensitivity) { //Case-sensitive backwards search.
      while ( 1 ) { //Record loop.
        char *Text = TextBuf->CurrentRec->text;
        int Byte = StartPoint;
        if (0 <= StartPoint) {
          do { //First-character-search loop.
            if ( (SearchString[(i=0)] == Text[Byte])) { //Found a first-character match - check the remaining characters.
              do {
                if (SearchString[i] != Text[Byte+i])
                  break;
                if (SearchStringLength <= ++i) {
                  ThisRecMatchedByte = Byte;
                  Byte = -1; } }
                while (ThisRecMatchedByte < 0); } }
            while ( 0 <= --Byte ); }
          if (0 <= ThisRecMatchedByte)  //Has a match been found in this record?
            break;
          if (AdvanceRecord(TextBuf, -1) )
            return FALSE;
          Text = TextBuf->CurrentRec->text;
          StartPoint = 0;
          for (StartPoint = s_CurrentBuf->CurrentRec->MaxLength-1; 0 <= StartPoint; StartPoint--)
            if (Text[StartPoint])
              break; } }

    else {  //Case-insensitive backwards search
      char StringArgCopy[StringMaxChr];
      if (StringMaxChr < strlen(OrigSearchString)) {
        Fail("Overlong search string");
        return FALSE; }
      for ( i = 0; i <= strlen(OrigSearchString); i++)
        StringArgCopy[i] = toupper(OrigSearchString[i]);
      SearchString = StringArgCopy;
      
      while ( 1 ) { //Record loop.
        char *Text = TextBuf->CurrentRec->text;
        int Byte = StartPoint;
        if (0 <= StartPoint) {
          do { //First-character-search loop.
            if ( (SearchString[(i=0)] == (char)toupper(Text[Byte]))) { //Found a first-character match - check the remaining characters.
              do {
                if (SearchString[i] != (char)toupper(Text[Byte+i]))
                  break;
                if (SearchStringLength <= ++i) {
                  ThisRecMatchedByte = Byte;
                  Byte = -1; } }
                while (ThisRecMatchedByte < 0); } }
            while ( 0 <= --Byte ); }
          if (0 <= ThisRecMatchedByte)  //Has a match been found in this record?
            break;
          if (AdvanceRecord(TextBuf, -1) )
            return FALSE;
          Text = TextBuf->CurrentRec->text;
          StartPoint = 0;
          for (StartPoint = s_CurrentBuf->CurrentRec->MaxLength-1; 0 <= StartPoint; StartPoint--)
            if (Text[StartPoint])
              break; } } }

      if (Reverse) {
        TextBuf->CurrentByte = ThisRecMatchedByte+SearchStringLength;
        TextBuf->SubstringLength = -SearchStringLength; }
      else {
        TextBuf->CurrentByte = ThisRecMatchedByte;
        TextBuf->SubstringLength = SearchStringLength; }
      return TRUE; }

//----------------------------------------------RegexSearchBuffer
int RegexSearchBuffer(struct Buf *TextBuf, char BufferKey, char *SearchString, int Direction, int LineCount, int LastByte)
  { // Regular-expression Search of buffer, limited to a number of records.
#if defined(NoRegEx)
  return FALSE;
#else
  regex_t re;
  regmatch_t pmatch[100];
  int TempCount = 0;  //Indicates no. of records searched.
  int frame = 0;      //Used to allocate each regexec call a new frame in pmatch in backwards searches.
  size_t frameSize;   //Used in management of and extraction from frames in pmatch array.
  int IgnoreCase = s_CaseSensitivity ? 0 : REG_ICASE;
  int StartPoint = TextBuf->CurrentByte;
  struct Buf *DestBuf = NULL;
  int RegExFailed = FALSE, i, Byte;
  
  //This deals with the situation where the search follows an earlier successful search - possibly of the same string.
  if (TextBuf->SubstringLength) {
    if (TextBuf->SubstringLength < 0)
      StartPoint += TextBuf->SubstringLength + Direction;
    else
      StartPoint += Direction;
    TextBuf->SubstringLength = 0; }
    //Having done that, we may have left the StartPoint out of bounds.
    if (0 < Direction) { //Forwards search.
      if (strlen(TextBuf->CurrentRec->text) <= StartPoint) { //Oh dear - we need to move to the next record.
        if (LineCount-- == 0 || AdvanceRecord(TextBuf, 1))
          return FALSE;
        StartPoint = 0; } }
    else { //Backwards search.
      if (StartPoint <= 0) { //Oh dear - we need to move to the end of the previousrecord.
        if (LineCount-- == 0 || AdvanceRecord(TextBuf, -1))
          return FALSE;
        StartPoint = strlen(TextBuf->CurrentRec->text); } }
    
  if (BufferKey) {
    DestBuf = GetBuffer(BufferKey, AlwaysNew);
    if (DestBuf == NULL)
      return FALSE; }
  
  if (regcomp(&re, SearchString, IgnoreCase|REG_EXTENDED) != 0) { //Compile the regular expression.
    Fail("Invalid regular expression %s", SearchString);
    regfree(&re);
    if (DestBuf) {
      if ( ! DestBuf->CurrentRec )
        GetRecord(DestBuf, 1, "");
      DestBuf->CurrentRec->text[0] = '\0'; }
    return FALSE; }
  else if (DestBuf)
    GetRecord(DestBuf, 100, NULL);
     
  frameSize = re.re_nsub+1;
  
  if (0 < Direction) { //Forward search.
    for (;;) { //Record loop, if pattern not matched in this record, move record pointer.
      if (StartPoint < strlen(TextBuf->CurrentRec->text)) {
        if ( ! regexec(&re, (TextBuf->CurrentRec->text)+StartPoint, 100, pmatch, 0) )
          break; }
      StartPoint = 0;
      if ( ! LineCount-- || AdvanceRecord(TextBuf, Direction)) { //End of buffer or record-limit reached.
        regfree(&re);
        if (DestBuf)
          GetRecord(DestBuf, 1, "");
        return FALSE;
        TempCount++; } } }
  else {  //For reverse searches, do repeated forwards searches until last match before current character.
    int endStop = TextBuf->CurrentByte;
    
    for (;;) { //  Backwards-search record loop - implemented by repeated forward searches in the same line.
              //  Endstop indicates the rightmost allowable extent for valid matches.
              //  Repeat RE search in this line until endstop reached then return previous match.
      char *rec = TextBuf->CurrentRec->text;
      int firstChr;
      int pass2OrMore = 0; //Flag indicating 2nd or subsequent retry in this record, also an increment to offset the start point after first successful match.
       
      StartPoint = 0;
       
      //Inner loop locates the last matching substring before the endStop
      //If there was a previous successful match in this record then offset the start point.
      for ( ; ; ) {
        int status = regexec(&re, rec+StartPoint+pass2OrMore, 100-frame, pmatch+frame, 0);
        firstChr = pmatch[frame].rm_so;
        if ( status || firstChr <= 0 || (endStop <= (StartPoint+pass2OrMore+firstChr)) ) { //Exit RE-match loop if match fails or we've gone past the endStop.
          if (pass2OrMore)
            goto reverseRegexMatchFound;
          break; }
        StartPoint += firstChr+pass2OrMore;
        frame += frameSize;
        pass2OrMore = 1; }
      RegExFailed |= ( ! LineCount-- ) || AdvanceRecord(TextBuf, Direction);
      frame = 0; 
      endStop = strlen(TextBuf->CurrentRec->text);
      if (RegExFailed) {
        regfree(&re);
        if (DestBuf)
          GetRecord(DestBuf, 1, "");
        return FALSE; }
      TempCount--; }
      
reverseRegexMatchFound: //Last regexec call failed but the previous match was successful - rewind pmatch and StartPoint.
    TextBuf->CurrentByte = 0;
    frame -= frameSize; 
    StartPoint -= pmatch[frame].rm_so; }
  
  for (i = 0; i < frameSize; i++) {
    regoff_t start = pmatch[frame+i].rm_so;
    regoff_t end = pmatch[frame+i].rm_eo;
    Byte = StartPoint+pmatch[frame].rm_so;
    if ( LineCount || (Byte <= LastByte) ) { //The match was within the soecified range.
      TextBuf->CurrentByte = Byte;
      TextBuf->SubstringLength = end-start; }
    if (DestBuf) {
      GetRecord(DestBuf, end-start+1, NULL);
      strncat(DestBuf->CurrentRec->text, (s_CurrentBuf->CurrentRec->text)+StartPoint+start, end-start); } }
      
  if (DestBuf) {
    ResetBuffer(DestBuf);
    DestBuf->AbstractWholeRecordFlag = TRUE; }
  regfree(&re);
  return (TextBuf->SubstringLength != 0);
#endif
  }

//----------------------------------------------SubstituteString
int SubstituteString(struct Buf *DestBuf, const char *SourceString, int Length)
  {
  //
  //Replaces current substring of DestBuf with string in SourceString.
  //Length is the no. of SourceString characters to be used - any negative value indicates all of the SourceString.
  //If no substring in DestBuf (SubstringLength == 0) then inserts from SourceString.
  //A special case, delete string, when SourceString == NULL is also recognized.
  //
  struct Rec *DestRecord = DestBuf->CurrentRec;
  char *DestText = DestRecord->text;
  int SubLength = DestBuf->SubstringLength;        //The original substring length.
  int SubStart;                                    //The character no. in the dest record to receive the first chr from the source string.
  int SourceLength;                                //The size of the string to be pushed in.
  int Offset;                                      //The change in length of the destination record.
  int LastByte;                                    //The last byte of the original string (immediately before the terminating null byte).
  int Headroom;                                    //Number of bytes available for expansion.
  struct AnyTag *ThisTag = DestRecord->TagChain;   //The start of the tag chain.
  
  if (DestText == SourceString) {
    RunError("Can't substitute a string with itself");
    return 1; }
  if (0 <= SubLength)
    SubStart = DestBuf->CurrentByte;
  else {
    SubLength = -SubLength; 
    SubStart = DestBuf->CurrentByte - SubLength; }
  DestRecord->DisplayFlag = Redraw_Line;
  if (SourceString != NULL)
    SourceLength = (0 <= Length) ? Length : strlen(SourceString);
  else
    SourceLength = 0;
    
  if (SubStart < 0) {
    Fail("Invalid SubstringLength");
    return 1; }
  
  if (SubLength != 0)
    Offset = SourceLength-SubLength;
  else { // Null substring in destination - insert.
    if (SourceLength == 0)
      return 0;
    Offset = SourceLength;
    SubStart = DestBuf->CurrentByte;
    DestBuf->CurrentByte = SubStart; }
  
  //The use of LastByte and Headroom is intended to avoid overuse of strlen - this can be *very* expensive for long lines.
  LastByte = DestRecord->MaxLength-2;
  while ((0 < LastByte) && ! DestText[LastByte] )
    LastByte--;
  Headroom = DestRecord->MaxLength-LastByte-2;
      
  if (0 < Offset) { //The destination string is growing.
    //Now it could be that there's sufficient headroom anyway - try that first to avoid the malloc overhead.
    if (Headroom < Offset) { //No - we must malloc a bigger string.
      int DestStringLength = LastByte+Offset+1;
      char *NewText = (char *)JotMalloc(DestStringLength+1);
      
      if (NewText == NULL) {
        RunError("SubstituteString:run out of heap space.");
        return 1; }
      strncpy(NewText, DestText, SubStart);
      strncpy(NewText+SubStart, SourceString, SourceLength);
      strncpy(NewText+SubStart+SourceLength, DestText+SubStart+SubLength, LastByte-SubStart-SubLength+2);
      free(DestText);
      DestBuf->CurrentRec->text = NewText;
      DestRecord->MaxLength = DestStringLength+1; }
    else { //String is growing and record is big enough already, just copy in the new stuff - first find the end of the string.
      memmove(DestText+SubStart+Offset, DestText+SubStart, LastByte-SubStart+1);
      memcpy(DestText+SubStart, SourceString, SourceLength); }
    DestBuf->SubstringLength = SourceLength;
    DestBuf->CurrentByte = SubStart; }
      
  else { //String shrinks or remains the same length.
    if (Offset < 0) //String is shrinking - shift left.
      memmove(DestText+SubStart, DestText+SubStart-Offset, LastByte-SubStart+Offset+2);
    if (SourceString != NULL) //Insert the string now.
      strncpy(DestText+SubStart, SourceString, SourceLength);
    DestBuf->SubstringLength = SourceLength;
    DestBuf->CurrentByte = SubStart;
    
    //Now pad out the end of the record with null bytes - first locate end of old string then insert nulls.
    memset(DestText+LastByte+Offset+2, '\0', -Offset); }
    
  //Check and adjust tags.
  while (ThisTag) {
    struct AnyTag *NextTag = ThisTag->next;
    if (SubStart <= ThisTag->StartPoint)
      ThisTag->StartPoint = (ThisTag->StartPoint+Offset < 0) ? 0 : ThisTag->StartPoint+Offset;
    if ( (ThisTag->type != TagType_Target) && (SubStart < ThisTag->EndPoint) ) { //There's an endpoint that's affected by the substitution.
      if (0 < Offset)
        ThisTag->EndPoint += Offset;
      else
        ThisTag->EndPoint = (ThisTag->EndPoint < 0) ? 0 : ThisTag->EndPoint+Offset;
      if (ThisTag->EndPoint < ThisTag->StartPoint) //Should never happen.
        FreeTag(DestRecord, ThisTag); }
    
    ThisTag = NextTag; }
  return 0; }

//----------------------------------------------CheckNextCommand
char CheckNextCommand(struct Buf *Buffer, char *MatchChrsString)
  { //If the next character in the buffer matches any in the list then it extracts and returns that character, otherwise returns '\0'.
  char Chr = StealCharacter(Buffer);
  return strchr(MatchChrsString, Chr) ? ImmediateCharacter(Buffer) : '\0'; }

//----------------------------------------------StealCharacter
char StealCharacter(struct Buf *TextBuf)
  { //Returns next non-blank character in buffer TextBuf if buffer empty then returns 0.
  char TempChr = 0;
  int Pointer;
  
  for (Pointer = TextBuf->CurrentByte; Pointer <= strlen(TextBuf->CurrentRec->text); Pointer+=1)
    { // Skip spaces repeat loop.
    TempChr = (TextBuf->CurrentRec->text)[Pointer];
    if (TempChr == ' ')
      continue;
    TextBuf->CurrentByte = Pointer;
    return TempChr; }
  
  return 0; }

//----------------------------------------------NextNonBlank
char NextNonBlank(struct Buf *TextBuf)
  { //Returns next character in string and updates pointer.
  for ( ; ; ) { //Skip records loop.
    char *Text = TextBuf->CurrentRec->text;
    int Pointer = TextBuf->CurrentByte;
    int Length = strlen(Text);
    
    for ( ;Pointer < Length; Pointer++) { //Skip spaces repeat loop.
      if (*(Text+Pointer) == ' ')
        continue;
      TextBuf->CurrentByte = Pointer+1;
      return Text[Pointer]; }
    
    if (AdvanceRecord(TextBuf, 1))
      break; }
  
  TextBuf->CurrentByte = strlen(TextBuf->CurrentRec->text);
  return 0;  }

//----------------------------------------------ImmediateCharacter
char ImmediateCharacter(struct Buf *TextBuf)
  { //Returns next Character in string and updates pointer.
  int Pointer = TextBuf->CurrentByte;
  char *Text = TextBuf->CurrentRec->text;
  
  if ((strlen(Text)) <= Pointer)
    return 0;
  TextBuf->CurrentByte = Pointer+1;
  return *(Text+Pointer);  }

//----------------------------------------------ChrUpper
int ChrUpper(char Chr)
  {
  //
  //Function returns upper-case of any lower-case characters it is passed.
  //
  return ((('a' <= Chr) && (Chr <= 'z')) ? Chr-'a'+'A' : Chr);  }

//----------------------------------------------VerifyNonBlank
int VerifyNonBlank(struct Buf *TextBuf)
  { //Returns TRUE if any nonblank characters in string and updates pointer.
    for (;;) { //Skip records loop.
      char *Text = TextBuf->CurrentRec->text;
      int Pointer = TextBuf->CurrentByte;
  
      for ( ; Pointer < strlen(Text); Pointer++) { //Skip spaces repeat loop.
        char TempChr = *(Text+Pointer);
  
        if (TempChr == ' ')
          continue;
        TextBuf->CurrentByte = Pointer;
        return TRUE; }
      if (AdvanceRecord(TextBuf, 1)) { //No non-blank characters found.
        TextBuf->CurrentByte = Pointer;
        return FALSE; } }
  
  return FALSE; }

//----------------------------------------------VerifyKey
char VerifyKey(int Chr)
  {
  //
  //Function upper-case Chr if next character is a valid buffer key,
  //otherwise returns NULL.
  //
  Chr = ChrUpper(Chr);
  return (' ' < Chr) && (Chr < 'a') ? Chr : '\0';  }

//----------------------------------------------VerifyDigits
int VerifyDigits(struct Buf *TextBuf)
  {
  //
  //Function returns TRUE if next Character is an ASCII digit.
  //
  int Character = StealCharacter(TextBuf);
  
  return (((Character>='0') && (Character <= '9')) || ((Character == '+') || (Character == '-')));  }

//----------------------------------------------VerifyAlpha
int VerifyAlpha(struct Buf *TextBuf)
  {
  //
  //Function returns TRUE if next character is an ASCII A-Z or a-z.
  //  Does not affect any pointers.
  //
  int Character = 0;
   
  Character = ChrUpper(StealCharacter(TextBuf));
  return ('A' <= Character) && (Character <= 'Z');  }

//----------------------------------------------VerifyCharacter
int VerifyCharacter(struct Buf *TextBuf, int Character)
  {
  //Returns TRUE, and moves pointer if next character same as Character, FALSE and leaves pointer otherwise.
  if (StealCharacter(TextBuf) == Character) { //Yes it matches.
    TextBuf->CurrentByte++;
    return TRUE; }
  else 
    return FALSE; }

//----------------------------------------------GetDec
long long GetDec(struct Buf *TextBuf)
  { //Returns decimal translation of text in TextBuf.
  long long Total = 0;
  long long Character = NextNonBlank(TextBuf);
  char *Text = TextBuf->CurrentRec->text;
  int CharacterPointer;
  long long Minus = FALSE;
  int CharacterLimit = strlen(Text);
  
  switch (Character) {
  default:
    CharacterPointer = TextBuf->CurrentByte-1;
    break;
  case '-':
    Minus = TRUE;
  case '+':
    CharacterPointer = TextBuf->CurrentByte;
    break; } //] End of case block.
  
  for (; CharacterPointer < CharacterLimit; ) { // Main loop.
    Character = (Text)[CharacterPointer++];
    if ((Character < '0') || ('9' < Character)) {
      CharacterPointer--;
      break; }
    Total = (Total*10)+Character-'0'; }
     
  TextBuf->CurrentByte = CharacterPointer;
  return Minus ? -Total : Total; }

//----------------------------------------------GetOct
int GetOct(struct Buf *TextBuf)
  {
  //
  //Function returns Octal translation of text in TextBuf.
  //
  int Total = 0;
  int Character = NextNonBlank(TextBuf);
  struct Rec *Record = TextBuf->CurrentRec;
  int CharacterPointer = TextBuf->CurrentByte;
  int Flag = FALSE;
  int CharacterLimit = strlen(Record->text);
  int Pointer;
  
  for (Pointer = CharacterPointer; Pointer < CharacterLimit; Pointer +=1) { // Main loop.
    Total = (Total*8)+Character-'0';
    Character = *((Record->text) + Pointer);
    if (('0' <= Character) && (Character <= '7'))
      continue;
    TextBuf->CurrentByte = Pointer;
    if (Flag)
      Total = -Total;
    return Total;
    } //[)
  
  Total = (Total*8)+Character-'0';
  TextBuf->CurrentByte = CharacterLimit;
  if (Flag)
    Total = -Total;
  return Total;  }

//----------------------------------------------GetHex
int GetHex(struct Buf *TextBuf)
  { //Function returns Hex. translation of string
  int Total = 0;
  int Character = 0;
  
  Character = NextNonBlank(TextBuf);
  
  for (;;) { 
    Character = ChrUpper(Character);
    if ('A' <= Character) { // A or bigger.
      if ('F' < Character)
        break;
      Total = (Total<<4)+(Character-'A'+10); }
    else { //Less than 'A'
      if ((Character < '0') || ('9' < Character))
        break;
      Total = (Total<<4)+(Character-'0'); }
    Character = ImmediateCharacter(TextBuf); }
  if (Character != 0)
    TextBuf->CurrentByte--;
  return Total;  }

//----------------------------------------------GetFormatted
int GetFormatted(struct Buf *TextBuf, char FormatKey)
  {
  //
  //Function returns scanf conversion of text in TextBuf.
  //
  char *Record = TextBuf->CurrentRec->text;
  int CharacterPointer = TextBuf->CurrentByte;
  int Result;
  char Format[20];
  int CharCount;
  
  sprintf(Format, "%%%c%%n", FormatKey);
  sscanf(Record+CharacterPointer, Format, &Result, &CharCount);
  TextBuf->SubstringLength = CharCount;
  TextBuf->CurrentByte = CharacterPointer+CharCount;
  return Result;  }

//----------------------------------------------ClearRecord
void ClearRecord(struct Buf *TextBuf)
  {
  //
  //Resets Substring length to zero, does not remove old characters.
  //
  *(TextBuf->CurrentRec->text) = '\0';
  TextBuf->CurrentByte = 0;
  TextBuf->SubstringLength = 0;
  return;  }

//----------------------------------------------ResetBuffer
void ResetBuffer(struct Buf *TextBuf)
  {
  //
  //Resets character pointer to begining of first record.
  //
  TextBuf->CurrentRec = TextBuf->FirstRec;
  TextBuf->CurrentByte = 0;
  TextBuf->SubstringLength = 0;
  TextBuf->LineNumber = 1;
  return;  }

//----------------------------------------------SetPointer
void SetPointer(struct Buf *TextBuf, int NewPointer)
  {
  //
  //sets current character pointer of TextBuf
  //
  TextBuf->CurrentByte = NewPointer;
  TextBuf->SubstringLength = 0;
  return;  }

//----------------------------------------------AdvanceRecord
int AdvanceRecord(struct Buf *TextBuf, int Requested)
  {
  //
  //Resets character pointer to begining of record, 'Requested' records
  //  away from the current record. If one end of the text image is
  //  hit then the first/last record is taken.
  //
  //Function returns 0 if it actually moved by Requested records.
  //
  int Actual = 0;
  struct Rec *FirstRecordOfBuffer = TextBuf->FirstRec;
  struct Rec *LastRecordOfBuffer = FirstRecordOfBuffer->prev;
  struct Rec *OriginalRecord = TextBuf->CurrentRec;
  struct Rec *Record = OriginalRecord;
  int Count;
    
  TextBuf->CurrentByte = 0;
  TextBuf->SubstringLength = 0;
  
  if (0 <= Requested)
    for (Count = 1; Count <= Requested; Count++) {//+ve. or zero Requested: forwards loop.
      if (Record == LastRecordOfBuffer)
        break;
      Record = Record->next;
      Actual++; }
  else
    for (Count = 0; Requested < Count; Count--) {// -ve.  backwards loop.
      if (Record == FirstRecordOfBuffer)
        break;
      Record = Record->prev;
      Actual--; }
  
  TextBuf->CurrentRec = Record;
  TextBuf->LineNumber += Actual;
  return (Actual != Requested);
  } //AdvanceRecord procedure ends.

//----------------------------------------------DumpCommand
void DumpCommand(struct Com *ThisCommand, int *LengthPtr, char *Message)
  { // Dumps one command block.
  struct AnyArg *FirstArg = ThisCommand->ArgList;
  struct AnyArg *ThisArg = FirstArg;
  
  *LengthPtr += sprintf(Message+*LengthPtr, "{");
  
  if (ThisCommand->CommandKey == '%') {
    int SubKey = ((struct IntArg *)ThisArg)->IntArg;
    ThisArg = ThisArg->next;
    *LengthPtr += sprintf(Message+*LengthPtr, "%c%c", ThisCommand->CommandKey, SubKey); }
  else if (ThisCommand->CommandKey == 'O') {
    int SubKey = ((struct IntArg *)ThisArg)->IntArg;
    ThisArg = ThisArg->next;
    *LengthPtr += sprintf(Message+*LengthPtr, "%c%c", ThisCommand->CommandKey, SubKey); }
  else
    *LengthPtr += sprintf(Message+*LengthPtr, "%c", ThisCommand->CommandKey);
  
  while (ThisArg != NULL) {
    int Type = ThisArg->type;
    if (Type == IntArgType) {
      long long Value = (long long)(((struct IntArg *)ThisArg)->IntArg);
      *LengthPtr += sprintf(Message+*LengthPtr, "<%lld>", Value);
      if ((' ' <= Value) && (Value <= '~'))
        *LengthPtr += sprintf(Message+*LengthPtr, "=\'%c\'", (int)(Value&0x000000FF)); }
    else if (Type == FloatArgType)
      *LengthPtr += sprintf(Message+*LengthPtr, "=\'%lf\'", (double)(((struct FloatArg *)ThisArg)->FloatArg));
    else if (Type == StringArgType)
      *LengthPtr += sprintf(Message+*LengthPtr, "=\'%s\'", (char *)(((struct PtrArg *)ThisArg)->pointer));
    else if (Type == IndirectStringArgType) {
      int BufferKey = (char)(((struct IntArg *)ThisArg)->IntArg);
      struct Buf * Buffer = GetBuffer(BufferKey, NeverNew);
      *LengthPtr += sprintf(Message+*LengthPtr, "\'%C = \"%s\"", BufferKey, Buffer->CurrentRec->text); }
    else if (Type == BlockArgType) //Make no attempt to descend into blocks - just indicate their presence.
      *LengthPtr += sprintf(Message+*LengthPtr, "...");
    else
      Fail("Unknown Arg type found in DumpCommand() - type = %d", Type);
    ThisArg = ThisArg->next;
    if (ThisArg != NULL)
      strcpy(Message+(*LengthPtr)++, " "); }
      
  *LengthPtr += sprintf(Message+*LengthPtr, "} "); }

//----------------------------------------------FreeCommand
void FreeCommand(struct Com **ThisCommand)
  { // Frees one command block.
  struct AnyArg *ThisArg = (*ThisCommand)->ArgList;
  
  if ((*ThisCommand)->CommandKey == '(')
    FreeBlock((struct Block **)&(((struct PtrArg *)(*ThisCommand)->ArgList)->pointer));
    
  while (ThisArg) { // Arg loop.
    struct AnyArg *Next = ThisArg->next;
    while (ThisArg) {
      struct AnyArg * ContinuationStringArg = NULL;
      if (ThisArg->type == StringArgType) {
        ContinuationStringArg = (struct AnyArg *)((struct StringArg *)ThisArg)->Continuation;
        free(((struct StringArg *)ThisArg)->String); }
      else if (ThisArg->type == IntArgDeferredType)
        free(((struct IntArgDeferred *)ThisArg)->Expr);
      free(ThisArg);
      ThisArg = ContinuationStringArg; }
    ThisArg = Next; }
  
  free(*ThisCommand); 
  *ThisCommand = NULL; }

//----------------------------------------------FreeSequence
void FreeSequence(struct Seq **ThisSequence)
  { // Frees one Sequence.
  struct Com *ThisCom;
  
  if ( ! *ThisSequence)
    return;
  ThisCom = (*ThisSequence)->FirstCommand;
  while (ThisCom) { // Com loop.
    struct Com *Next = ThisCom->NextCommand;
    FreeCommand(&ThisCom);
    ThisCom = Next; }
     
  FreeSequence(&(*ThisSequence)->ElseSequence);
  FreeSequence(&(*ThisSequence)->NextSequence);
  free(*ThisSequence);
  *ThisSequence = NULL; }

//----------------------------------------------FreeBlock
void FreeBlock(struct Block **ThisBlock)
  { // Frees one Block.
  if ( ! *ThisBlock)
    return;
  FreeSequence(&(*ThisBlock)->BlockSequence);
  free(*ThisBlock);
  ThisBlock = NULL; }

//----------------------------------------------GetArg
long long GetArg(struct Buf *TextBuf, long long DefaultValue)
  { // Returns JOT style numeric argument. 
  int Direction = VerifyCharacter(TextBuf,'-') ? -1 : 1;
  if (VerifyDigits(TextBuf)) { // Get the arg.
    DefaultValue = GetDec(TextBuf);
    if (DefaultValue < 0) {
      SynError(TextBuf, "Overflow occured while reading decimal argument");
      return 0; } }
  if (DefaultValue == 0)
    return (Direction == 1) ? INT_MAX : INT_MIN;
  else
    return Direction*DefaultValue;  }
       
//----------------------------------------------GetBreakArg
int GetBreakArg(struct Buf *TextBuf, int DefaultValue)
  { //Similar to GetArg but must be positive and non-zero.
  if (VerifyDigits(TextBuf)) {
    long long Value = GetDec(TextBuf);
    if (Value <= 0)
      SynError(TextBuf, "Invalid break count");
    else
      return Value; }
  return DefaultValue;  }

//---------------------------------------------AddIntArg
void AddIntArg(struct Com *CommandBlock, long long ArgValue)
  { //Adds integer argument to current command.
  struct IntArg *NewArg = (struct IntArg *)JotMalloc(sizeof(struct IntArg));
  struct AnyArg *ThisArg = CommandBlock->ArgList;
   
  NewArg->type = IntArgType;
  NewArg->IntArg = ArgValue;
  NewArg->next = NULL;
  if (ThisArg == NULL)
    CommandBlock->ArgList = (struct AnyArg *)NewArg;
  else
    while (TRUE)
      if ((ThisArg->next) == NULL) { // Found last Arg.
        ThisArg->next = (struct AnyArg *)NewArg;
        return; }
      else {
        ThisArg = ThisArg->next; } }

//---------------------------------------------AddIntArgFromText
int AddIntArgFromText(struct Com *CommandBlock, struct Buf * CommandBuf, int Radix, char *ValidTerminators)
  { //Adds integer argument to current command, allows indirect expressions e.g: "'$" or "'.=fred|jim|chrs".
    //The CommandBuf should point to the start of the arg expression in the source code.
    //The ValidTerminators string is a list of characters that are allowed to terminate the expression.
    //Returns no.  command-string characters swallowed.
  struct AnyArg *NewArg = NULL, *ThisArg = CommandBlock->ArgList;
  long long Value;
  int LeadingBlanks = 0, Length = 0;
  char *CommandString = CommandBuf->CurrentRec->text + CommandBuf->CurrentByte;
   
  while (CommandString[0] == ' ')
    CommandString += ++LeadingBlanks;
  if (CommandString[0] == '\'') { //The integer value is dependant on some indirect expression.
    char *EndStop = strchr(CommandString, '\0'), Ch;
    int i = 0;
    
    while ( (Ch = ValidTerminators[i++]) ) { //Terminator search - take the leftmost valid terminator.
      char *Terminator = strchr(CommandString, Ch);
      if (Terminator && Terminator < EndStop)
        EndStop = Terminator; }
    
    Length = EndStop-CommandString;
    NewArg = (struct AnyArg *)JotMalloc(sizeof(struct IntArgDeferred));
    NewArg->type = IntArgDeferredType;
    ((struct IntArgDeferred *)NewArg)->Radix = Radix;
    ((struct IntArgDeferred *)NewArg)->Expr = JotMalloc(Length+1);
    memcpy(((struct IntArgDeferred *)NewArg)->Expr, CommandString, Length);
    ((struct IntArgDeferred *)NewArg)->Expr[Length] = '\0';
    Length += LeadingBlanks;
    NewArg->next = NULL; }
  else { //The integer value was fully specified in the source.
    sscanf(CommandString, Radix == 10 ? "%lld%n" : "%llx%n", &Value, &Length);
    if (Length) {
      Length += LeadingBlanks;
      NewArg = (struct AnyArg *)JotMalloc(sizeof(struct IntArg));
      NewArg->type = IntArgType;
      ((struct IntArg *)NewArg)->IntArg = Value;
      NewArg->next = NULL; } }
  if ( ! NewArg)
    return 0;
    
  if (ThisArg == NULL)
    CommandBlock->ArgList = (struct AnyArg *)NewArg;
  else
    while (TRUE)
      if ((ThisArg->next) == NULL) { // Found last Arg.
        ThisArg->next = (struct AnyArg *)NewArg;
        break; }
      else {
        ThisArg = ThisArg->next; }
  CommandBuf->CurrentByte += Length;
  return Length; }

//---------------------------------------------AddStringArgPercent
void AddStringArgPercent(struct Com *CommandBlock, struct Buf * CommandBuf, int SemicolonTerminator)
  { //Adds string argument to current command line, allows indirect expressions e.g: "'$".
    //The CommandString should point to the start of the arg expression in the source code.
    //Copies up to the terminating semicolon or the end of the string.
    //Semicolon indicates the full extent of the source string - either the end of line or an unescaped semicolon.
    //Ampersand is similar.
    //When SemicolonTerminator is TRUE it extracts to the first unescaped semicolon, otherwise only up to the end of the first unescaped blank.
  struct StringArg *NewArg = NULL;
  struct AnyArg *ThisArg = CommandBlock->ArgList;
  char *CommandString =       //The complete string.
    CommandBuf->CurrentRec->text + CommandBuf->CurrentByte;
  char *CommandEnd;           //The end of the CommandString.
  char *ExpressionEnd;        //The end of complete expression (identical to CommandString if SemicolonTerminator).
  int LeadingBlanks = 0;      //The number of leading blanks at the start of the first section.
  int OutputOffset = 0;       //The next character position in the output string.
  char *Ampersand = NULL;     //The position of the next ampersand in the current substring or NULL.
  char *Backslash;            //The position of the next backslash in the current substring or NULL.
  char *SubstringStart;       //The start point of the current substring.
  char *SubstringEnd;         //Either the substring-terminating ampersand or the end of the command string.
  
  NewArg = (struct StringArg *)JotMalloc(sizeof(struct StringArg));
  NewArg->type = StringArgType;
  NewArg->Continuation = NULL;
  if (ThisArg == NULL)
    CommandBlock->ArgList = (struct AnyArg *)NewArg;
  else
    while (TRUE)
      if ((ThisArg->next) == NULL) { // Found last Arg.
        ThisArg->next = (struct AnyArg *)NewArg;
        break; }
      else {
        ThisArg = ThisArg->next; }
  
  SubstringStart = CommandString;
  char *SearchStart, *SearchEnd = CommandString+strlen(CommandString);
  while (CommandString[LeadingBlanks] == ' ')
    LeadingBlanks += 1;
  SearchStart = CommandString+LeadingBlanks;
  
  while ( (CommandEnd = ExpressionEnd = memchr(SearchStart, ';', SearchEnd-SearchStart)) ) { //Loop searches for an unescaped semicolon.
    if (CommandEnd[-1] != '\\')
      break;
    else { //Count the Backslashes.
      Backslash = CommandEnd-1;
      while ( (CommandString < Backslash) && ((--Backslash)[0] == '\\') )
        { }
      if ( (CommandEnd-Backslash-1)%2 == 0 ) //An even number of backslashes - this semicolon terminates the command.
        break;
      else
        SearchStart = CommandEnd+1; } }
    
  if ( ! CommandEnd) //There are no unescaped terminators in the command string.
     CommandEnd = ExpressionEnd = CommandString+strlen(CommandString);
  
  if ( ! SemicolonTerminator) {
    SearchStart = CommandString+LeadingBlanks;
    while ( (ExpressionEnd = memchr(SearchStart, ' ', CommandEnd-SearchStart)) ) { //Loop searches for an unescaped blank.
      if (ExpressionEnd[-1] != '\\')
        break;
      else { //Count the Backslashes.
        Backslash = ExpressionEnd-1;
        while ( (CommandString < Backslash) && ((--Backslash)[0] == '\\') )
          { }
        if ( (ExpressionEnd-Backslash-1)%2 == 0 ) //An even number of backslashes - this blank terminates the expression.
          break;
        else
          SearchStart = ExpressionEnd+1; } }
    
    if ( ! ExpressionEnd) //There are no unescaped terminators in the expression.
       ExpressionEnd = CommandEnd; }
      
  CommandBuf->CurrentByte += ExpressionEnd-CommandString;
  SubstringStart = CommandString+LeadingBlanks;
    
  while ( TRUE ) { //Arg continuation loop - each ampersand-separated substring is copied to a new continuation string arg.
    struct StringArg * OldArg = NewArg;
    char * SearchStart = SubstringStart;
    SubstringEnd = ExpressionEnd;
    while ( (Ampersand = memchr(SearchStart, '&', ExpressionEnd-SearchStart)) ) { //Loop searches for a substring-terminating ampersand.
      if (Ampersand[-1] != '\\') {
        SubstringEnd = Ampersand;
        break; }
      else { //Count the Backslashes.
        Backslash = Ampersand-1;
        while ( (CommandString < Backslash) && ((--Backslash)[0] == '\\') )
          { }
        if ( (Ampersand-Backslash-1)%2 == 0 ) { //An even number of backslashes - this ampersand terminates the command.
          SubstringEnd = Ampersand;
          break; }
        else
          SearchStart = Ampersand+1; } }
      
    NewArg->String = (char *)JotMalloc(SubstringEnd-SubstringStart+1);
    memset(NewArg->String, '\0', SubstringEnd-SubstringStart+1);
    
    //Copy string up to next Backslash then restart at the character following the Backslash.
    OutputOffset = 0;
    while ( TRUE ) {
      Backslash = memchr(SubstringStart, '\\', SubstringEnd-SubstringStart);
      if (Backslash) {
        memcpy(NewArg->String+OutputOffset, SubstringStart, Backslash-SubstringStart);
        OutputOffset += Backslash-SubstringStart;
        if ( ! Backslash[1])
          break;
        NewArg->String[OutputOffset++] = Backslash[1];
        if ( ExpressionEnd <= (SubstringStart = Backslash+2) )
          break; }
      else {
        memcpy(NewArg->String+OutputOffset, SubstringStart, SubstringEnd-SubstringStart);
        OutputOffset += SubstringEnd-SubstringStart;
        NewArg->String[OutputOffset] ='\0';
        break; } }
        
    NewArg->StringLength = OutputOffset;
    NewArg->next = NULL;
    if ( ! Ampersand)
      break;
    SubstringStart = SubstringEnd+1;
    NewArg = (struct StringArg *)JotMalloc(sizeof(struct StringArg));
    NewArg->type = StringArgType;
    NewArg->Continuation = NULL;
    OldArg->Continuation = NewArg; }
  CommandString = CommandBuf->CurrentRec->text;
      
  return; }

//---------------------------------------------AddFloatArg
void AddFloatArg(struct Com *CommandBlock, double ArgValue)
  { //Adds floating-point argument to current command.
  struct FloatArg *NewArg = (struct FloatArg *)JotMalloc(sizeof(struct FloatArg));
  struct AnyArg *ThisArg = CommandBlock->ArgList;
   
  NewArg->type = FloatArgType;
  NewArg->FloatArg = ArgValue;
  NewArg->next = NULL;
  if (ThisArg == NULL)
    CommandBlock->ArgList = (struct AnyArg *)NewArg;
  else
    while (TRUE)
      if ((ThisArg->next) == NULL) { // Found last Arg.
        ThisArg->next = (struct AnyArg *)NewArg;
        return; }
      else
        ThisArg = ThisArg->next;  }

//---------------------------------------------AddBlock
void AddBlock(struct Com *CommandBlock, struct Block *Block)
  { //Adds sub-block argument to current command.
  struct PtrArg *NewArg = (struct PtrArg *)JotMalloc(sizeof(struct PtrArg));
  struct AnyArg *ThisArg = CommandBlock->ArgList;
   
  NewArg->type = BlockArgType;
  NewArg->pointer = Block;
  NewArg->next = NULL;
  if (ThisArg == NULL)
    CommandBlock->ArgList = (struct AnyArg *)NewArg;
  else
    while (TRUE)
      if ((ThisArg->next) == NULL) { // Found last Arg.
        ThisArg->next = (struct AnyArg *)NewArg;
        return; }
      else
        ThisArg = ThisArg->next;  }

//---------------------------------------------AddStringArg
void AddStringArg(struct Com *CommandBlock, struct Buf *CommandBuf)
  { //
    //Takes next nonblank character in CommandBuf as a delimeter,
    //and copies over all text to OutputString until 2nd. delimeter, or end of
    //text encountered.
    //
    //Since these string arguments are later copied into strings of StringMaxChr
    //the length is checked here.
    //
  int OutputIndex = 0;
  int DelimiterCharacter;
  char *InputText = CommandBuf->CurrentRec->text;
  char ResultString[StringMaxChr];
  int InputIndex;
  struct AnyArg *NewArg;
  struct AnyArg *ThisArg = CommandBlock->ArgList;
  int Type = StringArgType;
  char *FinalArgValue;
  int Length;
  char BufferKey;
  
  if (VerifyCharacter(CommandBuf, '\'')) { // Deferred argument just save the buffer key.
    BufferKey = ChrUpper(NextNonBlank(CommandBuf));
    if ( ! CheckBufferKey(BufferKey))
      SynError(CommandBuf, "Invalid buffer key");
    Type = IndirectStringArgType; }
  else { // Extract string argument now.
    DelimiterCharacter = ImmediateCharacter(CommandBuf);
    if (DelimiterCharacter == '\0')
      ResultString[0] = '\0';
    if ( (('A' <= DelimiterCharacter) && (DelimiterCharacter <= 'Z')) || (('a' <= DelimiterCharacter) && (DelimiterCharacter <= 'z')) || 
        (('0' <= DelimiterCharacter) && (DelimiterCharacter <= '9')) ) {
      SynError(CommandBuf, "Invalid delimiter");
      return; }
  
    for (InputIndex = CommandBuf->CurrentByte; InputIndex < strlen(InputText); InputIndex++) { // Copy-over-text loop.
      char Chr = InputText[InputIndex];
      if (StringMaxChr <= OutputIndex) {
        SynError(CommandBuf, "Argument string too long");
        return; }
      if (Chr == DelimiterCharacter)
        break;
      ResultString[OutputIndex++] = Chr; }
    
    ResultString[OutputIndex] = '\0';
    CommandBuf->CurrentByte = InputIndex+1;
    
    Length = strlen(ResultString);
    
    if ( (Length == 0) || (ResultString == NULL) )
      FinalArgValue = NULL;
    else { // Non-null string.
      FinalArgValue = (char *)JotMalloc(Length+1);
      strcpy(FinalArgValue, ResultString); } }
    
  if (Type == IndirectStringArgType) {
    NewArg = (struct AnyArg *)JotMalloc(sizeof(struct IntArg));
    NewArg->type = Type;
    ((struct IntArg *)NewArg)->IntArg = BufferKey; }
  else  {
    NewArg = (struct AnyArg *)JotMalloc(sizeof(struct StringArg));
    NewArg->type = Type;
    ((struct StringArg *)NewArg)->Continuation = NULL;
    ((struct StringArg *)NewArg)->String = FinalArgValue; }
  NewArg->next = NULL;
  if (ThisArg == NULL)
    CommandBlock->ArgList = (struct AnyArg *)NewArg;
  else
    while (TRUE)
      if ((ThisArg->next) == NULL) { // Found last Arg.
        ThisArg->next = (struct AnyArg *)NewArg;
        return; }
      else
        ThisArg = ThisArg->next;  }

//---------------------------------------------FetchIntArgFunc
int FetchIntArgFunc(struct AnyArg **CurrentArgs_a)
  { //Extracts integer arg value and indexes to next.
  long long Value = -1;
   
  FetchIntArg(&Value, (struct AnyArg **)CurrentArgs_a);
  return (int)Value;  }

//---------------------------------------------FetchLLIntArgFunc
long long FetchLLIntArgFunc(struct AnyArg **CurrentArgs_a)
  { //Extracts integer arg value and indexes to next.
  long long Value;
   
  FetchIntArg(&Value, (struct AnyArg **)CurrentArgs_a);
  return Value;  }

//---------------------------------------------FetchHexArgFunc
int FetchHexArgFunc(struct AnyArg **CurrentArgs_a)
  { //Extracts integer arg value and indexes to next.
  long long Value;
   
  FetchIntArg(&Value, (struct AnyArg **)CurrentArgs_a);
  return (int)Value;  }

//---------------------------------------------FetchLLHexArgFunc
long long FetchLLHexArgFunc(struct AnyArg **CurrentArgs_a)
  { //Extracts integer arg value and indexes to next.
  long long Value;
   
  FetchIntArg(&Value, (struct AnyArg **)CurrentArgs_a);
  return Value;  }

//---------------------------------------------FetchIntArg
int FetchIntArg(long long *Value, struct AnyArg **CurrentArgs_a)
  { //Extracts integer arg value and indexes to next.
  struct AnyArg *ThisArg = *CurrentArgs_a;
   
  if (*CurrentArgs_a == NULL) {
    RunError("Insufficient args in list.");
    *Value = -1;
    return 0; }
    
  *CurrentArgs_a = (*CurrentArgs_a)->next;
  
  if (ThisArg->type == IntArgType)
    *Value = ((struct IntArg *)ThisArg)->IntArg;
  else if (ThisArg->type == IntArgDeferredType) {
    return FetchIntArgDeferred(Value,  ((struct IntArgDeferred *)ThisArg)->Radix, ((struct IntArgDeferred *)ThisArg)->Expr); }
  else {
    RunError("Expecting an integer argument, found some other type.");
    return 1; }
  
  return 0;  }

//---------------------------------------------FetchIntArgDeferred
int FetchIntArgDeferred(long long *Value, int Radix, char * Expression)
  { //Used at run time to pick up the values of integer args deferred by referencing a buffer.
  char *CurrentText = Expression;
  int LocalFail = FALSE;
  struct intFrame *FinalFrame;
  
  while (CurrentText[0] == ' ')
    CurrentText++;
  if (CurrentText[0] == '\'') { //An indirect expression.
    struct DataHTabObj *DataHTabObj;
    struct intFrame * Frame;
    if (CurrentText[2] == '=') { //A pathname expression.
      struct Buf * ThisBuf;
      char * LastPathElem;
      if ( ! (ThisBuf = QueryPath(CurrentText+1, s_CurrentBuf, &LastPathElem, &FinalFrame, NULL)) )
        return 1;
      if ( ! (DataHTabObj = (struct DataHTabObj *)QueryData(LastPathElem, ThisBuf)) )
        return 1;
      if ( ! (Frame = DataHTabObj->Frame))
        return Fail("Hash-table entry has been defined but data-value not yet assigned.");
      if (Frame->type == FrameType_Int)
        *Value = Frame->Value;
      else if (Frame->type == FrameType_Buf) { //The buffer had better have something that looks like an integer at it's CurrentByte.
        struct Buf *IndirectBuf = ((struct bufFrame *)Frame)->BufPtr;
        if (sscanf(IndirectBuf->CurrentRec->text+IndirectBuf->CurrentByte, Radix == 10 ? "%lld" : "%llx", Value))
          return 0;
        else {
          char ParentPath[StringMaxChr];
          GetBufferPath(IndirectBuf, ParentPath, StringMaxChr);
          LocalFail = Fail("Integer conversion failed, from buffer %s", ParentPath); } } }
    else if (CurrentText[1] == '~') { //A reference to the stack.
      int FrameType;
      if (s_StackPtr < 1)
        return Fail("Stack underflow.");
      FrameType = (&s_Stack[s_StackPtr-1])->type;
      if (FrameType == FrameType_Int) { //Easy case - it's an integer.
        *Value = PopInt(&LocalFail);
        return LocalFail; }
      else if (FrameType == FrameType_Buf) {
        struct Buf *IndirectBuf;
        if (s_StackPtr < 1)
          return Fail("Stack underflow");
        Frame = &s_Stack[s_StackPtr-1];
        IndirectBuf = ((struct bufFrame*)Frame)->BufPtr;
        if (sscanf(IndirectBuf->CurrentRec->text+IndirectBuf->CurrentByte, Radix == 10 ? "%lld" : "%llx", Value)) {
          KillTop();
          return 0; }
        else { //A sprintf conversion error.
          char ParentPath[StringMaxChr];
          GetBufferPath(IndirectBuf, ParentPath, StringMaxChr);
          LocalFail = Fail("Integer conversion failed, from buffer %s", ParentPath); } } }
      else 
        return Fail("Incorrect stack-frame type (maybe float?) found while attempting to fetch an integer."); }
    else { //A reference to some other buffer.
      struct Buf *IndirectBuf = GetBuffer(CurrentText[1], NeverNew);
      if (sscanf(IndirectBuf->CurrentRec->text+IndirectBuf->CurrentByte, Radix == 10 ? "%lld" : "%llx", Value))
        return 0;
      else {
        char ParentPath[StringMaxChr];
        GetBufferPath(IndirectBuf, ParentPath, StringMaxChr);
        LocalFail = Fail("Integer conversion failed, from buffer %s", ParentPath); } }
  return LocalFail; }

//---------------------------------------------FetchStringArgPercent
int FetchStringArgPercent(char *DestString, int *LengthPtr, struct AnyArg **CurrentArgs_a)
  { //Extracts string arg pointer and indexes to next arg, NoExpand prevents expansion of indirect references.
    //The  destination buffer DestString, is assumed to have a total capacity of StringMaxChr.
  struct AnyArg *ThisArg = *CurrentArgs_a;
  int LocalFail = 0;
  int Length;
  int OutputIndex = 0;
   
  
  if ( ! LengthPtr)
    LengthPtr = &Length;
  *LengthPtr = 0;
  
  if (*CurrentArgs_a == NULL) {
    RunError("Insufficient args in list.");
    return 0; }
    
  *CurrentArgs_a = (*CurrentArgs_a)->next;
  DestString[0] = '\0';
      
  if (ThisArg->type != StringArgType) {
    RunError("Expecting a string argument, found some other type.");
    return 1; }
  while (ThisArg) {
    char *OrigString = ((struct StringArg *)ThisArg)->String;
    char *StartPoint = OrigString;
    char Temp[StringMaxChr], *Result = Temp;
    int PopTheStack = FALSE;
    
    Length = ((struct StringArg *)ThisArg)->StringLength;
    if (StringMaxChr <= Length) {
      DestString[0] = '\0';
      LocalFail = Fail("Overlong string truncated"); }
    if (Length) {
//      if (OrigString[0] == '\0')
//        break;
      if (OrigString[0] == '\\')
        Result = StartPoint+1;
      else if (OrigString[0] == '\'') { //It's some sort of indirect reference.
        char BufferKey = toupper(OrigString[1]);
        if (OrigString[1] == '~' || (OrigString[1] != '\0' && OrigString[2] == '=') ) { //It's either a stack or pathname-expression reference.
          struct DataHTabObj *DataHTabObj;
          struct intFrame * FrameObj;
          if (OrigString[2] == '=') { //It's a pathname expression.
            struct Buf * ThisBuf;
            char * LastPathElem;
            if ( ! (ThisBuf = QueryPath(OrigString+1, s_CurrentBuf, &LastPathElem, NULL, NULL)) ) {
              LocalFail = 1;
              break; }
            if ( ! (DataHTabObj = (struct DataHTabObj *)QueryData(LastPathElem, ThisBuf)) ) {
              LocalFail = 1;
              break; }
            if ( ! (FrameObj = DataHTabObj->Frame) ) {
              LocalFail = Fail("Hash-table entry has been defined but data-value not yet assigned.");
              break; } }
            else {
              if (s_StackPtr < 1) {
                Fail("Stack underflow");
                break; }
              FrameObj = &s_Stack[s_StackPtr-1];
              PopTheStack = TRUE; }
          if (FrameObj->type == FrameType_Int) //An integer value.
            Length = sprintf(Temp, "%lld", FrameObj->Value);
          else if (FrameObj->type == FrameType_Float) //An real value.
            Length = sprintf(Temp, "%e", ((struct floatFrame *)FrameObj)->fValue);
          else if (FrameObj->type == FrameType_Buf) { //A buffer.
            struct Buf * StackBuf = ((struct bufFrame *)FrameObj)->BufPtr;
            Result = StackBuf->CurrentRec->text;
            Length = strlen(Result); } }
          else { //It's a simple buffer reference.
            struct Buf * ThisBuf = GetBuffer(BufferKey, NeverNew);
            if ( ! ThisBuf || ! ThisBuf->CurrentRec ) {
              LocalFail = Fail("Attempt to reference an undefined buffer.");
              break; }
            Result = ThisBuf->CurrentRec->text;
            Length = strlen(Result); } }
      else if (OrigString[0] == '^') { //It's a control character.
        Result[0] = OrigString[1] - 0x40;
        Length = 1; }
      else //A literal string.
        Result = StartPoint; }
    if ( StringMaxChr < OutputIndex+Length ) {
      Fail("Overlong string");
      return TRUE; }
    memcpy(DestString+OutputIndex, Result, Length);
    OutputIndex += Length;
    DestString[OutputIndex] = '\0';
    ThisArg = (struct AnyArg *)((struct StringArg *)ThisArg)->Continuation;
    if (LengthPtr)
      *LengthPtr += Length;
    if (PopTheStack)
      KillTop(); }
  
  return LocalFail;  }

//---------------------------------------------FetchFloatArg
double FetchFloatArg(struct FloatArg **CurrentArgs_a)
  { //Extracts floating-point arg value and indexes to next.
  double Value;
   
  if (*CurrentArgs_a == NULL) {
    RunError("Insufficient args in list.");
    return 0; }
  if ((*CurrentArgs_a)->type != FloatArgType)
    RunError("Expecting a floating-point argument, found some other type.");
  Value = (*CurrentArgs_a)->FloatArg;
  *CurrentArgs_a = (struct FloatArg *)(*CurrentArgs_a)->next;
  return Value;  }

//---------------------------------------------FetchBlock
struct Block * FetchBlock(struct PtrArg **CurrentArgs_a)
  { //Extracts arg value and indexes to next.
  struct Block *Value;
   
  if (*CurrentArgs_a == NULL) {
    RunError("Insufficient args in list.");
    return NULL; }
  if ((*CurrentArgs_a)->type != BlockArgType) {
    RunError("Expecting a block argument, found some other type.");
    return NULL; }
  Value = (struct Block *)(*CurrentArgs_a)->pointer;
  *CurrentArgs_a = (struct PtrArg *)(*CurrentArgs_a)->next;
  return Value;  }

//---------------------------------------------FetchStringArg
char *FetchStringArg(struct PtrArg **CurrentArgs_a, char *DefaultString)
  {
  //Extracts string arg value and indexes to next.
  //If the type is IndirectStringArgType then returns pointer to text,
  //if no arg, returns DefaultString unchanged, if DefaultString is NULL, returns ""
  //other args are copied into the DefaultString, returning the default string.
  //
  char *Value = 0;
  
  if (*CurrentArgs_a == NULL) {
    RunError("Insufficient args in list.");
    return ""; }
    
  if (((*CurrentArgs_a)->type) == IndirectStringArgType) { // Indirection via buffer.
    char Key = ((struct IntArg *)(*CurrentArgs_a))->IntArg;
    struct Buf *ArgStringBuf;
    
    if (Key == '~') { //It's an indirect reference to the stack, but there's not a buffer at the top - just do the obvious decimal conversion.
      int Type;
      
      if (s_StackPtr < 1) {
        Fail("Stack underflow");
        *CurrentArgs_a = (struct PtrArg *)(*CurrentArgs_a)->next;
        return ""; }
      Type = s_Stack[s_StackPtr-1].type;
      if (Type == FrameType_Buf)
        ArgStringBuf = ((struct bufFrame *)(&s_Stack[s_StackPtr-1]))->BufPtr;
      else {
        if (Type == FrameType_Int)
          sprintf(s_ArgString, "%lld", (long long)s_Stack[s_StackPtr-1].Value);
        else
          sprintf(s_ArgString, "%f", ((struct floatFrame *)(&s_Stack[s_StackPtr-1]))->fValue);
        KillTop();
        *CurrentArgs_a = (struct PtrArg *)(*CurrentArgs_a)->next;
        return s_ArgString; } }
    else
    ArgStringBuf = GetBuffer((int)Key, NeverNew);
       
    if (ArgStringBuf && ArgStringBuf->CurrentRec)
      Value = ArgStringBuf->CurrentRec->text;
    else {
      Fail("Undefined buffer %c", Key);
      Value = ""; } }
       
  else if ((*CurrentArgs_a)->type != StringArgType) {
    RunError("Expecting a string argument, found some other type.");
    return ""; }
     
  else { // A literal string was specified.
    Value = (char *)(*CurrentArgs_a)->pointer;
    if (Value == NULL) {
      *CurrentArgs_a = (struct PtrArg *)(*CurrentArgs_a)->next;
      return DefaultString ? DefaultString : ""; }
    else if (DefaultString) {
      strncpy(DefaultString, Value, StringMaxChr);
      DefaultString[StringMaxChr-1] = '\0'; } }
       
  *CurrentArgs_a = (struct PtrArg *)(*CurrentArgs_a)->next;
  return Value; }

//-----------------------------------------------CompareRecs
int  CompareRecs(struct Rec *X, struct Rec *Y, int ColumnStarts[], int ColumnWidths[])
  { //Compares two records - used by quicksort
    //ColumnStarts is a list of start characters of columns to be compared in order, terminated by -1.
    //ColumnWidths is the corresponding list of column widths.
  int i = 0, Result;
  
  while (TRUE) {
    int ColumnStart=ColumnStarts[i], ColumnWidth=ColumnWidths[i++];
    if (ColumnStart < 0)
      return strcmp(X->text, Y->text);
    else {
      Result = memcmp(X->text+ColumnStart, Y->text+ColumnStart, ColumnWidth+1);
      if (Result != 0)
        break; } }
  return Result;  }

//-----------------------------------------------CompareRecsTab
int CompareRecsTab(struct Rec *X, struct Rec *Y, int TabNos[])
  { //Compares two records - used by quicksort, TabNos is a list of tab-delimited columns in priority order and terminated by -1
  int i = 0, Result;
  char TempX[StringMaxChr], TempY[StringMaxChr];
  
  while (TRUE) {
    int TabNo = TabNos[i++];
    if (TabNo < 0)
      return strcmp(X->text, Y->text);
    else {
      char *ColX = X->text;
      char *ColY = Y->text;
      char *nxTabX, *nxTabY;
      
      for ( ; 0 < TabNo; TabNo--) {
        ColX = strchr(ColX, s_TableSeparator);
        ColY = strchr(ColY, s_TableSeparator);
        if ( ! ColX++ || ! ColY++) 
          return strcmp(X->text, Y->text); }
           
      if ( (nxTabX = strchr(ColX, s_TableSeparator) )) {
        strncpy(TempX, ColX, nxTabX-ColX);
        TempX[nxTabX-ColX] = '\0'; }
      else
        strcpy(TempX, ColX);
      if ( (nxTabY = strchr(ColY, s_TableSeparator) )) {
        strncpy(TempY, ColY, nxTabY-ColY);
        TempY[nxTabY-ColY] = '\0'; }
      else
        strcpy(TempY, ColY);
      if ( (Result = strcmp(TempX, TempY)) )
        break; } }
    return Result; }

//-----------------------------------------------Swap
void  Swap(struct Rec *v[], int i, int j)
  { //Element swapper - used by quicksort only.
  struct Rec *temp;
  
  temp = v[i];
  v[i] = v[j];
  v[j] = temp; }

//-----------------------------------------------Quicksort
void Quicksort(struct Rec *v[], int left, int right, int (*comp)(struct Rec *, struct Rec *, int *, int *), int ColumnStarts[], int ColumnWidths[])
  { //The classic quicksort algorithm adapted for sorting JOT records
    //This version sorts fields delimited by <firstCol>-<lastCol>.
  int i, last;
  
  if (right <= left)
    return;
  Swap(v, left, (left + right) / 2);
  last = left;
  
  for (i = left + 1; i <= right; ++i) {
    if ((*comp)(v[i], v[left], ColumnStarts, ColumnWidths) < 0)
      Swap(v, ++last, i); }
  
  Swap(v, left, last);
  Quicksort(v, left,     last - 1, comp, ColumnStarts, ColumnWidths);
  Quicksort(v, last + 1, right,    comp, ColumnStarts, ColumnWidths); }

//-----------------------------------------------QuicksortTab
void QuicksortTab(struct Rec *v[], int left, int right, int (*comp)(struct Rec *, struct Rec *, int []), int TabNos[])
  { //The classic quicksort algorithm adapted for sorting JOT records
    //This version sorts by field numbers marked by tabs.
  int i, last;
  
  if (right <= left)
    return;
  Swap(v, left, (left + right) / 2);
  last = left;
  
  for (i = left + 1; i <= right; ++i) {
    if ((*comp)(v[i], v[left], TabNos) < 0)
      Swap(v, ++last, i); }
  
  Swap(v, left, last);
  QuicksortTab(v, left,     last - 1, comp, TabNos);
  QuicksortTab(v, last + 1, right,    comp, TabNos); }

//-----------------------------------------------DoSort
int DoSort(int TabSort, struct AnyArg **ThisArg)
  { //Analyzes command line given to %S and launches the sort.
  //First set up the buffer's records in an array for sorting. 
  int CompareRecsTab(struct Rec *, struct Rec *, int []);
  int CompareRecs(struct Rec *, struct Rec *, int [], int []);
  int ColumnStarts[100];
  int ColumnWidths[100];
  struct Rec *Record = s_CurrentBuf->FirstRec;
  struct Rec *LastRec = s_CurrentBuf->FirstRec->prev;
  struct Rec **Array;
  int Length = 0;
  int i, Col=0;
  struct Rec *PrevRec;
  int Relinks;
  
  if (s_CurrentBuf->EditLock & ReadOnly) {
    RunError("Attempt to modify a readonly buffer");
    return 1; }
  while (*ThisArg) {
    int ColStart = FetchIntArgFunc(ThisArg);
    ColumnStarts[Col] = ColStart;
    if ( ! TabSort) {
      int Delim = FetchIntArgFunc(ThisArg);
      int EndArg = FetchIntArgFunc(ThisArg);
      ColumnWidths[Col] = (Delim == B_sortPlusQual) ? EndArg : EndArg-ColStart; }
    Col += 1; }
  ColumnStarts[Col] = -1;
  
  //Get array length.
  for (; ; ) { 
    Length++;
    if (Record == LastRec)
      break; 
    Record = Record->next; }
    
  //Create and populate the array with Rec structs from the current buffer.
  Array = (struct Rec **)JotMalloc(sizeof(struct Rec **)*(Length+1));
  Record = s_CurrentBuf->FirstRec;
  for (i = 0; i < Length; i++) { //Copy-records-to-array loop.
    Array[i] = Record;
    Record = Record->next; }
    
  if (TabSort) {
    QuicksortTab(Array, 0, Length-1, CompareRecsTab, ColumnStarts); }
  else {
    Quicksort(Array, 0, Length-1, CompareRecs, ColumnStarts, ColumnWidths); }
   
  //After sorting the array, reorder buffer to match the array.
  PrevRec = NULL;
  Relinks = 0;    //Counts number of relinked records.
   
  Record = (struct Rec *)Array[0];
  s_CurrentBuf->FirstRec = Record;
  s_CurrentBuf->CurrentRec = Record;
  
  for (i = 0; i < Length; i++) {
    Record = Array[i];
    if (Record->prev != PrevRec)
      Relinks++;
    Record->prev = PrevRec;
    if (PrevRec)
      PrevRec->next = Record;
    PrevRec = Record; }
       
  Record->next = s_CurrentBuf->FirstRec;
  s_CurrentBuf->FirstRec->prev = Record;
  s_CurrentBuf->CurrentByte = 0;
  s_CurrentBuf->SubstringLength = 0;
  s_CurrentBuf->LineNumber = 1;
  free(Array);
  return 0; }

//-----------------------------------------------AddFormattedRecord
int AddFormattedRecord(int AddToJournal, struct Buf *Buffer, char *String, ...)
  { //Appends a new record to buffer using specified format string and values, retuns no. of characters.
  va_list ap;
  char *TempString, RealString[StringMaxChr];
  int Len;
   
  TempString = RealString;
  TempString[0] = '\0';
  va_start(ap, String);
Len = vsnprintf(TempString, StringMaxChr, String, ap);
//  if ( (Len = vsnprintf(TempString, StringMaxChr-1, String, ap))==StringMaxChr-1 )
//    TempString[StringMaxChr-1] = '\0';
  va_end(ap); 
  if (AddToJournal && s_JournalPath) {
    if (s_RecoveryMode) { //Replace given text with a line read from the recovery file.
      fgets(RealString, StringMaxChr, s_asConsole->FileHandle);
      s_asConsole->LineNo++;
      if (strstr(RealString, "%%Recovery record: ") == RealString) {
        TempString = RealString+19;
        Len = strlen(TempString);
        if (TempString[Len-1] != '\0')
          TempString[Len-1] = '\0'; }
      else if (strstr(RealString, "%%Recovery open-ended report ends") == RealString)
        return 0;
      else
        Disaster("Wrong record type at line %d in recovery script, should be \"%%%%Recovery record: ...\" - read: \"%s\"",   s_asConsole->LineNo, RealString); }
    else { //Using given text.
      char Buf[StringMaxChr];
      sprintf(Buf, "%%%%Recovery record: %s", TempString);
      if (s_JournalHand)
        UpdateJournal(Buf, NULL); } }
  TempString[StringMaxChr-1] = '\0';
  GetRecord(Buffer, Len+1, TempString);
  return strlen(TempString); }

//-----------------------------------------------GetBufferPath
int GetBufferPath(struct Buf *ThisBuf, char * ParentPath, int MaxSize) {
  //Sets ParentPath to string giving the correct hashtable hierarchy path for the nominated buffer.
  struct PathElem {
  struct Buf * Buf;
  struct PathElem * Next; };
  struct PathElem * ParentElem = NULL;
  int Len = 2;
    
  while (ThisBuf->ParentObj) {
    struct PathElem *ThisPathElem = (struct PathElem *)JotMalloc(sizeof(struct PathElem));
    ThisPathElem->Buf = ThisBuf;
    ThisPathElem->Next = ParentElem;
    ThisBuf = ThisBuf->ParentObj->ParentBuf;
    ParentElem = ThisPathElem; }
    
  ParentPath[0] = ThisBuf->BufferKey;
  if ( ! ParentElem)
    ParentPath[1] = '\0';
  else {
    ParentPath[1] = '=';
    ParentPath[2] = '\0';
    while (ParentElem) {
      struct PathElem * NextPathElem = ParentElem->Next;
      int SubLen = strlen(ParentElem->Buf->ParentObj->HashKey)+1;
      if (Len+SubLen < MaxSize) {
        strcpy(ParentPath+Len, ParentElem->Buf->ParentObj->HashKey);
        if (NextPathElem)
          strcpy(ParentPath+Len+1, "|");
        Len += SubLen; }
      else
        strcpy(ParentPath+Len-4, "...");
      free(ParentElem);
      ParentElem = NextPathElem; } }
    
  return Len < MaxSize; }

//-----------------------------------------------BinCmp
int BinCmp(char *String, char *x, int n) {
  //Returns TRUE if first n bytes of String are the same as those in x, works even if string contains '\0' bytes.
  int i;
  for (i = 0; i < n; i++)
    if (String[i] != x[i])
      return FALSE;
  return TRUE; }

//-----------------------------------------------Backtrace
int Backtrace(struct Buf * DestBuf) {
  //Writes backtrace to specified buffer or to console area.
  struct BacktraceFrame *ThisFrame = s_BacktraceFrame;
  struct Com *Command = s_CurrentCommand;
//In windowsland, writing to the last character of the line scrolls the console, there are more elegant ways of preventing this but this works well enough.
#if defined(VC)
#define EndChrGuard 1
#else
#define EndChrGuard 0
#endif
  
  Message(NULL, "Backtrace:");
   
  while (Command && ThisFrame) {
    int ComByteNo = Command->CommandByteNo;
    int ComChrs = Command->ComChrs;
    char String[StringMaxChr];
    int StringLimit = s_TTYMode ? StringMaxChr : s_TermWidth-EndChrGuard;
    int RedefinedBuffer = 0;
    int Bytes = 0, Chrs = 0;
    
    if (ThisFrame->type == InitializationCommand) { //InitializationCommand == 1
      char Key = Command->CommandBuf->BufferKey;
      Chrs = sprintf(String, "  Line %4d of -init commands %c: ", Command->CommandLineNo, (Key ? Key : ' ')); }
    else if (ThisFrame->type == ScriptFrame) { //ScriptFrame == 2
      struct CommandFile *ThisCommFile = ThisFrame->EditorInput;
      Chrs = sprintf(String, "  Line %4d of file %s: ",  ThisCommFile->LineNo, ThisCommFile->FileName); }
    else if (ThisFrame->type == MacroFrame) { //MacroFrame == 3
      char Key = Command->CommandBuf->BufferKey;
      struct BacktraceFrame *CountFrame = ThisFrame;
      unsigned char FrameCount = 0;
      while (CountFrame) { //Check loop - resets the Backtraced status of the buffer.
        FrameCount++;
        CountFrame = CountFrame->prev; }
      RedefinedBuffer = (Command->CommandBuf->FrameCount != FrameCount);
      Chrs = sprintf(String, "  Line %4d of macro %c: ", Command->CommandLineNo, Key ? Key : ' '); }
    else if (ThisFrame->type == CallFrame) {  //CallFrame == 4
      char Key = Command->CommandBuf->BufferKey;
      Chrs = sprintf(String, "  Line %4d of ( %c ) - function %s: ", Command->CommandLineNo, Key ? Key : ' ', ThisFrame->FirstRec->text); }
    else if (ThisFrame->type == ConsoleCommand) { //ConsoleCommand == 5
      char Key = Command->CommandBuf->BufferKey;
      Chrs = sprintf(String, "  Line %4d of console command %c: ", Command->CommandLineNo, (Key ? Key : ' ')); }
    else if (ThisFrame->type == DebuggerCommand) {  //DebuggerCommand == 6
      char Key = Command->CommandBuf->BufferKey;
      Chrs = sprintf(String, "  Line %4d of debugger %c: ",  Command->CommandLineNo, (Key ? Key : ' ')); }
    else
      strcpy(String, "Invalid backtrace stack-frame type");
      
    if (DestBuf) {
      if ( ! RedefinedBuffer ) { //The original source is available.
        int Len = strlen(String);
        GetRecord(DestBuf, Len+strlen(Command->CommandRec->text)+3, NULL);
        strcpy(DestBuf->CurrentRec->text, String);
        strncpy(DestBuf->CurrentRec->text + Len, Command->CommandRec->text, ComByteNo);
        strcpy(DestBuf->CurrentRec->text + Len + ComByteNo, "[");
        strncat(DestBuf->CurrentRec->text + Len, Command->CommandRec->text+ComByteNo, ComChrs);
        strcat(DestBuf->CurrentRec->text + Len, "]");
        strcat(DestBuf->CurrentRec->text + Len, Command->CommandRec->text+ComByteNo+ComChrs); }
      else {  //Original source is assumed to be overwritten.
        int i;
        GetRecord(DestBuf, 100, NULL);
        Chrs = sprintf(DestBuf->CurrentRec->text, "(disassembly) Line %4d: ", Command->CommandLineNo);
        DumpCommand(Command, &i, String+Chrs); } }
     else { //Backtracing to console.
      ScrollConsole(FALSE, FALSE);
      JotAddBoundedString(String, Chrs, Chrs, NULL, NULL, 0);
      if ( ! RedefinedBuffer ) { //The original source is available.
        int FirstByte = 0;                                              //The first byte of the command string, appears at the leftmost position of the line.
        int LastByte = strlen(Command->CommandRec->text);               //The last byte of the command, appears at end of the displayed line.
        int ChrLimit = StringLimit-Chrs;                                //That's the terminal width (or StringMaxChr) adjusted for the Message length.
        int ComBytes = (Command->NextCommand ? Command->NextCommand->CommandByteNo : LastByte)-ComByteNo;
        int ComChrNo = JotStrlenBytes(Command->CommandRec->text, ComByteNo);
        int TotalChrs = JotStrlenBytes(Command->CommandRec->text, LastByte);
        
        if (ChrLimit < LastByte) { //It's possible for the line to be too long for the screen
          if (ChrLimit <= TotalChrs) { //The command string is definitely to long - work out how to truncate it.
            if (ComChrNo <= ChrLimit) //Near the start of the command line - truncate at the end.
              LastByte = JotStrlenChrs(Command->CommandRec->text, ChrLimit);
            else if ((ChrLimit/2) <= TotalChrs-ComChrNo) { //Not near either the start or end of the line - place the current command near the centre of the line.
              FirstByte = ComByteNo-(ChrLimit/2);
              LastByte = FirstByte+JotStrlenChrs(Command->CommandRec->text + FirstByte, ChrLimit); }
            else //Near the end of the command line - truncate at the start.
              //This calculation is a bit dodgy because it's mixing bytes with characters - the worst that can happen is we waste some characters at the end of the line.
              FirstByte = LastByte-ChrLimit; } }
        JotAddBoundedString(Command->CommandRec->text + FirstByte, ComByteNo-FirstByte, ChrLimit, &Bytes, &Chrs, 0);
        TotalChrs = Chrs;
        if (s_TTYMode)
          JotAddBoundedString("[", 1, 1, &Bytes, &Chrs, 0);
        else
          JotSetAttr(Attr_CurrCmd);
        JotAddBoundedString(Command->CommandRec->text+ComByteNo, ComBytes, ChrLimit-TotalChrs, &Bytes, &Chrs, 0);
        TotalChrs += Chrs;
        if (s_TTYMode)
          JotAddBoundedString("]", 1, 1, &Bytes, &Chrs, 0);
        else
          JotSetAttr(Attr_Normal);
        JotAddBoundedString(Command->CommandRec->text+ComByteNo+ComBytes, LastByte-ComByteNo-ComBytes, ChrLimit-TotalChrs, &Bytes, &Chrs, 0); }
      else {  //The original source is no longer available - use a disassembly.
        int i, ChrNo;
        struct Com *ThisCom = Command;
        sprintf(String, "(disassembly) Line %4d: ", Command->CommandLineNo);
        JotAddBoundedString(String, strlen(String), StringLimit, &Bytes, &Chrs, 0);
        JotSetAttr(Attr_CurrCmd);
        ChrNo = 0;
        DumpCommand(ThisCom, &ChrNo, String);
        JotAddBoundedString(String, strlen(String), StringLimit, &Bytes, &Chrs, 0);
        JotSetAttr(Attr_Normal);
        for (i = 0; i < 10; i++) {
          if ( ( ! (ThisCom = ThisCom->NextCommand)) || (StringLimit <= Chrs) )
            break;
          ChrNo = 0;
          DumpCommand(ThisCom, &ChrNo, String);
          JotAddBoundedString(String, strlen(String), StringLimit, &Bytes, &Chrs, 0); } } }
      
    ThisFrame = ThisFrame->prev;
    if ( ! ThisFrame)
      break;
    Command = ThisFrame->LastCommand; }
    
  if (DestBuf)
    GetRecord(DestBuf, 1, "");
  return 0; }

//----------------------------------------------DoWindows
int DoWindows(char Key, struct AnyArg **ThisArg)
  { //Maintains the s_FirstWindow list, interpreting %w commands passed from main command scanner.
  int SpecifiedHeight = 0,  SpecifiedWidth = 0, SliceGuard = 0, WinNo = -1;
  int NextLine = 0, NextColumn = 0;
  int DeltaHeight = 0, DeltaWidth = 0, FreezeValue;
  int WindowIndex = 0;
  int PopupXOffset, PopupYOffset;
  int PredWinType = -1, PredWinTop = 0, PredWinBot = 0;
  int IsLeftmost = FALSE;
  char NewKey = -1;
  int PopupOpt = 0, DelimOpt = 0, WidthOpt = 0, HeightOpt = 0, NewKeyOpt = 0, DeleteOpt = 0, InsertOpt = 0, FreezeOpt = 0;
  struct Window *Win = s_FirstWindow, **WinPtr, *NewWin, *LeftmostSlice = NULL;
  struct Buf *Buffer;
  struct Rec *FirstRec;
  int Operation = FetchIntArgFunc((struct AnyArg **)ThisArg);
  
  if (s_TTYMode) {
    Fail("Attempt to set up windows in a -TTY session.");
    *ThisArg = NULL;
    return 1; }
  if (Key == ' ')
    Key = '\0';
    
  if (Operation == W_modify)
    WinNo = FetchIntArgFunc(ThisArg);
    
  while (*ThisArg) { //Unpack the qualifiers.
    int QualType;
    QualType = FetchIntArgFunc(ThisArg);
    switch (QualType) {
    case W_heightQual:
      HeightOpt = 1;
      SpecifiedHeight = FetchIntArgFunc(ThisArg);
      break;
    
    case W_widthQual:
      WidthOpt = 1;
      SpecifiedWidth = FetchIntArgFunc(ThisArg);
      break;
      
    case W_guardQual:
      SliceGuard = FetchIntArgFunc(ThisArg);
      break;
      
    case W_keyQual: {
      char KeyString[StringMaxChr];
      NewKeyOpt = 1;
      FetchStringArgPercent(KeyString, NULL, ThisArg);
      NewKey = KeyString[0];
      break; }
      
    case W_freezeQual:
      FreezeOpt = 1;
      FreezeValue = FetchIntArgFunc(ThisArg);
      break;
      
    case W_deleteQual:
      DeleteOpt = 1;
      break;
      
    case W_delimQual:
      DelimOpt = 1;
      break;
      
    case W_popupQual:
      PopupOpt = 1;
      PopupXOffset = FetchIntArgFunc(ThisArg);
      PopupYOffset = FetchIntArgFunc(ThisArg);
      break;
      
    case W_insertQual:
      InsertOpt = 1;
      WinNo = FetchIntArgFunc(ThisArg);
      break; } }
      
  if (Operation == W_clear) { //Delete all windows and clear screen.
    int Line;
    while (Win) { // Window-clear loop.
      struct Window *NextWin = Win->next;
      free(Win);
      Win = NextWin; }
    for (Line = 0; Line <= s_TermHeight; Line++) {
      s_TermTable[Line] = NULL;
      JotGotoXY(0, Line);
      JotClearToEOL(); }
    memset(s_TermTable, 0, (s_TermHeight+1)*(s_MaxSlices+10)*sizeof(char *));
    s_FirstWindow = NULL;
    s_ConsoleTop = 0;
    return 0; }
    
  if (Operation == W_refresh) { //Redraw entire screen.
    JotUpdateWindow();
    s_NewConsoleLines = 0;
    if ( ( ! s_InView) && (s_Verbose & Verbose_NonSilent) )
      DisplayDiag(s_CurrentBuf, TRUE, "");
    return 0; }
   if ( (Operation == W_new) && ( (SpecifiedHeight == 0) && (SpecifiedWidth == 0) ) ) { //Add delimiter to last-defined window.
    SynError(NULL, "Window height and width are both zero.");
    return 1; }

  //Search the window list for selected window or the last window/leftmost slice of last window.
  WinPtr = &s_FirstWindow;
  while( (Win = *WinPtr) ) { //Selected-window search loop.
    int Type = (Win->WindowType) & WinType_Mask;
    if (InsertOpt && WindowIndex == WinNo)
      break;
    if ( ((Win->WindowType & WinType_Mask) == WinType_Slice) && (Win->WindowType & WinType_Leftmost) )
      LeftmostSlice = Win;
    else if ( (Win->WindowType & WinType_Mask) != WinType_Slice)
      LeftmostSlice = NULL;
    if (WindowIndex++ == WinNo) { //Found window for modification or insertion.
      if (HeightOpt)
        DeltaHeight = SpecifiedHeight - (InsertOpt ? 0 : Win->Bot-Win->Top+1);
      if (WidthOpt)
        DeltaWidth = SpecifiedWidth - (InsertOpt ? 0 : Win->Right-Win->Left+1);
      break; }
    NextLine = Win->Bot+1;
    if (Win->Left == 0 && Win->Right == s_TermWidth-1)
      NextColumn = 0;
    else
      NextColumn = Win->Right+1;
    if ( InsertOpt && ( ! SpecifiedHeight) && (Type != WinType_Slice) && ( ! (Win->next && (Win->next->WindowType & WinType_Mask) != WinType_Slice)) ) {
      Fail("Attempt to add a slice to a non-sliced window.");
      return 1; }
    PredWinType = (Win->WindowType & WinType_Mask);
    PredWinTop = Win->Top;
    PredWinBot = Win->Bot;
    WinPtr = &(Win->next); }

  if (Operation == W_modify) { //Modify an existing window.
    int Type;
    NewWin = Win;
    if ( ! Win)
      return Fail("Attempt to modify a nonexistent window.");
    Type = (Win->WindowType) & WinType_Mask;
    if ( (DeltaHeight || HeightOpt) && ( (Win->Bot+DeltaHeight < Win->Top) || (s_TermHeight-1 < s_ConsoleTop+DeltaHeight) ) )
      return Fail("Invalid window-height change.");
    if (HeightOpt && (Type == WinType_Slice) && ! (Win->WindowType & WinType_Leftmost))
      return Fail("For window slices, only the leftmost slice can have it's height redefined.");
      
    //Do the requested modifications to the selected window.
    //  -insert is dealt with by the normal window-creation sequence.
    if (DeleteOpt) { //Delete selected window and move later windows/slices up/left.
      struct Window **NewLink = WinPtr;
      IsLeftmost = Win->WindowType & WinType_Leftmost;
      if ( IsLeftmost && ( ( ! Win->next) || (Win->next->WindowType & WinType_Leftmost)) ) //Deleting either a full-width slice or the one-and-only slice in a group.
        DeltaHeight = Win->Top - Win->Bot - 1;
      else if (Type == WinType_Slice)
        DeltaWidth = Win->Left - Win->Right - 1;
      *NewLink = Win->next;
      free(Win);
      if (*NewLink) {
        Win = *NewLink; }
      else
        Win = NULL; }
    if (Win && (DeltaHeight || DeltaWidth) ) { //For height or width adjustment, only change the Bot or Right attributes respectively.
      if (DeltaWidth && PredWinType == WinType_Normal)
        return Fail(InsertOpt ? "Attempt to add a slice to a full-width window" : "Attempt to redefine the width of a normal window.");
      if (Type == WinType_Slice) {
        struct Window *TestWin = Win;
        if (DeltaHeight && ( ! (Win->WindowType && WinType_Leftmost)))
          return Fail("Attempt to change the height of a secondary slice.");
        if (DeltaWidth < 0 && TestWin && ! ((TestWin->WindowType) & WinType_Leftmost) && (TestWin->Left+DeltaWidth < 0) )
            return Fail("Invalid window-width shrink.");
        if (0 < DeltaWidth) {
          while ( TestWin && TestWin->next && ! (TestWin->next->WindowType & WinType_Leftmost))
            TestWin = TestWin->next;
          if (TestWin && s_TermWidth < TestWin->Right+DeltaWidth)
            return Fail("Invalid window-width expand."); } }
      if (SpecifiedWidth) {
        if (InsertOpt && (Win->WindowType & WinType_Leftmost) && (Win->WindowType & WinType_Slice) ) {
          Win->WindowType = (Win->WindowType & (-1^WinType_Leftmost));
          IsLeftmost = FALSE;
          NextColumn = 0; }
        else if ( (Win->WindowType & WinType_Mask) == WinType_Normal ) //This changes a normal (full-width) window to a slice.
          Win->WindowType = WinType_LeftmostSlice; }
      if ( ! DeleteOpt && ! InsertOpt ) {
        Win->Bot += DeltaHeight;
        Win->Right += DeltaWidth; } } }

  if (Operation == W_new) {
    if ( ! SpecifiedHeight && ! SpecifiedWidth) //Add a new window.
      return Fail("No window height or width specified for new window.");
    if (SpecifiedWidth < 0)
      return Fail("Negative width in %%W width expression.");
    if (SpecifiedHeight < 0)
      return Fail("Negative height in %%W height expression.");
    if (SpecifiedWidth && ! SpecifiedHeight && ! LeftmostSlice )
      return Fail("Attempt to define a slice with no explicit or implicit window height specification.");
    NewWin = (struct Window *)JotMalloc(sizeof(struct Window));
    NewWin->PopupWidth = 0;
    NewWin->DisplayFooter = 0;
    NewWin->WindowType = (PopupOpt ? WinType_Popup : (SpecifiedWidth ? WinType_Slice : WinType_Normal));
    if (PopupOpt) { //Popup windows calculate Top, Bottom, Left and Right directly from the width and height.
      if (0 < SpecifiedHeight) {
        NewWin->Top = 0;
        NewWin->Bot = SpecifiedHeight-1; }
      else if (SpecifiedHeight == 0) {
        NewWin->Top = 0;
        NewWin->Bot = s_TermHeight-1; }
      else {
        NewWin->Top = s_ConsoleTop+SpecifiedHeight-1;
        NewWin->Bot = s_ConsoleTop-1; }
      if (0 < SpecifiedWidth) {
        NewWin->Left = 0;
        NewWin->Right = SpecifiedWidth-1; }
      else if (SpecifiedWidth == 0) {
        NewWin->Left = 0;
        NewWin->Right = s_TermWidth-1; }
      if (PopupXOffset < 0) { //Negative popup x-offset indicates position relative to screen's righthand margin.
        NewWin->Left = s_TermWidth + PopupXOffset - SpecifiedWidth+1;
        NewWin->Right = s_TermWidth + PopupXOffset; }
      else {
        NewWin->Left = PopupXOffset;
        NewWin->Right = PopupXOffset + SpecifiedWidth - 1; }
      if (PopupYOffset < 0) { //Negative popup y-offset indicates position relative to screen's bottom margin.
        NewWin->Bot = s_TermHeight + PopupYOffset;
        NewWin->Top = s_TermHeight + PopupYOffset - SpecifiedHeight - 1; }
      else {
        NewWin->Top = PopupYOffset;
        NewWin->Bot = PopupYOffset + SpecifiedHeight - 1; } }
    else { //Any non-popup window.
      if (s_TermHeight < (NextLine+SpecifiedHeight+1) ) {
        free(NewWin);
        return RunError("Total height of windows exceeds terminal height"); }
      if (s_TermWidth < (NextColumn+SpecifiedWidth+SliceGuard) ) {
        free(NewWin);
        return RunError("Total width of slices exceeds terminal width"); }
        
      if (InsertOpt && SpecifiedHeight)
        DeltaHeight = SpecifiedHeight;
      if (SpecifiedWidth) { //For sliced windows left and Right are calculated from the aggregate slice widths.
        if (InsertOpt)
          DeltaWidth = SpecifiedWidth;
        if (SpecifiedHeight) { //Only the first slice has height defined.
          NewWin->Top = NextLine;
          NewWin->Bot = NextLine+SpecifiedHeight-1;
          NewWin->WindowType |= WinType_Leftmost;
          NextColumn = 0; }
        else if (LeftmostSlice) { //Further slices of the same window inherit Top and Bot from the first window.
          NewWin->Top = LeftmostSlice->Top;
          NewWin->Bot = LeftmostSlice->Bot;
          if (InsertOpt && (LeftmostSlice == *WinPtr) ) //The original leftmost slice is now the second slice.
            NewWin->WindowType |= WinType_Leftmost; }
        else if (InsertOpt && (PredWinType == WinType_Slice) ) {
          NewWin->Top = PredWinTop;
          NewWin->Bot = PredWinBot; }
        else {
          free(NewWin);
          return SynError(NULL, "No window height specified"); }
        NewWin->Left = NextColumn+SliceGuard;
        NewWin->Right = NewWin->Left + SpecifiedWidth - 1; }
      else { //Normal, full-width, window.
        NewWin->Top = NextLine;
        NewWin->Bot = NextLine+SpecifiedHeight-1;
        NewWin->WindowType |= WinType_Leftmost;
        NewWin->Left = 0;
        NewWin->Right = s_TermWidth-1; }
      if ( ! InsertOpt)
        s_ConsoleTop = NewWin->Bot+1; }
    
    if (InsertOpt) { //Insert the new window into the window chain.
      NewWin->next = *WinPtr;
      *WinPtr = NewWin; }
    else { //Append new window to end of window chain.
      NewWin->next = NULL;
      *WinPtr = NewWin; }
      
    NewWin->OldFirstLineNo = 1;
    NewWin->LastSubstringStart = 0;
    NewWin->LastSubstringLength = 0;
    Key = ChrUpper(Key);
    NewWin->WindowKey = Key;
    NewWin->LastKey = '\0';
    InitTermTable();
    if ( (Key != '\0') && (Buffer = GetBuffer(Key, OptionallyNew)) ) {
      FirstRec = Buffer->FirstRec;
      if (FirstRec != NULL)
        FirstRec->prev->DisplayFlag = Redraw_Line;
      else
        GetRecord(Buffer, 1, ""); } }

  if (NewWin) {
    if (NewKeyOpt)
      NewWin->WindowKey = toupper(NewKey);
    if (FreezeOpt)
      NewWin->WindowType = FreezeValue ? (NewWin->WindowType | WinType_Frozen) : (NewWin->WindowType & (-1^WinType_Frozen));
    if (DelimOpt)
      NewWin->DisplayFooter = ! NewWin->DisplayFooter; }
        
  if (Win && (DeltaWidth || DeltaHeight) ) { //Adjust all follwing slice-group members.
    int Type = (Win->WindowType) & WinType_Mask,  AdjustedWindowTop = Win->Top;
    if (Type == WinType_Slice) { 
      if ( ! DeleteOpt && ! InsertOpt )
        Win = Win->next;
      while ( Win && ! (Win->WindowType & WinType_Leftmost)) { //Adjust Left and Right of successor windows.
        if ((Win->Left+DeltaWidth < 0) || s_TermWidth < Win->Right+DeltaWidth) {
          Fail("Invalid window-width change.");
          return 1; }
        Win->Bot += DeltaHeight;
        Win->Left += DeltaWidth;
        Win->Right += DeltaWidth;
        if (IsLeftmost && Win)
          Win->WindowType |= WinType_Leftmost;
        IsLeftmost = FALSE;
        Win = Win->next; } }
          
      //Adjust Top and Bot of successor windows.
      while ( Win ) { //Adjust following windows loop.
        if (DeleteOpt || InsertOpt || AdjustedWindowTop < Win->Top) { //Adjusting Top and Bot of windows below selected window.
          Win->Top += DeltaHeight;
          Win->Bot += DeltaHeight; }
        Win = Win->next; } }
        
  s_ConsoleTop += DeltaHeight;
  if ( ! InsertOpt) {
    JotUpdateWindow();
    return 0; }

  JotUpdateWindow();
  return 0; }

//----------------------------------------------InitTermTable
int InitTermTable()
  { //Frees existing screen table, mallocs and initializes another one, resets right margin of normal windows and clears the screen.
  int LocalFail = 0, Slices = 0, WinNo = 0, Line, Errors = 0;
  struct Window *Win = s_FirstWindow;
  
  if (s_TermTable)
    free(s_TermTable);
  s_MaxSlices = 1;
    
  while (Win) { // Slice-counting loop.
    Win->WindowType &= (-1^WinType_Void);
    if ((Win->WindowType & WinType_Mask) == WinType_Normal)
      Win->Right = s_TermWidth-1;
    if ((Win->WindowType & WinType_Mask) == WinType_Slice) {
      if (s_MaxSlices < ++Slices)
        s_MaxSlices = Slices;
      if (s_TermWidth < Win->Right) {
        Errors += 1;
        Win->WindowType |= WinType_Void; } }
      else
        Slices = 1;
    if (s_TermHeight < Win->Bot) {
        Errors += 1;
      Win->WindowType |= WinType_Void; }
    WinNo++;
    Win = Win->next; }
      
  s_TermTable = (char **)JotMalloc((s_TermHeight+1)*(s_MaxSlices+10)*sizeof(char *));
  memset(s_TermTable, 0, (s_TermHeight+1)*(s_MaxSlices+10)*sizeof(char *));
  
  for (Line = 0; Line < s_ConsoleTop; Line +=1) {
    JotGotoXY(0, Line);
    JotClearToEOL(); }
    
  if (Errors)
    LocalFail = Fail("Check window sizes, terminal dimensions exceeded by %d windows.", Errors);
    
  return LocalFail; }

//-----------------------------------------------ForceLineRefresh
void ForceLineRefresh()
  { //Resets the s_TermTable entry for the current line of the current buffer.
    //Required for the occasional operation affecting a line of the display without changing the text (e.g. adding or removing colour tags).
  struct Window *Win = s_FirstWindow;
  
  while (Win) { // Slice-search loop.
    if (Win->LastKey == s_CurrentBuf->BufferKey) { //Found first window displaying the current buffer.
      int CurrentLineInWin = (s_CurrentBuf->LineNumber)-(Win->OldFirstLineNo);
      if ( (1 <= CurrentLineInWin) && (CurrentLineInWin <= (Win->Bot-Win->Top)) ) { //Current line is visible in this window.
        int TermLineNo = Win->Top + s_CurrentBuf->LineNumber - Win->OldFirstLineNo;
        s_TermTable[TermLineNo] = NULL; } }
    Win = Win->next; } }

//----------------------------------------------AnalyzeTabs
int AnalyzeTabs(struct Buf *TextBuf, struct Rec *FirstRec, int LineCount)
  { //FirstRec (the first record in the screen window) and LineCount (the number of lines in window), together define a sequence of visible records in the buffer, 
    //thisFinds position of each tab in the section and assigns auto-TabStops
    //If the buffer has a header record defined, this may contain tabs so this record is also included in the analysis.
    //Returns 0 if no change to tabstops, otherwise 1.
    //
  struct Rec *Record = FirstRec;    //The current record, at the top of the window.
  int TabsHWM = 0;                  //The most tabs that have been found in any one line, to date.
  int TabIndex;                     //Used as an index for final tweaks.
  int LineIndex;                    //Counts lines.
  int TabstopsChange = FALSE;       //Flag indicates that the tabstops have been changed.
  int NewTabStops[MaxAutoTabStops]; //The newly-assigned set of tabstops.
  char *String = Record->text;      //The record text from a record currently in the display.
  
  NewTabStops[0] = 0; 
   
  for (LineIndex = 0; LineIndex <= LineCount+1; LineIndex++) { //Note the width of each column.
    int CellWidth;                                             //The width of the text in the current cell.
    int TabStopIndex = 0;                                      //Counts tabs in the current record.
    char *SubString;                                           //The text following the most recently-found tab.
    
    if (strchr(String, s_TableSeparator))
      while ( 1 ) {
        if ( ! (SubString = strchr(String, s_TableSeparator)))
          SubString = String+strlen(String);
        if (MaxAutoTabStops < ++TabStopIndex) {
          Fail("More than %d tabs in record %s while assigning TabStops", MaxAutoTabStops, String);
          return 0; }
        CellWidth = JotStrlenBytes(String, SubString-String)+1;
           
        if (TabsHWM < TabStopIndex) {
          NewTabStops[TabStopIndex] = 0;
          TabsHWM = TabStopIndex;
          NewTabStops[0] = TabStopIndex; }
        if (NewTabStops[TabStopIndex] < CellWidth)
          NewTabStops[TabStopIndex] = CellWidth;
        if (SubString[0] == '\0')
          break;
        String = SubString+1; }
      
    if (LineIndex < LineCount-1) {
      Record = Record->next;
      String = Record->text; }
    else //The extra line is reserved for the header - if specified.
      if (TextBuf->Header)
        String = TextBuf->Header;
      else
        break; }
    
  for (TabIndex = 2; TabIndex <= NewTabStops[0]; TabIndex++) { //Currently the table only has column widths - change to absolute column numbers.
    NewTabStops[TabIndex] += NewTabStops[TabIndex-1];
    if ( (TextBuf->TabStops[0] < TabIndex) || (TextBuf->TabStops[TabIndex] != NewTabStops[TabIndex]) )
      TabstopsChange = TRUE; }
    
  if ( ! TabstopsChange)
    return 0;
     
  for (TabIndex = 0; TabIndex <= NewTabStops[0]; TabIndex++) //Copy new tabstops.
    TextBuf->TabStops[TabIndex] = NewTabStops[TabIndex];
     
  for (LineIndex = 0; LineIndex < s_TermHeight; LineIndex +=1)
    s_TermTable[LineIndex] = NULL;
  return 1; }

//----------------------------------------------MoveDown
int MoveDown(struct Buf *TextBuf, int Offset, char *EndRecord)
  { // Resets character pointer to begining of record, 'Offset' records away from the current record.  
    //If one end of the text image or the optional specified EndRecord is encountered then it stops there.
    //Function returns no. (+ve. or -ve.) records actually moved. 
     
  int Distance = 0;
  struct Rec *Record = TextBuf->CurrentRec;
  struct Rec *FirstRecordOfBuf = TextBuf->FirstRec;
  struct Rec *LastRecordOfBuf = FirstRecordOfBuf->prev;
  int Count;
  
  if (0 <= Offset)
    for (Count = 1; Count <= Offset; Count++) {// +ve. or zero offset: forwards loop.
      if (Record == LastRecordOfBuf || Record->text == EndRecord)
        break;
      Record = Record->next;
      Distance = Distance+1; }
  else
    for (Count = 1; Count <= -Offset; Count++) {// -ve. offset backwards loop.
      if (Record == FirstRecordOfBuf || Record->text == EndRecord)
        break;
      Record = Record->prev;
      Distance = Distance-1; }
  
  TextBuf->CurrentRec = Record;
  return Distance; }

//-----------------------------------------------GetChrNo
int GetChrNo(struct Buf *TextBuf)
  { //Returns position of cursor ignoring any leftoffset setting for linear or tabular text.
  int PerceivedChr = 0;
  int *TabStops = TextBuf->TabStops;
  int TabCells = TextBuf->TabCells;
  int CurrentByte = TextBuf->CurrentByte;
  int TabCount = 1;
  int TabStop = 0;
  int Bytes = 0;
  int FirstByte = 0;
  int CellWidth, FullWidth, VisibleWidth;
  char *CellStart, *CellEnd;
   
  if (TabStops && TabCells) { //Text displayed as tabcells as in spreadsheets.
    for ( ; Bytes <= CurrentByte; Bytes++) {
        if (TextBuf->CurrentRec->text[Bytes] == s_TableSeparator) {
          if (TabStops[0] <= TabCount)
            TabStop += TabStops[TabCount] - (TabCount==1 ? 0 : TabStops[TabCount-1]);
          else
            TabStop = TabStops[TabCount++];
          FirstByte = Bytes+1;
          PerceivedChr = TabStop; } }
      CellStart = TextBuf->CurrentRec->text+FirstByte;
      CellEnd = strchr(CellStart, s_TableSeparator);
      CellWidth = TabStops[TabCount] - (TabCount==1 ? 0 : TabStops[TabCount-1]);
      FullWidth = JotStrlenBytes(CellStart, (CellEnd ? CellEnd-CellStart : strlen(CellStart)));
      VisibleWidth = JotStrlenBytes(CellStart, Bytes-FirstByte);
      PerceivedChr += CellWidth - FullWidth + VisibleWidth - 1; }
      
  else if (TabStops) { //Simple tabular text.
    int ByteNo = 0, TabPoint = 0, TabIndex = 0, *TabStops = TextBuf->TabStops, PrevTab = 0;
    char *String = TextBuf->CurrentRec->text, *NextSubstring;
    
    while ( (NextSubstring = strchr(String+ByteNo, s_TableSeparator)) ) {
      int TabByte = NextSubstring-String, StringChrs;
      if (CurrentByte < TabByte)
        break;
      StringChrs = TabPoint + JotStrlenBytes(String+ByteNo, TabByte-ByteNo);
      
      while (TabPoint <= StringChrs) { //Search for the relevant tabstop.
        if (TabIndex < TabStops[0]) {
          PrevTab = TabPoint;
          TabPoint = TabStops[++TabIndex]; }
        else
          TabPoint += TabStops[TabIndex]-PrevTab; }
          
      ByteNo = NextSubstring-String+1; }
        
    PerceivedChr = TabPoint+JotStrlenBytes(String+ByteNo, CurrentByte-ByteNo); }
    
  else
    PerceivedChr = JotStrlenBytes(TextBuf->CurrentRec->text, TextBuf->CurrentByte);
    
  return PerceivedChr; }

#ifdef linux
int JotRefresh() { return refresh(); }
#endif

//----------------------------------------------WriteString
int WriteString(
  char *OrigString, int ScreenLine, int Attr, int New, int LastByte, int LeftOffset, int ColourPair, int OrigTabStops[], int TabCells, struct Window *Win)
  { //
    //Checks length of string then outputs text string to window, trapping control characters.
    //New is set true at the start of a new line.
    //The static variables ByteNo and s_ChrNo, for normal ascii text contining no tabs or unicode characters,  will always track one another.
    //Lines containing tabstops tends to make the s_ChrNo overtake ByteNo - as WriteString inserts padding blanks to align the columns.
    //Lines containing unicode tends to make ByteNo overtake s_ChrNo as additional bytes are extracted to define one unicode character on the screen.
    //The static variable TabCount indicates the current TabChrs[] entry of the current character.
    //N.B. This proceedure is designed to be called in sequence, with different Attr settings starting at byte 0 of the record.
    //Returns TRUE if, on exit, s_ChrNo is in view.
    //
  static int ByteNo;              //Points to next byte in string.
  static int TabIndex;            //Counts tabs found in String.
  static int TabIndexMax;         //Last valid entry in TabBytes and TabChrs.
  static int PrevTab;             //Used to extrapolate tabstops from an incomplete tabstops specification.
  static int *TabBytes = NULL;    //Indicates character nos. of each tab point, TabBytes[0] must bt set to 0.
  static int *TabChrs = NULL;     //Expansion of OrigTabStops, TabChrs[0] is 0.
  static char *String = NULL;     //Copy of original string with control characters changed to tildes.
  static int EndStopChr;          //The last character of the cell to appear on screen (normally CellChrs[CellIndex+1]-1, but sometimes limited by screenwidth.
  static int PaddingChrs;         //No. of blanks to be used as a prefix to cell characters. If negative, the cell text is to be truncated.
  static int InView = TRUE;       //Set FALSE by various out-of-view checks.
  static int TotalBytes;          //Total length of original string.
  static int TabPoint;            //Used for simple tabbed text - notes position of last tab in screen view.
  static int SliceWidth;          //The width, in characters, of this slice.
      
  int ByteCount, ChrCount;
  short unsigned int Left;
  short unsigned int Right;
  int PopupWindow = (Win->WindowType & WinType_Mask) == WinType_Popup;
  int PopupOffset = 0;
  int i;

  if (PopupWindow) {
     PopupOffset = Win->Right - Win->Left - Win->PopupWidth;
    if (Win->Left == 0) { //Popup is in a left corner.
      Left = Win->Left;
      Right = Win->Right - PopupOffset; }
    else { //Popup is in a right corner.
      Left = Win->Left + PopupOffset;
      Right = Win->Right; } }
  else {
    Left = Win->Left;
    Right = Win->Right; }
      
  if (New) {
    SliceWidth = Right - Left + 1;
    TotalBytes = strlen(OrigString);
    TabPoint = 0;
    TabIndexMax = 0;
    InView = TRUE;
    ByteNo = 0;
    s_ChrNo = 1;
    if (TabBytes) {
      free(TabBytes);
      free(TabChrs); }
    TabBytes = TabChrs = NULL;
       
    if (OrigTabStops) { //Initialize for tabular text.
      if (TabCells) { //Initialize for tabcells.
        char *EndPoint = OrigString;
        int OrigTabIndex = 1;
        int PrevTabStop = 0;
         
        TabBytes = (int *)JotMalloc(sizeof(int)*(TotalBytes+2));
        TabChrs = (int *)JotMalloc(sizeof(int)*(TotalBytes+2));
        TabBytes[0] = 0;
        TabChrs[0] = 0;
        TabIndex = 0;
        PrevTabStop = 0;
        while ( (EndPoint = strchr(EndPoint, s_TableSeparator)) ) {
          TabBytes[++TabIndex] = (EndPoint++)-OrigString+1;
          TabChrs[TabIndex] = (OrigTabIndex <= OrigTabStops[0]) ? OrigTabStops[OrigTabIndex++] : (TabChrs[TabIndex-1]*2)-PrevTabStop;
          PrevTabStop = TabChrs[TabIndex-1]; }
        TabBytes[++TabIndex] = TotalBytes+1;
        TabChrs[TabIndex] = (OrigTabIndex <= OrigTabStops[0]) ? OrigTabStops[OrigTabIndex++] : (TabChrs[TabIndex-1]*2)-PrevTabStop;
        TabIndexMax = TabIndex;
        TabIndex = -1;
        EndStopChr = LeftOffset+SliceWidth; }
      else { //Initialize for tabstops.
        s_ChrNo = 0;
        TabIndex = 0;
        if (OrigString) { //Just count tabs.
          char *SubString = OrigString, *NextSubString;
          while ( (NextSubString = strchr(SubString, s_TableSeparator)) ) {
            SubString = NextSubString+1;
            TabIndexMax++; } } } }
     
    if (String)
      free(String);
    String = (char *)JotMalloc(TotalBytes+1);
    if (OrigTabStops && ! TabCells) { //Initialize for simple tabbed text.
      for (i=0; i < TotalBytes; i++) { //Replace control characters, except tabs, with tildes.
        signed char Chr = OrigString[i];
        if (Chr < ' ' && Chr != s_TableSeparator) {
          if (Chr < 0) {
            if (s_NoUnicode)
              Chr = '~'; } }
        String[i] = Chr; } }
    else {
      for (i=0; i < TotalBytes; i++) { //Replace all control characters with tildes.
        signed char Chr = OrigString[i];
        if (Chr < ' ') {
          if (Chr < 0) {
            if (s_NoUnicode)
              Chr = '~'; }
          else
            Chr = '~'; }
        String[i] = Chr; } }
    String[TotalBytes] = '\0';
       
    JotGotoXY(Left, ScreenLine); }
       
  JotSetAttr(Attr);

  if (TabIndexMax < 1) { //Either linear text or there are no tabcells or tabstops set for this buffer.
    int Bytes = 0, Chrs = 0, FirstByte = 0;    //The first byte of the string, this appears in the leftmost character of the slice line.
    
    if (New)
      s_ChrNo = Left+LeftOffset;
    FirstByte = JotStrlenChrs(String, s_ChrNo-Left);    //The first byte of the string, this appears in the leftmost character of the slice line.
    
    InView = TRUE;
    if (FirstByte < LastByte) {
      if (ByteNo < TotalBytes)
        JotAddBoundedString(String+FirstByte, LastByte-FirstByte, SliceWidth-(s_ChrNo-Left-LeftOffset), &Bytes, &Chrs, ColourPair);
      s_ChrNo += Chrs;
      ByteNo = LastByte;
      InView = LastByte-FirstByte == Bytes; }
    else
      InView = FALSE;
        
    return InView; }

  if (TabChrs) { //Display a section of a tab-separated-cells record.
    // TabBytes[TabIndex] = Byte at start of current-cell text.
    // TabBytes[TabIndex+1]-2 = Last Byte of current cell.
    // TabChrs[TabIndex] = (X-coord-LeftOffset) of character at start of cell display
    // TabChrs[TabIndex+1]-1 = (X-coord-LeftOffset) of character at end of cell display.
    if (TabChrs[TabIndexMax] < LeftOffset) {  //The LeftOffset has been set beyond the end of this line.
      s_ChrNo = LeftOffset;
      return FALSE; }
    for ( ; ByteNo < LastByte && s_ChrNo < TabChrs[TabIndexMax]; ) {
      if (TabIndex < 0 || TabBytes[TabIndex+1]-2 < ByteNo) {            //New cell - setup.
        int CellChrs;
        if (TabIndexMax <= ++TabIndex)
          break;
        if (TabChrs[TabIndex+1]-1 < LeftOffset) {                       //All of this cell is left of the LeftOffset - skip to next cell.
          ByteNo = TabBytes[TabIndex+1];
          InView = FALSE;
          continue; }
        else
          InView = TRUE;
        ByteNo = TabBytes[TabIndex];
        s_ChrNo = TabChrs[TabIndex];
        EndStopChr = LeftOffset+SliceWidth < TabChrs[TabIndex+1] ? LeftOffset+SliceWidth : TabChrs[TabIndex+1];
        if (EndStopChr <= s_ChrNo)
          break;
        CellChrs = JotStrlenBytes(String+TabBytes[TabIndex], TabBytes[TabIndex+1]-TabBytes[TabIndex]-1);
        PaddingChrs = TabChrs[TabIndex+1]-s_ChrNo-CellChrs;
        if (SliceWidth+LeftOffset <= s_ChrNo+PaddingChrs)               //All of current cell's text is off the right margin of the screen - exit now.
          PaddingChrs = SliceWidth+LeftOffset-s_ChrNo;
        if (s_ChrNo < LeftOffset) {                                     //Current cell has text left of LeftOffset - truncation required.
          int ChrsToLop = LeftOffset-s_ChrNo;
          if (PaddingChrs < ChrsToLop)                                  //Truncate leftmost characters.
            ByteNo += JotStrlenChrs(String+ByteNo, ChrsToLop-PaddingChrs);
          PaddingChrs -= ChrsToLop;
          s_ChrNo += ChrsToLop; }
        if (0 < PaddingChrs) {                                          //Text is shorter than allocated cell - justify with blanks.
          char Padding[StringMaxChr];
          memset(Padding, ' ', PaddingChrs);
          Padding[PaddingChrs] = '\0';
          JotAddBoundedString(Padding, PaddingChrs, SliceWidth, &ByteCount, &ChrCount, ColourPair);
          s_ChrNo += ChrCount; }
        else if (PaddingChrs < 0) {                                     //Negative padding indicates a truncated cell.
          JotAddBoundedString("!", 1, 1, NULL, NULL, ColourPair);
          ByteNo += JotStrlenChrs(String+ByteNo, 1);
          s_ChrNo++;
          InView = FALSE; }
        if (SliceWidth+LeftOffset <= TabChrs[TabIndex+1]-1 && s_ChrNo < EndStopChr) { //Current cell truncated by right margin.
          JotAddBoundedString("!", 1, 1, NULL, NULL, ColourPair);
          ByteNo += JotStrlenChrs(String+ByteNo, 1);
          s_ChrNo++;
          InView = FALSE; } }
          
      if (SliceWidth <= EndStopChr-s_ChrNo) //No point in attempting to display anything beyond the right margin.
        return FALSE;
      JotAddBoundedString(String+ByteNo, LastByte-ByteNo, EndStopChr-s_ChrNo, &ByteCount, &ChrCount, ColourPair);
      ByteNo += ByteCount;
      s_ChrNo += ChrCount;
      if (SliceWidth+LeftOffset <= s_ChrNo)
        return LastByte <= ByteNo && InView;
         
      if (ByteNo < LastByte)
        ByteNo = TabBytes[TabIndex+1]; }
        
    if ( ! s_ChrNo)                                                         //All  tabs are left of LeftOffset - s_ChrNo needs to keep up withLeftOffset
      s_ChrNo = LeftOffset;                                                 //to prevent JotClrToEndOfSlice() hangups:
                                                                            //jot_dev ${JOT_RESOURCES}/test_table.txt -in="%b=tabcells -1; %b=leftoffset 100000000;"
    return LastByte <= ByteNo && InView; }

  else { //String contains at least one tab - display as simple tabular text.
    int Bytes = 0, Chrs = 0;
    
    while (s_ChrNo-LeftOffset < SliceWidth && ByteNo < LastByte ) { //Tab-separated substring loop - exits when we reach the right-screen boundary.
      char *NextSubstring = strchr(String+ByteNo, s_TableSeparator);
      int EndStopByte =  NextSubstring ? NextSubstring-String : strlen(String);
      
      if (LastByte < EndStopByte)
        EndStopByte = LastByte;
      InView = TRUE;
      
      //Write the substring.
      Chrs = JotStrlenBytes(String+ByteNo, EndStopByte-ByteNo);
      if (s_ChrNo+Chrs < LeftOffset) {                                                            //None of this substring is viewable - all left of the leftOffset.
        ByteNo = EndStopByte;
        s_ChrNo += Chrs; }
      else {
        if (s_ChrNo < LeftOffset) {                                                               //Only part of this substring is in view.
          int NextVisibleByte = JotStrlenChrs(String+ByteNo, LeftOffset-s_ChrNo);
          JotAddBoundedString(String+ByteNo+NextVisibleByte, EndStopByte-ByteNo-NextVisibleByte, SliceWidth-s_ChrNo+LeftOffset, &Bytes, NULL, ColourPair);
          ByteNo += Bytes+NextVisibleByte; }
        else if (ByteNo < TotalBytes) {                                                           //All of this substring is in view.
          JotAddBoundedString(String+ByteNo, EndStopByte-ByteNo, SliceWidth-s_ChrNo+LeftOffset, &Bytes, NULL, ColourPair);
          ByteNo += Bytes; }
        else if (ByteNo == TotalBytes && Attr == Attr_CurrChr && s_ChrNo+LeftOffset < SliceWidth)
          JotAddBoundedString("~", 1, 1, NULL, &Chrs, ColourPair);                                //CurrentByte is at the end of the string.
        s_ChrNo += Chrs;
        InView = (LastByte-ByteNo <= Bytes); }
      if (LastByte <= ByteNo)
        break;
        
      //Add the padding whitespace at the current tab point.
      if (NextSubstring && NextSubstring[0] == s_TableSeparator) { //Tab-terminated substring - calculate and add correct number of padding blanks.
        char Padding[StringMaxChr];
        
        while (TabPoint <= s_ChrNo) { //Search for the next relevant tabstop.
          if (TabIndex < OrigTabStops[0]) {
            PrevTab = TabPoint;
            TabPoint = OrigTabStops[++TabIndex]; }
          else
            TabPoint += OrigTabStops[TabIndex]-PrevTab; }  
              
        if (LeftOffset <= TabPoint) { //Some of the padding is in the visible area of line.
          int PaddingChrs = (s_ChrNo < LeftOffset) ? TabPoint-LeftOffset : TabPoint-s_ChrNo;
          memset(Padding, ' ', PaddingChrs);
          Padding[PaddingChrs] = '\0';
          JotAddBoundedString(Padding, PaddingChrs, SliceWidth-s_ChrNo+LeftOffset, NULL, &Chrs, ColourPair); }
        s_ChrNo = TabPoint;
        ByteNo++; }
      else
        return LastByte <= ByteNo && InView; }
      
    return LastByte <= ByteNo && InView; } }

//-----------------------------------------------JotUpdateWindow
int JotUpdateWindow()
  { //Calculates new start and end points then displays new and changed lines, returns FALSE if current buffer has not been displayed.
  struct Window *Win = s_FirstWindow;
  int DisplayedCurrentByte = FALSE;                   //Flag is set TRUE only when the CurrentByte of the s_CurrentBuf appears in display. 
  char StuckOnce = FALSE;                             //When the situation is too confusing, it resets and reiterates *once*
  int SliceIndex = 0;                                 //The first n entries in s_TermTable are lines, followed by secondary slices.
  
  s_InView = FALSE;
  if (s_TTYMode || ((s_CurrentBuf->CurrentRec) == NULL) ) 
    return DisplayedCurrentByte;
#if defined(VC)
  JotGotoXY(0, 0);
#endif
     
  s_EditorOutput = stdout;
   
  while (Win) { // Window loop.
    char WindowKey = Win->WindowKey;
    struct Buf *TextBuf = WindowKey ? GetBuffer(WindowKey, NeverNew) : s_CurrentBuf;
    unsigned short int Top = Win->Top;                      //The topmost screen line of the image.
    unsigned short int Bot = Win->Bot+1;                    //The bottommost screen line of the image.
    unsigned short int Left = Win->Left;                    //The topmost column of the slice.
    unsigned short int Right = Win->Right;                  //The rightmost column of the slice.
    if ( ! WindowKey && (Win->LastKey != s_CurrentBuf->BufferKey) ) { //A floating window that's currently displaying some other buffer.
      struct Window *ThisWin = s_FirstWindow;
      while (ThisWin) { //Search for the current buffer displayed in other windows.
        if (ThisWin->WindowKey == s_CurrentBuf->BufferKey)
          break;
        ThisWin = ThisWin->next; }
      if (ThisWin) { //Some matching static window found - so don't display it in this window.
        Win = Win->next;
        continue; } }
    if (TextBuf && ! (Win->WindowType & (WinType_Frozen | WinType_Void))) {
      int WindowHeight = Bot-Top;                           //No. of lines in the window/slice.
      char *SeparatorString = Win->DisplayFooter ? (TextBuf->Footer ? TextBuf->Footer : (TextBuf->PathName ? TextBuf->PathName : "")) : NULL;
      short unsigned int SliceWidth = Right-Left+1;         //Width of current slice.
      int OldFirstLineNo =                                  //The first line in display when this buffer was last visited.
        (Win->WindowKey == '\0') ? TextBuf->OldFirstLineNo : Win->OldFirstLineNo;
      int FirstLineNo = OldFirstLineNo;                     //The first line in display for this visit.
      int LineNumber = TextBuf->LineNumber;                 //The line number of the current line.
      int FirstLineShift = 0;                               //Top of view shifts back by this much when lines deleted from end of buffer.
      int ThisLine;
      struct Rec *Record = TextBuf->CurrentRec;
      int LeftOffset = TextBuf->LeftOffset;
      int CurrentByte = (TextBuf->CurrentByte);
      int AllPadding = (TextBuf->Header ? 1 : 0) + (Win->DisplayFooter ? 1 : 0);
      int SubstringLength = TextBuf->SubstringLength;
      int CurrentGuard = (s_GuardBandSize*2 <= WindowHeight-AllPadding) ? s_GuardBandSize : (WindowHeight-AllPadding-1)/2;
      int CheckRange = WindowHeight-(LineNumber-OldFirstLineNo)-AllPadding-CurrentGuard;
      int TabCells = TextBuf->TabCells;
      int ChangedTabstops = 1;
      int PopupWindow = ((Win->WindowType & WinType_Mask) == WinType_Popup);
      char * OffsetText;
      
      //Some basic setup.
      s_NoUnicode = TextBuf->NoUnicode;
        
      //This logic determines which line should appear at the top line of the window and hence how much scrolling is required.
      //If we're near the end of the buffer and lines have been deleted, shift the FirstLineNo back by a few lines.
      if (WindowHeight < CheckRange) { //Large displacement - forget all about previous view and reset to a completely new one.
        //This works near the end of the buffer, anywhere else it seems to work OK anyway.
        FirstLineNo = OldFirstLineNo = LineNumber;
        CheckRange = WindowHeight-(LineNumber-OldFirstLineNo)-AllPadding-CurrentGuard; }
      ThisLine = 1;
      while ( ++ThisLine <= CheckRange )
        if (Record->next == TextBuf->FirstRec)
          FirstLineShift++;
        else
          Record = Record->next;
      if (LineNumber < OldFirstLineNo+CurrentGuard)
        FirstLineNo = LineNumber-FirstLineShift+AllPadding-CurrentGuard-1;              //New current line is above area visible in previous view - scroll down.
      else if (OldFirstLineNo+WindowHeight-AllPadding-CurrentGuard-1 < LineNumber)
        FirstLineNo = LineNumber-WindowHeight+AllPadding+CurrentGuard+1;                //New current line is below area visible in previous view - scroll up.
      else
        FirstLineNo -= FirstLineShift;                                                  //New current line remains in original view.
 
      //Plod back up to first line designated to appear at the top of the window, for popups acquire popup dimensions on the way.
      Record = TextBuf->CurrentRec;
      for (ThisLine = LineNumber; (FirstLineNo < ThisLine) && Record != TextBuf->FirstRec; ThisLine--)
        Record = Record->prev;
      FirstLineNo = ThisLine;
      
      //Scroll display while making suitable adjustments to s_TermTable.
      JotScroll(Win, FirstLineNo-OldFirstLineNo);
       
      if (PopupWindow) { //Setup popup window size.
        int PopupLineNo = 0, PopupMaxHeight = Bot-Top, LineWidthHWM = 0;
        struct Rec *PopupRec = Record;
        int OrigPopupWidth = Win->PopupWidth;
        
        do { //Line-counting and measuring loop.
          int Width = strlen(PopupRec->text);
          
          s_TermTable[SliceIndex+PopupLineNo] = NULL;  //Force re-write of each popup line.
          if (LineWidthHWM < Width)
            LineWidthHWM = Width;
          PopupRec = PopupRec->next; }
        while ( (++PopupLineNo <= PopupMaxHeight-1) && (PopupRec != TextBuf->FirstRec) );
        
        Win->PopupWidth = (Left+LineWidthHWM < Right) ? LineWidthHWM : Right-Left;
        WindowHeight = (PopupLineNo <= PopupMaxHeight) ? PopupLineNo : PopupMaxHeight;
        if ( ! Win->PopupWidth) {
          int LineNo, SliceIndex = 0;
          struct Window *ClrWin = s_FirstWindow;
          if (OrigPopupWidth) //This popup was active, reset it all now and clear all s_TermTable entries up to the popup window.
            while (ClrWin != Win) {
              for (LineNo = Win->Top; LineNo <= Win->Bot; LineNo++)
                s_TermTable[SliceIndex + LineNo] = NULL;
              ClrWin = ClrWin->next;
              SliceIndex = (ClrWin && (ClrWin->WindowType & WinType_Leftmost)) ? 0 : SliceIndex+s_TermHeight; }
          Win = Win->next;
          continue; } }
        
      if (FirstLineNo+WindowHeight < LineNumber) {
        //This happens when, for example, someones gone above the window deleted some lines and then returned to the original view - see test_14 of test_visual.
        Win->OldFirstLineNo = TextBuf->OldFirstLineNo = LineNumber;
#if defined(LINUX)
        refresh();
#endif
        if (StuckOnce)
          Win = Win->next;
        StuckOnce = TRUE;
        continue; }
      
      if (TextBuf->TabStops && TextBuf->AutoTabStops) { //Automatically-assigned TabStops.
        int EffectiveHeight = WindowHeight - (TextBuf->Header ? 1 : 0) - (Win->DisplayFooter ? 1 : 0);
        if (EffectiveHeight <= 0) {
          Fail("Insufficient window height.");
          break; }
        ChangedTabstops = AnalyzeTabs(TextBuf, Record, EffectiveHeight); }
      if (TextBuf->Header) {
        int HeaderLen = strlen(TextBuf->Header);
        char * OffsetHeader = (LeftOffset < HeaderLen) ? TextBuf->Header + LeftOffset : "";
        if (ChangedTabstops || s_TermTable[SliceIndex+Top] != OffsetHeader) { //Write header.
          WriteString(TextBuf->Header, Top, Attr_CurrChr, TRUE, HeaderLen, LeftOffset, 0, TextBuf->TabStops, TabCells, Win);
          JotClrToEndOfSlice(Top, LeftOffset, Left, Right);
          s_TermTable[SliceIndex+Top] = OffsetHeader; }
        Top++; }

      for (ThisLine = Top; ThisLine <= Top+WindowHeight-AllPadding-1; ThisLine++) { // Record update loop.
        int New = TRUE, AddTilde = FALSE;
        int EndStop, NextStop, ColourPair = 0, Attr = 0;
        int Attrs[5], AttrPoints[5], AttrIndex = 0;
        struct AnyTag *StartTag, *EndTag;
        int LocalInView;
              
        if (s_TermHeight-AllPadding-s_NewConsoleLines-1 <= ThisLine)
          break;
        if( ! Record) { //Reached end of buffer before end of window - just fill in with blank lines.
          if (s_TermTable[SliceIndex+ThisLine] || s_TermHeight-s_OldConsoleLines-1 <= ThisLine) { // Clearing unused lines loop.
            JotGotoXY(Left, ThisLine);
            JotClrToEndOfSlice(ThisLine, 0, Left, Right);
            s_TermTable[SliceIndex+ThisLine] = NULL; } 
          continue; }
        OffsetText = (LeftOffset <= strlen(Record->text)) ? Record->text + LeftOffset : "";
        EndStop = strlen(Record->text);
        
        if ( ! DisplayedCurrentByte && (Record == TextBuf->CurrentRec) && (TextBuf == s_CurrentBuf) ) { //Current line of current buffer, overlay line with cursor and substring highlights.
          int CurrentChrEndByte = CurrentByte+JotStrlenChrs(Record->text+CurrentByte, 1);
          if ( (CurrentByte == EndStop) && (CurrentByte-LeftOffset < (Win->Right - Win->Left) ) ) //There is space for the tilde.
            AddTilde = TRUE;
          AttrIndex = 0;
          if (0 < SubstringLength) { //Positive substring indicates that substring follows the current character.
            if (CurrentByte) {
              AttrPoints[AttrIndex] = CurrentByte;
              Attrs[AttrIndex++] = Attr_Normal; }
            else {
              AttrPoints[AttrIndex] = 0;
              Attrs[AttrIndex++] = Attr_CurrChr; }
            AttrPoints[AttrIndex] = CurrentChrEndByte;
            Attrs[AttrIndex++] = Attr_CurrChr;
            AttrPoints[AttrIndex] = CurrentByte+SubstringLength;
            Attrs[AttrIndex++] = Attr_SelSubStr;
            AttrPoints[AttrIndex] = EndStop;
            Attrs[AttrIndex++] = Attr_Normal; }
          else if (SubstringLength < 0) { //Negative substring indicates that current character follows the substring.
            if (CurrentByte+SubstringLength) {
              AttrPoints[AttrIndex] = CurrentByte+SubstringLength;
              Attrs[AttrIndex++] = Attr_Normal; }
            else {
              AttrPoints[AttrIndex] = 0;
              Attrs[AttrIndex++] = Attr_SelSubStr; }
            AttrPoints[AttrIndex] = CurrentByte;
            Attrs[AttrIndex++] = Attr_SelSubStr;
            AttrPoints[AttrIndex] = CurrentChrEndByte;
            Attrs[AttrIndex++] = Attr_CurrChr;
            AttrPoints[AttrIndex] = EndStop;
            Attrs[AttrIndex++] = Attr_Normal; }
          else {
            if (CurrentByte) {
              AttrPoints[AttrIndex] = CurrentByte;
              Attrs[AttrIndex++] = Attr_Normal; }
            else {
              AttrPoints[AttrIndex] = 0;
              Attrs[AttrIndex++] = Attr_CurrChr; }
            AttrPoints[AttrIndex] = CurrentChrEndByte;
            Attrs[AttrIndex++] = Attr_CurrChr;
            AttrPoints[AttrIndex] = EndStop;
            Attrs[AttrIndex++] = Attr_Normal; }
          if (s_CommandMode & CommandMode_ScreenMode) {
            s_y = ThisLine;
            s_x = JotStrlenBytes(s_CurrentBuf->CurrentRec->text, s_CurrentBuf->CurrentByte); } }
        else {
          AttrPoints[AttrIndex] = 0;
          Attrs[AttrIndex++] = 0;
          AttrPoints[AttrIndex] = EndStop;
          Attrs[AttrIndex++] = 0; }
          
        AttrIndex = 0;
        StartTag = Record->TagChain;
        EndTag = Record->TagChain;
        while (StartTag) 
          if (StartTag->type == TagType_Colour)
            break;
          else
            (StartTag = StartTag->next);
        while (EndTag) 
          if (EndTag->type == TagType_Colour)
            break;
          else
            (EndTag = EndTag->next);
        
        do { //Main WriteString() Colour-tag/Attr loop
          NextStop = EndStop;
          
          if (StartTag && (StartTag->StartPoint < NextStop) )
            NextStop = StartTag->StartPoint;
          if (EndTag && (EndTag->EndPoint < NextStop) )
            NextStop = EndTag->EndPoint+1;
          if (AttrPoints[AttrIndex] < NextStop)
            NextStop = AttrPoints[AttrIndex];
            
//s_DbgCtr++;
//fprintf(Dbg, "JotUpdateWindow : s_DbgCtr %3d, SliceIndex %2d, ThisLine %2d, OffsetText %s\n", s_DbgCtr, SliceIndex,  ThisLine, OffsetText ? OffsetText : "(NULL)"); fflush(Dbg);
          LocalInView = WriteString(Record->text, ThisLine, Attr, New, NextStop, LeftOffset, ColourPair, TextBuf->TabStops, TabCells, Win);
          if (Attr == Attr_CurrChr)
            s_InView = LocalInView;
          
          if (EndTag && (NextStop == (EndTag->EndPoint)+1) ) {
            ColourPair = 0;
            while ( (EndTag = EndTag->next) ) 
              if (EndTag->type == TagType_Colour)
                break; }
          if (StartTag && (NextStop == (StartTag->StartPoint)) ) {
            ColourPair = ((struct ColourObj *)StartTag->Attr)->ColourPair;
            while ( (StartTag = StartTag->next) ) 
              if (StartTag->type == TagType_Colour)
                break; }
          if (AttrPoints[AttrIndex] == NextStop)
            Attr = Attrs[++AttrIndex];
            
          New = FALSE; }
          while ( NextStop < EndStop );
          
        if (AddTilde) {
          JotSetAttr(Attr_CurrChr);
          JotAddBoundedString("~", 1, 1, NULL, NULL, ColourPair);
          s_ChrNo++;
          s_InView = TRUE; }
#if !defined(VC)
        attron(s_DefaultColourPair);
#endif
        JotSetAttr(Attr_Normal);
            
        JotClrToEndOfSlice(ThisLine, LeftOffset, Left, Right);
        if ( ! DisplayedCurrentByte)
          Record->DisplayFlag = Redraw_None;
        
        s_TermTable[SliceIndex+ThisLine] = OffsetText;
        Record = (Record->next == TextBuf->FirstRec) ? NULL : Record->next; }

      Win->OldFirstLineNo = FirstLineNo;
      TextBuf->OldFirstLineNo = FirstLineNo;
      
      if (SeparatorString && (s_TermTable[SliceIndex+Bot-1] != SeparatorString) ) {
        if (Bot < s_TermHeight-s_NewConsoleLines) {
          int Chr;
          int MaxNameLength = SliceWidth-10;
          int ActualLength;
          int SeparatorLen = strlen(SeparatorString);
            
          WindowKey = TextBuf->BufferKey;
          JotGotoXY(Left, Bot-1);
          JotSetAttr(Attr_WinDelim);
          if (MaxNameLength <= 0) { //The slice is too narrow for the usual separator - just put in the buffer key.
            JotGotoXY(Left + SliceWidth/2, Bot-1);
            JotAddChr(WindowKey); }
          else {
            if (SeparatorLen < MaxNameLength) { //Use full separator string.
              ActualLength = SeparatorLen;
              JotAddBoundedString(SeparatorString, ActualLength, ActualLength, NULL, NULL, 0); }
            else { //Use truncated separator string.
              int SepStart = SeparatorLen-MaxNameLength;
              ActualLength = MaxNameLength;
              JotAddBoundedString("...", 3, 3, NULL, NULL, 0);
              JotAddBoundedString(SeparatorString+SepStart, SliceWidth, MaxNameLength, NULL, NULL, 0); }
            
            for (Chr = ActualLength; Chr++ < SliceWidth; )
  #if defined(VC)
              putchar(' ');
#else
              addch(' ');
#endif
            JotGotoXY(Left + SliceWidth - 5, Bot-1);
            JotAddChr(WindowKey);
            JotSetAttr(Attr_Normal);
            s_TermTable[SliceIndex+Bot-1] = TextBuf->PathName; } } }
      
      Win->LastKey = TextBuf->BufferKey; }
      
    Win = Win->next;
    SliceIndex = (Win && (Win->WindowType & WinType_Leftmost)) ? 0 : SliceIndex+s_TermHeight; } // Window loop ends.
     
  JotGotoXY(0, s_TermHeight-1);
  JotSetAttr(Attr_Normal);
  s_OldConsoleLines = s_NewConsoleLines;
  s_DisplayedConsole = TRUE;
#if defined(LINUX)
  refresh(); 
#endif
  return DisplayedCurrentByte; }

//----------------------------------------------JotMkdir
int JotMkdir(char * PathName, int Mode)
  { //Jot system-independent replacement for mkdir.
#if defined(VC)
  return mkdir(PathName);
  chmod(PathName, Mode);
#else
  return mkdir(PathName, Mode);
#endif
  }

//----------------------------------------------UpdateJournal
void UpdateJournal(char *TextArg, char *EscapeString)
  { //Adds TextArg to journal file.
  char TempText[StringMaxChr];
  if (TextArg)
    strcpy(TempText, TextArg);
  else
    TempText[0] = '\0';
  if (EscapeString)
    strcat(TempText, EscapeString);
  strcat(TempText, "\n");
  s_JournalDataLines++;
  if ( (fputs(TempText, s_JournalHand) == EOF) || (fflush(s_JournalHand) == EOF) ) {
    RunError("Journal write failed");
    return; } }

//----------------------------------------------ReadBuffer
#if defined(VC)
//NB - The windows version has an extra parameter for the windows HANDLE in addition to FILE and Channel nos.
int ReadBuffer(FILE *File, HANDLE *FileHand, int FileDesc, int NonUnicode, char *Name, struct Buf * DestBuf, 
                       int BinaryRecSize, long long SeekOffset, long long *MaxBytes, int BlockMode, int MaxRecords)
#else
int ReadBuffer(FILE *File,                    int FileDesc, int NonUnicode, char *Name, struct Buf * DestBuf, 
                       int BinaryRecSize, long long SeekOffset, long long *MaxBytes, int BlockMode, int MaxRecords)
#endif
  { //Reads stream (from either File or FileDesc) into specified buffer if buffer already exists then buffer is scrubbed.
    //When FileDesc is set it reads from FileDesc with ReadUnbufferedStreamRec() otherwise it reads from File using ReadNewRecord()
    //With BinaryRecSize set it reads fixed-length records ignoring any '\0','\n' bytes in the stream.
    //With MaxBytes set it reads, at most, that many bytes, this is only efective when reading from FileDesc using read(), silently ignored if from File.
    //With Hold set it keeps the file handle open.
    //With nonzero MaxRecords, the no. of records read is limited to that value.
    //If both MaxBytes and Becords are set then limit is determined by whichever is satisfied first.
    //If successful, returns total no. of records read (possibly 0), if the final record has no CR then this is still counted as one record.
    //If unsuccessful, returns -1.
    //
  int ReadOK = 0, RecordsRead = 0;
  int PartFile = SeekOffset || MaxBytes;
  
  Prompt("Reading \"%s\"", Name);
  if (s_TTYMode)
    ScrollConsole(FALSE, FALSE);
             
  if (SeekOffset)
#if defined(VC)
    {
    _lseeki64(FileDesc, 0L, SEEK_SET);
    _lseeki64(FileDesc, SeekOffset, SEEK_SET);
     }
#else
    lseek64(FileDesc, SeekOffset, SEEK_SET);
#endif

  if ( ! DestBuf->IOBlock) {
    DestBuf->IOBlock = (struct IOBlock *)JotMalloc(sizeof(struct IOBlock));
    DestBuf->IOBlock->NxInteractiveBuf = NULL;
    DestBuf->IOBlock->IoFd = 0;
#if defined(VC)
    DestBuf->IOBlock->OutPipe = 0;
#else
    DestBuf->IOBlock->ChildPid = 0;
#endif
    DestBuf->IOBlock->HoldDesc = 0;
    DestBuf->IOBlock->ExitCommand = NULL;
    DestBuf->IOBlock->BucketBlock = NULL;
    DestBuf->IOBlock->WaitforSequence = NULL;
    DestBuf->IOBlock->Code = NULL; }
  if (BlockMode) { //In block mode a block of bytes, size specified by MaxBytes, is read into one record without formatting or buffering.
    long BytesRead;
    GetRecord(DestBuf, (*MaxBytes)+1, NULL);
#if defined(VC)
    if (FileHand)
      BytesRead = ReadBucketFromHandle(FileHand, DestBuf->CurrentRec->text, *MaxBytes);
    else
      BytesRead = read(FileDesc, DestBuf->CurrentRec->text, *MaxBytes);
#else
    BytesRead = read(FileDesc, DestBuf->CurrentRec->text, *MaxBytes);
#endif
    memset(DestBuf->CurrentRec->text+BytesRead, '\0', *MaxBytes-BytesRead+1);
    return BytesRead ? 1 : -1; }
            
  while ( 1 ) { //Record-read loop.
#if defined(VC)
    ReadOK = (FileDesc || FileHand) ? ReadUnbufferedStreamRec(DestBuf, FileDesc, FileHand, BinaryRecSize, MaxBytes) : ReadNewRecord(DestBuf, File, NonUnicode);
#else
    ReadOK = FileDesc ?               ReadUnbufferedStreamRec(DestBuf, FileDesc,           BinaryRecSize, MaxBytes) : ReadNewRecord(DestBuf, File, NonUnicode);
#endif
    if (s_BombOut) {
      Message(NULL, "Abandoning read \"%s\", (line %d \"%s\")", Name, s_EditorInput->LineNo, s_CommandBuf->CurrentRec->text);
      break; }
    if (ReadOK)
      RecordsRead += 1;
    else {
      break; }
    if (MaxRecords && MaxRecords <= RecordsRead) {
      RecordsRead++;
      break; } }
    
  if (MaxRecords ||  ! DestBuf->FirstRec)
    GetRecord(DestBuf, 1, "");
  DestBuf->CurrentRec = DestBuf->FirstRec;
  if ( ! PartFile) {
    if (DestBuf->PathName)
      free(DestBuf->PathName);
    DestBuf->PathName = (char *)JotMalloc(strlen(Name)+1);
    strcpy(DestBuf->PathName, Name); }
  DestBuf->NewPathName = TRUE;
  ResetBuffer(DestBuf);
  DestBuf->UnchangedStatus |= SameSinceIO;
   
  if (s_JournalHand) {
    struct stat Stat;
    char DateTimeStamp[20];
    char RecoveryPathname[StringMaxChr], Temp[StringMaxChr+60];
    char NameCopy[StringMaxChr];
    FILE *JournalHand = NULL;
    
#if defined(VC)
    strcpy(NameCopy, (Name[1] == ':' ) ? Name+2 : Name);  //Strip out device-letter prefix.
#else
    strcpy(NameCopy, (Name[0] == '.' && Name[1] == '/') ? Name+2 : Name);  //Strip out that crappy "./"path prefix.
#endif
    RecoveryPathname[0] = '\0';
    if (NameCopy[0] == '[' || euidaccess(NameCopy, W_OK) == 0) { //Reading from %E report or writable file.
      struct Rec *rec = DestBuf->FirstRec;
      struct Rec *LastRec = DestBuf->FirstRec->prev;
      int i;
      
      for (i=0; NameCopy[i]; i++) { //Remove forward slashes.
#if defined(VC)
        if (NameCopy[i] == '\\')
          NameCopy[i] = '_';
#endif
        if (NameCopy[i] == '/')
          NameCopy[i] = '_'; }
      if (NameCopy[0] == '.')
        NameCopy[0] = '_';
      
      strcpy(RecoveryPathname, s_JournalPath);
      strcat(RecoveryPathname, "/");
      strcat(RecoveryPathname, NameCopy);
      
      if (Name[0] != '[') { 
        if ( ! stat(Name, &Stat)) { //Surely the file must exist to get down here???
          strftime(DateTimeStamp, 20, "%Y%m%d_%H%M%S", localtime(&Stat.st_mtime));
          strcat(RecoveryPathname, "_");
          strcat(RecoveryPathname, DateTimeStamp); } }
      else
        sprintf(RecoveryPathname+strlen(RecoveryPathname), "_%d", s_JournalUniquifier++);
          
      if (stat(RecoveryPathname, &Stat)) { //An archive for that file name-timestamp does not already exist in the journal area.
        if ( ! (JournalHand = fopen(RecoveryPathname, "w"))) {
          RunError("Can't write to backup file %s in journal area", RecoveryPathname);
          return -1; }
        for ( ; ; ) {
          fprintf(JournalHand, "%s\n", rec->text);
          rec = rec->next;
          if (rec == LastRec)
          break; }
        fclose(JournalHand); }
      sprintf(Temp, "%%%%Recovery pathname:%4d %s, Original pathname:%4d %s", (int)strlen(RecoveryPathname), RecoveryPathname, (int)strlen(Name), Name); }
    else //Read-only files.
      sprintf(Temp, "%%%%Recovery pathname:%4ld %s, Original pathname:%4ld %s", strlen(Name), Name, strlen(Name), Name);
    UpdateJournal(Temp, NULL); }
    
  return RecordsRead; }

//----------------------------------------------ReadUnbufferedStreamRec
#if defined(VC)
int ReadUnbufferedStreamRec(struct Buf *ThisBuf, int FileDesc, HANDLE *FileHand, int BinaryRecSize, long long *MaxBytes)
#else
int ReadUnbufferedStreamRec(struct Buf *ThisBuf, int FileDesc, int BinaryRecSize, long long *MaxBytes)
#endif
  { //Reads from FileDesc (typically stdin) to a new record in nominated buffer - only called when stdin is a pipe, all other reads go via ReadNewRecord().
    //MaxBytes limits the number of bytes read to no. specified, if zero reads to end of file.
    //returns 1 if read was successful, otherwise 0.
    //
  static int Its_Binary = FALSE; 
  int RecordBytes = 0;                   //Total record length.
  int BytesCopied = 0;                   //Counts bytes copied into destination record.
  int EncodingSize;                      //No. of bytes/chr in the current unicode encoding scheme.
  int EncodingLE;                        //1 for little-endien encodings, otherwise 0.
  struct BucketBlock *ThisBucketBlock;
  
  if ( ! ThisBuf->IOBlock) { //First call - initialize.
    ThisBuf->IOBlock = (struct IOBlock *)JotMalloc(sizeof(struct IOBlock));
    ThisBuf->IOBlock->ExitCommand = NULL;
    ThisBuf->IOBlock->WaitforSequence = NULL;
    ThisBuf->IOBlock->Code = NULL;
    ThisBuf->IOBlock->BucketBlock = NULL; }
  if ( ! ThisBuf->IOBlock->BucketBlock) { //First call - initialize.
    Its_Binary = FALSE;
    ThisBuf->IOBlock->BucketBlock = (struct BucketBlock *)JotMalloc(sizeof(struct BucketBlock));
    ThisBuf->IOBlock->BucketBlock->Next = 0;
#if defined(VC)
    if (FileHand)
      ThisBuf->IOBlock->BucketBlock->BufBytes = ReadBucketFromHandle(FileHand, ThisBuf->IOBlock->BucketBlock->Bucket, (MaxBytes && *MaxBytes<BucketSize) ? *MaxBytes : BucketSize);
    else
      ThisBuf->IOBlock->BucketBlock->BufBytes = read(FileDesc, ThisBuf->IOBlock->BucketBlock->Bucket, (MaxBytes && *MaxBytes<BucketSize) ? *MaxBytes : BucketSize);
#else
    ThisBuf->IOBlock->BucketBlock->BufBytes = read(FileDesc, ThisBuf->IOBlock->BucketBlock->Bucket, (MaxBytes && *MaxBytes<BucketSize) ? *MaxBytes : BucketSize);
#endif
    if (ThisBuf->IOBlock->BucketBlock->BufBytes < 0) //A read error - probably a defunct child process driving a pty.
      ThisBuf->IOBlock->BucketBlock->BufBytes = 0;
    if (MaxBytes)
      *MaxBytes -= ThisBuf->IOBlock->BucketBlock->BufBytes;
    if (ThisBuf->IOBlock->BucketBlock->BufBytes < 0) {
      Fail("Error reading that stream");
      free(ThisBuf->IOBlock->BucketBlock);
      ThisBuf->IOBlock->BucketBlock = NULL;
      return 0; }
    ThisBuf->IOBlock->BucketBlock->RecStartByte = 0; 
    if (BinaryRecSize) {
      ThisBuf->IOBlock->BucketBlock->RecEndByte = -1;
      Its_Binary = TRUE;
      RecordBytes = 0;
      ThisBuf->FileType = BinaryRecSize; }
    else
      ThisBuf->FileType = ASCII; }
        
  if (ThisBuf->IOBlock->BucketBlock->BufBytes == 0) { //Already read an EOF from file.
    free(ThisBuf->IOBlock->BucketBlock);
    ThisBuf->IOBlock->BucketBlock = NULL;
    return 0; }
    
  ThisBucketBlock = ThisBuf->IOBlock->BucketBlock;
       
  if ( ( ! Its_Binary) && ( ! (ThisBuf->UCEncoding) ) ) { //This is *always* the first bucket - check for BOM and set UCEncoding if appropriate.
    //Also, now's a good time to remove BOM from the bucket.
    if ( (ThisBucketBlock->Bucket[0] == (char)0xEF) && (ThisBucketBlock->Bucket[1] == (char)0xBB) && (ThisBucketBlock->Bucket[2] == (char)0xBF) ) {
      ThisBuf->UCEncoding = UCEncoding_UTF_8;
      ThisBucketBlock->RecStartByte = 3; }
    else if (ThisBucketBlock->Bucket[0] == (char)0xFF && ThisBucketBlock->Bucket[1] == (char)0xFE) {
      if (ThisBucketBlock->Bucket[2] == (char)0x00 && ThisBucketBlock->Bucket[3] == (char)0x00) {
        ThisBuf->UCEncoding = UCEncoding_UTF_32LE;
        ThisBucketBlock->RecStartByte = 4; }
      else {
        ThisBuf->UCEncoding = UCEncoding_UTF_16LE;
        ThisBucketBlock->RecStartByte = 2; } }
    else if ( (ThisBucketBlock->Bucket[0] == (char)0xfe) && (ThisBucketBlock->Bucket[1] == (char)0xff) ) {
      ThisBuf->UCEncoding = UCEncoding_UTF_16BE; 
      ThisBucketBlock->RecStartByte = 2; }
    else if (ThisBucketBlock->Bucket[0] == (char)0x00 && ThisBucketBlock->Bucket[1] == (char)0x00 &&
             ThisBucketBlock->Bucket[2] == (char)0xFE && ThisBucketBlock->Bucket[3] == (char)0xFF) {
      ThisBuf->UCEncoding = UCEncoding_UTF_32BE;
      ThisBucketBlock->RecStartByte = 4; }
    else
      ThisBuf->UCEncoding = UCEncoding_Plain; }
  EncodingSize = ThisBuf->UCEncoding <= UCEncoding_UTF_8 ? 1 : (ThisBuf->UCEncoding == UCEncoding_UTF_16LE || ThisBuf->UCEncoding == UCEncoding_UTF_16BE ? 2 : 4);
  EncodingLE = ThisBuf->UCEncoding & UCEncoding_LE;
      
  while ( 1 ) { //Bucket loop - read sufficient bucketfuls (usually none) to define one record of the destination buffer.
                //Exit when MaxBytes limit reached or when it reads a valid NewLine, an EOL or an empty bucket.
    struct BucketBlock *NextBucketBlock;
    
    //Another bucketful required to complete current line?
    if (Its_Binary) {
      if ( (ThisBucketBlock->RecStartByte + BinaryRecSize - RecordBytes) <= ThisBucketBlock->BufBytes) { //A complete record in this bucket.
        ThisBucketBlock->RecEndByte += BinaryRecSize - RecordBytes;
        RecordBytes += ThisBucketBlock->RecEndByte - ThisBucketBlock->RecStartByte + 1;
        break; }
      else {
        ThisBucketBlock->RecEndByte = ThisBucketBlock->BufBytes - 1;
        RecordBytes += ThisBucketBlock->RecEndByte - ThisBucketBlock->RecStartByte + 1;
        if ( (MaxBytes && ! *MaxBytes ) || ! ThisBucketBlock->BufBytes) //Already read to limit or EOF.
          break; } }
    else { //Non-binary (text) read.
      char *Terminator;   //Position of the NewLine character or 0 (for UTF-8) or beyond the valid bucket (UTF-16/32).
      if (EncodingSize == 1)
        Terminator = memchr(ThisBucketBlock->Bucket+ThisBucketBlock->RecStartByte, '\n', ThisBucketBlock->BufBytes-ThisBucketBlock->RecStartByte);
      else {
        Terminator = ThisBucketBlock->Bucket + ThisBucketBlock->RecStartByte;
        while (Terminator < (ThisBucketBlock->Bucket + ThisBucketBlock->BufBytes))
          if (EncodingSize == 2) {
            char16_t NewLine = EncodingLE ? 0x000a : 0x0a00;
            if ( ((char16_t *)Terminator)[0] == NewLine)
              break;
            else
              Terminator += 2; }
          else { //4-byte encodings.
            char32_t NewLine = EncodingLE ? 0x0000000a : 0x0a000000;
            if ( ((char32_t *)Terminator)[0] == NewLine)
              break;
            else
              Terminator += 4; } }
      if (Terminator && Terminator < (ThisBucketBlock->Bucket + ThisBucketBlock->BufBytes)) {
         //This bucketful is the last, or contains a valid EOL marker ( \n ) - we're now ready to copy bucket(s) to a new record.
        ThisBucketBlock->RecEndByte = Terminator - ThisBucketBlock->Bucket;
        RecordBytes += ThisBucketBlock->RecEndByte-ThisBucketBlock->RecStartByte;
        break; }
      else if ( MaxBytes && ! *MaxBytes) { //Last part-record in a MaxBytes-limited read.
        RecordBytes += ThisBucketBlock->BufBytes - ThisBucketBlock->RecStartByte;
        ThisBucketBlock->RecEndByte = ThisBucketBlock->BufBytes;
        break; }
      else { //Normal incomplete record, read another bucketful.
        RecordBytes += ThisBucketBlock->BufBytes - ThisBucketBlock->RecStartByte;
        ThisBucketBlock->RecEndByte = ThisBucketBlock->BufBytes; } }
    if ( ! ThisBucketBlock->BufBytes)
      break;
      
#if defined(VC)
    if (ThisBuf->IOBlock->OutPipe && ( ! TestInteractivePipe(ThisBuf))) {
      break; }
    NextBucketBlock = (struct BucketBlock *)JotMalloc(sizeof(struct BucketBlock));
    if (FileHand)
      NextBucketBlock->BufBytes = ReadBucketFromHandle(FileHand, NextBucketBlock->Bucket, (MaxBytes && *MaxBytes<=BucketSize) ? *MaxBytes : BucketSize);
    else
      NextBucketBlock->BufBytes = read(FileDesc, NextBucketBlock->Bucket, (MaxBytes && *MaxBytes<=BucketSize) ? *MaxBytes : BucketSize);
#else
    if (ThisBuf->IOBlock->IoFd && ( ! TestInteractivePipe(ThisBuf))) {
      break; }
    NextBucketBlock = (struct BucketBlock *)JotMalloc(sizeof(struct BucketBlock));
    NextBucketBlock->BufBytes = read(FileDesc, NextBucketBlock->Bucket, (MaxBytes && *MaxBytes<=BucketSize) ? *MaxBytes : BucketSize);
#endif
    if (NextBucketBlock->BufBytes < 0) //Read error - maybe the child has exited.
      NextBucketBlock->BufBytes = 0;
    if (MaxBytes)
      *MaxBytes -= NextBucketBlock->BufBytes;
    ThisBucketBlock->Next = NextBucketBlock;
    NextBucketBlock->Next = NULL;
    NextBucketBlock->RecStartByte = 0;
    if (Its_Binary)
      NextBucketBlock->RecEndByte = 0;
    ThisBucketBlock = NextBucketBlock; }

  //At this point we have sufficient pending buckets to define the next record - if UTF-16/32 this is only temporary.
  GetRecord(ThisBuf, (Its_Binary ? (RecordBytes*3)+1 : RecordBytes+((UCEncoding_UTF_8 < ThisBuf->UCEncoding) ? 3 : 1)), NULL);
  ThisBucketBlock = ThisBuf->IOBlock->BucketBlock;
   
  while ( ThisBucketBlock ) { //Record  loop - Intermediate blocks each contain a full bucketload, for text files, the last one terminates with a {Newline}.
    struct BucketBlock *NextBucketBlock = ThisBucketBlock->Next;
    
    if (Its_Binary) {
      int i, BytesFromThisBucket = ThisBucketBlock->RecEndByte - ThisBucketBlock->RecStartByte + 1;
      for (i = 0; i < BytesFromThisBucket; i++)
        sprintf(ThisBuf->CurrentRec->text+(BytesCopied+i)*3, "%02X ", (unsigned char)(ThisBucketBlock->Bucket+ThisBucketBlock->RecStartByte)[i]);
      BytesCopied += BytesFromThisBucket; }
    else {
      int BytesFromThisBucket = ThisBucketBlock->RecEndByte-ThisBucketBlock->RecStartByte;
#if defined(VC)
      if (FileHand && (ThisBucketBlock->Bucket+ThisBucketBlock->RecStartByte)[BytesCopied+BytesFromThisBucket-1] == 0xD) {
        memcpy(ThisBuf->CurrentRec->text+BytesCopied, ThisBucketBlock->Bucket+ThisBucketBlock->RecStartByte, BytesFromThisBucket);
        memset(ThisBuf->CurrentRec->text+BytesCopied+BytesCopied+BytesFromThisBucket-1, '\0', 2); }
      else
        memcpy(ThisBuf->CurrentRec->text+BytesCopied, ThisBucketBlock->Bucket+ThisBucketBlock->RecStartByte, BytesFromThisBucket);
#else
      memcpy(ThisBuf->CurrentRec->text+BytesCopied, ThisBucketBlock->Bucket+ThisBucketBlock->RecStartByte, BytesFromThisBucket);
#endif
      BytesCopied += BytesFromThisBucket; }
      
    if (NextBucketBlock) {
      free(ThisBucketBlock);
      ThisBucketBlock = NextBucketBlock;
      if (Its_Binary) {
        ThisBucketBlock->RecEndByte = BinaryRecSize-BytesCopied - 1;
        if (ThisBucketBlock->BufBytes <= ThisBucketBlock->RecEndByte)
          ThisBucketBlock->RecEndByte = ThisBucketBlock->BufBytes - 1; } }
    else {
      ThisBucketBlock->RecStartByte = ThisBucketBlock->RecEndByte+EncodingSize;
      break; }
    ThisBuf->IOBlock->BucketBlock = ThisBucketBlock; }
  if (Its_Binary)
    memset(ThisBuf->CurrentRec->text + (BytesCopied*3)+1, '\0', (RecordBytes-BytesCopied)*3);
   
  if (UCEncoding_UTF_8 < ThisBuf->UCEncoding) { //UTF_16 and UTF_32 encodings are translated to UTF_8 here.
    int InChr = ThisBuf->CurrentByte;
    mbstate_t State;
    char *NewText, *Temp = (char *)JotMalloc(ThisBuf->CurrentRec->MaxLength * 2);
    
    memset(Temp, '\0', ThisBuf->CurrentRec->MaxLength * 2);
    memset(&State, '\0', sizeof(mbstate_t));
    RecordBytes = 0;
    if (EncodingSize == 2) {
      for (InChr = 0; InChr < (ThisBuf->CurrentRec->MaxLength-2)/2; InChr++) {
        if ( ! EncodingLE) { //Big-endien.
          char A = ((char *)(((char16_t *)ThisBuf->CurrentRec->text)+InChr))[0];
          ((char *)(((char16_t *)ThisBuf->CurrentRec->text)+InChr))[0] = ((char *)(((char16_t *)ThisBuf->CurrentRec->text)+InChr))[1];
          ((char *)(((char16_t *)ThisBuf->CurrentRec->text)+InChr))[1] = A; }
        RecordBytes += c16rtomb(Temp+RecordBytes, ((char16_t *)ThisBuf->CurrentRec->text)[InChr], &State); } }
    else { //UTF-32
      for (InChr = 0; InChr < (ThisBuf->CurrentRec->MaxLength-2)/4; InChr++) {
        if ( ! EncodingLE) { //Big-endien.
          char A = ((char *)(((char32_t *)ThisBuf->CurrentRec->text)+InChr))[0];
          char B = ((char *)(((char32_t *)ThisBuf->CurrentRec->text)+InChr))[1];
          ((char *)(((char32_t *)ThisBuf->CurrentRec->text)+InChr))[0] = ((char *)(((char32_t *)ThisBuf->CurrentRec->text)+InChr))[3];
          ((char *)(((char32_t *)ThisBuf->CurrentRec->text)+InChr))[1] = ((char *)(((char32_t *)ThisBuf->CurrentRec->text)+InChr))[2];
          ((char *)(((char32_t *)ThisBuf->CurrentRec->text)+InChr))[2] = B;
          ((char *)(((char32_t *)ThisBuf->CurrentRec->text)+InChr))[3] = A; }
        RecordBytes += c32rtomb(Temp+RecordBytes, ((char32_t *)ThisBuf->CurrentRec->text)[InChr], &State); } }
    NewText = (char *)JotMalloc(RecordBytes+1);
    strcpy(NewText, Temp);
    ThisBuf->CurrentRec->MaxLength = RecordBytes+1;
    free(ThisBuf->CurrentRec->text);
    ThisBuf->CurrentRec->text = NewText;
    free(Temp); }
      
  if ( ! Its_Binary)
    ThisBuf->CurrentRec->text[RecordBytes] = '\0';
      
  if ( (ThisBucketBlock->RecStartByte + (Its_Binary ? 1 : 0)) <= ThisBucketBlock->BufBytes )
    return 1;
  ThisBucketBlock = ThisBuf->IOBlock->BucketBlock;
  while (ThisBucketBlock) {
    struct BucketBlock *NextBucketBlock = ThisBucketBlock->Next;
    free(ThisBucketBlock);
    ThisBucketBlock = NextBucketBlock; }
  ThisBuf->IOBlock->BucketBlock = NULL;
  return 1; }

#if defined(VC)
//----------------------------------------------ReadBucketFromHandle
int ReadBucketFromHandle(HANDLE *FileHand, char *Bucket, int MaxBytes) {
  //Reads a bucketfull from a windows command, returns number of bytes read.
  unsigned long dwRead = 0, Avail = 0;
   
  if (GetFileType(FileHand) == FILE_TYPE_PIPE) {
    PeekNamedPipe(FileHand, NULL, 0, NULL, &Avail, NULL);
    if (Avail == 0)
      usleep(200000);
    PeekNamedPipe(FileHand, NULL, 0, NULL, &Avail, NULL);
    if (Avail == 0) {
      if (s_Verbose & Verbose_QuiteChatty)
        Message(NULL, "ReadBucket: Empty pipe");
      return 0; }
    ReadFile(FileHand, Bucket, MaxBytes, &dwRead, NULL);
    return (int)dwRead; }
  else if (GetFileType(FileHand) == FILE_TYPE_DISK) {
    ReadFile(FileHand, Bucket, MaxBytes, &dwRead, NULL);
    return (int)dwRead; }
  return 0; }
#endif

//----------------------------------------------TestInteractivePipe
int TestInteractivePipe(struct Buf *InteractiveBuf)
  { //Returns TRUE if the specified buffer's IoFd is ready for reading.
#if defined(VC)
  DWORD Ready = 0;
  
  PeekNamedPipe(InteractiveBuf->IOBlock->OutPipe, NULL, 0, NULL, &Ready, NULL);
  return (Ready != 0); }
#else
  struct timeval Timeout;
  fd_set InteractiveFDs;
  
  Timeout.tv_sec  = 0;
  Timeout.tv_usec = 10000;
  
  FD_ZERO(&InteractiveFDs);
  FD_SET(InteractiveBuf->IOBlock->IoFd, &InteractiveFDs);
      
  return select(s_TopInteractiveFD+1, &s_InteractiveFDs, NULL, NULL, &Timeout); }
#endif

//----------------------------------------------InteractiveReadRecs
int InteractiveReadRecs()
  { //If there are interactive reads pending then read one record from the first available stream.
    //Returns TRUE if a -waitfor condition was specified and is satistfied or if no -waitfor was specified if some records were read - otherwise returns FALSE.
    //Calls ReadUnbufferedStreamRec() to read each new record from the appropriate stream.
    //
  int RecCount = 0;
#if defined(VC)
  struct timespec TimeSpec;
  struct Buf *ThisInteractiveBuf = s_FirstInteractiveBuf;
  
  usleep(1000);
    
  while (ThisInteractiveBuf) { //This loop searches for child processes with bytes to send.
    int Timeout = ThisInteractiveBuf->IOBlock->Timeout;
    int FileDesc = ThisInteractiveBuf->IOBlock->IoFd;
    
    if (TestInteractivePipe(ThisInteractiveBuf)) { //This child has something to say.
      int OrigByte = ThisInteractiveBuf->CurrentByte;
      int OriginalLineNo = ThisInteractiveBuf->LineNumber;
            
      if (s_Interrupt) //The select() operation was interrupted by a {Ctrl+C}
        break;
        
      do { //Now read any new records from child.
        ReadUnbufferedStreamRec(ThisInteractiveBuf, 0, ThisInteractiveBuf->IOBlock->OutPipe, 0, 0);
        RecCount++; }
        while (ThisInteractiveBuf->IOBlock->BucketBlock); //Suck out all lines of text.
      AdvanceRecord(ThisInteractiveBuf, -RecCount);
      ThisInteractiveBuf->CurrentByte = OrigByte;
      ThisInteractiveBuf->LineNumber = OriginalLineNo;
      if ( ! ThisInteractiveBuf->IOBlock->WaitforSequence)
        return TRUE; }
        
    if (ThisInteractiveBuf->IOBlock->WaitforSequence) { //A -waitfor command sequence.
      if ( ! ThisInteractiveBuf->IOBlock->Code) { 
        ThisInteractiveBuf->IOBlock->Code = (struct Seq *)JotMalloc(sizeof(struct Seq));
        JOT_Sequence(ThisInteractiveBuf->IOBlock->WaitforSequence, ThisInteractiveBuf->IOBlock->Code, FALSE);
        if (s_CommandChr == ')')
          SynError(s_CommandBuf, "Surplus \')\' in OnCommand sequence"); }
      s_TraceLevel++;
      if ( ! Run_Sequence(ThisInteractiveBuf->IOBlock->Code)) { //The -waitfor sequence was successful.
        ThisInteractiveBuf->IOBlock->Timeout = 0;
        return TRUE; } }
      else
        return TRUE;
      
    if (Timeout) {
      struct _timeb tb;
      int EndTime;
      
      _ftime64_s(&tb);
      EndTime = tb.time;
      if (Timeout < (EndTime-(ThisInteractiveBuf->IOBlock->StartTime)) ) //Timeout period exceeded - exit now.
        break;
      else { //Read/Match failed within timeout period - try again.
        FD_SET(ThisInteractiveBuf->IOBlock->IoFd, &s_InteractiveFDs);
        if (s_TopInteractiveFD < FileDesc)
          s_TopInteractiveFD = FileDesc;
        continue; } }
                
    ThisInteractiveBuf = ThisInteractiveBuf->IOBlock->NxInteractiveBuf; }
    
  if (s_Interrupt) { //The select() operation was interrupted by a {Ctrl+C}
    FD_CLR(ThisInteractiveBuf->IOBlock->IoFd, &s_InteractiveFDs);
    FreeIOBlock(ThisInteractiveBuf);
    return 0; }
  
#else
  struct timespec TimeSpec;
  struct Buf *ThisInteractiveBuf = s_FirstInteractiveBuf;
    
  usleep(1000);
  
  while (ThisInteractiveBuf) { //This loop searches for child processes with bytes to send.
    int Timeout = ThisInteractiveBuf->IOBlock->Timeout;
    int FileDesc = ThisInteractiveBuf->IOBlock->IoFd;
    
    if (TestInteractivePipe(ThisInteractiveBuf)) { //This child has something to say.
      int OrigByte = ThisInteractiveBuf->CurrentByte;
      int OriginalLineNo = ThisInteractiveBuf->LineNumber;
            
      if (s_Interrupt) //The select() operation was interrupted by a {Ctrl+C}
        break;;
        
      do { //Now read any new records from child.
        ReadUnbufferedStreamRec(ThisInteractiveBuf, FileDesc, 0, 0);
        RecCount++; }
        while (ThisInteractiveBuf->IOBlock->BucketBlock); //Suck out all lines of text.
      AdvanceRecord(ThisInteractiveBuf, -RecCount);
      ThisInteractiveBuf->CurrentByte = OrigByte;
      ThisInteractiveBuf->LineNumber = OriginalLineNo;
      if ( ! ThisInteractiveBuf->IOBlock->WaitforSequence)
        return TRUE; }
        
    if (ThisInteractiveBuf->IOBlock->WaitforSequence) { //A -waitfor command sequence.
      if ( ! ThisInteractiveBuf->IOBlock->Code) { 
        ThisInteractiveBuf->IOBlock->Code = (struct Seq *)JotMalloc(sizeof(struct Seq));
        JOT_Sequence(ThisInteractiveBuf->IOBlock->WaitforSequence, ThisInteractiveBuf->IOBlock->Code, FALSE);
        if (s_CommandChr == ')')
          SynError(s_CommandBuf, "Surplus \')\' in OnCommand sequence"); }
      s_TraceLevel++;
      if ( ! Run_Sequence(ThisInteractiveBuf->IOBlock->Code)) { //The -waitfor sequence was successful.
        ThisInteractiveBuf->IOBlock->Timeout = 0;
        return TRUE; } }
      else
        return TRUE;
      
    if (Timeout) {
      clock_gettime(CLOCK_REALTIME, &TimeSpec);
      long long EndTime = (long long)(TimeSpec.tv_sec);
      if (Timeout < (EndTime-(ThisInteractiveBuf->IOBlock->StartTime)) ) //Timeout period exceeded - exit now.
        break;
      else { //Read/Match failed within timeout period - try again.
        FD_SET(ThisInteractiveBuf->IOBlock->IoFd, &s_InteractiveFDs);
        if (s_TopInteractiveFD < FileDesc)
          s_TopInteractiveFD = FileDesc;
        continue; } }
                
    ThisInteractiveBuf = ThisInteractiveBuf->IOBlock->NxInteractiveBuf; }
    
  if (s_Interrupt) { //The select() operation was interrupted by a {Ctrl+C}
    FD_CLR(ThisInteractiveBuf->IOBlock->IoFd, &s_InteractiveFDs);
    FreeIOBlock(ThisInteractiveBuf);
    return 0; }
        
#endif
  return FALSE; }

//----------------------------------------------ReadNewRecord
int ReadNewRecord(struct Buf *TextBuf, FILE *File, int NonUnicode)
  {
  //Reads one line from currently selected input channel.
  //If EndOfFile then returns 0, otherwise does a GetRecord of the
  //  right size and returns 
  //Gracefully handles oversize records.
   
  struct TempRec {
    char Bucket[BucketSize+1];
    struct TempRec *PrevRec;
    struct TempRec *NextRec; };
  struct TempRec *ThisRec = NULL;
  struct TempRec *NextRec;
  int TotalSize = 0;
  int ChrIndex = BucketSize;
  int Chr;
  int ExitFlag = FALSE;
  int FromKeyboard = s_EditorConsole && (File == s_EditorConsole->FileHandle);
  
  while ( ! ExitFlag) { //Chr read loop.
    if (BucketSize <= ChrIndex) { // Bucket full - time to extend.
      NextRec = (struct TempRec *)JotMalloc(sizeof(struct TempRec));
      if (ThisRec)
        ThisRec->NextRec = NextRec; 
      NextRec->PrevRec = ThisRec;
      ThisRec = NextRec;
      ThisRec->NextRec = NULL;
      (ThisRec->Bucket)[0] = '\0';
      ChrIndex = 0; }
       
    if (FromKeyboard) { // Reading from TTY - do rubouts and reflect to screen.
      Chr = JotGetCh(File);
      if (s_BombOut)
        break;
      switch (Chr) {
        case Control_U: //Erase all of current input line.
          while (ThisRec->PrevRec)
            ThisRec = ThisRec->PrevRec;
          while (ThisRec) { // Record-erase loop.
            NextRec = ThisRec->NextRec;
            free(ThisRec);
            ThisRec = NextRec; }
          for (ChrIndex = 0; ChrIndex++ < TotalSize; )
            JotAddBoundedString("\b \b", 3, s_TermWidth, NULL, NULL, 0);
          TotalSize = 0;
          ChrIndex = 0;
          ThisRec = (struct TempRec *)JotMalloc(sizeof(struct TempRec));
          break;
         
        case RubOut: // Erase last character from bucket. 
          if (ChrIndex == 0 && ThisRec->PrevRec) { // Delete this rec and erase chr from predecessor.
            ThisRec = ThisRec->PrevRec;
            free(ThisRec->NextRec);
            ThisRec->NextRec = NULL;
            ChrIndex = BucketSize; }
          if (0 < ChrIndex) { // Still some characters left to rub out.
            ChrIndex--;
            TotalSize--;
            JotAddBoundedString("\b \b", 3, s_TermWidth, NULL, NULL, 0); }
          break;
      
        case EOF:
        case Control_D:
          // End of File mark. 
          ExitFlag = EOF;
          break;
      
        case '\n':
        case '\r':
          // End of record. 
          ExitFlag = TRUE;
          break;
      
        default:
          { // Any other character. 
            ThisRec->Bucket[ChrIndex] = Chr;
            ChrIndex++;
            TotalSize++;
            JotAddChr(Chr); }
          break; } }
           
      else { // For anything other than TTY, use ReadString.
        if ( ! (NonUnicode ? fgets(ThisRec->Bucket, BucketSize+1, File)!=NULL : ReadString(ThisRec->Bucket, BucketSize+1, File)))
          ExitFlag = EOF;
        else { // No EOF detected - have we seen an end-of-line mark?.
          ChrIndex = strcspn(ThisRec->Bucket, "\n\r");
          if (ChrIndex < BucketSize)
            ExitFlag = TRUE;
          else
            ChrIndex = BucketSize; }
        TotalSize = TotalSize+ChrIndex; } }
    
  if (TextBuf->FirstRec && s_BombOut && ! TotalSize) {
    AdvanceRecord(TextBuf, 1);
    return 1; }
  GetRecord(TextBuf, TotalSize+1, NULL);
  ThisRec->Bucket[ChrIndex] = '\0';
  while (ThisRec->PrevRec)
    ThisRec = ThisRec->PrevRec;
  TotalSize = 0;
  while (ThisRec) { // Final-record-copy loop.
    strcpy((TextBuf->CurrentRec->text)+TotalSize, ThisRec->Bucket);
    TotalSize = TotalSize+BucketSize;
    NextRec = ThisRec->NextRec;
    free(ThisRec);
    ThisRec = NextRec; }
  if (s_JournalHand && FromKeyboard)
    UpdateJournal(TextBuf->CurrentRec->text, NULL);
  if (s_BombOut)
    AdvanceRecord(TextBuf, 1);
  return (ExitFlag == EOF) ? 0 : 1; }

//----------------------------------------------ReadString
int ReadString(char *Buffer, int BufferLength, FILE *FileHandle)
  { //Reads an entire record from command stream.
  int Chr, i = 0;
  
  while (i < BufferLength) {
    Chr = JotGetCh(FileHandle);
    if (Chr == '\n' || Chr == '\r' || Chr == EOF)
      break;
    else if (Chr == (char)EOF && ! feof(FileHandle)) //Some non-unicode negative characters in the stream - substitute '?' and continue.   
      Chr = '?';
    Buffer[i++] = Chr; }
  Buffer[i] = '\0';
  return Chr != EOF; }

//----------------------------------------------JotGetCh
#if defined(VC)
int JotGetCh(FILE *FileHandle)
  { //Replacement for curses getch() - returns next character from keyboard buffer.
    //See ../ed/wine_32/MSDKs/v7.1/Include/WinCon.h
  struct Window *Win = s_FirstWindow;
  DWORD MouseEvent;
  struct Buf *ThisBuf;
  int FirstLineNumber, FirstLineNo, BufKey;
  static int count = 0;
  static char OutBuf[100];
  static int InPtr = 0;
  static int OutPtr = 0;
  static int Button = 0, Release = FALSE, xp, yp, x, y;  //Identity of button pressed, flag indicating a release-event is pending then mouse x, y at button-press time.
  static int MouseBufKey, MouseLine, Mouse_x, Mouse_y;
  INPUT_RECORD EvBuf[100];
  DWORD EvsRead = 0;
  unsigned long long EventsPending = 0; 
  int i, Chr;
  wchar_t WChr;
  
  if (OutPtr < InPtr)
    return OutBuf[OutPtr++];
  else { //Collect and filter some more keyboard events
    OutPtr = InPtr = 0;
    if (Release) { //A button-release event is pending - send the escape-sequence now.
      s_MouseBufKey = MouseBufKey;
      s_MouseLine = MouseLine;
      s_Mouse_x = x;
      s_Mouse_y = y;
      InPtr = sprintf(OutBuf, "MB%dR", Button);
      OutPtr = 0;
      Button = 0;
      Release = FALSE;
      return ASCII_Escape; } }
    
  while (InPtr == 0) {
    if (s_ObeyStdin && FileHandle==s_EditorConsole->FileHandle)
      return fgetc(stdin); 
    else if ( ! s_EditorConsole || (FileHandle != s_EditorConsole->FileHandle) ) {
      Chr = fgetc(FileHandle);
      if (Chr == EOF)
        return EOF;
      OutBuf[OutPtr] = Chr;
      return OutBuf[OutPtr++]; }
    else
      ReadConsoleInputW(hStdin, EvBuf, 100, &EvsRead);
      
    for (i = 0; i < EvsRead; i++) {
      if (EvBuf[i].Event.KeyEvent.wRepeatCount > 0 ) 
        count = EvBuf[i].Event.KeyEvent.wRepeatCount - 1;
        
      switch (EvBuf[i].EventType) {
        case MOUSE_EVENT:
          if (s_MouseMask != 0) {
            MouseEvent = EvBuf[i].Event.MouseEvent.dwEventFlags;
            
            if (MouseEvent == 0) { //A mouse-button event.
              InPtr = OutPtr = 0;
              if (EvBuf[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
                Button = 1; //Mouse Button 1 Pressed.
              else if (EvBuf[i].Event.MouseEvent.dwButtonState == FROM_LEFT_2ND_BUTTON_PRESSED)
                Button = 2; //Mouse Button 2 Pressed.
              else if (EvBuf[i].Event.MouseEvent.dwButtonState == FROM_LEFT_3RD_BUTTON_PRESSED)
                Button = 3; //Mouse Button 3 Pressed.
              else if (Button)
                Release = TRUE; } 
                
            else if (MouseEvent == DOUBLE_CLICK) {
              if (EvBuf[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
                InPtr = sprintf(OutBuf, "MB1C"); //Mouse Button 1 double-click.
              else if (EvBuf[i].Event.MouseEvent.dwButtonState == FROM_LEFT_2ND_BUTTON_PRESSED)
                InPtr = sprintf(OutBuf, "MB2C"); //Mouse Button 2 double-click.
              else if (EvBuf[i].Event.MouseEvent.dwButtonState == FROM_LEFT_3RD_BUTTON_PRESSED)
                InPtr = sprintf(OutBuf, "MB3C"); //Mouse Button 3 Presseddouble-click.
              return ASCII_Escape; }
              
            else //Mouse movement etc. - these are of no interest.
              continue;
         
            x = EvBuf[i].Event.MouseEvent.dwMousePosition.X;
            y = EvBuf[i].Event.MouseEvent.dwMousePosition.Y;
              
            if (Release) { //The release event has arrived.
              if (xp == x && yp == y) { //No change in mouse position - count it as a click.
                InPtr = sprintf(OutBuf, "MB%dc", Button);
                Button = 0;
                Release = FALSE; }
              else { //Mouse moved - it's a mouse-drag event - first send the button-press details.
                InPtr = sprintf(OutBuf, "MB%dP", Button); }
              s_MouseBufKey = MouseBufKey;
              s_MouseLine = MouseLine;
              s_Mouse_x = xp;
              s_Mouse_y = yp;
              OutPtr = 0; }
            else if (Button) { //A button-press.
              xp = x;
              yp = y; }
            
            while (Win != NULL) { // Window loop - search windows for mouse-event x, y.
              if (Win->Top <= s_Mouse_y && s_Mouse_y <= Win->Bot && Win->Left <= x && x < Win->Right) {
                BufKey = (Win->WindowKey == '\0') ? s_CurrentBuf->BufferKey : Win->WindowKey;
                
                ThisBuf = GetBuffer(BufKey, NeverNew);
                FirstLineNo = ThisBuf->OldFirstLineNo;
                if (Win->Top <= s_Mouse_y && s_Mouse_y < Win->Bot) {
                  MouseBufKey = BufKey;
                  MouseLine = FirstLineNo + y - Win->Top;
                  s_Mouse_x = x - (((Win->WindowType & WinType_Mask) == WinType_Popup) ? (Win->Left == 0 ? 0 : Win->Right-Win->PopupWidth) : Win->Left);
                  Mouse_y = y; } }
              Win = Win->next; }
              
            if (InPtr)
              return ASCII_Escape;
            else
              continue; }
            
        case KEY_EVENT:
          Chr = EvBuf[i].Event.KeyEvent.wVirtualKeyCode; 
          if ( ! (EvBuf[i].Event.KeyEvent.bKeyDown) || (0x10 <= Chr && Chr <= 0x12) )
            continue;
             
          else if (Chr == '\0')
            continue;
            
          else if ( (0x21 <= Chr && Chr <= 0x2F) || (0x5F <= Chr && Chr <= 0xB8) || (EvBuf[i].Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) ) {
            if (Chr == 0x57 || Chr == 0x58)  // fix F11 and F12
              Chr = Chr - 0x12;
            InPtr = sprintf(OutBuf, "\x1BX%03X",  ( Chr | 0x100 | 
              (EvBuf[i].Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)      << 5  |
              (EvBuf[i].Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED)  << 7  |
              (EvBuf[i].Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED) << 8  |
              (EvBuf[i].Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED)   << 10 |
              (EvBuf[i].Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED)  << 11)); }
             
          else //Normal typewriter keys returning normal ascii codes.
            InPtr = WideCharToMultiByte(CP_UTF8, 0, &EvBuf[i].Event.KeyEvent.uChar.UnicodeChar, 1, OutBuf, 100,  NULL, NULL);
                 
        default:  //Ignore all other (mouse-button, focus, resize or popup) events
          continue; } } }
          
  return OutBuf[OutPtr++]; }

#else
int JotGetCh(FILE *FileHandle)
  { //Replacement for curses getch() - returns next character from keyboard buffer.
  static char Result[100];
  static int InPtr = 0, OutPtr = 0;
  MEVENT event;
  int Chr;
  wchar_t WChr;
  
  if (OutPtr < InPtr)
    return Result[OutPtr++];
  InPtr = OutPtr = 0;
  
  while ( 1 ) {
    if (s_EditorConsole && (FileHandle == s_EditorConsole->FileHandle) ) //Reading from console.
      Chr = (s_TTYMode || s_ObeyStdin) ? fgetc(stdin) : getch();
    else if (s_asConsole && (FileHandle == s_asConsole->FileHandle) ) { //Reading from an -asConsole file.
      Chr = fgetc(FileHandle);
      if (Chr == EOF) 
        return Chr; }
    else { //Reading from a file
      WChr = fgetwc(FileHandle);
      if (WChr == WEOF)
        return (int)EOF;
      else if ((((int)WChr) & 255) == 0xC2)  //UCS-BOM prefix???
        continue;
      InPtr = wctomb(Result, WChr);
      return Result[OutPtr++]; }
    if (s_BombOut)
      return '\0';
    if (s_Interrupt) { //There's been a {Ctrl+C} but s_BombOut was not set - that means we need to insert a {Ctrl+C} in the command stream.
      s_Interrupt = FALSE;
      Chr = '\3'; }
    if (Chr == ERR) //An error here usually means that there were no characters ready for reading.
      continue;
    else if (Chr < KEY_MIN)
      return Chr;
    else if (Chr == KEY_BACKSPACE || Chr == RubOut)
      return RubOut;
    else if (Chr == KEY_MOUSE) {
      if (getmouse(&event) == OK) {
        struct Window *Win = s_FirstWindow, *ClickWin = NULL;
        int x = event.x;
        
        s_Mouse_y = event.y;
        while (Win != NULL) { // Window loop.
          if (Win->Top <= s_Mouse_y && s_Mouse_y <= Win->Bot && Win->Left <= x && x < Win->Right) {
            int FirstLineNo = Win->OldFirstLineNo;
            int BufKey = (Win->WindowKey == '\0') ? s_CurrentBuf->BufferKey : Win->WindowKey;
            s_Mouse_x = x - (((Win->WindowType & WinType_Mask) == WinType_Popup) ? (Win->Left == 0 ? 0 : Win->Right-Win->PopupWidth) : Win->Left);
            s_MouseBufKey = BufKey;
            s_MouseLine = FirstLineNo + s_Mouse_y - Win->Top;
            ClickWin = Win; }
          Win = Win->next; }
        if ( ! ClickWin)
          continue;
        InPtr = sprintf(Result, "%cM%04X", ASCII_Escape, (unsigned int)event.bstate); }
      else
        continue; }
    else if (Chr < 0x023D)
      InPtr = sprintf(Result, "%cX%04X", ASCII_Escape, Chr);
    else //Assume it's unicode.
      return Chr;
    if (InPtr < 0)
      continue;
    return Result[OutPtr++]; } }
#endif

#if defined(VC)
//----------------------------------------------OpenWindowsCommand
int OpenWindowsCommand(HANDLE *InPipe, HANDLE *OutPipe, HANDLE *ProcessId, int StreamInBuf, int Interactive, char *Command)
  { //In windows, performs a function similar to popen() in unix.
    //For interactive usage InPipe is the command stream otherwise InPipe is NULL.
  PROCESS_INFORMATION piProcInfo; 
  STARTUPINFO siStartInfo;
  int BytesSent;
  DWORD avail = 0;
  static HANDLE s_hChildStd_IN_Rd = NULL;
  static HANDLE s_hChildStd_IN_Wr = NULL;
  static HANDLE s_hChildStd_OUT_Rd = NULL;
  static HANDLE s_hChildStd_OUT_Wr = NULL;
  static SECURITY_ATTRIBUTES s_saAttr; 
  
  // Set the bInheritHandle flag so pipe handles are inherited. 
  s_saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
  s_saAttr.bInheritHandle = TRUE; 
  s_saAttr.lpSecurityDescriptor = NULL; 
  
  CreatePipe(&s_hChildStd_OUT_Rd, &s_hChildStd_OUT_Wr, &s_saAttr, 0);
  SetHandleInformation(s_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);
  if (StreamInBuf || InPipe) { //Create a pipe to push the current-buffers text down the child's stdin.
    CreatePipe(&s_hChildStd_IN_Rd, &s_hChildStd_IN_Wr, &s_saAttr, 0);
    SetHandleInformation(s_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0); }
  
  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION) );
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO) );
  siStartInfo.cb = sizeof(STARTUPINFO); 
  siStartInfo.hStdError = s_hChildStd_OUT_Wr;
  siStartInfo.hStdOutput = s_hChildStd_OUT_Wr;
  siStartInfo.hStdInput = s_hChildStd_IN_Rd;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
  
  if ( ! CreateProcess(NULL, Command, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo)) {
    Fail("CreateProcess failed");
    return 1; }
  else {
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread); }
 
  if (StreamInBuf) { //Copy current-buffer text to child's stdin.
    struct Rec *ThisRec = s_CurrentBuf->FirstRec;
     
    for ( ; ; ) { 
      if ( ! WriteFile(s_hChildStd_IN_Wr, ThisRec->text, strlen(ThisRec->text)-1, &BytesSent, NULL))
        Fail("Failed to write to child's stdin pipe");
      ThisRec = ThisRec->next;
      if (ThisRec == s_CurrentBuf->FirstRec)
        break;
      WriteFile(s_hChildStd_IN_Wr, "\n", 1, &BytesSent, NULL); }
    CloseHandle(s_hChildStd_IN_Wr);
       
    while (avail == 0) {
      if (s_BombOut)
        break;
      PeekNamedPipe(s_hChildStd_OUT_Rd, NULL, 0, NULL, &avail, NULL); } }
  s_BombOut = FALSE;
  
  if (ProcessId)
    *ProcessId = piProcInfo.hProcess;
  
  if (OutPipe)
    *OutPipe = s_hChildStd_OUT_Rd;
    
  if (InPipe)
    *InPipe = s_hChildStd_IN_Wr;
    
  return 0; }

#else
//----------------------------------------------SigwinchHandler
void SigwinchHandler(int signal) {
  //Propagate window size changes from input file descriptor to master side of pty.
  PropagateSigwinch = 1; }

//----------------------------------------------DoublePopen
int DoublePopen(int *InPipe, int *OutPipe, pid_t *ChildPid, struct Buf *TextBuf, char *Commands)
  { //Similar to popen execpt that it can read it's stdin and write to it's stdout, popen can only do one or the other.
  int PipeIn[2];
  int PipeOut[2];
  pid_t child;
  char *ArgV[100];
  char *Command = strtok(Commands, " ");
  int i = 1;
  int LocalFail = FALSE;
  
  ArgV[0] = Command; 
  do 
    ArgV[i] = strtok(NULL, " ");
    while (ArgV[i++]);
         
  if ((pipe(PipeIn)) == -1)
    return RunError("pipe");
  if ((pipe(PipeOut)) == -1)
    return RunError("pipe");
  
  child = fork();
  
  if (child == 0) { // This is child! 
    if ((dup2(PipeIn[0], STDIN_FILENO)) == -1)
      return RunError("dup2 stdin");
    if ((dup2(PipeOut[1], STDOUT_FILENO)) == -1)
        return RunError("dup2 stdout");
    if ((dup2(PipeOut[1], STDERR_FILENO)) == -1)
        return RunError("dup2 stderr");
    if ((close(PipeIn[0])) == -1)
        return RunError("close");
    if ((close(PipeOut[1])) == -1)
        return RunError("close");
    if ((close(PipeIn[1])) == -1)
      return RunError("close");
    if ((close(PipeOut[0])) == -1)
      return RunError("close");
    if ((execvp(Command, ArgV)) == -1)
      return RunError("execvp"); }
           
  else if (child == -1)
    return RunError("fork");
       
  else { // This is parent!
    if (ChildPid)
      *ChildPid = child;
    if ((close(PipeIn[0])) == -1)
        return RunError("close");
    if ((close(PipeOut[1])) == -1)
      return RunError("close");
      
    if (TextBuf) {
      struct Rec *ThisRec = TextBuf->FirstRec;
       
      do { //Record loop - copy TextBuf's text to child's stdin.
        if ((write(PipeIn[1], ThisRec->text, strlen(ThisRec->text))) == -1 || (write(PipeIn[1], "\n", 1) == -1))
          return RunError("write 1");
        ThisRec = ThisRec->next; }
        while (ThisRec != TextBuf->FirstRec); }
    
    if (OutPipe)
      *OutPipe = PipeOut[0];
    else if ((close(PipeOut[0])) == -1)
      RunError("close");
      
    if (InPipe)
      *InPipe = PipeIn[1];
    else if ((close(PipeIn[1])) == -1)
      return RunError("close"); }
      
  return FALSE; }
#endif

//----------------------------------------------BinFind
char *BinFind(char *String, int Lim, char *SubString, int SubLim) {
  //Finds substring in string and returns byte offset or -1, works with strings containing '\0' characters.
  char *i;
  int j;
  
  for (i = String; i < String+Lim-SubLim; i++) {
    for (j = 0; j < SubLim; j++) {
      if (i[j] != SubString[j])
        goto Next_i; }
      return i;
Next_i: continue; }
  return NULL; }

//----------------------------------------------ReadCommand
char ReadCommand(struct Buf *DestBuf, FILE *FileHandle, char *FormatString, ...)
  { // Reads command from TTY prompting as specified by Prompt arguments into
    // current record of TextBuf.
    //Text goes in starting at current character position.
    //The terminating character is not copied in.
    //Returns EOF or 1 if error detected, otherwise 0.
  va_list ap;
  char string[StringMaxChr];
  int Chr;
  int ByteCount = 0;                                                    //Counts characters typed into DestBuf
  int Untranslated = FALSE;                                             //A non-ascii character string was not in the translations table - it's probably unicode.
  char EscapeString[EscapeSequenceLength+1];                            //Escape string derived from curses key code.
  int ScreenMode = (s_CommandMode & CommandMode_ScreenMode);
  char *CommandText = DestBuf->CurrentRec->text;
  
  if (ScreenMode && s_CurrentBuf->EditLock & ReadOnly) {
    Message(NULL, "Buffer is readonly - exiting insert mode.");
    ScreenMode = s_CommandMode = 0; }
  va_start(ap, FormatString);
  vsnprintf(string, StringMaxChr, FormatString, ap);
  va_end(ap); 
  
  if (s_ModifiedCommandString) { //The command string has been defined by %s=commandstring <command> 
    ByteCount = strlen(s_ModifiedCommandString);
    strcpy(DestBuf->CurrentRec->text, s_ModifiedCommandString);
    DestBuf->CurrentByte = ByteCount;
    free(s_ModifiedCommandString);
    s_ModifiedCommandString = NULL;
    CommandPrompt("%s%s", string, DestBuf->CurrentRec->text); }
  else {
    if ( ! ScreenMode) {
      int LeftOffset = s_CurrentBuf->LeftOffset;
      JotGotoXY(-LeftOffset, s_TermHeight-1);
      CommandPrompt(string); }
    if (s_TTYMode)
      fflush(s_EditorOutput);
    DestBuf->CurrentRec = DestBuf->CurrentRec->next;
    DestBuf->FirstRec = DestBuf->CurrentRec->next;
    DestBuf->CurrentByte = 0; }

  for ( ; ; ) { // Main character loop.
    int RecordLength = DestBuf->CurrentRec->MaxLength;
    CommandText = DestBuf->CurrentRec->text;
    Chr = JotGetCh(FileHandle);
    if (feof(s_EditorInput->FileHandle))
      return EOF;
    if (s_BombOut)
      return CTRL_C_INTERRUPT;
      
    if (s_CommandMode & CommandMode_EscapeAll) {
      int TransStatus;
      CommandText[ByteCount] = '\0';
      TransStatus = TransEscapeSequence(DestBuf, FileHandle, Chr, EscapeString);
      return TransStatus; }
      
    switch ((Chr) ) {
    
    case EOF: //We've already checked for a genuine EOF - this is a wide-character translation error.
      if (s_EditorInput->asConsole) {
        RunError("Wide-character translation error in -asConsole script %s", s_EditorInput->FileName);
        return EOF; }

    case RubOut:
      { //First calculate no. of bytes to be removed.
      if (ScreenMode) {
        int LeftOffset = s_CurrentBuf->LeftOffset;
        int ChrByteCount = JotBytesInChEndr(CommandText, ByteCount-1);
        
        s_CurrentBuf->UnchangedStatus = 0;
        if (0 < ByteCount) { //Easy case - delete characters from CommandBuf.
          JotDeleteChr(s_x = ByteCount+(s_CurrentBuf->TabStops ? GetChrNo(s_CurrentBuf)+1 : s_CurrentBuf->CurrentByte)-LeftOffset-1, s_y);
          ByteCount -= ChrByteCount; }
        else { //Delete character from current buffer.
          if (s_CurrentBuf->CurrentByte <= 0) { //No more characters before cursor in current record first join with predecessor record.
            AdvanceRecord(s_CurrentBuf, -1);
            JoinRecords(s_CurrentBuf); }
          else { //Delete main-buffer characters to the left of the cursor.
            int ChrByteCount = JotBytesInChEndr(s_CurrentBuf->CurrentRec->text, s_CurrentBuf->CurrentByte-1);
            int ChrPtr = s_CurrentBuf->CurrentByte -= ChrByteCount;
            s_CurrentBuf->SubstringLength = ChrByteCount;
            SubstituteString(s_CurrentBuf, NULL, -1);
            JotDeleteChr(s_x = ChrPtr - LeftOffset, s_y); }
          if (s_JournalHand)
            UpdateJournal(NULL, "<<DEL>>"); } }
      else if (ByteCount) {
        int ChrByteCount = JotBytesInChEndr(CommandText, ByteCount-1);
        
        ByteCount -= ChrByteCount;
        CommandText[ByteCount] = '\0';
#if defined(VC)
        printf("%s", "\b \b");
#else
        JotAddBoundedString("\b \b", 3, s_TermWidth, NULL, NULL, 0);
#endif
        }
#if defined(LINUX)
      refresh();
#endif
      break; }

    default:
      if ( (Chr == ASCII_Escape) || (s_CommandMode & CommandMode_EscapeAll) ) { //Initiate an escape-sequence search.
        int TransStatus;
        CommandText[ByteCount] = '\0';
        //N.B. TransEscapeSequence() will read more characters directly to DestBuf->CurrentRec->text (i.e. CommandText).
        TransStatus = TransEscapeSequence(DestBuf, FileHandle, Chr, EscapeString);
        if (TransStatus == UNTRANSLATED_CHR) {
          if (Untranslated && 0 < mblen(CommandText, MB_CUR_MAX)) {
            Untranslated = FALSE;
            Prompt("%s", CommandText); }
          else {
            Untranslated = TRUE; }
          if (RecordLength <= ++ByteCount)
//          if (RecordLength < ByteCount++)
            return 1;
          continue; }
        return TransStatus; }
      
#if defined(VC)
      if ( (32 <= Chr || Chr < 0 || Chr == '\t') && Chr < 127 ) //An ordinary printable ASCII character a TAB, or a unicode byte.
#else
      if ( 32 <= Chr || Chr < 0 || Chr == '\t') //An ordinary printable ASCII character a TAB, or a unicode byte.
#endif
        {
        if (ScreenMode) {
          s_CurrentBuf->UnchangedStatus = 0;
          s_x = ByteCount+(s_CurrentBuf->TabStops ? GetChrNo(s_CurrentBuf)-1 : s_CurrentBuf->CurrentByte)-s_CurrentBuf->LeftOffset;
          if (s_CommandMode & CommandMode_OverTypeMode) {
            JotDeleteChr(s_x, s_y); }
          JotGotoXY(s_x-2, s_y);
          JotInsertChr(Chr);
          JotGotoXY(0, s_TermHeight-1); }
        else if( ! s_TTYMode)
#if defined(VC)
          JotAddChr(Chr);
#else
          {
          int x, y;
          getyx(mainWin, y, x);
          if (s_TermWidth-1 <= x) {
            move(y, 1);
            delch();
            move(y, x-1);
            refresh(); }
          JotAddChr(Chr); }
        refresh();
#endif
        fflush(s_EditorOutput);
        CommandText[ByteCount] = Chr;
        if (RecordLength <= ++ByteCount) { //Abandon this command line, but first suck out any further characters that might screw things up later.
          while (Chr = JotGetCh(FileHandle))
            if (Chr == '\n')
              break;
          return 1; }
        break; }

    case '\n':
    case '\r':
      //End of command line detected.
      if (ScreenMode) { //There are typed-into-screen characters insert them to s_CurrentBuf then exit - deal with the escape sequence next time around.
        int Overtype = s_CommandMode & CommandMode_OverTypeMode;
        s_CurrentBuf->SubstringLength = Overtype ? ByteCount : 0;
        SubstituteString(s_CurrentBuf, CommandText, ByteCount);
        s_CurrentBuf->CurrentByte += ByteCount;
        s_CurrentBuf->LineNumber--;
        s_CurrentBuf->SubstringLength = 0;
        BreakRecord(s_CurrentBuf);
        JotUpdateWindow();
        if (s_JournalHand) {
          CommandText[ByteCount] = '\0';
          UpdateJournal(CommandText, "<<BRK>>"); }
        ByteCount = 0; }
      else {
        CommandText[ByteCount] = '\0';
        if (s_JournalHand) {
          UpdateJournal(CommandText, NULL); }
        DestBuf->SubstringLength = 0;
        DestBuf->CurrentByte = 0;
        return Untranslated; } } } }

//----------------------------------------------TransEscapeSequence
int TransEscapeSequence(struct Buf *DestBuf, FILE *FileHandle, signed char Chr, char *EscapeString)
  { //Translates the escape string using '^' buffer translations.
    //Either an escape sequence follows or a control character was hit.
    //If translation exists for the escape sequence then translate,
    //otherwise copy in sequence minus escape character.
    //
  int ChrPointer = strlen(DestBuf->CurrentRec->text);
  int asConsole = s_EditorInput->asConsole;
  int ScreenMode = (s_CommandMode & CommandMode_ScreenMode);
  char *CommandText = DestBuf->CurrentRec->text;
  struct Buf *TranslationBuf;
  int Parameter = FALSE;          //TRUE if translated string contains ## - response to prompt follows escape string.
  char *TransText;                //Translated command text from ^ buffer.
  char ThisChr;
  int TransFailed = FALSE;
     
  if (s_CommandMode & CommandMode_ToggleScreen)
    s_CommandMode ^= (CommandMode_ToggleScreen | CommandMode_ScreenMode);
  if (ChrPointer && ScreenMode) { //There are typed-into-screen characters insert them to s_CurrentBuf then deal with the escape sequence.
    if (s_CommandMode & CommandMode_OverTypeMode) {
      int RightSubstringLen = strlen(s_CurrentBuf->CurrentRec->text) - s_CurrentBuf->CurrentByte;
      s_CurrentBuf->SubstringLength = RightSubstringLen < strlen(CommandText) ? RightSubstringLen : strlen(CommandText); }
    else
      s_CurrentBuf->SubstringLength = 0;
    SubstituteString(s_CurrentBuf, CommandText, ChrPointer);
    s_CurrentBuf->CurrentByte += ChrPointer;
    s_CurrentBuf->SubstringLength = 0;
    if (s_JournalHand)
      UpdateJournal(CommandText, "<<INS>>");
    CommandText[0] = '\0'; }
  TranslationBuf = GetBuffer('^', NeverNew);
  if ( ! TranslationBuf)
    return FALSE;
     
  //Some translations to try - read characters one at a time and try matching with translations buffer.
  ResetBuffer(TranslationBuf);
  memset(EscapeString, '\0', EscapeSequenceLength+1);
  EscapeString[0] = Chr;

  for ( ; ; ) { // Translation-search record loop.
    int SearchIndex = 0;            //The character index used for character-by-character comparisons of the escape sequence and current translations record.
    int ModifyCount = 0;            //Counts extra characters removed for Ctrl+<chr> descriptions in key translations table, eg: ^x means {Ctrl+x}
    
    for ( ; SearchIndex+ModifyCount < EscapeSequenceLength; )  { // Escape-sequence character loop.
      ThisChr = *((TranslationBuf->CurrentRec->text)+SearchIndex+ModifyCount);
      if (ThisChr == '^')
        ThisChr = *((TranslationBuf->CurrentRec->text)+SearchIndex+(++ModifyCount)) - 0x40;
      else if (ThisChr == '*')
        ThisChr = *((TranslationBuf->CurrentRec->text)+SearchIndex+(++ModifyCount)) + 0x80;
      else if (ThisChr == '&')
        ThisChr = *((TranslationBuf->CurrentRec->text)+SearchIndex+(++ModifyCount)) + 0x40;
      if (ThisChr == ' ')
        EscapeString[SearchIndex] = ' ';
      else if (EscapeString[SearchIndex] == 0)
        EscapeString[SearchIndex] = JotGetCh(FileHandle);
      if (EscapeString[SearchIndex++] != ThisChr)
        break; }
    
    if (EscapeSequenceLength <= SearchIndex+ModifyCount)
      break;
    if ( (TransFailed = (AdvanceRecord(TranslationBuf, 1))) )
      break; }
    
  if (TransFailed) { // No match.
    unsigned char Temp[2] = {Chr, '\0'};
     
    DestBuf->SubstringLength = 0;
    DestBuf->CurrentByte = ChrPointer;
    SubstituteString(DestBuf, (char *)Temp, -1);
    if (s_JournalHand) {
      fputs("<Invalid Escape sequence>\n", s_JournalHand);
      fflush(s_JournalHand); }
    return UNTRANSLATED_CHR; }
  else { // Translations Match found.
    int Index;
    int PromptStart = 0;
    int PromptEnd = 0;
    
    DestBuf->SubstringLength = 0;
    TransText = TranslationBuf->CurrentRec->text;
    TranslationBuf->SubstringLength = strlen(TransText)-EscapeSequenceLength;
    
    for (Index = EscapeSequenceLength; Index <= strlen(TransText); Index++) { // Parameter-flag search loop.
      if (TransText[Index] == '#' && TransText[Index+1] == '#') { // Parameter tag '##' found.
        char PromptString[StringMaxChr];
        PromptString[0] = '\0';
        PromptStart = Index;
        PromptEnd = Index+2;
        if (TransText[Index+2] == '\"') { //Prompt string follows.
          char *Terminator = strchr(TransText+Index+3, '\"');
          if ( ! Terminator) {
            SynError(NULL, "No terminating \" found for prompt string");
            break; }
          PromptEnd = Terminator - TransText+1;
          strncpy(PromptString, TransText+PromptStart+3, PromptEnd-PromptStart-4);
          PromptString[PromptEnd-PromptStart-4] = '\0'; }
        if (ScreenMode && PromptString[0]) { // Prompt for parameters.
          JotGotoXY(0, s_TermHeight-1);
          Prompt(PromptString);
          Chr = '\0';
              
          while (Chr != '\r') { // Parameter character read loop.
            Chr = JotGetCh(FileHandle);
            if (s_BombOut)
              break;
            Prompt("%c", Chr);
            if (Chr == RubOut) {
              if (ChrPointer == 0)
                continue;
              JotAddBoundedString("\b \b", 3, s_TermWidth, NULL, NULL, 0);
              ChrPointer--; }
            else if (Chr ==  '\r' || Chr == '\n') {
              DestBuf->CurrentByte = ChrPointer;
              CommandText[ChrPointer] = '\0';
              break; }
            else
              CommandText[ChrPointer++] = Chr; } }
        break; } }
    
    if (s_JournalHand) {
      EscapeString[EscapeSequenceLength] = '\0';
      UpdateJournal(CommandText, EscapeString); }
    Parameter = TRUE;
    TranslationBuf->SubstringLength = Index-EscapeSequenceLength;
    if (s_JournalHand && ! Parameter) {
      EscapeString[EscapeSequenceLength] = '\0';
      UpdateJournal(CommandText, EscapeString); }
    int ArgLength = strlen(CommandText);
    if (PromptStart) { //This translation has an argument and, maybe, a prompt string.
      memmove(CommandText+PromptStart-EscapeSequenceLength, CommandText, ArgLength);
      memcpy(CommandText, TransText+EscapeSequenceLength, PromptStart-EscapeSequenceLength);         //Parameter prefix
      strcpy(CommandText+PromptStart-EscapeSequenceLength+ArgLength, TransText+PromptEnd); }         //Parameter Suffix
    else { //No arg - just copy in the translated command string.
      strcpy(CommandText, TransText+EscapeSequenceLength); } }
  
  DestBuf->SubstringLength = 0;
  DestBuf->CurrentByte = 0;
  if (asConsole && s_RecoveryMode) //Suck out any trailing whitespace and the '\n' from the recovery file.
    while ( (Chr = JotGetCh(FileHandle) == ' ') )
      { }
  return asConsole ? TransFailed : FALSE; }

//----------------------------------------------Ctrl_C_Interrupt
#if defined(VC)
BOOL Ctrl_C_Interrupt( DWORD fdwCtrlType ) 
#else
void Ctrl_C_Interrupt(int sig)
#endif
  { //Ctrl+C handler.
  if (s_OnIntCommandBuf) {
    int Status;
    struct Seq *OnIntSequence = NULL;
    s_CommandBuf->CurrentByte = 0;
    OnIntSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
    JOT_Sequence(s_OnIntCommandBuf, OnIntSequence, FALSE);
    if (s_CommandChr == ')')
      SynError(s_CommandBuf, "Surplus \')\' in OnInt sequence");
    s_TraceLevel++;
    Status = Run_Sequence(OnIntSequence);
    s_TraceLevel--;
    if (Status && ! s_BombOut)
      RunError("Command-sequence failed.");
    FreeSequence(&OnIntSequence); }
  s_Interrupt = TRUE;
  if (s_CommandMode & CommandMode_Pass_Ctrl_C) { //Ignore {Ctrl+C}, pass it on like any other character.
    return FALSE; }
#if defined(VC)
  if (fdwCtrlType != CTRL_C_EVENT) // Handle the CTRL-C signal. 
    return TRUE;
  if (s_BombOut) {
    Message(NULL, "Unanswered Ctrl+C interrupt");
    return FALSE; }
#else
  if (sig != SIGINT)
    RunError("Unknown signal detected"); 
  sigrelse(sig);
  if (s_BombOut) {
    Message(NULL, "Unanswered Ctrl+C interrupt");
    if (s_RestartPoint)
      longjmp(*s_RestartPoint, 1);
    else
      Disaster("Restart point not set"); }
#endif
  s_CommandMode = 0;
  s_Verbose |= 1;
  if (s_JournalHand) {
    char Temp[100];
    sprintf(Temp, "<<INT %lld %d>>", s_CommandCounter, s_JournalDataLines);
    UpdateJournal(NULL, Temp); }
  if (s_Verbose & Verbose_ReportCounter)
    RunError("Interrupt detected - command count is %lld", s_CommandCounter);
  if (s_TraceMode & Trace_Interrupt) {
    s_TraceMode = s_DefaultTraceMode;
    if (s_DebugLevel) {
      s_DebugLevel = 0;
      RunError("Exiting debugger now.");
#if defined(VC)
      return FALSE;
#else
      return;
#endif
      }
    JotTrace("Ctrl_C");
    JotBreak("Ctrl+C interrupt"); }
  else
    RunError("Interrupt detected");
#if defined(VC)
  return TRUE;
#else
  refresh();
  return;
#endif
  }

//----------------------------------------------ExpandEnv
void ExpandEnv(char *Full, int MaxLength)
  { // Recursive, expands any environmental variables at start of name. 
    //Max length of expanded string given by MaxLength
  char Temp[StringMaxChr];
  char *EnvEnd, *Trans, *Ptr;
  int TransLen;
  
  while ( (Ptr = strstr(Full, "${")) ) { 
    if ( ! (EnvEnd = strstr(Ptr, "}"))) {
      RunError("Invalid env reference in \"%s\"", Ptr);
      return; }
    EnvEnd[0] = '\0';
    if ( ! (Trans = getenv(Ptr+2))) {
      RunError("Invalid env variable name \"%s\"", Temp);
      Temp[0] = '\0';
      return; }
    if (MaxLength < strlen(Full)+strlen(Trans)-(EnvEnd-Ptr)-1) {
      RunError("Translation of env %s is too long", Temp);
      return; }
    TransLen = strlen(Trans);
    memmove(Ptr+TransLen-1, EnvEnd, strlen(EnvEnd+1)+2);
    memcpy(Ptr, Trans, TransLen);
#if defined(VC)
    { 
      char * BackSlash;
      while (BackSlash = strchr(Ptr, '\\') )
        BackSlash[0] = '/'; }
#endif
  return; } }

//----------------------------------------------IsAFile
int IsAFile(char *PathName)
  { //Returns TRUE if the pathname points to a file in the filing system, FALSE if it doesn't exist or is a directory.
  struct stat Stat;
   
  if ( ! stat(PathName, &Stat))
    //Object exists, but is it a directory?
    return (Stat.st_mode & S_IFMT) != S_IFDIR;
  return FALSE; }

//----------------------------------------------CrossmatchPathnames
int CrossmatchPathnames(int *FileDesc, FILE **File, char AccessMode[], char *PathName, char *Default, char *Fallback, char *Actual)
  { //PathName and Default are given, sets Actual to the pathname actually used - all simple BCPL strings.
    //Normally *FileDesc or **File will be specified by the calling function, if neither then it exits after setting Actual.
    //Returns 0 if a valid pathname was found and, if requested, the file was successfully opened.
    //Returns -1 in all other cases.
  struct stat Stat;
  int FileExists = FALSE, Pass;
  char *Path = NULL, *Name = NULL, *Extn = NULL, *TempPath = NULL, *TempName = NULL, *TempExtn = NULL;
  char PathNameCopy[StringMaxChr], DefaultCopy[StringMaxChr], FallbackCopy[StringMaxChr];
  char *Orig[] = {PathName, Default, Fallback};
  char *Copy[] = {PathNameCopy, DefaultCopy, FallbackCopy};
  
  if ( ! Actual) 
    Actual = "";
  if ( ! Actual[0] ) { //When no destination is specified, skip pathname matching and just use the PathName as-is.
    for (Pass = 0; Pass <= 2; Pass++) {
      if (Orig[Pass]) {
        char *Sep;
        if(StringMaxChr <= strlen(Orig[Pass])) {
          RunError("Pathname too long.");
          return -1; }
        strcpy(Copy[Pass], Orig[Pass]);
        ExpandEnv(Copy[Pass], StringMaxChr);
        
        if ( (TempName = strrchr(Copy[Pass], '/')) ) {
          Copy[Pass][++TempName-Copy[Pass]-1] = '\0';
          if ( ! TempName[0] )
            TempName = NULL;
          TempPath = Copy[Pass]; }
        else { //If no path separators ( / ) then assume it's just a file name.
          TempPath = NULL;
          TempName = (0 < strlen(Copy[Pass])) ? Copy[Pass] : NULL; }
        if (TempName && (TempExtn = strrchr(TempName, '.'))) {
          TempName[++TempExtn-TempName-1] = '\0';
          if ( ! TempName[0] )
            TempName = NULL; }
        if ( ! TempPath)
          if ( (Sep = strrchr(Copy[Pass], '/')) ) {
            Copy[Pass][Sep-Copy[Pass]] = '\0';
            TempPath = Copy[Pass]; }
            
        Actual[0] = '\0';
        if (TempPath) {
          if (Path) {
            strcpy(Actual, TempPath);
            strcat(Actual, "/");
            strcat(Actual, Path); }
          else
            strcpy(Actual, TempPath); }
        else if (Path)
          strcpy(Actual, Path);
          
        if ( ( ! Name) && Extn) { //Before crossmatching with new name, try the new path with the old name extension (hidden files look like an extension with no name).
          int OriginalExtent = strlen(Actual);
          strcat(Actual, "/.");
          strcat(Actual, Extn);
          if ( (FileExists = ( ! stat(Actual, &Stat))))
            break;
          Actual[OriginalExtent] = '\0'; }
        if ( ! Name && TempName)
          Name = TempName;
        if ( ! Extn && TempExtn)
          Extn = TempExtn;
          
        if ( ! Pass)
          Path = TempPath;
        if (Actual[0])
          strcat(Actual, "/"); }
          
      if (Name)
        strcat(Actual, Name);
      if (Extn) {
        strcat(Actual, ".");
        strcat(Actual, Extn); }
      if ( Actual[0] && ((AccessMode[0] != 'r') || (FileExists = ( ! stat(Actual, &Stat) && (Stat.st_mode & S_IFMT) != S_IFDIR))))
        break; } }
     
  if ( ! FileDesc && ! File)
    return FileExists;
  if ( (AccessMode[0] != 'r') || IsAFile(Actual) ) {
    if(FileDesc)
      return ! (*FileDesc = open(Actual, (AccessMode[0] == 'r') ? O_RDONLY : O_WRONLY));
    else
      return ! (*File = fopen(Actual, AccessMode)); }
  else
    return -1; }

//---------------------------------------------NewCommandFile
int NewCommandFile(FILE *File, char *Name, struct Com *CurrentCommand)
  { //Sets up CommandFile structure for this input file,
    //does not use GetBuffer() because it has no BufferKey field.
  struct CommandFile *NewFile;
  struct Buf *NewBuf;
     
  if (File == NULL)
    return FALSE;
  NewFile = (struct CommandFile *)JotMalloc(sizeof (struct CommandFile));
  NewFile->FileHandle = File;
  NewFile->ReturnFile = s_EditorInput;
  NewFile->OldCommandBuf = s_CommandBuf;
  NewFile->FileName = (Name == NULL) ? NULL : strcpy((char *)JotMalloc(strlen(Name)+1), Name);
  NewFile->LineNo = 0;
  NewFile->asConsole = FALSE;
  NewFile->Pred = s_EditorInput;
  NewBuf = (struct Buf *)JotMalloc(sizeof(struct Buf));
  NewBuf->BufferKey = 'c';
  NewBuf->UnchangedStatus = 0;
  NewBuf->CurrentRec = NULL;
  NewBuf->Predecessor = CurrentCommand;
  NewBuf->FrameCount = 0;
  NewFile->CommandBuf = NewBuf;
  GetRecord(NewBuf, StringMaxChr, NULL);
  memset(NewBuf->CurrentRec->text, '\0', StringMaxChr);
  NewBuf->SubstringLength = 0;
  NewBuf->LineNumber = 1;
  s_EditorInput = NewFile;
  return TRUE; }

//---------------------------------------------FreeCommandFile
int FreeCommandFile()
  { //Frees CommandFile structure for current input file.
  struct CommandFile *OldComFile;
  int ReturnValue;
  
  if ( ! s_EditorInput)
    return FALSE;
  ReturnValue = (s_EditorInput->ReturnFile) != NULL;
  fclose(s_EditorInput->FileHandle);
  s_CommandBuf = s_EditorInput->OldCommandBuf;
  OldComFile = s_EditorInput->ReturnFile;
  s_EditorInput->CommandBuf->CurrentRec = s_EditorInput->CommandBuf->FirstRec;
  while ( 1 ) {
    struct Rec *NextRec = s_EditorInput->CommandBuf->CurrentRec->next;
    free(s_EditorInput->CommandBuf->CurrentRec->text);
    free(s_EditorInput->CommandBuf->CurrentRec);
    s_EditorInput->CommandBuf->CurrentRec = NextRec;
    if (s_EditorInput->CommandBuf->CurrentRec == s_EditorInput->CommandBuf->FirstRec)
      break; }
  free(s_EditorInput->CommandBuf);
  if (s_EditorInput->FileName)
    free(s_EditorInput->FileName);
  if (s_EditorInput->asConsole) //It can happen that the original session crashed in a G0 operation - then the G0 will swollow the end of the recovery script, leaving s_RecoveryMode set.
    s_RecoveryMode = 0;
  free(s_EditorInput);
  s_EditorInput = OldComFile;
  return ReturnValue; }

//---------------------------------------------OpenJournalFile
int OpenJournalFile(int *FileDesc, FILE **File, char * Actual)
  { // In recovery mode, reads replacement-file details from the history file and opens the file,
    //If writing the journal, copies the file to the journal dir and writes the pathname to the journal.
  char Buf[StringMaxChr], *RecoveryPathName, *OrigName;
  int RecNameLen, OrigNameLen;
  fgets(Buf, StringMaxChr, s_asConsole->FileHandle);
  s_asConsole->LineNo++;
  if ( ! strstr(Buf, "%%Recovery pathname:") )
    Disaster("Failed to find \"%%%%Recovery pathname: ...\" line, instead found line \"%s\" in history file", Buf);
  RecoveryPathName = Buf+25;
  sscanf(Buf+20, "%4d", &RecNameLen);
  RecoveryPathName[RecNameLen] = '\0';
  OrigName = RecoveryPathName+RecNameLen+25;
  sscanf(RecoveryPathName+RecNameLen+20, "%4d", &OrigNameLen);
  OrigName[OrigNameLen] = '\0';
  if (OrigNameLen && ( ! strstr(Actual, OrigName) || strlen(Actual) != OrigNameLen) )
    Disaster("At line %d of recovery script \"%s\": recovery file-name (%s) does not match original (%s)", 
      s_asConsole->LineNo, s_asConsole->FileName, Actual, OrigName);
  CrossmatchPathnames(FileDesc, File, "r", NULL, NULL, NULL, RecoveryPathName);
  return 0; }

//---------------------------------------------SwitchToComFile
int SwitchToComFile(char *NameString, int asConsole, char *Args, struct Com *CurrentCommand)
  { 
  //
  //  Parses and checks file name substituting undefined bits with default
  //  startup file name (defined by s_DefaultComFileName or s_DefaultComFileName,
  //  then searches the users default directory for the specified file
  //  if the file is not there then searches the ${JOT_HOME}/coms directory.
  //
  FILE *File = NULL;
  int Status = 0;
  char Actual[StringMaxChr];
  struct Buf *ArgsBuf;
  int IsDollar = s_CurrentBuf->BufferKey == '$';
  
  if (s_JournalPath) {
    if (s_RecoveryMode) {
      if (OpenJournalFile(NULL, &File, NameString))
        return 1; }
    else if (s_JournalHand) {
      FILE *CopyHand, *OrigHand;
      char ArchivePath[StringMaxChr];
      struct stat Stat;
      char DateTimeStamp[20];
      char *NameExtnElem = strrchr(NameString, '/');
      char Temp[(2*StringMaxChr)+100];
      
      if ( ! stat(NameString, &Stat)) {
        strftime(DateTimeStamp, 20, "%Y%m%d_%H%M%S", localtime(&Stat.st_mtime));
        sprintf(Temp, "%s_%s", NameExtnElem ? NameExtnElem+1 : NameString, DateTimeStamp);
        strcpy(ArchivePath, s_JournalPath);
        strcat(ArchivePath, "/");
        strcat(ArchivePath, Temp);
        if ( ! IsAFile(ArchivePath)) { //journal archive of this version does not exist, create one now.
          CrossmatchPathnames(0, &CopyHand, "w", NULL, NULL, NULL, ArchivePath);
          if ( ! CopyHand)
            Disaster("Failed to open journal file %s", ArchivePath);
          else {
            if ( ! (OrigHand = fopen(NameString, "r")))
              RunError("Failed to open file %s for copying to journal archive", NameString);
            else {
              while (fgets(Temp, StringMaxChr, OrigHand))
                fputs(Temp, CopyHand);
              fclose(OrigHand); }
            fclose(CopyHand); } }
        sprintf(Temp, "%%%%Recovery pathname:%4ld %s, Original pathname:%4ld %s", strlen(ArchivePath), ArchivePath, strlen(NameString), NameString); }
      else { //Nonexistent file - make sure there's always going to be a nonexistent file at recovery time.
        strcpy(ArchivePath, s_JournalPath);
        strcat(ArchivePath, "/nonexistent");
        sprintf(Temp, "%%%%Recovery pathname:%4ld %s/nonexistent, Original pathname:%4ld %s", strlen(ArchivePath)+12, ArchivePath, strlen(NameString), NameString); }
      UpdateJournal(Temp, NULL); } }
  
  Actual[0] = '\0';
  CrossmatchPathnames(0, &File, "r", NameString, s_DefaultComFileName, NULL, Actual);
  if ( ! File)
    return Fail("Can't find jot command script.");
    
  if ( ! (ArgsBuf = GetBuffer('$', AlwaysNew)))
    return Fail("Can't copy args to $ buffer.");
  if (IsDollar)
    s_CurrentBuf = ArgsBuf;
  GetRecord(ArgsBuf, strlen(Args)+1, Args);
     
  if (s_Verbose & Verbose_NonSilent)
    Message(NULL, "Running command file \"%s\"", Actual);
  NewCommandFile(File, Actual, CurrentCommand);
  s_EditorInput->asConsole = asConsole; 
  if (asConsole) {
    if (s_asConsole) {
      Fail("Attempt to nest -asConsole scripts"); 
      return 1; }
    s_asConsole = s_EditorInput; }
  s_CommandBuf =  asConsole ? s_EditorConsole->CommandBuf : s_EditorInput->CommandBuf;
  s_BacktraceFrame->EditorInput = s_EditorInput;
  
  if (asConsole) { //-asConsole scripts exit on Ctrl+C
    if (InteractiveEditor())
    Status = 0; }
  else {
    for (;;) { // Command-file-record loop. 
      struct Seq *ComFileSequence = NULL;
      
#if defined(LINUX)
      if (s_FirstInteractiveBuf) { //Set up to check for I/O-ready interactive subprocesses.
        struct Buf **ThisInteractiveBufPtr = &s_FirstInteractiveBuf;
        s_TopInteractiveFD = 0;
        while (*ThisInteractiveBufPtr) { //This loop checks for dead children.
          FD_SET((*ThisInteractiveBufPtr)->IOBlock->IoFd, &s_InteractiveFDs);
          if (s_TopInteractiveFD < (*ThisInteractiveBufPtr)->IOBlock->IoFd)
            s_TopInteractiveFD = (*ThisInteractiveBufPtr)->IOBlock->IoFd;
          ThisInteractiveBufPtr = &((*ThisInteractiveBufPtr)->IOBlock->NxInteractiveBuf); } }
#endif
            
      s_EditorInput->LineNo++;
      if ( ! ReadString(s_CommandBuf->CurrentRec->text, s_CommandBuf->CurrentRec->MaxLength, s_EditorInput->FileHandle))
        break;
      if (s_BombOut) {
        if (s_Verbose & Verbose_Prompt)
          Message(NULL, "Exiting script \"%s\", line %d \"%s\"", Actual, s_EditorInput->LineNo, s_CommandBuf->CurrentRec->text);
        break; }
         
      s_CommandBuf->CurrentByte = 0;
      
      ComFileSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
      JOT_Sequence(s_CommandBuf, ComFileSequence, FALSE);
      s_TraceLevel++;
      s_Return = FALSE;
      Status = Run_Sequence(ComFileSequence);
      s_TraceLevel--;
      if (Status && ! s_BombOut) {
        Fail("Command-file failure, line %d of \"%s\"", s_EditorInput->LineNo, Actual);
        FreeSequence(&ComFileSequence);
        break; }
      FreeSequence(&ComFileSequence); } }
          
  if (asConsole)
    s_asConsole = NULL;
  if ( ! FreeCommandFile())
    return 1;
  return Status; }

//----------------------------------------------JotDeleteChr
void JotDeleteChr(int x, int y)
  { //Replacement for curses delch() - deletes the character below the cursor
#if defined(VC)
  const SMALL_RECT start = { (short)x, (short)y, (short)s_TermWidth-1, (short)y };
  const COORD      dest = { x-1, y };
  const CHAR_INFO  fill = { { ' ' }, Attr_Normal };
   
  JotGotoXY(x, y);
  ScrollConsoleScreenBuffer(hStdout, &start, NULL, dest, &fill);
#elif defined(LINUX)
  JotGotoXY(x, y);
  delch();
#endif
  }

//----------------------------------------------JotStrlenBytes
int JotStrlenBytes(char *String, int Bytes)
  { //Returns the number of characters for string limited to Bytes bytes or -1 for invalid unicode.
  int Chrs = 0, ByteIndex = 0;
  
  if ( ! Bytes)
    return 0;
  while (String[ByteIndex]) {
    if ( ((String[ByteIndex] & (unsigned char)0xc0) != (unsigned char)0x80) )
      Chrs++;
    if (Bytes <= ++ByteIndex)
      break; }
      
  return Chrs; }

//----------------------------------------------JotStrlenChrs
int JotStrlenChrs(char *String, int Chrs)
  { //Returns the number of bytes for string limited to Chrs characters or -1 for invalid unicode.
  int ByteIndex = 0;
  
  if ( ! Chrs)
    return 0;
  while ( String[ByteIndex] )
    if ( ((String[++ByteIndex] & (unsigned char)0xc0) != (unsigned char)0x80) && ! (--Chrs) )
      break;
      
  return ByteIndex; }

//----------------------------------------------JotRecLenChrs
int JotRecLenChrs(struct Buf * TextBuf, int Chrs)
  { //Returns the number of bytes for current record of buffer limited to Chrs characters.
    //If Chrs is negative, returns bytes for characters before the current byte.
  int ByteIndex = 0;
  int EndStop = TextBuf->CurrentByte;
  char * String = TextBuf->CurrentRec->text + TextBuf->CurrentByte;
  unsigned char Mask = TextBuf->NoUnicode ? 0x00 : 0xc0;
  
  if (0 <= Chrs) {
    while ( String[ByteIndex] )
      if ( ((String[++ByteIndex] & Mask) != (unsigned char)0x80) && ! (--Chrs) )
        break; }
  else {
    while ( 0 < (EndStop + ByteIndex) )
      if ( ((String[--ByteIndex] & Mask) != (unsigned char)0x80) && ! (++Chrs) )
        break; }
      
  return Chrs ? 0 : ByteIndex; }

//----------------------------------------------JotBytesInChEndr
int JotBytesInChEndr(char * String, int LastByte)
  { //Returns no. of bytes in chr indicated by LastByte - this is designed for working backwards and LastByte points to the last byte of a multibyte chr.
    //Also returns 1 if ChrNo points to the string terminating zero.
  int ByteCount = 0;
#if defined(VC)
  int CodePage = s_CurrentBuf->CodePage;
#endif
  
  if (0 < String[LastByte])
    return 1;
#if defined(VC)
  MultiByteToWideChar(CodePage, MB_ERR_INVALID_CHARS, String+LastByte-(++ByteCount), ByteCount, NULL, 0);
#else
  while (mblen(String+LastByte-(++ByteCount), MB_CUR_MAX) < 0)
    { }
#endif
  return ByteCount+1; }

//----------------------------------------------JotInsertChr
void JotInsertChr(char Chr)
  { //Replacement for curses insch() - inserts one character in (s_x, s_y) location, pushes characters rightwards rather than overwriting rest of line.
#if defined(VC)
  SMALL_RECT start = { (short)s_x, (short)s_y, (short)s_TermWidth-1, (short)s_y };
  COORD dest = { s_x+1, s_y };
  CHAR_INFO fill = { { ' ' }, Attr_Normal };
   
  JotGotoXY(s_x, s_y);
  ScrollConsoleScreenBuffer(hStdout, &start, NULL, dest, &fill);
  if (JotAddChr(Chr))
    s_x++;
  else {
    SMALL_RECT NewStart = { (short)s_x+1, (short)s_y, (short)s_TermWidth-1, (short)s_y };
    COORD NewDest = { s_x, s_y };
    ScrollConsoleScreenBuffer(hStdout, &NewStart, NULL, NewDest, &fill); }
#elif defined(LINUX)
  JotGotoXY(s_x, s_y);
  insch(' ');
  if (JotAddChr(Chr)) {
    JotGotoXY(s_x, s_y);
    s_x++; }
  else
    delch();
#endif
  }

//----------------------------------------------JotAddChr
int JotAddChr(signed char Chr)
  { //Replacement for curses addch() - adds a character in screen at current cursor position, overwriting any previous character.
     //Returns 1 when a complete character has been added.
  static char UTF_8_Chr[10];  
  static int NextByteOfUTF_8 = 0;
  int CurrentByteSave = s_CurrentBuf->CurrentByte;
  int Chrs = 1;
#if defined(VC)
  const CHAR_INFO  fill = { { ' ' }, Attr_Normal };
  PCONSOLE_SCREEN_BUFFER_INFO Info;
#endif
  
  if (Chr < 0) {
    if (s_NoUnicode)
      Chr = '~';
    else {
      int Chrs;
      UTF_8_Chr[NextByteOfUTF_8++] = Chr;
      UTF_8_Chr[NextByteOfUTF_8] = '\0';
      JotAddBoundedString(UTF_8_Chr, NextByteOfUTF_8, 1, NULL, &Chrs, 0);
      if (1 <= Chrs) {
        NextByteOfUTF_8 = 0;
        return 1; }
      else
        return 0; }
      return 1; }
  else if (Chr < ' ')
    Chr = '~';
#if defined(VC)
  printf("%c", Chr);
#else
  Chrs = addch(Chr) ? 0 : 1;
  refresh();
#endif
  s_CurrentBuf->CurrentByte = CurrentByteSave;
  return Chrs; }

//----------------------------------------------JotAddBoundedString
void JotAddBoundedString(char *OrigString, int MaxBytes, int MaxChrs, int *ByteCount, int *ChrCount, int ColourPair)
  { //Writes OrigString to screen, string is bounded by MaxBytes and/or MaxChrs, sets ByteCount (bytes consumed) and ChrCount (screen-columns occupied).
  int i;
#if defined(VC)
  WCHAR WideBuf[StringMaxChr];
  int ThisByteChrLen, FirstByte = 0;
  int LocalByteCount = 0, LocalChrCount = 0, Chrs;
  int CodePage = s_CurrentBuf->CodePage;
#endif
  if ( s_TTYMode) { //Just use fputc to write the string to stdout.
    int i;
    for (i = 0; i<MaxBytes && i<MaxChrs; i++)
      fputc(OrigString[i], stdout);
    fflush(stdout);
    if (ChrCount)
      *ChrCount = i;
    if (ByteCount)
      *ByteCount = i;
    return; }
#if defined(VC)
  if ( ! ByteCount)
    ByteCount = &LocalByteCount;
  if ( ! ChrCount)
    ChrCount = &LocalChrCount;
  *ChrCount = 0;
  if (MaxBytes <= 0 || MaxChrs <= 0) {
    *ByteCount = 0;
    return; }
  *ByteCount = ((MaxBytes < MaxChrs) ? MaxBytes : MaxChrs)-1; //Start-point for MaxChrs search.
  
  while (*ChrCount < MaxChrs && *ByteCount < MaxBytes)
    //Plod through trying every string length up to MaxBytes, until we find one meeting the required MaxChrs character count.
    *ChrCount = MultiByteToWideChar(CodePage, MB_ERR_INVALID_CHARS, OrigString, ++(*ByteCount), NULL, 0);
    
  SetConsoleOutputCP(CodePage);
  Chrs = MultiByteToWideChar(28591, MB_ERR_INVALID_CHARS, OrigString, *ByteCount, WideBuf, StringMaxChr-1);
  WideBuf[Chrs] = L'\0';
  if (ColourPair)
    SetConsoleTextAttribute(hStdout, ColourPair);
  for (i = 0; i < *ChrCount; i++)
    if (WideBuf[i] < 32)
      WideBuf[i] = L'~';
  wprintf(L"%s", WideBuf);

#elif defined(LINUX)
  int StringLen = strlen(OrigString);
  int LocalChrCount, LocalByteCount;
  int StringSize = StringMaxChr+1;
  mbstate_t ShiftState;
  wchar_t WideString[StringSize];
  const char *Temp = OrigString;
  char *EndChr;
  
  if ( ! ByteCount)
    ByteCount = &LocalByteCount;
  if ( ! ChrCount)
    ChrCount = &LocalChrCount;
  *ChrCount = 0;
  *ByteCount = 0;
  if (StringLen == 0 || MaxBytes <= 0 || MaxChrs <= 0)
    return;
      
  if (s_NoUnicode) { //Display any non-ASCII bytes as tildes ( ~ ).
    char String[strlen(OrigString)+1];
    
    for (*ByteCount = 0; *ByteCount < MaxBytes && *ByteCount < MaxChrs; ) {
      signed char Chr = OrigString[*ByteCount];
      String[(*ByteCount)++] = (Chr < '\b') ? '~' : Chr; }
      
    String[*ByteCount] = '\0';
    *ChrCount = *ByteCount;
    addstr(String); }
  else {
    memset(&ShiftState, '\0', sizeof(mbstate_t));
    wmemset(&WideString[0], L'\0', StringSize);
    EndChr = OrigString+MaxBytes;
    
    while ( 1 ) {
      if (MaxChrs <= *ChrCount || !Temp || MaxBytes <= Temp-OrigString)
        break;
      int ChrsThisPass = mbsnrtowcs(WideString+*ChrCount, &Temp, EndChr-Temp, MaxChrs-*ChrCount, &ShiftState);
      if (0 <= ChrsThisPass)
        *ChrCount += ChrsThisPass;
      else {
        *ChrCount += wcslen(WideString+*ChrCount);
        mbsnrtowcs(WideString+*ChrCount, &Temp, 1, MaxBytes, &ShiftState);
        (*ChrCount)++; } }
    if (ColourPair)
      attron(COLOR_PAIR(ColourPair));
    else
      attron(s_DefaultColourPair);
      
    for (i = 0; i < *ChrCount; i++)
      if (WideString[i] < 32)
        WideString[i] = L'~';
    addwstr(WideString);
    *ByteCount = Temp ? Temp-OrigString : 0; }
#endif
 }

//----------------------------------------------JotClrToEndOfSlice
void JotClrToEndOfSlice(int TermLine, int LeftOffset, int Left, int Right)
  { //Starting from current x position, clears to end of slice.
#if defined(VC)
  int x, y, Chrs;
  
  COORD StartXY;
  DWORD ChrsWritten;
   
  if ( (s_ChrNo-LeftOffset < Left) || (Right < s_ChrNo-LeftOffset) )   //This is required for test 127
    return;
    
  StartXY.X = s_ChrNo-LeftOffset;
  StartXY.Y = TermLine;
  Chrs = Right-s_ChrNo+LeftOffset+1;
  FillConsoleOutputCharacter(hStdout, ' ', Chrs, StartXY, &ChrsWritten);
  FillConsoleOutputAttribute(hStdout, Attr_Normal, Chrs, StartXY, &ChrsWritten);
#elif defined(LINUX)
    
  while (s_ChrNo-LeftOffset <= Right) {
    addch(' ');
    s_ChrNo++; }
#endif
  }

//----------------------------------------------JotClearToEOL
void JotClearToEOL()
  { //Replacement for curses clrtoeol().
#if defined(VC)
  int x, y;
  COORD StartXY;
  DWORD Chrs, ChrsWritten;
  CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
   
  GetConsoleScreenBufferInfo(hStdout, &csbiInfo); 
  StartXY.X = csbiInfo.dwCursorPosition.X;
  StartXY.Y = csbiInfo.dwCursorPosition.Y;
  Chrs = s_TermWidth - StartXY.X;
  FillConsoleOutputAttribute(hStdout, Attr_Normal, Chrs, StartXY, &ChrsWritten);
  FillConsoleOutputCharacter(hStdout, ' ', Chrs, StartXY, &ChrsWritten);
#elif defined(LINUX)
  clrtoeol();
#endif
  }

//----------------------------------------------JotSetAttr
void JotSetAttr(int DisplayAttrs)
  { //Replacement for curses attrset() - sets forground/background colour/boldness/highligting attributes.
  int Physical_Attr = s_Attrs[DisplayAttrs];
#if defined(VC)
  SetConsoleTextAttribute(hStdout, Physical_Attr);
#elif defined(LINUX)
  attrset(Physical_Attr);
#endif
  }

//----------------------------------------------ScrollConsole
void ScrollConsole(int CommandPrompt, int OverideQuiet)
  { // Performs scroll-up in console area, also allows for temporary expansion of console area up to s_MaxConsoleLines.
  int OneLineConsole = s_TermHeight-s_ConsoleTop <= 1;
  if ( (s_Verbose & Verbose_NonSilent) || OverideQuiet) {
    if (s_TTYMode)
      fputs("\r\n", stdout);
    else if (s_MaxConsoleLines == 0) {
      JotGotoXY(0, s_TermHeight-1);
      JotScroll(NULL, 1); }
    else {
      JotGotoXY(0, s_TermHeight-1);
      if (OneLineConsole && CommandPrompt && ! s_NewConsoleLines )    //Don't scroll command prompts for single-line consoles.
        JotClearToEOL(); 
      else {
        if (s_NewConsoleLines+1 < s_TermHeight-s_ConsoleTop)          //Console has space for at least one more line.
          s_NewConsoleLines++;
        else {
          if (s_NewConsoleLines < s_MaxConsoleLines) {                //We're OK to borrow one more window line.
            s_TermTable[s_TermHeight-s_NewConsoleLines-2] = NULL;
            s_NewConsoleLines++; } }
        JotScroll(NULL, 1); }
      if (s_DisplayedConsole && CommandPrompt) {                      //The text in borrowed console lines has been displayed.
        s_DisplayedConsole = FALSE;
        s_NewConsoleLines = 0; } } } }

//----------------------------------------------JotScroll
void JotScroll(struct Window * Win, int Offset)
  { //defines the scroll region (Top-to-Bot) and scrolls up (+ve) Lines or down (-ve) by Offset lines.
  int Line, Top, Bot;
#if defined(VC)
  int Left, Right; 
#endif
  
  if (Win) {
    Top = Win->Top;
    Bot = Win->Bot-1; }
  else { //Scrolling the console area.
    Top = (s_NewConsoleLines < s_TermHeight-s_ConsoleTop-1) ? s_ConsoleTop : s_TermHeight-s_NewConsoleLines-1;
    Bot = s_TermHeight-1; }  //It doesn't seem to mind that the first call to JotScroll() gives Bot set to -1 ???
   
#if defined(VC)
  if (Win) {
    Left = Win->Left;
    Right = Win->Right; }
  else {
    Left = 0;
    Right = s_TermWidth-1; }
  if (Bot < Top) //This happens when some genius has defined a one-line window with a delimiter line.
    return;
if (s_JournalHand) { //For debugging only.
  char Temp[StringMaxChr];
  sprintf(Temp, "Scrolling: Offset %d", Offset); 
  UpdateJournal(Temp, NULL); }
  if (0 < Offset) { //Scrolling up.
    const SMALL_RECT StartXY = { (short)Left, (short)Top+Offset, (short)Right, (short)Bot };
    const COORD dest = { (short)Left, Top };
    const CHAR_INFO fill = { { ' ' }, Attr_Normal };
    ScrollConsoleScreenBuffer(hStdout, &StartXY, NULL, dest, &fill); }
  else if (Offset < 0) { //Scrolling down.
    const SMALL_RECT StartXY = { (short)Left, (short)Top, (short)Right, (short)Bot+Offset };
    const COORD dest = { (short)Left, Top-Offset };
    const CHAR_INFO fill = { { ' ' }, Attr_Normal };
    ScrollConsoleScreenBuffer(hStdout, &StartXY, NULL, dest, &fill); }
     
#elif defined(LINUX)
  if ( ! Offset || (Win && (Win->WindowType & WinType_Mask) != WinType_Normal) ) { //For slices, doing nothing here will force JotUpdateWindow() to just plod down redrawing each part line in the slice.
    return; }
  setscrreg(Top, Bot);
  scrollok(mainWin, TRUE);
  //scrl(Offset);
  //scrl() messes up sometimes - e.g: release/v1.6.2/bin/lin64/jot l99.t -in="%w; %w 9; %w 0; zai/ol0(o# %m=abc'~; o~)30 ok/z.'a"
  //Console-area should display abc1 to abc29
  if (Top == Bot)
    JotClearToEOL();
  if (0 < Offset) //Scrolling up.
    for (Line = 1; (Line <= Offset) && (Line <= Bot-Top); Line++)
      scrl(1);
  else //Scrolling down.
    for (Line = 1; (Line <= -Offset) && (Line < Bot-Top); Line++)
      scrl(-1);
  scrollok(mainWin, FALSE);
  refresh();
#endif
  
  if ( ! Win)
    return;
 
  if (0 < Offset) //Scrolling up.
    for (Line = Top; Line <= Bot; Line++)
      s_TermTable[Line] = (Line+Offset <= Bot) ? s_TermTable[Line+Offset] : NULL;
  else //Scrolling down.
    for (Line = Bot; Top <= Line; Line--)
      s_TermTable[Line] = (Line+Offset < Top) ? NULL : s_TermTable[Line+Offset]; }

//----------------------------------------------JotGotoXY
void JotGotoXY(int x, int y)
  { //Replacement for curses move(), TC gotoxy() etc - moves cursor to specified coordinate.
#if defined(VC)
  const COORD pos = { (short)x, (short)y };
   
  if (s_TTYMode)
    return;
  SetConsoleCursorPosition(hStdout, pos);
#elif defined(LINUX)
  if (s_TTYMode)
    return;
  move(y, x);
#endif
  s_ChrNo = x; }

//----------------------------------------------SynError
int SynError(struct Buf *CommandBuf, char *FormatString, ...)
  { // Flags syntax errors. The command buf is usually s_CommandBuf except when running a macro.
  va_list ap;
  char String[StringMaxChr];
   
  va_start(ap, FormatString);
  vsnprintf(String, StringMaxChr, FormatString, ap);
  va_end(ap); 
  if (CommandBuf) {
    DisplayDiag(CommandBuf, TRUE, String);
    CommandBuf->Predecessor = NULL; }
  else
    DisplaySource(s_CurrentCommand, String);
  if (s_Verbose & Verbose_AnnoyingBleep)
#if defined(VC)
    Beep(750, 300);
#else
    beep();
#endif
  s_BombOut = TRUE;
  return 1; } 

//----------------------------------------------RunError
int RunError(char *FormatString, ...)
  { // Flags serious cockups detected at run time.
  va_list ap;
  char String[StringMaxChr];
  int Chrs, MaxLen = StringMaxChr;
  
  String[0] = '\0';
  if ( ! s_TTYMode && s_TermWidth < StringMaxChr)
    MaxLen = s_TermWidth;
  va_start(ap, FormatString);
  Chrs = vsnprintf(String, MaxLen, FormatString, ap);
  va_end(ap); 
  if ( ( (s_Verbose & Verbose_QuiteChatty) || (s_Verbose & Verbose_PrintLine) ) && s_CurrentBuf->FirstRec )
    DisplayDiag(s_CurrentBuf, TRUE, "");
  if (MaxLen < Chrs+15) {
    Chrs = MaxLen-20;
    strcpy(String+MaxLen-20, " ... "); }
  Chrs += sprintf(String+Chrs, " (%lld)", s_CommandCounter);
  ScrollConsole(FALSE, FALSE);
  DisplaySource(s_CurrentCommand, String);
  if (s_Verbose & Verbose_AnnoyingBleep)
#if defined(VC)
    Beep(750, 300);
#else
    beep();
#endif
  s_BombOut = TRUE;
  return 1; }

//----------------------------------------------Fail
int Fail(char *FormatString, ...)
  { //Gives optional diagnostic help with selected failures.
  if ( (s_Verbose & Verbose_QuiteChatty) || (s_Verbose & Verbose_PrintLine) ) {
    va_list ap;
    char String[StringMaxChr];
    int Chrs, MaxLen = StringMaxChr;
    
    if (s_TermWidth < StringMaxChr)
      MaxLen = s_TermWidth;
    if (s_Verbose & Verbose_ReportCounter)
      MaxLen -= 20;
    va_start(ap, FormatString);
    Chrs = vsnprintf(String, MaxLen, FormatString, ap);
    va_end(ap); 
    if (s_Verbose & Verbose_ReportCounter) {
      sprintf(String+(Chrs <= MaxLen ? Chrs : MaxLen), " (%lld)", s_CommandCounter); }
    if (s_Verbose & Verbose_QuiteChatty)
      DisplaySource(s_CurrentCommand, String);
    if ( (s_Verbose & Verbose_PrintLine) && s_CurrentBuf->FirstRec)
      DisplayDiag(s_CurrentBuf, TRUE, ""); }
  return 1; }

//----------------------------------------------Message
void Message(struct Buf *CommandBuf, char *FormatString, ...)
  { //Throws a new line, then uses prompt to write the string.
  va_list ap;
  char string[StringMaxChr];
  int Orig_x = s_x, Orig_y = s_y, OrigChrNo = s_ChrNo;      //When debugging, we want to avoid disturbing the window.
  
  s_ChrNo = 0;
  va_start(ap, FormatString);
  vsnprintf(string, StringMaxChr, FormatString, ap);
  if (s_Verbose & Verbose_ReportCounter) {
    char Temp[20];
    sprintf(Temp, " (%lld)", s_CommandCounter);
    strcat(string, Temp); }
  if (CommandBuf)
    DisplayDiag(CommandBuf, TRUE, string);
  ScrollConsole(FALSE, TRUE);
  JotAddBoundedString(string, strlen(string), s_TermWidth, NULL, NULL, 0);
  JotGotoXY(Orig_x, Orig_y);
  s_ChrNo = OrigChrNo; }

//----------------------------------------------Prompt
void Prompt(char *FormatString, ...)
  { //In curses mode, directs messages to console otherwise to stdout. All output suppressed in QuietMode.
  va_list ap;
  
  if (s_Verbose & Verbose_Prompt) {
    char string[StringMaxChr];
    va_start(ap, FormatString);
    vsnprintf(string, StringMaxChr, FormatString, ap);
    ScrollConsole(FALSE, FALSE);
    s_NoUnicode = TRUE;
    JotAddBoundedString(string, strlen(string), s_TermWidth, NULL, NULL, 0);
#if defined(LINUX)
    refresh();
#endif
    va_end(ap);
    return; }
  return; }

//----------------------------------------------CommandPrompt
void CommandPrompt(char *FormatString, ...)
  { //In curses mode, directs messages to console otherwise to stout. All output suppressed in QuietMode.
    //Identical to Prompt() except that ScrollConsole() is called with NewCommandLine TRUE. 
  va_list ap;
  
  if (s_Verbose & Verbose_Prompt) {
    char string[StringMaxChr];
    va_start(ap, FormatString);
    vsnprintf(string, StringMaxChr, FormatString, ap);
    ScrollConsole(TRUE, FALSE);
    if (s_TTYMode)
      fprintf(stderr, "%s", string);
    else {
      s_NoUnicode = TRUE;
      JotAddBoundedString(string, strlen(string), s_TermWidth, NULL, NULL, 0); }
#if defined(LINUX)
    refresh();
#endif
    va_end(ap);
    return; }
  return; }

//----------------------------------------------PriorityPrompt
void PriorityPrompt(char *FormatString, ...)
  { //Same as Prompt() except that it ignores quiet setting in s_Verbose.
  va_list ap;
  char string[StringMaxChr];
  
  va_start(ap, FormatString);
  vsnprintf(string, StringMaxChr, FormatString, ap);
  ScrollConsole(FALSE, FALSE);
  if (s_TTYMode)
    fprintf(stderr, "%s", string);
  else {
    s_NoUnicode = TRUE;
    JotAddBoundedString(string, strlen(string), s_TermWidth, NULL, NULL, 0); }
#if defined(LINUX)
  refresh();
#endif
  va_end(ap);
  return; }

//----------------------------------------------DisplaySource
void DisplaySource(struct Com *Command, char *Message)
  { //Extracts and checks pointer to source command string for debug/diagnostic display, constructs a dummy command buffer to pass into DisplayDiag().
  struct Buf SourceCopyBuf;
  struct Com *NextCommand;
  struct Rec *ThisRec;
  int NextCommandStart, Key;
  char MacroStatus;
  
  if ( ! Command) {
    PriorityPrompt(Message);
    return; }
  MacroStatus = Command->CommandBuf->UnchangedStatus;
  if ( (Command != NULL) && (MacroStatus & SameSinceCompiled) && ((MacroStatus & MacroRun) == 0) ) { //Commannd buffer has not been redefined.
    NextCommand = Command->NextCommand;
    ThisRec = Command->CommandRec;
    NextCommandStart = (NextCommand == NULL) ? strlen(ThisRec->text) : NextCommand->CommandByteNo;
    Key = Command->CommandKey;
    if (Key == '(' || Key == ')')
      NextCommandStart = (Command->CommandByteNo)+1;
    SourceCopyBuf.FirstRec = ThisRec;
    SourceCopyBuf.CurrentRec = ThisRec;
    SourceCopyBuf.SubstringLength = NextCommandStart-(Command->CommandByteNo);
    SourceCopyBuf.CurrentByte = Command->CommandByteNo; 
    if (strlen(SourceCopyBuf.CurrentRec->text) < SourceCopyBuf.SubstringLength) //Tack - but it seems to fix error with display of new command lines shorter than previous one.
      SourceCopyBuf.SubstringLength = 0;
    if (Command->CommandBuf != s_CommandBuf) { //Add location details.
      char NewText[StringMaxChr];
      sprintf(NewText, "%s (line %d of buffer %c)", Message, Command->CommandLineNo, Command->CommandBuf->BufferKey);
      DisplayDiag(&SourceCopyBuf, TRUE, NewText); }
    else
      DisplayDiag(&SourceCopyBuf, TRUE, Message); }
  else {
    char Key = Command->CommandBuf->BufferKey;
    char String[StringMaxChr];
    int i, ChrNo;
    struct Com *ThisCom = Command;
    ChrNo = sprintf(String, "  Line %4d, Chr %d of macro %c: (disassembly) ", Command->CommandLineNo, Command->CommandByteNo, Key ? Key : ' ');
    
    for (i = 0; i < 10; i++) {
      DumpCommand(ThisCom, &ChrNo, String);
      if ( ! (ThisCom = ThisCom->NextCommand) )
        break; }
    PriorityPrompt("{%s} %s", Message, String); } }

//----------------------------------------------DisplayDiag
void DisplayDiag(struct Buf *TextBuf, int ShowCursor, char *MessageText)
  { // Procedure gives diagnostic data on state of string.
  char *Record = TextBuf->CurrentRec->text;
  int RecordLength, CurrentByte, SubstringLength, SubstringLeft, SubstringRight, FirstByte, MessageLength, WidthLimit, ChrLimit, Chrs, Bytes, Result;
  
  if ( ! TextBuf->CurrentRec)
    return;
    
  RecordLength = strlen(TextBuf->CurrentRec->text);
  CurrentByte = TextBuf->CurrentByte;
  SubstringLength = TextBuf->SubstringLength;
  FirstByte = 0;                   //The leftmost character to appear on the screen.
  MessageLength = strlen(MessageText);
  WidthLimit = s_TTYMode ? RecordLength : s_TermWidth;
  ChrLimit =                       //The length of line available to display the record. 
#if defined(VC)
    s_TTYMode ? MessageLength+RecordLength+2 : (MessageLength <= WidthLimit ? (WidthLimit-(MessageLength ? MessageLength+3 : 0)) : 0)-1;
#else
    s_TTYMode ? MessageLength+RecordLength+2 : (MessageLength <= WidthLimit ? (WidthLimit-(MessageLength ? MessageLength+3 : 0)) : 0);
#endif
  
  s_NoUnicode = TextBuf->NoUnicode;
  if (SubstringLength == 0)
    SubstringLeft = SubstringRight = CurrentByte;
  else if (0 <= SubstringLength) {
    SubstringLeft = CurrentByte;                          //The first byte of the next substring.
    SubstringRight = SubstringLeft+SubstringLength-1; }   //The last byte of the current substring.
  else {
    SubstringLeft = CurrentByte+SubstringLength;
    SubstringRight = SubstringLeft-SubstringLength; }
  if (ChrLimit < RecordLength) { //This record will overflow the available line width.
    if (ChrLimit <= CurrentByte) {
      FirstByte = (SubstringLength < ChrLimit) ? CurrentByte-(ChrLimit/2) : (SubstringLength < 0) ? CurrentByte+ChrLimit-3 : CurrentByte+3;
      if (FirstByte < 0)
        FirstByte = 0;
      if (RecordLength < FirstByte+ChrLimit)
        FirstByte = RecordLength-ChrLimit+1; } }
  if (0 < MessageLength) {
    PriorityPrompt("{%s}", MessageText);
    MessageLength += 2; }
  else
    ScrollConsole(FALSE, FALSE);
    
  if (ShowCursor) { // Display cursor.
    if (s_TTYMode) {
      fputs(TextBuf->CurrentRec->text, stdout);
      if (s_Verbose & Verbose_NonSilent) {
        int i = 0, Lim = MessageLength + (SubstringLeft ? SubstringLeft : CurrentByte);
        ScrollConsole(FALSE, TRUE);
        for ( ; i < Lim; i++)
          fputc(' ', stdout);
        if (CurrentByte == SubstringLeft)
          fputc('^', stdout);
        if (SubstringLeft < SubstringRight) {
          for ( ; i < MessageLength+SubstringRight; i++)
            fputc('~', stdout);
          if (SubstringRight && (CurrentByte == SubstringRight))
            fputc('^', stdout); }
        fflush(stdout); }
      else
        fputc('\n', stdout);
      return; }
    if ( ! s_StreamOut) {
      if (SubstringLeft-FirstByte < 0) //This is an abnormally long substring - ignore it.
        return;
      JotSetAttr(Attr_Normal);
      if (WidthLimit < ChrLimit)
        ChrLimit = WidthLimit;
      JotAddBoundedString(Record+FirstByte, SubstringLeft-FirstByte, ChrLimit, NULL, &Chrs, 0);
      Bytes = SubstringLeft;
      if (ChrLimit <= 0) //Console line already filled by message text.
        return;
      if (SubstringLeft == CurrentByte) { //Substring begins at current chr or no substring
        JotSetAttr(Attr_CurrChr);
        if (RecordLength-1 < Bytes && Chrs < WidthLimit) {
          JotAddBoundedString("~", 1, 1, &Result, NULL, 0);
          Bytes += Result-1; }
        else {
          JotAddBoundedString(Record+SubstringLeft, RecordLength-SubstringLeft, 1, &Result, NULL, 0);
          Bytes += Result; }
        Chrs++;
        if (SubstringLeft < SubstringRight) {
          JotSetAttr(Attr_SelSubStr);
        JotAddBoundedString(Record+Bytes, SubstringRight-Bytes+1, ChrLimit-Chrs, NULL, &Result, 0);
          Chrs += Result;
          Bytes = SubstringRight+1; } }
      else { // Current character follows end of substring
        JotSetAttr(Attr_SelSubStr);
        JotAddBoundedString(Record+SubstringLeft, SubstringRight-SubstringLeft, ChrLimit-Chrs, NULL, &Result, 0);
        Chrs += Result;
        if (WidthLimit <= Chrs)
          return;
        Bytes += SubstringRight-SubstringLeft;
        JotSetAttr(Attr_CurrChr);
        JotAddBoundedString(Record+Bytes, RecordLength-Bytes, 1, &Result, NULL, 0);
        Bytes += Result;
        Chrs++;
        if (RecordLength <= Bytes && Chrs < WidthLimit) {
          JotAddBoundedString("~", 1, 1,  NULL, &Result, 0);
          Chrs += Result; } }
      JotSetAttr(Attr_Normal);
      JotAddBoundedString(Record+Bytes, RecordLength-Bytes, ChrLimit-Chrs, NULL, NULL, 0);
      JotGotoXY(0, s_TermHeight-1); }
    else { //In stream-out mode, provide basic diagnostics via stdout
      fputs(TextBuf->CurrentRec->text, stderr);
      fputc('\n', stderr);
      if (s_Verbose & Verbose_NonSilent) {
        int i = 0, Lim = MessageLength + (SubstringLeft ? SubstringLeft : CurrentByte);
        ScrollConsole(FALSE, TRUE);
        for ( ; i < Lim; i++)
          fputc(' ', stderr);
        if (CurrentByte == SubstringLeft)
          fputc('^', stderr);
        if (SubstringLeft < SubstringRight) {
          for ( ; i < MessageLength+SubstringRight; i++)
            fputc('~', stderr);
          if (SubstringRight && (CurrentByte == SubstringRight))
            fputc('^', stderr); }
        fflush(stderr);
        fputc('\n', stderr); }
      else
        fputc('\n', stderr); } }
  else { // Print without showing cursor and substrings.
    JotAddBoundedString(Record, strlen(Record), WidthLimit, NULL, NULL, 0);
    JotGotoXY(s_x, s_y); } }

//----------------------------------------------JotTrace
void JotTrace(char * OrigMessage)
  { //Handles reporting at a selected trace point.
  char TraceMessage[StringMaxChr]; 
  
  if (s_MaxTraceLevel) {
    if (s_MaxTraceLevel < s_TraceLevel) {
      s_TraceSkipped = TRUE;
      return; }
    if (s_TraceSkipped && (s_TraceLevel == s_MaxTraceLevel) )
      s_MaxTraceLevel = 0; }
  TraceMessage[0] = '\0'; 
  if (OrigMessage)
    strcpy(TraceMessage, OrigMessage);
    
  if (TraceMessage[0]) {
    int Orig_NonSilent = s_Verbose & Verbose_NonSilent;
    s_Verbose |= Verbose_NonSilent;
    if (s_TraceMode & Trace_Stack)
      DumpStack(NULL);
    if (s_Verbose & Verbose_ReportCounter) {
      char Temp[20];
      sprintf(Temp, " (%lld)", s_CommandCounter);
      strcat(TraceMessage, Temp); }
    if (s_TraceMode & Trace_Backtrace)
      Backtrace(NULL);
    if (s_TraceMode & Trace_CmdCount)
      Prompt("Command counter = %lld", s_CommandCounter);
    if (s_TraceMode & Trace_Print)
      DisplayDiag(s_CurrentBuf, TRUE, "");
    if (s_TraceMode & Trace_Source)
      DisplaySource(s_CurrentCommand, TraceMessage);
    if (s_TraceMode & Trace_Break) {
      struct Com *CurrentCommandSave = s_CurrentCommand;
      int CommandChrSave = s_CommandChr;
      JotBreak(TraceMessage); 
      s_CurrentCommand = CurrentCommandSave;
      s_CommandChr = CommandChrSave; }
    s_Verbose |= Orig_NonSilent; } }

//----------------------------------------------JotBreak
void JotBreak(char *Text)
  { //Breakpoint prompt and command handler.
  int TraceModeSave = s_TraceMode;
  struct Com *CurrentCommandSave = s_CurrentCommand;
  struct BacktraceFrame ThisBacktraceFrame;
  char asConsole;
  unsigned long long int CommandCounterSave = s_CommandCounter;
  
  if ( ! s_EditorInput)
    return;
  asConsole = s_EditorInput->asConsole;
  s_EditorInput->asConsole = FALSE;
  s_DebugLevel++;
  JotUpdateWindow();
   
  for ( ; ; ) {
    int ExitNow = FALSE;
    int OrigCommandMode = s_CommandMode;
    struct Seq *DebugComSequence = NULL;
    s_DebugBuf->CurrentRec->text[0] = '\0';
    s_BombOut = FALSE;
    JotUpdateWindow();
    if ( ( ! s_InView) && (s_Verbose & Verbose_NonSilent) )
      DisplayDiag(s_CurrentBuf, TRUE, "");
    s_BombOut = FALSE;
    s_DebugBuf->CurrentRec->text[0] = '\0';
    s_CommandMode = 0;
    ExitNow |= ReadCommand(
      s_DebugBuf, 
      ( s_RecoveryMode && ! (s_TraceMode & Trace_Recovery) ? s_EditorInput : s_EditorConsole)->FileHandle, 
      "(Debug %d) %d %c> ", 
      s_DebugLevel, 
      s_CurrentBuf->LineNumber, 
      s_CurrentBuf->BufferKey);
    s_CommandMode = OrigCommandMode;
    if (s_BombOut) {
      if (s_JournalHand)
        UpdateJournal("%%Debugger off", NULL);
      s_TraceMode = 0;
      break; }
    if (ExitNow)
      break;
    if ( ! s_DebugBuf->CurrentRec->text[0])
      break;
    DebugComSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
    memset(DebugComSequence, 0, sizeof(struct Seq));
    JOT_Sequence(s_DebugBuf, DebugComSequence, FALSE);
    s_TraceMode = Trace_Break; 
    s_BacktraceFrame->LastCommand = s_CurrentCommand;
    ThisBacktraceFrame.LastCommand = NULL;
    ThisBacktraceFrame.prev = s_BacktraceFrame;
    ThisBacktraceFrame.type = DebuggerCommand;
    if (Run_Sequence(DebugComSequence))
      Message(NULL, "That command failed");
    if (s_TraceMode == Trace_Break)
      s_TraceMode = TraceModeSave;
    else if (s_TraceMode == 0) {
      FreeSequence(&DebugComSequence);
      break; }
    else
      s_TraceMode = TraceModeSave;
    s_CurrentCommand = CurrentCommandSave;
    s_BacktraceFrame = ThisBacktraceFrame.prev;
    FreeSequence(&DebugComSequence);
    if (s_MaxTraceLevel) // The command was a s_traceskip - continue normally until s_MaxTraceLevel is satisfied.
      break; }
     
  s_DebugLevel--;
  s_CurrentCommand = CurrentCommandSave;
  //On restoration of normal execution mode, the cummand counter is restored unmodified, otherwise remove this command from the counter.
  s_CommandCounter = CommandCounterSave- (s_DebugBuf->CurrentRec->text[0] ? 1 : 0);
  s_EditorInput->asConsole = asConsole; }

//----------------------------------------------Disaster
void Disaster(char *Text, ...)
  { // Flags showstoppers detected at startup time.
  va_list ap;
  char String[StringMaxChr];
  
  va_start(ap, Text);
  vsnprintf(String, StringMaxChr, Text, ap);
  va_end(ap);
#if defined(LINUX)
  if (mainWin) //Close down curses now to ensure message and backtrace are visible in linuxland.
    endwin();
#endif
  if (s_CurrentCommand) {
    s_TTYMode = TRUE;
    Backtrace(NULL);
    fputc('\n', stderr); }
  TidyUp(1, String);
  exit(1); } 

//----------------------------------------------TidyUp
int TidyUp(int ExitStatus, char * ExitMessage)
  { // Exit procedure
  int i = 0;
  struct Buf *ThisBuf = s_BufferChain;
  char BuffersToBeWritten[100] = "";
  struct Buf * DestBuf = NULL;
  struct Buf *ThisInteractiveBuf = s_FirstInteractiveBuf;
  char LockFilePathname[StringMaxChr];
  
#if defined(VC)
  if ( ! s_TTYMode) {
    SetConsoleMode(hStdin, s_OrigConsoleMode);
    SetConsoleTextAttribute(hStdout, s_OrigColorAttrs); }
#else
  mousemask(0, NULL);
#endif
  
  if (s_HoldScreen) {
    int Chr;
    Prompt("Exiting now (%d) hit C to continue, E to exit script or any other key to exit now ", s_CommandCounter);
    Chr = JotGetCh(s_EditorConsole->FileHandle);
    if (Chr == 'C')
      return 1;
    else if (Chr == 'E')
      RunError("Exiting script"); }
     
  while (ThisBuf != NULL) {
    if ( (ThisBuf->EditLock & WriteIfChanged) && ! (ThisBuf->UnchangedStatus & SameSinceIO) ) {
      BuffersToBeWritten[i++] = ThisBuf->BufferKey;
      if (ThisBuf->ParentObj) {
        char ParentPath[StringMaxChr];
        GetBufferPath(ThisBuf, ParentPath, StringMaxChr);
        if ( ! DestBuf) {
          DestBuf = GetBuffer('?', AlwaysNew);
          s_CurrentBuf = DestBuf;
          AddFormattedRecord(FALSE, DestBuf, "The following buffers have been changed and require writing:"); }
        AddFormattedRecord(FALSE, DestBuf, "  Parent path = %s", ParentPath); } }
      ThisBuf = ThisBuf->NextBuf; }
      
  BuffersToBeWritten[i] = '\0';
  if (0 < i) {
    RunError("The following buffers are WriteIfChanged and need writing: \"%s\"", BuffersToBeWritten);
    return 1; }

#if defined(VC)
  if ( ! s_TTYMode) { //Restore console.
    JotGotoXY(0, s_TermHeight-1);
    SetConsoleTextAttribute(hStdout, s_OrigColorAttrs); }
#elif defined(LINUX)
  if (mainWin) //Close down curses.
    endwin();
#endif
  
  if (ExitMessage[0]=='\'') { //It's an indirect reference to a buffer.
    struct Buf *ThisBuf = GetBuffer(toupper(ExitMessage[1]), NeverNew);
    struct Rec *ThisRec;
    if ( ! ThisBuf)
      return Fail("Invalid reference to a buffer.");
    while (ThisRec = ThisBuf->FirstRec->prev) {
      if ( ! ThisRec->text[0])
        break;
      FreeRecord(ThisBuf, AdjustBack); }
    
    ThisRec = ThisBuf->FirstRec;
    do {
      printf("%s\n", ThisRec->text);
      ThisRec = ThisRec->next; }
      while (ThisRec != ThisBuf->FirstRec); }
  else if (ExitMessage && s_Verbose)
    printf("%s\n", ExitMessage);

  while (ThisInteractiveBuf) { //Close child pipes and terminate child processes
    struct Buf *NxInteractiveBuf = ThisInteractiveBuf->IOBlock->NxInteractiveBuf;
    FreeBufferThings(ThisInteractiveBuf);
    ThisInteractiveBuf = NxInteractiveBuf; }
  
  while (FreeCommandFile())
    continue;
    
  if (s_StartupFileName)
    free(s_StartupFileName);
      
  if (s_OriginalSession && s_JournalHand) {
    strcpy(LockFilePathname, s_JournalPath);
    strcat(LockFilePathname, "/LOCK");
    if(unlink(LockFilePathname))
      printf("Failed to delete journal/backup file %s\n", LockFilePathname); }
  
  exit(ExitStatus); }

//----------------------------------------------DoHash
int DoHash(char NomBufKey, struct AnyArg **ThisArg, struct Com *CurrentCommand)
  { //Maintenance of hashtable entries in buffer nominated by NomBuf, defaults to s_CurrentBuf.
  struct Buf *NomBuf = NULL;
  int Operation = FetchIntArgFunc((struct AnyArg **)ThisArg);
      
  if (NomBufKey == '\0')
    NomBuf = s_CurrentBuf;
  else if ( (NomBuf = GetBuffer(NomBufKey, (Operation == H_create) ? OptionallyNew : NeverNew)) == NULL)
    return Fail("Buffer ( %c ) has not been initialized.", NomBufKey);
  if ( ! NomBuf->CurrentRec)
    GetRecord(NomBuf, 1, "");

  switch (Operation) {
  case H_create: { //create - set up the table with specified no. of elements.
    int Qual = FetchIntArgFunc((struct AnyArg **)ThisArg);
    long long NElem;
    int status = 0, PathLen = 0;
    char *LastPathElem, Path[StringMaxChr];
    struct DataHTabObj * ThisDataHTabObj;
    struct intFrame *FinalFrame;
    
    if ( (status |= FetchIntArg(&NElem, (struct AnyArg **)ThisArg)) ) {
      SynError(NULL, "Error in hashtable-size specification");
      return 0; }
    if (FetchStringArgPercent(Path, &PathLen, ThisArg)) {
      Fail("Syntax in hashtable path expression.");
      return 1; }
    if (Path[0]) {
      if ( ! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem, &FinalFrame, NULL)) )
        return 1;
      if (LastPathElem[0]) {
        if ( ! (ThisDataHTabObj = (struct DataHTabObj *)QueryData(LastPathElem, NomBuf)) ||
             ( ! (struct bufFrame *)ThisDataHTabObj->Frame || ((struct bufFrame *)ThisDataHTabObj->Frame)->type != FrameType_Buf) ) {
            Fail("Object \"%s\" in path \"%s\" is not a buffer.", LastPathElem, Path);
            return 1; }
        NomBuf = (struct Buf *)((struct bufFrame *)ThisDataHTabObj->Frame)->BufPtr; } }
    if ( ! NomBuf->CurrentRec)
      GetRecord(NomBuf, 1, "");
    if (NomBuf->htab)
      DestroyHtab(NomBuf);
    if (0 < NElem)
      NomBuf->htab = (struct hsearch_data *)JotMalloc(sizeof(struct hsearch_data));
    else
      return 0;
    NomBuf->HashtableMode = 
      (Qual == H_destroyQual) ? DestroyHashtables : 
      ((Qual == H_protectQual) ? ProtectEntries : 
      ((Qual == H_adjustQual) ? AdjustEntries : 
      ((Qual == H_deleteQual) ? DeleteEntries :
      AdjustEntries)));
    NomBuf->htab->table = NULL; 
    NomBuf->htab->size = 0; 
    NomBuf->htab->filled = 0; 
    status |= hcreate_r(NElem, NomBuf->htab);
    if (NomBuf->FirstObj) { //Move all non-zombie objects to the new hashtable.
      struct AnyHTabObj *ThisObj = NomBuf->FirstObj;
      ENTRY *FoundVal, SearchVal;
      while (ThisObj) {
        if (ThisObj->type == HTabObj_Zombie) { //A zombie - if it's got a NewObj then just take that one.
          struct AnyHTabObj *NewObj = ((struct ZombieHTabObj *)ThisObj)->NewObj;
          if ( ! NewObj) {
            ThisObj = ThisObj->next;
            continue; }
          ThisObj = NewObj; }
        SearchVal.key = ThisObj->HashKey;
        SearchVal.data = ThisObj;
        if ( ! hsearch_r(SearchVal, ENTER, &FoundVal, NomBuf->htab))
          return Fail("Failed to restore hashtable item %s - ", ThisObj->HashKey);
        ThisObj = ThisObj->next; } }
    return ( ! status); }

  case H_addjump:
  case H_newjump: { //add or new - puts item into table.
    ENTRY *FoundVal, SearchVal;
    struct JumpHTabObj *Object = NULL;
    struct AnyHTabObj *OldObj = NULL;
    char *KeyCopy;
    struct TargetTag *ThisTag;
    char *LastPathElem, Path[StringMaxChr];
    int PathLen = 0;
    
    if (FetchStringArgPercent(Path, &PathLen, ThisArg)) {
      Fail("Syntax in hashtable path expression.");
      return 1; }
    if ( ! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem, NULL, NULL)) )
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
      
    SearchVal.key = LastPathElem;
    SearchVal.data = NULL;
    if (hsearch_r(SearchVal, FIND, &FoundVal, NomBuf->htab)) { //A preexisting entry.
      Object = FoundVal->data;
      OldObj = (struct AnyHTabObj *)FoundVal->data;
      if (Object) {
        if (OldObj->type == HTabObj_Jump) { //The preexisting entry was a JumpHTabObj.
          if (Operation == H_newjump) { //Collision detected while adding 'newjump' entry - restore original and fail.
            Fail("Failed create new item \"%s\" in hashtable of buffer %c", LastPathElem, NomBuf->BufferKey);
            return 1; } }
        else if (OldObj->type == HTabObj_Zombie) //The preexisting entry is a zombie (was deleted).
          Object = NULL;
        else {
          Fail("Attempt to coerce some other type of hashtable object to a JumpHTabObj");
          return 1; } } }
        
    if ( ! Object) {
      Object = (struct JumpHTabObj *)JotMalloc(sizeof(struct JumpHTabObj));
      if (OldObj) {
        struct AnyHTabObj *OriginalSecondaryObj = ((struct ZombieHTabObj *)OldObj)->NewObj;
        if (OriginalSecondaryObj)
          Zombify(NomBuf, (struct AnyHTabObj *)OriginalSecondaryObj);
        ((struct ZombieHTabObj *)OldObj)->NewObj = (struct AnyHTabObj *)Object;
        Object->HashKey = OldObj->HashKey;
        OldObj->HashKey = NULL;
        Object->next = OldObj->next;
        OldObj->next = (struct AnyHTabObj *)Object; }
      else {
        KeyCopy = (char *)JotMalloc(strlen(LastPathElem)+1);
        strcpy(KeyCopy, LastPathElem); 
        SearchVal.key = KeyCopy;
        SearchVal.data = (void *)Object;
        if( ! hsearch_r(SearchVal, ENTER, &FoundVal, NomBuf->htab)) {
          Fail("Failed to create data item \"%s\" in hashtable of buffer %c", KeyCopy, NomBuf->BufferKey);
          free(Object);
          free(KeyCopy);
          return 1; }
        Object->HashKey = KeyCopy;
        Object->next = NomBuf->FirstObj;
        NomBuf->FirstObj = (struct AnyHTabObj *)Object; } }
    else //A preexisting object - this can only be  an old JumpHTabObj - delete it's target.
      FreeTarget(NomBuf, Object);
      
    //Create TargetPoint tag for target record.
    ThisTag = (struct TargetTag *)JotMalloc(sizeof(struct TargetTag));
    ThisTag->next = NULL;
    ThisTag->StartPoint = s_CurrentBuf->CurrentByte;
    ThisTag->type = TagType_Target;
    ThisTag->Attr = (void *)Object;
    AddTag(s_CurrentBuf->CurrentRec, (struct AnyTag *)ThisTag);
    
    //Set up the JumpHTabObj
    Object->type = HTabObj_Jump;
    Object->HashBuf = NomBuf;
    Object->TargetBuf = s_CurrentBuf;
    Object->TargetRec = s_CurrentBuf->CurrentRec;
    Object->Target = ThisTag;
    Object->LineNumber = s_CurrentBuf->LineNumber;
    NomBuf->UnchangedStatus |= SameSinceIndexed;
    
    return 0; }

  case H_code: { //Adds a code segment to the specified table.
    ENTRY *FoundVal, SearchVal;
    struct CodeHTabObj *Object = NULL;
    struct AnyHTabObj *OldObj = NULL;
    struct CodeHeader *CodeHeader;
    char *KeyCopy;
    struct TargetTag;
    char *LastPathElem, Path[StringMaxChr];
    int PathLen = 0;
    
    if (FetchStringArgPercent(Path, &PathLen, ThisArg)) {
      Fail("Syntax in hashtable path expression.");
      return 1; }
    if ( ! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem, NULL, NULL)) )
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
      
    SearchVal.key = LastPathElem;
    SearchVal.data = NULL;
    if (hsearch_r(SearchVal, FIND, &FoundVal, NomBuf->htab)) { //A preexisting entry.
      Object = FoundVal->data;
      OldObj = (struct AnyHTabObj *)FoundVal->data;
      if (Object) {
        if (OldObj->type == HTabObj_Code) { //The preexisting entry was a CodeHTabObj.
          Fail("Failed create new item \"%s\" in hashtable of buffer %c", LastPathElem, NomBuf->BufferKey);
          return 1; }
        else if (OldObj->type == HTabObj_Zombie) //The preexisting entry is a zombie (was deleted).
          Object = NULL;
        else {
          Fail("Attempt to coerce some other type of hashtable object to a CodeHTabObj");
          return 1; } } }
        
    if ( ! Object) {
      Object = (struct CodeHTabObj *)JotMalloc(sizeof(struct CodeHTabObj));
      if (OldObj) {
        struct AnyHTabObj *OriginalSecondaryObj = ((struct ZombieHTabObj *)OldObj)->NewObj;
        if (OriginalSecondaryObj)
          Zombify(NomBuf, (struct AnyHTabObj *)OriginalSecondaryObj);
        ((struct ZombieHTabObj *)OldObj)->NewObj = (struct AnyHTabObj *)Object;
        Object->next = OldObj->next;
        OldObj->next = (struct AnyHTabObj *)Object; }
      else {
        KeyCopy = (char *)JotMalloc(strlen(LastPathElem)+1);
        strcpy(KeyCopy, LastPathElem); 
        Object->HashKey = KeyCopy;
        SearchVal.key = KeyCopy;
        SearchVal.data = (void *)Object;
        if( ! hsearch_r(SearchVal, ENTER, &FoundVal, NomBuf->htab)) {
          Fail("Failed to create data item \"%s\" in hashtable of buffer %c", KeyCopy, NomBuf->BufferKey);
          free(Object);
          free(KeyCopy);
          return 1; }
        Object->next = NomBuf->FirstObj;
        NomBuf->FirstObj = (struct AnyHTabObj *)Object; } }
      
    //Set up the CodeHTabObj
    CodeHeader = (struct CodeHeader *)malloc(sizeof(struct CodeHeader));
    CodeHeader->SourceRec = NomBuf->CurrentRec;
    CodeHeader->CompiledCode = NULL;
    CodeHeader->next = NomBuf->CodeChain;
    NomBuf->CodeChain = CodeHeader;
    Object->type = HTabObj_Code;
    Object->CodeHeader = CodeHeader;
    NomBuf->UnchangedStatus |= SameSinceIndexed;
    
    return 0; }

  case H_setfsect:  //setfsect - adds pathname, seek offset and byte count fo %i ... -fsection=...; N.B. does not consume the stack values.
  case H_setsect: { //setsect - adds seek offset and byte count for %i ... -section=...; N.B. does not consume the stack values.
    ENTRY *FoundVal, SearchVal;
    struct SetsectHTabObj *Object = NULL, *OldObj = NULL;
    char *PathName;
    long long Seek, Bytes;
    char *KeyCopy;
    char *LastPathElem, Path[StringMaxChr];
    int PathLen = 0;
    int LocalFail = 0;
    
    if (FetchStringArgPercent(Path, &PathLen, ThisArg)) {
      Fail("Syntax in hashtable path expression.");
      return 1; }
    if ( ! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem, NULL, NULL)) )
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
    if (s_StackPtr < ((Operation == H_setsect) ? 2 : 3)) {
      Fail("Insufficient entries for this operation.");
      return 1; }
      
    SearchVal.key = LastPathElem;
    SearchVal.data = NULL;
    if (hsearch_r(SearchVal, FIND, &FoundVal, NomBuf->htab)) {
      Object = OldObj = FoundVal->data;
      if (OldObj && OldObj->type != HTabObj_Zombie) { //Allow zombies.
        Fail("Hashtable in buffer %c already has an entry for key \"%s\"", NomBuf->BufferKey, SearchVal.key);
        return 1; } }
        
    Bytes = PopInt(&LocalFail);
    Seek = PopInt(&LocalFail);
    if (LocalFail)
      return 1;
    s_StackPtr += 2;
    if (Operation == H_setfsect) {
      if (s_Stack[s_StackPtr-3].type != FrameType_Buf) {
        Fail("Incorrect stack-frame type for setfsect pathname entry.");
        return 1; }
      PathName = ((struct bufFrame *)(&s_Stack[s_StackPtr-3]))->BufPtr->CurrentRec->text; }
    
    if (Operation == H_setsect) {
      Object = (struct SetsectHTabObj *)JotMalloc(sizeof(struct SetsectHTabObj));
      Object->type = HTabObj_Setsect; }
    else {
      Object = (struct SetsectHTabObj *)JotMalloc(sizeof(struct SetfsectHTabObj));
      Object->type = HTabObj_Setfilesect; }
    if (OldObj) {
      struct AnyHTabObj *OriginalSecondaryObj = ((struct ZombieHTabObj *)OldObj)->NewObj;
      if (OriginalSecondaryObj)
        Zombify(NomBuf, (struct AnyHTabObj *)OriginalSecondaryObj);
      ((struct ZombieHTabObj *)OldObj)->NewObj = (struct AnyHTabObj *)Object;
      Object->HashKey = OldObj->HashKey;
      OldObj->HashKey = NULL;
      Object->next = OldObj->next;
      OldObj->next = (struct AnyHTabObj *)Object; }
    else {
      KeyCopy = (char *)JotMalloc(strlen(LastPathElem)+1);
      strcpy(KeyCopy, LastPathElem);
      Object->HashKey = KeyCopy;
      Object->next = NomBuf->FirstObj;
      NomBuf->FirstObj = (struct AnyHTabObj *)Object;
      SearchVal.key = KeyCopy;
      SearchVal.data = Object;
      if ( ! hsearch_r(SearchVal, ENTER, &FoundVal, NomBuf->htab)) {
        Fail("Failed to create setsect item \"%s\" in hashtable of buffer %c", LastPathElem, NomBuf->BufferKey);
        if (Operation == H_setfsect)
          ((struct SetfsectHTabObj *)Object)->PathName = NULL;
        Zombify(NomBuf, (struct AnyHTabObj *)Object);
        ((struct ZombieHTabObj *)Object)->NewObj = NULL;
        return 1; } }
    if (Operation == H_setfsect) {
      ((struct SetfsectHTabObj *)Object)->PathName = NULL;
      if (s_SetfsectPathName) {
        char *CurrentPathName = s_SetfsectPathName;
        if (strlen(CurrentPathName) == strlen(PathName) && strstr(CurrentPathName, PathName) == CurrentPathName && s_SetfsectBuffer == NomBuf)
          ((struct SetfsectHTabObj *)Object)->PathName = s_SetfsectPathName; }
      if ( ! ((struct SetfsectHTabObj *)Object)->PathName) {
        ((struct SetfsectHTabObj *)Object)->PathName = (char *)JotMalloc(strlen(PathName)+1);
        strcpy(((struct SetfsectHTabObj *)Object)->PathName, PathName);
        s_SetfsectPathName = ((struct SetfsectHTabObj *)Object)->PathName;
        s_SetfsectBuffer = NomBuf; } }
    Object->Bytes = Bytes;
    Object->Seek = Seek;
    return 0; }

  case H_data:    //data - creates an entry for stack-frame object.
  case H_setdata: //setdata - optionally creates the object and defines it's value from the stack.
    {
    ENTRY *FoundVal, SearchVal;
    struct DataHTabObj *Object = NULL, *OldObj = NULL;
    char *KeyCopy;
    char *LastPathElem = NULL, Path[StringMaxChr];
    int PathLen = 0;
    struct intFrame *FinalFrame;
    
    if (Operation == H_data || (*ThisArg)->next) { //Create the data object - if there's more than one arg the first (and only) int arg is the -new qualifier.
      if (Operation == H_setdata) //Skip past this arg.
        FetchIntArgFunc((struct AnyArg **)ThisArg);
      if (FetchStringArgPercent(Path, &PathLen, ThisArg))
        return Fail("Syntax in hashtable path expression.");
      NomBuf = QueryPath(Path, NomBuf, &LastPathElem, &FinalFrame, &OldObj);
      if (OldObj && OldObj->type != HTabObj_Zombie) //Allow zombies.
        return Fail("Hashtable in buffer %c already has an entry for key \"%s\"", NomBuf->BufferKey, LastPathElem);
      Object = (struct DataHTabObj *)JotMalloc(sizeof(struct DataHTabObj));
      Object->type = HTabObj_Data;
      Object->Frame = NULL;
      Object->ParentBuf = NomBuf;
      if (OldObj && OldObj->type == HTabObj_Zombie) {
        struct AnyHTabObj *OriginalSecondaryObj = ((struct ZombieHTabObj *)OldObj)->NewObj;
        if (OriginalSecondaryObj)
          Zombify(NomBuf, (struct AnyHTabObj *)OriginalSecondaryObj);
        ((struct ZombieHTabObj *)OldObj)->NewObj = (struct AnyHTabObj *)Object;
        Object->HashKey = OldObj->HashKey;
        OldObj->HashKey = NULL;
        Object->next = OldObj->next;
        OldObj->next = (struct AnyHTabObj *)Object; }
      else {
        if ( ! NomBuf->htab) {
          free(Object);
          return Fail("Buffer ( %c ) has no hashtable.", NomBuf->BufferKey); }
        KeyCopy = (char *)JotMalloc(strlen(LastPathElem)+1);
        strcpy(KeyCopy, LastPathElem); 
        Object->HashKey = KeyCopy;
        Object->next = NomBuf->FirstObj;
        NomBuf->FirstObj = (struct AnyHTabObj *)Object;
        SearchVal.key = KeyCopy;
        SearchVal.data = (void *)Object;
        if ( ! hsearch_r(SearchVal, ENTER, &FoundVal, NomBuf->htab)) {
          Fail("Failed to create data item \"%s\" in hashtable of buffer %c", KeyCopy, NomBuf->BufferKey);
          Zombify(NomBuf, (struct AnyHTabObj *)Object);
          return 1; } } }
      else if (FetchStringArgPercent(Path, &PathLen, ThisArg))
        return Fail("Syntax in hashtable path expression.");
    if (Operation == H_setdata)
      return SetDataObject(Path);
    
    return 0; }

  case H_getdata: { //data-object value is copied to stack.
    struct Buf * ThisBuf;
    char Path[StringMaxChr], *LastPathElem; 
    struct intFrame *FinalFrame = NULL;
    
    if (FetchStringArgPercent(Path, NULL, ThisArg)) {
      Fail("Syntax in hashtable path expression.");
      return 1; }
    if (s_StackSize <= s_StackPtr)
      return Fail("Stack overflow");
    if ( ! (ThisBuf = QueryPath(Path, s_CurrentBuf, &LastPathElem, &FinalFrame, NULL)) || ! FinalFrame )
      return 1;
    memcpy(&s_Stack[s_StackPtr++], FinalFrame, sizeof(struct intFrame));
    return 0; }

  case H_jump: { //jump - looks up item in table and changes context.
    struct Buf *TargetBuf;
    char *LastPathElem, Path[StringMaxChr];
    int PathLen = 0;
    
    if (FetchStringArgPercent(Path, &PathLen, ThisArg)) {
      Fail("Syntax in hashtable path expression.");
      return 1; }
    if ( ! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem, NULL, NULL)) )
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
      
    TargetBuf = QueryKey(LastPathElem, NomBuf);
    if (TargetBuf)
      s_CurrentBuf = TargetBuf;
    return TargetBuf == NULL; }

  case H_call: { //call - compiles and runs specified routine.
    struct CodeHeader *CodeHeader;
    struct Seq *DeferredSeq;
    struct BacktraceFrame ThisBacktraceFrame;
    int CallStatus;
    char FunctionName[StringMaxChr], Args[StringMaxChr];
    struct Buf *ArgsBuf;
    int NameLen = 0, ArgsLen = 0, OneLineQual;
    
    FunctionName[0] = '\0';
    if (NomBufKey == '\0')
      NomBuf = s_CurrentBuf;
    else if ( ! (NomBuf = GetBuffer(NomBufKey,  NeverNew)))
      return 1;  
    OneLineQual = FetchIntArgFunc((struct AnyArg **)ThisArg);
    if (FetchStringArgPercent(FunctionName, &NameLen, ThisArg)) {
      Fail("Syntax in fuction name.");
      return 1; }
    if ( ! (CodeHeader = QueryCode(FunctionName, NomBuf))) {
      Fail("Function \"%s\" not found in code repository ( %c )", FunctionName, NomBuf->BufferKey);
      return 1; }
    //Named function exists.
    FetchStringArgPercent(Args, &ArgsLen, ThisArg);
    if (ArgsLen) { //Some args have beem given.
      int IsDollar = s_CurrentBuf->BufferKey == '$';
      if ( ! (ArgsBuf = GetBuffer('$', AlwaysNew)))
        return Fail("Can't copy args to $ buffer.");
      GetRecord(ArgsBuf, ArgsLen+1, Args);
      if (IsDollar)
        s_CurrentBuf = ArgsBuf; }
   
    if ( ! CodeHeader) {
      Fail("Undefined function");
      return 0; }
    if (CodeHeader->CompiledCode)
      DeferredSeq = CodeHeader->CompiledCode;
    else { //Currently no compiled code for this function.
      struct Rec *OrigRec = NomBuf->CurrentRec;
      int OrigLineNumber = NomBuf->LineNumber, OrigCurrentByte = NomBuf->CurrentByte;
      NomBuf->CurrentRec = CodeHeader->SourceRec;
      NomBuf->SubstringLength = 0;
      NomBuf->CurrentByte = 0;
      NomBuf->LineNumber = 1;
      DeferredSeq = (struct Seq *)JotMalloc(sizeof(struct Seq));
      JOT_Sequence(NomBuf, DeferredSeq, OneLineQual);
      CodeHeader->CompiledCode = DeferredSeq;
      NomBuf->CurrentRec = OrigRec;
      NomBuf->LineNumber = OrigLineNumber;
      NomBuf->CurrentByte = OrigCurrentByte; }
    s_BacktraceFrame->LastCommand = CurrentCommand;
    ThisBacktraceFrame.LastCommand = NULL;
    ThisBacktraceFrame.prev = s_BacktraceFrame;
    ThisBacktraceFrame.FirstRec = CodeHeader->SourceRec;
    ThisBacktraceFrame.type = CallFrame;
    s_BacktraceFrame = &ThisBacktraceFrame;
    s_TraceLevel++;
    if (s_TraceMode & Trace_Functions)
      JotTrace("Function call.");
    CallStatus = Run_Sequence(DeferredSeq);
    if (CallStatus && (s_TraceMode & Trace_FailMacros) ) {
      s_BacktraceFrame->LastCommand = CurrentCommand;
      JotTrace("Function failure."); }
    s_TraceLevel--;
    s_BacktraceFrame = ThisBacktraceFrame.prev;
    s_CurrentCommand = NULL;
    if (s_Return) {
      s_Return = FALSE;
      s_ExitBlocks = 0; }
    return CallStatus; }

  case H_fix: { //fix - corrects line numbers in hashtable entries.
    struct Rec *Record;
    int LineNumber = 1;
    
    ResetBuffer(NomBuf);
    Record = NomBuf->FirstRec;
    do {
      struct AnyTag *ThisTag = Record->TagChain;
      while (ThisTag) {
        if (ThisTag->type == TagType_Target) //This record is a hash-table target.
          ((struct JumpHTabObj *)ThisTag->Attr)->LineNumber = LineNumber;
        ThisTag = ThisTag->next; }
      Record = Record->next;
      LineNumber++; }
      while (Record != NomBuf->FirstRec);
    
    return 0; }

  case H_delete: { //delete - removes item from the table.
    char *LastPathElem, Path[StringMaxChr];
    ENTRY *FoundVal, SearchVal;
    int PathLen = 0;
    
    if (FetchStringArgPercent(Path, &PathLen, ThisArg)) {
      Fail("Syntax in hashtable path expression.");
      return 1; }
    if ( ! Path[0])
      return 1;
    if ( ! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem, NULL, NULL)) )
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
    SearchVal.key = LastPathElem;
    SearchVal.data = NULL;
  
    if ( ! hsearch_r(SearchVal, FIND, &FoundVal, NomBuf->htab)) {
      Fail("Failed to find item \"%s\" in hashtable of buffer %c", LastPathElem, NomBuf->BufferKey);
      return 1; }
    else {
      struct AnyHTabObj *Object = (struct AnyHTabObj *)FoundVal->data;
      Zombify(NomBuf, Object);
      return 0; } }

  case H_testkey: { //testkey - verify the key.
    char *LastPathElem, Path[StringMaxChr];
    ENTRY *FoundVal, SearchVal;
    struct intFrame *FinalFrame;
    int PathLen = 0;
    int TypeQual = FetchIntArgFunc((struct AnyArg **)ThisArg);
    
    if (FetchStringArgPercent(Path, &PathLen, ThisArg)) {
      Fail("Syntax in hashtable path expression.");
      return 1; }
    if (Path[0]) {
      if ( ! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem, &FinalFrame, NULL)) )
        return 1; }
    else
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
    SearchVal.key = LastPathElem;
    SearchVal.data = NULL;
    
    if ( ! hsearch_r(SearchVal, FIND, &FoundVal, NomBuf->htab) || ! FoundVal) {
      Fail("Failed to find item \"%s\" in hashtable of buffer %c", LastPathElem, NomBuf->BufferKey);
      return 1; }
    if (TypeQual == H_typeQual) {
      long long Type = ((struct AnyHTabObj *)FoundVal->data)->type;
      PushInt(Type);
      return 0; }
      
    if ( ! FoundVal || ((struct AnyHTabObj *)FoundVal->data)->type == HTabObj_Zombie) { //A deleted entry.
      Fail("Entry for %s has been deleted.", LastPathElem);
      return 1; }
    return 0; }

  case H_destroy: { //destroy - frees the hashtable and unprotects records.
    int Qual = FetchIntArgFunc((struct AnyArg **)ThisArg);
    char Path[StringMaxChr];
    char *LastPathElem;
    int PathLen = 0;
    struct intFrame *FinalFrame;
    
    if (FetchStringArgPercent(Path, &PathLen, ThisArg)) {
      Fail("Syntax in hashtable path expression.");
      return 1; }
    
    if (Qual == H_allQual) {
      struct Buf *ThisBuf = s_BufferChain;
      while (ThisBuf) {
        DestroyHtab(ThisBuf);
        ThisBuf = ThisBuf->NextBuf; }
      return 0; }
    else {
      if ( ! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem, &FinalFrame, NULL)) )
        return 1;
      DestroyHtab(NomBuf); }
    return 0; }
  
  default: //Can't happen.
    RunError("Unrecognized %H qualifier"); 
    return 1; }
    
  return 1; }

//----------------------------------------------SetDataObject
int SetDataObject(char *Path)
  { //The Path specifies a object to be set to the value at the top of the stack.
  char *LastPathElem;
  struct DataHTabObj *ParentHTabObj;
  struct Buf *ParentBuf,  *BufFromStack = NULL;
  struct intFrame * TopItem = &s_Stack[s_StackPtr-1];
  int BufAtTop, LocalFail = FALSE;
  struct intFrame *FinalFrame;
  
  if (s_StackPtr < 1) {
    LocalFail = Fail("Stack underflow");
    return TRUE; }
  BufAtTop = ((struct intFrame *)TopItem)->type == FrameType_Buf;
  if ( ! (ParentBuf = QueryPath(Path, s_CurrentBuf, &LastPathElem, &FinalFrame, NULL)) )
    return TRUE;
  if ( ! (ParentHTabObj = (struct DataHTabObj *)QueryData(LastPathElem, ParentBuf)) ) 
    return TRUE;
  if (BufAtTop) {
    BufFromStack = ((struct bufFrame *)TopItem)->BufPtr;
    if (CheckCircularHier(BufFromStack)) {
      LocalFail = Fail("That assignment would result in a circular hierarchy;");
      return TRUE; }
    if (BufFromStack->ExistsInData) {
      LocalFail = Fail("That buffer is alreay used to define a DataHTabObj buffer.");
      return TRUE; }
    if (BufFromStack->ParentObj) {
      LocalFail = Fail("The buffer from stack is already a member of a data-object tree");
      return TRUE; } }
  if ( ! ParentHTabObj->Frame)
    ParentHTabObj->Frame = (struct intFrame *)JotMalloc(sizeof(struct intFrame));
  else { //Some preexisting frame.
    struct bufFrame *OldItem = (struct bufFrame *)ParentHTabObj->Frame;
    if (OldItem->type != TopItem->type) {
      LocalFail = Fail("Type mismatch, the data object and the top item on the stack do not match.");
      return TRUE; }
    if (OldItem->type == FrameType_Buf) { //There's a preexisting buffer hanging off the data object - free it now.
      struct Buf *OrigBuffer = OldItem->BufPtr;
      if (OrigBuffer != BufFromStack) {
        OldItem->BufPtr->ExistsInData = FALSE;
        if (s_CurrentBuf != OrigBuffer) {
          if (FreeBuffer(OrigBuffer)) {
            LocalFail = Fail("Attempt to free a read-only or locked buffer.");
            return TRUE; } } } } }
  if (BufAtTop) {
    BufFromStack->ParentObj = ParentHTabObj;
    BufFromStack->ExistsInData = TRUE; }
  memcpy(ParentHTabObj->Frame, TopItem, sizeof(struct intFrame));
  LocalFail = KillTop();
  
  return LocalFail; }

//----------------------------------------------DestroyHtab
void DestroyHtab(struct Buf *TextBuf)
  { //Destroys the specified h-list and it's children.
  struct AnyHTabObj *ThisAnyHTabObj;
  
  if (TextBuf->htab) {
    char *PendingPathName = NULL;
    
    ThisAnyHTabObj = TextBuf->FirstObj;
    while(ThisAnyHTabObj) {
      struct AnyHTabObj *NextObj = (struct AnyHTabObj *)ThisAnyHTabObj->next;
      if (ThisAnyHTabObj->type == HTabObj_Setfilesect || (ThisAnyHTabObj->type == HTabObj_Zombie && ((struct ZombieHTabObj *)ThisAnyHTabObj)->WasSetFSectHTabObj) ) { 
        //A setfsect object, or a zombie setfsect - free the pathName buffer whenever this is a new pathname.
        char *ThisPathName = ((struct SetfsectHTabObj *)ThisAnyHTabObj)->PathName;
        if (ThisPathName != PendingPathName) {
          if (PendingPathName)
            free(PendingPathName);
          PendingPathName = ThisPathName; } }
      if (ThisAnyHTabObj->HashKey)
        free(ThisAnyHTabObj->HashKey);
      Zombify(TextBuf, ThisAnyHTabObj);
      ThisAnyHTabObj = NextObj; }
    if (PendingPathName) {
      free(PendingPathName);
      PendingPathName = NULL; }
      
    //Zombify() frees the objects children but the ; object is only set to be a zombie - this next loop frees these zombies.
    ThisAnyHTabObj = TextBuf->FirstObj;
    while(ThisAnyHTabObj) {
      struct AnyHTabObj *NextObj = (struct AnyHTabObj *)ThisAnyHTabObj->next;
      free(ThisAnyHTabObj);
      ThisAnyHTabObj = NextObj; }
      
    hdestroy_r(TextBuf->htab);
    free(TextBuf->htab);
    TextBuf->htab = NULL;
    TextBuf->FirstObj = NULL;
    TextBuf->HashtableMode = NoHashtable;
    TextBuf->UnchangedStatus &= !SameSinceIndexed;
    
    s_SetfsectPathName = NULL; } }

//----------------------------------------------Zombify
void Zombify(struct Buf *TextBuf, struct AnyHTabObj *ThisObj)
  { //Frees the objects children and zombifies the object.
  if(ThisObj) {
    if (ThisObj->type == HTabObj_Jump) { //A find object.
      struct Buf *TargetBuf = ((struct JumpHTabObj *)ThisObj)->TargetBuf;
      FreeTarget(TargetBuf, (struct JumpHTabObj *)ThisObj);
      ((struct JumpHTabObj *)ThisObj)->TargetRec = NULL; }
    else if (ThisObj->type == HTabObj_Data) { //A Data object.
      struct bufFrame *Frame = (struct bufFrame *)((struct DataHTabObj *)ThisObj)->Frame;
      if (Frame && Frame->type == FrameType_Buf) { //A buffer frame - delete the buffer.
        struct Buf *Buffer = Frame->BufPtr;
        Buffer->ExistsInData = FALSE;
        Buffer->ParentObj = NULL;
        if (Buffer != s_CurrentBuf && ! SearchStackForBuf(Buffer) )
          FreeBuffer(Buffer); }
      free(((struct DataHTabObj *)ThisObj)->Frame);
      ((struct DataHTabObj *)ThisObj)->Frame = NULL;
      ((struct ZombieHTabObj *)ThisObj)->NewObj = NULL; }
    if (ThisObj->type == HTabObj_Setfilesect) {
      ((struct ZombieHTabObj *)ThisObj)->NewObj = NULL;
      ((struct ZombieHTabObj *)ThisObj)->WasSetFSectHTabObj = TRUE; }
    else if (ThisObj->type == HTabObj_Zombie)
      return;
    else if (ThisObj->type == HTabObj_Code)
      free(((struct CodeHTabObj*)ThisObj)->CodeHeader);
    else {
      if (ThisObj->type == HTabObj_Setsect) 
        ((struct ZombieHTabObj *)ThisObj)->NewObj = NULL;
      ((struct ZombieHTabObj *)ThisObj)->WasSetFSectHTabObj = FALSE; }
    ThisObj->type = HTabObj_Zombie;
    return; } }

//----------------------------------------------FreeTarget
void FreeTarget(struct Buf *TextBuf, struct JumpHTabObj *ThisJumpHTabObj)
  { //Removes links to Hash-table target tag and free's it.
  struct Rec *Record = ThisJumpHTabObj->TargetRec;
  
  if (Record) {
    struct AnyTag **PrevTagPtr = &(Record->TagChain);
    struct AnyTag *ThisTag = *PrevTagPtr;
    
    while (ThisTag) {
      if ( (ThisTag->type == TagType_Target) && (ThisTag->Attr == ThisJumpHTabObj) ) {
        *PrevTagPtr = ThisTag->next;
        ThisJumpHTabObj->Target = NULL;
        free(ThisTag);
        return; }
      PrevTagPtr = &(ThisTag->next);
      ThisTag = *PrevTagPtr; } } }

//----------------------------------------------FreeTag
void FreeTag(struct Rec *Record, struct AnyTag *TheTag)
  { //Removes any tag from Record's tag chain.
  struct AnyTag **PrevTagPtr = &(Record->TagChain);
  struct AnyTag *ThisTag = *PrevTagPtr;
  
  while (ThisTag) {
    if (ThisTag == TheTag) {
      *PrevTagPtr = ThisTag->next;
      if (TheTag->type == TagType_Text)
        free(TheTag->Attr);
      else if (TheTag->type == TagType_Target)
        ((struct JumpHTabObj *)ThisTag->Attr)->TargetRec = NULL;
      free(TheTag);
      return; }
    PrevTagPtr = &(ThisTag->next);
    ThisTag = *PrevTagPtr; } }

//----------------------------------------------QueryKey
struct Buf * QueryKey(char *Key, struct Buf *Buffer)
  { //Queries the hashtable in the specified buffer using the specified key. Returns target buffer or NULL if failure.
  ENTRY *FoundVal, SearchVal;
  struct Buf *TargetBuf = NULL;
   
  if ( ! Buffer->htab) {
    Fail("No hash-table associated with buffer %c", Buffer->BufferKey);
    return TargetBuf; }
  SearchVal.key = Key;
  SearchVal.data = NULL;
  
  if ( ! hsearch_r(SearchVal, FIND, &FoundVal, Buffer->htab))
    Fail("Failed to find item \"%s\" in buffer %c hashtab", Key, Buffer->BufferKey);
  else {
    struct JumpHTabObj *Object = (struct JumpHTabObj *)FoundVal->data;
    if (Object) {
      if (Object->type == HTabObj_Zombie) //A zombie - it may have a NewObj set though?
        if ( ! (Object = (struct JumpHTabObj *)((struct ZombieHTabObj *)Object)->NewObj) )
          return NULL;
      if (Object && (Object->type == HTabObj_Jump) ) { //A genuine JumpHTabObj
        if ( ! Object->TargetRec) //A deleted entry.
          return TargetBuf;
        if ( (TargetBuf = Object->TargetBuf) ) {
          int NewCurrentByte = Object->Target ? Object->Target->StartPoint : 0;
          int Len = strlen(Object->TargetRec->text);
          TargetBuf->CurrentRec = Object->TargetRec;
          TargetBuf->LineNumber = Object->LineNumber;
          TargetBuf->CurrentByte = (NewCurrentByte <= Len) ? NewCurrentByte : Len;
          TargetBuf->SubstringLength = 0; } } } }
    return TargetBuf; }

//----------------------------------------------QueryCode
struct CodeHeader * QueryCode(char *Key, struct Buf *Buffer)
  { //Queries the hashtable in the specified buffer using the specified key. Returns target CodeHeader or NULL if failure.
  ENTRY *FoundVal, SearchVal;
  struct CodeHeader *TargetHeader = NULL;
   
  if ( ! Buffer->htab) {
    Fail("No hash-table associated with buffer %c", Buffer->BufferKey);
    return TargetHeader; }
  SearchVal.key = Key;
  SearchVal.data = NULL;
  
  if ( ! hsearch_r(SearchVal, FIND, &FoundVal, Buffer->htab))
    Fail("Failed to find item \"%s\" in buffer %c hashtab", Key, Buffer->BufferKey);
  else {
    struct CodeHTabObj *Object = (struct CodeHTabObj *)FoundVal->data;
    if (Object->type == HTabObj_Code)
      return Object->CodeHeader; }
  return NULL; }

//----------------------------------------------QueryPath
struct Buf *QueryPath(char *Path, struct Buf *DefaultBuf, char **LastPathElem, struct intFrame **FinalFrame, struct DataHTabObj **ThisDataHTabObj)
  { //Checks and descends the hashtable hierarchy specified by Path.
    //if the FinalFrame pointer is set then it desends to the parent of the final path element and returns 
    //  with FinalFrame pointing to the frame and LastPathElem pointing to it's neme.
    //When FinalFrame is not specified, it descends the entire path.
    //In the event of an error (e.g. a path element was not found) it returns the last valid path-element buffer aith LastPathElem set to the remaining path.
    //
  char *PathElemEnd, Elem[StringMaxChr], *PathElemCopy = Elem;
  struct DataHTabObj *DataHTabObj;
  struct Buf *ParentBuf, *ThisBuf;
  ENTRY *FoundVal = NULL, SearchVal;
  
  *LastPathElem = Path;
  
  if (ThisDataHTabObj)
    *ThisDataHTabObj = NULL;
  if (Path[0] && Path[1] == '=') { //Path[0] is the root buffer key.
    if ( ! (ParentBuf = ThisBuf = GetBuffer(ChrUpper(Path[0]), OptionallyNew)) )
      return NULL;
    if ( ! ThisBuf->CurrentRec)
      GetRecord(ThisBuf, 1, "");
    *LastPathElem = Path+2; }
  else {
    ParentBuf = ThisBuf = DefaultBuf;
    *LastPathElem = Path; }
    
  while (ThisBuf->htab) {
    if ( (PathElemEnd = strchr(*LastPathElem, '|')) ) {
      strncpy(PathElemCopy, *LastPathElem, PathElemEnd-*LastPathElem);
      PathElemCopy[PathElemEnd-*LastPathElem] = '\0'; }
    else
      strcpy(PathElemCopy, *LastPathElem);
    if ( ! ThisBuf->htab) //The hashtable is not set up.
      return ParentBuf;
    SearchVal.key = PathElemCopy;
    SearchVal.data = NULL;
    
    if ( ! hsearch_r(SearchVal, FIND, &FoundVal, ThisBuf->htab)) //The last path element does not exist in the parent-buffers hashtable.
      return ThisBuf;
    if ( ! (DataHTabObj = QueryData(PathElemCopy, ThisBuf)) ) //No further path elements.
      break;
    if (DataHTabObj->type != HTabObj_Data) {
      Fail("Incorrect hashtable entry, at path element \"%s\", in path \"%s\"", PathElemCopy, Path);
      return NULL; }
    if (FinalFrame && ! PathElemEnd) {
      *FinalFrame = DataHTabObj->Frame;
      break; }
    if ( ! DataHTabObj->Frame || DataHTabObj->Frame->type != FrameType_Buf)
      break;
    ParentBuf = ThisBuf;
    ThisBuf = ((struct bufFrame *)DataHTabObj->Frame)->BufPtr;
    if ( ! PathElemEnd) {
      ThisBuf = ParentBuf;
      break; }
    *LastPathElem = PathElemEnd+1; }
    
  if (ThisDataHTabObj && FoundVal)
    *ThisDataHTabObj = (struct DataHTabObj *)FoundVal->data;
  return ThisBuf; }

//----------------------------------------------QueryData
struct DataHTabObj * QueryData(char *Key, struct Buf *Buffer)
  { //Queries the hashtable in the specified hashtable path, checks that it's a DataHTabObj and returns the struct.
  ENTRY *FoundVal, SearchVal;
   
  if ( ! Buffer->htab) {
    Fail("No hash-table associated with buffer %c", Buffer->BufferKey);
    return NULL; }
  SearchVal.key = Key;
  SearchVal.data = NULL;
  
  if ( ! hsearch_r(SearchVal, FIND, &FoundVal, Buffer->htab))
    Fail("Failed to find item \"%s\" in hashtable of buffer %c", Key, Buffer->BufferKey);
  else {
    struct DataHTabObj *Object;
    if ( ! FoundVal->data) {
      Fail("No entry associated with key \"%s\" in hashtable of buffer %c", Key, Buffer->BufferKey);
      return NULL; }
    Object = (struct DataHTabObj *)FoundVal->data;
    if (Object->type == HTabObj_Zombie) //A zombie - it may have a NewObj set though
      if ( ! (Object = (struct DataHTabObj *)((struct ZombieHTabObj *)Object)->NewObj) )
        return NULL;
    if (Object->type == HTabObj_Data) //A DataHTabObj
      return Object; }
  return NULL; }

//----------------------------------------------QuerySection
struct SetsectHTabObj * QuerySection(char *Key, struct Buf *Buffer)
  { //Queries the hashtable in the specified buffer using the specified key, checks that it's a SetsectHTabObj and returns struct.
  ENTRY *FoundVal, SearchVal;
   
  if ( ! Buffer->htab) {
    Fail("No hash-table associated with buffer %c", Buffer->BufferKey);
    return NULL; }
  SearchVal.key = Key;
  SearchVal.data = NULL;
  
  if ( ! hsearch_r(SearchVal, FIND, &FoundVal, Buffer->htab))
    Fail("Failed to find item \"%s\" in hashtable of buffer %c", Key, Buffer->BufferKey);
  else {
    struct SetsectHTabObj *Object = (struct SetsectHTabObj *)FoundVal->data;
    if (Object->type == HTabObj_Zombie) //A zombie - it may have a NewObj set though
      if ( ! (Object = (struct SetsectHTabObj *)((struct ZombieHTabObj *)Object)->NewObj) )
       return NULL;
    if (Object->type == HTabObj_Setsect) //A SetsectHTabObj
      return Object;
    else
      Fail("Item \"%s\" in hashtable of buffer %c is not a section type.", Key, Buffer->BufferKey); }
  return NULL; }

//----------------------------------------------QueryFSection
struct SetfsectHTabObj * QueryFSection(char *Key, struct Buf *Buffer)
  { //Queries the hashtable in the specified buffer using the specified key, checks that it's a SetfsectHTabObj and returns struct.
  ENTRY *FoundVal, SearchVal;
   
  if ( ! Buffer->htab) {
    Fail("No hash-table associated with buffer %c", Buffer->BufferKey);
    return NULL; }
  SearchVal.key = Key;
  SearchVal.data = NULL;
  
  if ( ! hsearch_r(SearchVal, FIND, &FoundVal, Buffer->htab))
    Fail("Failed to find item \"%s\" in hashtable of buffer %c", Key, Buffer->BufferKey);
  else {
    struct SetfsectHTabObj *Object = (struct SetfsectHTabObj *)FoundVal->data;
    if (Object->type == HTabObj_Zombie) //A zombie - it may have a NewObj set though
      if ( ! (Object = (struct SetfsectHTabObj *)((struct ZombieHTabObj *)Object)->NewObj) )
       return NULL;
    if (Object->type == HTabObj_Setfilesect) //A SetfsectHTabObj
      return Object;
    else
      Fail("Item \"%s\" in hashtable of buffer %c is not a section type.", Key, Buffer->BufferKey); }
  return NULL; }
