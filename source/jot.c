//This file contains all of the source code for (Joy Of Text - a text editor).
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
#include <io.h>
#include <WinBase.h>
#include "../wine_32/libgw32c-0.4/include/glibc/search.h"
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
#include <wchar.h>
#include <locale.h>
#include <ctype.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <sys/wait.h>
 
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
#define Command_ToggleScreen      0x00000001      //Reverts to command mode temporarily for each new hotkey.
#define Command_ScreenMode        0x00000002      //Currently in screen mode.
#define Command_OverTypeMode      0x00000004      //Typing into screen overwrites preexisting text.
#define Command_EscapeAll         0x00000008      //Each new character can initiate an escape string.
#define Command_EscapeCtrl        0x00000010      //Any control character can initiate an escape string.

#if !defined(VERSION_STRING)
#define VERSION_STRING "Test version"
#endif

#define TRUE 1
#define FALSE 0
#define StringMaxChr 1024         //The largest string that can be accommodated by most static and local strings.
#define BucketSize 1000           //Size of temporary buffer used by ReadUnbufferedStreamRec and ReadNewRecord.
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
#define Trace_SubSequence  0x0004 //Traces at start of new subsequence (new block or else clause).
#define Trace_Failures     0x0008 //Traces all failing commands.
#define Trace_FailNoElse   0x0010 //Traces failing commands with no local else sequence.
#define Trace_FailBlocks   0x0020 //Traces on exit from failed block.
#define Trace_FailMacros   0x0040 //Traces on exit from failed macro call.
#define Trace_Interrupt    0x0080 //Traces command following a Ctrl+C interrupt.
#define Trace_Recovery     0x0100 //Allows stepping through recovery files.
#define Trace_DumpSeq      0x0400 //Dumps command-sequence.
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
#define ObjType_Jump        0     //Defines a jump object.
#define ObjType_Data        1     //Defines a pointer to a data-object frame (see stack-frame-type.
#define ObjType_Setsect     2     //Defines section-start and bytecount in an already-opened file. 
#define ObjType_Setfilesect 3     //Defines file pathname as well as section-start and bytecount.
#define ObjType_Zombie      4     //hashtable-object has been deleted.
//The following objects are referenced by hashtables.
struct AnyObj {                   //Object is one of the following: JumpObj, DataObj, SetsectObj, SetfsectObj or ZombiObj (a deleted object).
  unsigned char type;             //The object type see above for type codes.
  struct AnyObj *next;            //Points to next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  };

struct ZombieObj {                //Object was deleted but there's still an entry in the hashtable.
  unsigned char type;             //The object type = 4, see above for type codes.
  struct AnyObj *next;            //Points to next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  struct AnyObj *NewObj;          //The replacement object (if one is defined).
  short int WasSetFSectObj;       //The original object was a Setfsect Obj - there may be a pathname to be freed.
  };

struct JumpObj {                  //Htab entry points to a quick-lookup object - points to a record, character and linenumber etc.
  unsigned char type;             //The object type = 0, see above for type codes.
  struct AnyObj *next;            //Points to next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  struct Rec *TargetRec;          //The jump destination record, if NULL the object is deemed to have been deleted.
  int LineNumber;                 //The jump destination line no. - only valid when buffer is unchanged since JumpObj was created.
  struct Buf *TargetBuf;          //The jump destination buffer.
  struct Buf *HashBuf;            //The hash-table buffer key.
  struct TargetTag *Target;       //The target tag for the jump.
  };

struct DataObj {                  //Htab entry points to a seek/bytes to fetch a selected part of a file.
  unsigned char type;             //The object type = 1, see above for type codes.
  struct AnyObj *next;            //The next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  struct intFrame *Frame;         //Generic stack-frame object.
  struct Buf *ParentBuf;          //Points back to parent buffer, used to determine buffer-path.
  };

struct SetsectObj {               //Htab entry points to a seek/bytes to fetch a selected part of a file.
  unsigned char type;             //The object type = 2, see above for type codes.
  struct AnyObj *next;            //The next member in the chain.
  char *HashKey;                  //The hashtable key associated with this object.
  long long Seek;                 //Byte offset from start of file to selected section.
  long long Bytes;                //Byte count of selected section of the file.
  };

struct SetfsectObj {              //Htab entry points to a seek/bytes to fetch a selected part of a file.
  unsigned char type;             //The object type = 3, see above for type codes.
  struct AnyObj *next;            //The next member in the chain.
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
  int length;                     //The maximum allowable length for this record (the original malloc allocation).
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
  struct Com *Predecessor;        //The buffer was called as a macro by Com (or NULL).
  char *PathName;                 //The original pathName.
  char *Header;                   //Header to be displayed above window.
  char *Footer;                   //Footer to be displayed in window-separator line instead of file name.
  struct DataObj *ParentObj;      //Points to object in parent-buffers hashtable.
  struct ColourObj *FirstColObj;  //A list of colour-pair objects associated with this buffer.
  struct AnyObj *FirstObj;        //Chain of Htab/Seek/DataObj structs.
  struct hsearch_data *htab;      //Hash table associated with this buffer.
  struct IOBlock *IOBlock;        //Holds I/O/data while reading, also async I/O data for asynchronous buffers (%e=&... operations pending).
#if defined(VC)
  int CodePage;                   //Code page to be used for this buffer.
#endif
  char AbstractWholeRecordFlag;   //When inserting this text  (i.e. Here command) into some other buffer, the inserted text goes at the start of the current word.
  char UnchangedStatus;           //If zero, the buffer has not been changed since it was read/indexed/compiled.
  char EditLock;                  //The requested edit permission for buffer. One of Unrestricted (the default), WriteIfChanged and ReadOnly.
  char HashtableMode;             //Defines behaviour when targets of the hashtable are to be deleted (see above for values).
  char NewPathName;               //When TRUE, the PathName has been changed - triggers an update in the display.
  char NoUnicode;                 //When TRUE, turns off unicode support.
  char FileType;                  //File type identified from FileType or defined explicitly to ReadUnbufferedChan()
  char AutoTabStops;              //Assign tabstops automatically, when TRUE.
  char ExistsInData;              //When true the buffer is in a DataObj (or a clone of one) and the buffer should not be deleted while the DataObj exists.
  char BufferKey;                 //The key associated with this buffer.
  char TabCells;                  //The indicates that text between tabs must be treated as cells not simple tabular form.
  };

//Types of arguments in an Arg struct:
#define IntegerArg 1
#define FloatingPointArg 2
#define StringArg 3
#define DeferredStringArg 4
#define BlockArg 5

struct AnyArg   {                 //A generic arg.
  char           type;            //Must be one of these: IntegerArg, FloatingPointArg, StringArg, DeferredStringArg or BlockArg.
  struct AnyArg  *next;           //Points to next arg in chain.
  };

struct IntArg {                   //An integer arg.
  char           type;            //Must be IntegerArg
  struct AnyArg  *next;           //Points to next arg in chain.
  long long      IntArg;          //Integer value
  };

struct FloatArg {                 //A floating-point arg.
  char           type;            //Must be FloatingPointArg.
  struct AnyArg  *next;           //Points to next arg in chain.
  double         FloatArg;        //Floating-point value
  };

struct PtrArg {                   //An arg that points to either a string, a deferred string or a block.
  char          type;             //Must be one of these: StringArg, DeferredStringArg or BlockArg.
  struct AnyArg *next;            //Points to next arg in chain.
  char          Key;              //Indicates a buffer key for some flavours of pointer.
  void *        pointer;          //Points to string/data-struct arg or VOID.
  };

struct Com {                      //Stores one command and pointer to argument list. 
  int          CommandKey;        //Identifies command type.
  struct AnyArg  *ArgList;        //Points to first arg in list.
  struct Com  *NextCommand;       //Points to next command in sequence.
  struct Buf  *CommandBuf;        //Points to buffer holding the source.
  struct Rec  *CommandRec;        //Points to record with command line.
  int          CommandByteNo;     //Byte no. of command in line.
  int          ComChrs;           //No. of characters in command source, including arguments.
  int          CommandLineNo;     //Line no. of command in macro.
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
#define ScriptFrame 2             //The command is from a %r=<scriptPath> N.B. *not* an -asConsole script.
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
#define WinType_Leftmost 4          //The window is the first (leftmost) slice in a group.
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
  short unsigned int PopupWidth;    //The current width of the popup.
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
 
struct IOBlock {
  struct Buf *NextAsyncBuf;         //Points to next asynchronous I/O buffer.
  struct BucketBlock *BucketBlock;  //Buffers input operations.
  int ReadyForReading;              //Normally unset by EOF, for asynchronous I/O unset by select() results.
  int HoldDesc;                     //File descriptor for normal (synchronous) reads with -hold qualifier.
  char *ExitCommand;                //The user-nominated command that causes the child to exit.
#if defined(VC)
  HANDLE InPipe;                    //Asynchronous input stream.
  HANDLE OutPipe;                   //Asynchronous output stream.
  HANDLE ProcessId;                 //The child-process ID.
#else
  int InPipe;                       //Asynchronous input stream.
  int OutPipe;                      //Asynchronous output stream.
  pid_t ChildPid;                   //Used to kill the child.
#endif
  };
  
struct BucketBlock {                //Used to buffer reads from unbuffered streams.
  int RecStartChr;                  //The start of the next record in this bucket.
  int RecEndChr;                    //The end of the next record in this bucket.
  int BufBytes;                     //The total number of bytes available in the bucket.
  struct BucketBlock *Next;         //Next bucket in chain.
  char Bucket[BucketSize];          //The read bucket.
  };

#define Attr_Normal    0
#define Attr_SelSubStr 1
#define Attr_CurrChr   2
#define Attr_WinDelim  3
#define Attr_CurrCmd   4
static int s_Attrs[5];              //Translates various jot display attributes to system display attributes.
  
#if defined(VC)
static int s_DefaultColourPair = Attr_Normal;
static int s_CodePage = 65001;          //Changed from 65001 to avoid corrupting wine session.
DWORD fdwMode, fdwOldMode; 
HANDLE hStdout, hStdin;             //The handles used by low-level I/O routines read() and write() to access stdin/stdout.
CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
WORD wOldColorAttrs; 
int OpenWindowsCommand(HANDLE *, HANDLE *, HANDLE *, int, int, char *);
BOOL Ctrl_C_Interrupt(DWORD);
int ReadBuffer(FILE *, HANDLE *, int, int, char *, struct Buf *, int, off64_t, long *, int, int);
int ReadBucketFromHandle(HANDLE *, char *, int);
int ReadUnbufferedStreamRec(struct Buf *, int, HANDLE *, int, long *);
#else

int RegexSearchBuffer(struct Buf *, char, char *, int );
static int s_DefaultColourPair;
static char *s_Locale;
void Ctrl_C_Interrupt(int);
int ReadBuffer(FILE *, int, int, char *, struct Buf *, int, off64_t, long *, int, int);
int ReadUnbufferedStreamRec(struct Buf *, int, int, long *);
void DoublePopen(int *, int *, pid_t *, struct Buf *, char *);
#endif

int GetBufferPath(struct Buf *, char * , int);
void TextViaJournal(struct Buf *, char *);
void Zombify(struct Buf *, struct AnyObj *);
void FreeTarget(struct Buf *, struct JumpObj *);
void FreeTag(struct Rec *, struct AnyTag *);
int ReadNewRecord(struct Buf *, FILE *, int);
int MoveDown(struct Buf *, int, char *);
int Backtrace(struct Buf *);
int Run_Sequence(struct Seq *);
int Run_Block(struct Block *);
int GetChrNo(struct Buf *);
char CheckNextCommand(struct Buf *, char *);
struct Block *JOT_Block(struct Block *, struct Buf *, int);
void StartEdit();
void ExpandEnv(char *, int);
void DestroyHtab(struct Buf *);
int AddFormattedRecord(int, struct Buf *, char *String, ...);
int JotGetCh(FILE *);

void DumpSequence(struct Seq *, int *, char *);
void DumpBlock(struct Block *, int *, char *);
void DumpCommand(struct Com *, int *, char *);
struct Buf *QueryKey(char *, struct Buf *);
struct Buf *QueryPath(char *, struct Buf *, char **);
struct DataObj * QueryData(char *, struct Buf *);
struct SetsectObj * QuerySeek(char *, struct Buf *);
struct SetsectObj * QuerySection(char *, struct Buf *);
struct SetfsectObj * QueryFSection(char *, struct Buf *);
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
int ExpandDeferredString(struct Buf *, char *, int);
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
int GetRecord(struct Buf *, int);
int CheckBufferKey(char );
void *JotMalloc(int);
struct Buf *GetBuffer(int, int);
int FreeBufferThings(struct Buf *);
void ClearBuffer(struct Buf *);
void CopyBuffer(struct Buf *, struct Buf *);
char ImmediateCharacter(struct Buf *);
int JoinRecords(struct Buf *);
int ShiftRight(struct Buf *, int);
void MoveRecord(struct Buf *, struct Buf *);
int Abstract(struct Buf *, struct Buf *, char );
char NextCommand(void);
char NextNonBlank(struct Buf *);
void ResetBuffer(struct Buf *);
int SearchRecord(struct Buf *, char *, int Direction);
void SetPointer(struct Buf *, int);
char StealCharacter(struct Buf *);
int SubstituteString(struct Buf *, const char *, int);
int VerifyAlpha(struct Buf *);
int VerifyCharacter(struct Buf *, int);
int VerifyDigits(struct Buf *);
int VerifyNonBlank(struct Buf *);
int ChangeWindows(char, struct Buf *);
void JotBreak(char *);
void JotTrace(char *);
int JotTidyUp();
int TidyUp(int, char *);
int SynError(struct Buf *, char *, ...);
int Fail(char *, ...);
int RunError(char *, ...);
void Disaster(char *, ...);
void Message(struct Buf *, char *, ...);
void JotDebug(struct Buf *, char *, ...);
void PriorityPrompt(char *, ...);
void CommandPrompt(char *, ...);
void Prompt(char *, ...);
void InitializeEditor(int, char *[]);
int Noise(void);
int SwitchToComFile(char *, char *, char *);
char ReadCommand(struct Buf *, FILE *, char *, ...);
int TransEscapeSequence(struct Buf *, FILE *, signed char, char *);
int AnalyzeTabs(struct Buf *, struct Rec *, int);
int WriteString(char *, int, int, int, int, int, int, int [], int, struct Window *);
int JotUpdateWindow(void); 
void ScrollConsole(int, int);
char VerifyKey(int);
int NewCommandFile(FILE *, char *);
int EndCommandFile();
int ChrUpper(char);
void ExitNormally(char *);
void FreeCommand(struct Com **);
void FreeSequence(struct Seq **);
void FreeBlock(struct Block **);
long long GetArg(struct Buf *, long long);
void AddBlock(struct Com *, struct Block *);
void AddIntArg(struct Com *, long long);
void AddFloatArg(struct Com *, double);
void AddStringArg(struct Com *, struct Buf *);
int AddPercentArg(struct Com *, struct Buf *);
long long FetchIntArg(struct IntArg **);
double FetchFloatArg(struct FloatArg **);
struct Block *FetchBlock(struct PtrArg **);
char *FetchStringArg(struct PtrArg **, char *);
int DoHash(char *, char, int, int, int );
int QuerySomething(char *, char, int, int, int);
void JOT_Sequence(struct Buf *, struct Seq *, int);
int ReadString(char *, int, FILE *);
int CrossmatchPathnames(int *, FILE **, char [], char *, char *, char *, char *);
void DisplayDiag(struct Buf *, int, char *);
long long PopInt(int *);
double PopFloat(int *);
int ParseSort(char *, int );
  
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
static int s_JournalDataLines;                        //Counts data lines in journal, only used for <<Debugger on>> events.
static int s_RecoveryMode = FALSE;                    //Disables %O writing and %I reads path from recovery script when set.
static int s_OriginalSession = TRUE;                  //Reset on entry to recovery mode, prevents deletion of journal files on exit from recovery session.
static struct Buf *s_InitCommands = NULL;             //Buffer holding -init command sequence.
static int s_HoldScreen = FALSE;                      //Set by -Hold - holds the screen on exit.

static struct intFrame *s_Stack; 
static struct Window *s_FirstWindow;
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
#define Verbose_DebugMessages 64                      //Allows JotDebug() to emit messages.
static int s_Verbose = 3;                             //Current verbosity level.
static char s_TableSeparator = '\t';                  //Used to identify tabular-text boundaries.
static int s_CommandMode = 0;                         //Goes non-zero for insert-mode.
static int s_StackSize = 100;                         //The maximum size of the main operand stack.
static int s_StackPtr;                                //Points to next item on the operand stack.
static int s_CaseSensitivity;                         //Set to TRUE for case-sensitive searches.
static struct Buf *s_BufferChain;                     //First member of a linked-list of all buffers.
static struct Buf *s_CommandBuf;                      //The source-code buffer currently being scanned for commands.
static struct Com *s_CurrentCommand = NULL;           //The currently-executed command object.
static struct Rec *s_PrevCommandRec;      ;           //For implementation of Trace_CommandLines.
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
static int s_BacktraceDone;                           //Flag set when the top frame has been backtraced - prevents further backtraces as the call stack unwinds.
static int s_TraceLevel;                              //Counts macro/command-file/function depth
static int s_TraceSkipped;                            //Indicates that s_MaxTraceLevel has been exceeded at least once.
static int s_MaxTraceLevel;                           //When +ve, suppresses tracing when s_TraceLevel exceeds this level.
static int s_TTYMode;                                 //Set TRUE when in teletype (non-window) mode.
static int s_TermHeight = 0;                          //The current-terminal height.
static int s_TermWidth = 0;                           //The current-terminal width.
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
static char s_SearchString[StringMaxChr];
static char s_RegExString[StringMaxChr];
static char s_QualifyString[StringMaxChr];
static char s_PromptString[StringMaxChr];
struct Buf *s_OnKey = NULL;
static char s_ArgString[StringMaxChr];
static int s_Y_ColumnNo = -1;                        //Column no. used by Y command, -1 indicates it may be set.
static int s_NoteLine;                               //Line no. of latest note point.
static int s_NoteByte;                               //Byte offset to latest note point.
static struct Buf *s_NoteBuffer = NULL;
static struct Buf *s_TempBuf;                        //Used for expanding deferred arguments.
static struct Buf *s_DebugBuf;                       //Command buffer for the jot debugger.
static int s_AbstractWholeRecordFlag = TRUE;
static int s_ExitBlocks;                             //Counts no. of blocks to be exited following an X command.
static int s_Return;                                 //Implementation of X0 command - early RETURN from outermost Macro, Function or Script.
 
static int s_InView;
struct Buf *s_FirstAsyncBuf;                         //Points to first buffer using asynchronous I/O.
int AsyncReadRecs();

#if defined(LINUX)
static WINDOW *mainWin = NULL;                       //In non-curses implementations this is set to -1 - assumed to be NULL in s_TTYMode.
short s_NextColourPair;                              //The next-available colour-pair ID number.
fd_set s_AsyncFDs;                                   //List of active asynchronous handles - currently only .
int s_TopAsyncFD = 0;                                //Highest-numbered member of s_AsyncFDs - used by select().
int TestAsyncPipe(struct Buf *, int);
#endif

//Definitions for %Q - QuerySomething()
#define Q_version  1                     //version - print version or send it to a buffer.
#define Q_windows  2                     //windows - fails if not operating under windows.
#define Q_linux  3                       //linux fails if not operating under linux.
#define Q_case  4                        //case - fails if not case sensitive.
#define Q_system  5                      //System settings etc.
#define Q_hashtables  6                  //hashtables - find buffers with hashtables, list internal and external references and target buffers
#define Q_wd  7                          //wd - return current directory only.
#define Q_heap  8                        //heap - report heap stats to stdout.
#define Q_backtrace  9                   //Writes diagnostic backtrace to nominated buffer.
#define Q_history 10                     //history - dumps history to specified buffer.
#define Q_tabstops 11                    //tabstops - pushes TRUE if tabstops or tabcells are set otherwise FALSE.
#define Q_tabcells 12                    //tabcells - pushes TRUE if tabcells are set otherwise FALSE.
#define Q_keys 13                        //keys - sends a list of keys and truncated endpoint strings to nominated buffer.
#define Q_key 14                         //key - reports as above but only for selected key.
#define Q_tags 15                        //tags - sends a list of record tags in current buffer to nominated buffer.
#define Q_here    1                      //Report only tags active at current character position.
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

#define H_create 1     //create - initialize the table with specified no. of elements.
#define H_adjustQual 0     
#define H_destroyQual 1    
#define H_protectQual 2    
#define H_deleteQual 3     
#define H_add 2        //add - redefines or puts new item into table.
#define H_new 3        //new - puts new item into table.
#define H_setfsect 4   //setfsect - adds FileHand, seek offset and byte count for %i ... -section=...; N.B. does not consume the stack values.
#define H_setsect 5    //setsect - adds seek offset and byte count fo %i ... -section=...; N.B. does not consume the stack values.
#define H_data 6       //setdata - creates an entry for stack-frame object.
#define H_jump 7       //jump - looks up item in table and switches context.
#define H_call 8       //call - compiles and runs specified routine.
#define H_fix 9        //fix - corrects line numbers in hashtable entries.
#define H_delete 10    //delete - removes item from the table.
#define H_testkey 11   //testkey - verify that there is at least one key matching the given string,
#define H_destroy 12   //destroy - frees the hashtable and unprotects records.
#define H_allQual 1       

//----------------------------------------------TidyUp
int TidyUp(int status, char *ExitMessage)
  { // Editor-only TidyUp procedure called by Tripos. 
  if (JotTidyUp()) //Only fails if there are writeifchanged buffers to be saved.
    return 0;
  if (ExitMessage && s_Verbose) {
    if (s_Verbose & Verbose_ReportCounter)
      fprintf(stdout, "\n(%lld)%s\n", s_CommandCounter, ExitMessage);
    else
      fprintf(stdout, "\n%s\n", ExitMessage); }
  exit(status);
  } // TidyUp procedure ends.

//----------------------------------------------main
int main(int ArgC, char *ArgV[])
  { // Outer procedure - calls editor. 
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
  s_Attrs[Attr_Normal] =     ( FOREGROUND_INTENSITY | FOREGROUND_GREEN  | FOREGROUND_BLUE  | FOREGROUND_RED );
  s_Attrs[Attr_SelSubStr] =  ( FOREGROUND_INTENSITY | FOREGROUND_RED );
  s_Attrs[Attr_CurrChr] =    ( FOREGROUND_INTENSITY | BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
  s_Attrs[Attr_WinDelim] =   (                                                        BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE  | BACKGROUND_RED );
  s_Attrs[Attr_CurrCmd] =    ( FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_GREEN  | BACKGROUND_BLUE  | BACKGROUND_RED );
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
  s_TempBuf = GetBuffer('t', AlwaysNew);
  GetRecord(s_TempBuf, StringMaxChr);
  s_DebugBuf = GetBuffer('d', AlwaysNew);
  GetRecord(s_DebugBuf, StringMaxChr);
  s_InsertString[0] = '\0';
  s_SearchString[0] = '\0';
  s_RegExString[0] = '\0';
  s_QualifyString[0] = '\0';
  s_StackSize = StackSize;
  s_StackPtr = 0;
  s_asConsole = NULL;
  s_ExitBlocks = 0;
  
  while (++Arg < ArgC) // Argument extraction loop.
    if (ArgV[Arg][0] != '-') { //Argument looks like a filespec.
      if (FileName != NULL)
        Disaster("Multiple filenames \"%s\" and \"%s\"\n", FileName, ArgV[Arg]);
      else
        FileName = ArgV[Arg]; }
  
    else {
      int EqChr;
      
      for (EqChr = 0; EqChr <= strlen(ArgV[Arg]); ) 
        if (ArgV[Arg][EqChr++] == '=')
          break;
      
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
          s_InitCommands = GetBuffer('i', OptionallyNew);
          if (s_InitCommands->CurrentRec == NULL)
            GetRecord(s_InitCommands, 1);
          for ( ; ; ) { //Identify any further args that form part of the init string.
            int ArgLength;
            s_InitCommands->SubstringLength = 0;
            s_InitCommands->CurrentByte = strlen(s_InitCommands->CurrentRec->text);
            SubstituteString(s_InitCommands, ArgV[Arg]+EqChr, -1);
            ArgLength = strlen(s_InitCommands->CurrentRec->text)-1;
            if ( (s_InitCommands->CurrentRec->text[ArgLength] == '\\') && (++Arg <= ArgC) ) {
              s_InitCommands->CurrentRec->text[ArgLength] = ' ';
              s_InitCommands->CurrentByte = ArgLength+1; }
            else
              break;
            EqChr = 0; }
          s_InitCommands->CurrentByte = 0;
          break;
           
        case 'S': // 'S' commands switch.
          switch (toupper(ArgV[Arg][2]))
            {
          case 'C': // -SCreensize size follows.
            sscanf(ArgV[Arg]+EqChr, "%ix%i", &s_TermWidth, &s_TermHeight);
            s_PredefinedScreenSize = TRUE;
            break;
          case 'T':
            if (toupper(ArgV[Arg][4]) == 'C') //-STACksize=<n> - set the operand stack size.
              sscanf(ArgV[Arg]+EqChr, "%d", &s_StackSize);
            else { // -STartup file name follows, optionally, followed by args.
              if (strlen(ArgV[Arg]) <= EqChr) {
                free(s_StartupFileName);
                s_StartupFileName = NULL; }
              else { //Startup pathname specified.
                free(s_StartupFileName);
                s_StartupFileName = (char *)JotMalloc(strlen(ArgV[Arg]+EqChr)+1);
                strcpy(s_StartupFileName, ArgV[Arg]+EqChr); }
              break; } }
          break;
      
        case 'T':
          switch (toupper(ArgV[Arg][2])) { // 'T' commands switch.
            case 'O': // -To = <O/P filespec.
              ToFile = ArgV[Arg]+EqChr;
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
          sscanf(ArgV[Arg]+EqChr, "%ix", &s_CodePage);
          break;
#else
        case 'L': //-Locale=<locale> - for a specific locale setting.
          s_Locale = ArgV[Arg]+EqChr;
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
            sscanf(ArgV[Arg]+EqChr, "%d", &CommandBufSize); 
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
    strcpy(s_JournalPath, "./");
    strcat(s_JournalPath, (NameElem = strrchr(FileName, '/')) ? NameElem+1 : FileName);
    strcat(s_JournalPath, ".jnl/");
    strcpy(LockFilePathname, s_JournalPath);
    strcat(LockFilePathname, "/LOCK");

    if ( ! stat(LockFilePathname, &Stat)) //LOCK exists - exit now.
      Disaster("Detected a lock file ( %s ) - original session did not close down normally", LockFilePathname);
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
        rmdir(s_JournalPath); }
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
        if(rmdir(s_JournalPath))
          printf("Failed to delete journal directory %s\n", s_JournalPath); }
    mousemask(0, NULL);
#endif
    }

    if (JotMkdir(s_JournalPath, 00777))
      Disaster("Can't create journal directory %s\n", s_JournalPath);
#if defined(VC)
    chmod(s_JournalPath, 00777);
#endif
    strcpy(FullPath, s_JournalPath);
    strcat(FullPath, "history.txt");
    Actual[0] = '\0';
    CrossmatchPathnames(NULL, &s_JournalHand, "w", FullPath, NULL, NULL, Actual);
    if ( s_JournalHand == NULL)
      Disaster("Failed to open journal file \"%s\"", FullPath);
    close(open(LockFilePathname, 0700)); }

#if defined(VC)
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if ( ! GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) 
      Fail("GetConsoleScreenBufferInfo: stdout Console Error"); 
    SrWindow = csbiInfo.srWindow;
    if ( ! s_PredefinedScreenSize) {
      s_TermWidth = SrWindow.Right-SrWindow.Left+1;
      s_TermHeight = SrWindow.Bottom-SrWindow.Top+1; }
#else
  if ( ! s_TTYMode) { //Using screen attributes - curses/windows screen initialization happens here.
    mainWin = initscr();
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
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
      
  s_FirstAsyncBuf = NULL;
#endif
  //Read file/stdin or create an empty file image.
  if (s_NewFile) { //Create a new file.
    s_CurrentBuf = GetBuffer('.', AlwaysNew);
    s_CurrentBuf->LineNumber = 1;
    GetRecord(s_CurrentBuf, StringMaxChr);
    GetRecord(s_CurrentBuf, StringMaxChr);
    s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec;
    ToFile = FileName; }
  else {
    if (s_StreamIn) { // Reading from stdin stream.
      s_CurrentBuf = GetBuffer('.', AlwaysNew);
#if defined(VC)
      ReadBuffer(stdin, NULL, 0, FALSE, "-", s_CurrentBuf, 0, 0, NULL, FALSE, 0);
#else
      ReadBuffer(stdin, 0, FALSE, "-", s_CurrentBuf, 0, 0, NULL, FALSE, 0);
#endif
      }
    else { //Reading from a real file.
#if defined(VC)
      int FileDesc = 0;
      
      ActualFile[0] = '\0';
      CrossmatchPathnames(&FileDesc, NULL, "r", FileName, "./", NULL, ActualFile);
#else
      int FileDesc = 0;
      ActualFile[0] = '\0';
      CrossmatchPathnames(&FileDesc, NULL, "r", FileName, NULL, NULL, ActualFile);
#endif
      s_CurrentBuf = GetBuffer('.', AlwaysNew);
      
      if (s_JournalHand) {
        char Temp[StringMaxChr], TimeStamp[30];
        time_t DateTime;
        
        DateTime = time(NULL);
        strftime(TimeStamp, 100, "%d/%m/%y, %H:%M:%S", localtime(&DateTime));
        sprintf(Temp, "<<Primary session pid %d, at %s>>", getpid(), TimeStamp);
        UpdateJournal(Temp, NULL);
        sprintf(Temp, "<<jot version %s>>", VERSION_STRING);
        UpdateJournal(Temp, NULL);
        sprintf(Temp, "<<Primary file %s>>", FileName);
        UpdateJournal(Temp, NULL); }
      if ( ! FileDesc) {
        if ( ! FileName )
          FileName = "";
        Fail("Can't open file \"%s\"", FileName);
        GetRecord(s_CurrentBuf, 0);
        s_CurrentBuf->PathName = (char *)JotMalloc(strlen(FileName)+25);
        strcpy(s_CurrentBuf->PathName, "[ Nonexistent file: ");
        strcat(s_CurrentBuf->PathName, FileName);
        strcat(s_CurrentBuf->PathName, " ]"); }
      else {
#if defined(VC)
        ReadBuffer(NULL, NULL, FileDesc, FALSE, ActualFile, s_CurrentBuf, 0, 0, NULL, FALSE, 0);
#else
        ReadBuffer(NULL, FileDesc, FALSE, ActualFile, s_CurrentBuf, 0, 0, NULL, FALSE, 0);
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
  if ( ! s_TTYMode) { //Normal screen mode - use console screen-text routines.
    wOldColorAttrs = csbiInfo.wAttributes; 
    if ( ! SetConsoleTextAttribute(hStdout, Attr_Normal))
      Fail("SetConsoleTextAttribute: Console Error");
  if (s_CodePage) {  
    if ( ! SetConsoleOutputCP(s_CodePage))
      puts("SetConsoleOutputCP failed"); } }
#endif

  InitTermTable();
  NewCommandFile(fopen(KBD_NAME, "r"), "<The Keyboard>");
  s_EditorConsole = s_EditorInput;
  s_EditorOutput = stdout;
  s_CommandBuf = s_EditorConsole->CommandBuf;
  if (CommandBufSize <= 0)
    Disaster("Invalid history-buffer size");
  while (CommandBufSize--)
    GetRecord(s_CommandBuf, StringMaxChr);
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
  s_Stack[0].type = FrameType_Void;
  if (FileName == NULL) { //No file name given.
    if ( ! s_StreamIn)
      Disaster("No file to edit.\n"); } }

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
   
  if (s_JournalHand) {
    char Temp[StringMaxChr];
    sprintf(Temp, "<<Screen width %d, Screen height %d, Guard band size %d>>", s_TermWidth, s_TermHeight, s_GuardBandSize);
    UpdateJournal(Temp, NULL);
    if (s_InitCommands) {
      UpdateJournal("<<Init commands follow>>", NULL);
      UpdateJournal(s_InitCommands->FirstRec->text, NULL); }
    if (s_StartupFileName) {
      sprintf(Temp, "<<Startup script>>");
      UpdateJournal(Temp, NULL); } }
  
  if (setjmp(*s_RestartPoint)) //Error detected in editor setup.
    Disaster("Error detected in startup sequence - session abandoned");
  if (s_StartupFileName) {
    char * Args = strchr(s_StartupFileName, ' ');
    char PathName[StringMaxChr];
    if (Args) {
      strncpy(PathName, s_StartupFileName, Args-s_StartupFileName);
      PathName[Args-s_StartupFileName] = '\0';
      SwitchToComFile(PathName, NULL, Args); }
    else
      SwitchToComFile(s_StartupFileName, NULL, ""); }
 
  if (setjmp(*s_RestartPoint)) //Error detected in editor setup.
    Disaster("Error detected in -init commands sequence - session abandoned");
  if (s_JournalHand) {
    char Temp[StringMaxChr];
    sprintf(Temp, "<<Startup Sequence ends, buffer %c>>", s_CurrentBuf->BufferKey);
    UpdateJournal(Temp, NULL); }
  if (s_InitCommands) {
    s_BombOut = FALSE;
    s_CommandCounter = 0ll;
    InitSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
    JOT_Sequence(s_InitCommands, InitSequence, FALSE);
    s_TraceLevel++;
    if (Run_Sequence(InitSequence))
      RunError("-init sequence failed.");
    s_TraceLevel--; }
  
  FreeSequence(&InitSequence); 
  s_BacktraceFrame = NULL; 
  ThisBacktraceFrame.LastCommand = NULL;
  ThisBacktraceFrame.prev = s_BacktraceFrame;
  ThisBacktraceFrame.type = ConsoleCommand;
  ThisBacktraceFrame.EditorInput = s_EditorInput;
  s_BacktraceFrame = &ThisBacktraceFrame;
  InteractiveEditor();
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
    if (s_OnKey && InteractiveInput) { //on_key command sequence set and we're interactive.
      struct Seq *OnCommandSequence = NULL;
      s_CommandBuf->CurrentByte = 0;
      OnCommandSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
      JOT_Sequence(s_OnKey, OnCommandSequence, FALSE);
      if (s_CommandChr == ')')
        SynError(s_CommandBuf, "Surplus \')\' in OnCommand sequence");
      s_TraceLevel++;
      Status = Run_Sequence(OnCommandSequence);
      s_TraceLevel--;
      if (Status && ! s_BombOut)
        RunError("Command-sequence failed.");
      FreeSequence(&OnCommandSequence); }
    
    JotUpdateWindow();
    if ( ( ! s_InView) && (s_Verbose & Verbose_NonSilent) )
      DisplayDiag(s_CurrentBuf, TRUE, "");
    s_CurrentCommand = NULL;
    s_ExitBlocks = 0;
    s_Return = FALSE;
    //Detecting input from the console is not enough for the recovery-mode case - in recovery mode all  recovery activity 
    //is initiated by an -init command - normally -startup=recover or -init="%r=./recover_now -asConsole"
    //So, reset command counter in recovery mode if command source is -asConsole?
    if (InteractiveInput) {
      s_CommandCounter = s_CommandCounterInit;
      s_CommandCounterInit = 0ll; }
     
    Status = ReadCommand(
      s_CommandBuf,
      s_EditorInput->FileHandle,
      ((s_CurrentBuf->CurrentRec) == (s_CurrentBuf->FirstRec->prev) ? "****End****%d %c> " : "%d %c> "),
      s_CurrentBuf->LineNumber,
      s_CurrentBuf->BufferKey);
#if defined(LINUX)
    refresh();
#endif
    if (Status) {
      if ( s_asConsole && (s_Interrupt || Status == (char)EOF) ) //Status is only set to EOF at the end of an -asConsole script.
        break;
      else
        continue; }
    
    if (VerifyDigits(s_CommandBuf)) { //Repeat command.
      long long Repeat = GetDec(s_CommandBuf);
      int Count = 0;
      
      if (MainSequence == NULL) {
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
    MainSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
    JOT_Sequence(s_CommandBuf, MainSequence, FALSE);
    if (s_CommandChr == ')')
      SynError(s_CommandBuf, "Surplus \')\'");
    s_BacktraceDone = FALSE;
    s_TraceLevel++;
    LocalFail = Run_Sequence(MainSequence);
    s_TraceLevel--;
    if (LocalFail && ! s_BombOut)
      RunError("Command-sequence failed."); }
  
  FreeSequence(&MainSequence);
  return LocalFail; }

//---------------------------------------------JOT_Sequence
void JOT_Sequence(struct Buf *CommandBuf, struct Seq *ThisSequence, int StopOnTag)
  { //The main entry point for the JOT compiler.
    //Compiles a sequence of JOT commands, terminated by one of the following:
    // ',' - an JOT-style else clause follows - calls itself recursively,
    // ')' - end of JOT-style block - pass it back to calling JOT_Block
    // '\0' - the end of a command line. */
    // StopOnTag is used to identify the end point when compiling a selected routine from the repositort using %h=call ...;
  struct Com *ThisCommand = NULL;
  struct Com *PrevCommand = NULL;
  
  ThisSequence->FirstCommand = NULL;
  ThisSequence->ElseSequence = NULL;
  ThisSequence->NextSequence = NULL;
  CommandBuf->UnchangedStatus |= SameSinceCompiled;
  s_PrevCommandRec = NULL;
   
  for ( ; ; ) { // Command-sequence loop.
    int CommandByteNo;
    int CommandLineNo;
    
    s_CommandChr = ChrUpper(NextNonBlank(CommandBuf));
    if (StopOnTag && CommandBuf->CurrentRec->TagChain)
      return;
    CommandByteNo = (CommandBuf->CurrentByte)-1;
    CommandLineNo = CommandBuf->LineNumber;
    if (s_CommandChr == ',') { // ELSE Clause follows. 
      ThisSequence->ElseSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
      JOT_Sequence(CommandBuf, ThisSequence->ElseSequence, StopOnTag);
      return; }
    else if (s_CommandChr == '\0') { // End of command line reached.
      if ( ! AdvanceRecord(CommandBuf, 1))
        continue;
      return; }
  
    ThisCommand = (struct Com *)JotMalloc(sizeof(struct Com));
    if (ThisCommand == NULL) {
      SynError(NULL, "Failed to allocate space for command.");
      return; }
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
       
    switch (s_CommandChr) {;
    case '\'':  //' - Deferred command specification.
      AddIntArg(ThisCommand, ChrUpper(ImmediateCharacter(CommandBuf)));
      break;
  
    case '(': { //( - Open a new Block. 
      struct Block *NewBlock = (struct Block *)JotMalloc(sizeof(struct Block));
      NewBlock->BlockSequence = NULL;
      AddBlock(ThisCommand, NewBlock);
      if ( ! JOT_Block(NewBlock, CommandBuf, StopOnTag))
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
  
    case 'W':  //W - update Window command.
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
      case '^':  //O^ Test state
        AddIntArg(ThisCommand, ChrUpper(ImmediateCharacter(CommandBuf)));
        break;
      case 'G':  //OG prompts and reads one character from console stream.
      case 'W':  //OW Scrolls up by lines set in top of stack.
      case '@':  //O@ reset stack
      case '?':  //O? Dump contents of stack
      case 'S':  //OS Swap 1st and 2nd items of stack.
      case '#':  //O# Duplicate top of stack.
      case '=':  //O= Fail if top two items on stack not equal.
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
        SynError(CommandBuf, "Unknown stack operator");
      };//Stack-operation switch block ends.
    break;
  
    case 'Z':  //Z - zoom into another buffer.
      Arg2 = ChrUpper(ImmediateCharacter(CommandBuf));
      if ( ! CheckBufferKey(Arg2))
        SynError(CommandBuf, "Invalid buffer tag.");
      AddIntArg(ThisCommand, Arg2);
      break; }

    case '%': { //% - Percent-command-argument block.
      char *Text;
      char BufferKey;
      int BufferKeyChr;
      int TextAsIs;     //Set TRUE when the command is terminated with " -hereEndsThePercentCommand;"
      
      s_CommandChr = ChrUpper(NextNonBlank(CommandBuf));
      AddIntArg(ThisCommand, s_CommandChr);
      BufferKeyChr = CommandBuf->CurrentByte;
      BufferKey = ChrUpper(CommandBuf->CurrentRec->text[BufferKeyChr]);
      if (BufferKey == ';') { //This is a very short command terminated with a semicolon - like "%w; m" - ensures that the following command is parsed correctly.
        BufferKey = '0';
        CommandBuf->CurrentByte = BufferKeyChr-1; }
      if (BufferKey == '=' && CommandBuf->CurrentRec->text[BufferKeyChr+1] != '=') //No buffer key given.
        BufferKey = '\0';
      else if (CommandBuf->CurrentRec->text[BufferKeyChr] != '\0')
        CommandBuf->CurrentByte++;
      
      if ( (s_CommandChr == 'G') &&  (CommandBuf != s_CommandBuf) ) { //%G command from a macro - copy the text block now.
        struct Buf *DestBuf;
         
        if (BufferKey == CommandBuf->BufferKey) {
          Fail("Invalid destination buffer.");
          continue; }
        DestBuf = GetBuffer(BufferKey, AlwaysNew);
        CommandBuf->CurrentByte--;
        for ( ; ; ) { 
          if (AdvanceRecord(CommandBuf, 1))
            break;
          if ( (CommandBuf->CurrentRec->text[0] == ':' && CommandBuf->CurrentRec->text[1] == '\0') ) {
            CommandBuf->CurrentByte = 1;
            break; }
          GetRecord(DestBuf, strlen(CommandBuf->CurrentRec->text));
          strcpy(DestBuf->CurrentRec->text, CommandBuf->CurrentRec->text); } }
      else {
        Text = CommandBuf->CurrentRec->text + CommandBuf->CurrentByte + 1;
        if (s_CommandChr == '%') //Skip all the comment text.
          CommandBuf->CurrentByte = strlen(CommandBuf->CurrentRec->text);
        if (s_CommandChr == 'F') { //%F has sloppy, confusing syntax for backwards searches.
          int Direction = 1;
          if (BufferKey == '-') { //Always take a '-' here to indicate a backwards search.
            Direction = -1;
            if (CommandBuf->CurrentRec->text[BufferKeyChr+2] == '=') { //Buffer key follows the minus sign.
              BufferKey = ChrUpper(CommandBuf->CurrentRec->text[BufferKeyChr+1]);
              CommandBuf->CurrentByte++; }
            else
              BufferKey = '0'; }
          AddIntArg(ThisCommand, BufferKey);
          TextAsIs = AddPercentArg(ThisCommand, CommandBuf);
          AddIntArg(ThisCommand, TextAsIs);
          AddIntArg(ThisCommand, Direction); }
        else if (s_CommandChr == 'A') { //%A<status>[=<message>] - pick up status and message
          CommandBuf->CurrentByte--;
          AddIntArg(ThisCommand, (BufferKey) ? GetDec(CommandBuf) : 1); //Specified exit status is wrapped up as BufferKey.
          TextAsIs = AddPercentArg(ThisCommand, CommandBuf);
          AddIntArg(ThisCommand, TextAsIs); }
        else {
          AddIntArg(ThisCommand, BufferKey);
          TextAsIs = AddPercentArg(ThisCommand, CommandBuf);
          AddIntArg(ThisCommand, TextAsIs); } }
  
      switch (s_CommandChr) { // Percent-command switch block.
        case 'I':  //%I - Secondary input file.
        case 'A':  //%A - Exit without writing file.
        case 'B':  //%B - Set buffer attributes.
        case 'C':  //%C - Exit writing new file.
        case 'D':  //%D - Define a buffer from console or last console command.
        case 'E':  //%E - Execute following CLI command line.
        case 'F':  //%F - RegEx
        case 'G':  //%G - Get - but from current command file or console (macro case dealt with above).
        case 'L':  //%L - set line Length.
        case 'M':  //%M - Message.
        case 'O':  //%O - Output current buffer as specified file.
        case 'P':  //%P - send message down async Pipe - the pipe is previously set up by %E command with & modifier.
        case 'R':  //%R - Run a command file.
        case 'S':  //%S - System settings 
        case 'U':  //%U - Undo last substitution.
        case 'W':  //%W - Set up a screen window.
        case 'X':  //%X - Exit via a run-time error message.
        case '~':  //%~ - Insert or display control character.
        case '%':  //%% - Comment line.
          break;
        
        case 'H': { //%H - Hashtable maintenance
          int Operation, Qual = 0, IsQual = FALSE;
          
          if (strstr(Text, "create") == Text) { 
            IsQual = TRUE;
            Operation = H_create;
              if (strstr(Text, "-destroy"))
                Qual = H_destroyQual;
              else if(strstr(Text, "-protect"))
                Qual = H_protectQual;
              else if(strstr(Text, "-adjust"))
                Qual = H_adjustQual;
              else if(strstr(Text, "-delete"))
                Qual = H_deleteQual; }
          else if ( (strstr(Text, "add") == Text) )
            Operation = H_add;
          else if ( (strstr(Text, "new") == Text) )
            Operation = H_new;
          else if (strstr(Text, "setsect") == Text)
            Operation = H_setsect;
          else if (strstr(Text, "setfsect") == Text)
            Operation = H_setfsect;
          else if (strstr(Text, "data") == Text)
            Operation = H_data;
          else if (strstr(Text, "jump") == Text)
            Operation = H_jump;
          else if (strstr(Text, "call") == Text)
            Operation = H_call;
          else if (strstr(Text, "fix") == Text)
            Operation = H_fix;
          else if (strstr(Text, "delete") == Text)
            Operation = H_delete;
          else if (strstr(Text, "testkey") == Text)
            Operation = H_testkey;
          else if (strstr(Text, "destroy") == Text) { 
            Operation = H_destroy;
            IsQual = TRUE;
            if (strstr(Text, " -all"))
              Qual = H_allQual; }
          AddIntArg(ThisCommand, Operation);
          if (IsQual)
            AddIntArg(ThisCommand, Qual);
          break; }
          
        case 'Q': { //%Q - system Query.
          int QueryType, SubQueryType = 0, IsSubQuery = FALSE;
          if (strstr(Text, "version") == Text) //version - print version or send it to a buffer.
            QueryType = Q_version;
          else if (strstr(Text, "windows") == Text) //windows - fails if not operating under windows.
            QueryType = Q_windows;
          else if (strstr(Text, "linux") == Text) //linux fails if not operating under linux.
            QueryType = Q_linux;
          else if (strstr(Text, "case") == Text) //case - fails if not case sensitive.
            QueryType = Q_case;
          else if (strstr(Text, "system") == Text) //System settings etc.
            QueryType = Q_system;
          else if (strstr(Text, "hashtables") == Text) //hashtables - find buffers with hashtables, list internal and external references and target buffers
            QueryType = Q_hashtables;
          else if (strstr(Text, "wd") == Text) //wd - return current directory only.
            QueryType = Q_wd;
          else if (strstr(Text, "heap") == Text) //heap - report heap stats to stdout.
            QueryType = Q_heap;
          else if (strstr(Text, "backtrace") == Text) //Writes diagnostic backtrace to nominated buffer.
            QueryType = Q_backtrace;
          else if (strstr(Text, "history") == Text) //history - dumps history to specified buffer.
            QueryType = Q_history;
          else if (strstr(Text, "tabstops") == Text) //tabstops - pushes TRUE if tabstops or tabcells are set otherwise FALSE.
            QueryType = Q_tabstops;
          else if (strstr(Text, "tabcells") == Text) //tabcells - pushes TRUE if tabcells are set otherwise FALSE.
            QueryType = Q_tabcells;
          else if (strstr(Text, "keys") == Text) //keys - sends a list of keys and truncated endpoint strings to nominated buffer.
            QueryType = Q_keys;
          else if (strstr(Text, "key") == Text) //keys - as above but only reports on specified key.
            QueryType = Q_key;
          else if (strstr(Text, "tags") == Text) { //tags - sends a list of record tags in current buffer to nominated buffer.
            QueryType = Q_tags;
            if (strstr(Text, "-here")) { //Report only tags active at current character position.
              IsSubQuery = TRUE;
              SubQueryType = Q_here; } }
          else if (strstr(Text, "buffer") == Text) //buffer - returns info on buffer.
            QueryType = Q_buffer;
          else if (strstr(Text, "window") == Text) //window - returns window size and allocation.
            QueryType = Q_window;
          else if (strstr(Text, "time") == Text) //time - return seconds since start of unix epoch.
            QueryType = Q_time;
          else if (strstr(Text, "date") == Text) //date - return todays date only.
            QueryType = Q_date;
          else if (strstr(Text, "inview") == Text) //Is current character visible with current LeftOffset setting.
            QueryType = Q_inview;
          else if (strstr(Text, "commandmode") == Text) //Query in command mode
            QueryType = Q_commandmode;
          else if (strstr(Text, "samesinceio") == Text) //Query SameSinceIO in this buffer.
            QueryType = Q_samesinceio;
          else if (strstr(Text, "samesinceindexed") == Text) //Query SameSinceIndexed in this buffer.
            QueryType = Q_samesinceindexed;
          else if (strstr(Text, "samesincecompiled") == Text) //Query SameSinceCompiled in this buffer.
            QueryType = Q_samesincecompiled;
          else if (strstr(Text, "sameflag1") == Text) //Query or set SameFlag1 in this buffer.
            QueryType = Q_sameflag1;
          else if (strstr(Text, "pid") == Text) //pid - Process ID of current process.
            QueryType = Q_pid;
          else if (strstr(Text, "env") == Text) //env - return translation of env variable.
            QueryType = Q_env;
          else if (strstr(Text, "dir") == Text) //dir <path> - return directory contents.
            QueryType = Q_dir;
          else if (strstr(Text, "file") == Text) //file <pathName> - a selection of info on the selected file.
            QueryType = Q_file;
          else if (strstr(Text, "stack") == Text) //stack - dumps stack to selected buffer.
            QueryType = Q_stack;
          else if (strstr(Text, "verify") == Text) //Verifies the basic integrity of the buffer and the sequence.
            QueryType = Q_verify;
          else if (strstr(Text, "cputime") == Text) //Pushes time counted since system startup time.
            QueryType = Q_cputime;
          AddIntArg(ThisCommand, QueryType);
          if (IsSubQuery)
            AddIntArg(ThisCommand, SubQueryType);
          break; }
            
        default:
          SynError(CommandBuf, "Unknown %% command"); } }
      break;

    default:  //Invalid command character.
      CommandBuf->CurrentByte--;
      if (s_JournalHand) {
        fputs("<Invalid command>\n", s_JournalHand);
        fflush(s_JournalHand); }
      SynError(CommandBuf, "Invalid command character.");
      return; } // Command case block ends.
      
    PrevCommand = ThisCommand;
    ThisCommand->ComChrs = (CommandBuf->CurrentByte)-(ThisCommand->CommandByteNo)-1; } }

//---------------------------------------------JOT_Block
struct Block *JOT_Block(struct Block *ThisBlock, struct Buf *CommandBuf, int StopOnTag)
  { //Compiles a Block of JOT commands, terminated by a ')'. 
  ThisBlock->BlockSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
  JOT_Sequence(CommandBuf, ThisBlock->BlockSequence, StopOnTag);
  if (s_CommandChr != ')') {
    SynError(CommandBuf, "Missing \')\'");
    return NULL; }
  ThisBlock->Repeats = GetArg(CommandBuf, 1);
  return ThisBlock;  }

//---------------------------------------------Run_Sequence
int Run_Sequence(struct Seq *Sequence)
  { //Executes a sequence of compiled commands, normally returns 0 or nonzero if something fails.
  int LocalFail = 0;
  struct Com *NextCommand = NULL;
  struct Seq *ElseCommands = Sequence->ElseSequence;
  struct AnyArg *ThisArg;
  
  s_CurrentCommand = Sequence->FirstCommand;
  s_ExitBlocks = 0;
  s_Return = FALSE;
  
  for (;;) { // Command-key loop. 
    if (s_CurrentCommand) {
      s_CommandChr = s_CurrentCommand->CommandKey;
      ThisArg = s_CurrentCommand->ArgList;
      NextCommand = s_CurrentCommand->NextCommand; }
    else
      break;
    if ( ! ++s_CommandCounter) { //Reached terminal count.
      if (s_RecoveryMode) //In recovery mode, execution is abruptly terminated.
        LocalFail = RunError("Command sequence aborted by simulated Ctrl+C");
      else {
        Message(s_CommandBuf, "Reached specified command count");
        s_TraceMode = s_DefaultTraceMode; } }
    
    if ( (s_TraceMode & Trace_CommandLines) && (s_CurrentCommand->CommandRec != s_PrevCommandRec) ) {
      s_PrevCommandRec = s_CurrentCommand->CommandRec;
      JotTrace("New command line.");
      if (s_BombOut)
        break; }
    if ( (s_TraceMode & Trace_SubSequence) && (Sequence->FirstCommand == s_CurrentCommand) ) {
      JotTrace("New subsequence.");
      if (s_BombOut)
        break; }
    if (s_TraceMode & Trace_AllCommands) {
      JotTrace("New command.");
      if (s_BombOut)
        break; }
    switch (s_CommandChr) {

    case '(': { //( - Block.
      struct Com *CurrentCom = s_CurrentCommand;
      LocalFail = Run_Block(FetchBlock((struct PtrArg **)&ThisArg));
      s_CurrentCommand = CurrentCom;
      if (s_TraceMode & Trace_FailBlocks)
        JotTrace("New command line.");
      if (s_ExitBlocks)
        return LocalFail;
      break; }

    case ')':  //) - End of current Block.
      break;

    case '\'': { //' - Macro call - deferred sequence block.
      char Key = FetchIntArg((struct IntArg **)&ThisArg);
      struct Buf *CommandBuf = GetBuffer(Key, NeverNew);
      struct Seq *DeferredSeq;
      struct Com *CurrentCommand = s_CurrentCommand;
      struct BacktraceFrame ThisBacktraceFrame;
  
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
      if (s_CommandChr == ')') {
        SynError(CommandBuf, "Surplus \')\'");
        free(DeferredSeq);
        break; }
      s_BacktraceFrame->LastCommand = s_CurrentCommand;
      ThisBacktraceFrame.LastCommand = NULL;
      ThisBacktraceFrame.prev = s_BacktraceFrame;
      ThisBacktraceFrame.type = MacroFrame;
      s_BacktraceFrame = &ThisBacktraceFrame;
      CommandBuf->UnchangedStatus &= ~MacroRun;
      
      s_TraceLevel++;
      LocalFail = Run_Sequence(DeferredSeq);
      s_TraceLevel--;
      s_CurrentCommand = CurrentCommand;
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
      s_ExitBlocks = FetchIntArg((struct IntArg **)&ThisArg);
      if (s_ExitBlocks == INT_MAX)
        s_Return = TRUE;
      break;

    case 'N':  //N - Note current Record for later abstract.
      s_AbstractWholeRecordFlag = ! FetchIntArg((struct IntArg **)&ThisArg);
      s_NoteLine = s_CurrentBuf->LineNumber;
      s_NoteBuffer = s_CurrentBuf;
      s_NoteByte = s_AbstractWholeRecordFlag ? 0 : s_CurrentBuf->CurrentByte;
      break;

    case 'A': { //A - Abstract using note record pointer.
      char BufferKey = FetchIntArg((struct IntArg **)&ThisArg);
      char ModeQualifier = FetchIntArg((struct IntArg **)&ThisArg);  // NULL, +, - or .
      char FillQualifier = FetchIntArg((struct IntArg **)&ThisArg);  // &.
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
        GetRecord(DestBuf, 1);
      DestBuf->UnchangedStatus = 0;
       
      switch (ModeQualifier) {;
        case '+': //Append
          AdvanceRecord(DestBuf, INT_MAX);
          DestBuf->CurrentByte = strlen(DestBuf->CurrentRec->text);
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
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      char *StringArgument = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_QualifyString);
      char *RecordText = s_CurrentBuf->CurrentRec->text;
      int RecordTextLength = strlen(RecordText);
      int CurrentByte = s_CurrentBuf->CurrentByte;
      char Chr;
      int Length, w_Length, Index;
      
      if ( ! StringArgument || mblen(StringArgument, MB_CUR_MAX) < 0)
        break;
      
      s_CurrentBuf->SubstringLength = 0;
      LocalFail = 1;
      //Extract match character, set to 0 if already at end stop. 
      if (Direction < 0)
        Chr = (CurrentByte == 0) ? '\0' : RecordText[CurrentByte-1];
      else
        Chr = (RecordTextLength < CurrentByte) ? '\0' : RecordText[CurrentByte];
      if (Chr == 0) 
        break;
      else if ( ((signed char)Chr < 0) && ! (s_CurrentBuf->NoUnicode) ) { //Assume we're matching a unicode character.
        int ChrFirstByte = CurrentByte, ChrsConverted;
#ifdef VC
        wchar_t w_Chr, w_StringArgument[StringMaxChr];
        Length = JotStrlenChrs(StringArgument, strlen(StringArgument));
#else
        Length = JotStrlenChrs(StringArgument, strlen(StringArgument));
        wchar_t w_Chr, w_StringArgument[Length+1];
#endif
        if (mbstowcs(w_StringArgument, StringArgument, Length+1) <= 0)
          break;
        w_Length = wcslen(w_StringArgument);
        if (Direction < 0) { //Ugh - this is messy but necessary.
          ShiftRight(s_CurrentBuf, -1);
          ChrFirstByte = s_CurrentBuf->CurrentByte;
          s_CurrentBuf->CurrentByte = CurrentByte; }
        ChrsConverted = mbstowcs(&w_Chr, s_CurrentBuf->CurrentRec->text + ChrFirstByte, 1);
        if (ChrsConverted <= 0)
          break;
          
        for (Index = 0; Index < w_Length; Index +=1) { // Search loop. 
          if (w_StringArgument[Index] == '-' && (0 < Index && Index < w_Length-1) ) { // A range of characters.
            if ((w_StringArgument[Index-1] <= w_Chr) && (w_Chr <= w_StringArgument[Index+1])) { // Chr in range. 
              LocalFail = 0;
              break; } }
          else if (w_StringArgument[Index] == w_Chr) { // Chr match. 
            if (w_Chr == '-' && (Index != 0 && Index != w_Length-1) ) //It's a character-range hyphen.
              continue;
            LocalFail = 0;
            break; } } }
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
            break; } } }
      break; }

    case 'V': { //V - Verify text string.
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      char *StringArgument = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_SystemMode ? NULL : s_SearchString);
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
      char Arg1 = FetchIntArg((struct IntArg **)&ThisArg);
      long long Arg2 = FetchIntArg((struct IntArg **)&ThisArg);
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
        ShiftRight(s_CurrentBuf, s_Y_ColumnNo); }
      LocalFail &= ( (Arg2 != INT_MIN) && (Arg2 != INT_MAX) );
      break; }

    case 'B': { //B - Break line.
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      long long Count = FetchIntArg((struct IntArg **)&ThisArg);
      int Index;
    
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
      s_CurrentBuf->UnchangedStatus = 0;
       
      for (Index = 1; Index <= Count; Index +=1) { // Record-loop. 
        if ( (LocalFail = BreakRecord(s_CurrentBuf)) )
          break;
        if (Direction < 0) { // Backwards.
          s_CurrentBuf->CurrentRec = s_CurrentBuf->CurrentRec->prev;
          s_CurrentBuf->CurrentByte = strlen(s_CurrentBuf->CurrentRec->text);
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
          sprintf(Temp, "<<Debugger on>> %d", s_JournalDataLines);
          UpdateJournal(Temp, NULL); }
        else
          UpdateJournal("<<Debugger off>>", NULL); }
      break;

    case 'J': { //J - Join line(s).
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      long long Count = FetchIntArg((struct IntArg **)&ThisArg);
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
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      long long SearchSpan = FetchIntArg((struct IntArg **)&ThisArg);
      char *StringArgument = FetchStringArg((struct PtrArg **)&ThisArg, s_SystemMode ? NULL : s_SearchString);
      long long Count = FetchIntArg((struct IntArg **)&ThisArg);
      int Traverse = FALSE;
        
      if (Count < 0) { 
        Traverse = TRUE;
        Count = -Count; }
      for ( ; (0 < Count)  && ( ! LocalFail); Count--) {
        for ( ; ; ) { // Record loop, if not found move record pointer.
          int InitialPointer = s_CurrentBuf->CurrentByte;
          if (SearchRecord(s_CurrentBuf, StringArgument, Direction)) // Substring found.
            break;
          else if ( ! --SearchSpan) { // Hit the endstop.
            s_CurrentBuf->CurrentByte = InitialPointer;
            LocalFail = 1;
            break; }
          if ( (LocalFail |= AdvanceRecord(s_CurrentBuf, Direction)) )
            break;
          if (Direction < 0)
            s_CurrentBuf->CurrentByte = strlen(s_CurrentBuf->CurrentRec->text); } }
          
      if (LocalFail) {
        if (0 < SearchSpan)
          s_CurrentBuf->CurrentByte = Direction<0 ? 0 : strlen(s_CurrentBuf->CurrentRec->text);
        break; }
      if (Traverse) {
        s_CurrentBuf->CurrentByte = (s_CurrentBuf->CurrentByte) + (s_CurrentBuf->SubstringLength);
        s_CurrentBuf->SubstringLength = - s_CurrentBuf->SubstringLength; }
      break; }

    case 'G':  //G - Get - Input text from console.
      { //Get block. 
      struct Rec *PrevRecordSave = s_CurrentBuf->CurrentRec->prev;
      long long Count = FetchIntArg((struct IntArg **)&ThisArg);
      int Infinate = (Count == INT_MAX);
      
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
            UpdateJournal("<<G-command terminated>>", NULL);
          break; }
        Rec = s_CurrentBuf->CurrentRec;
        if ((strlen(Rec->text) == 1) && ((Rec->text)[0] == ':')) { // Last record. 
          FreeRecord(s_CurrentBuf, AdjustForwards);
          LocalFail = 1;;
          s_CurrentBuf->LineNumber--;
          break; }
        s_CurrentBuf->CurrentRec = Rec->next;
        s_EditorInput->LineNo++;
        JotUpdateWindow();
        if (LocalFail)
          break; }
  
      if ((s_CurrentBuf->CurrentRec) == (s_CurrentBuf->FirstRec))
        s_CurrentBuf->FirstRec = PrevRecordSave->next;
      LocalFail |= ( ! Infinate && (0 < Count) );
      break; }

