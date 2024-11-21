/***************************************************************************************************
**    Copyright (C) 2024 HMS Technology Center Ravensburg GmbH, all rights reserved
****************************************************************************************************
**
**        File: TINYmain.c
**     Summary: 
**      Author: Joachim Stolberg
** Responsible: (optional)
**
****************************************************************************************************
****************************************************************************************************
**
**   Functions: TINY_Run
**              TINY_Hex2Bin
**
****************************************************************************************************
**    Template Version 4
***************************************************************************************************/

/***************************************************************************************************
**    include-files
***************************************************************************************************/
#include "IXXtypes.h"
#include "IXXtarget.h"
#include "IXXmacros.h"
#include "TINY.h"

// PRINTF via UART
#if defined(_WIN32) || defined(WIN32)
  #define PRINTF printf
#else
  #include "fsl_debug_console.h"
  #include "board.h"
#endif
/***************************************************************************************************
**    global variables
***************************************************************************************************/

/***************************************************************************************************
**    static constants, types, macros, variables
***************************************************************************************************/

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

#define STACK_SIZE      8
#define NUM_VARS        10

#define DPUSH(x) datastack[dsp++ % STACK_SIZE] = x
#define DPOP() datastack[--dsp % STACK_SIZE]
#define CPUSH(x) callstack[csp++ % STACK_SIZE] = x
#define CPOP() callstack[--csp % STACK_SIZE]

#define CHAR(x) (((x) - 0x30) & 0x1F)

/***************************************************************************************************
**    static function-prototypes
***************************************************************************************************/

/***************************************************************************************************
**    global functions
***************************************************************************************************/

/***************************************************************************************************
  Function:
    TINY_Run

  Description:
    Run the Tiny Basic Interpreter with the given program.
    Some measures:
    - run TINY_Run with an empty program: 680 ticks
    - run TINY_Run with a program with 20 instructions: ~1200 ticks
    => Each instruction needs 26 ticks or 16 ns (for 160 MHz)
    => Currently needs 0x350 bytes of code space

  Parameters:
    program  (IN) - pointer to the program

  Return value:
    Number of executed instructions

***************************************************************************************************/
IXX_U16 TINY_Run(IXX_U8* pu8_programm)
{
  IXX_U32 variables[NUM_VARS] = { 0 };
  IXX_U32 datastack[STACK_SIZE] = { 0 };
  IXX_U32 callstack[STACK_SIZE] = { 0 };
  IXX_U32 tmp1, tmp2;
  IXX_U16 pc = 1;
  IXX_U16 cnt = 0;
  IXX_U8  dsp = 0;
  IXX_U8  csp = 0;
  IXX_U8  var;

  assert(pu8_programm[0] == k_TAG);
  assert(pu8_programm[1] == k_VERSION);
  pc = 3;
  while(1)
  {
    //printf("%04X: %02X\n", pc, pu8_programm[pc]);
    switch (pu8_programm[pc])
    {
      case k_END:
        return cnt;
      case k_PRINTS:
        PRINTF("%s ", &pu8_programm[pc + 1]);
        pc += strlen((char*)&pu8_programm[pc + 1]) + 2;
        break;
      case k_PRINTV:
        PRINTF("%d ", DPOP());
        pc += 1;
        break;
      case k_PRINTNL:
        PRINTF("\b\n");
        pc += 1;
        break;
      case k_NUMBER:
        DPUSH(*(IXX_U32*)&pu8_programm[pc + 1]);
        pc += 5;
        break;
      case k_BYTENUM:
        DPUSH(pu8_programm[pc + 1]);
        pc += 2;
        break;
      case k_VAR:
        var = pu8_programm[pc + 1];
        DPUSH(variables[var % NUM_VARS]);
        pc += 2;
        break;
      case k_LET:
        var = pu8_programm[pc + 1];
        variables[var % NUM_VARS] = DPOP();
        pc += 2;
        break;
      case k_ADD:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 + tmp2);
        pc += 1;
        break;
      case k_SUB:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 - tmp2);
        pc += 1;
        break;
      case k_MUL:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 * tmp2);
        pc += 1;
        break;
      case k_DIV:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 / tmp2);
        pc += 1;
        break;
      case k_EQUAL:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 == tmp2);
        pc += 1;
        break;
      case k_NEQUAL :
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 != tmp2);
        pc += 1;
        break;
      case k_LESS:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 < tmp2);
        pc += 1;
        break;
      case k_LESSEQUAL:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 <= tmp2);
        pc += 1;
        break;
      case k_GREATER:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 > tmp2);
        pc += 1;
        break;
      case k_GREATEREQUAL:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 >= tmp2);
        pc += 1;
        break;
      case k_GOTO:
        pc = *(IXX_U16*)&pu8_programm[pc + 1];
        break;
      case k_GOSUB:
        CPUSH(pc + 3);
        pc = *(IXX_U16*)&pu8_programm[pc + 1];
        break;
      case k_RETURN:
        pc = (IXX_U16)CPOP();
        break;
      case k_IF:
        if(DPOP() == 0)
        {
          pc = *(IXX_U16*)&pu8_programm[pc + 1];
        }
        else
        {
          pc += 3;
        }
        break;
      default:
        return cnt;
    }
    cnt++;
  }
  return cnt;
}

/***************************************************************************************************
  Function:
    TINY_Hex2Bin

  Description:
    Convert the HEX string to a binary string.
    The HEX string must have an even number of characters and the characters A - F must
    be in upper case.

  Parameters:
    pc8_in  (IN)  - pointer to the HEX string
    u16_len (IN)  - length of the HEX string in bytes
    pu8_out (OUT) - pointer to the binary string

  Return value:
    -

***************************************************************************************************/
void TINY_Hex2Bin(const char* pc8_in, IXX_U16 u16_len, IXX_U8* pu8_out){

  static const unsigned char TBL[] = {
     0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   0,   0,   0,   0,
     0,   0,  10,  11,  12,  13,  14,  15,   0,   0,   0,   0,   0,   0,   0,
  };

  const char* end = pc8_in + u16_len;

  while (pc8_in < end)
  {
    *(pu8_out++) = TBL[CHAR(*pc8_in)] << 4 | TBL[CHAR(*(pc8_in + 1))];
    pc8_in += 2;
  }
}
