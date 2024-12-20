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

#define STRBUF   1

#define DPUSH(x) vm->datastack[(uint8_t)(vm->dsp++) % cfg_DATASTACK_SIZE] = x
#define DPOP() vm->datastack[(uint8_t)(--vm->dsp) % cfg_DATASTACK_SIZE]
#define DTOP() vm->datastack[(uint8_t)(vm->dsp - 1) % cfg_DATASTACK_SIZE]
#define DPEEK(x) vm->datastack[(uint8_t)((vm->dsp + x)) % cfg_DATASTACK_SIZE]

#define CPUSH(x) vm->callstack[(uint8_t)(vm->csp++) % cfg_STACK_SIZE] = x
#define CPOP() vm->callstack[(uint8_t)(--vm->csp) % cfg_STACK_SIZE]

#define PPUSH(x) vm->paramstack[(uint8_t)(vm->psp++) % cfg_STACK_SIZE] = x
#define PPOP() vm->paramstack[(uint8_t)(--vm->psp) % cfg_STACK_SIZE]

/***************************************************************************************************
**    static function-prototypes
***************************************************************************************************/
static char *get_string(t_VM *vm, uint16_t addr);

/***************************************************************************************************
**    global functions
***************************************************************************************************/
void nb_reset(void *pv_vm) {
    t_VM *vm = pv_vm;
    vm->pc = 1;
    vm->dsp = 0;
    vm->csp = 0;
    vm->psp = 0;
    memset(vm->variables, 0, sizeof(vm->variables));
    memset(vm->datastack, 0, sizeof(vm->datastack));
    memset(vm->callstack, 0, sizeof(vm->callstack));
    memset(vm->paramstack, 0, sizeof(vm->paramstack));
    memset(vm->heap, 0, sizeof(vm->heap));
    nb_mem_init(vm);
}

/*
** Debug Interface
*/
uint32_t nb_get_number(void *pv_vm, uint8_t var) {
    t_VM *vm = pv_vm;
    if(var >= cfg_NUM_VARS) {
        return 0;
    }
    return vm->variables[var];
}

char *nb_get_string(void *pv_vm, uint8_t var) {
    t_VM *vm = pv_vm;
    if(var >= cfg_NUM_VARS) {
        return 0;
    }
    return get_string(vm, vm->variables[var]);
}

uint32_t nb_get_arr_elem(void *pv_vm, uint8_t var, uint16_t idx) {
    t_VM *vm = pv_vm;
    if(var >= cfg_NUM_VARS) {
        return 0;
    }
    uint16_t addr = vm->variables[var];
    return ACS32(vm->heap[(addr & 0x7FFF) + idx * sizeof(uint32_t)]);
}

/*
** External function interface
*/
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
    strncpy(str, get_string(vm, addr), len);
    return str;
}

void nb_push_str(void *pv_vm, char *str) {
    t_VM *vm = pv_vm;
    if(vm->psp < cfg_STACK_SIZE) {
        uint16_t len = strlen(str);
        uint16_t addr = nb_mem_alloc(vm, len + 1);
        if(addr == 0) {
            nb_print("Error: Out of memory\n");
            return;
        }
        strcpy((char*)&vm->heap[addr & 0x7FFF], str);
        PPUSH(addr);
    }
}

uint16_t nb_pop_arr_addr(void *pv_vm) {
    t_VM *vm = pv_vm;
    if(vm->psp == 0) {
        return 0;
    }
    return (uint16_t)PPOP();
}

uint16_t nb_read_arr(void *pv_vm, uint16_t addr, uint8_t *arr, uint16_t bytes) {
    t_VM *vm = pv_vm;
    if(addr < 0x8000) {
        memset(arr, 0, bytes);
        return 0;
    }
    uint16_t size = nb_mem_get_blocksize(vm, addr);
    if(size == 0) {
        memset(arr, 0, bytes);
        return 0;
    }
    size = MIN(size, bytes);
    memcpy(arr, &vm->heap[addr & 0x7FFF], size);
    return size;
}