    case 'H': { //H - Here command.
      char BufferKey = FetchIntArg((struct IntArg **)&ThisArg);
      long long Count = FetchIntArg((struct IntArg **)&ThisArg);
      int OriginalCurrentByte = s_CurrentBuf->CurrentByte;
      int OriginalLineNumber = s_CurrentBuf->LineNumber;
      int Index, Reverse = FALSE;
      int OrigSourceLineNumber, OrigSourceCurrentByte, OrigSourceSubstrLen;
      struct Rec *OrigSourceCurrentRec;
      struct Buf *SourceBuf = GetBuffer(BufferKey, NeverNew);
      
      if (  (! SourceBuf) || ! CheckBufferKey(BufferKey) || (SourceBuf == s_CurrentBuf)) {
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
      long long Arg1 = FetchIntArg((struct IntArg **)&ThisArg);
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
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      char *StringArgument = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_SystemMode ? NULL : s_InsertString);
      long long Arg2 = FetchIntArg((struct IntArg **)&ThisArg);
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

    case 'E': { //E - Erase bytes or characters.
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      long long Count = FetchIntArg((struct IntArg **)&ThisArg);
      
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
      s_CurrentBuf->UnchangedStatus = 0;
       
      if (Direction < 0) { // -ve. - Erase left Count charcaters. 
        int Current = s_CurrentBuf->CurrentByte;
        
        if (Count == INT_MAX)
          Count = Current;
        else { //Delete a finite no. of characters left of current character.
          int CurrentByte = s_CurrentBuf->CurrentByte;
          if (ShiftRight(s_CurrentBuf, -Count))
            LocalFail = 1;
          else
            Count = CurrentByte-s_CurrentBuf->CurrentByte;
          s_CurrentBuf->CurrentByte = CurrentByte; }
        if (Current < Count) {
          LocalFail = 1;
          break; }
        s_CurrentBuf->CurrentByte -= Count; }
      else { // +ve. - Erase right. 
        int Length = strlen(s_CurrentBuf->CurrentRec->text);
        int Current = s_CurrentBuf->CurrentByte;
  
        if (Count == INT_MAX)
          Count = Length-Current;
        else { //Delete a finite no. of characters right of current character.
          int CurrentByte = s_CurrentBuf->CurrentByte;
          if (ShiftRight(s_CurrentBuf, Count))
            LocalFail = 1;
          else
            Count = s_CurrentBuf->CurrentByte-CurrentByte;
          s_CurrentBuf->CurrentByte = CurrentByte; }
        if (Length-Current < Count) {
          LocalFail = 1;
          break; } }
        
      s_CurrentBuf->SubstringLength = Count;
      LocalFail = SubstituteString(s_CurrentBuf, NULL, -1);
      break; }

    case 'C':  //C - Change case of characters.
      { // Change case of alpha characters. 
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      long long Count = FetchIntArg((struct IntArg **)&ThisArg);
      char *RecordText = s_CurrentBuf->CurrentRec->text;
      int Current = s_CurrentBuf->CurrentByte;
      int Chr, Last;
      
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
       
      if (Direction < 0) { // -ve change hop back first.
        Chr = (Count == INT_MAX) ? 0 : Current-Count;
        if (Chr < 0) {
          LocalFail = 1;
          break; }
        Last = Current;
        s_CurrentBuf->CurrentByte = Chr; }
      else { // +ve change.
        int Length = strlen(RecordText);
          
        Last = (Count == INT_MAX) ? Length : Current+Count;
        if (Length < Last) {
          LocalFail = 1;
          break; }
        Chr = Current;
        s_CurrentBuf->CurrentByte = Last; }
      
      for ( ; Chr != Last; Chr++) { // Case-change loop.
        int C = RecordText[Chr];
        
        RecordText[Chr] = (('a' <=  C && C <= 'z') ? C-'a'+'A' : (('A' <=  C && C <= 'Z') ? C-'A'+'a' : C)); }
      
      s_CurrentBuf->SubstringLength = 0;
      s_CurrentBuf->CurrentRec->DisplayFlag = Redraw_Line;
      s_CurrentBuf->UnchangedStatus = 0;
      break; }

    case 'R': { //R - move current character pointer Right characters.
      int Displacement = FetchIntArg((struct IntArg **)&ThisArg);
      
      s_CurrentBuf->SubstringLength = 0; 
      if (Displacement == INT_MAX)
        s_CurrentBuf->CurrentByte = strlen(s_CurrentBuf->CurrentRec->text);
      else if (Displacement == INT_MIN)
        s_CurrentBuf->CurrentByte = 0;
      else
        LocalFail = ShiftRight(s_CurrentBuf, Displacement);
      break; }

    case 'K': { //K - delete (Kill) some lines.
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      long long Count = FetchIntArg((struct IntArg **)&ThisArg);
      int Index, LastRec = FALSE, FirstRec = FALSE;
    
      if (s_CurrentBuf->EditLock & ReadOnly) {
        LocalFail = RunError("Attempt to modify a readonly buffer");
        break; }
      s_CurrentBuf->SubstringLength = 0;
      s_CurrentBuf->CurrentByte = 0;
       
      for (Index = 1; ! LocalFail && (Index <= Count); Index+=1) { // Record Loop. 
        if (s_CurrentBuf->CurrentRec->next == s_CurrentBuf->FirstRec)
          LastRec = TRUE;
        if (s_CurrentBuf->CurrentRec == s_CurrentBuf->FirstRec)
          FirstRec = TRUE;
          
        if (FirstRec && LastRec) {
          struct AnyTag * ThisTag = s_CurrentBuf->CurrentRec->TagChain;
          s_CurrentBuf->CurrentRec->text[0] = '\0';
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
          
      if (Count == INT_MAX)
        LocalFail = 0;
      break; }

    case 'P': { //P - Print line(s).
      long long Direction = FetchIntArg((struct IntArg **)&ThisArg);
      long long Count = FetchIntArg((struct IntArg **)&ThisArg);
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

    case 'W':  //W - update Window command.
      JotUpdateWindow();
      if ( ( ! s_InView) && (s_Verbose & Verbose_NonSilent) )
        DisplayDiag(s_CurrentBuf, TRUE, "");
      s_NewConsoleLines = 0;
      break;

    case 'O': { //O - Stack Operation.
      char Chr = FetchIntArg((struct IntArg **)&ThisArg);
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
          Bytes[i++] = JotGetCh(s_asConsole ? s_asConsole->FileHandle : s_EditorConsole->FileHandle);
          if (s_BombOut) {
            LocalFail = 1;
            break; } }
          while (mblen(Bytes, i) < 0);
          
        mbtowc(&WChr, Bytes, i);
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
        int Type = FetchIntArg((struct IntArg **)&ThisArg);
        if (Type == 0)
          LocalFail = PushInt(FetchIntArg((struct IntArg **)&ThisArg));
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
        char FormatChr = ChrUpper(FetchIntArg((struct IntArg **)&ThisArg));
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
              Size = mbstowcs(&w_Chr, s_CurrentBuf->CurrentRec->text + s_CurrentBuf->CurrentByte, 1);
              if (Size <= 0)
                LocalFail = 1;
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
        //After checking, attach the top stack frame to the DataObj located by the QueryPath() call.
        struct DataObj *ParentObj;
        struct Buf *ParentBuf;
        char *LastPathElem, *Path = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_TempBuf->CurrentRec->text);
        struct intFrame * TopItem = &s_Stack[s_StackPtr-1];
        struct Buf * BufFromStack = NULL;
        int BufAtTop;
        
        if (s_StackPtr < 1) {
          LocalFail = Fail("Stack underflow");
          break; }
        BufAtTop = ((struct intFrame *)TopItem)->type == FrameType_Buf;
        if ( ! (ParentBuf = QueryPath(Path, s_CurrentBuf, &LastPathElem)) ) {
          LocalFail = 1;
          break; }
        if ( ! (ParentObj = (struct DataObj *)QueryData(LastPathElem, ParentBuf)) ) {
          LocalFail = 1;
          break; }
        if (BufAtTop) {
          BufFromStack = ((struct bufFrame *)TopItem)->BufPtr;
          if (CheckCircularHier(BufFromStack)) {
            LocalFail = Fail("That assignment would result in a circular hierarchy;");
            break; }
          if (BufFromStack->ExistsInData) {
            LocalFail = Fail("That buffer is alreay used to define a dataObj buffer.");
            break; }
          if (BufFromStack->ParentObj) {
            LocalFail = Fail("The buffer from stack is already a member of a data-object tree");
            break; } }
        if ( ! ParentObj->Frame)
          ParentObj->Frame = (struct intFrame *)JotMalloc(sizeof(struct intFrame));
        else { //Some preexisting frame.
          struct bufFrame *OldItem = (struct bufFrame *)ParentObj->Frame;
          if (OldItem->type != TopItem->type) {
            LocalFail = Fail("Type mismatch, the data object and the top item on the stack do not match.");
            break; }
          if (OldItem->type == FrameType_Buf) { //There's a preexisting buffer hanging off the data object - free it now.
            struct Buf *OrigBuffer = OldItem->BufPtr;
            if (OrigBuffer != BufFromStack) {
              OldItem->BufPtr->ExistsInData = FALSE;
              if (s_CurrentBuf != OrigBuffer) {
                if (FreeBuffer(OrigBuffer)) {
                  LocalFail = Fail("Attempt to free a read-only or locked buffer.");
                  break; } } } } }
        if (BufAtTop) {
          BufFromStack->ParentObj = ParentObj;
          BufFromStack->ExistsInData = TRUE; }
        memcpy(ParentObj->Frame, TopItem, sizeof(struct intFrame));
        LocalFail = KillTop();
        break; }

      case 'Q': { //OQ Query hashtable, copy previously-defined stack frame to top of stack.
        struct DataObj *ThisDataObj;
        struct Buf * ThisBuf;
        char *LastPathElem, *Path = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_TempBuf->CurrentRec->text);
        
        if (s_StackSize <= s_StackPtr) {
          LocalFail = Fail("Stack overflow");
          break; }
        if ( ! (ThisBuf = QueryPath(Path, s_CurrentBuf, &LastPathElem)) ) {
          LocalFail = 1;
          break; }
        if ( ! (ThisDataObj = (struct DataObj *)QueryData(LastPathElem, ThisBuf)) ) {
          LocalFail = 1;
          break; }
        if ( ! ThisDataObj->Frame) {
          LocalFail = Fail("Hash-table entry has been defined but data-value not yet assigned.");
          break; }
        memcpy(&s_Stack[s_StackPtr++], ThisDataObj->Frame, sizeof(struct intFrame));
        break; }

      case 'O': { //OO Output to current text, using given string as format spec.
        char *StringArgument = (char *)FetchStringArg((struct PtrArg **)&ThisArg, s_TempBuf->CurrentRec->text);
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
          Chrs = sprintf(TempString, StringArgument, Buf->CurrentRec->text); }
        else if (s_Stack[s_StackPtr-1].type == FrameType_Float) {
          Chrs = sprintf(TempString, StringArgument, ((struct floatFrame *)(&s_Stack[s_StackPtr-1]))->fValue);
          KillTop(); }
        else { //An integer.
          Chrs = sprintf(TempString, StringArgument, s_Stack[s_StackPtr-1].Value);
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
        if (s_StackPtr <= 1) {
          LocalFail = Fail("Less than two items on stack");
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
        int ByteCount = s_CurrentBuf->SubstringLength ? s_CurrentBuf->SubstringLength : strlen(s_CurrentBuf->CurrentRec->text);
        
        if (s_CurrentBuf->NoUnicode) //Unicode is disabled - just duplicate the byte-count entry.
          LocalFail = PushInt(ByteCount);
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
          UpdateJournal(Buf, "\e<<MOUSE>>"); }
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
          int CurrentByte = s_CurrentBuf->CurrentByte;
          LocalFail |= ShiftRight(s_CurrentBuf, Chrs);
          s_CurrentBuf->SubstringLength = s_CurrentBuf->CurrentByte-CurrentByte;
          s_CurrentBuf->CurrentByte = CurrentByte; }
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
        if ( (Arg1 <= ' ' || 'a' <= Arg1 ) && (Arg1 != '~') ) {
          LocalFail = Fail("Invalid buffer key 0x%X", Arg1);
          break; }
        DestBuf = GetBuffer(Arg1, OptionallyNew);
        if (DestBuf == NULL) {
          LocalFail = Fail("Uninitialized buffer");
          break; }
        if ((DestBuf->CurrentRec) == NULL)
          GetRecord(DestBuf, 1);
        s_CurrentBuf = DestBuf;
        break; }

      case 'R': { //OR shift Right (left if -ve) no. of bytes popped off stack.
        long long Arg1 =  PopInt(&LocalFail);
        if (LocalFail)
          break;
        LocalFail |= ShiftRight(s_CurrentBuf, Arg1);
        break; }

