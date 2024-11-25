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

#define k_STACK_SIZE    8

// Opcode definitions
enum {
  k_END,                // End of programm
  k_PRINTS,             // (print string from stack)
  k_PRINTV,             // (print value from stack)
  k_PRINTNL,            // (print new line)
  k_PRINTT,             // (print tab)
  k_STRING,             // xx xx (push 16 bit string address)
  k_NUMBER,             // xx xx xx xx (push const value)
  k_BYTENUM,            // xx (push const value)     
  k_VAR,                // idx (push variable)
  k_LET,                // idx (pop variable)
  k_ADD,                // (add two values from stack)
  k_SUB,                // (sub ftwo values rom stack)
  k_MUL,                // (mul two values from stack)
  k_DIV,                // (div two values from stack)
  k_MOD,                // (mod two values from stack)
  k_AND,                // (pop two values from stack)
  k_OR,                 // (pop two values from stack)
  k_NOT,                // (pop one value from stack)
  k_EQUAL,              // (compare two values from stack)
  k_NEQUAL,             // (compare two values from stack)
  k_LESS,               // (compare two values from stack)     
  k_LESSEQUAL,          // (compare two values from stack) 
  k_GREATER,            // (compare two values from stack)      
  k_GREATEREQUAL,       // (compare two values from stack)
  k_GOTO,               // (16 bit programm address)
  k_GOSUB,              // (16 bit programm address)
  k_RETURN,             // (pop return address)
  k_NEXT,               // (16 bit programm address), (variable)
  k_IF,                 // xx xx (pop val, END address)
  k_FUNC,               // xx (function call)
  k_VECTS1,             // xx (vector: set one byte)
  k_VECTG1,             // xx (vector: get one byte)
  k_VECTS2,             // xx (vector: set one short)
  k_VECTG2,             // xx (vector: get one short)
  k_VECTS4,             // xx (vector: set one long)
  k_VECTG4,             // xx (vector: get one long)
};

// List of functions
enum {
  k_PEEK = 1,
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
  "VECTS1",
  "VECTG1",
};
#endif

typedef struct {
  uint16_t pc;   // Programm counter
  uint8_t  dsp;  // Data stack pointer
  uint8_t  csp;  // Call stack pointer
  uint32_t datastack[k_STACK_SIZE];
  uint32_t callstack[k_STACK_SIZE];
  uint32_t variables[1];
} t_VM;

char *jbi_scanner(char *p_in, char *p_out);
