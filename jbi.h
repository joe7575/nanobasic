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
#include "jbi_cfg.h"

enum {
  JBI_END = 0,  // programm end reached
  JBI_ERROR,    // error in programm
  JBI_BUSY,     // programm still willing to run
  JBI_CMD,      // send command (buffer)
  JBI_BREAK,    // break command
};

void jbi_init(void);
uint8_t *jbi_compiler(char *filename, uint16_t *p_len);
void *jbi_create(uint8_t* p_programm);
uint16_t jbi_run(void *pv_vm, uint8_t* p_programm, uint16_t len, uint16_t cycles, uint8_t num_vars);
void jbi_destroy(void * pv_vm);

void jbi_dump_code(uint8_t *code, uint16_t size);
void jbi_output_symbol_table(void);

uint8_t jbi_get_num_vars(void);
uint16_t jbi_get_var_num(char *name);
uint16_t jbi_get_label_address(char *name);
uint32_t *jbi_get_var_address(void *pv_vm, uint8_t var);
uint8_t *jbi_get_arr_address(void *pv_vm, uint8_t idx);
uint32_t jbi_pop_var(void *pv_vm);
void jbi_push_var(void *pv_vm, uint32_t value);
uint8_t jbi_stack_depth(void *pv_vm);

void jbi_set_pc(void * pv_vm, uint16_t addr);