      case 'E': { //OE Erase no. of characters specified by top of stack.
        long long Chrs =  PopInt(&LocalFail);
        int Bytes;
        if (LocalFail)
          break;
        if (s_CurrentBuf->EditLock & ReadOnly) {
          LocalFail = RunError("Attempt to modify a readonly buffer");
          break; }
        s_CurrentBuf->UnchangedStatus = 0;
         
        if (Chrs < 0) { // -ve. - Erase left Chrs charcaters. 
          int OrigCurrentByte = s_CurrentBuf->CurrentByte;
          if (ShiftRight(s_CurrentBuf, Chrs)) {
            s_CurrentBuf->CurrentByte = OrigCurrentByte;
            LocalFail = 1;
            break; }
          else
            Chrs = 0-Chrs; }
            
        Bytes = JotStrlenBytes(s_CurrentBuf->CurrentRec->text+s_CurrentBuf->CurrentByte, Chrs);
        if (strlen(s_CurrentBuf->CurrentRec->text+s_CurrentBuf->CurrentByte) < Bytes) {
          LocalFail = 1;
          break; }
        s_CurrentBuf->SubstringLength = Bytes;
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
      char Chr = FetchIntArg((struct IntArg **)&ThisArg);
      struct Buf *DestBuf;
      
      if (Chr == '~')
        DestBuf = GetBuffer(Chr, NeverNew);
      else {
        if ( (Chr <= ' ') || ('z' < Chr) ) {
          LocalFail = Fail("Invalid buffer key");
          break; }
        DestBuf = GetBuffer(Chr, OptionallyNew); }
      if ( ! DestBuf) {
        LocalFail = 1;
        break; }
      if ( ! DestBuf->CurrentRec)
        GetRecord(DestBuf, 1);
      s_CurrentBuf = DestBuf;
      break; }

    case '%': { //% - Percent-commands block.
      char BufferKey = '\0', *FoundEquals, *RightArg;
      int TextAsIs;
       
      s_CommandChr = FetchIntArg((struct IntArg **)&ThisArg);
      BufferKey = FetchIntArg((struct IntArg **)&ThisArg);
      RightArg = FetchStringArg((struct PtrArg **)&ThisArg, NULL);
      if ( (FoundEquals = strchr(RightArg, '=')) )
        RightArg += 1;
      TextAsIs = FetchIntArg((struct IntArg **)&ThisArg);
       
      switch (s_CommandChr) {
      case 'O': { //%O - Write buffer to file.
        char ResolvedPathName[StringMaxChr], *AppendQual, *OpenMode = "w";
        FILE *File = NULL;
        struct Rec *Record = s_CurrentBuf->FirstRec;
        struct Rec *LastRec = s_CurrentBuf->FirstRec->prev;
    
        ResolvedPathName[0] = '\0';
        
        if (FoundEquals)
          ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
        else
          s_TempBuf->CurrentRec->text[0] = '\0';
        if ( (AppendQual = strstr(s_TempBuf->CurrentRec->text, " -append")) ) {
          OpenMode = "a";
          AppendQual[0] = '\0'; }
        if (s_RecoveryMode) {
          if (s_Verbose & Verbose_NonSilent)
            Message(NULL, "Writing to \"%s\" was disabled", s_CurrentBuf->PathName); }
        else { //Really do the write.
          CrossmatchPathnames(NULL, &File, OpenMode, s_TempBuf->CurrentRec->text, s_CurrentBuf->PathName, "./", ResolvedPathName);
          if ( ! File) {
            LocalFail = Fail("Failed to open file \"%s\" for writing.", ResolvedPathName);
            break; }
          if (s_Verbose & Verbose_NonSilent) {
            Prompt("Writing to \"%s\"", ResolvedPathName);
            if (s_TTYMode)
              ScrollConsole(FALSE, FALSE); }
      
          while ( TRUE ) { // Write out records loop. 
            if (Record == LastRec) {
              if(0 < strlen(Record->text))
                fprintf(File, "%s", Record->text);
              break; }
            fprintf(File, "%s\n", Record->text);
            Record = Record->next; }
    
          if (fclose(File))
            LocalFail = 1; }
        s_CurrentBuf->UnchangedStatus |= SameSinceIO;
        break; }

      case 'C':  //%C - Exit writing new file.
        if (s_CurrentBuf->BufferKey != '.') {
          LocalFail = RunError("Can only %%%%C from main buffer.");
          break; }
        if (s_RecoveryMode) {
          Message(NULL, "Writing to \"%s\" was disabled", s_CurrentBuf->PathName);
          break; }
        if (strlen(RightArg)) {
          ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
          ExitNormally(s_TempBuf->CurrentRec->text); }
        else
          ExitNormally("Normal exit");
        break;

      case 'A': { //%A - Exit without writing file.
        int ExitStatus = BufferKey;
        if (FoundEquals) {
          sscanf(RightArg-1, "%d=", &ExitStatus);
          if (strlen(FoundEquals+1)) {
            ExpandDeferredString(s_TempBuf, FoundEquals+1, TextAsIs);
            if (TidyUp(ExitStatus, s_TempBuf->CurrentRec->text)) {
              LocalFail = 1;
              break; } } }
        else 
          if (TidyUp(ExitStatus, "Edit abandoned"))
            LocalFail = 1;
        break; }

      case 'R': { //%R - Run a command file.
        char *asConsole = NULL, *FirstBlank = NULL;
        char ScriptPath[StringMaxChr];
         
        ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
        if ((FirstBlank = strchr(s_TempBuf->CurrentRec->text, ' '))) {
          FirstBlank++[0] = '\0'; 
          if ((asConsole = strstr(FirstBlank, "-asConsole")))
            FirstBlank = strchr(FirstBlank, ' '); }
        ScriptPath[0] = '\0';
        CrossmatchPathnames(NULL, NULL, "r", s_TempBuf->CurrentRec->text, "./.jot", s_DefaultComFileName, ScriptPath);
        LocalFail = SwitchToComFile(ScriptPath, asConsole, FirstBlank ? FirstBlank : "");
        if (s_Return) {
          s_Return = FALSE;
          s_ExitBlocks = 0; }
        break; }

      case 'I': { //%I - secondary input.
        char *FileName = s_CurrentBuf->PathName;
        char Actual[StringMaxChr];
        char RecoveryPathName[StringMaxChr];
        char *AppendQual = NULL, *InsertQual = NULL, *HoldQual = NULL, *SectionQual = NULL, *FSectionQual = NULL;
        char *SeekQual = NULL, *BlockQual = NULL, *BytesQual = NULL, *RecordsQual = NULL, *BinaryQual = NULL;
        char *PathName;
        off64_t SeekOffset = 0;
        long Bytes = 0, OrigBytes = 0ll;
        int Records = 0;
        char * SectionKey = NULL;
        char * FSectionKey = NULL;
        struct Rec *OrigCurrentRec, *OrigNextRec, *OrigLastRec;
        int BinaryRecordSize = 0, NewRecCount;
        int LineNumber, CurrentByte = -1, SubstringLength;
        struct Buf *DestBuf;
        int FileDesc = 0;
        
        if (BufferKey == '\0')
          BufferKey = s_CurrentBuf->BufferKey;
        ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
        PathName = s_TempBuf->CurrentRec->text;
        SectionQual = strstr(s_TempBuf->CurrentRec->text, " -section=");
        FSectionQual = strstr(s_TempBuf->CurrentRec->text, " -fsection=");
        SeekQual = strstr(s_TempBuf->CurrentRec->text, " -seek=");
        BytesQual = strstr(s_TempBuf->CurrentRec->text, " -bytes=");
        BlockQual = strstr(s_TempBuf->CurrentRec->text, " -block=");
        RecordsQual = strstr(s_TempBuf->CurrentRec->text, " -records=");
        BinaryQual = strstr(s_TempBuf->CurrentRec->text, " -binary");
        InsertQual = strstr(s_TempBuf->CurrentRec->text, " -insert");
        AppendQual = strstr(s_TempBuf->CurrentRec->text, " -append");
        HoldQual = strstr(s_TempBuf->CurrentRec->text, " -hold");
        
        if (InsertQual)
          InsertQual[0] = '\0';
        else if (AppendQual)
          AppendQual[0] = '\0';
        if (HoldQual) {
          HoldQual[0] = '\0'; }
        if (SectionQual || FSectionQual) {
          if (SeekQual || BytesQual || BlockQual || RecordsQual) {
            LocalFail = SynError(s_CommandBuf, "%%%%I -section cannot be used with -seek, -bytes, -records or -block");
            break; }
          if (FSectionQual) {
            FSectionKey = FSectionQual+11;
            FSectionQual[0] = '\0'; }
          else {
            SectionKey = SectionQual+10;
            SectionQual[0] = '\0'; } }
        if (SeekQual) {
          sscanf(SeekQual, " -seek=%ld", (long *)&SeekOffset);
          SeekQual[0] = '\0'; }
        if (BytesQual) {
          sscanf(BytesQual, " -bytes=%ld", &Bytes);
          OrigBytes = Bytes;
          BytesQual[0] = '\0'; }
        if (BlockQual) {
          sscanf(BlockQual, " -block=%ld", &Bytes);
          OrigBytes = Bytes;
          BytesQual = "";
          BlockQual[0] = '\0'; }
        if (RecordsQual) {
          sscanf(RecordsQual, " -records=%d", &Records);
          RecordsQual[0] = '\0'; }
        if (BinaryQual) {
          sscanf(BinaryQual, " -binary=%d", &BinaryRecordSize);
          if ( ! BinaryRecordSize) {
            BinaryRecordSize = 16;
            sscanf(BinaryQual, " -binary"); }
          BinaryQual[0] = '\0'; }
        Actual[0] = '\0';
        if (SectionKey) {
          struct SetsectObj *Object;
          SeekQual = BytesQual = "";
          if ( ! (Object = QuerySection(SectionKey, s_CurrentBuf)) ) {
            LocalFail = Fail("Missing or invalid entry for key \"%s\"", SectionKey);
            break; }
          SeekOffset = (off64_t)Object->Seek;
          Bytes = Object->Bytes; }
        if (FSectionKey) {
          struct SetfsectObj *Object;
          SeekQual = BytesQual = "";
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
          CrossmatchPathnames(&FileDesc, NULL, "r", FoundEquals ? PathName : FileName, FileName, "./", Actual);
        if ( ! FileDesc) {
          LocalFail = 1;
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
            if ( ! s_CurrentBuf->CurrentRec )
              GetRecord(s_CurrentBuf, 1);
            else {
              LineNumber = DestBuf->LineNumber;
              CurrentByte = DestBuf->CurrentByte;
              SubstringLength = s_CurrentBuf->SubstringLength;
              BreakRecord(s_CurrentBuf);
              OrigNextRec = s_CurrentBuf->CurrentRec;
              AdvanceRecord(s_CurrentBuf, -1);
              OrigCurrentRec = s_CurrentBuf->CurrentRec;
              OrigLastRec = s_CurrentBuf->FirstRec->prev->prev; } }
          if (AppendQual) {
            s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec->prev;
            s_CurrentBuf->CurrentByte = strlen(s_CurrentBuf->CurrentRec->text); } }
        if (s_RecoveryMode) { //In recovery mode prompt then read the journal-area pathname.
          if (FileDesc)
            close(FileDesc);
          Prompt("Enter recovery-substitution file for %s", FoundEquals ? s_TempBuf->CurrentRec->text : FileName);
          if (fgets(RecoveryPathName, StringMaxChr, s_asConsole->FileHandle)) {
            RecoveryPathName[strlen(RecoveryPathName)-1] = '\0';
            Message(NULL, "Using recovery-substitution \"%s\"", RecoveryPathName); }
          else {
            LocalFail = RunError("Failed to read recovery-substitution pathname");
            break; }
          if ( ! (FileDesc = open(RecoveryPathName, O_RDONLY))) {
            LocalFail = RunError("Error opening recovery-substitution file \"%s\"", RecoveryPathName);
            break; } }
            
        if (FileDesc) {
#if defined(VC)
          NewRecCount = ReadBuffer(NULL, NULL, FileDesc, FALSE, Actual, s_CurrentBuf, BinaryRecordSize, SeekOffset, BytesQual ? &Bytes : NULL, BlockQual ? TRUE : FALSE, Records);
#else
          NewRecCount = ReadBuffer(NULL, FileDesc, FALSE, Actual, s_CurrentBuf, BinaryRecordSize, SeekOffset, (BytesQual ? &Bytes : NULL), BlockQual ? TRUE : FALSE, Records);
#endif
          if (HoldQual)
            DestBuf->IOBlock->HoldDesc = FileDesc;
          else
            close(FileDesc); }
        else {
          if (s_JournalHand)
            UpdateJournal("<<Read file failed>>", NULL);
          LocalFail = 1;
          break; }
          
        if ( ! BlockQual) {
          if (AppendQual || InsertQual) {
            if (CurrentByte < 0) { //This signifies that the buffer was originally undefined - but with a null record, remove that now unless it's the only record.
              if (s_CurrentBuf->FirstRec != s_CurrentBuf->FirstRec->next) {
                s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec;
                FreeRecord(s_CurrentBuf, DeleteNotAdjust); } }
            else if ( ! NewRecCount) {
              s_CurrentBuf->CurrentRec = s_CurrentBuf->CurrentRec->prev;
              JoinRecords(s_CurrentBuf);
              s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec;
              FreeRecord(s_CurrentBuf, DeleteNotAdjust); }
            else if (AppendQual && OrigCurrentRec ==  s_CurrentBuf->FirstRec && CurrentByte == 0) { //Was at start of buffer.
              s_CurrentBuf->CurrentRec = OrigLastRec->next;
              JoinRecords(s_CurrentBuf); 
              s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec;
              JoinRecords(s_CurrentBuf);
              ResetBuffer(s_CurrentBuf); }
            else {
              if (InsertQual) {
                s_CurrentBuf->CurrentRec = OrigNextRec->prev;
                JoinRecords(s_CurrentBuf); }
              else {
                s_CurrentBuf->CurrentRec = OrigLastRec->next;
                if (s_CurrentBuf->CurrentRec != s_CurrentBuf->FirstRec->prev)
                  JoinRecords(s_CurrentBuf); }
              s_CurrentBuf->CurrentRec = OrigCurrentRec;
              JoinRecords(s_CurrentBuf);
              s_CurrentBuf->LineNumber = LineNumber;
              s_CurrentBuf->CurrentByte = CurrentByte;
              if (AppendQual)
                s_CurrentBuf->SubstringLength = SubstringLength; } }
          if (InsertQual)
            s_CurrentBuf->UnchangedStatus = 0;
          else
            s_CurrentBuf->UnchangedStatus |= SameSinceIO; }
        if (BytesQual && OrigBytes == 0ll)
          break;
        if (NewRecCount < 0)
          LocalFail = 1;
        break; }

      case 'D':  //%D - Define a buffer from console or last console command.
      case 'G':  //%G - Get - but from current command file (or console).
        if ( (s_CommandChr == 'G') && (s_CurrentCommand->CommandBuf != s_CommandBuf) ) //%G command from a macro - buffer set up at compile time.
          break;
        else {
          int IsCurrentBuf = (BufferKey == s_CurrentBuf->BufferKey);
          struct Buf *DestBuf = NULL;
           
          if ( ( ! CheckBufferKey(BufferKey)) || ( ( ! (DestBuf = GetBuffer(BufferKey, AlwaysNew))) ) ) {
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
         
          if (s_CommandChr == 'D') { //%D
            LocalFail |= ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
            LocalFail |= GetRecord(DestBuf, strlen(s_TempBuf->CurrentRec->text)+1);
            LocalFail |= SubstituteString(DestBuf, s_TempBuf->CurrentRec->text, -1); }
          else if (s_CommandChr == 'G') { //%G
            for ( ; ; ) { // Insert records loop. 
              char *Record;
              if (s_EditorInput == s_EditorConsole) {
                Prompt("%s", strlen(s_PromptString) ? s_PromptString : "> "); }
              else
                s_EditorInput->LineNo++;
              if ( ! ReadNewRecord(DestBuf, s_EditorInput->FileHandle, FALSE)) {
                if (s_JournalHand)
                  UpdateJournal("<<G-command terminated>>", NULL);
                break; }
              Record = (DestBuf->CurrentRec->text);
              if ((strlen(Record) == 1) && (Record[0] == ':')) { // Last record. 
                if (DestBuf->FirstRec == DestBuf->FirstRec->next)
                  DestBuf->CurrentRec->text[0] = '\0';
                else
                  LocalFail |= FreeRecord(DestBuf, AdjustForwards);
                break; }
              if (LocalFail)
                break; }
            ResetBuffer(DestBuf); }
          break; }

      case 'E': { //%E - Execute following CLI command line.
        int Status = 0;
        if (BufferKey != '\0') {
          char *NameString;
          struct Buf *DestBuf;
          FILE *File = NULL;
          char *ActualCommands = RightArg;
          int StreamInBuf = FALSE, Async = FALSE, ReadOK;
#if defined(VC)
          HANDLE InHand = NULL, OutHand = NULL;
          HANDLE ProcessId;
          int OutDesc = 0;
#else
          int InDesc = 0, OutDesc = 0;
          pid_t ChildPid;
#endif
           
          DestBuf = GetBuffer(BufferKey, AlwaysNew);
          if (DestBuf == NULL) {
            LocalFail = Fail("Invalid buffer key");
            break; }
          
          if (s_RecoveryMode) {
            char RecoveryPathName[StringMaxChr];
            Prompt("Enter recovery-substitution file for %s", RightArg);
            do
              ReadString(RecoveryPathName, StringMaxChr, s_asConsole->FileHandle);
              while ( RecoveryPathName[0] == '\0');
            Message(NULL, "Using recovery-substitution \"%s\"", RecoveryPathName);
            if ( (OutDesc = open(RecoveryPathName, O_RDONLY)) <= 0) {
                LocalFail = RunError("Error opening recovery-substitution file \"%s\"", RecoveryPathName);
                break; } }
          else { //Normal command mode (i.e. not a recovery session).
            if (RightArg[0] == '|') {
              ActualCommands += 1;
              StreamInBuf = TRUE; }
            else if (RightArg[0] == '&') {
              if (s_JournalHand) {
                LocalFail = Fail("Asynchronous I/O not available  with journal files.");
                break; }
              ActualCommands += 1;
              Async = TRUE;
              
              if (DestBuf->IOBlock) {
                if (DestBuf->IOBlock->OutPipe) {
                  LocalFail = Fail("Buffer ( %c ) already has asynchronous I/O set up.", DestBuf->BufferKey);
                  break; } }
              else {
                DestBuf->IOBlock = (struct IOBlock *)JotMalloc(sizeof(struct IOBlock));
#if defined(LINUX)
                DestBuf->IOBlock->ChildPid = 0;
#endif
                DestBuf->IOBlock->HoldDesc = 0;
                DestBuf->IOBlock->ExitCommand = NULL;
                DestBuf->IOBlock->BucketBlock = NULL; } }
            
#if defined(VC)
            if (Async)
              OpenWindowsCommand(&InHand, &OutHand, &ProcessId, StreamInBuf, Async, ActualCommands);
            else
              OpenWindowsCommand(NULL, &OutHand, NULL, StreamInBuf, Async, ActualCommands);
#else
            if (StreamInBuf)
              DoublePopen(NULL, &OutDesc, &ChildPid, s_CurrentBuf, ActualCommands);
            else if (Async) //Add this File to select list.
              DoublePopen(&InDesc, &OutDesc, &ChildPid, NULL, ActualCommands);
            else if ( ! (File = popen(ActualCommands, "r")) ) {
              GetRecord(DestBuf, 0);
              LocalFail = Fail("Could not open reply stream");
              break; }
#endif
            }
          NameString = (char *)JotMalloc(strlen(RightArg)+25);
          sprintf(NameString, "[ From CLI command %s ]", RightArg);
            
          if ( ! CheckBufferKey(BufferKey)) {
            LocalFail = Fail("Invalid buffer key \'%c\'", BufferKey);
            break; }
          s_CurrentBuf = DestBuf;
#if defined(VC)
          ReadOK = (0 < ReadBuffer(File, OutHand, OutDesc, TRUE, NameString, s_CurrentBuf, 0, 0, NULL, FALSE, 0));
#else
          if ( ! Async)
            ReadOK = (0 < ReadBuffer(File, OutDesc, TRUE, NameString, s_CurrentBuf, 0, 0, NULL, FALSE, 0));
#endif
           
          free(NameString);
          
          if (Async) {
            DestBuf->IOBlock->NextAsyncBuf = s_FirstAsyncBuf;
            s_FirstAsyncBuf = DestBuf;
#if defined(VC)
            DestBuf->IOBlock->InPipe = InHand;
            DestBuf->IOBlock->OutPipe = OutHand; 
            DestBuf->IOBlock->ProcessId = ProcessId; 
#else
            DestBuf->IOBlock->InPipe = InDesc;
            DestBuf->IOBlock->OutPipe = OutDesc;
            DestBuf->IOBlock->ChildPid = ChildPid;
#endif
            if ( ! s_CurrentBuf->CurrentRec)
              GetRecord(s_CurrentBuf, 1);
            usleep(100000);
            LocalFail = AsyncReadRecs();
            break; }
            
          if ( ! ReadOK) {
            LocalFail = 1;
            break; }
#ifdef VC
          if (DestBuf->IOBlock->InPipe)
            CloseHandle(DestBuf->IOBlock->InPipe);
          if (DestBuf->IOBlock->OutPipe)
            CloseHandle(DestBuf->IOBlock->OutPipe);
          if (s_RecoveryMode)
            close(OutDesc);
#else
          if (DestBuf->IOBlock->InPipe)
            close(DestBuf->IOBlock->InPipe);
          if (DestBuf->IOBlock->OutPipe)
            close(DestBuf->IOBlock->OutPipe);
          if (s_RecoveryMode)
            close(OutDesc);
          if (File)
            Status = pclose(File);
          if (StreamInBuf)
            waitpid(ChildPid, NULL, 0);
#endif
          }
        else { //Just run and, if it writes to it's stdout, let it mess up the screen.
          char CommandString[StringMaxChr];
           
          strcpy(CommandString, RightArg);
          Status = system(CommandString);
#ifdef LINUX
          clearok(mainWin, TRUE);
#endif
          }
        if (Status != 0)
          LocalFail = 1;
        break; }

      case 'F': { //%F - Regular-expression search.
        int Direction = FetchIntArg((struct IntArg **)&ThisArg);
        ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
        if (BufferKey == s_CurrentBuf->BufferKey) {
          LocalFail = SynError(s_CommandBuf, "Destination buffer for %%F is current buffer");
          break; }
        if (s_TempBuf->CurrentRec->text[0] == '\0') {
          LocalFail = Fail("Empty search expression in %%F.");
          break; }
        if (s_TempBuf->CurrentRec->text[0] != '\0') {
          if (s_TempBuf->CurrentRec->text[1] != '\0') //This is bollox but single-character RegEx expressions cause valgrind to throw a wobblyy (doubtless it's found a real error though)
          strncpy(s_RegExString, s_TempBuf->CurrentRec->text, StringMaxChr); }
#if defined(NoRegEx)
        for ( ; ; ) { // Record loop, if not found move record pointer.
          int InitialPointer = s_CurrentBuf->CurrentByte;
          if ( (LocalFail |= SearchRecord(s_CurrentBuf, s_RegExString, Direction)) ) // Substring found.
            break;
          if (LocalFail |= AdvanceRecord(s_CurrentBuf, Direction))
            break;
          if (Direction < 0)
            s_CurrentBuf->CurrentByte = strlen(s_CurrentBuf->CurrentRec->text); }
        break;
#else
        if (RegexSearchBuffer(s_CurrentBuf, BufferKey, s_RegExString, Direction) == 0)
          LocalFail = 1;
        break;
#endif
        }

      case 'U':  //%U - Undo last substitution.
        if (s_CurrentBuf->EditLock & ReadOnly) {
          LocalFail = RunError("Attempt to modify a readonly buffer");
          break; }
        s_CurrentBuf->UnchangedStatus = 0;
         
        LocalFail = SubstituteString(s_CurrentBuf, s_SearchString, -1);
        break;

      case 'L': { //%L - set line Length.
        int NewLength, GivenWidth, GivenHeight;
         
        if (s_TTYMode)
          break;
        if (s_PredefinedScreenSize)
          break;
          
        NewLength = 0;
#if defined(VC)
        GetConsoleScreenBufferInfo(hStdout, &csbiInfo);
        {
          SMALL_RECT SrWindow = csbiInfo.srWindow;
          s_TermWidth = SrWindow.Right-SrWindow.Left+1;
          s_TermHeight = SrWindow.Bottom-SrWindow.Top+1; }
#elif defined(LINUX)
        s_TermHeight = getmaxy(mainWin);
        s_TermWidth = getmaxx(mainWin);
#endif
        if ( (RightArg == NULL) || (strlen(RightArg) == 0) )
          NewLength = s_TermWidth;
        else
          sscanf(RightArg, "%i", &NewLength);
          
        if (RightArg[0]) { //Term-size expression follows.
          ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
          sscanf(s_TempBuf->CurrentRec->text + s_TempBuf->CurrentByte, "%d %d", &GivenWidth, &GivenHeight);
          if (s_TermWidth < GivenWidth || s_TermHeight < GivenHeight) {
            LocalFail = Fail("Height and/or width given in %%L expression exceedes corresponding terminal dimension - reverting to %dx%d",
            s_TermWidth, s_TermHeight);
            break; }
          s_TermWidth = GivenWidth;
          if (GivenHeight)
            s_TermHeight = GivenHeight; }
        else if (s_JournalHand) {
          char Buf[100];
          sprintf(Buf, "%d %d", s_TermWidth, s_TermHeight);
          UpdateJournal(Buf, "\e<<TERM>>"); }
        LocalFail |= InitTermTable();
#if defined(LINUX)
        clear();
        refresh(); 
#endif
        break; }

      case '%':  //%% - Comment line.
        break;

      case '~':  //%~ - Insert or display control character.
        if (strlen(RightArg) != 0) { // Insert a byte. 
          char TempString[1000];
          int Value;
           
          if (s_CurrentBuf->EditLock & ReadOnly) {
            LocalFail = RunError("Attempt to modify a readonly buffer");
            break; }
           
          sscanf(RightArg, "%x", &Value);
          TempString[0] = Value;
          TempString[1] = '\0';
          if ( ( ! s_CurrentBuf->NoUnicode) && (mblen(TempString, 1) <= 0) )
            LocalFail = Fail("The %%~ operation would have introduced invalid unicode into the buffer.");
          else {
            LocalFail = SubstituteString(s_CurrentBuf, TempString, -1);
            s_CurrentBuf->UnchangedStatus = 0; }
          break; }
        Message(NULL, "\"%X\"", (int)(s_CurrentBuf->CurrentRec->text)[s_CurrentBuf->CurrentByte]);
        break;

      case 'W': //%W - Set up a screen window.
        ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
        LocalFail = ChangeWindows(BufferKey, s_TempBuf);
        break;

      case 'X': {  //%X - Exit with a run-time error message.
        ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
        if ( ! s_TTYMode && s_TermWidth < strlen(s_TempBuf->CurrentRec->text)) {
          Message(NULL, "Over-long %%X message string truncated to %d characters", s_TermWidth-1);
          s_TempBuf->CurrentRec->text[s_TermWidth-1] = '\0'; }
        Message(NULL, "%s", s_TempBuf->CurrentRec->text);
        LocalFail = 1;
        s_BombOut = TRUE;
        break; }

     case 'S': { //%S - System settings and utilities.
        char Qualifier[20], *Args, ValueString[StringMaxChr];
        int ArgsPtr;
        
        if (sscanf(RightArg, "%s %n%s", (char *)&Qualifier, &ArgsPtr, (char *)&ValueString) <=1 )
          ValueString[0] = '\0';
        Args = RightArg+ArgsPtr;
        if (strstr(Qualifier, "commandmode") == Qualifier) { //%s=commandmode - controls command/type-into-screen mode.
          int Value, OrigValue = s_CommandMode;
          sscanf(ValueString, "%x", &Value);
          if (ValueString[0] == '+')
            s_CommandMode = s_CommandMode ^ Value;
          else
            s_CommandMode = Value;
          Message(NULL, "Command mode was %X, now set to %X", OrigValue, s_CommandMode);
          break; }

        else if (strstr(Qualifier, "recoverymode") == Qualifier) { //%s=recoverymode <n> -  disables %O command, %I prompts and reads filename from console (the recovery script).
          int Value;
          struct Buf *PrimaryBuffer = GetBuffer('.', NeverNew);
          
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          if (sscanf(s_TempBuf->CurrentRec->text, "%d", &Value) <= 0)
            Value = 0;
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
              CrossmatchPathnames(NULL, &s_JournalHand, "a", s_JournalPath, "history.txt", NULL, Actual);
              if ( s_JournalHand == NULL)
                Disaster("Failed to open journal file \"%s\"", FullPath);
              DateTime = time(NULL);
              strftime(TimeStamp, 100, "%d/%m/%y, %H:%M:%S", localtime(&DateTime));
              sprintf(Temp, "<<Recovery session pid %d, at %s>>", getpid(), TimeStamp);
              UpdateJournal(Temp, NULL); } } }

        else if (strstr(Qualifier, "guardband") == Qualifier) { //%s=guardband <n> - Set window guardband.
          int Value;
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          if (sscanf(s_TempBuf->CurrentRec->text, "%d", &Value) <= 0)
            Value = 0;
          s_GuardBandSize = Value;
          Message(NULL, "Window guardband set to %d", s_GuardBandSize); }

        else if (strstr(Qualifier, "console") == Qualifier) { //%s=console <n> - Set the overall console-area size.
          int Value;
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          if (sscanf(s_TempBuf->CurrentRec->text, "%d", &Value) <= 0)
            Value = 0;
          if (s_TermHeight < Value) {
            Fail("Console size cannot be greater than the terminal height.");
            Value = 0; }
          s_MaxConsoleLines = Value; }

        else if (strstr(Qualifier, "verbose") == Qualifier) { //%s=verbose <n> - Set/reset verbose mode.
          int Value;
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          if (sscanf(s_TempBuf->CurrentRec->text, "%x", &Value) <= 0)
            Value = 0;
          if (s_Verbose & Verbose_NonSilent && Value & Verbose_NonSilent)
            Message(NULL, "Verbosity set to %X (was %X)", Value, s_Verbose);
          s_Verbose = Value; }

//#if !defined(CHROME)
//#if !defined(noX11)
//        else if (strstr(Qualifier, "copy") == Qualifier) { //%s=copy - Text between latest note point and current chr defines paste buffer.
//          int Size = 0, Chr = 0, StatusBits; 
//          char *Temp;
//          int ToByteNo = s_AbstractWholeRecordFlag ? strlen(s_CurrentBuf->CurrentRec->text) : s_CurrentBuf->CurrentByte;
//          int FromByteNo = s_AbstractWholeRecordFlag ? 0 : s_NoteByte;
//          int FromLineNo = s_NoteLine;
//          int ToLineNo = s_CurrentBuf->LineNumber;
//#if defined(VC)
//          HANDLE hData;
//          char *ptrData;
//#endif
//          
//          if ( (ToLineNo < FromLineNo) || ((ToLineNo == FromLineNo) && (ToByteNo < FromByteNo)) ) { //Abstracting backwards - switch around.
//            FromLineNo = ToLineNo;
//            ToLineNo = s_NoteLine;
//            FromByteNo = ToByteNo; 
//            ToByteNo = s_AbstractWholeRecordFlag ? 0 : s_NoteByte;
//            LocalFail |= AdvanceRecord(s_CurrentBuf, FromLineNo-ToLineNo); }
//          if ( ! s_NoteBuffer) {
//            LocalFail = 1;
//            break; }
//          if (s_NoteBuffer != s_CurrentBuf) {
//            LocalFail = RunError("The note point is in a different buffer");
//            break; }
//          s_NoteBuffer = NULL;
//          if (s_CurrentBuf->BufferKey == BufferKey) {
//            LocalFail = Fail("Abstract source and destination is the same buffer");
//            break; }
//            
//          AdvanceRecord(s_CurrentBuf, FromLineNo-ToLineNo);
//          while (s_CurrentBuf->LineNumber != ToLineNo) {
//            Size += strlen(s_CurrentBuf->CurrentRec->text)+1;
//            AdvanceRecord(s_CurrentBuf, 1); }
//          Size += ToByteNo - FromByteNo;
//          
//          Temp = (char *)JotMalloc(Size+1);
//          AdvanceRecord(s_CurrentBuf, FromLineNo-ToLineNo);
//          s_CurrentBuf->CurrentByte = FromByteNo;
//          while (s_CurrentBuf->LineNumber != ToLineNo) {
//            char *Line = s_CurrentBuf->CurrentRec->text + s_CurrentBuf->CurrentByte;
//            strcpy(Temp+Chr, Line);
//            Chr += strlen(Line);
//            Temp[Chr++] = '\n';
//            AdvanceRecord(s_CurrentBuf, 1); }
//          strncpy(Temp+Chr, s_CurrentBuf->CurrentRec->text+s_CurrentBuf->CurrentByte, ToByteNo-s_CurrentBuf->CurrentByte);
//          Chr += ToByteNo-s_CurrentBuf->CurrentByte;
//          Temp[Chr++] = '\0';
//          s_NoteBuffer = NULL;
//          
//#if defined(VC)
//          hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, Size);
//          ptrData = (char*)GlobalLock(hData);
//          memcpy(ptrData, Temp, Chr);
//          GlobalUnlock(hData);
//          OpenClipboard(NULL);
//          EmptyClipboard();
//          SetClipboardData(CF_TEXT, hData);
//          CloseClipboard();
//          Message(NULL, "Sent %d bytes", Size);
//#else
//          Display *Disp = XOpenDisplay(NULL);
//          Widget top;
//          XtAppContext context;
//          int Args = 0;
//          int xtime_out = 300; // in ms
//          jmp_buf *RestartPoint = (jmp_buf *)JotMalloc(sizeof(jmp_buf));
//          void CopyTimeout(XtPointer p, XtIntervalId*i) {
//            longjmp(*RestartPoint, 1); }
//          
//          if (setjmp(*RestartPoint) != 0)
//            return 0;
//          
//          top = XtVaAppInitialize(&context, "ClearCutBuffers", 0, 0, &Args, NULL, NULL,NULL);
//          Disp = XtDisplay(top);
//          StatusBits = XStoreBuffer(Disp, Temp, Size, 0);
//          XtAppAddTimeOut(context, xtime_out, CopyTimeout, 0);
//          XtAppMainLoop(context);
//          Message(NULL, "Sent %d bytes, status %X", Size, StatusBits);
//#endif
//          free(Temp); }
//
//        else if (strstr(Qualifier, "paste") == Qualifier) { //%s=paste - returns contents of paste buffer to nominated buffer.
//          char *PasteBuf;
//          int FirstLineNo = s_CurrentBuf->LineNumber;
//          int LineStart = 0;
//          int LastLineNo;
//#if defined(VC)
//          HANDLE hClipboardData;
//         
//          OpenClipboard(NULL);
//          hClipboardData = GetClipboardData(CF_TEXT);
//          if ( ! (PasteBuf = (char*)GlobalLock(hClipboardData))) {
//            LocalFail = Fail("Failed to open windows paste buffer");
//            break; }
//#else
//          int BufLength = 0;
//          Display *Disp = XOpenDisplay(NULL);
//          PasteBuf = XFetchBuffer(Disp, &BufLength, 0);
//#endif
//          BreakRecord(s_CurrentBuf);
//          AdvanceRecord(s_CurrentBuf, -1);
//          while ( PasteBuf[LineStart] != '\0' ) {
//            char * LineEnd = strchr(PasteBuf+LineStart, '\n');
//            if (LineEnd)
//              LineEnd[0] = '\0';
//            else
//              LineEnd = PasteBuf+strlen(PasteBuf);
//            AddFormattedRecord(TRUE, s_CurrentBuf, PasteBuf+LineStart);
//            LineStart += LineEnd-(PasteBuf+LineStart)+1; }
//          LastLineNo = s_CurrentBuf->LineNumber;
//          AdvanceRecord(s_CurrentBuf, FirstLineNo-LastLineNo+1);
//          JoinRecords(s_CurrentBuf);
//          AdvanceRecord(s_CurrentBuf, LastLineNo-FirstLineNo-2);
//          JoinRecords(s_CurrentBuf);
//          s_CurrentBuf->LineNumber -= 1;
//#if defined(VC)
//          GlobalUnlock(hClipboardData);
//          CloseClipboard();
//#endif
//           }
//#endif
//#endif

        else if (strstr(Qualifier, "mousemask") == Qualifier) { //%s=mousemask <x> - Define the mouse mask
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          if (sscanf(s_TempBuf->CurrentRec->text, "%x", &s_MouseMask) <= 0)
#if defined(VC)
            s_MouseMask = -1;
          SetConsoleMode(hStdin, s_MouseMask ? ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT : 0);
#else
            s_MouseMask = ALL_MOUSE_EVENTS;
          mousemask(s_MouseMask, NULL);
#endif
          }

        else if (strstr(Qualifier, "setenv") == Qualifier) { //%s=setenv <name> <value> - set env variable for the session.
          char EnvName[StringMaxChr];
          int ChrPtr;
          ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
          if (sscanf(s_TempBuf->CurrentRec->text+6, "%s %n", (char *)&EnvName, &ChrPtr) <= 0) {
            LocalFail = RunError("Bad syntax for SetEnv");
            break; }
          if (setenv(EnvName, RightArg+6+ChrPtr, -1))
            LocalFail = 1;
          break; }
             
        else if (strstr(Qualifier, "case") == Qualifier) { //%s=case {0|1} -  controls case sensitivity for F command.
          int Value;
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          if (sscanf(s_TempBuf->CurrentRec->text, "%x", &Value) <= 0)
            Value = 0;
          s_CaseSensitivity = Value;
          if (s_Verbose & Verbose_QuiteChatty) {
            if (s_CaseSensitivity)
              Message(NULL, "Case sensitivity on"); 
            else
              Message(NULL, "Case sensitivity off"); } }
             
        else if (strstr(Qualifier, "system") == Qualifier) { //%s=system {0|1} - preserves current value of default strings.
          if (sscanf(ValueString, "%d", &s_SystemMode) <= 0)
            s_SystemMode = 0; }
        
        else if (strstr(Qualifier, "traceskip") == Qualifier) { //%s=traceskip - skip-over blocks, macros, functions, scripts etc.
          int RelativeSkipLevel;
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          if ( ! (sscanf(s_TempBuf->CurrentRec->text, "%d", &RelativeSkipLevel) ) )
            RelativeSkipLevel = 0;
          s_MaxTraceLevel = s_TraceLevel-RelativeSkipLevel;
          s_TraceSkipped = FALSE; }
          
        else if (strstr(Qualifier, "tracedefault") == Qualifier) { //%s=tracedefsult <x> - Set default trace vector set by t command.
          int Value, OrigValue = s_DefaultTraceMode;
          char Sign;
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          Sign = s_TempBuf->CurrentRec->text[0];
          if (sscanf(s_TempBuf->CurrentRec->text+((Sign == '+' || Sign == '-') ? 1 : 0), "%x", &Value) <= 0)
            Value = 0;
          if (Sign == '+')
            s_DefaultTraceMode |= Value;
          else if (Sign == '-')
            s_DefaultTraceMode &= ~Value;
          else
            s_DefaultTraceMode = Value;
          if (s_Verbose & Verbose_NonSilent)
            Message(NULL, "Trace-default mode was %X, now set to %X", OrigValue, s_DefaultTraceMode); }
         
        else if (strstr(Qualifier, "trace") == Qualifier) { //%s=trace <x> - set Trace mode. Prefix argument with '+' to XOR with current value.
          int Value, OrigValue = s_TraceMode;
          char Sign;
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          Sign = s_TempBuf->CurrentRec->text[0];
          if (sscanf(s_TempBuf->CurrentRec->text+((Sign == '+' || Sign == '-') ? 1 : 0), "%x", &Value) <= 0)
            Value = 0;
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
              sprintf(Temp, "<<Debugger on>> %d", s_JournalDataLines);
              UpdateJournal(Temp, NULL); }
            else
              UpdateJournal("<<Debugger off>>", NULL); }
          break; }

