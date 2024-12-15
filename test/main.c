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
#include "nb_int.h"

#define MAX_CODE_SIZE   (1024 * 16)

extern void test_memory(void *p_vm);

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(uint32_t msec)
{
    struct timespec ts;
    int res;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    //clock_t begin = clock();
    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
    //clock_t end = clock();
    //double time_spent = (double)(end - begin); //in microseconds
    //printf("Time spent: %f\n", time_spent);
    return res;
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
    uint16_t res = NB_BUSY;
    uint16_t cycles;
    uint16_t errors;
    
    if (argc != 2) {
        nb_print("Usage: %s <programm>\n", argv[0]);
        return 1;
    }
    nb_print("NanoBasic Compiler V1.0\n");
    nb_init();
    assert(nb_define_external_function("setcur", 2, (uint8_t[]){NB_NUM, NB_NUM}, NB_NONE) == 0);
    assert(nb_define_external_function("clrscr", 0, (uint8_t[]){}, NB_NONE) == 1);
    assert(nb_define_external_function("clrline", 1, (uint8_t[]){NB_NUM}, NB_NONE) == 2);
    //assert(nb_define_external_function("efunc2", 0, (uint8_t[]){}, NB_NUM) == 1);
    //assert(nb_define_external_function("efunc3", 0, (uint8_t[]){}, NB_STR) == 2);
    void *instance = nb_create();
    //FILE *fp = fopen("../examples/basicV2.bas", "r");
    //FILE *fp = fopen("../examples/lineno.bas", "r");
    //FILE *fp = fopen("../examples/test.bas", "r");
    //FILE *fp = fopen("../examples/basis.bas", "r");
    //FILE *fp = fopen("../examples/ext_func.bas", "r");
    //FILE *fp = fopen("../examples/read_data.bas", "r");
    FILE *fp = fopen("../examples/temp.bas", "r");
    if(fp == NULL) {
        nb_print("Error: could not open file\n");
        return -1;
    }

    errors = nb_compile(instance, fp);
    fclose(fp);

    if(errors >0) {
        return 1;
    }

    nb_output_symbol_table(instance);
    //efunc4 = nb_get_label_address(instance, "efunc4");
    //ext_buf = nb_get_var_num(instance, "extbuf");

    nb_print("\nNanoBasic Interpreter V1.0\n");
    nb_dump_code(instance);

    while(res >= NB_BUSY) {
        cycles = 50;
        while(cycles > 0 && res >= NB_BUSY) {
            res = nb_run(instance, &cycles);
            if(res == NB_XFUNC) {
                // setcur
                uint8_t x = nb_pop_num(instance);
                uint8_t y = nb_pop_num(instance);
                x = MAX(1, MIN(x, 60));
                y = MAX(1, MIN(y, 20));
                nb_print("\033[%u;%uH", x, y);
            } else if(res == NB_XFUNC + 1) {
                // clrscr
                nb_print("\033[2J");
            } else if(res == NB_XFUNC + 2) {
                // clrline
                nb_print("\033[2K");
            } else if(res > NB_XFUNC + 2) {
                nb_print("Unknown external function\n");
            }
        }
        msleep(100);
    }
    nb_destroy(instance);
    nb_print("Ready.\n");
    return 0;
}
