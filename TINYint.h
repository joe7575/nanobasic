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
#define k_MUL           10  // 10 (mul two values from stack)
#define k_DIV           11  // 11 (div two values from stack)
#define k_EQUAL         12  // 12 (compare two values from stack)
#define k_NEQUAL        13  // 13 (compare two values from stack)
#define k_LESS          14  // 14 (compare two values from stack)
#define k_LESSEQUAL     15  // 15 (compare two values from stack)
#define k_GREATER       16  // 16 (compare two values from stack)
#define k_GREATEREQUAL  17  // 17 (compare two values from stack)
#define k_GOTO          18  // 18 xx xx (16 bit programm address) 
#define k_GOSUB         19  // 19 xx xx (16 bit programm address) 
#define k_RETURN        20  // 20 (pop return address)
#define k_IF            21  // 21 xx xx (pop val, END address)

char *TINY_Scanner(char *pc8_in, char *pc8_out);