        else if (strstr(Qualifier, "commandcounter") == Qualifier) { //%s=commandcounter <n> - sets the debug counter to some positive value.
          unsigned long long int Value;
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          if (sscanf(s_TempBuf->CurrentRec->text, "%lld", &Value) <= 0)
            SynError(s_CommandBuf, "No valid value given for command counter");
          else
            s_CommandCounter = s_CommandCounterInit = 0ll-Value;
          if (s_Verbose & Verbose_NonSilent)
            Message(NULL, "Command sequence will be interrupted after %lld commands", Value); }

        else if (strstr(Qualifier, "tab") == Qualifier) { //%s=tab <n> - table-column separator character.
          char Value;
          ExpandDeferredString(s_TempBuf, ValueString, TextAsIs);
          if (sscanf(s_TempBuf->CurrentRec->text, "%c", &Value) <= 0)
            Value = '\t';
          s_TableSeparator = Value; }

        else if (strstr(Qualifier, "commandstring") == Qualifier) { //%s=commandstring <string> - return modified string back to command-line input.
          if (Args[0] == ' ')
            Args++;
          if (s_ModifiedCommandString) {
            LocalFail = RunError("Recursive use of %s=commandstring is not supported.");
            break; }
          s_ModifiedCommandString = (char *)JotMalloc(strlen(Args)+1);
          strcpy(s_ModifiedCommandString, Args);
          if (s_JournalHand)
            UpdateJournal("<<%s=commandstring>>", NULL); }

        else if (strstr(Qualifier, "prompt") == Qualifier) { //%s=prompt <string> - sets the prompt string used by G command.
          ExpandDeferredString(s_TempBuf, RightArg+7, TextAsIs);
          if (strlen(s_TempBuf->CurrentRec->text)) {
            strncpy(s_PromptString, s_TempBuf->CurrentRec->text, StringMaxChr);
            s_PromptString[StringMaxChr-1] = '\0'; }
          else
            s_PromptString[0] = '\0'; }

        else if (strstr(Qualifier, "on_key") == Qualifier) { //%s=on_key <CommandString> - specifies a command string to be executed after a command
          int Chrs;
          ExpandDeferredString(s_TempBuf, RightArg+7, TextAsIs);
          Chrs = strlen(s_TempBuf->CurrentRec->text);
          if (s_OnKey)
            FreeBuffer(s_OnKey);
          if (Chrs) {
            s_OnKey = GetBuffer('o', AlwaysNew);
            GetRecord(s_OnKey, Chrs+1);
            strcpy(s_OnKey->CurrentRec->text, s_TempBuf->CurrentRec->text); }
          else
            s_OnKey = NULL; }

        else if (strstr(Qualifier, "setmouse") == Qualifier) { //%s=setmouse <value> - sets the mouse coordinates returned by OP and %q=window commands.
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
            Win = Win->next; } }
           
        else { 
          LocalFail = SynError(s_CommandBuf, "Invalid %%%%s command qualifier \"%s\"", Qualifier);
          break; }
          
