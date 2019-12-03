// hello.c
//
 
#include <stdio.h>
#include <sys/stat.h>
#include "level_3/level_3.c"

int level_2()
{
  printf("Level 2 In\n");
  level_3();
  printf("Level 2 Out\n");
  }

