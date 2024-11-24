#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "TINY.h"
#include "TINYint.h"

void test_memory(void)
{
  IXX_U8 *p1, *p2, *p3, *p4;
  void *pv_mem = TINY_MemoryCreate(64);
  TINY_MemoryDump(pv_mem);
  assert((p1 = TINY_MemoryAlloc(pv_mem, 30)) != NULL);
  assert((p2 = TINY_MemoryAlloc(pv_mem, 31)) != NULL);
  assert((p3 = TINY_MemoryAlloc(pv_mem, 32)) != NULL);
  assert((p4 = TINY_MemoryAlloc(pv_mem, 128)) != NULL);

  memset(p1, 0x11, 30);
  memset(p2, 0x22, 31);
  memset(p3, 0x33, 32);
  memset(p4, 0x44, 128);

  TINY_MemoryDump(pv_mem);
  TINY_MemoryFree(pv_mem, p2);
  TINY_MemoryFree(pv_mem, p4);
  TINY_MemoryDump(pv_mem);
  TINY_MemoryFree(pv_mem, p1);
  TINY_MemoryFree(pv_mem, p3);
  TINY_MemoryDump(pv_mem);
  TINY_MemoryDestroy(pv_mem);
}

int main(int argc, char* argv[])
{
  IXX_U16 size;
  IXX_U8 num_vars;
  
  //test_memory();

  if (argc != 2)
  {
    printf("Usage: %s <programm>\n", argv[0]);
    return 1;
  }
  printf("Tiny Basic Compiler V1.0\n");

  IXX_U8 *code = TINY_Compiler("../temp.bas", &size, &num_vars);

  if(code == NULL)
  {
    return 1;
  }
  TINY_OutputSymbolTable();

  printf("\nTiny Basic Interpreter V1.0\n");
  void *instance = TINY_Create(num_vars);
  //TINY_SetVar(instance, 0, 0); // TODO: Use real variable address
  //TINY_SetVar(instance, 1, 16);
  TINY_DumpCode(code, size);
  
  TINY_Run(instance, code, size);
  TINY_Destroy(instance);

  printf("Ready.\n");
  return 0;
}