        break; }

      case 'M': { //%M - MessageText.
        struct Buf *DestBuf;
        ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
        if (BufferKey != '\0') { //Direct message to buffer as well as console.
          if ( ! (DestBuf = GetBuffer(BufferKey, OptionallyNew))) {
            LocalFail = 1;
            break; }
          if ( ! DestBuf->FirstRec) //A newly-minted buffer with no records.
            GetRecord(DestBuf, 1);
          
          DestBuf->CurrentRec = DestBuf->FirstRec->prev;
          GetRecord(DestBuf, strlen(s_TempBuf->CurrentRec->text)+1);
          strcpy(DestBuf->CurrentRec->text, s_TempBuf->CurrentRec->text); }
        Message(NULL, "%s", s_TempBuf->CurrentRec->text);
#if defined(LINUX)
        refresh();
#endif
        break; }

      case 'P': { //%P - Pipe text (pipe set up by %E with & modifier).
        struct Buf *DestBuf;
#if defined(VC)
        int Bytes;
#endif
        char *ExitOpt;
        
        ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
        if (BufferKey != '\0') {
          if ( ! (DestBuf = GetBuffer(BufferKey, NeverNew))) {
            LocalFail = 1;
            break; } }
        else
          DestBuf = s_CurrentBuf;
        if ( ! DestBuf->IOBlock || ! DestBuf->IOBlock->InPipe) {
          LocalFail = Fail("Buffer ( %c ) is not set up for asynchronous I/O.");
          break; }
        if ( (ExitOpt = strstr(s_TempBuf->CurrentRec->text, " -exit=")) ) {
          if (DestBuf->IOBlock->ExitCommand)
            free(DestBuf->IOBlock->ExitCommand);
          DestBuf->IOBlock->ExitCommand = (char *)JotMalloc(strlen(s_TempBuf->CurrentRec->text + 7)+1);
          strcpy(DestBuf->IOBlock->ExitCommand, s_TempBuf->CurrentRec->text + 7);
          break; }
#if defined(VC)
        WriteFile(DestBuf->IOBlock->InPipe, s_TempBuf->CurrentRec->text, strlen(s_TempBuf->CurrentRec->text), &Bytes, NULL);
        WriteFile(DestBuf->IOBlock->InPipe, "\n", 1, &Bytes, NULL);
#else
        write(DestBuf->IOBlock->InPipe, s_TempBuf->CurrentRec->text, strlen(s_TempBuf->CurrentRec->text));
        write(DestBuf->IOBlock->InPipe, "\n", 1);
#endif
        DestBuf->CurrentRec = DestBuf->FirstRec->prev;
        DestBuf->CurrentByte = strlen(DestBuf->CurrentRec->text);
        DestBuf->SubstringLength = 0;
        SubstituteString(DestBuf, s_TempBuf->CurrentRec->text, -1);
        usleep(100000);
        LocalFail = AsyncReadRecs();
        break; }

      case 'B': { //%B - Buffer attributes and operations.
        char Qualifier[20], *Modifier, FirstChr;
        int Chrs;
        ExpandDeferredString(s_TempBuf, RightArg, TextAsIs);
        sscanf(s_TempBuf->CurrentRec->text, "%s %c%n", (char *)&Qualifier, &FirstChr, &Chrs);
         
        if (strstr(Qualifier, "unrestricted") == Qualifier) { //As it says - no restrictions.
          s_CurrentBuf->EditLock = 0;
          break; }
        
        else if (strstr(Qualifier, "readonly") == Qualifier) { //Prohibits changes to this buffer.
          int NewReadonlyState;
          s_CurrentBuf->EditLock |= ReadOnly;
          if ((0 < sscanf(s_TempBuf->CurrentRec->text+8, "%d", &NewReadonlyState)) && ! NewReadonlyState)
            s_CurrentBuf->EditLock ^= ReadOnly;
          break; }
        
        else if (strstr(Qualifier, "writeifchanged") == Qualifier) { //Insists that changes are written out.
          int NewWriteIfChangedState;
            s_CurrentBuf->EditLock |= WriteIfChanged;
          if ((0 < sscanf(s_TempBuf->CurrentRec->text+14, "%d", &NewWriteIfChangedState)) && ! NewWriteIfChangedState)
            s_CurrentBuf->EditLock ^= WriteIfChanged;
          break; }
          
        else if (strstr(Qualifier, "sameflag1") == Qualifier) { //Sets the user change flag.
          s_CurrentBuf->UnchangedStatus |= SameFlag1;
          break; }
           
#if defined(VC)
        else if (strstr(Qualifier, "codepage") == Qualifier) { //Define CodePage for this buffer.
          sscanf(s_TempBuf->CurrentRec->text+8, "%d", &s_CurrentBuf->CodePage);
          break; }
#endif

        else if (strstr(Qualifier, "header") == Qualifier) { //Define a header string for display above window.
          char *Header;
          if (s_TempBuf->CurrentRec->text[6]) {
            Header = (char *)JotMalloc(strlen(s_TempBuf->CurrentRec->text+7)+1);
            strcpy(Header, s_TempBuf->CurrentRec->text+7); }
          else
            Header = NULL;
          if (s_CurrentBuf->Header) 
            free(s_CurrentBuf->Header);
          s_CurrentBuf->Header = Header;
          LocalFail = InitTermTable();
          JotUpdateWindow();
          break; }

        else if (strstr(Qualifier, "footer") == Qualifier) { //Define a string to replace pathname in window-separator line.
          char *Footer = (char *)JotMalloc(strlen(s_TempBuf->CurrentRec->text+7)+7);
          strcpy(Footer, s_TempBuf->CurrentRec->text+7);
          if (s_CurrentBuf->Footer) 
            free(s_CurrentBuf->Footer);
          s_CurrentBuf->Footer = Footer;
          JotUpdateWindow();
          break; }

        else if (strstr(Qualifier, "leftoffset") == Qualifier) { //Define LeftOffset for this buffer (leftmost character in view).
          int OrigLeftOffset = s_CurrentBuf->LeftOffset;
          int NewLeftOffset;
          if (sscanf(s_TempBuf->CurrentRec->text+10, "%d", &NewLeftOffset) <= 0) {
            s_CurrentBuf->LeftOffset = 0;
            LocalFail = 1;
            break; }
          if (1000000000 < NewLeftOffset) {
            LocalFail = Fail("Yikes!!! left offset setting too large (%d)", NewLeftOffset);
            break; }
          s_CurrentBuf->LeftOffset = (NewLeftOffset < 0) ? 0 : NewLeftOffset;
          if (OrigLeftOffset == s_CurrentBuf->LeftOffset) 
            break;
          LocalFail |= InitTermTable();
          JotUpdateWindow();
          break; }

        else if (strstr(Qualifier, "tabstops") == Qualifier || strstr(Qualifier, "tabcells") == Qualifier) { //Define TabStops - cell or column widths for this buffer.
          struct Chain { int TabStop; struct Chain *next; };
          struct Chain *FirstLink, *ThisLink, *PrevLink = NULL;
          int Chrs, Index = 1, TabStop, LastTabStop = 0, *TabStops;
          char *Args = s_TempBuf->CurrentRec->text+8;
          
          s_CurrentBuf->TabCells = strstr(Qualifier, "tabcells") != 0;
          if (s_CurrentBuf->TabStops) {
            free(s_CurrentBuf->TabStops);
            s_CurrentBuf->TabStops = NULL; }
          s_TempBuf->CurrentByte += 8; //Length of 'TabStops'.
          
          for ( ; ; ) {
            if(sscanf(Args, " %d%n", &TabStop, &Chrs) <= 0) {
              if ( ! PrevLink) //No tabstop setting supplied, buffer reverts to non-tabular behaviour.
                break;
              ThisLink->next = NULL;
              break; }
            if (PrevLink)
              ThisLink = PrevLink->next = (struct Chain *)JotMalloc(sizeof(struct Chain));
            else
              ThisLink = FirstLink = (struct Chain *)JotMalloc(sizeof(struct Chain));
            Args += Chrs;
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

        else if (strstr(Qualifier, "tagtype") == Qualifier) { //tagtype - define a user tag, currently only colour tags supported.
          char TagName[StringMaxChr];
          int Foreground = 0, Background = 7;
          struct ColourObj *NewColourObj;
          sscanf(s_TempBuf->CurrentRec->text+8, "%s colour %d %d", &TagName[0], &Foreground, &Background);
#if defined(VC)
          if (15 < Foreground || 15 < Background) {
            LocalFail = Fail("Invalid colour number for windows (must be in the range 0 to 15)");
            break; }
#else
          if (7 < Foreground || 7 < Background) {
            LocalFail = Fail("Invalid colour number for unix (must be in the range 0 to 7)");
            break; }
          if (COLOR_PAIRS-1 < s_NextColourPair) {
            LocalFail = Fail("System colour-pair limit has been reached");
            break; }
#endif
          if (12 < strlen(TagName)) {
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
          NewColourObj->ColourPair = s_NextColourPair;
          init_pair(s_NextColourPair++, Foreground, Background);
#endif
          break; }

        else if (strstr(Qualifier, "addtag") == Qualifier) { //addtag - applies a tag defined by %b=tagtype to the current text.
          char Name[StringMaxChr];
          struct ColourObj *ThisColourObj = s_CurrentBuf->FirstColObj;
          char *TextString = NULL;
          struct AnyTag *ThisTag;
          
          sscanf(s_TempBuf->CurrentRec->text+7, "%s", &Name[0]);
          if (s_CurrentBuf->SubstringLength == 0) {
            LocalFail = 1;
            break; }
          ThisTag = (struct AnyTag *)JotMalloc(sizeof(struct AnyTag));
          if (strstr(Name, "-text=") == Name) { //It's the %b=addtag -string=<string> form
            char *Text = strstr(s_TempBuf->CurrentRec->text+7, "-text=")+6;
            TextString = (char *)JotMalloc(strlen(Text)+1);
            strcpy(TextString, Text);
            ThisTag->Attr = TextString; }
          else { //A colour tag.
            s_CurrentBuf->CurrentRec->DisplayFlag = Redraw_Line;
            while (ThisColourObj) { //Tagname search loop.
              if ((strstr(Name, ThisColourObj->TagName) == Name) && (strlen(Name) == strlen(ThisColourObj->TagName)) ) {
                ThisTag->Attr = ThisColourObj;
                break; }
              else
                ThisColourObj = ThisColourObj->next; } }
          if ( ! TextString && ! ThisColourObj) {
            free(ThisTag);
            LocalFail = Fail("Colour tag \"%s\" does not exist in buffer %c", Name, s_CurrentBuf->BufferKey);
            break; }
          else if (s_CurrentBuf->SubstringLength < 0) {
            ThisTag->StartPoint = s_CurrentBuf->CurrentByte+s_CurrentBuf->SubstringLength;
            ThisTag->EndPoint = s_CurrentBuf->CurrentByte; }
          else {
            ThisTag->StartPoint = s_CurrentBuf->CurrentByte;
            ThisTag->EndPoint = (0<s_CurrentBuf->SubstringLength) ? s_CurrentBuf->CurrentByte+s_CurrentBuf->SubstringLength-1 : strlen(s_CurrentBuf->CurrentRec->text); }
          ThisTag->next = NULL;
          ThisTag->type = TextString ? TagType_Text : TagType_Colour;
          AddTag(s_CurrentBuf->CurrentRec, ThisTag);
          break; }

        else if (strstr(Qualifier, "remove_tag") == Qualifier) { //remove_tag - identifies a tag and removes it from the record structure.
          int TagType;
          char *TagName;
          int TagStartPoint, TagEndPoint;
          struct Rec *CurrentRec = s_CurrentBuf->CurrentRec;
          struct AnyTag *ThisTag, *StartTag = NULL;
          
          if (strstr(s_TempBuf->CurrentRec->text+10, " colour ")) {
            TagType = TagType_Colour;
            TagName = s_TempBuf->CurrentRec->text+18; }
          else if (strstr(s_TempBuf->CurrentRec->text+10, " text ")) {
            TagType = TagType_Text;
            TagName = s_TempBuf->CurrentRec->text+16; }
          else if (strstr(s_TempBuf->CurrentRec->text+10, " target ")) {
            TagType = TagType_Target;
            TagName = s_TempBuf->CurrentRec->text+18; }
          else {
            LocalFail = Fail("Syntax error in remove_tag command");
            break; }
            
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
                struct JumpObj *HasJumpObj = ThisTag->Attr;
                if ( ! strcmp(HasJumpObj->HashKey, TagName) && ThisTag->StartPoint == TagStartPoint) {
                  StartTag = ThisTag;
                  break; } } }
            ThisTag = ThisTag->next; }
            
          if ( ! StartTag) {
            LocalFail = Fail("Can't locate the tag");
            break; }
          FreeTag(CurrentRec, StartTag);
          ForceLineRefresh(); }

        else if (strstr(Qualifier, "sort") == Qualifier) { //sort - perfom a qsort on the records in this buffer.
          if (ParseSort(RightArg, 0))
            LocalFail = 1;
          break; }
        
        else if (strstr(Qualifier, "tabsort") == Qualifier) { //tabsort - perfom a qsort on the records in this buffer.
          if (ParseSort(RightArg, 1))
            LocalFail = 1;
          break; }
             
        else if (strstr(Qualifier, "unicode") == Qualifier) { //Controls unicode support for this buffer.
          int Line, UnicodeMode = TRUE;
          if (sscanf(s_TempBuf->CurrentRec->text+8, "%d", &UnicodeMode) < 0)
            s_CurrentBuf->NoUnicode = FALSE;
          else
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
           
        else if (strstr(Qualifier, "pathname") == Qualifier) { //Sets the pathName for when the buffer is written.
          int Line;
           
          if (s_CurrentBuf->PathName) 
            free(s_CurrentBuf->PathName);
          if (FirstChr)
            Modifier = s_TempBuf->CurrentRec->text+Chrs-1;
          else {
            LocalFail = Fail("Invalid pathname syntax");
            break; }
          s_CurrentBuf->PathName = strcpy((char *)JotMalloc(strlen(Modifier)+1), Modifier);
          if (s_TermTable) {
            for (Line = 0; Line <= s_TermHeight; Line +=1)
                s_TermTable[Line] = NULL; }
          JotUpdateWindow();
          break; }
          
        else
          LocalFail = RunError("Unrecognized %%B qualifier \"%s\"", Qualifier);
           
        break; }

      case 'H': { //%H - Hashtable maintenance.
        int Operation, Qual = 0;
        struct Com *CurrentCommand = s_CurrentCommand;
        if (ThisArg) {
          Operation = FetchIntArg((struct IntArg **)&ThisArg);
          if (ThisArg)
            Qual = FetchIntArg((struct IntArg **)&ThisArg); }
            
        LocalFail = DoHash(RightArg, BufferKey, Operation, Qual, TextAsIs);
        s_CurrentCommand = CurrentCommand;
        break; }

      case 'Q': { //%Q - Query.
        int Query1 = 0, Query2 = 0;
        if (ThisArg) {
          Query1 = FetchIntArg((struct IntArg **)&ThisArg);
          if (ThisArg)
            Query2 = FetchIntArg((struct IntArg **)&ThisArg); }
        LocalFail = QuerySomething(RightArg, BufferKey, Query1, Query2, TextAsIs);
        if (BufferKey != '\0') { //With no destination buffer, dir and file just return status, version prints version to console, others options fail.
          GetRecord(s_CurrentBuf, 1);
          ResetBuffer(s_CurrentBuf); }
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
    if (NextCommand == NULL) { //Last command in sequence.
      if ( ! LocalFail)
        return 0;
      else if (ElseCommands == NULL)
        return LocalFail; }
    else { //At least one more command to follow.
      s_CommandChr = NextCommand->CommandKey;
      ThisArg = NextCommand->ArgList;
      if (s_CommandChr == '?')       //? - Unset failure flag
        LocalFail = 0;
      else if (s_CommandChr == '\\') //\ - Reverse failure flag.
        LocalFail = ! LocalFail; }
    
    if (LocalFail && (s_TraceMode & Trace_Failures) )
      JotTrace("Command failure.");
      
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
  CrossmatchPathnames(NULL, &File, "w", s_CurrentBuf->PathName, "./", NULL, TempString);
  if (File == NULL) { // Denied write access.
    RunError("Error opening file \"%s\" for writing.", TempString);
    return; }
  else { // File opend OK. 
    struct Rec *Record = s_CurrentBuf->FirstRec;
    struct Rec *LastRec = s_CurrentBuf->FirstRec->prev;
    
    if (s_Verbose & Verbose_NonSilent)
      Message(NULL, "Writing to \"%s\"", TempString);
    for ( ; ; ) { // Write out records loop. 
      if (Record == LastRec) {
        if(0 < strlen(Record->text))
          fprintf(File, "%s", Record->text);
        break; }
      fprintf(File, "%s\n", Record->text);
      Record = Record->next; }
    
    if (fclose(File) == 0) { // File wriitten OK.
      s_CurrentBuf->UnchangedStatus |= SameSinceIO;
      if (TidyUp(0, ExitMessage))
        return; }
    Fail("Error Closing your file"); }
  return;  }

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
    if (DestBuf) {
      GetRecord(DestBuf, strlen(Line)+1);
      strcpy(DestBuf->CurrentRec->text, Line); }
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
  struct Rec *NextRecord = Record->next;
  int FirstRecord = (TextBuf->FirstRec) == Record;
  struct AnyTag *ThisTag = Record->TagChain;
  
  if ( ! TextBuf->CurrentRec)
    return 1;
  if (TextBuf->EditLock & ReadOnly) {
    Fail("Can't delete records from read-only buffer %c", TextBuf->BufferKey);
    return 1; }
    
  while (ThisTag) {
    struct AnyTag *NextTag = ThisTag->next;
    
    if (ThisTag->type == TagType_Target) {
      struct JumpObj *ThisJumpObj = (struct JumpObj *)(ThisTag->Attr);
      struct Buf *RemoteBuf = ThisJumpObj->HashBuf;
      
      if (ThisJumpObj->TargetRec) {
        if (RemoteBuf) {
          if (RemoteBuf->HashtableMode == DestroyHashtables) {
            DestroyHtab(RemoteBuf);
            //N.B. DestroyHtab() also takes out tags pointing to the hashtable - including the one we're currently holding.
            NextTag = Record->TagChain; }
          else if (RemoteBuf->HashtableMode == DeleteEntries) {
            FreeTag(Record, ((struct AnyTag *)ThisJumpObj->Target));
            ThisJumpObj->Target = NULL;
            ThisJumpObj->TargetRec = NULL; }
          else if (RemoteBuf->HashtableMode == AdjustEntries) {
            if (AdjustMode == DeleteNotAdjust)
              ThisJumpObj->TargetRec = NULL;
            else { //Adjust tag by making a copy in the adjacent record.
              struct AnyTag *NewTag = (struct AnyTag *)JotMalloc(sizeof(struct AnyTag));
              ThisJumpObj->TargetRec = (AdjustMode == AdjustForwards) ?
                ((ThisJumpObj->TargetRec->next == TextBuf->FirstRec) ? ThisJumpObj->TargetRec->prev : ThisJumpObj->TargetRec->next) :
                ((ThisJumpObj->TargetRec == TextBuf->FirstRec) ? ThisJumpObj->TargetRec->next : ThisJumpObj->TargetRec->prev);
              NewTag->next = NULL;
              NewTag->StartPoint = 0;
              ThisJumpObj->Target = (struct TargetTag *)NewTag;
              NewTag->type = TagType_Target;
              NewTag->Attr = (void *)ThisJumpObj;
              AddTag(ThisJumpObj->TargetRec, NewTag); } }
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
    //The buffer's been cloned to a DataObject - the FreeBuffer operation is unnecessary and silently abandonded.
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
  
  if (TextBuf == s_NoteBuffer)
    s_NoteBuffer = NULL;
  TextBuf->UnchangedStatus = 0;
  
  while (ThisColourObj) {
    struct ColourObj *NextColourObj = ThisColourObj->next;
    free(ThisColourObj);
    ThisColourObj = NextColourObj; }
        
  if (TextBuf->PathName)
    free(TextBuf->PathName);
  if (TextBuf->IOBlock) {
    if (TextBuf->IOBlock->HoldDesc)
      close(TextBuf->IOBlock->HoldDesc);
    if (TextBuf->IOBlock->BucketBlock)
      free(TextBuf->IOBlock->BucketBlock);
    if (TextBuf->IOBlock->OutPipe) { //The buffer has asynchronous I/O set up.
      struct Buf **ThisAsyncBufPtr = &s_FirstAsyncBuf;
      while (*ThisAsyncBufPtr) {
        if (*ThisAsyncBufPtr == TextBuf) {
          *ThisAsyncBufPtr = TextBuf->IOBlock->NextAsyncBuf;
          break; }
        ThisAsyncBufPtr = &(*ThisAsyncBufPtr)->IOBlock->NextAsyncBuf; }
#if defined(VC)
      if (TextBuf->IOBlock->ExitCommand) { //An exit command has been defined.
        int Bytes;
        WriteFile(TextBuf->IOBlock->InPipe, TextBuf->IOBlock->ExitCommand, strlen(TextBuf->IOBlock->ExitCommand), &Bytes, NULL);
        WriteFile(TextBuf->IOBlock->InPipe, "\n", 1, &Bytes, NULL); }
      else {
        if (TerminateProcess(TextBuf->IOBlock->ProcessId, 0))
          Fail("Failed to terminate the child process"); }
      CloseHandle(TextBuf->IOBlock->OutPipe);
      CloseHandle(TextBuf->IOBlock->InPipe);
#else
      if (TextBuf->IOBlock->ExitCommand) { //An exit command has been defined.
        write(TextBuf->IOBlock->InPipe, TextBuf->IOBlock->ExitCommand, strlen(TextBuf->IOBlock->ExitCommand));
        write(TextBuf->IOBlock->InPipe, "\n", 1); }
      else {
        kill(TextBuf->IOBlock->ChildPid, SIGTERM);
        waitpid(TextBuf->IOBlock->ChildPid, NULL, 0); }
      close(TextBuf->IOBlock->OutPipe);
      close(TextBuf->IOBlock->InPipe);
#endif
    }
    if (TextBuf->IOBlock->ExitCommand)
      free(TextBuf->IOBlock->ExitCommand);
    free(TextBuf->IOBlock); }
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

//--------------------------------------------------CheckChildBuffers
int CheckChildBuffers(struct Buf *ThisBuf)
  { //Sniffs out all the sub buffers dangling off the current buffers hashtable, returns number of changed writeifchanged buffers.
  struct AnyObj *ThisObj = (struct AnyObj *)ThisBuf->FirstObj;
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
    if (ThisObj->type == ObjType_Data) { //A DataObj
      struct DataObj *ThisDataObj = (struct DataObj *)ThisObj;
      if (ThisDataObj->Frame && ((struct DataObj *)ThisObj)->Frame->type == FrameType_Buf) //It's a buffer - check it.
        Count += CheckChildBuffers(((struct bufFrame *)ThisDataObj->Frame)->BufPtr); }
    ThisObj = ((struct JumpObj *)ThisObj)->next; }
        
  return Count; }

//--------------------------------------------------CheckCircularHier
int CheckCircularHier(struct Buf *ThisBuf)
  { //Sniffs out all the sub buffers dangling off the specified buffer's hashtable, returns 1 if the buffer exists in the subtree, otherwise returns 0.
  struct AnyObj *ThisObj = (struct AnyObj *)ThisBuf->FirstObj;
  int Count = 0;
  
  if (ThisBuf == s_CurrentBuf) {
    char ParentPath[StringMaxChr];
    GetBufferPath(ThisBuf, ParentPath, StringMaxChr);
    Fail("The current buffer belongs to the tree offered for removal \"%s\"", ParentPath);
    Count++; }
  
  while (ThisObj) {
    if (ThisObj->type == ObjType_Data) { //A DataObj
      struct DataObj *ThisDataObj = (struct DataObj *)ThisObj;
      if (ThisDataObj->Frame && ((struct DataObj *)ThisObj)->Frame->type == FrameType_Buf) //It's a buffer - check it.
        Count += CheckChildBuffers(((struct bufFrame *)ThisDataObj->Frame)->BufPtr); }
    ThisObj = ((struct JumpObj *)ThisObj)->next; }
        
  return Count; }

//--------------------------------------------------DuplicateRecord
int DuplicateRecord(struct Buf *OutBuf, struct Buf *InBuf)
  { //Gets a record of the right size then copies in text.
  char *InRecord = InBuf->CurrentRec->text;
  
  if (GetRecord(OutBuf, strlen(InRecord)+1))
    return 1;
  strcpy(OutBuf->CurrentRec->text, InRecord);
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
  ThisBuf->NewPathName = TRUE;
  ThisBuf->NoUnicode = FALSE;
  ThisBuf->UnchangedStatus = 0;
  ThisBuf->EditLock = 0;
  ThisBuf->AutoTabStops = FALSE; 
  ThisBuf->FirstColObj = NULL;
  ThisBuf->htab = NULL;
  ThisBuf->FirstObj = NULL;
  ThisBuf->FileType = 0;
  ThisBuf->HashtableMode = NoHashtable;
  if (ThisBuf->IOBlock) {
    struct BucketBlock *ThisBucketBlock = ThisBuf->IOBlock->BucketBlock;
    while (ThisBucketBlock) {
      struct BucketBlock *NextBucketBlock = ThisBucketBlock->Next;
      free(ThisBucketBlock);
      ThisBucketBlock = NextBucketBlock; }
    if (ThisBuf->IOBlock->OutPipe)
#if defined(VC)
      CloseHandle(ThisBuf->IOBlock->OutPipe);
#else
      close(ThisBuf->IOBlock->OutPipe);
#endif
    ThisBuf->IOBlock->OutPipe = 0; }
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
    ToRec->length = strlen(FromRec->text)+1;
    ToRec->text = (char *)JotMalloc(ToRec->length);
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
int GetRecord(struct Buf *TextBuf, int Size)
  { //Extracts new record from heap to follow original current record,
    //  then sets it to current and initializes it.
    //If null Buffer then create first record.
  struct Rec *Record = TextBuf->CurrentRec;
  struct Rec *NewRecord;
  char *NewText;
  
  if (Size < 0) {
    RunError("Negative record size in GetRecord()");
    return 1; }
  NewRecord = (struct Rec *)JotMalloc(sizeof(struct Rec));
  NewText = (char *)JotMalloc(Size+1);
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
  *(NewRecord->text) = '\0';
  NewRecord->DisplayFlag = Redraw_Line;
  NewRecord->length = Size;
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
      GetRecord(DestBuf, SourceBytes+1);
      strcpy(DestBuf->CurrentRec->text, SourceBuf->CurrentRec->text); 
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
      struct JumpObj *ThisJumpObj = (struct JumpObj *)(ThisTag->Attr);
      struct Buf *RemoteBuf = ThisJumpObj->HashBuf;
      
      if (RemoteBuf->HashtableMode == DestroyHashtables) {
        DestroyHtab(RemoteBuf);
        //N.B. DestroyHtab() also takes out tags pointing to the hashtable - including the one we're currently holding.
        NextTag = FromRecord->TagChain; }
      else if (RemoteBuf->HashtableMode == DeleteEntries)
        ThisJumpObj->TargetRec = NULL;
      else if (RemoteBuf->HashtableMode == AdjustEntries) { //Normally push the target to the next record
        FreeTarget(FromBuf, ThisJumpObj);
        if (ThisJumpObj->TargetRec) {
          struct AnyTag *NewTag = (struct AnyTag *)JotMalloc(sizeof(struct AnyTag));
          struct Rec *NewTargetRec = (ThisJumpObj->TargetRec->next == FromBuf->FirstRec) ? ThisJumpObj->TargetRec->prev : ThisJumpObj->TargetRec->next;
          ThisJumpObj->type = ObjType_Jump;
          NewTag->next = NULL;
          NewTag->StartPoint = 0;
          NewTag->type = TagType_Target;
          NewTag->Attr = (void *)ThisJumpObj;
          ThisJumpObj->TargetRec = NewTargetRec;
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
  
  GetRecord(TextBuf, BreakPointer+1);
  FirstRec = TextBuf->CurrentRec;
  if (GetRecord(TextBuf, (strlen(BreakRec->text))-BreakPointer+1))
    return 1;
  SecondRec = TextBuf->CurrentRec;
    
  strncpy(FirstRec ->text, BreakRec->text, BreakPointer);
  strcpy(SecondRec->text, (BreakRec->text)+BreakPointer);
  
  //Now split any tags between the two halves adjusting target points as appropriate.
  if (*ThisTagPtr) {
    if ((*ThisTagPtr)->StartPoint < BreakPointer)
      FirstRec->TagChain = BreakRec->TagChain;
    while ((*ThisTagPtr) && (*ThisTagPtr)->StartPoint < BreakPointer) {
      if ((*ThisTagPtr)->type == TagType_Target)
        ((struct JumpObj *)(*ThisTagPtr)->Attr)->TargetRec = FirstRec;
      else if (BreakPointer < (*ThisTagPtr)->EndPoint)
        (*ThisTagPtr)->EndPoint = BreakPointer;
      FirstRecLastTag = *ThisTagPtr;
      (*ThisTagPtr) = (*ThisTagPtr)->next; }
    
    SecondRec->TagChain = *ThisTagPtr;
    while ((*ThisTagPtr)) {
      (*ThisTagPtr)->StartPoint -= BreakPointer;
      if ((*ThisTagPtr)->type == TagType_Target)
        ((struct JumpObj *)(*ThisTagPtr)->Attr)->TargetRec = SecondRec;
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
  if (GetRecord(TextBuf, FirstLength+strlen(SecondText)+1))
    return 1;
  NewRecord = TextBuf->CurrentRec;
  NewText = NewRecord->text;
  
  strcpy(NewText, FirstText);
  strcat(NewText, SecondText);
  
  //Concatinate tags, adjusting tags, target points and, in 2nd. record, the character no.
  NewRecord->TagChain = *FirstTagsPtr ? *FirstTagsPtr : SecondTags;
  while (*FirstTagsPtr) {
    if ((*FirstTagsPtr)->type == TagType_Target)
      ((struct JumpObj *)(*FirstTagsPtr)->Attr)->TargetRec = NewRecord;
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
      ((struct JumpObj *)SecondTags->Attr)->TargetRec = NewRecord;
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

#if !defined(NoRegEx)
//----------------------------------------------RegexSearchBuffer
int RegexSearchBuffer(struct Buf *TextBuf, char BufferKey, char *SearchString, int Direction)
  { // Regular-expression Search of buffer, limited to a number of records.
  regex_t re;
  regmatch_t pmatch[100];
  int TempCount = 0;  //Indicates no. of records searched.
  int frame = 0;      //Used to allocate each regexec call a new frame in pmatch in backwards searches.
  size_t frameSize;   //Used in management of and extraction from frames in pmatch array.
  int IgnoreCase = s_CaseSensitivity ? 0 : REG_ICASE;
  int StartPoint = TextBuf->CurrentByte;
  struct Buf *DestBuf = NULL;
  int RegExFailed = FALSE, i;
  
  if (TextBuf->SubstringLength) {
    StartPoint += Direction;
    TextBuf->SubstringLength = 0; }
    
  if (BufferKey) {
    DestBuf = GetBuffer(BufferKey, AlwaysNew);
    if (DestBuf == NULL)
      return FALSE; }
  
  if (TextBuf->SubstringLength) 
    StartPoint += Direction;
  
  if (regcomp(&re, SearchString, IgnoreCase|REG_EXTENDED) != 0) {
    Fail("Invalid regular expression %s", SearchString);
    regfree(&re);
    if (DestBuf) {
      if ( ! DestBuf->CurrentRec )
        GetRecord(DestBuf, 1);
      DestBuf->CurrentRec->text[0] = '\0'; }
    return FALSE; }
  else if (DestBuf)
    GetRecord(DestBuf, 100);
     
  frameSize = re.re_nsub+1;
  
  if (0 < Direction) { //Forward search.
    for (;;) { //Record loop, if not found move record pointer.
      if (StartPoint < strlen(TextBuf->CurrentRec->text)) {
        if ( ! regexec(&re, (TextBuf->CurrentRec->text)+StartPoint, 100, pmatch, 0) )
          break; }
      StartPoint = 0;
      if (AdvanceRecord(TextBuf, Direction)) {
        regfree(&re);
        if (DestBuf)
          GetRecord(DestBuf, 0);
        return FALSE;
        TempCount++; } }
    TextBuf->CurrentByte = StartPoint+pmatch[frame].rm_so;
    TextBuf->SubstringLength = pmatch[frame].rm_eo - pmatch[frame].rm_so; }
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
      RegExFailed |= AdvanceRecord(TextBuf, Direction);
      frame = 0; 
      endStop = strlen(TextBuf->CurrentRec->text);
      if (RegExFailed) {
        regfree(&re);
        if (DestBuf)
          GetRecord(DestBuf, 0);
        return FALSE; }
      TempCount--; }
      
reverseRegexMatchFound: //Last regexec call failed but the previous match was successful - rewind pmatch and StartPoint.
    TextBuf->CurrentByte = StartPoint;
    frame -= frameSize; 
    StartPoint -= pmatch[frame].rm_so; }
  
  for (i = 0; i < frameSize; i++) {
    regoff_t start = pmatch[frame+i].rm_so;
    regoff_t end = pmatch[frame+i].rm_eo;
    TextBuf->SubstringLength = end-start;
    if (DestBuf) {
      GetRecord(DestBuf, end-start);
      strncat(DestBuf->CurrentRec->text, (s_CurrentBuf->CurrentRec->text)+StartPoint+start, end-start); } }
      
  if (DestBuf) {
    ResetBuffer(DestBuf);
    DestBuf->AbstractWholeRecordFlag = TRUE; }
  regfree(&re);
  return TRUE; }
#endif

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
  int SubLength = DestBuf->SubstringLength;  //The original substring length.
  int DestTotalLength;                       //The overall size of the destination string.
  int RemainderLength;                       //The length of the destination substring following the current substring.
  int SubStart;                              //The character no. in the dest record to receive the first chr from the source string.
  int FinalLength;                           //The size of the destination string after substitution.
  int SourceLength;                          //The size of the string to be pushed in.
  int Offset;                                //The change in length of the destination record.
  struct AnyTag *ThisTag = DestRecord->TagChain;
  int DestIndex;
  
  if (DestText == SourceString) {
    RunError("Can't substitute a string with itself");
    return 1; }
  if (0 <= SubLength)
    SubStart = DestBuf->CurrentByte;
  else {
    SubLength = -SubLength; 
    SubStart = DestBuf->CurrentByte - SubLength; }
  DestIndex = SubStart;
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
    DestBuf->CurrentByte = SubStart;
    DestIndex = SubStart; }
  
  DestTotalLength = strlen(DestText);
  RemainderLength = DestTotalLength-SubStart-SubLength;
  FinalLength = SubStart+SourceLength+RemainderLength; 
  
  if (0 < Offset) { //The destination string is growing.
    if ((DestRecord->length) < FinalLength+1) { //Insufficient space for insertion, get a bigger record and copy text.
      char *NewText = (char *)JotMalloc(FinalLength+1);
      
      if (NewText == NULL) {
        RunError("SubstituteString:run out of heap space.");
        return 1; }
      strncpy(NewText, DestText, SubStart);
      strncpy(NewText+SubStart, SourceString, SourceLength);
      strncpy(NewText+SubStart+SourceLength, DestText+SubStart+SubLength, DestTotalLength-SubStart-SubLength);
      NewText[SubStart+SourceLength+RemainderLength] = '\0';
      free(DestText);
      DestBuf->CurrentRec->text = NewText;
      DestRecord->length = FinalLength+1; }
    else { //String is growing and record is already big enough.
      memmove(DestText+SubStart+Offset, DestText+SubStart, DestTotalLength-SubStart);
      memcpy(DestText+SubStart, SourceString, SourceLength);
      DestText[DestTotalLength+Offset] = '\0'; }
    DestBuf->SubstringLength = SourceLength;
    DestBuf->CurrentByte = SubStart; }
      
  else { //String shrinks or remains the same length.
    if (Offset < 0) { //String is shrinking - shift left.
      int i = SubStart+SourceLength;
      do DestText[i] = DestText[i-Offset];
        while (DestText[i++]); }
    if (SourceString != NULL) //Insert the string now.
      strncpy(DestText+DestIndex, SourceString, SourceLength);
    DestBuf->SubstringLength = SourceLength;
    DestBuf->CurrentByte = SubStart; }
    
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

//----------------------------------------------ShiftRight
int ShiftRight(struct Buf *TextBuf, int Displacement)
  { //Moves character pointer by Displacement characters, text may contain UTF-8 or ISO-8859 unicode and Displacement may be negative to count backwards.
  int ChrCount = 0;                             //Running total of no. of characters checked.
  int ByteCount = 0;                            //Running total of bytecount.
  int Bytes;                                    //Byte count of this character.
  int FirstByte = TextBuf->CurrentByte;          //Points to start-point byte.
  char *TheString = TextBuf->CurrentRec->text;  //The string to be shifted.
  int TotalBytes = strlen(TheString);           //Total bytes in String.
#if defined(VC)
  int CodePage;
#endif
  int Fail = FALSE;
  
  TextBuf->SubstringLength = 0; 
   
  if (TextBuf->NoUnicode) {
    TextBuf->CurrentByte += Displacement;
    if ( (0 < Displacement) && (TotalBytes <= FirstByte+Displacement) ) { // Overflow - Set to end of string.
      TextBuf->CurrentByte = TotalBytes;
      Fail = TRUE; }
    else if ( (Displacement < 0) && (FirstByte+Displacement < 0) ) { //Underflow - Set to first byte in record.
      TextBuf->CurrentByte = 0;
      Fail = TRUE; }
       
    return Fail; }
     
  else {   
#if defined(VC)
    CodePage = TextBuf->CodePage;
    if (0 <= Displacement) {
      for ( ; (ChrCount < Displacement) && (++ByteCount <= TotalBytes-FirstByte); ) {
        ChrCount = MultiByteToWideChar(CodePage, MB_ERR_INVALID_CHARS, TheString+FirstByte, ByteCount, NULL, 0);
        if (TotalBytes <= FirstByte+ByteCount) { //Reached last byte of substring.
          if (ChrCount != Displacement) //Ooops - something failed.
            ChrCount = 0;
          break; } } }
    else {
      for (ByteCount = -1; 0 <= FirstByte+ByteCount; ByteCount--) { //Reached First byte of substring.
        ChrCount -= (TheString[FirstByte+ByteCount] < 0 && !MultiByteToWideChar(CodePage, MB_ERR_INVALID_CHARS, TheString+FirstByte+ByteCount, -ByteCount, NULL, 0)) ? 0 : 1;
        if (ChrCount <= Displacement)
          break; } }
#else
    if (0 <= Displacement)
      while ( (ChrCount < Displacement) && (FirstByte+ByteCount <= TotalBytes) ) {
        if ((Bytes = mblen(TheString+FirstByte+ByteCount, MB_CUR_MAX)) < 0 )
          Bytes = 1;
        else if (Bytes == 0)
          break;
        ChrCount++;
        ByteCount += Bytes; }
    else { //Reading backwards, it's not easy to tell a mid/end UTF-8 byte from an ISO-8859 character.
           //So we work back, sniffing each byte as a possible UTF-8 start byte, an ASCII chr. or the start of the string.
      for (ByteCount = 0; ( (Displacement < ChrCount) && (0 < FirstByte+ByteCount) ); ) {
        if (0 < (Bytes = mblen(TheString+FirstByte+ByteCount-1, 1-ByteCount))) {
          ByteCount -= Bytes;
          ChrCount--; }
        else {
          int BackByte;
          for (BackByte = 1; BackByte <= MB_CUR_MAX; BackByte++) {
            if ( (FirstByte+ByteCount-BackByte-1 < 0) || (0 < (signed char)TheString[FirstByte+ByteCount-BackByte-1]) ) { //Start of the line or an ASCII chr - original character was ISO-8859.
              ByteCount--;
              ChrCount--;
              break; }
            else if (BackByte == MB_CUR_MAX) { //Encountered a string of more than MB_CUR_MAX ISO-8859 characters - count only the first one.
              ByteCount--;
              ChrCount--; }
            else if (0 < mblen(TheString+FirstByte+ByteCount-BackByte-1, -BackByte)) {  //It was part of a UTF-8 sequence.
              ByteCount -= BackByte+1;
              ChrCount--;
              break; } } } } }
          
#endif
      if (ChrCount != Displacement) {
        TextBuf->CurrentByte = (0 < Displacement) ? TotalBytes : 0;
        Fail = TRUE; }
      else
        TextBuf->CurrentByte += ByteCount; }
  return Fail; }

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
    if (Type == IntegerArg) {
      long long Value = (long long)(((struct IntArg *)ThisArg)->IntArg);
      *LengthPtr += sprintf(Message+*LengthPtr, "<%lld>", Value);
      if ((' ' <= Value) && (Value <= '~'))
        *LengthPtr += sprintf(Message+*LengthPtr, "=\'%c\'", (int)(Value&0x000000FF)); }
    else if (Type == FloatingPointArg)
      *LengthPtr += sprintf(Message+*LengthPtr, "=\'%lf\'", (double)(((struct FloatArg *)ThisArg)->FloatArg));
    else if (Type == StringArg)
      *LengthPtr += sprintf(Message+*LengthPtr, "=\'%s\'", (char *)(((struct PtrArg *)ThisArg)->pointer));
    else if (Type == DeferredStringArg) {
      int BufferKey = (char)(((struct IntArg *)ThisArg)->IntArg);
      struct Buf * Buffer = GetBuffer(BufferKey, NeverNew);
      *LengthPtr += sprintf(Message+*LengthPtr, "\'%C = \"%s\"", BufferKey, Buffer->CurrentRec->text); }
    else if (Type == BlockArg) //Make no attempt to descend into blocks - just indicate their presence.
      *LengthPtr += sprintf(Message+*LengthPtr, "...");
    else
      Fail("Unknown Arg type found in DumpCommand() - type = %d", Type);
    ThisArg = ThisArg->next;
    if (ThisArg != NULL)
      *LengthPtr += sprintf(Message+*LengthPtr, " "); }
      
  *LengthPtr += sprintf(Message+*LengthPtr, "} "); }

//----------------------------------------------FreeCommand
void FreeCommand(struct Com **ThisCommand)
  { // Frees one command block.
  struct AnyArg *ThisArg = (*ThisCommand)->ArgList;
  
  if ((*ThisCommand)->CommandKey == '(')
    FreeBlock((struct Block **)&(((struct PtrArg *)(*ThisCommand)->ArgList)->pointer));
    
  while (ThisArg) { // Arg loop.
    struct AnyArg *Next = ThisArg->next;
    if (ThisArg->type == StringArg && ((struct PtrArg *)ThisArg)->pointer)
      free(((struct PtrArg *)ThisArg)->pointer);
    free(ThisArg);
    ThisArg = Next; }
  
  free(*ThisCommand); 
  *ThisCommand = NULL; }

//----------------------------------------------FreeSequence
void FreeSequence(struct Seq **ThisSequence)
  { // Frees one Sequence.
  struct Com *ThisCom;
  
  if (*ThisSequence == NULL)
    return;
  ThisCom = (*ThisSequence)->FirstCommand;
  while (ThisCom != NULL) { // Com loop.
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
  if (*ThisBlock == NULL)
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
   
  NewArg->type = IntegerArg;
  NewArg->IntArg = ArgValue;
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

//---------------------------------------------AddFloatArg
void AddFloatArg(struct Com *CommandBlock, double ArgValue)
  { //Adds floating-point argument to current command.
  struct FloatArg *NewArg = (struct FloatArg *)JotMalloc(sizeof(struct FloatArg));
  struct AnyArg *ThisArg = CommandBlock->ArgList;
   
  NewArg->type = FloatingPointArg;
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
   
  NewArg->type = BlockArg;
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
  struct PtrArg *NewArg;
  struct AnyArg *ThisArg = CommandBlock->ArgList;
  int Type = StringArg;
  char *FinalArgValue;
  int Length;
  char BufferKey;
  
  NewArg = (struct PtrArg *)JotMalloc(sizeof(struct PtrArg));
  if (VerifyCharacter(CommandBuf, '\'')) { // Deferred argument just save the buffer key.
    BufferKey = ChrUpper(NextNonBlank(CommandBuf));
    if ( ! CheckBufferKey(BufferKey))
      SynError(CommandBuf, "Invalid buffer key");
    Type = DeferredStringArg; }
  else { // Extract string argument now.
    DelimiterCharacter = ImmediateCharacter(CommandBuf);
    if (DelimiterCharacter == '\0')
      ResultString[0] = '\0';
    if ( (('A' <= DelimiterCharacter) && (DelimiterCharacter <= 'Z')) || (('a' <= DelimiterCharacter) && (DelimiterCharacter <= 'z')) || 
        (('0' <= DelimiterCharacter) && (DelimiterCharacter <= '9')) ) {
      SynError(CommandBuf, "Invalid delimiter");
      free(NewArg);
      return; }
  
    for (InputIndex = CommandBuf->CurrentByte; InputIndex < strlen(InputText); InputIndex++) { // Copy-over-text loop.
      char Chr = InputText[InputIndex];
      if (StringMaxChr <= OutputIndex) {
        SynError(CommandBuf, "Argument string too long");
        free(NewArg);
        return; }
      if (Chr == DelimiterCharacter)
        break;
      if (Chr == '\\') {
        if (InputText[++InputIndex] == 't') //Translate tab.
          Chr = '\t';
        else if (InputText[InputIndex] == 'r') //Translate carriage return.
          Chr = '\r';
        else if (InputText[InputIndex] == 'n') //Translate new line.
          Chr = '\n';
        else
          InputIndex--; }
      ResultString[OutputIndex++] = Chr; }
    
    ResultString[OutputIndex] = '\0';
    CommandBuf->CurrentByte = InputIndex+1;
    
    Length = strlen(ResultString);
    
    if ( (Length == 0) || (ResultString == NULL) )
      FinalArgValue = NULL;
    else { // Non-null string.
      FinalArgValue = (char *)JotMalloc(Length+1);
      strcpy(FinalArgValue, ResultString); } }
    
  NewArg->type = Type;
  if (Type == DeferredStringArg)
    ((struct IntArg *)NewArg)->IntArg = BufferKey;
  else 
    NewArg->pointer = (void *)FinalArgValue;
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

//---------------------------------------------AddPercentArg
int AddPercentArg(struct Com *CommandBlock, struct Buf *CommandBuf)
  { //Copies percent-command argument to next compiled arg struct, respecting escaped semicolons.
    //In parsing backslashes, there are special cases for strings begining "'". ".\" and "..\'" which are 
    //dealt with first, it is then safe to assume at least two preceeding characters before any "'" or ";"
  struct PtrArg *NewArg = (struct PtrArg *)JotMalloc(sizeof(struct PtrArg));
  struct AnyArg *ThisArg = CommandBlock->ArgList;
  char *String = (CommandBuf->CurrentRec->text)+(CommandBuf->CurrentByte), *SubString = String, *RealEnd = NULL, *BackSlash = NULL;
  char *OutputString = (char *)JotMalloc(strlen(String)+2);
  int TextAsIs = FALSE;
  
  OutputString[0] = '\0';
  TextAsIs = (RealEnd = strstr(String, " -hereEndsThePercentCommand;")) != 0;
  if (RealEnd) { //A "-hereEndsThePercentCommand" - let all of the string through without trying to interpret quotes or semicolons and without stripping backslashes.
    CommandBuf->CurrentByte += RealEnd-String+28;
    strncpy(OutputString, String, RealEnd-String);
    OutputString[RealEnd-String] = '\0'; }
  else { //Deal with backslashes.
  
    //First determine the end of the percent-command string.
    while ( (RealEnd = strchr(SubString, ';')) ) { //Search for end of percent command.
      if ((RealEnd-1)[0] != '\\') //It's not an escaped semicolon.
        break;
      SubString = RealEnd+1; }
    if ( ! RealEnd)
      RealEnd = String+strlen(String);
      
    //Now process any backslashes in the command string.
    SubString = String;
    while ( (BackSlash = memchr(SubString, '\\', RealEnd-SubString)) ) { //There's a single backslash - remove it and then copy any more that follow on immediately after.
      while ( (++BackSlash)[0] == '\\')
        { }
      strncat(OutputString, SubString, BackSlash-SubString-1);
      CommandBuf->CurrentByte += BackSlash-SubString;
      if ( ! BackSlash[0] )
        break;
      SubString = BackSlash; }
      
    CommandBuf->CurrentByte += RealEnd-SubString+1;
    if (BackSlash)
      strcat(OutputString, SubString+(RealEnd-SubString));
    else {
      int OrigLen = strlen(OutputString);
      strncat(OutputString, SubString, RealEnd-SubString);
      OutputString[OrigLen+RealEnd-SubString+1] = '\0'; } }
      
  NewArg->type = StringArg;
  NewArg->pointer = (void *)OutputString;
  NewArg->next = NULL;
    
  if (ThisArg == NULL)
    CommandBlock->ArgList = (struct AnyArg *)NewArg;
  else
    while (TRUE)
      if ((ThisArg->next) == NULL) { // Found last Arg.
        ThisArg->next = (struct AnyArg *)NewArg;
        return TextAsIs; }
      else
        ThisArg = ThisArg->next;
        
  return TextAsIs; }

//---------------------------------------------FetchIntArg
long long FetchIntArg(struct IntArg **CurrentArgs_a)
  { //Extracts integer arg value and indexes to next.
  long long Value;
   
  if (*CurrentArgs_a == NULL) {
    RunError("Insufficient args in list.");
    return 0; }
  if ((*CurrentArgs_a)->type != IntegerArg)
    RunError("Expecting an integer argument, found some other type.");
  Value = (*CurrentArgs_a)->IntArg;
  *CurrentArgs_a = (struct IntArg *)(*CurrentArgs_a)->next;
  return Value;  }

//---------------------------------------------FetchFloatArg
double FetchFloatArg(struct FloatArg **CurrentArgs_a)
  { //Extracts floating-point arg value and indexes to next.
  double Value;
   
  if (*CurrentArgs_a == NULL) {
    RunError("Insufficient args in list.");
    return 0; }
  if ((*CurrentArgs_a)->type != FloatingPointArg)
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
  if ((*CurrentArgs_a)->type != BlockArg) {
    RunError("Expecting a block argument, found some other type.");
    return NULL; }
  Value = (struct Block *)(*CurrentArgs_a)->pointer;
  *CurrentArgs_a = (struct PtrArg *)(*CurrentArgs_a)->next;
  return Value;  }

//-----------------------------------------------CompareRecs
int  CompareRecs(struct Rec *X, struct Rec *Y, int ColumnStarts[], int ColumnWidths[])
  { //Compares two records - used by quicksort
    //ColumnStarts is a list of start characters of columns to be compared in order, terminated by -1.
    //ColumnWidths is the corresponding list of column widths.
    int i;
    
    for (i=0; ; ) {
      int ColumnStart=ColumnStarts[i], ColumnWidth=ColumnWidths[i++];
      if (ColumnStart < 1)
        return strcmp(X->text, Y->text);
      else {
        int Result = memcmp(X->text+ColumnStart, Y->text+ColumnStart, ColumnWidth+1);
        if (Result != 0)
          return Result; } } }

//-----------------------------------------------CompareRecsTab
int CompareRecsTab(struct Rec *X, struct Rec *Y, int TabNos[])
  { //Compares two records - used by quicksort, TabNos is a list of tab-delimited columns in priority order and terminated by -1
    int i, Result;
    char TempX[StringMaxChr], TempY[StringMaxChr];
    
    for (i = 0; ; ) {
      int TabNo = TabNos[i++];
      if (TabNo < 1)
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
          return Result; } } }

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
  Quicksort(v, left, last - 1, comp, ColumnStarts, ColumnWidths);
  Quicksort(v, last + 1, right, comp, ColumnStarts, ColumnWidths); }

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

//-----------------------------------------------ParseSort
int ParseSort(char *Text, int TabSort)
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
  int i, Col=0, Chrs;
  struct Rec *PrevRec;
  int Relinks;
  
  if (s_CurrentBuf->EditLock & ReadOnly) {
    RunError("Attempt to modify a readonly buffer");
    return 1; }
  //Extract any tabstops. 
  if (TabSort) { 
    for (i = 7; i < strlen(Text); ) {
      int TabNo;
      if (sscanf(Text+i, "%d%n", &TabNo, &Chrs) <= 0)
        return 1;
      ColumnStarts[Col++] = TabNo;
      i += Chrs+1; }
    ColumnStarts[Col++] = -1; }
  else {
    for (i = 4; i < strlen(Text); ) {
      int Start, Width;
      char Delim;
      if (sscanf(Text+i, "%d%c%d%n", &Start, &Delim, &Width, &Chrs) <= 0)
        return 1;
      if (Delim == '-')
        Width -= Start;
      ColumnStarts[Col] = Start;
      ColumnWidths[Col++] = Width;
      i += Chrs+1; }
    ColumnStarts[Col++] = -1; }
  
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
  char TempString[StringMaxChr];
  int Len;
   
  TempString[0] = '\0';
  va_start(ap, String);
  if ( (Len = vsnprintf(TempString, StringMaxChr-1, String, ap))==StringMaxChr-1 )
    TempString[StringMaxChr-1] = '\0';
  va_end(ap); 
  if (AddToJournal && s_JournalPath) {
    if (s_RecoveryMode) { //Replace given text with a line read from the recovery file.
      if ( ! fgets(TempString, StringMaxChr, s_asConsole->FileHandle))
        return 0;
      Len = strlen(TempString);
      if (TempString[Len-1] != '\0')
        TempString[Len-1] = '\0'; }
    else { //Using given text.
      if (s_JournalHand)
        UpdateJournal(TempString, NULL); } }
  GetRecord(Buffer, Len+1);
  strcpy(Buffer->CurrentRec->text, TempString);
  return strlen(TempString); }

//----------------------------------------------QuerySomething
int QuerySomething(char *QueryString, char BufferKey, int QueryType, int SubQuery, int TextAsIs)
  { //Reports on directory, file or other system tables - returns 0 if no errors.
  struct Buf *DestBuf = NULL, *SourceBuf = s_CurrentBuf;
  int SourceBufKey = SourceBuf->BufferKey;
#if defined(VC)
  HANDLE DirHand = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATA FileData;
  FILETIME ftCreate, ftAccess, ftWrite;
  SYSTEMTIME stUTC, stLocal;
  DWORD dwRet;
  int Chrs, EndStop;
  struct __stat64 Stat;
  WORD AttrWords[StringMaxChr];
#else
    struct stat Stat;
#endif
  
  if (QueryType == Q_dir || QueryType == Q_file || QueryType == Q_key)
    ExpandDeferredString(s_TempBuf, QueryString, TextAsIs);

  if (BufferKey != '\0') {
    DestBuf = GetBuffer(BufferKey, AlwaysNew);
    if (DestBuf) {
      if (QueryType != Q_dir && QueryType != Q_file)
        AddFormattedRecord(FALSE, DestBuf, QueryString);
      s_CurrentBuf = DestBuf; }
    else
      return 1; }
   
  switch (QueryType) {
  case Q_version: { //version - print version or send it to a buffer.
    if ( ! DestBuf) {
#if defined(VC)
      Message(NULL, VERSION_STRING);
#else
      Message(NULL, "%s%s", VERSION_STRING, curses_version());
#endif
      return 0; }
    GetRecord(DestBuf, strlen(VERSION_STRING)+1);
    strcpy(DestBuf->CurrentRec->text, VERSION_STRING);
#if ! defined(VC)
    DestBuf->CurrentByte = strlen(VERSION_STRING);
    SubstituteString(DestBuf, curses_version(), -1);
#endif
    break; }

  case Q_windows: //windows - fails if not operating under windows.
#if defined(VC)
    return 0;
#else
    return 1;
#endif

  case Q_linux: //linux fails if not operating under linux.
#if defined(VC)
    return 1;
#else
    return 0;
#endif

  case Q_case: //case - fails if not case sensitive.
    return ! s_CaseSensitivity;

  case Q_system: { //System settings etc.
    struct Buf *ThisBuf = s_BufferChain;
    int DataObjs = 0, NoteChr, i;
    struct Rec * Record = SourceBuf->CurrentRec;
    
    //s_NoteByte indocates a byte offset - to get the character offset go back to that record.
    if (SourceBuf->LineNumber < s_NoteLine)
      for (i = SourceBuf->LineNumber; i < s_NoteLine; i++)
        Record = Record->next;
    else if (s_NoteLine < SourceBuf->LineNumber)
      for (i = SourceBuf->LineNumber; s_NoteLine < i; i--)
        Record = Record->prev;
    NoteChr = JotStrlenBytes(Record->text, s_NoteByte);
    
    if ( ! DestBuf)
      return 1;
    AddFormattedRecord(FALSE, DestBuf, "            Trace vector = %4X", s_TraceMode);
    AddFormattedRecord(FALSE, DestBuf, "    Default trace vector = %4X", s_DefaultTraceMode);
    AddFormattedRecord(FALSE, DestBuf, "               Verbosity = %4d", s_Verbose);
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
    AddFormattedRecord(FALSE, DestBuf, "             Find string = \"%s\"", s_SearchString);
    AddFormattedRecord(FALSE, DestBuf, "            RegEx string = \"%s\"", s_RegExString);
    AddFormattedRecord(FALSE, DestBuf, "           Insert string = \"%s\"", s_InsertString);
    AddFormattedRecord(FALSE, DestBuf, "          Qualify string = \"%s\"", s_QualifyString);
    AddFormattedRecord(FALSE, DestBuf, "           Prompt string = \"%s\"", s_PromptString);
    if (s_OnKey)
      AddFormattedRecord(FALSE, DestBuf, "                  On key = \"%s\"", s_OnKey->CurrentRec->text);
    else
      AddFormattedRecord(FALSE, DestBuf, "                  On key = Not defined");
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
        if ( ! DataObjs++)
          AddFormattedRecord(FALSE, DestBuf, "Data-object buffers:");
        AddFormattedRecord(FALSE, DestBuf, "  data obj %20s: %4d \"%s\"", Path, ThisBuf->LineNumber, TruncatedRec); }
      ThisBuf = ThisBuf->NextBuf; }
    break; }

  case Q_hashtables: { //hashtables - find buffers with hashtables, list internal and external references and target buffers
    struct Buf *ThisBuf = s_BufferChain;
    char Buffers[512];
    if ( ! DestBuf)
      return 1;
    
    while (ThisBuf != NULL) {
      Buffers[0] = '\0';
      if (ThisBuf->FirstObj) {
        struct JumpObj *ThisJumpObj = (struct JumpObj *)ThisBuf->FirstObj;
        while (ThisJumpObj) {
          char Key = ThisJumpObj->TargetBuf->BufferKey;
          if ( ! strchr(Buffers, Key)) {
            strcat(Buffers, "  ");
            Buffers[strlen(Buffers)-1] = Key; }
          ThisJumpObj = (struct JumpObj *)ThisJumpObj->next; } }
        if (strlen(Buffers))
          AddFormattedRecord(FALSE, DestBuf, "  Buffer %c has hash-table entries pointing to these buffers:%s", ThisBuf->BufferKey, Buffers);
        ThisBuf = ThisBuf->NextBuf; }
    break; }

  case Q_wd: { //wd - return current directory only.
    char *trans;
     
    if ( ! DestBuf)
      return 1;
    trans = getenv("PWD");
    if (trans == 0) {
      char temp[100];
      GetRecord(DestBuf, 20);
      strcpy(temp, " - <Undefined> - ");
      strcpy(DestBuf->CurrentRec->text, temp);
      return 1; }
    GetRecord(DestBuf, strlen(trans)+1);
    strcpy(DestBuf->CurrentRec->text, trans);
  break; }

  case Q_heap: { //heap - report heap stats to stdout.
#if defined(LINUX)
    if ( ! DestBuf)
      return 1;
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
    return Backtrace(DestBuf);

  case Q_history: { //history - dumps history to specified buffer.
    struct Rec *CommandRec = s_EditorConsole->CommandBuf->FirstRec->next;
    if ( ! DestBuf)
      return 1;
    while (CommandRec != s_EditorConsole->CommandBuf->FirstRec) {
      AddFormattedRecord(FALSE, DestBuf, "%s", CommandRec->text);
      CommandRec = CommandRec->next; }
      
    DestBuf->CurrentRec = DestBuf->FirstRec->prev;
    break; }

  case Q_tabstops: //tabstops - pushes TRUE if tabstops are set otherwise FALSE.
    return  ! SourceBuf->TabStops;
  case Q_tabcells: //tabcells - pushes TRUE if tabcells are set otherwise FALSE.
    return  ! SourceBuf->TabCells;

  case Q_keys:  //keys - Sends a list of hashtable items and values to specified buffer.
  case Q_key: { //key -  As above but reports only one item, specified by keystring.
    struct AnyObj *ThisObj;
    char *Name = s_TempBuf->CurrentRec->text+4;
    
    if ( ! DestBuf)
      return (SourceBuf->htab == NULL);
    if (QueryType == Q_key) { //Pick up key string from command line.
      ENTRY *Ptr;
      ENTRY Entry;
   
      if ( ! SourceBuf->htab) {
        Fail("No hash-table associated with buffer %c", SourceBuf->BufferKey);
        return 1; }
      Entry.key = Name;
      Entry.data = NULL;
      
      if ( ! hsearch_r(Entry, FIND, &Ptr, SourceBuf->htab)) {
        Fail("Failed to find item \"%s\" in hashtable of buffer %c", Name, SourceBuf->BufferKey);
        return 1; }
      ThisObj = Ptr->data; }
    else if ( ! SourceBuf->htab) {
      AddFormattedRecord(FALSE, DestBuf, "Buffer %c has no hashtable.", SourceBuf->BufferKey);
      return 0; }
    else {
      if (SourceBuf->BufferKey == '~') { //Also print the pathname for this floating buffer.
        char ParentPath[StringMaxChr];
        GetBufferPath(SourceBuf, ParentPath, StringMaxChr);
        AddFormattedRecord(FALSE, DestBuf, "Keys for buffer %c (Pathname %s):", SourceBuf->BufferKey, ParentPath); }
      else
        AddFormattedRecord(FALSE, DestBuf, "Keys for buffer %c :", SourceBuf->BufferKey);
      ThisObj = (struct AnyObj *)SourceBuf->FirstObj; }
    
    while (ThisObj) {
      if (ThisObj->type == ObjType_Zombie) { //A deleted entry.
        struct AnyObj * NewObj = ((struct ZombieObj *)ThisObj)->NewObj;
        if (NewObj) {
          AddFormattedRecord(FALSE, DestBuf, "key %20s replaced by following item:", ThisObj->HashKey);
          ThisObj = NewObj;
          continue; }
        else
          AddFormattedRecord(FALSE, DestBuf, "key %20s deleted", ThisObj->HashKey); }
      else if (ThisObj->type == ObjType_Data) { //A DataObj
        struct DataObj *ThisDataObj = (struct DataObj *)ThisObj;
        if (ThisDataObj->Frame) {
          int Type = ((struct DataObj *)ThisObj)->Frame->type;
          if (Type == FrameType_Int)
            AddFormattedRecord(FALSE, DestBuf, "key %20s, (DataObj) Int %lld", ThisDataObj->HashKey, ThisDataObj->Frame->Value);
          else if (Type == FrameType_Float)
            AddFormattedRecord(FALSE, DestBuf, "key %20s, (DataObj) Float %f", ThisDataObj->HashKey, ((struct floatFrame *)ThisDataObj->Frame)->fValue);
          else if (Type == FrameType_Buf)
            AddFormattedRecord(FALSE, DestBuf, "key %20s, (DataObj) Buffer \"%s\"", 
              ThisDataObj->HashKey, ((struct bufFrame *)ThisDataObj->Frame)->BufPtr->CurrentRec->text); }
        else //An undefined-value DataObj.
          AddFormattedRecord(FALSE, DestBuf, "key %20s, (DataObj) Undefined type and value", ThisDataObj->HashKey); }
      else if (ThisObj->type == ObjType_Setsect) { //A SetsectObj
        struct SetsectObj *ThisSetsectObj = (struct SetsectObj *)ThisObj;
        AddFormattedRecord(FALSE, DestBuf, "key %20s, (SetsectObj) Seek %lld, Bytes %lld", ThisSetsectObj->HashKey, ThisSetsectObj->Seek, ThisSetsectObj->Bytes); }
      else if (ThisObj->type == ObjType_Setfilesect) { //A SetfsectObj
        struct SetfsectObj *ThisSetfsectObj = (struct SetfsectObj *)ThisObj;
        AddFormattedRecord(FALSE, DestBuf, "key %20s, (SetfsectObj) Section Pathname %s, Seek %lld, Bytes %lld", 
          ThisSetfsectObj->HashKey, ThisSetfsectObj->PathName ? ThisSetfsectObj->PathName : "<Invalid pathname>", ThisSetfsectObj->Seek, ThisSetfsectObj->Bytes); }
      else { //It's a bog-standard JumpObj
        char TruncText[50];
        memset(TruncText, '\0', 50);
        if (((struct JumpObj *)ThisObj)->TargetRec) {
          strncpy(TruncText, ((struct JumpObj *)ThisObj)->TargetRec->text, 30);
          TruncText[30] = '\0';
          if (((struct JumpObj *)ThisObj)->Target)
            AddFormattedRecord(FALSE, 
              DestBuf, 
              "key %20s, (JumpObj) buf %c, line no. %4d, chr no. %d, Rec:%s", 
              ThisObj->HashKey, ((struct JumpObj *)ThisObj)->TargetBuf->BufferKey, 
              ((struct JumpObj *)ThisObj)->LineNumber,
              JotStrlenBytes(((struct JumpObj *)ThisObj)->TargetRec->text, ((struct JumpObj *)ThisObj)->Target->StartPoint),
              TruncText);
          else
            AddFormattedRecord(FALSE, 
              DestBuf, 
              "key %20s, (JumpObj) buf %c, line no. %4d, No target character, Rec:%s", 
              ThisObj->HashKey, ((struct JumpObj *)ThisObj)->TargetBuf->BufferKey, ((struct JumpObj *)ThisObj)->LineNumber, TruncText); }
        else
          strcpy(TruncText, "deleted key"); }
      ThisObj = (QueryType == Q_key) ? NULL : ((struct JumpObj *)ThisObj)->next; }
    break; }

  case Q_tags: { //tags - sends a list of record tags in current buffer to nominated buffer.
    int LineNo = 1, ErrorRecs = 0, ErrorTags = 0;
    int Here = FALSE;
    struct Rec *ThisRec;
    struct Rec *LastRec;
    char TruncText[51];
    
    if ( ! DestBuf || SourceBufKey == BufferKey)
      return 1;
      
    if (SubQuery == Q_here) { //Report only tags active at current character position.
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
            struct JumpObj *Object = ThisTag->Attr;
            int ErrorTag = (Object->TargetRec != ThisRec);
            AddFormattedRecord(FALSE, DestBuf, "  Type target %c, Chr %2d, %s", 
              Object->HashBuf->BufferKey, ChrFirst, ErrorTag ? "  **Error JumpObj-record mismatch **" : Object->HashKey);
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
    int HTabCount = 0;
    struct AnyObj *ThisAnyObj;
    struct ColourObj *TagType;
    
    if ( ! DestBuf)
      return 1;
    if (SourceBufKey == BufferKey)
      return 1;
      
    TagType = SourceBuf->FirstColObj;
    PathName = SourceBuf->PathName;
    ThisAnyObj = (struct AnyObj *)SourceBuf->FirstObj;
    while (ThisAnyObj) {
      HTabCount++;
      ThisAnyObj = (struct AnyObj *)ThisAnyObj->next; }
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
    AddFormattedRecord(FALSE, DestBuf, "    CurrentRec length = %d", SourceBuf->CurrentRec->length);
    AddFormattedRecord(FALSE, DestBuf, "               Header = %s", SourceBuf->Header);
    AddFormattedRecord(FALSE, DestBuf, "               Footer = %s", SourceBuf->Footer);
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
      AddFormattedRecord(FALSE, DestBuf, "               InPipe = %d",  SourceBuf->IOBlock->InPipe);
      AddFormattedRecord(FALSE, DestBuf, "              OutPipe = %d",  SourceBuf->IOBlock->OutPipe);
      AddFormattedRecord(FALSE, DestBuf, "             HoldDesc = %d",  SourceBuf->IOBlock->HoldDesc);
#if defined(VC)
      AddFormattedRecord(FALSE, DestBuf, "            ProcessId = %d",  SourceBuf->IOBlock->ProcessId);
#else
      AddFormattedRecord(FALSE, DestBuf, "             ChildPid = %d",  SourceBuf->IOBlock->ChildPid);
#endif
      if (SourceBuf->IOBlock->ExitCommand)
        AddFormattedRecord(FALSE, DestBuf, "        ExitCommand = %s",  SourceBuf->IOBlock->ExitCommand);
      if ( ! SourceBuf->IOBlock->BucketBlock)
        AddFormattedRecord(FALSE, DestBuf, "  Buffer ( %c ) has finished reading", SourceBuf->BufferKey);
      else {
        AddFormattedRecord(FALSE, DestBuf, "  Buffer ( %c ) has pending IO operations",  SourceBuf->BufferKey);
        AddFormattedRecord(FALSE, DestBuf, "  %s for reading",  SourceBuf->IOBlock->ReadyForReading ? "Ready" : "Not ready");
        if (SourceBuf->IOBlock->BucketBlock->Bucket) {
          char TruncatedRec[101];
          AddFormattedRecord(FALSE, DestBuf, "Bucket start chr %d", SourceBuf->IOBlock->BucketBlock->RecStartChr);
          AddFormattedRecord(FALSE, DestBuf, "  Bucket end chr %d", SourceBuf->IOBlock->BucketBlock->RecEndChr);
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
     
    if ( ! DestBuf)
      return 1;
    AddFormattedRecord(FALSE, DestBuf, " screenHeight = %d", s_TermHeight);
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
      AddFormattedRecord(FALSE, DestBuf, "  Console area %d lines (%-2d to %d), extendable up to %d lines", s_TermHeight-s_ConsoleTop, s_ConsoleTop, s_TermHeight, s_MaxConsoleLines);
    else
      AddFormattedRecord(FALSE, DestBuf, "  Console area %d lines (%-2d to %d)", s_TermHeight-s_ConsoleTop, s_ConsoleTop, s_TermHeight);
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
    time_t Time = time(NULL);
    PushInt(Time);
    break; }

  case Q_date: { //date - return todays date only.
    time_t DateTime;
    char temp[100];
     
    if ( ! DestBuf)
      return 1;
    DateTime = time(NULL);
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
        if ( (SourceBuf->LeftOffset < PerceivedChr) && (PerceivedChr <= SourceBuf->LeftOffset+VisibleLimit+1) ) {
          PushInt(SourceBuf->LeftOffset);
          PushInt(VisibleLimit);
          PushInt(PerceivedChr);
          return 0; } }
        Win = Win->next; }
    if ( VisibleLimit == -1 ) {
      RunError("Buffer ( %c ) is not displayed in any window - %q=inview failed", SourceBuf->BufferKey);
      return 1; }
      
    PushInt(SourceBuf->LeftOffset);
    PushInt(VisibleLimit);
    PushInt(PerceivedChr);
    return 1; }
       
  case Q_commandmode: //Query in command mode
    return s_CommandMode;
  
  case Q_samesinceio: //Query SameSinceIO in this buffer.
    return ((SourceBuf->UnchangedStatus) & SameSinceIO) == 0;
   
  case Q_samesinceindexed: //Query SameSinceIndexed in this buffer.
    return ((SourceBuf->UnchangedStatus) & SameSinceIndexed) == 0;
   
  case Q_samesincecompiled: //Query SameSinceCompiled in this buffer.
    return ((SourceBuf->UnchangedStatus) & SameSinceCompiled) == 0;
   
  case Q_sameflag1: //Query or set SameFlag1 in this buffer.
    return  ((SourceBuf->UnchangedStatus) & SameFlag1) == 0;
   
  case Q_pid: //pid - Process ID of current process.
    AddFormattedRecord(FALSE, DestBuf, "%d", getpid());
    return 0;
  
  case Q_env: { //env - return translation of env variable.
    char *trans;
    char Name[StringMaxChr];
     
    if ( ! DestBuf)
      return 1;
    sscanf(QueryString+4, "%s", (char *)&Name);
    trans = getenv(Name);
    AddFormattedRecord(TRUE, DestBuf, "%s", trans ? trans : " - <Undefined> - ");
    break; }

  case Q_dir: { //dir <path> - return directory contents.
#if defined(VC)
    char *Qualifier;
#else
    DIR *PathElemDIR;
    struct dirent *Entry = NULL;
#endif
    char temp[StringMaxChr], Path[StringMaxChr];
    int EndQual = 4;
    int PathLen;
    char *ExpandedQuery;
    
    if ( ! DestBuf)
      return 1;
    ExpandedQuery = s_TempBuf->CurrentRec->text;
    AddFormattedRecord(FALSE, DestBuf, ExpandedQuery);
    if (s_RecoveryMode) { //Pick up records directly from recovery scrit.
      while ( AddFormattedRecord(TRUE, DestBuf, "") )
        { }
      FreeRecord(DestBuf, DeleteNotAdjust);
      s_CurrentBuf->CurrentRec = s_CurrentBuf->FirstRec->prev;
      return 0; }
       
    if ( ! ExpandedQuery[4])
      return 1;
    if ( ! DestBuf)
      return 0;
      
    EndQual = 4;
    while (0 < strlen(ExpandedQuery+EndQual)) {
      if(ExpandedQuery[EndQual] == ' ')
        EndQual++;
      else if (ExpandedQuery[EndQual] == '-') {
        DestBuf->FirstRec->text[EndQual-1] = '\t';
        if (strstr(ExpandedQuery+EndQual, "-msecs") == ExpandedQuery+EndQual)
          EndQual +=6;
        else if (strstr(ExpandedQuery+EndQual, "-mtime") == ExpandedQuery+EndQual)
          EndQual +=6;
        else if (strstr(ExpandedQuery, "-atime") == ExpandedQuery+EndQual)  
          EndQual +=6;
        else if (strstr(ExpandedQuery, "-ctime") == ExpandedQuery+EndQual)  
          EndQual +=6;
        else if (strstr(ExpandedQuery, "-uid") == ExpandedQuery+EndQual)  
          EndQual +=4;
        else if (strstr(ExpandedQuery, "-gid") == ExpandedQuery+EndQual)  
          EndQual +=4;
        else if (strstr(ExpandedQuery, "-size") == ExpandedQuery+EndQual)  
          EndQual +=5;
        else if (strstr(ExpandedQuery, "-mode") == ExpandedQuery+EndQual)  
          EndQual +=5;
        else if (strstr(ExpandedQuery, "-inode") == ExpandedQuery+EndQual)  
          EndQual +=6; }
      else
        break; }
      
    strcpy(Path, ExpandedQuery+EndQual);
    ExpandEnv(Path, StringMaxChr);
    PathLen = strlen(Path);
#ifdef VC
    if (Path[PathLen-1] == '/' || Path[PathLen-1] == '\\')
      Path[(PathLen--)-1] = '\0';
    if (__stat64(Path, &Stat)) {
      Fail("Nonexistent directory \"%s\"", Path); 
      return 1; }
    if ( ! DestBuf) 
      return 0;
    while (Qualifier = strstr(DestBuf->FirstRec->text, " -"))
      Qualifier[1] = '\t';
    if ((Stat.st_mode & S_IFMT) != S_IFDIR) {
      Fail("Not a directory \"%s\"", Path); 
      return 1; }
    if ( ! DestBuf)
      return 0;
    strcpy(temp, Path);
    strcat(temp, "\\*");
    DirHand = FindFirstFile(temp, &FileData);
    if (DirHand == INVALID_HANDLE_VALUE)
      return 1;
      
    while ( 1 ) {
      strcpy(temp, Path);
      strcat(temp, "/");
      strcpy(temp, FileData.cFileName);
      __stat64(temp, &Stat);
      if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        strcat(temp, "/");
#else
    if (stat(Path, &Stat)) {
      Fail("Nonexistent directory \"%s\"", Path); 
      return 1; }
    if ((Stat.st_mode & S_IFMT) != S_IFDIR) {
      Fail("Not a directory \"%s\"", Path); 
      return 1; }
    PathElemDIR = opendir(Path);
    if (PathElemDIR)
      Entry = readdir(PathElemDIR);
      
    while (Entry) {
      strcpy(temp, Path);
      strcat(temp, "/");
      strcat(temp, Entry->d_name);
      stat(temp, &Stat);
      if ((Stat.st_mode & S_IFMT) == S_IFDIR) 
        strcat(temp, "/");
#endif
        
    EndQual = 4;
    while (0 < strlen(ExpandedQuery+EndQual)) {
      char FmtResult[100];
      if(ExpandedQuery[EndQual] == ' ')
        EndQual++;
      else if (strstr(ExpandedQuery+EndQual, "-mtime") == ExpandedQuery+EndQual) {
        EndQual +=6;
        strftime(FmtResult, 100, "\t%Y/%m/%d-%H:%M:%S", localtime(&Stat.st_mtime));
        strcat(temp, FmtResult); }
      else if (strstr(ExpandedQuery, "-atime") == ExpandedQuery+EndQual) {  
        EndQual +=6;
        strftime(FmtResult, 100, "\t%Y/%m/%d-%H:%M:%S", localtime(&Stat.st_atime));
        strcat(temp, FmtResult); }
      else if (strstr(ExpandedQuery, "-ctime") == ExpandedQuery+EndQual) {  
        EndQual +=6;
        strftime(FmtResult, 100, "\t%Y/%m/%d-%H:%M:%S", localtime(&Stat.st_ctime));
        strcat(temp, FmtResult); }
      else if (strstr(ExpandedQuery, "-uid") == ExpandedQuery+EndQual) {  
        EndQual +=4;
        sprintf(FmtResult, "\t%4d", Stat.st_uid); 
        strcat(temp, FmtResult); }
      else if (strstr(ExpandedQuery, "-gid") == ExpandedQuery+EndQual) {  
        EndQual +=4;
        sprintf(FmtResult, "\t%4d", Stat.st_gid); 
        strcat(temp, FmtResult); }
      else if (strstr(ExpandedQuery, "-size") == ExpandedQuery+EndQual) {  
        EndQual +=5;
        sprintf(FmtResult, "\t%10d", (int)Stat.st_size); 
        strcat(temp, FmtResult); }
      else if (strstr(ExpandedQuery, "-mode") == ExpandedQuery+EndQual) {  
        EndQual +=5;
        sprintf(FmtResult, "\t%6o", Stat.st_mode); 
        strcat(temp, FmtResult); }
      else if (strstr(ExpandedQuery, "-inode") == ExpandedQuery+EndQual) {  
        EndQual +=6;
        sprintf(FmtResult, "\t%9d", (int)Stat.st_ino); 
        strcat(temp, FmtResult); }
      else
        break; }
#if defined(VC)
  AddFormattedRecord(TRUE, DestBuf, "%s", temp);
      if (FindNextFile(DirHand, &FileData) == 0)
        break; }
        
    FindClose(DirHand);
#else
      AddFormattedRecord(TRUE, DestBuf, "%s", temp+PathLen+1);
      Entry = readdir(PathElemDIR); }
    closedir(PathElemDIR);
#endif
  if (s_JournalHand) //Add the empty-line endstop marker.
    UpdateJournal("", NULL);
  break; }

  case Q_file: { //file <pathName> - a selection of info on the selected file.
    char PathName[StringMaxChr];
    char *ExpandedQuery;
    
    ExpandedQuery = s_TempBuf->CurrentRec->text;
    if (DestBuf)
      AddFormattedRecord(FALSE, DestBuf, ExpandedQuery);
    
    if (SourceBufKey == BufferKey)
      return 1;
    if (ExpandedQuery[4] != '\0')
      strcpy(PathName, ExpandedQuery+5);
    else if (SourceBuf->PathName)
      strcpy(PathName, SourceBuf->PathName);
    else {
      Fail("No pathname given");
      return 1; }
    ExpandEnv(PathName, StringMaxChr);
#if defined(VC)
    if (__stat64(PathName, &Stat))
#else
    if (stat(PathName, &Stat))
#endif
      { //Error - the pathName does not point to a valid file.
      if (DestBuf) {
        char temp[StringMaxChr+20];
        sprintf(temp, "Nonexistent file \"%s\"", PathName); 
        GetRecord(DestBuf, strlen(temp)+1);
        strcpy(DestBuf->CurrentRec->text, temp); }
      return 1; }
    else { //PathName points to a valid file.
      char temp[StringMaxChr];
      time_t Time;
      if ( ! DestBuf)
        return 0;
      AddFormattedRecord(TRUE, DestBuf, "                 Name = \"%s\"", PathName);
#if defined(VC)
      AddFormattedRecord(TRUE, DestBuf, "                Drive = %c:", Stat.st_dev+'A');
#else
      AddFormattedRecord(TRUE, DestBuf, "                inode = %d", Stat.st_ino);
#endif
      AddFormattedRecord(TRUE, DestBuf, "                 Mode = %6o", Stat.st_mode);
      AddFormattedRecord(TRUE, DestBuf, "                  uid = %d", Stat.st_uid);
      AddFormattedRecord(TRUE, DestBuf, "                  gid = %d", Stat.st_gid);
#if defined(VC)
      AddFormattedRecord(TRUE, DestBuf, "                 size = %lld", Stat.st_size);
#else
      AddFormattedRecord(TRUE, DestBuf, "                 size = %jd", Stat.st_size);
#endif
      AddFormattedRecord(TRUE, DestBuf, " writable by this UID = %s", (euidaccess(PathName, W_OK) == 0) ? "yes" : "no");
      AddFormattedRecord(TRUE, DestBuf, "            directory = %d", (Stat.st_mode & S_IFMT) == S_IFDIR);
      Time = Stat.st_atime;
      strftime(temp, 100, "%Y/%m/%d, %H:%M:%S", localtime(&Time));
      AddFormattedRecord(TRUE, DestBuf, "          Access time = %s (%ld)", temp, Time);
      Time = Stat.st_mtime;
      strftime(temp, 100, "%Y/%m/%d, %H:%M:%S", localtime(&Time));
      AddFormattedRecord(TRUE, DestBuf, "          Modify time = %s (%ld)", temp, Time);
      Time = Stat.st_ctime;
      strftime(temp, 100, "%Y/%m/%d, %H:%M:%S", localtime(&Time));
      AddFormattedRecord(TRUE, DestBuf, "        Creation time = %s (%ld)", temp, Time); }
    break; }

  case Q_stack: //stack - dumps stack to selected buffer.
    DumpStack(DestBuf);
    break;

#if defined(LINUX)
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
    
    if (DestBuf == SourceBuf)
      return 1;
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
      if (ThisRec->length < Length) {
        ErrorsDetected++;
        if (DestBuf)
          AddFormattedRecord(FALSE, DestBuf, "Ooops! at line %d of buffer %c, the text (%d chrs.) is longer than the allocated size of the record (%d chrs).", 
            LineNo, SourceBuf->BufferKey, Length, ThisRec->length);
        else
          Message(NULL, "Ooops! at line %d of buffer %c, the text (%d chrs.) is longer than the allocated size of the record (%d chrs).", 
            LineNo, SourceBuf->BufferKey, Length, ThisRec->length);
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
          struct JumpObj *ThisHTEntry = (struct JumpObj *)ThisTag->Attr;
          if (DestBuf)
            AddFormattedRecord(FALSE, DestBuf, "  JumpObj: Buffer %c, Key %s", ThisHTEntry->HashBuf->BufferKey, ThisHTEntry->HashKey);
          else
            Message(NULL, "  JumpObj: Buffer %c, Key %s", ThisHTEntry->HashBuf->BufferKey, ThisHTEntry->HashKey); }
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
    
    return ErrorsDetected; }

  default: //In theory, this just can't happen.
    RunError("Unrecognized %%Q qualifier %s", QueryString);
    return 1; }
    
  return 0; }

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
    ParentElem = NextPathElem; }
    
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
  
  if (s_BacktraceDone)
    return 0;
  Message(NULL, "Backtrace:");
   
  while (Command && ThisFrame) {
    int ComChrNo = Command->CommandByteNo;
    int ComChrs = Command->ComChrs;
    char String[StringMaxChr];
    int StringLimit = s_TTYMode ? StringMaxChr : s_TermWidth;
    int MacroStatus = Command->CommandBuf->UnchangedStatus;
    int RedefinedMacro = ( (Command->CommandBuf->BufferKey != '\'') && ! (MacroStatus & SameSinceCompiled) && ((MacroStatus & MacroRun) == 0));
    int Bytes = 0, Chrs = 0;
    
    if (ThisFrame->type == InitializationCommand) { //InitializationCommand == 1
      char Key = Command->CommandBuf->BufferKey;
      Chrs = sprintf(String, "  Line %4d of -init commands %c: ", Command->CommandLineNo, (Key ? Key : ' ')); }
    else if (ThisFrame->type == ScriptFrame) { //ScriptFrame == 2
      struct CommandFile *ThisCommFile = ThisFrame->EditorInput;
      Chrs = sprintf(String, "  Line %4d of file %s: ",  ThisCommFile->LineNo, ThisCommFile->FileName); }
    else if (ThisFrame->type == MacroFrame) { //MacroFrame == 3
      char Key = Command->CommandBuf->BufferKey;
      Chrs = sprintf(String, "  Line %4d of macro %c: ", Command->CommandLineNo, Key ? Key : ' '); }
    else if (ThisFrame->type == CallFrame) {  //CallFrame == 4
      char Key = Command->CommandBuf->BufferKey;
      Chrs = sprintf(String, "  Line %4d of ( %c ) - function %s: ", Command->CommandLineNo, Key ? Key : ' ', ThisFrame->FirstRec->prev->prev->text); }
    else if (ThisFrame->type == ConsoleCommand) { //ConsoleCommand == 5
      char Key = Command->CommandBuf->BufferKey;
      Chrs = sprintf(String, "  Line %4d of console command %c: ", Command->CommandLineNo, (Key ? Key : ' ')); }
    else if (ThisFrame->type == DebuggerCommand) {  //DebuggerCommand == 6
      char Key = Command->CommandBuf->BufferKey;
      Chrs = sprintf(String, "  Line %4d of debugger %c: ",  Command->CommandLineNo, (Key ? Key : ' ')); }
    else
      strcpy(String, "Invalid backtrace stack-frame type");
      
    if (DestBuf) {
      if ( ! RedefinedMacro ) { //The original source is available.
        GetRecord(DestBuf, strlen(Command->CommandRec->text)+3);
        strncpy(DestBuf->CurrentRec->text, Command->CommandRec->text, ComChrNo);
        strcpy(DestBuf->CurrentRec->text + ComChrNo, "[");
        strncat(DestBuf->CurrentRec->text, Command->CommandRec->text+ComChrNo, ComChrs);
        strcat(DestBuf->CurrentRec->text, "]");
        strcat(DestBuf->CurrentRec->text, Command->CommandRec->text+ComChrNo+ComChrs); }
      else {
        int i;
        GetRecord(DestBuf, 100);
        Chrs = sprintf(DestBuf->CurrentRec->text, "(disassembly) Line %4d: ", Command->CommandLineNo);
        DumpCommand(Command, &i, String+Chrs); } }
    else {
      ScrollConsole(FALSE, FALSE);
      JotAddBoundedString(String, Chrs, Chrs, NULL, NULL, 0);
      if ( ! RedefinedMacro ) { //The original source is available.
        int MaxBytes = strlen(Command->CommandRec->text);
        JotAddBoundedString(Command->CommandRec->text, MaxBytes, ComChrNo, &Bytes, &Chrs, 0);
        if (s_TTYMode)
          JotAddBoundedString("[", 1, 1, &Bytes, &Chrs, 0);
        else
          JotSetAttr(Attr_CurrCmd);
        JotAddBoundedString(Command->CommandRec->text+ComChrNo, MaxBytes, ComChrs+1, &Bytes, &Chrs, 0);
        if (s_TTYMode)
          JotAddBoundedString("]", 1, 1, &Bytes, &Chrs, 0);
        else
          JotSetAttr(Attr_Normal);
        JotAddBoundedString(Command->CommandRec->text+ComChrNo+ComChrs+1, MaxBytes-ComChrNo-ComChrs-1, MaxBytes, &Bytes, &Chrs, 0); }
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
    
  s_BacktraceDone = TRUE;
  if (DestBuf)
    GetRecord(DestBuf, 0);
  return 0; }

//---------------------------------------------FetchStringArg
char *FetchStringArg(struct PtrArg **CurrentArgs_a, char *DefaultString)
  { //
  //Extracts string arg value and indexes to next.
  //If the type is DeferredStringArg then returns pointer to text,
  //if no arg, returns DefaultString unchanged, if DefaultString is NULL, returns ""
  //other args are copied into the DefaultString, returning the default string.
  //
  char *Value = 0;
  
  if (*CurrentArgs_a == NULL) {
    RunError("Insufficient args in list.");
    return ""; }
    
  if (((*CurrentArgs_a)->type) == DeferredStringArg) { // Indirection via buffer.
    char Key = (*CurrentArgs_a)->Key;
    struct Buf *ArgStringBuf;
    
    if (Key == '~') { //It's an indirect reference to the stack, but there's not a buffer at the top - just do the obvious decimal conversion.
      int Type;
      
      if (s_StackPtr < 1) {
        Fail("Stack underflow");
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
      return ""; } }
       
  else if ((*CurrentArgs_a)->type != StringArg) {
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

//-----------------------------------------------ExpandDeferredString
int ExpandDeferredString(struct Buf *DestinationBuf, char *Arg, int TextAsIs)
  { //Expands an indirect reference to a string in nominated buffer - only called by percent commands.
    //Will normally respect quotes ( ' ) and ignore escaped quotes ( \' )
    //except when TextAsIs is TRUE (derived from -hereEndsThePercentCommand;) then it all passes through unchanged. 
  char *Quote;
  char *Rec;
  int StartPoint = 0;
  char RecursionCheck[256];
  int Index = 0;
  int LocalStackPtr = s_StackPtr;   //Stack items are removed at the end of each pass when it's safe to remove buffer items.
  
  s_ArgString[0] = '\0';
  if (DestinationBuf->CurrentRec->length < strlen(Arg)) {
    FreeRecord(DestinationBuf, AdjustForwards);
    GetRecord(DestinationBuf, strlen(Arg)+100); }
  strcpy(DestinationBuf->CurrentRec->text, Arg);
  RecursionCheck[0] = '\0';
   
  while ( ! TextAsIs && (Quote = strchr((Rec = DestinationBuf->CurrentRec->text)+StartPoint, '\''))) {
    char Key = ChrUpper(Quote[1]);
    struct Buf *ArgStringBuf;
    
    if ( (2 < Quote-Rec && ((Quote-1)[0] == '\\') && ((Quote-2)[0] == '\\') ) ) //It's a string of backslashes - let them through unchanged.
      StartPoint += Quote-Rec+1;
    else if ( (1 <= Quote-Rec && ((Quote-1)[0] == '\\') )) { //It's a single escaped quote - strip it out.
      DestinationBuf->CurrentByte = Quote-Rec-1;
      DestinationBuf->SubstringLength = 1;
      SubstituteString(DestinationBuf, NULL, -1); 
      StartPoint += Quote-Rec+1; }
    else {
      if ( ! CheckBufferKey(Key)) {
        RunError("Invalid buffer key.");
        return 1; }
      if (Key == '~') { //It's an indirect reference to the stack, but there's not a buffer at the top - just do the obvious decimal conversion.
        int Type;
        
        if (LocalStackPtr < 1) {
          Fail("Stack underflow");
          return 1; }
        Type = s_Stack[LocalStackPtr-1].type;
        if (Type == FrameType_Buf) {
          ArgStringBuf = ((struct bufFrame *)(&s_Stack[--LocalStackPtr]))->BufPtr;
          if (ArgStringBuf == s_CurrentBuf) {
            Fail("Cannot remove current buffer from stack.");
            return 1; } }
        else {
          ArgStringBuf = NULL;
          if (Type == FrameType_Int)
            sprintf(s_ArgString, "%lld", (long long)s_Stack[--LocalStackPtr].Value);
          else
            sprintf(s_ArgString, "%f", ((struct floatFrame *)(&s_Stack[--LocalStackPtr]))->fValue); } }
      else {
        ArgStringBuf = GetBuffer((int)Key, NeverNew);
        
        //In the event of a '~ reference to '~ this is *not* recursive since the stack gets poped as it takes the outer reference.
        if (strchr(RecursionCheck, Key)) { //Recursion detected.
          StartPoint = Quote-Rec+2;
          Fail("Recursion detected in \"%s\" buffer %c", Arg, Key);
          return 1; } }
          
      DestinationBuf->CurrentByte = Quote-Rec;
      DestinationBuf->SubstringLength = 2;
      if (ArgStringBuf && ! ArgStringBuf->CurrentRec) {
        if (ArgStringBuf == s_CurrentBuf)
          GetRecord(s_CurrentBuf, 0);
        RunError("Buffer ( %c ) is not defined.", Key);
        return 1; }
      SubstituteString(DestinationBuf, ArgStringBuf ? ArgStringBuf->CurrentRec->text : s_ArgString, -1);
      while (LocalStackPtr < s_StackPtr)
        KillTop();
      RecursionCheck[Index++] = Key;
      RecursionCheck[Index] = '\0'; } }
    
  DestinationBuf->CurrentByte = 0;
  return 0; }

//----------------------------------------------ChangeWindows
int ChangeWindows(char Key, struct Buf *CmdBuf)
  { //Maintains the s_FirstWindow list, interpreting %w commands passed from main command scanner.
  int SpecifiedHeight = 0,  SpecifiedWidth = 0, SliceGuard = 0, HistoricalSyntax = FALSE, WinNo = -1;
  int NextLine = 0, NextColumn = 0;
  int DeltaHeight = 0, DeltaWidth = 0, FreezeValue;
  int WindowIndex = 0;
  int PredWinType = -1, PredWinTop = 0, PredWinBot = 0;
  char Chr = '\0', NewKey = -1;
  char *PopupOpt = NULL, *DelimOpt = NULL, *WidthOpt = NULL, *HeightOpt = NULL, *NewKeyOpt = NULL, *WinNoOpt = NULL, *DeleteOpt = NULL, *InsertOpt = NULL, *FreezeOpt = NULL;
  struct Window *Win = s_FirstWindow, **WinPtr, *NewWin, *LeftmostSlice = NULL;
  struct Buf *Buffer;
  struct Rec *FirstRec;
    
  if (s_TTYMode)
    return 0;
  if (Key == ' ') {
    Key = '\0';
    HistoricalSyntax = TRUE; }
  Chr = HistoricalSyntax ? StealCharacter(CmdBuf) : CmdBuf->CurrentRec->text[0];
   
  if ( ! Chr) { //No arguments - delete all windows.
    while (Win) { // Window-clear loop.
      struct Window *NextWin = Win->next;
      free(Win);
      Win = NextWin; }
    s_FirstWindow = NULL;
    s_ConsoleTop = 0;
    return 0; }
    
  PopupOpt = strstr(CmdBuf->CurrentRec->text, " -popup");
  DelimOpt = strstr(CmdBuf->CurrentRec->text, " -delim");
  DeleteOpt = strstr(CmdBuf->CurrentRec->text, " -delete");
  InsertOpt = strstr(CmdBuf->CurrentRec->text, " -insert");
  if ( (FreezeOpt = strstr(CmdBuf->CurrentRec->text, " -freeze")) )
    sscanf(FreezeOpt, " -freeze=%d", &FreezeValue);
  if ( (WidthOpt = strstr(CmdBuf->CurrentRec->text, " -width=")) ) {
    if (strchr(WidthOpt, '+'))
      sscanf(WidthOpt, " -width=%d+%d", &SpecifiedWidth, &SliceGuard);
    else
      sscanf(WidthOpt, " -width=%d", &SpecifiedWidth);
    if (WidthOpt && ( ! PopupOpt) && (SpecifiedWidth <= 0) ) {
      Fail("Invalid window-width specification");
      return 1; } }
  
  if (HistoricalSyntax) {
    SpecifiedHeight = GetDec(CmdBuf);
    Key = ChrUpper(NextNonBlank(CmdBuf)); }
  else { //New syntax also allows these options.
    if ( (HeightOpt = strstr(CmdBuf->CurrentRec->text, " -height=")) )
      sscanf(HeightOpt, " -height=%d", &SpecifiedHeight);
    if ( (NewKeyOpt = strstr(CmdBuf->CurrentRec->text, " -key=")) ) {
      if (sscanf(NewKeyOpt, " -key=%c", &NewKey) < 0)
        NewKey = '\0'; }
    if ( (WinNoOpt = strstr(CmdBuf->CurrentRec->text, " -winno")) ) {
      sscanf(WinNoOpt, " -winno=%d", &WinNo); }
    if (HeightOpt && (SpecifiedHeight <= 0) ) {
      Fail("Invalid window-height specification.");
      return 1; } }
  
  if ( ! WinNoOpt && ( (SpecifiedHeight == 0) && (SpecifiedWidth == 0) ) ) { //Add delimiter to last-defined window.
    if ( ! HistoricalSyntax) {
      SynError(NULL, "Window height and width are both zero.");
      return 1; }
    while ( Win && Win->next )
      Win = Win->next;
    if (Win)
      Win->DisplayFooter = TRUE;
    return 0; }

  //Search the window list for selected window/last window/leftmost slice of last window.
  WinPtr = &s_FirstWindow;
  while( (Win = *WinPtr) ) { //Selected-window search loop.
    int Type = (Win->WindowType) & WinType_Mask;
    if ( ((Win->WindowType & WinType_Mask) == WinType_Slice) && (Win->WindowType & WinType_Leftmost) )
      LeftmostSlice = Win;
    if (WindowIndex++ == WinNo) { //Found window for modification.
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
  
  if (WinNoOpt && ! Win) {
    Fail("Can't identify window for modification.");
    return 1; }

  if (0 <= WinNo) { //Modify an existing window.
    int Type = (Win->WindowType) & WinType_Mask;
    int AdjustedWindowTop = Win->Top;
    int IsLeftmost = FALSE;
    if ( (DeltaHeight || HeightOpt) && ( (Win->Bot+DeltaHeight < Win->Top) || (s_TermHeight-1 < s_ConsoleTop+DeltaHeight) ) ) {
      Fail("Invalid window-height change.");
      return 1; }
    if (HeightOpt && (Type == WinType_Slice) && ! (Win->WindowType & WinType_Leftmost)) {
      Fail("For window slices, only the leftmost slice can have it's height redefined.");
      return 1; }
      
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
      if (DeltaWidth && PredWinType == WinType_Normal) {
        Fail(InsertOpt ? "Attempt to add a slice to a full-width window" : "Attempt to redefine the width of a normal window.");
        return 1; }
      if (Type == WinType_Slice) {
        struct Window *TestWin = Win;
        if (DeltaHeight && ( ! (Win->WindowType && WinType_Leftmost))) {
          Fail("Attempt to change the height of a secondary slice.");
          return 1; }
        if (DeltaWidth < 0 && TestWin && ! ((TestWin->WindowType) & WinType_Leftmost) && (TestWin->Left+DeltaWidth < 0) ) {
            Fail("Invalid window-width shrink.");
            return 1; }
        if (0 < DeltaWidth) {
          while ( TestWin && TestWin->next && ! (TestWin->next->WindowType & WinType_Leftmost))
            TestWin = TestWin->next;
          if (TestWin && s_TermWidth < TestWin->Right+DeltaWidth) {
            Fail("Invalid window-width expand.");
            return 1; } } }
      if (InsertOpt && SpecifiedWidth && (Win->WindowType & WinType_Leftmost) && (Win->WindowType & WinType_Slice) ) {
        Win->WindowType = (Win->WindowType & (-1^WinType_Leftmost));
        IsLeftmost = FALSE;
        NextColumn = 0; }
      if ( ! DeleteOpt && ! InsertOpt ) {
        Win->Bot += DeltaHeight;
        Win->Right += DeltaWidth; }
        
      //Adjust all follwing slice-group members.
      if (Type == WinType_Slice) { 
        if ( ! DeleteOpt && ! InsertOpt )
          Win = Win->next;
        while ( Win && ! (Win->WindowType & WinType_Leftmost)) {
          if ((Win->Left+DeltaWidth < 0) || s_TermWidth < Win->Right+DeltaWidth) {
            Fail("Invalid window-width change.");
            return 1; }
          Win->Bot += DeltaHeight;
          Win->Left += DeltaWidth;
          Win->Right += DeltaWidth;
          if (IsLeftmost && Win)
            Win->WindowType |= WinType_Leftmost;
          IsLeftmost = FALSE;
          Win = Win->next; } } }
    if (NewKeyOpt)
      Win->WindowKey = toupper(NewKey);
    if (FreezeOpt)
      Win->WindowType = FreezeValue ? (Win->WindowType | WinType_Frozen) : (Win->WindowType & (-1^WinType_Frozen));
        
    //Adjust Top and Bot of successor windows.
    while ( Win ) { //Adjust following windows loop.
      if (InsertOpt || AdjustedWindowTop < Win->Top) { //Adjusting Top and Bot of windows below selected window.
        Win->Top += DeltaHeight;
        Win->Bot += DeltaHeight; }
      Win = Win->next; }
        
    s_ConsoleTop += DeltaHeight;
    if ( ! InsertOpt) {
      JotUpdateWindow();
      return 0; } }

  if ( ! SpecifiedHeight && ! SpecifiedWidth) { //Add a new window.
    Fail("No window height or width specified.");
    return 1; }
  NewWin = (struct Window *)JotMalloc(sizeof(struct Window));
  NewWin->WindowType = (PopupOpt ? WinType_Popup : (SpecifiedWidth ? WinType_Slice : WinType_Normal));
  if (PopupOpt) { //Popup windows calculate Top, Bottom, Left and Right directly from the width and height.
    if (0 < SpecifiedHeight) {
      NewWin->Top = 0;
      NewWin->Bot = SpecifiedHeight; }
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
    else {
      NewWin->Left = s_TermWidth+SpecifiedWidth;
      NewWin->Right = s_TermWidth-1; } }
  else {
    if (s_TermHeight < (NextLine+SpecifiedHeight+1) ) {
      free(NewWin);
      RunError("Total height of windows exceeds terminal height");
      return 1; }
    if (s_TermWidth < (NextColumn+SpecifiedWidth-1) ) {
      free(NewWin);
      RunError("Total width of slices exceeds terminal width");
      return 1; }
      
    if (SpecifiedWidth) { //For sliced windows left and Right are calculated from the aggregate slice widths.
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
        SynError(NULL, "No window height specified");
        free(NewWin);
        return 1; }
      NewWin->Left = NextColumn+SliceGuard;
      NewWin->Right = NewWin->Left + SpecifiedWidth - 1;
      if (InsertOpt && (LeftmostSlice == *WinPtr) ) //The original leftmost slice is now the second slice.
        NewWin->WindowType |= WinType_Leftmost; }
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
  NewWin->DisplayFooter = (SpecifiedWidth && ! SpecifiedHeight) ? LeftmostSlice && LeftmostSlice->DisplayFooter : FALSE;
  NewWin->DisplayFooter = (DelimOpt != NULL);
  Key = ChrUpper(Key);
  NewWin->WindowKey = Key;
  NewWin->LastKey = '\0';
  InitTermTable();
  if (Key == '\0')
    return 0;
  if ( ! (Buffer = GetBuffer(Key, OptionallyNew)) )
    return 1;
  FirstRec = Buffer->FirstRec;
  if (FirstRec != NULL)
    FirstRec->prev->DisplayFlag = Redraw_Line;
  else
    GetRecord(Buffer, 1);
    
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
      PerceivedChr += CellWidth - FullWidth + VisibleWidth; }
      
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
        
    PerceivedChr = TabPoint+JotStrlenBytes(String+ByteNo, CurrentByte-ByteNo)+1; }
    
  else
    PerceivedChr = JotStrlenBytes(TextBuf->CurrentRec->text, TextBuf->CurrentByte+1);
    
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
      if (TabCells) {
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
      else {
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

  if (TabIndexMax <= 1) { //Either linear text or there are no tabcells or tabstops set for this buffer.
    int Bytes = 0, Chrs = 0, FirstByte = 0;    //The first byte of the string, this appears in the leftmost character of the slice line.
    
    if (New) {
      s_ChrNo = LeftOffset; }
    FirstByte = JotStrlenChrs(String, s_ChrNo);    //The first byte of the string, this appears in the leftmost character of the slice line.
    
    InView = TRUE;
    if (FirstByte <= LastByte) {
      if (ByteNo < TotalBytes)
        JotAddBoundedString(String+FirstByte, LastByte-FirstByte, SliceWidth-s_ChrNo+LeftOffset, &Bytes, &Chrs, ColourPair);
      s_ChrNo += Chrs;
      ByteNo = LastByte;
      InView = LastByte-FirstByte <= Bytes; }
    else
      InView = FALSE;
        
    return ( (1 <= s_ChrNo-LeftOffset) && InView ); }

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
//  else if (OrigTabStops && TabIndexMax) { //String contains at least one tab - display as simple tabular text.
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
  int DisplayedCurrentByte = FALSE;                    //Flag is set TRUE only when the CurrentByte of the s_CurrentBuf appears in display. 
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
        while ( (++PopupLineNo < PopupMaxHeight-1) && (PopupRec != TextBuf->FirstRec) );
        
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
              
        if (s_TermHeight-s_NewConsoleLines-2 <= ThisLine)
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
          if (s_CommandMode & Command_ScreenMode) {
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
        JotSetAttr(Attr_Normal);
            
        if (PopupWindow)
          s_ChrNo += Win->Right - Win->Left - Win->PopupWidth;
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
            
          WindowKey = TextBuf->BufferKey;
          JotGotoXY(Left, Bot-1);
          JotSetAttr(Attr_WinDelim);
          if(strlen(SeparatorString) < MaxNameLength) {
            ActualLength = strlen(SeparatorString);
            JotAddBoundedString(SeparatorString, ActualLength, ActualLength, NULL, NULL, 0); }
          else {
            int SepStart = strlen(SeparatorString)-MaxNameLength;
            ActualLength = MaxNameLength;
            JotAddBoundedString("...", 3, 3, NULL, NULL, 0);
            JotAddBoundedString(SeparatorString+SepStart, SliceWidth, MaxNameLength, NULL, NULL, 0); }
          
          for (Chr = ActualLength; Chr++ < SliceWidth; )
#if defined(VC)
            putchar(' ');
#else
            addch(' ');
#endif
          JotGotoXY(Win->Left + SliceWidth - 5, Bot-1);
          JotAddChr(WindowKey);
          JotSetAttr(Attr_Normal);
          s_TermTable[SliceIndex+Bot-1] = TextBuf->PathName; } }
      
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
//NB - The windows version has an extra parameter for the tatty old windows HANDLE in addition to FILE and Channel nos.
int ReadBuffer(FILE *File, HANDLE *FileHand, int FileDesc, int NonUnicode, char *Name, struct Buf * DestBuf, 
                       int BinaryRecSize, off64_t SeekOffset, long *MaxBytes, int BlockMode, int MaxRecords)
#else
int ReadBuffer(FILE *File, int FileDesc, int NonUnicode, char *Name, struct Buf * DestBuf, 
                       int BinaryRecSize, off64_t SeekOffset, long *MaxBytes, int BlockMode, int MaxRecords)
#endif
  { //Reads stream (from either File or FileDesc) into specified buffer if buffer already exists then buffer is scrubbed.
    //When FileDesc is set it reads from FileDesc with ReadUnbufferedStreamRec() otherwise it reads from File using ReadNewRecord()
    //With BinaryRecSize set it reads fixed-length records ignoring any '\0','\n' bytes in the stream.
    //With MaxBytes set it reads, at most, that many bytes, this is only efective when reading from FileDesc using read(), silently ignored if from File.
    //With Hold set it keeps the file handle open.
    //With nonzero MaxRecords, the no. of records read is limited to that value.
    //If both MaxBytes and Becords are set then limit is determined by whichever is satisfied first.
    //If successful, returns total no. of records read (possibly 0), if the final record has no CR then this is counted as one record.
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
    DestBuf->IOBlock->NextAsyncBuf = NULL;
#if defined(LINUX)
    DestBuf->IOBlock->ChildPid = 0;
#endif
    DestBuf->IOBlock->InPipe = 0;
    DestBuf->IOBlock->OutPipe = 0;
    DestBuf->IOBlock->HoldDesc = 0;
    DestBuf->IOBlock->ExitCommand = NULL;
    DestBuf->IOBlock->BucketBlock = NULL;
    DestBuf->IOBlock->ReadyForReading = TRUE; }
  if (BlockMode) { //In block mode a block of bytes, size specified by MaxBytes, is read into one record without formatting or buffering.
    long BytesRead;
    GetRecord(DestBuf, (*MaxBytes)+1);
#if defined(VC)
    if (FileHand)
      BytesRead = ReadBucketFromHandle(FileHand, DestBuf->CurrentRec->text, *MaxBytes);
    else
      BytesRead = read(FileDesc, DestBuf->CurrentRec->text, *MaxBytes);
#else
    BytesRead = read(FileDesc, DestBuf->CurrentRec->text, *MaxBytes);
#endif
    DestBuf->CurrentRec->text[BytesRead] = '\0';
    return BytesRead ? 1 : -1; }
            
  while ( 1 ) { //Record-read loop.
#if defined(VC)
    ReadOK = (FileDesc || FileHand) ? ReadUnbufferedStreamRec(DestBuf, FileDesc, FileHand, BinaryRecSize, MaxBytes) : ReadNewRecord(DestBuf, File, NonUnicode);
#else
    ReadOK = FileDesc ? ReadUnbufferedStreamRec(DestBuf, FileDesc, BinaryRecSize, MaxBytes) : ReadNewRecord(DestBuf, File, NonUnicode);
#endif
    if (s_BombOut) {
      Message(NULL, "Abandoning read \"%s\", (line %d \"%s\")", Name, s_EditorInput->LineNo, s_CommandBuf->CurrentRec->text);
      break; }
    if ( ! ReadOK) {
      DestBuf->IOBlock->ReadyForReading = FALSE;
      break; }
    RecordsRead += 1;
    if (MaxRecords && RecordsRead == MaxRecords)
      break; }
    
  if (MaxRecords ||  ! DestBuf->FirstRec)
    GetRecord(DestBuf, 0);
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
    char RecoveryPathname[StringMaxChr], Temp[StringMaxChr+40];
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
      
      for (i=0; i < strlen(NameCopy); i++) { //Remove forward slashes.
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
      sprintf(Temp, "<<Recovery pathname: %s to buffer %c >>", RecoveryPathname, DestBuf->BufferKey); }
    else //Read-only files.
      sprintf(Temp, "<<Recovery pathname: %s to buffer %c >>", Name, DestBuf->BufferKey);
    UpdateJournal(Temp, NULL); }
    
  return RecordsRead; }

//----------------------------------------------ReadUnbufferedStreamRec
#if defined(VC)
int ReadUnbufferedStreamRec(struct Buf *ThisBuf, int FileDesc, HANDLE *FileHand, int BinaryRecSize, long *MaxBytes)
#else
int ReadUnbufferedStreamRec(struct Buf *ThisBuf, int FileDesc, int BinaryRecSize, long *MaxBytes)
#endif
  { //Reads from FileDesc (typically stdin) to a new record in nominated buffer - only called when stdin is a pipe, all other reads go via ReadNewRecord().
    //MaxBytes limits the number of bytes read to no. specified, if zero reads to end of file.
    //returns 1 if read was successful, otherwise 0.
    //
  static int Its_Binary = FALSE; 
  int RecordBytes = 0;                   //Total record length.
  int BytesCopied = 0;                   //Counts bytes copied into destination record.
  char *Terminator;                      //Position of the terminator character (e.g. '\n' for ascii).
  struct BucketBlock *ThisBucketBlock;
  
  if ( ! ThisBuf->IOBlock) { //First call - initialize.
    ThisBuf->IOBlock = (struct IOBlock *)JotMalloc(sizeof(struct IOBlock));
    ThisBuf->IOBlock->ExitCommand = NULL;
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
    if (MaxBytes)
      *MaxBytes -= ThisBuf->IOBlock->BucketBlock->BufBytes;
    if (ThisBuf->IOBlock->BucketBlock->BufBytes < 0) {
      Fail("Error reading that stream");
      free(ThisBuf->IOBlock->BucketBlock);
      ThisBuf->IOBlock->BucketBlock = NULL;
      return 0; }
    ThisBuf->IOBlock->BucketBlock->RecStartChr = 0; 
    if (BinaryRecSize) {
      ThisBuf->IOBlock->BucketBlock->RecEndChr = -1;
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

  while ( 1 ) { //Bucket loop - read sufficient bucketfuls (usually none) to define one record of the destination buffer.
                //Exit when MaxBytes limit reached or when it reads a valid terminator - an EOL or an empty bucket.
    struct BucketBlock *NextBucketBlock;
    
    //Another bucketful required to complete current line?
    if (Its_Binary) {
      if ( (ThisBucketBlock->RecStartChr + BinaryRecSize - RecordBytes) <= ThisBucketBlock->BufBytes) { //A complete record in this bucket.
        ThisBucketBlock->RecEndChr += BinaryRecSize - RecordBytes;
        RecordBytes += ThisBucketBlock->RecEndChr - ThisBucketBlock->RecStartChr + 1;
        break; }
      else {
        ThisBucketBlock->RecEndChr = ThisBucketBlock->BufBytes - 1;
        RecordBytes += ThisBucketBlock->RecEndChr - ThisBucketBlock->RecStartChr + 1;
        if ( (MaxBytes && ! *MaxBytes ) || ! ThisBucketBlock->BufBytes) { //Already read to limit or EOF.
          ThisBuf->IOBlock->ReadyForReading = FALSE;
          break; } } }
    else { //Non-binary (text) read.
      if ((Terminator = (char *)memchr(ThisBucketBlock->Bucket+ThisBucketBlock->RecStartChr, '\n', ThisBucketBlock->BufBytes-ThisBucketBlock->RecStartChr)) ) {
         //This bucketful is the last, or contains a valid EOL marker - we're now ready to copy bucket(s) to a new record.
        ThisBucketBlock->RecEndChr = Terminator - ThisBucketBlock->Bucket;
        RecordBytes += ThisBucketBlock->RecEndChr-ThisBucketBlock->RecStartChr;
        break; }
      else if ( MaxBytes && ! *MaxBytes) { //Last part-record in a MaxBytes-limited read.
        RecordBytes += ThisBucketBlock->BufBytes - ThisBucketBlock->RecStartChr;
        ThisBucketBlock->RecEndChr = ThisBucketBlock->BufBytes;
        break; }
      else { //Normal incomplete record, read another bucketful.
        RecordBytes += ThisBucketBlock->BufBytes - ThisBucketBlock->RecStartChr;
        ThisBucketBlock->RecEndChr = ThisBucketBlock->BufBytes; } }
    if ( ! ThisBucketBlock->BufBytes)
      break;
      
    if (ThisBuf->IOBlock->OutPipe && ( ! TestAsyncPipe(ThisBuf, 100000))) {
      ThisBuf->IOBlock->ReadyForReading = FALSE; 
      break; }
    NextBucketBlock = (struct BucketBlock *)JotMalloc(sizeof(struct BucketBlock));
#if defined(VC)
    if (FileHand)
      NextBucketBlock->BufBytes = ReadBucketFromHandle(FileHand, NextBucketBlock->Bucket, (MaxBytes && *MaxBytes<=BucketSize) ? *MaxBytes : BucketSize);
    else
      NextBucketBlock->BufBytes = read(FileDesc, NextBucketBlock->Bucket, (MaxBytes && *MaxBytes<=BucketSize) ? *MaxBytes : BucketSize);
#else
    NextBucketBlock->BufBytes = read(FileDesc, NextBucketBlock->Bucket, (MaxBytes && *MaxBytes<=BucketSize) ? *MaxBytes : BucketSize);
#endif
    if (MaxBytes)
      *MaxBytes -= NextBucketBlock->BufBytes;
    ThisBucketBlock->Next = NextBucketBlock;
    NextBucketBlock->Next = NULL;
    NextBucketBlock->RecStartChr = 0;
    if (Its_Binary)
      NextBucketBlock->RecEndChr = 0;
    ThisBucketBlock = NextBucketBlock; }

  //At this point we have sufficient pending buckets to define the next record.
  GetRecord(ThisBuf, (Its_Binary ? RecordBytes*3 : RecordBytes)+3);
  ThisBucketBlock = ThisBuf->IOBlock->BucketBlock;
   
  while ( ThisBucketBlock ) { //Record  loop - Intermediate blocks each contain a full bucketload, for text files, the last one terminates with '\n'.
    struct BucketBlock *NextBucketBlock = ThisBucketBlock->Next;
    
    if (Its_Binary) {
      int i, BytesFromThisBucket = ThisBucketBlock->RecEndChr - ThisBucketBlock->RecStartChr + 1;
      for (i = 0; i < BytesFromThisBucket; i++)
        sprintf(ThisBuf->CurrentRec->text+(BytesCopied+i)*3, "%02X ", (unsigned char)(ThisBucketBlock->Bucket+ThisBucketBlock->RecStartChr)[i]);
      BytesCopied += BytesFromThisBucket; }
    else {
      int BytesFromThisBucket = ThisBucketBlock->RecEndChr-ThisBucketBlock->RecStartChr;
      memcpy(ThisBuf->CurrentRec->text+BytesCopied, ThisBucketBlock->Bucket+ThisBucketBlock->RecStartChr, BytesFromThisBucket);
      BytesCopied += BytesFromThisBucket; }
      
    if (NextBucketBlock) {
      free(ThisBucketBlock);
      ThisBucketBlock = NextBucketBlock;
      if (Its_Binary) {
        ThisBucketBlock->RecEndChr = BinaryRecSize-BytesCopied - 1;
        if (ThisBucketBlock->BufBytes <= ThisBucketBlock->RecEndChr)
          ThisBucketBlock->RecEndChr = ThisBucketBlock->BufBytes - 1; } }
    else {
      ThisBucketBlock->RecStartChr = ThisBucketBlock->RecEndChr+1;
      break; }
    ThisBuf->IOBlock->BucketBlock = ThisBucketBlock; }
    
  if (Its_Binary)
    ThisBuf->CurrentRec->text[(RecordBytes*3)+1] = '\0';
  else
    ThisBuf->CurrentRec->text[RecordBytes] = '\0';
  if ( (ThisBucketBlock->RecStartChr + (Its_Binary ? 1 : 0)) <= ThisBucketBlock->BufBytes )
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
      sleep(1);
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

//----------------------------------------------TestAsyncPipe
int TestAsyncPipe(struct Buf *AsyncBuf, int Timeout_us)
  { //Returns TRUE if the specified buffer's InPipe is ready for reading.
#if defined(VC)
  DWORD Ready = 0;
  
  PeekNamedPipe(AsyncBuf->IOBlock->OutPipe, NULL, 0, NULL, &Ready, NULL);
  return (Ready != 0); }
#else
  struct timeval Timeout;
  fd_set AsyncFDs;
  
  Timeout.tv_sec  = 0;
  Timeout.tv_usec = Timeout_us;
  
  FD_ZERO(&AsyncFDs);
  FD_SET(AsyncBuf->IOBlock->OutPipe, &AsyncFDs);
      
  return select(s_TopAsyncFD+1, &s_AsyncFDs, NULL, NULL, &Timeout); }
#endif

//----------------------------------------------AsyncReadRecs
int AsyncReadRecs()
  { //If there are asynchronous reads pending then read one record from the first available stream.
#if defined(VC)
  struct Buf *ThisAsyncBuf = s_FirstAsyncBuf;
  DWORD Ready = 0;
  
  while (ThisAsyncBuf) {
    PeekNamedPipe(ThisAsyncBuf->IOBlock->OutPipe, NULL, 0, NULL, &Ready, NULL);
    ThisAsyncBuf->IOBlock->ReadyForReading = Ready;
    
    while (ThisAsyncBuf->IOBlock->ReadyForReading)
      ReadUnbufferedStreamRec(ThisAsyncBuf, 0, ThisAsyncBuf->IOBlock->OutPipe, 0, 0);
      
    if (s_BombOut)
      break;
    ThisAsyncBuf = ThisAsyncBuf->IOBlock->NextAsyncBuf; }
    
  return 0;
  
#else
  int FileDesc, Activity;
  struct timeval Timeout;
  struct Buf *ThisAsyncBuf = s_FirstAsyncBuf;
  
  Timeout.tv_sec  = 0;
  Timeout.tv_usec = 100;
  
  FD_ZERO(&s_AsyncFDs);
  s_TopAsyncFD = 0;
  while (ThisAsyncBuf) {
    FD_SET(ThisAsyncBuf->IOBlock->OutPipe, &s_AsyncFDs);
    if (s_TopAsyncFD < ThisAsyncBuf->IOBlock->OutPipe)
      s_TopAsyncFD = ThisAsyncBuf->IOBlock->OutPipe;
    ThisAsyncBuf = ThisAsyncBuf->IOBlock->NextAsyncBuf; }
    
  ThisAsyncBuf = s_FirstAsyncBuf;
      
  if (0 < ( Activity = select(s_TopAsyncFD+1, &s_AsyncFDs, NULL, NULL, &Timeout)) ) {
    while (ThisAsyncBuf) {
      FileDesc = ThisAsyncBuf->IOBlock->OutPipe;
      if (FD_ISSET(FileDesc, &s_AsyncFDs)) { //select() says the FileDesc is ready - mark the stream as ready and read.
        struct Rec *CurrentRec;
        CurrentRec = ThisAsyncBuf->CurrentRec = ThisAsyncBuf->FirstRec->prev;
        ThisAsyncBuf->IOBlock->ReadyForReading = TRUE;
        while (ThisAsyncBuf->IOBlock->ReadyForReading)
          ReadUnbufferedStreamRec(ThisAsyncBuf, FileDesc, 0, 0);
        ThisAsyncBuf->CurrentRec = CurrentRec; }
      
      free(ThisAsyncBuf->IOBlock->BucketBlock);
      ThisAsyncBuf->IOBlock->BucketBlock = NULL;
      ThisAsyncBuf = ThisAsyncBuf->IOBlock->NextAsyncBuf; } }
  else
    return 1;
#endif
 return 0; }

//----------------------------------------------ReadNewRecord
int ReadNewRecord(struct Buf *TextBuf, FILE *File, int NonUnicode)
  {
  //Reads one line from currently selected I/P channel.
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
  
  while ( ! ExitFlag) { // Chr read loop.
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
    
  GetRecord(TextBuf, TotalSize+1);
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
    if(s_FirstAsyncBuf)
      AsyncReadRecs();
    if (s_ObeyStdin && FileHandle==s_EditorConsole->FileHandle)
      return fgetc(stdin); 
    else if ( ! s_EditorConsole || (FileHandle != s_EditorConsole->FileHandle) ) {
      WChr = fgetwc(FileHandle);
      if (WChr == WEOF)
        return EOF;
      else if ((((int)WChr) & 255) == 0xC2)  //Some sort of Microsoft UTC prefix???
        continue;
      InPtr = WideCharToMultiByte(CP_UTF8, 0, &WChr, 1, OutBuf, 100,  NULL, NULL);
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
    if(s_FirstAsyncBuf)
      AsyncReadRecs();
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
      else if ((((int)WChr) & 255) == 0xC2)  //Some sort of Microsoft UCS-BOM prefix???
        continue;
      InPtr = wctomb(Result, WChr);
      return Result[OutPtr++]; }
    if (s_BombOut)
      return '\0';
    if (Chr == ERR) //An error here usually means that there were no characters ready for reading.
      continue;
    else if (Chr < KEY_MIN)
      return Chr;
    else if (Chr == KEY_BACKSPACE)
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
        InPtr = sprintf(Result, "\eM%04X", (unsigned int)event.bstate); }
      else
        continue; }
    else if (Chr < 0x023D)
      InPtr = sprintf(Result, "\eX%04X", Chr);
    else //Assume it's unicode.
      return Chr;
    if (InPtr < 0)
      continue;
    return Result[OutPtr++]; } }
#endif

#if defined(VC)
//----------------------------------------------OpenWindowsCommand
int OpenWindowsCommand(HANDLE *InPipe, HANDLE *OutPipe, HANDLE *ProcessId, int StreamInBuf, int Async, char *Command)
  { //In windows, performs a function similar to popen() in unix.
  PROCESS_INFORMATION piProcInfo; 
  STARTUPINFO siStartInfo;
  int BytesSent;
  char *FullCommand = (char *)JotMalloc(strlen(Command)+10);
  DWORD avail = 0;
  static HANDLE s_hChildStd_IN_Rd = NULL;
  static HANDLE s_hChildStd_IN_Wr = NULL;
  static HANDLE s_hChildStd_OUT_Rd = NULL;
  static HANDLE s_hChildStd_OUT_Wr = NULL;
  static SECURITY_ATTRIBUTES s_saAttr; 
  
  sprintf(FullCommand, "%s\0", Command); 
     
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
  
  if ( ! CreateProcess(NULL, FullCommand, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo)) {
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
      sleep(1);
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
//----------------------------------------------DoublePopen
void DoublePopen(int *InPipe, int *OutPipe, pid_t *ChildPid, struct Buf *TextBuf, char *Commands)
  { //Similar to popen execpt that it can read it's stdin and write to it's stdout, popen can only do one or the other.
  int PipeIn[2];
  int PipeOut[2];
  pid_t child;
  char *ArgV[100];
  char *Command = strtok(Commands, " ");
  int i = 1;
  
  ArgV[0] = Command; 
  do 
    ArgV[i] = strtok(NULL, " ");
    while (ArgV[i++]);
         
  if ((pipe(PipeIn)) == -1) {
    RunError("pipe");
    return; }
  if ((pipe(PipeOut)) == -1) {
    RunError("pipe");
    return; }
  
  child = fork();
  
  if (child == 0) { // This is child! 
    if ((dup2(PipeIn[0], STDIN_FILENO)) == -1) {
      RunError("dup2 stdin");
      return; }
    if ((dup2(PipeOut[1], STDOUT_FILENO)) == -1) {
        RunError("dup2 stdout");
        return; }
    if ((dup2(PipeOut[1], STDERR_FILENO)) == -1) {
        RunError("dup2 stderr");
        return; }
    if ((close(PipeIn[0])) == -1) {
        RunError("close");
        return; }
    if ((close(PipeOut[1])) == -1) {
        RunError("close");
        return; }
    if ((close(PipeIn[1])) == -1) {
      RunError("close");
      return; }
    if ((close(PipeOut[0])) == -1) {
      RunError("close");
      return; }
    if ((execvp(Command, ArgV)) == -1) {
      RunError("execvp");
      return; } }
           
  else if (child == -1) {
    RunError("fork");
    return; }
       
  else { // This is parent!
    if (ChildPid)
      *ChildPid = child;
    if ((close(PipeIn[0])) == -1) {
        RunError("close");
        return; }
    if ((close(PipeOut[1])) == -1) {
      RunError("close");
      return; }
      
    if (TextBuf) {
      struct Rec *ThisRec = TextBuf->FirstRec;
       
        do { //Record loop - copy TextBuf's text to child's stdin.
        if ((write(PipeIn[1], ThisRec->text, strlen(ThisRec->text))) == -1 || (write(PipeIn[1], "\n", 1) == -1)) {
          RunError("write 1");
          return; }
        ThisRec = ThisRec->next; }
      while (ThisRec != TextBuf->FirstRec); }
    
    if (OutPipe)
      *OutPipe = PipeOut[0];
    else if ((close(PipeOut[0])) == -1)
      RunError("close");
      
    if (InPipe)
      *InPipe = PipeIn[1];
    else if ((close(PipeIn[1])) == -1)
      RunError("close"); } }
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
  int ScreenMode = (s_CommandMode & Command_ScreenMode);
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
    int RecordLength = DestBuf->CurrentRec->length;
    CommandText = DestBuf->CurrentRec->text;
    Chr = JotGetCh(FileHandle);
    if (feof(s_EditorInput->FileHandle))
      return EOF;
    if (s_BombOut)
      return CTRL_C_INTERRUPT;
      
    if (s_CommandMode & Command_EscapeAll) {
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
            UpdateJournal(NULL, "\e<<DEL>>"); } }
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
      if ( (Chr == ASCII_Escape) || ( (0 <= Chr) && (Chr < ' ') && (s_CommandMode & Command_EscapeCtrl) ) || (s_CommandMode & Command_EscapeAll) ) { //Initiate an escape-sequence search.
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
          if (RecordLength < ByteCount++)
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
          if (s_CommandMode & Command_OverTypeMode) {
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
        if (RecordLength < ByteCount++)
          return 1;
        break; }

    case '\n':
    case '\r':
      //End of command line detected.
      if (ScreenMode) { //There are typed-into-screen characters insert them to s_CurrentBuf then exit - deal with the escape sequence next time around.
        int Overtype = s_CommandMode & Command_OverTypeMode;
        s_CurrentBuf->SubstringLength = Overtype ? ByteCount : 0;
        SubstituteString(s_CurrentBuf, CommandText, ByteCount);
        s_CurrentBuf->CurrentByte += ByteCount;
        s_CurrentBuf->LineNumber--;
        s_CurrentBuf->SubstringLength = 0;
        BreakRecord(s_CurrentBuf);
        if (s_JournalHand) {
          s_JournalDataLines = 0;
          CommandText[ByteCount] = '\0';
          UpdateJournal(CommandText, "\e<<BRK>>"); }
        ByteCount = 0; }
      else {
        CommandText[ByteCount] = '\0';
        if (s_JournalHand) {
          s_JournalDataLines = 0;
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
  int ScreenMode = (s_CommandMode & Command_ScreenMode);
  char *CommandText = DestBuf->CurrentRec->text;
  struct Buf *TranslationBuf;
  int Parameter = FALSE;          //TRUE if translated string contains ## - response to prompt follows escape string.
  char *TransText;                //Translated command text from ^ buffer.
  char ThisChr;
  int TransFailed = FALSE;
     
  if (s_CommandMode & Command_ToggleScreen)
    s_CommandMode ^= (Command_ToggleScreen | Command_ScreenMode);
  if (ChrPointer && ScreenMode) { //There are typed-into-screen characters insert them to s_CurrentBuf then deal with the escape sequence.
    if (s_CommandMode & Command_OverTypeMode) {
      int RightSubstringLen = strlen(s_CurrentBuf->CurrentRec->text) - s_CurrentBuf->CurrentByte;
      s_CurrentBuf->SubstringLength = RightSubstringLen < strlen(CommandText) ? RightSubstringLen : strlen(CommandText); }
    else
      s_CurrentBuf->SubstringLength = 0;
    SubstituteString(s_CurrentBuf, CommandText, ChrPointer);
    s_CurrentBuf->CurrentByte += ChrPointer;
    s_CurrentBuf->SubstringLength = 0;
    if (s_JournalHand)
      UpdateJournal(CommandText, "\e<<INS>>");
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
             
        if (s_JournalHand) {
          EscapeString[EscapeSequenceLength] = '\0';
          UpdateJournal(CommandText, EscapeString); }
        Parameter = TRUE;
        DestBuf->CurrentByte = ChrPointer;
        SubstituteString(DestBuf, TransText+PromptEnd, -1);           //Suffix
        TranslationBuf->SubstringLength = Index-EscapeSequenceLength;
        break; } }
    
    if (s_JournalHand && ! Parameter) {
      EscapeString[EscapeSequenceLength] = '\0';
      UpdateJournal(CommandText, EscapeString); }
    DestBuf->CurrentByte = 0;
    DestBuf->SubstringLength = 0;
    SubstituteString(DestBuf, TransText + EscapeSequenceLength, PromptStart - EscapeSequenceLength);  //Prefix
    TranslationBuf->SubstringLength = 0;
    ChrPointer = strlen(CommandText);
    } // Translations-Match-found block ends.
  
  DestBuf->SubstringLength = 0;
  DestBuf->CurrentByte = 0;
  DestBuf->SubstringLength = 0;
  CommandText[ChrPointer] = '\0';
  if (asConsole && s_RecoveryMode) //Suck out any trailing whitespace and the '\n' from the recovery file.
    while ( (Chr = JotGetCh(FileHandle) == ' ') )
      { }
  return asConsole ? TransFailed : FALSE; }

//----------------------------------------------Ctrl_C_Interrupt
#if defined(VC)
BOOL Ctrl_C_Interrupt( DWORD fdwCtrlType ) 
  { //Ctrl+C handler for windows.
  if (fdwCtrlType == CTRL_C_EVENT) { // Handle the CTRL-C signal. 
    if (s_BombOut) {
      Message(NULL, "Unanswered Ctrl+C interrupt");
      return FALSE; }
    s_CommandMode = 0;
    s_Verbose |= 1;
    s_Interrupt = TRUE;
    if (s_TraceMode & Trace_Interrupt) {
      int OrigTraceMode = s_TraceMode;
      s_TraceMode = s_DefaultTraceMode;
      s_TraceMode |= Trace_AllCommands;
      s_TraceMode &= (-1 ^ Trace_Interrupt);
      if (s_JournalHand) {
        char Temp[20];
        sprintf(Temp, "<<Debugger on>> %d", s_JournalDataLines);
        UpdateJournal(Temp, NULL); }
      Message(NULL, "Command count reached %lld", s_CommandCounter);
      if (s_DebugLevel) {
        s_DebugLevel = 0;
        RunError("Exiting debugger now.");
        return FALSE; }
      JotTrace("{Ctrl_C}");
      JotBreak("Ctrl+C interrupt");
      s_TraceMode = OrigTraceMode; }
    if (s_JournalHand) {
      char Temp[100];
      sprintf(Temp, "\e<<INT %lld>> %d", s_CommandCounter, s_JournalDataLines);
      UpdateJournal(NULL, Temp); } }
      
  if (s_Verbose & Verbose_ReportCounter)
    RunError("Interrupt detected - command count is %lld", s_CommandCounter);
  else
    RunError("Interrupt detected");
  return TRUE; }

#elif defined(LINUX)
void Ctrl_C_Interrupt(int sig)
  { // Sets flag for control+c interrupt. 
  s_CommandMode = 0;
  s_Verbose |= 1;
  s_Interrupt = TRUE;
  if (sig == SIGINT) {
    sigrelse(sig);
    if (s_BombOut) {
      Message(NULL, "Unanswered Ctrl+C interrupt");
      if (s_RestartPoint)
        longjmp(*s_RestartPoint, 1);
      else
        Disaster("Restart point not set"); }
    if (s_JournalHand) {
      char Temp[100];
      sprintf(Temp, "\e<<INT %lld>> %d", s_CommandCounter, s_JournalDataLines);
      UpdateJournal(NULL, Temp); }
    if (s_Verbose & Verbose_ReportCounter)
      RunError("Interrupt detected - command count is %lld", s_CommandCounter);
    if (s_TraceMode & Trace_Interrupt) {
      int OrigTraceMode = s_TraceMode;
      s_TraceMode = s_DefaultTraceMode;
      s_TraceMode |= Trace_AllCommands;
      s_TraceMode &= (-1 ^ Trace_Interrupt);
      if (s_JournalHand) {
        char Temp[20];
        sprintf(Temp, "<<Debugger on>> %d", s_JournalDataLines);
        UpdateJournal(Temp, NULL); }
      if (s_DebugLevel) {
        s_DebugLevel = 0;
        RunError("Exiting debugger now.");
        return; }
      JotTrace("Ctrl_C");
      JotBreak("Ctrl+C interrupt");
      s_Interrupt = FALSE;
      refresh();
      s_TraceMode = OrigTraceMode; }
    else
      RunError("Interrupt detected"); }
  else
    RunError("Unknown signal detected"); 
  return; }
#endif

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
    memcpy(Ptr, Trans, TransLen); } }

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
  if ( ! strlen(Actual)) { //When no destination is specified, skip pathname matching and just use the PathName as-is.
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
          if ( ! strlen(TempName))
            TempName = NULL;
          TempPath = Copy[Pass]; }
        else { //If no path separators ( / ) then assume it's just a file name.
          TempPath = NULL;
          TempName = (0 < strlen(Copy[Pass])) ? Copy[Pass] : NULL; }
        if (TempName && (TempExtn = strrchr(TempName, '.'))) {
          TempName[++TempExtn-TempName-1] = '\0';
          if ( ! strlen(TempName))
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
int NewCommandFile(FILE *File, char *Name)
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
  NewBuf->Predecessor = s_CurrentCommand;
  NewFile->CommandBuf = NewBuf;
  GetRecord(NewBuf, StringMaxChr);
  memset(NewBuf->CurrentRec->text, '\0', StringMaxChr);
  NewBuf->SubstringLength = 0;
  NewBuf->LineNumber = 1;
  s_EditorInput = NewFile;
  return TRUE; }

//---------------------------------------------EndCommandFile
int EndCommandFile()
  { //Deletes CommandFile structure for current input file.
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
  free(s_EditorInput);
  s_EditorInput = OldComFile;
  return ReturnValue; }

//---------------------------------------------SwitchToComFile
int SwitchToComFile(char *NameString, char *asConsole, char *Args)
  { 
  //
  //  Parses and checks file name substituting undefined bits with default
  //  startup file name (defined by s_DefaultComFileName or s_DefaultComFileName,
  //  then searches the users default directory for the specified file
  //  if the file is not there then searches the ${JOT_HOME}/coms directory.
  //
  FILE *File = NULL;
  int Status = 0;
  struct BacktraceFrame ThisBacktraceFrame;
  char Actual[StringMaxChr];
  struct Buf *ArgsBuf;
  int IsDollar = s_CurrentBuf->BufferKey == '$';
  char RecoveryPathName[StringMaxChr];
  
  Actual[0] = '\0';
  CrossmatchPathnames(NULL, NULL, "r", NameString, s_DefaultComFileName, NULL, Actual);
  if (s_RecoveryMode) { //In recovery mode prompt then read the journal-area pathname.
    Prompt("Enter recovery-substitution file for %s", NameString ? s_TempBuf->CurrentRec->text : NameString);
    if (fgets(RecoveryPathName, StringMaxChr, s_asConsole->FileHandle)) {
      RecoveryPathName[strlen(RecoveryPathName)-1] = '\0';
      Message(NULL, "Using recovery-substitution \"%s\"", RecoveryPathName); }
    else {
      RunError("Failed to read recovery-substitution pathname");
      return 1; }
    CrossmatchPathnames(NULL, &File, "r", NULL, NULL, NULL, RecoveryPathName);
    if ( ! File) {
      RunError("Error opening recovery-substitution file \"%s\"", RecoveryPathName);
      return 1; } }
  else {
    char Temp[StringMaxChr+40];
    CrossmatchPathnames(NULL, &File, "r", NULL, NULL, NULL, Actual);
    if ( ! File) {
      Fail("Can't find jot command script.");
      return 1; }
    if (s_JournalHand) {
      FILE *CopyHand, *OrigHand;
      char ArchivePath[StringMaxChr];
      struct stat Stat;
      char DateTimeStamp[20];
      char *NameExtnElem = strrchr(Actual, '/');
      
      if ( ! stat(Actual, &Stat))
        strftime(DateTimeStamp, 20, "%Y%m%d_%H%M%S", localtime(&Stat.st_mtime));
      sprintf(Temp, "%s_%s", NameExtnElem ? NameExtnElem+1 : Actual, DateTimeStamp);
      strcpy(ArchivePath, s_JournalPath);
      strcat(ArchivePath, "/");
      strcat(ArchivePath, Temp);
      if ( ! IsAFile(ArchivePath)) { //journal archive of this version does not exist, create one now.
        CrossmatchPathnames(NULL, &CopyHand, "w", NULL, NULL, NULL, ArchivePath);
        if ( ! CopyHand)
          Disaster("Failed to open journal file %s", ArchivePath);
        else {
          if ( ! (OrigHand = fopen(Actual, "r")))
            RunError("Failed to open file %s for copying to journal archive", Actual);
          while (fgets(Temp, StringMaxChr, OrigHand))
            fputs(Temp, CopyHand);
          fclose(CopyHand);
          fclose(OrigHand); } }
      sprintf(Temp, "<<Recovery pathname: %s>>", ArchivePath);
      UpdateJournal(Temp, NULL); } }
      
  if ( ! (ArgsBuf = GetBuffer('$', AlwaysNew))) {
    Fail("Can't copy args to $ buffer.");
    return 1; }
  if (IsDollar)
    s_CurrentBuf = ArgsBuf;
  GetRecord(ArgsBuf, strlen(Args)+1);
  strcpy(ArgsBuf->FirstRec->text, Args);
     
  if (s_Verbose & Verbose_NonSilent)
    Message(NULL, "Running command file \"%s\"", Actual);
  NewCommandFile(File, Actual);
  s_EditorInput->asConsole = (asConsole != NULL); 
  if (asConsole) {
    if (s_asConsole) {
      Fail("Attempt to nest -asConsole scripts"); 
      return 1; }
    s_asConsole = s_EditorInput; }
  s_BacktraceFrame->LastCommand = s_CurrentCommand;
  s_CommandBuf =  asConsole ? s_EditorConsole->CommandBuf : s_EditorInput->CommandBuf;
  ThisBacktraceFrame.LastCommand = NULL;
  ThisBacktraceFrame.prev = s_BacktraceFrame;
  ThisBacktraceFrame.type = ScriptFrame;
  ThisBacktraceFrame.EditorInput = s_EditorInput;
  s_BacktraceFrame = &ThisBacktraceFrame;
  if (asConsole) { //-asConsole scripts exit on Ctrl+C
    while( ! s_Interrupt && ! feof(s_EditorInput->FileHandle))
      InteractiveEditor();
      
    Status = 0; }
  else {
    for (;;) { // Command-file-record loop. 
      struct Seq *ComFileSequence = NULL;
      
      s_EditorInput->LineNo++;
      if ( ! ReadString(s_CommandBuf->CurrentRec->text, s_CommandBuf->CurrentRec->length, s_EditorInput->FileHandle))
        break;
      if (s_BombOut) {
        if (s_Verbose & Verbose_QuiteChatty)
          Message(NULL, "Exiting script \"%s\", line %d \"%s\"", Actual, s_EditorInput->LineNo, s_CommandBuf->CurrentRec->text);
        break; }
         
      s_CommandBuf->CurrentByte = 0;
      
      ComFileSequence = (struct Seq *)JotMalloc(sizeof(struct Seq));
      JOT_Sequence(s_CommandBuf, ComFileSequence, FALSE);
      s_TraceLevel++;
      Status = Run_Sequence(ComFileSequence);
      s_TraceLevel--;
      if (Status && ! s_BombOut) {
        if (s_EditorInput->asConsole) //Forgive failing lines in -asConsole scripts.
          Message(NULL, "Command-file failure, line %d of \"%s\"", s_EditorInput->LineNo, Actual);
        else {
          Fail("Command-file failure, line %d of \"%s\"", s_EditorInput->LineNo, Actual);
          FreeSequence(&ComFileSequence);
          break; } }
      FreeSequence(&ComFileSequence);
      s_BacktraceFrame->LastCommand = NULL;
      if (s_Return) {
        s_Return = FALSE;
        s_ExitBlocks = 0; } } }
          
  s_BacktraceFrame = ThisBacktraceFrame.prev; 
  s_CurrentCommand = s_BacktraceFrame->LastCommand;
  s_BacktraceFrame->LastCommand = NULL;
  if (asConsole)
    s_asConsole = NULL;
  if ( ! EndCommandFile())
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
  int Chrs = 0, ByteCount = 0;
   
#if defined(VC)
  int CodePage = s_CurrentBuf->CodePage;
  Chrs = MultiByteToWideChar(CodePage, 0, String, Bytes, NULL, 0);
     
  return Chrs;
#else
  if (Bytes) {
    int ChrBytes;
    do {
      ChrBytes = mblen(String+ByteCount, MB_CUR_MAX);
      if (ChrBytes < 0)
        return -1;
      ByteCount += (ChrBytes <= 0) ? 1 : ChrBytes;
      Chrs++; }
      while (ChrBytes && ByteCount < Bytes); }
     
  //If the last byte is part-way through a multi-byte character the no. of characters should be reduced.   
  return Chrs+Bytes-ByteCount;
#endif
  }

//----------------------------------------------JotStrlenChrs
int JotStrlenChrs(char *String, int Chrs)
  { //Returns the number of bytes for String limited to Chrs characters - String may contain unicode, returns 0 in the event of invalid unicode.
  int TotalChrs = 0;
  int ByteCount = 0;
#if defined(VC)
  int CodePage = s_CurrentBuf->CodePage;
  int Length = strlen(String);
  
  while (TotalChrs < Chrs && String[ByteCount]) {
    if (Length <= ++ByteCount)
      return Length;
    TotalChrs = MultiByteToWideChar(CodePage, MB_ERR_INVALID_CHARS, String, ByteCount, NULL, 0); }
#else
  int MBLen;
 
  while (TotalChrs++ < Chrs && String[ByteCount]) {
    if (0 < (MBLen = mblen(String+ByteCount, MB_CUR_MAX)))
      ByteCount += MBLen;
    else
      return 0; }
#endif
  return ByteCount; }

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
  int LocalByteCount, LocalChrCount, Chrs;
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
  
//This might work?
//  *ChrCount = JotStrlenBytes(OrigString, MaxBytes);
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
      
  if (s_NoUnicode) { //Display any unicode bytes as tildes ( ~ ).
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
      
    //
    //addwstr() seems to behave inconsistently when filling a complete line of the screen - normally for, say, an 80-column term,
    //if x is initially 72 then write an 8-character string, it wraps around to the first chr of the next line.
    //But if x is initially 0, then writing an 80-chr string will set the current x to 79.
    //
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
   
  if (Right <= s_ChrNo-LeftOffset)
    return;
    
  StartXY.X = Left+s_ChrNo-LeftOffset;
  StartXY.Y = TermLine;
  Chrs = Right-Left-s_ChrNo+LeftOffset+1;
  FillConsoleOutputAttribute(hStdout, Attr_Normal, Chrs, StartXY, &ChrsWritten);
  FillConsoleOutputCharacter(hStdout, ' ', Chrs, StartXY, &ChrsWritten);
#elif defined(LINUX)
  
  if ( (s_ChrNo < LeftOffset) || (Right-Left <= s_ChrNo-LeftOffset) ) //Exit early s_ChrNo is outside range visible in this window.
    return;
    
  while (s_ChrNo+Left-LeftOffset <= Right) {
    s_ChrNo++;
    addch(' '); }
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
    Right = s_TermWidth; }
  if (Bot < Top) //This happens when some genius has defined a one-line window with a delimiter line.
    return;
  if (0 < Offset) {
    const SMALL_RECT StartXY = { (short)Left, (short)Top+Offset, (short)Right, (short)Bot };
    const COORD dest = { (short)Left, Top };
    const CHAR_INFO fill = { { ' ' }, Attr_Normal };
     
    ScrollConsoleScreenBuffer(hStdout, &StartXY, NULL, dest, &fill); }
  else {
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
  if (s_Verbose & Verbose_ReportCounter) {
    if (MaxLen < Chrs+15) {
      Chrs = MaxLen-20;
      strcpy(String+MaxLen-20, " ... "); }
    Chrs += sprintf(String+Chrs, " (%lld)", s_CommandCounter); }
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
    if ( (s_Verbose & Verbose_PrintLine) &&  s_CurrentBuf->FirstRec)
      DisplayDiag(s_CurrentBuf, TRUE, ""); }
  return 1; }

//----------------------------------------------Message
void Message(struct Buf *CommandBuf, char *FormatString, ...)
  { //Throws a new line, then uses prompt to write the string.
  va_list ap;
  char string[StringMaxChr];
   
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
  JotGotoXY(s_x, s_y); }

//----------------------------------------------JotDebug
void JotDebug(struct Buf *CommandBuf, char *FormatString, ...)
  { //As Message() except conditional on s_Verbose & Debug
  va_list ap;
  char string[StringMaxChr];
   
  if ( ! (s_Verbose & Verbose_DebugMessages) )
    return;
  va_start(ap, FormatString);
  vsnprintf(string, StringMaxChr, FormatString, ap);
  if (s_Verbose & Verbose_ReportCounter) {
    char Temp[20];
    sprintf(Temp, " (%lld)", s_CommandCounter);
    strcat(string, Temp); }
  if (CommandBuf)
    DisplayDiag(CommandBuf, TRUE, string);
  else {
    ScrollConsole(FALSE, FALSE);
    JotAddBoundedString(string, strlen(string), s_TermWidth, NULL, NULL, 0);
    JotGotoXY(s_x, s_y); } }

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
    SubstringLeft = CurrentByte;                           //The first byte of the next substring.
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
    if ( ! s_StreamOut) {
      if (SubstringLeft-FirstByte < 0) //This is an abnormally long substring - forget about it.
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
void JotTrace(char * Message)
  { //Handles reporting at a selected trace point.
  char TraceMessage[StringMaxChr]; 
  
  if (s_MaxTraceLevel) {
    if (s_MaxTraceLevel < s_TraceLevel) {
      s_TraceSkipped = TRUE;
      return; }
    if (s_TraceSkipped && (s_TraceLevel == s_MaxTraceLevel) )
      s_MaxTraceLevel = 0; }
  TraceMessage[0] = '\0'; 
  if (Message)
    strcpy(TraceMessage, Message);
    
  if (TraceMessage[0]) {
    int Orig_NonSilent = s_Verbose & Verbose_NonSilent;
    s_Verbose |= Verbose_NonSilent;
    if (s_TraceMode & Trace_Stack)
      DumpStack(NULL);
    if (s_Verbose & Verbose_ReportCounter) {
      char Temp[20];
      sprintf(Temp, " (%lld)", s_CommandCounter);
      strcat(TraceMessage, Temp); }
    if (s_TraceMode & Trace_Print)
      DisplayDiag(s_CurrentBuf, TRUE, "");
    if (s_TraceMode & Trace_Backtrace)
      Backtrace(NULL);
    if (s_TraceMode & Trace_Source)
      DisplaySource(s_CurrentCommand, TraceMessage);
    if (s_TraceMode & Trace_DumpSeq) {
      char Text[StringMaxChr];
      int Length = 0;
      struct Com *DumpCom = s_CurrentCommand;
      while (Length < 40 && DumpCom) {
        DumpCommand(DumpCom, &Length, Text);
        DumpCom = DumpCom->NextCommand; }
      Prompt(Text); }
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
  
  if ( ! s_EditorInput)
    return;
  asConsole = s_EditorInput->asConsole;
  s_EditorInput->asConsole = FALSE;
  s_DebugLevel++;
  JotUpdateWindow();
   
  for ( ; ; ) {
    int ExitNow = FALSE;
    struct Seq *DebugComSequence = NULL;
    s_BacktraceDone = FALSE;
    s_DebugBuf->CurrentRec->text[0] = '\0';
    s_BombOut = FALSE;
    JotUpdateWindow();
    if ( ( ! s_InView) && (s_Verbose & Verbose_NonSilent) )
      DisplayDiag(s_CurrentBuf, TRUE, "");
    s_BombOut = FALSE;
    s_DebugBuf->CurrentRec->text[0] = '\0';
    s_JournalDataLines++;
    ExitNow |= ReadCommand(
      s_DebugBuf, 
      ( s_RecoveryMode && (s_TraceMode & Trace_Recovery) ? s_EditorInput : s_EditorConsole)->FileHandle, 
      "(Debug %d) %d %c> ", 
      s_DebugLevel, 
      s_CurrentBuf->LineNumber, 
      s_CurrentBuf->BufferKey);
    if (s_BombOut) {
      if (s_JournalHand)
        UpdateJournal("<<Debugger off>>", NULL);
      s_TraceMode = Trace_Interrupt;
      break; }
    if (ExitNow)
      break;
    if (strlen(s_DebugBuf->CurrentRec->text) == 0)
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
    s_CurrentCommand = CurrentCommandSave;
    s_BacktraceFrame = ThisBacktraceFrame.prev;
    FreeSequence(&DebugComSequence);
    if (s_MaxTraceLevel) // The command was a s_traceskip - continue normally until s_MaxTraceLevel is satisfied.
      break; }
     
  s_DebugLevel--;
  s_CurrentCommand = CurrentCommandSave;
  s_EditorInput->asConsole = asConsole; }

//----------------------------------------------Disaster
void Disaster(char *Text, ...)
  { // Flags showstoppers detected at startup time.
  va_list ap;
  char string[StringMaxChr];
  
  va_start(ap, Text);
  vsnprintf(string, StringMaxChr, Text, ap);
  va_end(ap);
  JotTidyUp();
  fprintf(stderr, "\nDisaster: %s\n\r", string);
  if (s_CurrentCommand) {
    s_TTYMode = TRUE;
    Backtrace(NULL);
    fputc('\n', stderr); }
  exit(1); } 

//----------------------------------------------JotTidyUp
int JotTidyUp()
  { // Exit procedure called by TidyUp(). 
  int i = 0;
  struct Buf *ThisBuf = s_BufferChain;
  char BuffersToBeWritten[100] = "";
  struct Buf * DestBuf = NULL;
  struct Buf *ThisAsyncBuf = s_FirstAsyncBuf;
  char LockFilePathname[StringMaxChr];
  
  if (s_HoldScreen) {
    int Chr;
    Prompt("Hit C to continue, E to exit script or any other key to exit now ");
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
  
  while (ThisAsyncBuf) { //Close child pipes and terminat child processes
    struct Buf *NextAsyncBuf = ThisAsyncBuf->IOBlock->NextAsyncBuf;
    FreeBufferThings(ThisAsyncBuf);
    ThisAsyncBuf = NextAsyncBuf; }
  
  while (EndCommandFile())
    continue;
    
  if (s_StartupFileName)
    free(s_StartupFileName);
      
#if defined(VC)
  if ( ! s_TTYMode) { //Restore console.
    JotGotoXY(0, s_TermHeight-1);
    SetConsoleTextAttribute(hStdout, wOldColorAttrs); }
#elif defined(LINUX)
  if (mainWin) //Close down curses.
    endwin();
#endif
  
  if (s_OriginalSession && s_JournalHand) {
    strcpy(LockFilePathname, s_JournalPath);
    strcat(LockFilePathname, "/LOCK");
    if(unlink(LockFilePathname))
      printf("Failed to delete journal/backup file %s\n", LockFilePathname); }

  return 0; }

//----------------------------------------------DoHash
int DoHash(char *QueryString, char NomBufKey, int Operation, int Qual, int TextAsIs)
  { //Maintenance of hashtable entries in buffer nominated by NomBuf, defaults to s_CurrentBuf.
  char *Text;
  struct Buf *NomBuf = NULL;
  
  ExpandDeferredString(s_TempBuf, QueryString, TextAsIs);
  Text = s_TempBuf->CurrentRec->text;
  if (NomBufKey == '\0')
    NomBuf = s_CurrentBuf;
  else {
    if ( (NomBuf = GetBuffer(NomBufKey, (Operation == H_create) ? OptionallyNew : NeverNew)) == NULL)
      return 1; }
  if ( ! NomBuf->CurrentRec)
    GetRecord(NomBuf, 0);

  switch (Operation) {
  case H_create: { //create - initialize the table with specified no. of elements.
    int NElem = 0, status, Chrs;
    char *LastPathElem, Path[StringMaxChr];
    struct DataObj * ThisDataObj;
    
    Path[0] = '\0';
    Chrs = sscanf(Text+7, "%d %s", &NElem, (char *)&Path);
    if (Chrs == 0) {
      SynError(NULL, "No hash-table size specified");
      return 1; }
    if (Path[0] == '-' && Path[1] != '|') //It's a qualifier not a path.
      Path[0] = '\0';
    if (Path[0]) {
      if (! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem)) )
        return 1;
      if (LastPathElem[0]) {
        if ( ! (ThisDataObj = (struct DataObj *)QueryData(LastPathElem, NomBuf)) || ((struct bufFrame *)ThisDataObj->Frame)->type != FrameType_Buf) {
          Fail("Object \"%s\" in path \"%s\" is not a buffer.", LastPathElem, Path);
          return 1; }
        NomBuf = (struct Buf *)((struct bufFrame *)ThisDataObj->Frame)->BufPtr; } }
    if ( ! NomBuf->CurrentRec)
      GetRecord(NomBuf, 0);
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
    status = hcreate_r(NElem, NomBuf->htab);
    if (NomBuf->FirstObj) { //Restore all non-zombie objects in new hashtable.
      struct AnyObj *ThisObj = NomBuf->FirstObj;
      ENTRY *OldVal, NewVal;
      while (ThisObj) {
        if (ThisObj->type == ObjType_Zombie) { //A zombie - if it's got a NewObj then just take that one.
          struct AnyObj *NewObj = ((struct ZombieObj *)ThisObj)->NewObj;
          if ( ! NewObj) {
            ThisObj = ThisObj->next;
            continue; }
          ThisObj = NewObj; }
        NewVal.key = ThisObj->HashKey;
        NewVal.data = ThisObj;
        if ( ! hsearch_r(NewVal, ENTER, &OldVal, NomBuf->htab))
          return Fail("Failed to restore hashtable item %s - ", ThisObj->HashKey);
        ThisObj = ThisObj->next; } }
    return ( ! status); }

  case H_add:
  case H_new: { //add or new - puts item into table.
    ENTRY *OldVal, NewVal;
    struct JumpObj *Object = NULL;
    struct AnyObj *OldObj = NULL;
    char *KeyCopy;
    struct TargetTag *ThisTag;
    char *LastPathElem, *Path = Text+4;
    
    if (! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem)) )
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
      
    NewVal.key = LastPathElem;
    NewVal.data = NULL;
    if (hsearch_r(NewVal, FIND, &OldVal, NomBuf->htab)) { //A preexisting entry.
      Object = OldVal->data;
      OldObj = (struct AnyObj *)OldVal->data;
      if (Object) {
        if (OldObj->type == ObjType_Jump) { //The preexisting entry was a JumpObj.
          if (Operation == H_new) { //Collision detected while adding 'new' entry - restore original and fail.
            Fail("Failed create new item \"%s\" in hashtable of buffer %c", LastPathElem, NomBuf->BufferKey);
            return 1; } }
        else if (OldObj->type == ObjType_Zombie) //The preexisting entry is a zombie (was deleted).
          Object = NULL;
        else {
          Fail("Attempt to coerce some other type of hashtable object to a JumpObj");
          return 1; } } }
        
    if ( ! Object) {
      Object = (struct JumpObj *)JotMalloc(sizeof(struct JumpObj));
      if (OldObj) {
        struct AnyObj *OriginalSecondaryObj = ((struct ZombieObj *)OldObj)->NewObj;
        if (OriginalSecondaryObj)
          Zombify(NomBuf, (struct AnyObj *)OriginalSecondaryObj);
        ((struct ZombieObj *)OldObj)->NewObj = (struct AnyObj *)Object;
        Object->HashKey = OldObj->HashKey;
        OldObj->HashKey = NULL;
        Object->next = OldObj->next;
        OldObj->next = (struct AnyObj *)Object; }
      else {
        KeyCopy = (char *)JotMalloc(strlen(LastPathElem)+1);
        strcpy(KeyCopy, LastPathElem); 
        NewVal.key = KeyCopy;
        NewVal.data = (void *)Object;
        if( ! hsearch_r(NewVal, ENTER, &OldVal, NomBuf->htab)) {
          Fail("Failed to create data item \"%s\" in hashtable of buffer %c", KeyCopy, NomBuf->BufferKey);
          free(Object);
          free(KeyCopy);
          return 1; }
        Object->HashKey = KeyCopy;
        Object->next = NomBuf->FirstObj;
        NomBuf->FirstObj = (struct AnyObj *)Object; } }
    else //A preexisting object - this can only be  an old JumpObj - delete it's target.
      FreeTarget(NomBuf, Object);
      
    //Create TargetPoint tag for target record.
    ThisTag = (struct TargetTag *)JotMalloc(sizeof(struct TargetTag));
    ThisTag->next = NULL;
    ThisTag->StartPoint = s_CurrentBuf->CurrentByte;
    ThisTag->type = TagType_Target;
    ThisTag->Attr = (void *)Object;
    AddTag(s_CurrentBuf->CurrentRec, (struct AnyTag *)ThisTag);
    
    //Set up the JumpObj
    Object->type = ObjType_Jump;
    Object->HashBuf = NomBuf;
    Object->TargetBuf = s_CurrentBuf;
    Object->TargetRec = s_CurrentBuf->CurrentRec;
    Object->Target = ThisTag;
    Object->LineNumber = s_CurrentBuf->LineNumber;
    NomBuf->UnchangedStatus |= SameSinceIndexed;
    
    return 0; }

  case H_setfsect:  //setfsect - adds pathname, seek offset and byte count fo %i ... -fsection=...; N.B. does not consume the stack values.
  case H_setsect: { //setsect - adds seek offset and byte count for %i ... -section=...; N.B. does not consume the stack values.
    ENTRY *OldVal, NewVal;
    struct SetsectObj *Object = NULL, *OldObj = NULL;
    char *PathName;
    long long Seek, Bytes;
    char *KeyCopy;
    char *LastPathElem, *Path = Text+(Operation == H_setsect ? 8 : 9);
    int LocalFail = 0;
    
    if (! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem)) )
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
    if (s_StackPtr < ((Operation == H_setsect) ? 2 : 3)) {
      Fail("Insufficient entries for this operation.");
      return 1; }
      
    NewVal.key = LastPathElem;
    NewVal.data = NULL;
    if (hsearch_r(NewVal, FIND, &OldVal, NomBuf->htab)) {
      Object = OldObj = OldVal->data;
      if (OldObj && OldObj->type != ObjType_Zombie) { //Allow zombies.
        Fail("Hashtable in buffer %c already has an entry for key \"%s\"", NomBuf->BufferKey, NewVal.key);
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
      Object = (struct SetsectObj *)JotMalloc(sizeof(struct SetsectObj));
      Object->type = ObjType_Setsect; }
    else {
      Object = (struct SetsectObj *)JotMalloc(sizeof(struct SetfsectObj));
      Object->type = ObjType_Setfilesect; }
    if (OldObj) {
      struct AnyObj *OriginalSecondaryObj = ((struct ZombieObj *)OldObj)->NewObj;
      if (OriginalSecondaryObj)
        Zombify(NomBuf, (struct AnyObj *)OriginalSecondaryObj);
      ((struct ZombieObj *)OldObj)->NewObj = (struct AnyObj *)Object;
      Object->HashKey = OldObj->HashKey;
      OldObj->HashKey = NULL;
      Object->next = OldObj->next;
      OldObj->next = (struct AnyObj *)Object; }
    else {
      KeyCopy = (char *)JotMalloc(strlen(LastPathElem)+1);
      strcpy(KeyCopy, LastPathElem);
      Object->HashKey = KeyCopy;
      Object->next = NomBuf->FirstObj;
      NomBuf->FirstObj = (struct AnyObj *)Object;
      NewVal.key = KeyCopy;
      NewVal.data = Object;
      if ( ! hsearch_r(NewVal, ENTER, &OldVal, NomBuf->htab)) {
        Fail("Failed to create setsect item \"%s\" in hashtable of buffer %c", LastPathElem, NomBuf->BufferKey);
        if (Operation == H_setfsect)
          ((struct SetfsectObj *)Object)->PathName = NULL;
        Zombify(NomBuf, (struct AnyObj *)Object);
        ((struct ZombieObj *)Object)->NewObj = NULL;
        return 1; } }
    if (Operation == H_setfsect) {
      ((struct SetfsectObj *)Object)->PathName = NULL;
      if (s_SetfsectPathName) {
        char *CurrentPathName = s_SetfsectPathName;
        if (strlen(CurrentPathName) == strlen(PathName) && strstr(CurrentPathName, PathName) == CurrentPathName && s_SetfsectBuffer == NomBuf)
          ((struct SetfsectObj *)Object)->PathName = s_SetfsectPathName; }
      if ( ! ((struct SetfsectObj *)Object)->PathName) {
        ((struct SetfsectObj *)Object)->PathName = (char *)JotMalloc(strlen(PathName)+1);
        strcpy(((struct SetfsectObj *)Object)->PathName, PathName);
        s_SetfsectPathName = ((struct SetfsectObj *)Object)->PathName;
        s_SetfsectBuffer = NomBuf; } }
    Object->Bytes = Bytes;
    Object->Seek = Seek;
    return 0; }

  case H_data: { //data - creates an entry for stack-frame object.
    ENTRY *OldVal, NewVal;
    struct DataObj *Object = NULL, *OldObj = NULL;
    char *KeyCopy;
    char *LastPathElem, *Path = Text+5;
    
    if (! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem)) )
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
      
    NewVal.key = LastPathElem;
    NewVal.data = NULL;
    if (hsearch_r(NewVal, FIND, &OldVal, NomBuf->htab)) {
      Object = OldObj = OldVal->data;
      if (OldObj && OldObj->type != ObjType_Zombie) { //Allow zombies.
        Fail("Hashtable in buffer %c already has an entry for key \"%s\"", NomBuf->BufferKey, NewVal.key);
        return 1; } }
        
    Object = (struct DataObj *)JotMalloc(sizeof(struct DataObj));
    Object->type = ObjType_Data;
    Object->Frame = NULL;
    Object->ParentBuf = NomBuf;
    if (OldObj) {
      struct AnyObj *OriginalSecondaryObj = ((struct ZombieObj *)OldObj)->NewObj;
      if (OriginalSecondaryObj)
        Zombify(NomBuf, (struct AnyObj *)OriginalSecondaryObj);
      ((struct ZombieObj *)OldObj)->NewObj = (struct AnyObj *)Object;
      Object->HashKey = OldObj->HashKey;
      OldObj->HashKey = NULL;
      Object->next = OldObj->next;
      OldObj->next = (struct AnyObj *)Object; }
    else {
      KeyCopy = (char *)JotMalloc(strlen(LastPathElem)+1);
      strcpy(KeyCopy, LastPathElem); 
      Object->HashKey = KeyCopy;
      Object->next = NomBuf->FirstObj;
      NomBuf->FirstObj = (struct AnyObj *)Object;
      NewVal.key = KeyCopy;
      NewVal.data = (void *)Object;
      if ( ! hsearch_r(NewVal, ENTER, &OldVal, NomBuf->htab)) {
        Fail("Failed to create data item \"%s\" in hashtable of buffer %c", KeyCopy, NomBuf->BufferKey);
        Zombify(NomBuf, (struct AnyObj *)Object);
        return 1; } }
    return 0; }

  case H_jump: { //jump - looks up item in table and changes context.
    struct Buf *TargetBuf;
    char *LastPathElem, *Path = Text+5;
    
    if ( ! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem)) )
      return 1;
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
      
    TargetBuf = QueryKey(LastPathElem, NomBuf);
    if (TargetBuf)
      s_CurrentBuf = TargetBuf;
    return TargetBuf == NULL; }

  case H_call: { //call - compiles and runs specified routine.
    struct Buf *CodeRepositoryBuf;
    struct Seq *DeferredSeq;
    struct BacktraceFrame ThisBacktraceFrame;
    int CallStatus;
    
    if (NomBufKey == '\0')
      NomBuf = s_CurrentBuf;
    else if ( ! (NomBuf = GetBuffer(NomBufKey,  NeverNew)))
      return 1;  
    if ( ! (CodeRepositoryBuf = QueryKey(Text+5, NomBuf))) {
      Fail("Function \"%s\" not found in code repository ( %c )", Text+5, NomBuf->BufferKey);
      return 1; }
    //Named routine exists - compile it and run.
   
    if (CodeRepositoryBuf == NULL) {
      Fail("Undefined buffer");
      return 0; }
    CodeRepositoryBuf->CurrentRec = CodeRepositoryBuf->CurrentRec->next;
    CodeRepositoryBuf->SubstringLength = 0;
    CodeRepositoryBuf->CurrentByte = 0;
     
    DeferredSeq = (struct Seq *)JotMalloc(sizeof(struct Seq));
    s_BacktraceFrame->LastCommand = s_CurrentCommand;
    ThisBacktraceFrame.LastCommand = NULL;
    ThisBacktraceFrame.prev = s_BacktraceFrame;
    ThisBacktraceFrame.FirstRec = CodeRepositoryBuf->CurrentRec->next;
    ThisBacktraceFrame.type = CallFrame;
    JOT_Sequence(CodeRepositoryBuf, DeferredSeq, TRUE);
    if (s_BombOut)
      Message(CodeRepositoryBuf, "Exiting macro %c, line no %d", CodeRepositoryBuf->BufferKey, CodeRepositoryBuf->LineNumber);
    if (s_CommandChr == ')') {
      SynError(CodeRepositoryBuf, "Surplus \')\'");
      free(DeferredSeq);
      return 1; }
    s_BacktraceFrame = &ThisBacktraceFrame;
    s_TraceLevel++;
    CallStatus = Run_Sequence(DeferredSeq);
    if (CallStatus && (s_TraceMode & Trace_FailMacros) ) {
      s_BacktraceFrame->LastCommand = s_CurrentCommand;
      JotTrace("Function failure."); }
    s_TraceLevel--;
    s_BacktraceFrame = ThisBacktraceFrame.prev;
    FreeSequence(&DeferredSeq);
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
          ((struct JumpObj *)ThisTag->Attr)->LineNumber = LineNumber;
        ThisTag = ThisTag->next; }
      Record = Record->next;
      LineNumber++; }
      while (Record != NomBuf->FirstRec);
    
    return 0; }

  case H_delete: { //delete - removes item from the table.
    char *LastPathElem, Path[StringMaxChr];
    ENTRY *Ptr, Entry;
    
    Path[0] = '\0';
    sscanf(Text+6, "%s", (char *)&Path);
    if (Path[0]) {
      if (! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem)) )
        return 1; }
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
    Entry.key = LastPathElem;
    Entry.data = NULL;
  
    if ( ! hsearch_r(Entry, FIND, &Ptr, NomBuf->htab)) {
      Fail("Failed to find item \"%s\" in hashtable of buffer %c", LastPathElem, NomBuf->BufferKey);
      return 1; }
    else {
      struct AnyObj *Object = (struct AnyObj *)Ptr->data;
      Zombify(NomBuf, Object);
      return 0; } }

  case H_testkey: { //testkey - verify that there is at least one key matching the given string,
    char *LastPathElem, *Path = Text+8;
    
    if (Path[0]) {
      if (! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem)) )
        return 1; }
    if ( ! NomBuf->htab) {
      Fail("No hash-table associated with buffer %c", NomBuf->BufferKey);
      return 1; }
      
    if (NomBuf->htab) {
      struct JumpObj *ThisJumpObj = (struct JumpObj *)NomBuf->FirstObj;
    
      while (ThisJumpObj) {
        if (ThisJumpObj->HashKey && strstr(ThisJumpObj->HashKey, Path))
          return 0;
        ThisJumpObj = (struct JumpObj *)ThisJumpObj->next; }
      return 1; }
    else
      return 1; }

  case H_destroy: { //destroy - frees the hashtable and unprotects records.
    if (Qual == H_allQual) {
      struct Buf *ThisBuf = s_BufferChain;
      while (ThisBuf) {
        DestroyHtab(ThisBuf);
        ThisBuf = ThisBuf->NextBuf; } }
    else {
      char *LastPathElem, *Path = Text+8;
      
      if (! (NomBuf = QueryPath(Path, NomBuf, &LastPathElem)) )
        return 1;
      DestroyHtab(NomBuf); }
    return 0; }
  
  default: //Can't happen.
    RunError("Unrecognized %H qualifier %s", Text); 
    return 1; }
    
  return 1; }