uint16_t nb_write_arr(void *pv_vm, uint16_t addr, uint8_t *arr, uint16_t bytes) {
    t_VM *vm = pv_vm;
    if(addr < 0x8000) {
        return 0;
    }
    uint16_t size = nb_mem_get_blocksize(vm, addr);
    if(size == 0) {
        return 0;
    }
    size = MIN(size, bytes);
    memcpy(&vm->heap[addr & 0x7FFF], arr, size);
    return size;
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

/*
** Run the programm
*/
uint16_t nb_run(void *pv_vm, uint16_t *p_cycles) {
    uint32_t tmp1, tmp2;
    uint16_t idx;
    uint16_t addr, size;
    uint16_t offs1;
#ifdef cfg_BYTE_ACCESS
    uint16_t offs2, size1, size2;
#endif
    uint8_t  var, val;
#ifdef cfg_STRING_SUPPORT
    char     *str;
#endif
    t_VM *vm = pv_vm;

    while((*p_cycles)-- > 1)
    {
        switch (vm->code[vm->pc])
        {
        case k_END:
            return NB_END;
        case k_PRINT_STR_N1:
            tmp1 = DPOP();
            nb_print("%s", get_string(vm, tmp1));
            vm->pc += 1;
            break;
        case k_PRINT_VAL_N1:
            nb_print("%d", DPOP());
            vm->pc += 1;
            break;
        case k_PRINT_NEWL_N1:
            nb_print("\n");
            vm->pc += 1;
            break;
        case k_PRINT_TAB_N1:
            nb_print("\t");
            vm->pc += 1;
            break;
        case k_PRINT_SPACE_N1:
            nb_print(" ");
            vm->pc += 1;
            break;
        case k_PRINT_BLANKS_N1:
            val = DPOP();
            for(uint8_t i = 0; i < val; i++) {
                nb_print(" ");
            }
            vm->pc += 1;
            break;
        case k_PRINT_LINENO_N3:
            tmp1 = ACS16(vm->code[vm->pc + 1]);
            nb_print("[%u] ", tmp1);
            vm->pc += 3;
            break;
        case k_PUSH_STR_Nx:
            tmp1 = vm->code[vm->pc + 1]; // string length
            DPUSH(vm->pc + 2);  // push string address
            vm->pc += tmp1 + 2;
            break;
        case k_PUSH_NUM_N5:
            DPUSH(ACS32(vm->code[vm->pc + 1]));
            vm->pc += 5;
            break;
        case k_PUSH_NUM_N2:
            DPUSH(vm->code[vm->pc + 1]);
            vm->pc += 2;
            break;
        case k_PUSH_VAR_N2:
            var = vm->code[vm->pc + 1];
            DPUSH(vm->variables[var]);
            vm->pc += 2;
            break;
        case k_POP_VAR_N2:
            var = vm->code[vm->pc + 1];
            vm->variables[var] = DPOP();
            vm->pc += 2;
            break;
        case k_POP_STR_N2:
            var = vm->code[vm->pc + 1];
            if(vm->variables[var] > 0x7FFF) {
                nb_mem_free(vm, vm->variables[var]);
            }
            vm->variables[var] = DPOP();
            vm->pc += 2;
            break;
        case k_DIM_ARR_N2:
            var = vm->code[vm->pc + 1];
            size = DPOP();
            addr = nb_mem_alloc(vm, (size + 1) * sizeof(uint32_t));
            if(addr == 0) {
                nb_print("Error: Out of memory\n");
                return NB_ERROR;
            }
            memset(&vm->heap[addr & 0x7FFF], 0, size * sizeof(uint32_t));
            vm->variables[var] = addr;
            vm->pc += 2;
            break;
        case k_BREAK_INSTR_N3:
            tmp1 = ACS16(vm->code[vm->pc + 1]);
            PPUSH(tmp1);
            vm->pc += 3; 
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
              nb_print("Error: Division by zero\n");
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
            vm->pc = ACS16(vm->code[vm->pc + 1]);
            break;
        case k_GOSUB_N3:
        if(vm->csp < cfg_STACK_SIZE) {
                CPUSH(vm->pc + 3);
                vm->pc = ACS16(vm->code[vm->pc + 1]);
            } else {
                nb_print("Error: Call stack overflow\n");
                return NB_ERROR;
            }
            break;
        case k_RETURN_N1:
            vm->pc = (uint16_t)CPOP();
            break;
        case k_NEXT_N4:
            // ID = ID + stack[-1]
            // IF ID <= stack[-2] GOTO start
            tmp1 = ACS16(vm->code[vm->pc + 1]);
            var = vm->code[vm->pc + 3];
            vm->variables[var] = vm->variables[var] + DTOP();
            if(vm->variables[var] <= DPEEK(-2)) {
              vm->pc = tmp1;
            } else {
              vm->pc += 4;
              (void)DPOP();  // remove step value
              (void)DPOP();  // remove loop end value
            }
            break;
        case k_IF_N3:
            if(DPOP() == 0) {
              vm->pc = ACS16(vm->code[vm->pc + 1]);
            } else {
              vm->pc += 3;
            }
            break;
        case k_READ_NUM_N4:
            var = vm->code[vm->pc + 1];
            addr = ACS16(vm->code[vm->pc + 2]);
            offs1 = vm->variables[var];
            if(addr + offs1 + 4 > vm->code_size) {
                nb_print("Error: Data address out of bounds\n");
                return NB_ERROR;
            }
            DPUSH(ACS32(vm->code[addr + offs1]));
            vm->variables[var] += 4;
            vm->pc += 4;
            break;
        case k_READ_STR_N4:
            var = vm->code[vm->pc + 1];
            addr = ACS16(vm->code[vm->pc + 2]);
            offs1 = vm->variables[var] & 0xFFFF;
            if(addr + offs1 + 4 > vm->code_size) {
                nb_print("Error: Data address out of bounds\n");
                return NB_ERROR;
            }
            DPUSH(ACS32(vm->code[addr + offs1]));
            vm->variables[var] += 4;
            vm->pc += 4;
            break;
        case k_RESTORE_N2:
            var = vm->code[vm->pc + 1];
            offs1 = DPOP() * sizeof(uint32_t);
            vm->variables[var] = offs1;
            vm->pc += 2;
            break;
#ifdef cfg_ON_COMMANDS
        case k_ON_GOTO_N2:
            idx = DPOP();
            val = vm->code[vm->pc + 1];
            vm->pc += 2;
            if(idx == 0 || idx > val) {
                vm->pc += val * 3;
            } else {
                vm->pc += (idx - 1) * 3;
            }
            break;
        case k_ON_GOSUB_N2:
            idx = DPOP();
            val = vm->code[vm->pc + 1];
            vm->pc += 2;
            if(idx == 0 || idx > val) {
                vm->pc += val * 3;  // skip all addresses
            } else {
                if(vm->csp < cfg_STACK_SIZE) {
                    CPUSH(vm->pc + val * 3);  // return address to the next instruction
                    vm->pc += (idx - 1) * 3;  // jump to the selected address
                } else {
                    nb_print("Error: Call stack overflow\n");
                    return NB_ERROR;
                }
            }
            break;
#endif
        case k_SET_ARR_ELEM_N2:
            var = vm->code[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            tmp2 = DPOP() * sizeof(uint32_t);
            if(tmp2 >= nb_mem_get_blocksize(vm, addr)) {
                nb_print("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            ACS32(vm->heap[addr + tmp2]) = tmp1;
            vm->pc += 2;
            break;
        case k_GET_ARR_ELEM_N2:
            var = vm->code[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP() * sizeof(uint32_t);
            if(tmp1 >= nb_mem_get_blocksize(vm, addr)) {
                nb_print("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            DPUSH(ACS32(vm->heap[addr + tmp1]));
            vm->pc += 2;
            break;
#ifdef cfg_BYTE_ACCESS            
        case k_SET_ARR_1BYTE_N2:
            var = vm->code[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            tmp2 = DPOP();
            if(tmp2 >= nb_mem_get_blocksize(vm, addr)) {
                nb_print("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            ACS8(vm->heap[addr + tmp2]) = tmp1;
            vm->pc += 2;
            break;
        case k_GET_ARR_1BYTE_N2:
            var = vm->code[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            if(tmp1 >= nb_mem_get_blocksize(vm, addr)) {
                nb_print("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            DPUSH(ACS8(vm->heap[addr + tmp1]));
            vm->pc += 2;
            break;
        case k_SET_ARR_2BYTE_N2:
            var = vm->code[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            tmp2 = DPOP();
            if(tmp2 + 1 >= nb_mem_get_blocksize(vm, addr)) {
                nb_print("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            ACS16(vm->heap[addr + tmp2]) = tmp1;
            vm->pc += 2;
            break;
        case k_GET_ARR_2BYTE_N2:
            var = vm->code[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            if(tmp1 + 1 >= nb_mem_get_blocksize(vm, addr)) {
                nb_print("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            DPUSH(ACS16(vm->heap[addr + tmp1]));
            vm->pc += 2;
            break;
        case k_SET_ARR_4BYTE_N2:
            var = vm->code[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            tmp2 = DPOP();
            if(tmp2 + 3 >= nb_mem_get_blocksize(vm, addr)) {
                nb_print("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            ACS32(vm->heap[addr + tmp2]) = tmp1;
            vm->pc += 2;
            break;
        case k_GET_ARR_4BYTE_N2:
            var = vm->code[vm->pc + 1];
            addr = vm->variables[var] & 0x7FFF;
            tmp1 = DPOP();
            if(tmp1 + 3 >= nb_mem_get_blocksize(vm, addr)) {
                nb_print("Error: Array index out of bounds\n");
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
                nb_print("Error: Array index out of bounds\n");
                return NB_ERROR;
            }
            memcpy(&vm->heap[tmp1 + offs1], &vm->heap[tmp2 + offs2], size);
            vm->pc += 1;
            break;
#endif
        case k_PARAM_N1:
        case k_PARAMS_N1:
            if(vm->psp > 0) {
                    tmp1 = PPOP();
            } else {
                    tmp1 = 0;
            }
            DPUSH(tmp1);
            vm->pc += 1;
            break;
        case k_XFUNC_N2:
            val = vm->code[vm->pc + 1];
            vm->pc += 2;
            return NB_XFUNC + val;
        case k_PUSH_PARAM_N1:
            PPUSH(DPOP());
            vm->pc += 1;
            break;
        case k_ERASE_ARR_N2:
            var = vm->code[vm->pc + 1];
            addr = vm->variables[var];
            if(addr > 0x7FFF) {
                nb_mem_free(vm, addr);
            }
            vm->variables[var] = 0;
            vm->pc += 2;
            break;
        case k_FREE_N1:
            nb_print(" %u/%u/%u bytes free (code/data/heap)\n", cfg_MAX_CODE_SIZE - vm->code_size,
                sizeof(vm->variables) - (vm->num_vars * sizeof(uint32_t)), nb_mem_get_free(vm));
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
            uint16_t len = strlen(get_string(vm, tmp1)) + strlen(get_string(vm, tmp2)) + 1;
            addr = nb_mem_alloc(vm, len);
            if(addr == 0) {
                nb_print("Error: Out of memory\n");
                return NB_ERROR;
            }
            strcpy((char*)&vm->heap[addr & 0x7FFF], get_string(vm, tmp1));
            strcat((char*)&vm->heap[addr & 0x7FFF], get_string(vm, tmp2));
            DPUSH(addr);
            vm->pc += 1;
            break;
        case k_STR_EQUAL_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(get_string(vm, tmp1), get_string(vm, tmp2)) == 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_STR_NOT_EQU_N1 :
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(get_string(vm, tmp1), get_string(vm, tmp2)) == 0 ? 0 : 1);
            vm->pc += 1;
            break;
        case k_STR_LESS_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(get_string(vm, tmp1), get_string(vm, tmp2)) < 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_STR_LESS_EQU_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(get_string(vm, tmp1), get_string(vm, tmp2)) <= 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_STR_GREATER_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(get_string(vm, tmp1), get_string(vm, tmp2)) > 0 ? 1 : 0);
            vm->pc += 1;
        case k_STR_GREATER_EQU_N1:
            tmp2 = DPOP();
            tmp1 = DPOP();
            DPUSH(strcmp(get_string(vm, tmp1), get_string(vm, tmp2)) >= 0 ? 1 : 0);
            vm->pc += 1;
            break;
        case k_LEFT_STR_N1:
            tmp2 = DPOP();  // number of characters
            tmp1 = DPOP();  // string address
            strncpy(vm->strbuf, get_string(vm, tmp1), tmp2);
            DPUSH(STRBUF);
            vm->pc += 1;
            break;
        case k_RIGHT_STR_N1:
            tmp2 = DPOP();  // number of characters
            tmp1 = DPOP();  // string address
            strncpy(vm->strbuf, get_string(vm, tmp1) + strlen(get_string(vm, tmp1)) - tmp2, tmp2);
            DPUSH(STRBUF);
            vm->pc += 1;
            break;
        case k_MID_STR_N1:
            tmp2 = DPOP();  // number of characters
            tmp1 = DPOP();  // start position
            idx = DPOP();   // string address
            strncpy(vm->strbuf, get_string(vm, idx) + tmp1, tmp2);
            DPUSH(STRBUF);
            vm->pc += 1;
            break;
        case k_STR_LEN_N1:
            tmp1 = DPOP();
            DPUSH(strlen(get_string(vm, tmp1)));
            vm->pc += 1;
            break;
        case k_STR_TO_VAL_N1:
            tmp1 = DPOP();
            DPUSH(atoi(get_string(vm, tmp1)));
            vm->pc += 1;
            break;
        case k_VAL_TO_STR_N1:
            tmp1 = DPOP();
            snprintf(vm->strbuf, sizeof(vm->strbuf), "%d", tmp1);
            DPUSH(STRBUF);
            vm->pc += 1;
            break;
        case k_VAL_TO_HEX_N1:
            tmp1 = DPOP();
            snprintf(vm->strbuf, sizeof(vm->strbuf), "%X", tmp1);
            DPUSH(STRBUF);
            vm->pc += 1;
            break;
        case k_INSTR_N1:
            tmp2 = DPOP();  // string address
            tmp1 = DPOP();  // search string
            val = DPOP() - 1;  // start position
            val = MAX(val, 0);
            tmp1 += MIN(val, strlen(get_string(vm, tmp1)));
            str = strstr(get_string(vm, tmp1), get_string(vm, tmp2));
            if(str == NULL) {
                DPUSH(0);
            } else {
                DPUSH(str - get_string(vm, tmp1) + val + 1);
            }
            vm->pc += 1;
            break;
#endif
#if defined(cfg_BASIC_V2) || defined(cfg_STRING_SUPPORT)
        case k_ALLOC_STR_N1:
            tmp2 = DPOP();  // fill char
            tmp1 = DPOP();  // string length
            addr = nb_mem_alloc(vm, tmp1 + 1);
            if(addr == 0) {
                nb_print("Error: Out of memory\n");
                return NB_ERROR;
            }
            for(uint16_t i = 0; i < tmp1; i++) {
                vm->heap[(addr & 0x7fff) + i] = tmp2;
            }
            vm->heap[(addr & 0x7fff) + tmp1] = 0;
            DPUSH(addr);
            vm->pc += 1;
            break;
#endif
        default:
            nb_print("Error: unknown opcode '%u'\n", vm->code[vm->pc]);
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
static char *get_string(t_VM *vm, uint16_t addr) {
    if(addr >= 0x8000) {
        if(vm->heap[addr & 0x7FFF] == 0) {
            return "";
        }
        return (char*)&vm->heap[addr & 0x7FFF];
    } else if(addr == 0) {
        return "";
    } else if(addr == STRBUF) {
        return vm->strbuf;
    } else {
        if(vm->code[addr] == 0) {
            return "";
        }
        return (char*)&vm->code[addr];
    }
}
