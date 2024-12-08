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
#include "nb_cfg.h"

enum {
  NB_END = 0,  // programm end reached
  NB_ERROR,    // error in programm
  NB_BUSY,     // programm still willing to run
  NB_XFUNC,    // 'call' external function
  NB_BREAK,    // break command
};

extern uint8_t nb_ReturnValue;

void nb_init(void);
uint8_t nb_define_external_function(char *name, uint8_t num_params, uint8_t *types, uint8_t return_type);
uint8_t *nb_compiler(char *filename, uint16_t *p_len);
void *nb_create(uint8_t* p_programm);
uint16_t nb_run(void *pv_vm, uint8_t* p_programm, uint16_t len, uint16_t cycles, uint8_t num_vars);
void nb_destroy(void * pv_vm);

void nb_dump_code(uint8_t *code, uint16_t size);
void nb_output_symbol_table(void);

uint8_t nb_get_num_vars(void);
uint16_t nb_get_var_num(char *name);
uint16_t nb_get_label_address(char *name);

void nb_set_pc(void * pv_vm, uint16_t addr);

uint8_t nb_stack_depth(void *pv_vm);
uint32_t nb_pop_num(void *pv_vm);
void nb_push_num(void *pv_vm, uint32_t value);
char *nb_pop_str(void *pv_vm, char *str, uint8_t len);
void nb_push_str(void *pv_vm, char *str);
uint32_t *nb_pop_arr(void *pv_vm, uint32_t *arr, uint8_t len);
void nb_push_arr(void *pv_vm, uint32_t *arr, uint8_t len);