//----------------------------------------------DestroyHtab
void DestroyHtab(struct Buf *TextBuf)
  { //Destroys the specified h-list and it's children.
  struct AnyObj *ThisAnyObj;
  
  if (TextBuf->htab) {
    char *PendingPathName = NULL;
    
    ThisAnyObj = TextBuf->FirstObj;
    while(ThisAnyObj) {
      struct AnyObj *NextObj = (struct AnyObj *)ThisAnyObj->next;
      if (ThisAnyObj->type == ObjType_Setfilesect || (ThisAnyObj->type == ObjType_Zombie && ((struct ZombieObj *)ThisAnyObj)->WasSetFSectObj) ) { 
        //A setfsect object, or a zombie setfsect - free the pathName buffer whenever this is a new pathname.
        char *ThisPathName = ((struct SetfsectObj *)ThisAnyObj)->PathName;
        if (ThisPathName != PendingPathName) {
          if (PendingPathName)
            free(PendingPathName);
          PendingPathName = ThisPathName; } }
      if (ThisAnyObj->HashKey)
        free(ThisAnyObj->HashKey);
      Zombify(TextBuf, ThisAnyObj);
      ThisAnyObj = NextObj; }
    if (PendingPathName) {
      free(PendingPathName);
      PendingPathName = NULL; }
      
    //Zombify() frees the objects children but the actual object is only set to be a zombie - this next loop frees these zombies.
    ThisAnyObj = TextBuf->FirstObj;
    while(ThisAnyObj) {
      struct AnyObj *NextObj = (struct AnyObj *)ThisAnyObj->next;
      free(ThisAnyObj);
      ThisAnyObj = NextObj; }
      
    hdestroy_r(TextBuf->htab);
    free(TextBuf->htab);
    TextBuf->htab = NULL;
    TextBuf->FirstObj = NULL;
    TextBuf->HashtableMode = NoHashtable;
    TextBuf->UnchangedStatus &= !SameSinceIndexed;
    
    s_SetfsectPathName = NULL; } }

