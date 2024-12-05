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

#define k_TAG           0xBC
#define k_VERSION       0x01

#define k_STACK_SIZE        (16)
#define k_NUM_VARS          (255)
#define k_MEM_BLOCK_SIZE    (16)    // Must be a multiple of 4 (real size is MIN_BLOCK_SIZE - 1)
#define k_MEM_FREE_TAG      (0)     // Also used for number of blocks
#define k_MEM_HEAP_SIZE     (1024 * 16) // 16k

#define ACS8(x)   *(uint8_t*)&(x)
#define ACS16(x)  *(uint16_t*)&(x)
#define ACS32(x)  *(uint32_t*)&(x)


// Opcode definitions
enum {
    k_END,                // 00 End of programm
    k_PRINT_STR_N1,       // 01 (pop addr from stack)
    k_PRINT_VAL_N1,       // 02 (pop value from stack)
    k_PRINT_NEWL_N1,      // 03
    k_PRINT_TAB_N1,       // 04
    k_PRINT_BLANKS_N1,    // 05 (function spc)
    k_PUSH_STR_Nx,        // 06 nn S T R I N G 00 (push string address (16 bit))
    k_PUSH_NUM_N5,        // 07 (push 4 byte const value)
    k_PUSH_NUM_N2,        // 08 (push 1 byte const value)     
    k_PUSH_VAR_N2,        // 09 (push variable)
    k_POP_VAR_N2,         // 0A (pop variable)
    k_POP_STR_N2,         // 0B (pop variable)
    k_DIM_ARR_N2,         // 0C (pop variable, pop size)
    k_BREAK_INSTR_N1,     // 0D (break)
    k_ADD_N1,             // 0E (add two values from stack)
    k_SUB_N1,             // 0F (sub ftwo values rom stack)
    k_MUL_N1,             // 10 (mul two values from stack)
    k_DIV_N1,             // 11 (div two values from stack)
    k_MOD_N1,             // 12 (mod two values from stack)
    k_AND_N1,             // 13 (pop two values from stack)
    k_OR_N1,              // 14 (pop two values from stack)
    k_NOT_N1,             // 15 (pop one value from stack)
    k_EQUAL_N1,           // 16 (compare two values from stack)
    k_NOT_EQUAL_N1,       // 17 (compare two values from stack)
    k_LESS_N1,            // 18 (compare two values from stack)     
    k_LESS_EQU_N1,        // 19 (compare two values from stack) 
    k_GREATER_N1,         // 1A (compare two values from stack)      
    k_GREATER_EQU_N1,     // 1B (compare two values from stack)
    k_GOTO_N2,            // 1C (16 bit programm address)
    k_GOSUB_N2,           // 1D (16 bit programm address)
    k_RETURN_N1,          // 1E (pop return address)
    k_NEXT_N4,            // 1F (16 bit programm address), (variable)
    k_IF_N2,              // 20 (pop val, END address)
    k_SET_ARR_ELEM_N2,    // 21 (set array element)
    k_GET_ARR_ELEM_N2,    // 22 (get array element)
    k_SET_ARR_1BYTE_N2,   // 23 (array: set one byte)
    k_GET_ARR_1BYTE_N2,   // 24 (array: get one byte)
    k_SET_ARR_2BYTE_N2,   // 25 (array: set one short)
    k_GET_ARR_2BYTE_N2,   // 26 (array: get one short)
    k_SET_ARR_4BYTE_N2,   // 27 (array: set one long)
    k_GET_ARR_4BYTE_N2,   // 28 (array: get one long)
    k_COPY_N1,            // 29 (copy)
    k_FUNC_CALL,          // 2A (function call)
    k_ADD_STR_N1,         // 2B (add two strings from stack)
    k_STR_EQUAL_N1,       // 2C (compare two values from stack)
    k_STR_NOT_EQU_N1,     // 2D (compare two values from stack)
    k_STR_LESS_N1,        // 2E (compare two values from stack)     
    k_STR_LESS_EQU_N1,    // 2F (compare two values from stack) 
    k_STR_GREATER_N1,     // 30 (compare two values from stack)      
    k_STR_GREATER_EQU_N1, // 31 (compare two values from stack)
};
// #if k_STR_GREATER_EQU_N1 != 0x31
//     #error "Opcode definitions are not correct" k_STR_GREATER_EQU_N1  
// #endif

