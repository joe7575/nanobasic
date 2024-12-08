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
#include <stdarg.h>
#include "nb.h"

#define MAX_CODE_SIZE   (1024 * 16)

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

void nb_reset_file_pos(void *fp) {
    fseek(fp, 0, SEEK_SET);
}
char *nb_get_code_line(void *fp, char *line, int max_line_len) {
    return fgets(line, max_line_len, fp);
}

void nb_print(const char * format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

int main(int argc, char* argv[]) {
    uint16_t code_size;
    uint8_t *p_code;
    uint8_t num_vars;
    uint16_t res = NB_BUSY;
    uint32_t cycles = 0;
    uint16_t on_can;
    uint16_t errors;
    char s[80];
    
    if (argc != 2) {
        nb_print("Usage: %s <programm>\n", argv[0]);
        return 1;
    }
    nb_print("NanoBasic Compiler V1.0\n");
    nb_init();
    assert(nb_define_external_function("foo", 2, (uint8_t[]){1, 2}, 0) == 0);
    assert(nb_define_external_function("foo2", 0, (uint8_t[]){}, 1) == 1);
    assert(nb_define_external_function("foo3", 0, (uint8_t[]){}, 2) == 2);

    //FILE *fp = fopen("../examples/temp.bas", "r");
    //FILE *fp = fopen("../examples/lineno.bas", "r");
    FILE *fp = fopen("../examples/test.bas", "r");
    //FILE *fp = fopen("../examples/basis.bas", "r");
    if(fp == NULL) {
        nb_print("Error: could not open file\n");
        return -1;
    }

    p_code = malloc(MAX_CODE_SIZE);
    code_size = MAX_CODE_SIZE;
    errors = nb_compile(fp, p_code, &code_size, &num_vars);
    fclose(fp);

    if(errors >0) {
        return 1;
    }

    nb_output_symbol_table();
    on_can = nb_get_label_address("200");
    if(on_can == 0) {
        on_can = nb_get_label_address("on_can");
    }

    nb_print("\nNanoBasic Interpreter V1.0\n");
    void *instance = nb_create(p_code, code_size, MAX_CODE_SIZE, num_vars);
    nb_dump_code(p_code, code_size);

    while(res >= NB_BUSY) {
        // A simple for loop "for i = 1 to 100: print i: next i" 
        // needs ~500 ticks or 1 second (50 cycles per 100 ms)
        res = nb_run(instance, 50);
        cycles += 50;
        msleep(100);
        if(res >= NB_XFUNC) {
            if(res == NB_XFUNC) {
                nb_print("External function foo(%d, \"%s\")\n", nb_pop_num(instance), nb_pop_str(instance, s, 80));
            } else if(res == NB_XFUNC + 1) {
                nb_print("External function foo2()\n");
                nb_push_num(instance, 87654);
            } else if(res == NB_XFUNC + 2) {
                nb_print("External function foo3()\n");
                nb_push_str(instance, "Solali");
            } else {
                nb_print("Unknown external function\n");
            }
        } else if(on_can > 0){
            nb_push_num(instance, 87654);
            nb_set_pc(instance, on_can);
        }
    }
    nb_destroy(instance);

    nb_print("Cycles: %u\n", cycles);
    nb_print("Ready.\n");
    return 0;
}
