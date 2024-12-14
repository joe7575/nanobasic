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

#define k_MEM_BLOCK_SIZE    (8)     // Must be a multiple of 4 (real size is MIN_BLOCK_SIZE - 1)
#define k_MEM_FREE_TAG      (0)     // Also used for number of blocks

#define ACS8(x)   *(uint8_t*)&(x)
#define ACS16(x)  *(uint16_t*)&(x)
#define ACS32(x)  *(uint32_t*)&(x)
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#define MAX(a,b)  ((a) > (b) ? (a) : (b))

// Opcode definitions
enum {
    k_END,                // End of programm
    k_PRINT_STR_N1,       // (pop addr from stack)
    k_PRINT_VAL_N1,       // (pop value from stack)
    k_PRINT_NEWL_N1,      // 
    k_PRINT_TAB_N1,       // 
    k_PRINT_SPACE_N1,     // 
    k_PRINT_BLANKS_N1,    // (function spc)
    k_PRINT_LINENO_N3,    // (print line number for debugging purposes)
    k_PUSH_STR_Nx,        // nn S T R I N G 00 (push string address (16 bit))
    k_PUSH_NUM_N5,        // (push 4 byte const value)
    k_PUSH_NUM_N2,        // (push 1 byte const value)     
    k_PUSH_VAR_N2,        // (push variable)
    k_POP_VAR_N2,         // (pop variable)
    k_POP_STR_N2,         // (pop variable)
    k_DIM_ARR_N2,         // (pop variable, pop size)
    k_BREAK_INSTR_N1,     // (break)
    k_ADD_N1,             // (add two values from stack)
    k_SUB_N1,             // (sub two values from stack)
    k_MUL_N1,             // (mul two values from stack)
    k_DIV_N1,             // (div two values from stack)
    k_MOD_N1,             // (mod two values from stack)
    k_AND_N1,             // (pop two values from stack)
    k_OR_N1,              // (pop two values from stack)
    k_NOT_N1,             // (pop one value from stack)
    k_EQUAL_N1,           // (compare two values from stack)
    k_NOT_EQUAL_N1,       // (compare two values from stack)
    k_LESS_N1,            // (compare two values from stack)     
    k_LESS_EQU_N1,        // (compare two values from stack) 
    k_GREATER_N1,         // (compare two values from stack)      
    k_GREATER_EQU_N1,     // (compare two values from stack)
    k_GOTO_N3,            // (16 bit programm address)
    k_GOSUB_N3,           // (16 bit programm address)
    k_RETURN_N1,          // (pop return address)
    k_NEXT_N4,            // (16 bit programm address), (variable)
    k_IF_N3,              // (pop val, END address)
    k_READ_NUM_N4,        // (read const value from DATA section)
    k_RESTORE_N2,         // (restore the read pointer)
#ifdef cfg_ON_COMMANDS
    k_ON_GOTO_N2,         // (on...goto with last number)
    k_ON_GOSUB_N2,        // (on...gosub with last number)
#endif
    k_SET_ARR_ELEM_N2,    // (set array element)
    k_GET_ARR_ELEM_N2,    // (get array element)
#ifdef cfg_BYTE_ACCESS    
    k_SET_ARR_1BYTE_N2,   // (array: set one byte)
    k_GET_ARR_1BYTE_N2,   // (array: get one byte)
    k_SET_ARR_2BYTE_N2,   // (array: set one short)
    k_GET_ARR_2BYTE_N2,   // (array: get one short)
    k_SET_ARR_4BYTE_N2,   // (array: set one long)
    k_GET_ARR_4BYTE_N2,   // (array: get one long)
    k_COPY_N1,            // (copy)
#endif
    k_PARAM_N1,           // (pop and push value)
    k_PARAMS_N1,          // (pop and push string address)
    k_XFUNC_N2,           // (external function call)
    k_PUSH_PARAM_N1,      // (push value to parameter stack)
    k_ERASE_ARR_N2,       // (erase array)
    k_FREE_N1,            // (free memory)
    k_RND_N1,             // (random number)
#ifdef cfg_STRING_SUPPORT
    k_ADD_STR_N1,         // (add two strings from stack)
    k_STR_EQUAL_N1,       // (compare two values from stack)
    k_STR_NOT_EQU_N1,     // (compare two values from stack)
    k_STR_LESS_N1,        // (compare two values from stack)     
    k_STR_LESS_EQU_N1,    // (compare two values from stack) 
    k_STR_GREATER_N1,     // (compare two values from stack)      
    k_STR_GREATER_EQU_N1, // (compare two values from stack)
    k_LEFT_STR_N1,        // (left$)
    k_RIGHT_STR_N1,       // (right$)
    k_MID_STR_N1,         // (mid$)
    k_STR_LEN_N1,         // (len)
    k_STR_TO_VAL_N1,      // (val)
    k_VAL_TO_STR_N1,      // (str$)
    k_VAL_TO_HEX_N1,      // (hex$)
    k_VAL_TO_CHR_N1,      // (chr$)
    k_INSTR_N1,           // (instr)
    k_ALLOC_STR_N1,       // (alloc string)
#endif
};

typedef struct {
    uint16_t code_size; // size of the compiled byte code
    uint16_t num_vars;  // number of used variables
    uint16_t pc;   // Programm counter
    uint8_t  dsp;  // Data stack pointer
    uint8_t  csp;  // Call stack pointer
    uint8_t  psp;  // Parameter stack pointer
    uint32_t datastack[cfg_DATASTACK_SIZE];
    uint32_t callstack[cfg_STACK_SIZE];
    uint32_t paramstack[cfg_STACK_SIZE];
    uint32_t variables[cfg_NUM_VARS];
    uint8_t  code[cfg_MAX_CODE_SIZE];
    uint16_t mem_start_addr;
    uint8_t  heap[cfg_MEM_HEAP_SIZE];
} t_VM;

char *nb_scanner(char *p_in, char *p_out);
void nb_mem_init(t_VM *p_vm);
uint16_t nb_mem_alloc(t_VM *p_vm, uint16_t bytes);
void nb_mem_free(t_VM *p_vm, uint16_t addr);
uint16_t nb_mem_realloc(t_VM *p_vm, uint16_t addr, uint16_t bytes);
uint16_t nb_mem_get_blocksize(t_VM *p_vm, uint16_t addr);
uint16_t nb_mem_get_free(t_VM *p_vm);