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
    k_PRINTS,             // 01 (print string from stack)
    k_PRINTV,             // 02 (print value from stack)
    k_PRINTNL,            // 03 (print new line)
    k_PRINTT,             // 04 (print tab)
    k_STRING,             // 05 nn S T R I N G 00 (push string address (16 bit))
    k_NUMBER,             // 06 xx xx xx xx (push const value)
    k_BYTENUM,            // 07 xx (push const value)     
    k_VAR,                // 08 idx (push variable)
    k_LET,                // 09 idx (pop variable)
    k_DIM,                // 0A idx xx (pop variable, pop size)
    k_ADD,                // 0B (add two values from stack)
    k_SUB,                // 0C (sub ftwo values rom stack)
    k_MUL,                // 0D (mul two values from stack)
    k_DIV,                // 0E (div two values from stack)
    k_MOD,                // 0F (mod two values from stack)
    k_AND,                // 10 (pop two values from stack)
    k_OR,                 // 11 (pop two values from stack)
    k_NOT,                // 12 (pop one value from stack)
    k_EQU,                // 13 (compare two values from stack)
    k_NEQU,               // 14 (compare two values from stack)
    k_LE,                 // 15 (compare two values from stack)     
    k_LEEQU,              // 16 (compare two values from stack) 
    k_GR,                 // 17 (compare two values from stack)      
    k_GREQU,              // 18 (compare two values from stack)
    k_GOTO,               // 19 (16 bit programm address)
    k_GOSUB,              // 1A (16 bit programm address)
    k_RETURN,             // 1B (pop return address)
    k_NEXT,               // 1C (16 bit programm address), (variable)
    k_IF,                 // 1D xx xx (pop val, END address)
    k_FUNC,               // 1E xx (function call)
    k_S_ADD,              // 1F (add two strings from stack)
    k_S_EQU,              // 20 (compare two values from stack)
    k_S_NEQU,             // 21 (compare two values from stack)
    k_S_LE,               // 22 (compare two values from stack)     
    k_S_LEEQU,            // 23 (compare two values from stack) 
    k_S_GR,               // 24 (compare two values from stack)      
    k_S_GREQU,            // 25 (compare two values from stack)
};

// Function call definitions (in addition to JBI_CMD,... return values)
enum {
    k_LEFTS = 0x10,       // 10 (left$)
    k_RIGHTS,             // 11 (right$)
    k_MIDS,               // 12 (mid$)
    k_LEN,                // 13 (len)
    k_VAL,                // 14 (val)
    k_STRS,               // 15 (str$)
    k_SPC,                // 16 (spc)
    k_SET1,               // 17 xx (array: set one byte)
    k_GET1,               // 18 xx (array: get one byte)
    k_SET2,               // 19 xx (array: set one short)
    k_GET2,               // 1A xx (array: get one short)
    k_SET4,               // 1B xx (array: set one long)
    k_GET4,               // 1C xx (array: get one long)
};

#ifdef DEBUG
char *Opcodes[] = {
    "END",
    "PRINTS",
    "PRINTV",
    "PRINTNL",
    "PRINTT",
    "STRING",
    "NUMBER",
    "BYTENUM",
    "VAR",
    "LET",
    "DIM",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "MOD",
    "AND",
    "OR",
    "NOT",
    "EQUAL",
    "NEQUAL",
    "LESS",
    "LESSEQUAL",
    "GREATER",
    "GREATEREQUAL",
    "GOTO",
    "GOSUB",
    "RETURN",
    "NEXT",
    "IF",
    "FUNC",
    "SET1",
    "GET1",
    "SET2",
    "GET2",
    "SET4",
    "GET4",
};
#endif

typedef struct {
    uint16_t pc;   // Programm counter
    uint8_t  dsp;  // Data stack pointer
    uint8_t  csp;  // Call stack pointer
    uint32_t datastack[k_STACK_SIZE];
    uint32_t callstack[k_STACK_SIZE];
    uint32_t variables[k_NUM_VARS];
    uint16_t str_start_addr;
    uint8_t  compensation;  // To force the alignment of buffers to 4 bytes
    uint8_t  heap[k_MEM_HEAP_SIZE];
} t_VM;

char *jbi_scanner(char *p_in, char *p_out);

void jbi_mem_init(t_VM *p_vm);
uint16_t jbi_mem_alloc(t_VM *p_vm, uint16_t bytes);
void jbi_mem_free(t_VM *p_vm, uint16_t addr);
uint16_t jbi_mem_realloc(t_VM *p_vm, uint16_t addr, uint16_t bytes);