//----------------------------------------------Zombify
void Zombify(struct Buf *TextBuf, struct AnyObj *ThisObj)
  { //Frees the objects children and zombifies the object.
  if(ThisObj) {
    if (ThisObj->type == ObjType_Jump) { //A find object.
      struct Buf *TargetBuf = ((struct JumpObj *)ThisObj)->TargetBuf;
      FreeTarget(TargetBuf, (struct JumpObj *)ThisObj);
      ((struct JumpObj *)ThisObj)->TargetRec = NULL; }
    else if (ThisObj->type == ObjType_Data) { //A Data object.
      struct bufFrame *Frame = (struct bufFrame *)((struct DataObj *)ThisObj)->Frame;
      if (Frame && Frame->type == FrameType_Buf) { //A buffer frame - delete the buffer.
        struct Buf *Buffer = Frame->BufPtr;
        Buffer->ExistsInData = FALSE;
        Buffer->ParentObj = NULL;
        if (Buffer != s_CurrentBuf && ! SearchStackForBuf(Buffer) )
          FreeBuffer(Buffer); }
      free(((struct DataObj *)ThisObj)->Frame);
      ((struct DataObj *)ThisObj)->Frame = NULL;
      ((struct ZombieObj *)ThisObj)->NewObj = NULL; }
    if (ThisObj->type == ObjType_Setfilesect) {
      ((struct ZombieObj *)ThisObj)->NewObj = NULL;
      ((struct ZombieObj *)ThisObj)->WasSetFSectObj = TRUE; }
    else if (ThisObj->type == ObjType_Zombie)
      return;
    else {
      if (ThisObj->type == ObjType_Setsect) 
        ((struct ZombieObj *)ThisObj)->NewObj = NULL;
      ((struct ZombieObj *)ThisObj)->WasSetFSectObj = FALSE; }
    ThisObj->type = ObjType_Zombie;
    return; } }

