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
#define k_BUFF_SIZE         (128)
#define k_NUM_BUFF          (2)
#define k_NUM_VARS          (64)
#define k_STR_BLOCK_SIZE    (16)    // Must be a multiple of 4 (real size is MIN_BLOCK_SIZE - 1)
#define k_STR_FREE_TAG      (0)     // Also used for number of blocks
#define k_STR_HEAP_SIZE     (1024)

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
    k_ADD,                // 0A (add two values from stack)
    k_SUB,                // 0B (sub ftwo values rom stack)
    k_MUL,                // 0C (mul two values from stack)
    k_DIV,                // 0D (div two values from stack)
    k_MOD,                // 0E (mod two values from stack)
    k_AND,                // 0F (pop two values from stack)
    k_OR,                 // 10 (pop two values from stack)
    k_NOT,                // 11 (pop one value from stack)
    k_EQUAL,              // 12 (compare two values from stack)
    k_NEQUAL,             // 13 (compare two values from stack)
    k_LESS,               // 14 (compare two values from stack)     
    k_LESSEQUAL,          // 15 (compare two values from stack) 
    k_GREATER,            // 16 (compare two values from stack)      
    k_GREATEREQUAL,       // 17 (compare two values from stack)
    k_GOTO,               // 18 (16 bit programm address)
    k_GOSUB,              // 19 (16 bit programm address)
    k_RETURN,             // 1A (pop return address)
    k_NEXT,               // 1B (16 bit programm address), (variable)
    k_IF,                 // 1C xx xx (pop val, END address)
    k_FUNC,               // 1D xx (function call)
    k_BUFF_S1,            // 1E xx (buffer: set one byte)
    k_BUFF_G1,            // 1F xx (buffer: get one byte)
    k_BUFF_S2,            // 20 xx (buffer: set one short)
    k_BUFF_G2,            // 21 xx (buffer: get one short)
    k_BUFF_S4,            // 22 xx (buffer: set one long)
    k_BUFF_G4,            // 23 xx (buffer: get one long)
    k_SADD,               // 24 (add two strings from stack)
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
    "BUFF_S1",
    "BUFF_G1",
    "BUFF_S2",
    "BUFF_G2",
    "BUFF_S4",
    "BUFF_G4",
};
#endif

typedef struct {
    uint16_t pc;   // Programm counter
    uint8_t  dsp;  // Data stack pointer
    uint8_t  csp;  // Call stack pointer
    uint32_t datastack[k_STACK_SIZE];
    uint32_t callstack[k_STACK_SIZE];
    uint8_t  buffer[k_NUM_BUFF][k_BUFF_SIZE];
    uint32_t variables[k_NUM_VARS];
    uint16_t str_start_addr;
    uint8_t  compensation;  // To force the alignment of buffers to 4 bytes
    uint8_t  string_heap[k_STR_HEAP_SIZE];
} t_VM;

char *jbi_scanner(char *p_in, char *p_out);

void jbi_str_init(t_VM *p_vm);
uint16_t jbi_str_alloc(t_VM *p_vm, uint16_t bytes);
void jbi_str_free(t_VM *p_vm, uint16_t addr);
uint16_t jbi_str_realloc(t_VM *p_vm, uint16_t addr, uint16_t bytes);
