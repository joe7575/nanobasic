#define k_TAG           0xBC
#define k_VERSION       0x01

#define k_END           0   // 00
#define k_PRINTS        1   // 01 S T R I N G 00 (constant string)
#define k_PRINTV        2   // 02 (print from stack)
#define k_PRINTNL       3   // 03
#define k_NUMBER        4   // 04 xx xx xx xx (push const value)
#define k_BYTENUM       5   // 05 xx (push const value) 
#define k_VAR           6   // 06 idx (push variable)
#define k_LET           7   // 07 idx (pop variable)
#define k_ADD           8   // 08 (add two values from stack)
#define k_SUB           9   // 09 (sub ftwo values rom stack)
#define k_MUL           10  // 0A (mul two values from stack)
#define k_DIV           11  // 0B (div two values from stack)
#define k_EQUAL         12  // 0C (compare two values from stack)
#define k_NEQUAL        13  // 0D (compare two values from stack)
#define k_LESS          14  // 0E (compare two values from stack)
#define k_LESSEQUAL     15  // 0F (compare two values from stack)
#define k_GREATER       16  // 10 (compare two values from stack)
#define k_GREATEREQUAL  17  // 11 (compare two values from stack)
#define k_GOTO          18  // 12 xx xx (16 bit programm address) 
#define k_GOSUB         19  // 13 xx xx (16 bit programm address) 
#define k_RETURN        20  // 14 (pop return address)
#define k_IF            21  // 15 xx xx (pop val, END address)
#define k_AND           22  // 16 (pop two values from stack)
#define k_OR            23  // 17 (pop two values from stack)
#define k_NOT           24  // 18 (pop one value from stack)
#define k_STRING        25  // 19 xx xx (16 bit string address)

char *TINY_Scanner(char *pc8_in, char *pc8_out);
