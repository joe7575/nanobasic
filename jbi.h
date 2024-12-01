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
  JBI_ERROR,    // error in programm
  JBI_BUSY,     // programm still willing to run
  JBI_CMD,   // send command (buffer)
  JBI_EVENT,   // send event (variable)
};

void jbi_init(void);
uint8_t jbi_add_variable(char *name);
uint8_t jbi_add_buffer(char *name, uint8_t idx);
uint8_t *jbi_compiler(char *filename, uint16_t *p_len, uint8_t *p_num_vars);
void *jbi_create(uint8_t num_vars, uint8_t* p_programm);
uint32_t *jbi_get_variable_address(void *pv_vm, uint8_t var);
uint8_t *jbi_get_buffer_address(void *pv_vm, uint8_t idx);
uint32_t jbi_pop_variable(void *pv_vm);
uint16_t jbi_run(void *pv_vm, uint8_t* p_programm, uint16_t len, uint16_t cycles);
void jbi_destroy(void * pv_vm);
void jbi_dump_code(uint8_t *code, uint16_t size);
void jbi_output_symbol_table(void);
uint16_t jbi_get_label_address(char *name);
void jbi_call_0(void * pv_vm, uint16_t addr);
void jbi_call_1(void * pv_vm, uint16_t addr, uint32_t param1);
void jbi_call_2(void * pv_vm, uint16_t addr, uint32_t param1, uint32_t param2);
void jbi_call_3(void * pv_vm, uint16_t addr, uint32_t param1, uint32_t param2, uint32_t param3);
