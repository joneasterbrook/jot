  the              find     command           will           normally  begin  it's  search  at  the  current
  character.
  
  the              find     command           will           normally  begin  it's  search  at  the  current
  character.

  the              find     command           will           normally  begin  it's  search  at  the  current
  character.
 
  the              find     command           will           normally  begin  it's  search  at  the  current
  character.
   
  the              find     command           will           normally  begin  it's  search  at  the  current
  character.
    command you can also reverse
  the              find     command           will           normally  begin  it's  search  at  the  current
  character.
    - abc def ghi
   
The above should all be identified as separate paragraphs.
----------------------------------------------------------

This is an ordinary hyphan and not a bullet point
-------------------------------------------------
  abc def ghi
  - 123 456

Should not insert an extra blank line after either of these single-line paragraph ending at exactly chr 78.
-----------------------------------------------------------------------------------------------------------
 
  OI[C|D|O|X|F] - formatted Input conversion (of string at current character).
  
    - A range of characters <ch1>-<ch2> in the ASCII/POSIX collating sequence.
  
These bullet points should remain as separate paragraphs.
---------------------------------------------------------
    - -split - splits up sections and moves contents into parent sections.
    - -index - Adds index at start of html (or each page if -split is also set).
    - -noLinks - skips link-resolution stage, this saves time for debugging, but
      the html is of little use.

Should not merge or corrupt the two lines in the followin:.
-----------------------------------------------------------
    - `hashtable find` 
  See also see `about hashtables` and `X`
   
    - [<string>]<<Substitute>> {F5} 
        Replaces currently-selected substring with the specified string, if no

Should not merge the 'See also' line with the bullet line.
----------------------------------------------------------
    - `hashtable find`
  See also see `about hashtables` and `X`

First line should read: 'command you can also reverse the status of a command that's already had'
-----------------------------------------------------------------------------------------------------
       command you   can also   reverse the   status of   a command   that's already had
       it's status reset by the  `?`  command  -  in  this  case  the  command     *always* fails - which is sometimes useful.
        
Should reformat both of these bullet paragraphs and the following paragraph properly - was skipping 2nd.
--------------------------------------------------------------------------------------------------------
    - the command to erase n characters erases n characters starting with  the
      current character and
    - the find  command  will  normally  begin  it's  search  at  the  current
      character.
  the find command will normally begin it's search at the current character.
    
The word 'trickery' should appear on the second line of the paragraph.
----------------------------------------------------------------------
  powerful as any yet with the absolute minimum of  magic  modes  and  similar
  trickery.
   
  This is a command-driven text editor with a good set of  primitive  commands
  and a simple (ECCE-like) command structure.  The  result  is  an  editor  as
  powerful as any yet with the absolute minimum of  magic  modes  and  similar
  trickery.
   
Should not attempt to insert any padding blanks in this one.
------------------------------------------------------------
  This is a command-driven text editor.
  
Should deal with UTF-8 unicode correctly
----------------------------------------
  3 :1234567890
  4 :Test file for £jonathans wonderful editor.
  4 :Test file for ££jonathans wonderful editor.
  4 :Test file for £££jonathans wonderful editor.
  4 :Test file for ££££££££££££££££jonathans wonderful editor.
  7 :qwertyuiop[]
  8 :asdgfghjkl;'
  9 :zxcvbnm,./
  10:<>\|~!@#$%^&*()_+-=

Should also deal with ISO-8859-x unicode
----------------------------------------
  The Java� programming language is a general-purpose, concurrent,

A nice uncomplicated, but longer paragraph.
-------------------------------------------
  By default no journal is maintained - the editor is not particularly  crashy
  and modern computers and power grids are generally pretty  reliable  -  even
  modern operating systems are not too bad - although  some,  maybe,  less  so
  than others. It is, however, pretty distressing to see hours of  work  wiped
  out by such an event.  If  the  crash  was  due  to  some  unfortunate  data
  sensitivity bug in the editor, you may need  to  edit  the  recovery  script
  before running it - the journal will, of course, replicate the very  command
  that caused all that grief the first time around.
