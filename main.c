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
#include <time.h>
#include <errno.h>    
#include "nb.h"

extern void test_memory(void *p_vm);

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(uint32_t msec)
{
    struct timespec ts;
    int res;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

int main(int argc, char* argv[]) {
    uint16_t size;
    uint8_t num_vars;
    uint16_t res = NB_BUSY;
    uint32_t cycles = 0;
    uint16_t buf1;
    uint8_t *pArr = NULL;
    uint16_t on_can;
    char s[80];
    
    if (argc != 2) {
        printf("Usage: %s <programm>\n", argv[0]);
        return 1;
    }
    printf("Joes Basic Compiler V1.0\n");

    nb_init();
    //assert(nb_define_external_function("foo", 2, (uint8_t[]){1, 2}, 0) == 0);
    //assert(nb_define_external_function("foo2", 0, (uint8_t[]){}, 1) == 1);
    //assert(nb_define_external_function("foo3", 0, (uint8_t[]){}, 2) == 2);
    //uint8_t *code = nb_compiler("../examples/temp.bas", &size);
    //uint8_t *code = nb_compiler("../examples/lineno.bas", &size);
    uint8_t *code = nb_compiler("../examples/test.bas", &size);
    //uint8_t *code = nb_compiler("../examples/basis.bas", &size);

    if(code == NULL) {
        return 1;
    }
    nb_output_symbol_table();
    on_can = nb_get_label_address("200");
    if(on_can == 0) {
        on_can = nb_get_label_address("on_can");
    }
    buf1 = nb_get_var_num("buf1");

    printf("\nJoes Basic Interpreter V1.0\n");
    void *instance = nb_create(code);
    //test_memory(instance);
    //return 0;
    nb_dump_code(code, size);
    num_vars = nb_get_num_vars();

    while(res >= NB_BUSY) {
        // A simple for loop "for i = 1 to 100: print i: next i" 
        // needs ~500 ticks or 1 second (50 cycles per 100 ms)
        res = nb_run(instance, code, size, 50, num_vars);
        cycles += 50;
        msleep(100);
        if(res == NB_XFUNC) {
            if(nb_ReturnValue == 0) {
                printf("External function foo(%d, \"%s\")\n", nb_pop_num(instance), nb_pop_str(instance, s, 80));
            } else if(nb_ReturnValue == 1) {
                printf("External function foo2()\n");
                nb_push_num(instance, 87654);
            } else if(nb_ReturnValue == 2) {
                printf("External function foo3()\n");
                nb_push_str(instance, "Solali");
            } else {
                printf("Unknown external function\n");
            }
        } else if(on_can > 0){
            nb_push_num(instance, 87654);
            nb_set_pc(instance, on_can);
        }
    }
    nb_destroy(instance);

    printf("Cycles: %u\n", cycles);
    printf("Ready.\n");
    return 0;
}

            // case k_POP_PTR_N2:
            //     var = p_programm[vm->pc + 1];
            //     addr = vm->variables[var];
            //     if(addr > 0) {
            //         nb_mem_free(vm, addr);
            //     }
            //     addr = DPOP();
            //     size = nb_mem_get_blocksize(vm, addr);
            //     addr2 = nb_mem_alloc(vm, size);
            //     if(addr2 == 0) {
            //         PRINTF("Error: Out of memory\n");
            //         return NB_ERROR;
            //     }
            //     memcpy(&vm->heap[addr2 & 0x7FFF], &vm->heap[addr & 0x7FFF], size);
            //     vm->variables[var] = addr2;
            //     vm->pc += 2;
            //     break;
