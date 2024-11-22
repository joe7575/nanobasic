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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "TINY.h"
#include "TINYint.h"

// PRINTF via UART
#if defined(_WIN32) || defined(WIN32) || defined(__linux__)
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
IXX_U16 TINY_Run(IXX_U8* pu8_programm, IXX_U16 u16_len)
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
  pc = 2;
  while(1)
  {
    //printf("%04X: %02X\n", pc, pu8_programm[pc]);
    switch (pu8_programm[pc])
    {
      case k_END:
        return cnt;
      case k_PRINTS:
        PRINTF("%s ", &pu8_programm[DPOP() % u16_len] );
        pc += 1;
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
        if(tmp2 == 0)
        {
          PRINTF("Error: Division by zero\n");
          DPUSH(0);
        }
        else
        {
          DPUSH(tmp1 / tmp2);
        }
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
      case k_AND:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 && tmp2);
        pc += 1;
        break;
      case k_OR:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 || tmp2);
        pc += 1;
        break;
      case k_NOT:
        DPUSH(!DPOP());
        pc += 1;
        break;
      case k_STRING:
        tmp1 = pu8_programm[pc + 1]; // string length
        DPUSH(pc + 2);  // push string address
        pc += tmp1 + 2;
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
