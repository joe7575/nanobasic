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
#include "nb.h"
#include "nb_int.h"

// PRINTF via UART
#if defined(_WIN32) || defined(WIN32) || defined(__linux__)
    #define PRINTF printf
#else
    #include "fsl_debug_console.h"
    #include "board.h"
#endif

#define DPUSH(x) vm->datastack[(uint8_t)(vm->dsp++) % cfg_DATASTACK_SIZE] = x
#define DPOP() vm->datastack[(uint8_t)(--vm->dsp) % cfg_DATASTACK_SIZE]
#define DTOP() vm->datastack[(uint8_t)(vm->dsp - 1) % cfg_DATASTACK_SIZE]
#define DPEEK(x) vm->datastack[(uint8_t)((vm->dsp + x)) % cfg_DATASTACK_SIZE]

#define CPUSH(x) vm->callstack[(uint8_t)(vm->csp++) % cfg_STACK_SIZE] = x
#define CPOP() vm->callstack[(uint8_t)(--vm->csp) % cfg_STACK_SIZE]

#define PPUSH(x) vm->paramstack[(uint8_t)(vm->psp++) % cfg_STACK_SIZE] = x
#define PPOP() vm->paramstack[(uint8_t)(--vm->psp) % cfg_STACK_SIZE]

#define STR(x) (x >= 0x8000 ? (char*)&vm->heap[x & 0x7FFF] : (char*)&vm->p_programm[x])

uint8_t nb_ReturnValue = 0;

/***************************************************************************************************
**    static function-prototypes
***************************************************************************************************/

/***************************************************************************************************
**    global functions
***************************************************************************************************/
void *nb_create(uint8_t* p_programm) {
    t_VM *vm = malloc(sizeof(t_VM));
    memset(vm, 0, sizeof(t_VM));
    nb_mem_init(vm);
    srand(time(NULL));
    assert(p_programm[0] == k_TAG);
    assert(p_programm[1] == k_VERSION);
    vm->pc = 2;
    return vm;
}

uint32_t nb_pop_num(void *pv_vm) {
    t_VM *vm = pv_vm;
    if(vm->psp == 0) {
        return 0;
    }
    return PPOP();
}

void nb_push_num(void *pv_vm, uint32_t value) {
    t_VM *vm = pv_vm;
    if(vm->psp < cfg_STACK_SIZE) {
        PPUSH(value);
    }
}

char *nb_pop_str(void *pv_vm, char *str, uint8_t len) {
    t_VM *vm = pv_vm;
    if(vm->psp == 0) {
        return NULL;
    }
    uint16_t addr = PPOP();
    strncpy(str, STR(addr), len);
    return str;
}

void nb_push_str(void *pv_vm, char *str) {
    t_VM *vm = pv_vm;
    if(vm->psp < cfg_STACK_SIZE) {
        uint16_t len = strlen(str);
        uint16_t addr = nb_mem_alloc(vm, len + 1);
        if(addr == 0) {
            PRINTF("Error: Out of memory\n");
            return;
        }
        strcpy(&vm->heap[addr & 0x7FFF], str);
        PPUSH(addr);
    }
}

uint32_t *nb_pop_arr(void *pv_vm, uint32_t *arr, uint8_t len) {
    t_VM *vm = pv_vm;
    if(vm->psp == 0) {
        return NULL;
    }
    uint16_t addr = PPOP();
    uint16_t size = nb_mem_get_blocksize(vm, addr);
    size = MIN(size, len * sizeof(uint32_t));
    memcpy(arr, &vm->heap[addr & 0x7FFF], size);
    return arr;
}

void nb_push_arr(void *pv_vm, uint32_t *arr, uint8_t len) {
    t_VM *vm = pv_vm;
    if(vm->psp < cfg_STACK_SIZE) {
        uint16_t size = len * sizeof(uint32_t);
        uint16_t addr = nb_mem_alloc(vm, size);
        if(addr == 0) {
            PRINTF("Error: Out of memory\n");
            return;
        }
        memcpy(&vm->heap[addr & 0x7FFF], arr, size);
        PPUSH(addr);
    }
}

uint8_t nb_stack_depth(void *pv_vm) {
    t_VM *vm = pv_vm;
    return vm->psp;
}

