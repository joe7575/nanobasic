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
    uint32_t timeout = 0;
    uint32_t startval = time(NULL);
    
    if (argc != 2) {
        nb_print("Usage: %s <programm>\n", argv[0]);
        return 1;
    }
    nb_print("NanoBasic Compiler V1.0\n");
    nb_init();
    assert(nb_define_external_function("setcur", 2, (uint8_t[]){NB_NUM, NB_NUM}, NB_NONE) == NB_XFUNC + 0);
    assert(nb_define_external_function("clrscr", 0, (uint8_t[]){}, NB_NONE) == NB_XFUNC + 1);
    assert(nb_define_external_function("clrline", 1, (uint8_t[]){NB_NUM}, NB_NONE) == NB_XFUNC + 2);
    assert(nb_define_external_function("time", 0, (uint8_t[]){}, NB_NUM) == NB_XFUNC + 3);
    assert(nb_define_external_function("sleep", 1, (uint8_t[]){NB_NUM}, NB_NONE) == NB_XFUNC + 4);
    assert(nb_define_external_function("input", 1, (uint8_t[]){NB_STR}, NB_NUM) == NB_XFUNC + 5);
    assert(nb_define_external_function("input$", 1, (uint8_t[]){NB_STR}, NB_STR) == NB_XFUNC + 6);
    assert(nb_define_external_function("bcmd", 3, (uint8_t[]){NB_NUM, NB_NUM, NB_ARR}, NB_NUM) == NB_XFUNC + 7);
    assert(nb_define_external_function("cmd$", 3, (uint8_t[]){NB_NUM, NB_STR, NB_STR}, NB_STR) == NB_XFUNC + 8);
    assert(nb_define_external_function("breq", 3, (uint8_t[]){NB_NUM, NB_NUM, NB_ARR}, NB_NUM) == NB_XFUNC + 9);
    assert(nb_define_external_function("breq$", 3, (uint8_t[]){NB_NUM, NB_NUM, NB_ARR}, NB_STR) == NB_XFUNC + 10);
    assert(nb_define_external_function("dclr", 1, (uint8_t[]){NB_NUM}, NB_NONE) == NB_XFUNC + 11);
    assert(nb_define_external_function("dputs", 3, (uint8_t[]){NB_NUM, NB_NUM, NB_STR}, NB_NONE) == NB_XFUNC + 12);
    assert(nb_define_external_function("chat", 1, (uint8_t[]){NB_STR}, NB_NONE) == NB_XFUNC + 13);
    assert(nb_define_external_function("iname$", 1, (uint8_t[]){NB_STR}, NB_STR) == NB_XFUNC + 14);
    assert(nb_define_external_function("door", 2, (uint8_t[]){NB_STR, NB_STR}, NB_NONE) == NB_XFUNC + 15);

    void *instance = nb_create();

    //FILE *fp = fopen("../examples/basicV2.bas", "r");
    //FILE *fp = fopen("../examples/lineno.bas", "r");
    //FILE *fp = fopen("../examples/read_data.bas", "r");
    //FILE *fp = fopen("../examples/test.bas", "r");
    //FILE *fp = fopen("../examples/basis.bas", "r");
    //FILE *fp = fopen("../examples/ext_func.bas", "r");
    //FILE *fp = fopen("../examples/on_gosub.bas", "r");
    FILE *fp = fopen("../examples/temp.bas", "r");

    if(fp == NULL) {
        nb_print("Error: could not open file\n");
        return -1;
    }

    errors = nb_compile(instance, fp);
    fclose(fp);

    if(errors > 0) {
        return 1;
    }

    nb_output_symbol_table(instance);
    nb_print("\nNanoBasic Interpreter V1.0\n");
    nb_dump_code(instance);

    while(res >= NB_BUSY) {
        cycles = 50;
        while(cycles > 0 && res >= NB_BUSY && timeout <= time(NULL)) {
            res = nb_run(instance, &cycles);
            if(res == NB_BREAK) {
                uint32_t lineno = nb_pop_num(instance);
                nb_print("Break in line %u\n", lineno);
                uint32_t val = nb_get_number(instance, 0);
                printf("read num %d = %d\n", 0, val);
                char *ptr = nb_get_string(instance, 1);
                if(ptr != NULL) {
                    printf("read str %d = %s\n", 1, ptr);
                }
                val = nb_get_arr_elem(instance, 3, 0);
                printf("read arr %d(%d) = %d\n", 3, 0, val);
            } else if(res == NB_XFUNC) {
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
            } else if(res == NB_XFUNC + 3) {
                // time
                nb_push_num(instance, time(NULL) - startval);
            } else if(res == NB_XFUNC + 4) {
                // sleep
                timeout = time(NULL) + nb_pop_num(instance);
            } else if(res == NB_XFUNC + 5) {
                // input
                char str[80];
                nb_pop_str(instance, str, 80);
                nb_print("%s?  ", str);
                fgets(str, 80, stdin);
                str[strlen(str)-1] = '\0';
                nb_push_num(instance, atoi(str));
            } else if(res == NB_XFUNC + 6) {
                // input$
                char str[80];
                nb_pop_str(instance, str, 80);
                nb_print("%s?  ", str);
                fgets(str, 80, stdin);
                str[strlen(str)-1] = '\0';
                nb_push_str(instance, str);
            } else if(res == NB_XFUNC + 7) {
                uint32_t arr[4];
                // bcmd
                uint16_t addr = nb_pop_arr_addr(instance);
                uint32_t cmd = nb_pop_num(instance);
                nb_read_arr(instance, addr, (uint8_t*)arr, sizeof(arr));
                nb_print("BCMD %u: %u %u %u %u\n", cmd, arr[0], arr[1], arr[2], arr[3]);
                nb_push_num(instance, 3);
            } else if(res == NB_XFUNC + 8) {
                char str[80];
                // cmd$
                uint32_t cmd = nb_pop_num(instance);
                char *str1 = nb_pop_str(instance, str, 80);
                char *str2 = nb_pop_str(instance, str, 80);
                nb_print("CMD$ %u: %s %s\n", cmd, str1, str2);
                nb_push_str(instance, "OK");
            } else if(res == NB_XFUNC + 9) {
                uint32_t arr[4];
                // breq
                uint16_t addr = nb_pop_arr_addr(instance);
                uint32_t cmd = nb_pop_num(instance);
                nb_read_arr(instance, addr, (uint8_t*)arr, sizeof(arr));
                nb_print("BREQ %u: %u %u %u %u\n", cmd, arr[0], arr[1], arr[2], arr[3]);
                nb_push_num(instance, 3);
            } else if(res == NB_XFUNC + 10) {
                char str[80];
                // breq$
                uint32_t cmd = nb_pop_num(instance);
                char *str1 = nb_pop_str(instance, str, 80);
                char *str2 = nb_pop_str(instance, str, 80);
                nb_print("BREQ$ %u: %s %s\n", cmd, str1, str2);
                nb_push_str(instance, "OK");
            } else if(res == NB_XFUNC + 11) {
                // dclr
                uint8_t var = nb_pop_num(instance);
                nb_print("DCLR %u\n", var);
            } else if(res == NB_XFUNC + 12) {
                // dputs
                char str[80];
                char *str1 = nb_pop_str(instance, str, 80);
                uint16_t row = nb_pop_num(instance);
                uint16_t addr = nb_pop_num(instance);
                nb_print("DPUTS %u: %d %s\n", addr, row, str1);
            } else if(res == NB_XFUNC + 13) {
                // chat
                char str[80];
                char *str1 = nb_pop_str(instance, str, 80);
                nb_print("CHAT: %s\n", str1);
            } else if(res == NB_XFUNC + 14) {
                // iname$
                char str[80];
                char *str1 = nb_pop_str(instance, str, 80);
                nb_print("INAME$: %s\n", str1);
                nb_push_str(instance, "OK");
            } else if(res == NB_XFUNC + 15) {
                // door
                char str[80];
                char *str1 = nb_pop_str(instance, str, 80);
                char *str2 = nb_pop_str(instance, str, 80);
                nb_print("DOOR: %s %s\n", str1, str2);
            } else if(res >= NB_XFUNC + 16) {
                nb_print("Unknown external function\n");
            }
        }
        msleep(100);
    }
    nb_destroy(instance);
    nb_print("Ready.\n");
    return 0;
}
