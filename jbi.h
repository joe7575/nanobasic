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

#include <stdint.h>
#include <stdbool.h>

enum {
  JBI_END = 0,  // programm end reached
  JBI_BUSY,     // programm still willing to run
  JBI_ERROR,    // error in programm
};

uint8_t *jbi_compiler(char *filename, uint16_t *p_len, uint8_t *p_num_vars);
void *jbi_create(uint8_t num_vars);
//void jbi_SetVar(void *pv_vm, uint8_t var, uint32_t val);
uint16_t jbi_run(void *pv_vm, uint8_t* p_programm, uint16_t len, uint16_t cycles);
void jbi_destroy(void * pv_vm);
void jbi_dump_code(uint8_t *code, uint16_t size);
void jbi_output_symbol_table(void);

void *jbi_mem_create(uint16_t num_blocks);
void  jbi_mem_dump(void *pv_mem);
void  jbi_mem_destroy(void *pv_mem);
void *jbi_mem_alloc(void *pv_mem, uint16_t bytes);
void  jbi_mem_free(void *pv_mem, void *pv_ptr);
void *jbi_mem_realloc(void *pv_mem, void *pv_ptr, uint16_t bytes);
