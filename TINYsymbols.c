#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "TINY.h"
#include "TINYint.h"

#define SYM(s)        *(IXX_U32*)(s) 

static char ac8_symbol[][4] = {
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "IF\0\0",   
  "THEN", 
  "PRIN",
  "LET\0",  
  "GOTO", 
  "END\0",  
};

IXX_U8 TINY_SymGet(const char *pc8_in)
{
  for(int i = 0; i < sizeof(ac8_symbol)/4; i++)
  {
    if(SYM(pc8_in) == SYM(ac8_symbol[i]))
    {
      return i;
    }
  }
  return 0xFF;
}

IXX_U8 TINY_SymAdd(const char *pc8_in)
{
  for(int i = 0; i < sizeof(ac8_symbol)/4; i++)
  {
    if(SYM(ac8_symbol[i]) == 0)
    {
      memcpy(ac8_symbol[i], pc8_in, 4);
      return i;
    }
  }
  return 0xFF;
}

#ifdef TEST
int main(void)
{
  printf("%d\n",TINY_SymGet("IF\0\0"));
  printf("%d\n",TINY_SymGet("PRINT"));
  printf("%d\n",TINY_SymAdd("var\0"));
  printf("%d\n",TINY_SymAdd("var2"));
  printf("%d\n",TINY_SymAdd("var3"));
  printf("%d\n",TINY_SymGet("var\0"));
  printf("%d\n",TINY_SymGet("var2"));
  printf("%d\n",TINY_SymGet("var3"));

  return 0;
}
#endif