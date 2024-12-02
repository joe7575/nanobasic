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

//#define DEBUG

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "jbi.h"
#include "jbi_int.h"

// PRINTF via UART
#if defined(_WIN32) || defined(WIN32) || defined(__linux__)
    #define PRINTF printf
#else
    #include "fsl_debug_console.h"
    #include "board.h"
#endif

#define DPUSH(x) vm->datastack[(uint8_t)(vm->dsp++) % k_STACK_SIZE] = x
#define DPOP() vm->datastack[(uint8_t)(--vm->dsp) % k_STACK_SIZE]
#define DSTACK(x) vm->datastack[(uint8_t)((vm->dsp + x)) % k_STACK_SIZE]
#define CPUSH(x) vm->callstack[(uint8_t)(vm->csp++) % k_STACK_SIZE] = x
#define CPOP() vm->callstack[(uint8_t)(--vm->csp) % k_STACK_SIZE]

#define STR(x) (x >= 0x8000 ? (char*)&vm->heap[x & 0x7FFF] : (char*)&p_programm[x])

/***************************************************************************************************
**    static function-prototypes
***************************************************************************************************/

/***************************************************************************************************
**    global functions
***************************************************************************************************/
void *jbi_create(uint8_t num_vars, uint8_t* p_programm) {
    t_VM *vm = malloc(sizeof(t_VM));
    memset(vm, 0, sizeof(t_VM));
    jbi_mem_init(vm);
    assert(p_programm[0] == k_TAG);
    assert(p_programm[1] == k_VERSION);
    vm->pc = 2;
    return vm;
}

uint32_t *jbi_get_var_address(void *pv_vm, uint8_t var) {
    t_VM *vm = pv_vm;
    return &vm->variables[var];
}

uint8_t *jbi_get_arr_address(void *pv_vm, uint8_t var) {
    t_VM *vm = pv_vm;
    uint_least16_t addr = vm->variables[var];
    return &vm->heap[(addr & 0x7FFF) + 1];
}

uint32_t jbi_pop_var(void *pv_vm) {
    t_VM *vm = pv_vm;
    return DPOP();
}

void jbi_push_var(void *pv_vm, uint32_t value) {
    t_VM *vm = pv_vm;
    DPUSH(value);
}

uint8_t jbi_stack_depth(void *pv_vm) {
    t_VM *vm = pv_vm;
    return vm->dsp;
}

void jbi_set_pc(void * pv_vm, uint16_t addr) {
    t_VM *vm = pv_vm;
    CPUSH(vm->pc);
    vm->pc = addr;
}