// Function call definitions
enum {
    k_CALL_CMD_N2,        // 00 (call command)
    k_LEFT_STR_N2,        // 02 (left$)
    k_RIGHT_STR_N2,       // 03 (right$)
    k_MID_STR_N2,         // 04 (mid$)
    k_STR_LEN_N2,         // 05 (len)
    k_STR_TO_VAL_N2,      // 06 (val)
    k_VAL_TO_STR_N2,      // 07 (str$)
};

#ifdef DEBUG
char *Opcodes[] = {
    "k_END",
    "k_PRINT_STR_N1",
    "k_PRINT_VAL_N1",
    "k_PRINT_NEWL_N1",
    "k_PRINT_TAB_N1",
    "k_PRINT_BLANKS_N1",
    "k_PUSH_STR_Nx",
    "k_PUSH_NUM_N5",
    "k_PUSH_NUM_N2",
    "k_PUSH_VAR_N2",
    "k_POP_VAR_N2",
    "k_POP_STR_N2",
    "k_DIM_ARR_N2",
    "k_BREAK_INSTR_N1",
    "k_ADD_N1",
    "k_SUB_N1",
    "k_MUL_N1",
    "k_DIV_N1",
    "k_MOD_N1",
    "k_AND_N1",
    "k_OR_N1",
    "k_NOT_N1",
    "k_EQUAL_N1",
    "k_NOT_EQUAL_N1",
    "k_LESS_N1",
    "k_LESS_EQU_N1",
    "k_GREATER_N1",
    "k_GREATER_EQU_N1",
    "k_GOTO_N2",
    "k_GOSUB_N2",
    "k_RETURN_N1",
    "k_NEXT_N4",
    "k_IF_N2",
    "k_SET_ARR_ELEM_N2",
    "k_GET_ARR_ELEM_N2",
    "k_SET_ARR_1BYTE_N2",
    "k_GET_ARR_1BYTE_N2",
    "k_SET_ARR_2BYTE_N2",
    "k_GET_ARR_2BYTE_N2",
    "k_SET_ARR_4BYTE_N2",
    "k_GET_ARR_4BYTE_N2",
    "",
    "k_FUNC_CALL",
    "k_ADD_STR_N1",
    "k_STR_EQUAL_N1",
    "k_STR_NOT_EQU_N1",
    "k_STR_LESS_N1",
    "k_STR_LESS_EQU_N1",
    "k_STR_GREATER_N1",
    "k_STR_GREATER_EQU_N1",
};

assert(sizeof(Opcodes) / sizeof(Opcodes[0]) == k_STR_GREATER_EQU_N1 + 1);
#endif


typedef struct {
    uint16_t pc;   // Programm counter
    uint8_t  dsp;  // Data stack pointer
    uint8_t  csp;  // Call stack pointer
    uint32_t datastack[k_STACK_SIZE];
    uint32_t callstack[k_STACK_SIZE];
    uint32_t variables[k_NUM_VARS];
    uint16_t str_start_addr;
    uint8_t  heap[k_MEM_HEAP_SIZE];
} t_VM;

char *jbi_scanner(char *p_in, char *p_out);

void jbi_mem_init(t_VM *p_vm);
uint16_t jbi_mem_alloc(t_VM *p_vm, uint16_t bytes);
void jbi_mem_free(t_VM *p_vm, uint16_t addr);
uint16_t jbi_mem_realloc(t_VM *p_vm, uint16_t addr, uint16_t bytes);
uint16_t jbi_mem_get_blocksize(t_VM *p_vm, uint16_t addr);