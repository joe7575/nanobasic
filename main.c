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
#include "jbi.h"

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
    uint16_t res = JBI_BUSY;
    uint32_t cycles = 0;
    uint16_t buf1;
    uint8_t *pArr = NULL;
    uint16_t on_can;
    
    if (argc != 2) {
        printf("Usage: %s <programm>\n", argv[0]);
        return 1;
    }
    printf("Joes Basic Compiler V1.0\n");

    jbi_init();
    //uint8_t *code = jbi_compiler("../temp.bas", &size, &num_vars);
    //uint8_t *code = jbi_compiler("../lineno.bas", &size, &num_vars);
    //uint8_t *code = jbi_compiler("../test.bas", &size, &num_vars);
    uint8_t *code = jbi_compiler("../basis.bas", &size, &num_vars);

    if(code == NULL) {
        return 1;
    }
    //jbi_output_symbol_table();
    on_can = jbi_get_label_address("200");
    if(on_can == 0) {
        on_can = jbi_get_label_address("on_can");
    }
    buf1 = jbi_get_var_num("buf1");

    printf("\nJoes Basic Interpreter V1.0\n");
    void *instance = jbi_create(num_vars, code);
    //est_memory(instance);
    //return 0;
    jbi_dump_code(code, size);

    while(res >= JBI_BUSY) {
        // A simple for loop "for i = 1 to 100: print i: next i" 
        // needs ~500 ticks or 1 second (50 cycles per 100 ms)
        res = jbi_run(instance, code, size, 50);
        cycles += 50;
        msleep(100);
        if(res == JBI_CMD) {
            pArr = jbi_get_arr_address(instance, buf1);
            if(pArr != NULL) {
                uint32_t num = jbi_pop_var(instance);
                printf("Send command to %u: %02X %02X %02X %02X\n", num, pArr[0], pArr[1], pArr[2], pArr[3]);
            }
        } else if(on_can > 0){
            jbi_push_var(instance, 87654);
            jbi_set_pc(instance, on_can);
        }
    }
    jbi_destroy(instance);

    printf("Cycles: %u\n", cycles);
    printf("Ready.\n");
    return 0;
}

            // case k_POP_PTR_N2:
            //     var = p_programm[vm->pc + 1];
            //     addr = vm->variables[var];
            //     if(addr > 0) {
            //         jbi_mem_free(vm, addr);
            //     }
            //     addr = DPOP();
            //     size = jbi_mem_get_blocksize(vm, addr);
            //     addr2 = jbi_mem_alloc(vm, size);
            //     if(addr2 == 0) {
            //         PRINTF("Error: Out of memory\n");
            //         return JBI_ERROR;
            //     }
            //     memcpy(&vm->heap[addr2 & 0x7FFF], &vm->heap[addr & 0x7FFF], size);
            //     vm->variables[var] = addr2;
            //     vm->pc += 2;
            //     break;