/***************************************************************************************************
  Function:
    jbi_run

  Description:
    Run the Tiny Basic Interpreter with the given program.
    Some measures:
    - run jbi_run with an empty program: 680 ticks
    - run jbi_run with a program with 20 instructions: ~1200 ticks
    => Each instruction needs 26 ticks or 16 ns (for 160 MHz)
    => Currently needs 0x350 bytes of code space

  Parameters:
    program  (IN) - pointer to the program

  Return value:
    Number of executed instructions

***************************************************************************************************/
uint16_t jbi_run(void *pv_vm, uint8_t* p_programm, uint16_t len, uint16_t cycles) {
    uint32_t tmp1, tmp2;
    uint16_t idx;
    uint16_t addr, size;
    uint8_t  var;
    uint8_t  *ptr;
    t_VM *vm = pv_vm;

    for(uint16_t i = 0; i < cycles; i++)
    {
#ifdef DEBUG    
        printf("%04X: %02X %s\n", vm->pc, p_programm[vm->pc], Opcodes[p_programm[vm->pc]]);
#endif
        switch (p_programm[vm->pc])
        {
        case k_END:
            return JBI_END;
        case k_PRINTS:
            tmp1 = DPOP();
            PRINTF("%s", STR(tmp1));
            vm->pc += 1;
            break;
        case k_PRINTV:
            PRINTF("%d", DPOP());
            vm->pc += 1;
            break;
        case k_PRINTNL:
            PRINTF("\n");
            vm->pc += 1;
            break;
        case k_PRINTT:
            PRINTF("\t");
            vm->pc += 1;
            break;
        case k_STRING:
            tmp1 = p_programm[vm->pc + 1]; // string length
            DPUSH(vm->pc + 2);  // push string address
            vm->pc += tmp1 + 2;
            break;
        case k_NUMBER:
            DPUSH(ACS32(p_programm[vm->pc + 1]));
            vm->pc += 5;
            break;
        case k_BYTENUM:
            DPUSH(p_programm[vm->pc + 1]);
            vm->pc += 2;
            break;
        case k_VAR:
            var = p_programm[vm->pc + 1];
            DPUSH(vm->variables[var]);
            vm->pc += 2;
            break;
        case k_LET:
            var = p_programm[vm->pc + 1];
            vm->variables[var] = DPOP();
            vm->pc += 2;
            break;
        case k_DIM:
            var = p_programm[vm->pc + 1];
            size = DPOP();
            addr = jbi_mem_alloc(vm, size);
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return JBI_ERROR;
            }
            vm->variables[var] = addr;
            vm->pc += 2;
            break;
        case k_ADD:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 + tmp2);
            vm->pc += 1;
            break;
        case k_SUB:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 - tmp2);
            vm->pc += 1;
            break;
        case k_MUL:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 * tmp2);
            vm->pc += 1;
            break;
        case k_DIV:
            tmp2 = DPOP();
            tmp1 = DPOP();
            if(tmp2 == 0) {
              PRINTF("Error: Division by zero\n");
              DPUSH(0);
            } else {
              DPUSH(tmp1 / tmp2);
            }
            vm->pc += 1;
            break;
        case k_MOD:
            tmp2 = DPOP();
            tmp1 = DPOP();
            if(tmp2 == 0) {
              DPUSH(0);
            } else {
              DPUSH(tmp1 % tmp2);
            }
            vm->pc += 1;
            break;
        case k_AND:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 && tmp2);
            vm->pc += 1;
            break;
        case k_OR:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 || tmp2);
            vm->pc += 1;
            break;
        case k_NOT:
            DPUSH(!DPOP());
            vm->pc += 1;
            break;
        case k_EQU:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 == tmp2);
            vm->pc += 1;
            break;
        case k_NEQU :
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 != tmp2);
            vm->pc += 1;
            break;
        case k_LE:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 < tmp2);
            vm->pc += 1;
            break;
        case k_LEEQU:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 <= tmp2);
            vm->pc += 1;
            break;
        case k_GR:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 > tmp2);
            vm->pc += 1;
            break;
        case k_GREQU:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(tmp1 >= tmp2);
            vm->pc += 1;
            break;
        case k_GOTO:
            vm->pc = ACS16(p_programm[vm->pc + 1]);
            break;
        case k_GOSUB:
            CPUSH(vm->pc + 3);
            vm->pc = ACS16(p_programm[vm->pc + 1]);
            break;
        case k_RETURN:
            vm->pc = (uint16_t)CPOP();
            break;
        case k_NEXT:
            // ID = ID + stack[-1]
            // IF ID <= stack[-2] GOTO start
            tmp1 = ACS16(p_programm[vm->pc + 1]);
            var = p_programm[vm->pc + 3];
            vm->variables[var] = vm->variables[var] + DSTACK(-1);
            if(vm->variables[var] <= DSTACK(-2)) {
              vm->pc = tmp1;
            } else {
              vm->pc += 4;
              DPOP();  // remove step value
              DPOP();  // remove loop end value
            }
            break;
        case k_IF:
            if(DPOP() == 0) {
              vm->pc = ACS16(p_programm[vm->pc + 1]);
            } else {
              vm->pc += 3;
            }
            break;
        case k_FUNC:
            switch(p_programm[vm->pc + 1]) {
            case JBI_CMD: vm->pc += 2; return JBI_CMD;
            case k_LEFTS:
                tmp2 = DPOP();  // number of characters
                tmp1 = DPOP();  // string address
                addr = jbi_mem_alloc(vm, tmp2 + 1);
                if(addr == 0) {
                    PRINTF("Error: Out of memory\n");
                    return JBI_ERROR;
                }
                strncpy(&vm->heap[addr & 0x7FFF], STR(tmp1), tmp2);
                DPUSH(addr);
                vm->pc += 2;
                break;
            case k_RIGHTS:
                tmp2 = DPOP();  // number of characters
                tmp1 = DPOP();  // string address
                addr = jbi_mem_alloc(vm, tmp2 + 1);
                if(addr == 0) {
                    PRINTF("Error: Out of memory\n");
                    return JBI_ERROR;
                }
                strncpy(&vm->heap[addr & 0x7FFF], STR(tmp1) + strlen(STR(tmp1)) - tmp2, tmp2);
                DPUSH(addr);
                vm->pc += 2;
                break;
            case k_MIDS:
                tmp2 = DPOP();  // number of characters
                tmp1 = DPOP();  // start position
                idx = DPOP();   // string address
                addr = jbi_mem_alloc(vm, tmp2 + 1);
                if(addr == 0) {
                    PRINTF("Error: Out of memory\n");
                    return JBI_ERROR;
                }
                strncpy(&vm->heap[addr & 0x7FFF], STR(idx) + tmp1, tmp2);
                DPUSH(addr);
                vm->pc += 2;
                break;
            case k_LEN:
                tmp1 = DPOP();
                DPUSH(strlen(STR(tmp1)));
                vm->pc += 2;
                break;
            case k_VAL:
                tmp1 = DPOP();
                DPUSH(atoi(STR(tmp1)));
                vm->pc += 2;
                break;
            case k_STRS:
                tmp1 = DPOP();
                addr = jbi_mem_alloc(vm, 12);
                if(addr == 0) {
                    PRINTF("Error: Out of memory\n");
                    return JBI_ERROR;
                }
                sprintf(&vm->heap[addr & 0x7FFF], "%d", tmp1);
                DPUSH(addr);
                vm->pc += 2;
                break;
            case k_SET1:
                var = p_programm[vm->pc + 2];
                addr = vm->variables[var] & 0x7FFF;
                tmp1 = DPOP();
                tmp2 = DPOP();
                ACS8(vm->heap[addr + tmp2]) = tmp1;
                vm->pc += 3;
                break;
            case k_GET1:
                var = p_programm[vm->pc + 2];
                addr = vm->variables[var] & 0x7FFF;
                tmp1 = DPOP();
                DPUSH(ACS8(vm->heap[addr + tmp1]));
                vm->pc += 3;
                break;
            case k_SET2:
                var = p_programm[vm->pc + 2];
                addr = vm->variables[var] & 0x7FFF;
                tmp1 = DPOP();
                tmp2 = DPOP();
                ACS16(vm->heap[addr + tmp2]) = tmp1;
                vm->pc += 3;
                break;
            case k_GET2:
                var = p_programm[vm->pc + 2];
                addr = vm->variables[var] & 0x7FFF;
                tmp1 = DPOP();
                DPUSH(ACS16(vm->heap[addr + tmp1]));
                vm->pc += 3;
                break;
            case k_SET4:
                var = p_programm[vm->pc + 2];
                addr = vm->variables[var] & 0x7FFF;
                tmp1 = DPOP();
                tmp2 = DPOP();
                ACS32(vm->heap[addr + tmp2]) = tmp1;
                vm->pc += 3;
                break;
            case k_GET4:
                var = p_programm[vm->pc + 2];
                addr = vm->variables[var] & 0x7FFF;
                tmp1 = DPOP();
                DPUSH(ACS32(vm->heap[addr + tmp1]));
                vm->pc += 3;
                break;
            default:
                PRINTF("Error: unknown function '%u'\n", p_programm[vm->pc + 1]);
                vm->pc += 2;
                break;
            }
            break;
        case k_S_ADD:
            tmp2 = DPOP();
            tmp1 = DPOP();
            uint8_t len = strlen(STR(tmp1)) + strlen(STR(tmp2)) + 1;
            addr = jbi_mem_alloc(vm, len);
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return JBI_ERROR;
            }
            strcpy(&vm->heap[addr & 0x7FFF], STR(tmp1));
            strcat(&vm->heap[addr & 0x7FFF], STR(tmp2));
            DPUSH(addr);
            vm->pc += 1;
            break;
        case k_S_EQU:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) == 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_S_NEQU :
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) == 0 ? 0 : 1);
            vm->pc += 1;
            break;
        case k_S_LE:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) < 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_S_LEEQU:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) <= 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_S_GR:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) > 0 ? 1 : 0);
            vm->pc += 1;
        case k_S_GREQU:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) >= 0 ? 1 : 0);
            vm->pc += 1;
            break;
        default:
            return JBI_ERROR;
        }
    }
    return JBI_BUSY;
}

void jbi_destroy(void * pv_vm) {
  free(pv_vm);
}

/***************************************************************************************************
* Static functions
***************************************************************************************************/