//----------------------------------------------FreeTarget
void FreeTarget(struct Buf *TextBuf, struct JumpObj *ThisJumpObj)
  { //Removes links to Hash-table target tag and free's it.
  struct Rec *Record = ThisJumpObj->TargetRec;
  
  if (Record) {
    struct AnyTag **PrevTagPtr = &(Record->TagChain);
    struct AnyTag *ThisTag = *PrevTagPtr;
    
    while (ThisTag) {
      if ( (ThisTag->type == TagType_Target) && (ThisTag->Attr == ThisJumpObj) ) {
        *PrevTagPtr = ThisTag->next;
        ThisJumpObj->Target = NULL;
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
        ((struct JumpObj *)ThisTag->Attr)->TargetRec = NULL;
      free(TheTag);
      return; }
    PrevTagPtr = &(ThisTag->next);
    ThisTag = *PrevTagPtr; } }

//----------------------------------------------QueryKey
struct Buf * QueryKey(char *Key, struct Buf *Buffer)
  { //Queries the hashtable in the specified buffer using the specified key. Returns target buffer or NULL if failure.
  ENTRY *Ptr;
  ENTRY Entry;
  struct Buf *TargetBuf = NULL;
   
  if ( ! Buffer->htab) {
    Fail("No hash-table associated with buffer %c", Buffer->BufferKey);
    return TargetBuf; }
  Entry.key = Key;
  Entry.data = NULL;
  
  if ( ! hsearch_r(Entry, FIND, &Ptr, Buffer->htab))
    Fail("Failed to find item \"%s\" in buffer %c hashtab", Key, Buffer->BufferKey);
  else {
    struct JumpObj *Object = (struct JumpObj *)Ptr->data;
    if (Object) {
      if (Object->type == ObjType_Zombie) //A zombie - it may have a NewObj set though?
        if ( ! (Object = (struct JumpObj *)((struct ZombieObj *)Object)->NewObj) )
          return NULL;
      if (Object && (Object->type == ObjType_Jump) ) { //A genuine JumpObj
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

//----------------------------------------------QueryPath
struct Buf *QueryPath(char *Path, struct Buf *DefaultBuf, char ** LastPathElem)
  { //Checks and descends the hashtable hierarchy specified by Path and returns the buffer holding the final entry.
  char *PathElem = Path, *PathElemEnd, PathElemCopy[StringMaxChr];
  struct DataObj *ThisDataObj;
  struct Buf *ThisBuf;
  
  if (Path[0] && Path[1] == '=') { //Path[0] is the primary buffer key.
    if ( ! (ThisBuf = GetBuffer(ChrUpper(Path[0]), OptionallyNew)) )
      return NULL;
    PathElem = Path+2; }
  else
    ThisBuf = DefaultBuf;
    
  while ( (PathElemEnd = strchr(PathElem, '|')) ) { //Path-element loop.
    strncpy(PathElemCopy, PathElem, PathElemEnd-PathElem);
    PathElemCopy[PathElemEnd-PathElem] = '\0';
    if ( ! (ThisDataObj = QueryData(PathElemCopy, ThisBuf)) )
      return NULL;
    if (ThisDataObj->type != ObjType_Data) {
      Fail("Incorrect hashtable entry, at path element \"%s\", in path \"%s\"", PathElemCopy, Path);
      return(NULL); }
    ThisBuf = ((struct bufFrame *)ThisDataObj->Frame)->BufPtr;
    PathElem = PathElemEnd+1; }
    
  *LastPathElem = PathElem;
  return ThisBuf; }

//----------------------------------------------QueryData
struct DataObj * QueryData(char *Key, struct Buf *Buffer)
  { //Queries the hashtable in the specified hashtable path, checks that it's a DataObj and returns the struct.
  ENTRY *Ptr;
  ENTRY Entry;
   
  if ( ! Buffer->htab) {
    Fail("No hash-table associated with buffer %c", Buffer->BufferKey);
    return NULL; }
  Entry.key = Key;
  Entry.data = NULL;
  
  if ( ! hsearch_r(Entry, FIND, &Ptr, Buffer->htab))
    Fail("Failed to find item \"%s\" in hashtable of buffer %c", Key, Buffer->BufferKey);
  else {
    struct DataObj *Object;
    if ( ! Ptr->data) {
      Fail("No entry associated with key \"%s\" in hashtable of buffer %c", Key, Buffer->BufferKey);
      return NULL; }
    Object = (struct DataObj *)Ptr->data;
    if (Object->type == ObjType_Zombie) //A zombie - it may have a NewObj set though
      if ( ! (Object = (struct DataObj *)((struct ZombieObj *)Object)->NewObj) )
        return NULL;
    if (Object->type == ObjType_Data) //A DataObj
      return Object; }
  return NULL; }

//----------------------------------------------QuerySection
struct SetsectObj * QuerySection(char *Key, struct Buf *Buffer)
  { //Queries the hashtable in the specified buffer using the specified key, checks that it's a SetsectObj and returns struct.
  ENTRY *Ptr;
  ENTRY Entry;
   
  if ( ! Buffer->htab) {
    Fail("No hash-table associated with buffer %c", Buffer->BufferKey);
    return NULL; }
  Entry.key = Key;
  Entry.data = NULL;
  
  if ( ! hsearch_r(Entry, FIND, &Ptr, Buffer->htab))
    Fail("Failed to find item \"%s\" in hashtable of buffer %c", Key, Buffer->BufferKey);
  else {
    struct SetsectObj *Object = (struct SetsectObj *)Ptr->data;
    if (Object->type == ObjType_Zombie) //A zombie - it may have a NewObj set though
      if ( ! (Object = (struct SetsectObj *)((struct ZombieObj *)Object)->NewObj) )
       return NULL;
    if (Object->type == ObjType_Setsect) //A SetsectObj
      return Object;
    else
      Fail("Item \"%s\" in hashtable of buffer %c is not a section type.", Key, Buffer->BufferKey); }
  return NULL; }

//----------------------------------------------QueryFSection
struct SetfsectObj * QueryFSection(char *Key, struct Buf *Buffer)
  { //Queries the hashtable in the specified buffer using the specified key, checks that it's a SetfsectObj and returns struct.
  ENTRY *Ptr;
  ENTRY Entry;
   
  if ( ! Buffer->htab) {
    Fail("No hash-table associated with buffer %c", Buffer->BufferKey);
    return NULL; }
  Entry.key = Key;
  Entry.data = NULL;
  
  if ( ! hsearch_r(Entry, FIND, &Ptr, Buffer->htab))
    Fail("Failed to find item \"%s\" in hashtable of buffer %c", Key, Buffer->BufferKey);
  else {
    struct SetfsectObj *Object = (struct SetfsectObj *)Ptr->data;
    if (Object->type == ObjType_Zombie) //A zombie - it may have a NewObj set though
      if ( ! (Object = (struct SetfsectObj *)((struct ZombieObj *)Object)->NewObj) )
       return NULL;
    if (Object->type == ObjType_Setfilesect) //A SetfsectObj
      return Object;
    else
      Fail("Item \"%s\" in hashtable of buffer %c is not a section type.", Key, Buffer->BufferKey); }
  return NULL; }
