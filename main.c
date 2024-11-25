/*

Copyright 2024 Joachim Stolberg

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "jbi.h"

void test_memory(void)
{
  uint8_t *p1, *p2, *p3, *p4;
  void *pv_mem = jbi_mem_create(64);
  jbi_mem_dump(pv_mem);
  assert((p1 = jbi_mem_alloc(pv_mem, 30)) != NULL);
  assert((p2 = jbi_mem_alloc(pv_mem, 31)) != NULL);
  assert((p3 = jbi_mem_alloc(pv_mem, 32)) != NULL);
  assert((p4 = jbi_mem_alloc(pv_mem, 128)) != NULL);

  memset(p1, 0x11, 30);
  memset(p2, 0x22, 31);
  memset(p3, 0x33, 32);
  memset(p4, 0x44, 128);

  jbi_mem_dump(pv_mem);
  jbi_mem_free(pv_mem, p2);
  jbi_mem_free(pv_mem, p4);
  jbi_mem_dump(pv_mem);
  jbi_mem_free(pv_mem, p1);
  jbi_mem_free(pv_mem, p3);
  jbi_mem_dump(pv_mem);
  jbi_mem_destroy(pv_mem);
}

int main(int argc, char* argv[])
{
  uint16_t size;
  uint8_t num_vars;
  
  //test_memory();

  if (argc != 2)
  {
    printf("Usage: %s <programm>\n", argv[0]);
    return 1;
  }
  printf("Joes Basic Compiler V1.0\n");

  uint8_t *code = jbi_compiler("../temp.bas", &size, &num_vars);

  if(code == NULL)
  {
    return 1;
  }
  jbi_output_symbol_table();

  printf("\nJoes Basic Interpreter V1.0\n");
  void *instance = jbi_create(num_vars);
  //jbi_SetVar(instance, 0, 0); // TODO: Use real variable address
  //jbi_SetVar(instance, 1, 16);
  jbi_dump_code(code, size);
  
  jbi_run(instance, code, size, 10000);
  jbi_destroy(instance);

  printf("Ready.\n");
  return 0;
}
