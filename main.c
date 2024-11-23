#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "TINY.h"
#include "TINYint.h"

int main(int argc, char* argv[])
{
  IXX_U16 size;
  IXX_U8 num_vars;
 
  if (argc != 2)
  {
    printf("Usage: %s <programm>\n", argv[0]);
    return 1;
  }
  printf("Tiny Basic Compiler V1.0\n");

  IXX_U8 *code = TINY_Compiler("../test.bas", &size, &num_vars);

  if(code == NULL)
  {
    return 1;
  }
  TINY_OutputSymbolTable();

  printf("\nTiny Basic Interpreter V1.0\n");
  void *instance = TINY_Create(num_vars);
  TINY_SetVar(instance, 0, 0); // TODO: Use real variable address
  TINY_SetVar(instance, 1, 16);
  TINY_Run(instance, code, size);
  TINY_Destroy(instance);
  //TINY_DumpCode(IXX_U8 *code, IXX_U16 size);

  printf("Ready.\n");
  return 0;
}