void nb_set_pc(void * pv_vm, uint16_t addr) {
    t_VM *vm = pv_vm;
    CPUSH(vm->pc);
    vm->pc = addr;
}

/***************************************************************************************************
  Function:
    nb_run

  Description:
    Run the Tiny Basic Interpreter with the given program.
    Some measures:
    - run nb_run with an empty program: 680 ticks
    - run nb_run with a program with 20 instructions: ~1200 ticks
    => Each instruction needs 26 ticks or 16 ns (for 160 MHz)
    => Currently needs 0x350 bytes of code space

  Parameters:
    program  (IN) - pointer to the program

  Return value:
    Number of executed instructions

***************************************************************************************************/
uint16_t nb_run(void *pv_vm, uint8_t* p_programm, uint16_t len, uint16_t cycles, uint8_t num_vars) {
    uint32_t tmp1, tmp2;
    uint16_t idx;
    uint16_t addr, addr2, size;
    uint16_t offs1, offs2;
    uint16_t size1, size2;
    uint8_t  var, val;
    uint8_t  *ptr;
    char     *str;
    t_VM *vm = pv_vm;
    vm->p_programm = p_programm;

    for(uint16_t i = 0; i < cycles; i++)
    {
#ifdef DEBUG    
        printf("%04X: %02X %02X %02X\n", vm->pc, p_programm[vm->pc], p_programm[vm->pc + 1], p_programm[vm->pc + 2]);
#endif
        switch (p_programm[vm->pc])
        {
        case k_END:
            return NB_END;
        case k_PRINT_STR_N1:
            tmp1 = DPOP();
            PRINTF("%s", STR(tmp1));
            vm->pc += 1;
            break;
        case k_PRINT_VAL_N1:
            PRINTF("%d", DPOP());
            vm->pc += 1;
            break;
        case k_PRINT_NEWL_N1:
            PRINTF("\n");
            vm->pc += 1;
            break;
        case k_PRINT_TAB_N1:
            PRINTF("\t");
            vm->pc += 1;
            break;
        case k_PRINT_SPACE_N1:
            PRINTF(" ");
            vm->pc += 1;
            break;
        case k_PRINT_BLANKS_N1:
            val = DPOP();
            for(uint8_t i = 0; i < val; i++) {
                PRINTF(" ");
            }
            vm->pc += 1;
            break;
        case k_PRINT_LINENO_N3:
            tmp1 = ACS16(p_programm[vm->pc + 1]);
            PRINTF("[%u] ", tmp1);
            vm->pc += 3;
            break;
        case k_PUSH_STR_Nx:
            tmp1 = p_programm[vm->pc + 1]; // string length
            DPUSH(vm->pc + 2);  // push string address
            vm->pc += tmp1 + 2;
            break;
        case k_PUSH_NUM_N5:
            DPUSH(ACS32(p_programm[vm->pc + 1]));
            vm->pc += 5;
            break;
        case k_PUSH_NUM_N2:
            DPUSH(p_programm[vm->pc + 1]);
            vm->pc += 2;
            break;
        case k_PUSH_VAR_N2:
            var = p_programm[vm->pc + 1];
            DPUSH(vm->variables[var]);
            vm->pc += 2;
            break;
        case k_POP_VAR_N2:
            var = p_programm[vm->pc + 1];
            vm->variables[var] = DPOP();
            vm->pc += 2;
            break;
        case k_POP_STR_N2:
            var = p_programm[vm->pc + 1];
            if(vm->variables[var] > 0) {
                nb_mem_free(vm, vm->variables[var]);
            }
            vm->variables[var] = DPOP();
            vm->pc += 2;
            break;
        case k_DIM_ARR_N2:
            var = p_programm[vm->pc + 1];
            size = DPOP();
            addr = nb_mem_alloc(vm, (size + 1) * sizeof(uint32_t));
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return NB_ERROR;
            }
            memset(&vm->heap[addr & 0x7FFF], 0, size * sizeof(uint32_t));
            vm->variables[var] = addr;
            vm->pc += 2;
            break;
        case k_BREAK_INSTR_N1:
            vm->pc += 1; 
            return NB_BREAK;
        case k_ADD_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() + tmp2;
            vm->pc += 1;
            break;
        case k_SUB_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() - tmp2;
            vm->pc += 1;
            break;
        case k_MUL_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() * tmp2;
            vm->pc += 1;
            break;
        case k_DIV_N1:
            tmp2 = DPOP();
            if(tmp2 == 0) {
              PRINTF("Error: Division by zero\n");
              DPUSH(0);
            } else {
              DTOP() = DTOP() / tmp2;
            }
            vm->pc += 1;
            break;
        case k_MOD_N1:
            tmp2 = DPOP();
            if(tmp2 == 0) {
              DPUSH(0);
            } else {
              DTOP() = DTOP() % tmp2;
            }
            vm->pc += 1;
            break;
        case k_AND_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DTOP() = DTOP() && tmp2;
            vm->pc += 1;
            break;
        case k_OR_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() || tmp2;
            vm->pc += 1;
            break;
        case k_NOT_N1:
            DTOP() = !DTOP();
            vm->pc += 1;
            break;
        case k_EQUAL_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() == tmp2;
            vm->pc += 1;
            break;
        case k_NOT_EQUAL_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() != tmp2;
            vm->pc += 1;
            break;
        case k_LESS_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() < tmp2;
            vm->pc += 1;
            break;
        case k_LESS_EQU_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() <= tmp2;
            vm->pc += 1;
            break;
        case k_GREATER_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() > tmp2;
            vm->pc += 1;
            break;
        case k_GREATER_EQU_N1:
            tmp2 = DPOP();
            DTOP() = DTOP() >= tmp2;
            vm->pc += 1;
            break;
        case k_GOTO_N3:
            vm->pc = ACS16(p_programm[vm->pc + 1]);
            break;
        case k_GOSUB_N3:
            CPUSH(vm->pc + 3);
            vm->pc = ACS16(p_programm[vm->pc + 1]);
            break;
        case k_RETURN_N1:
            vm->pc = (uint16_t)CPOP();
            break;
        case k_NEXT_N4:
            // ID = ID + stack[-1]
            // IF ID <= stack[-2] GOTO start
            tmp1 = ACS16(p_programm[vm->pc + 1]);
            var = p_programm[vm->pc + 3];
            vm->variables[var] = vm->variables[var] + DTOP();
            if(vm->variables[var] <= DPEEK(-2)) {
              vm->pc = tmp1;
            } else {
              vm->pc += 4;
              DPOP();  // remove step value
              DPOP();  // remove loop end value
            }
            break;
        case k_IF_N3:
            if(DPOP() == 0) {
              vm->pc = ACS16(p_programm[vm->pc + 1]);
            } else {
              vm->pc += 3;
            }
            break;
#ifdef cfg_ON_COMMANDS
        case k_ON_GOTO_N2:
            idx = DPOP();
            val = p_programm[vm->pc + 1];
            vm->pc += 2;
            if(idx == 0 || idx > val) {
                vm->pc += val * 3;
            } else {
                vm->pc += (idx - 1) * 3;
            }
            break;
        case k_ON_GOSUB_N2:
            idx = DPOP();
            val = p_programm[vm->pc + 1];
            vm->pc += 2;
            if(idx == 0 || idx > val) {
                vm->pc += val * 3;  // skip all addresses
            } else {
                CPUSH(vm->pc + val * 3);  // return address to the next instruction
                vm->pc += (idx - 1) * 3;  // jump to the selected address
            }
            break;
#endif
        case k_SET_ARR_ELEM_N2:
            var = p_programm[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            tmp2 = DPOP() * sizeof(uint32_t);
            if(tmp2 >= nb_mem_get_blocksize(vm, addr)) {
                PRINTF("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            ACS32(vm->heap[addr + tmp2]) = tmp1;
            vm->pc += 2;
            break;
        case k_GET_ARR_ELEM_N2:
            var = p_programm[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP() * sizeof(uint32_t);
            if(tmp1 >= nb_mem_get_blocksize(vm, addr)) {
                PRINTF("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            DPUSH(ACS32(vm->heap[addr + tmp1]));
            vm->pc += 2;
            break;
#ifdef cfg_BYTE_ACCESS            
        case k_SET_ARR_1BYTE_N2:
            var = p_programm[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            tmp2 = DPOP();
            if(tmp2 >= nb_mem_get_blocksize(vm, addr)) {
                PRINTF("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            ACS8(vm->heap[addr + tmp2]) = tmp1;
            vm->pc += 2;
            break;
        case k_GET_ARR_1BYTE_N2:
            var = p_programm[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            if(tmp1 >= nb_mem_get_blocksize(vm, addr)) {
                PRINTF("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            DPUSH(ACS8(vm->heap[addr + tmp1]));
            vm->pc += 2;
            break;
        case k_SET_ARR_2BYTE_N2:
            var = p_programm[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            tmp2 = DPOP();
            if(tmp2 + 1 >= nb_mem_get_blocksize(vm, addr)) {
                PRINTF("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            ACS16(vm->heap[addr + tmp2]) = tmp1;
            vm->pc += 2;
            break;
        case k_GET_ARR_2BYTE_N2:
            var = p_programm[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            if(tmp1 + 1 >= nb_mem_get_blocksize(vm, addr)) {
                PRINTF("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            DPUSH(ACS16(vm->heap[addr + tmp1]));
            vm->pc += 2;
            break;
        case k_SET_ARR_4BYTE_N2:
            var = p_programm[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            tmp2 = DPOP();
            if(tmp2 + 3 >= nb_mem_get_blocksize(vm, addr)) {
                PRINTF("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            ACS32(vm->heap[addr + tmp2]) = tmp1;
            vm->pc += 2;
            break;
        case k_GET_ARR_4BYTE_N2:
            var = p_programm[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            if(tmp1 + 3 >= nb_mem_get_blocksize(vm, addr)) {
                PRINTF("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            DPUSH(ACS32(vm->heap[addr + tmp1]));
            vm->pc += 2;
            break;
        case k_COPY_N1:
            // copy(arr, offs, arr, offs, bytes)
            size = DPOP();  // number of bytes
            offs2 = DPOP();  // source offset
            tmp2 = DPOP() & 0x7FFF;  // source address
            offs1 = DPOP();  // destination offset
            tmp1 = DPOP() & 0x7FFF;  // destination address
            size1 = nb_mem_get_blocksize(vm, tmp1);
            size2 = nb_mem_get_blocksize(vm, tmp2);
            if(size + offs1 > size1 || size + offs2 > size2) {
                PRINTF("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            memcpy(&vm->heap[tmp1 + offs1], &vm->heap[tmp2 + offs2], size);
            vm->pc += 1;
            break;
#endif
        case k_STACK_N1:
            if(vm->psp > 0) {
                    tmp1 = PPOP();
            } else {
                    tmp1 = 0;
            }
            DPUSH(tmp1);
            vm->pc += 1;
            break;
        case k_XFUNC_N2:
            nb_ReturnValue = p_programm[vm->pc + 1];
            vm->pc += 2;
            return NB_XFUNC;
        case k_PUSH_PARAM_N1:
            PPUSH(DPOP());
            vm->pc += 1;
            break;
        case k_ERASE_ARR_N2:
            var = p_programm[vm->pc + 1];
            addr = vm->variables[var];
            if(addr > 0) {
                nb_mem_free(vm, addr);
            }
            vm->variables[var] = 0;
            vm->pc += 2;
            break;
        case k_FREE_N1:
            tmp1 = sizeof(vm->variables) / sizeof(vm->variables[0]);
            PRINTF(" Code: %u/%u, Data: %u/%u, Heap: %u/%u\n", cfg_MAX_CODE_LEN - len, cfg_MAX_CODE_LEN, 
                                                               tmp1 - num_vars, tmp1,
                                                               nb_mem_get_free(vm), cfg_MEM_HEAP_SIZE);
        case k_RND_N1:
            tmp1 = DPOP();
            if(tmp1 == 0) {
                DPUSH(0);
            } else {
                DPUSH(rand() % (tmp1 + 1));
            }
            vm->pc += 1;
            break;
#ifdef cfg_STRING_SUPPORT            
        case k_ADD_STR_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            uint8_t len = strlen(STR(tmp1)) + strlen(STR(tmp2)) + 1;
            addr = nb_mem_alloc(vm, len);
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return NB_ERROR;
            }
            strcpy(&vm->heap[addr & 0x7FFF], STR(tmp1));
            strcat(&vm->heap[addr & 0x7FFF], STR(tmp2));
            DPUSH(addr);
            vm->pc += 1;
            break;
        case k_STR_EQUAL_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) == 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_STR_NOT_EQU_N1 :
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) == 0 ? 0 : 1);
            vm->pc += 1;
            break;
        case k_STR_LESS_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) < 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_STR_LESS_EQU_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) <= 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_STR_GREATER_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) > 0 ? 1 : 0);
            vm->pc += 1;
        case k_STR_GREATER_EQU_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(STR(tmp1), STR(tmp2)) >= 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_LEFT_STR_N1:
            tmp2 = DPOP();  // number of characters
            tmp1 = DPOP();  // string address
            addr = nb_mem_alloc(vm, tmp2 + 1);
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return NB_ERROR;
            }
            strncpy(&vm->heap[addr & 0x7FFF], STR(tmp1), tmp2);
            DPUSH(addr);
            vm->pc += 1;
            break;
        case k_RIGHT_STR_N1:
            tmp2 = DPOP();  // number of characters
            tmp1 = DPOP();  // string address
            addr = nb_mem_alloc(vm, tmp2 + 1);
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return NB_ERROR;
            }
            strncpy(&vm->heap[addr & 0x7FFF], STR(tmp1) + strlen(STR(tmp1)) - tmp2, tmp2);
            DPUSH(addr);
            vm->pc += 1;
            break;
        case k_MID_STR_N1:
            tmp2 = DPOP();  // number of characters
            tmp1 = DPOP();  // start position
            idx = DPOP();   // string address
            addr = nb_mem_alloc(vm, tmp2 + 1);
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return NB_ERROR;
            }
            strncpy(&vm->heap[addr & 0x7FFF], STR(idx) + tmp1, tmp2);
            DPUSH(addr);
            vm->pc += 1;
            break;
        case k_STR_LEN_N1:
            tmp1 = DPOP();
            DPUSH(strlen(STR(tmp1)));
            vm->pc += 1;
            break;
        case k_STR_TO_VAL_N1:
            tmp1 = DPOP();
            DPUSH(atoi(STR(tmp1)));
            vm->pc += 1;
            break;
        case k_VAL_TO_STR_N1:
            tmp1 = DPOP();
            addr = nb_mem_alloc(vm, 14);
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return NB_ERROR;
            }
            sprintf(&vm->heap[addr & 0x7FFF], "%d", tmp1);
            DPUSH(addr);
            vm->pc += 1;
            break;
        case k_VAL_TO_HEX_N1:
            tmp1 = DPOP();
            addr = nb_mem_alloc(vm, 14);
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return NB_ERROR;
            }
            sprintf(&vm->heap[addr & 0x7FFF], "%X", tmp1);
            DPUSH(addr);
            vm->pc += 1;
            break;
        case k_VAL_TO_CHR_N1:
            tmp1 = DPOP();
            addr = nb_mem_alloc(vm, 2);
            if(addr == 0) {
                PRINTF("Error: Out of memory\n");
                return NB_ERROR;
            }
            vm->heap[addr & 0x7FFF] = tmp1;
            vm->heap[(addr + 1) & 0x7FFF] = 0;
            DPUSH(addr);
            vm->pc += 1;
            break;
        case k_INSTR_N1:
            tmp2 = DPOP();  // string address
            tmp1 = DPOP();  // search string
            val = DPOP() - 1;  // start position
            val = MAX(val, 0);
            tmp1 += MIN(val, strlen(STR(tmp1)));
            str = strstr(STR(tmp1), STR(tmp2));
            if(str == NULL) {
                DPUSH(0);
            } else {
                DPUSH(str - STR(tmp1) + val + 1);
            }
            vm->pc += 1;
            break;
#endif
        default:
            PRINTF("Error: unknown opcode '%u'\n", p_programm[vm->pc]);
            return NB_ERROR;
        }
    }
    return NB_BUSY;
}

void nb_destroy(void * pv_vm) {
  free(pv_vm);
}

/***************************************************************************************************
* Static functions
***************************************************************************************************/
