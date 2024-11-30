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
    uint8_t evt1, evt2, buf1, buf2;
    uint32_t *p1, *p2;
    uint8_t  *p3, *p4;
    
    if (argc != 2) {
        printf("Usage: %s <programm>\n", argv[0]);
        return 1;
    }
    printf("Joes Basic Compiler V1.0\n");

    jbi_init();
    evt1 = jbi_add_variable("evt1");
    evt2 = jbi_add_variable("evt2");
    buf1 = jbi_add_buffer("buf1", 0);
    buf2 = jbi_add_buffer("buf2", 1);
    uint8_t *code = jbi_compiler("../test.bas", &size, &num_vars);

    if(code == NULL) {
        return 1;
    }
    jbi_output_symbol_table();

    printf("\nJoes Basic Interpreter V1.0\n");
    void *instance = jbi_create(num_vars, code);
    // test_memory(instance);
    // return 0;
    jbi_dump_code(code, size);
    p1 = jbi_get_variable_address(instance, evt1);
    p2 = jbi_get_variable_address(instance, evt2);
    p3 = jbi_get_buffer_address(instance, buf1);
    p4 = jbi_get_buffer_address(instance, buf2);
    
    while(res >= JBI_BUSY) {
        // A simple for loop "for i = 1 to 100: print i: next i" 
        // needs ~500 ticks or 1 second (50 cycles per 100 ms)
        res = jbi_run(instance, code, size, 50);
        cycles += 50;
        msleep(100);
        if(res == JBI_CMD) {
            uint32_t num = jbi_pull_variable(instance);
            printf("Send command to %u: %02X %02X %02X %02X\n", num, p3[0], p3[1], p3[2], p3[3]);
        } else if(res == JBI_EVENT) {
            uint32_t evt = jbi_pull_variable(instance);
            uint32_t num = jbi_pull_variable(instance);
            printf("Send event to %u: %u\n", num, evt);
        }
    }
    jbi_destroy(instance);

    printf("Cycles: %u\n", cycles);
    printf("Ready.\n");
    return 0;
}
