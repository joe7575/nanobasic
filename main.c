#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "TINY.h"
#include "TINYint.h"

int main(int argc, char* argv[])
{
  IXX_U8 buffer[256];
  IXX_U16 size;
#if 0
  char s[] = "BC0100040A00000004030000000904040000000706000141203D000500020305000401000000070600050004640000000D143700111600050002030000";
  TINY_Hex2Bin(s, strlen(s), buffer);
  TINY_Run(buffer);
  return 0;
#endif
 
  if (argc != 2)
  {
    printf("Usage: %s <programm>\n", argv[0]);
    return 1;
  }
  printf("Tiny Basic V1.0\n");

  IXX_U8 *code = TINY_Compiler("../test.bas", &size);

  for(IXX_U16 i = 0; i < size; i++)
  {
    printf("%02X ", code[i]);
    if((i % 32) == 15)
    {
      printf("\n");
    }
  }
  printf("\n");

  TINY_Run(code);

  printf("Ready.\n");
  return 0;
}
