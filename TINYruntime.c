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
//#define DEBUG

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

#define ACS8(x)   *(IXX_U8*)&(x)
#define ACS16(x)  *(IXX_U16*)&(x)
#define ACS32(x)  *(IXX_U32*)&(x)

#define DPUSH(x) vm->datastack[vm->dsp++ % k_STACK_SIZE] = x
#define DPOP() vm->datastack[--vm->dsp % k_STACK_SIZE]
#define DSTACK(x) vm->datastack[(vm->dsp + x) % k_STACK_SIZE]
#define CPUSH(x) vm->callstack[vm->csp++ % k_STACK_SIZE] = x
#define CPOP() vm->callstack[--vm->csp % k_STACK_SIZE]

#define CHAR(x) (((x) - 0x30) & 0x1F)

/***************************************************************************************************
**    static function-prototypes
***************************************************************************************************/
static IXX_U32 peek(IXX_U32 addr);

static IXX_U8 au8_TestBuffer[256] = {0}; // TODO

/***************************************************************************************************
**    global functions
***************************************************************************************************/
void *TINY_Create(IXX_U8 u8_num_vars)
{
  IXX_U16 size = sizeof(t_VM) + u8_num_vars * sizeof(IXX_U32);
  t_VM *vm = malloc(size);
  memset(vm, 0, size);
  return vm;
}

void TINY_SetVar(void *pv_vm, IXX_U8 u8_var, IXX_U32 u32_val)
{
  t_VM *vm = pv_vm;
  vm->variables[u8_var] = u32_val;
}

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
IXX_U16 TINY_Run(void *pv_vm, IXX_U8* pu8_programm, IXX_U16 u16_len)
{
  IXX_U32 tmp1, tmp2;
  IXX_U16 cnt = 0;
  IXX_U16 idx;
  IXX_U8  var;
  IXX_U8  *ptr;
  t_VM *vm = pv_vm;

  assert(pu8_programm[0] == k_TAG);
  assert(pu8_programm[1] == k_VERSION);
  vm->pc = 2;

  while(1)
  {
#ifdef DEBUG    
    printf("%04X: %02X %s\n", vm->pc, pu8_programm[vm->pc], Opcodes[pu8_programm[vm->pc]]);
#endif
    switch (pu8_programm[vm->pc])
    {
      case k_END:
        return cnt;
      case k_PRINTS:
        PRINTF("%s", &pu8_programm[DPOP() % u16_len] );
        vm->pc += 1;
        break;
      case k_PRINTV:
        PRINTF("%d", DPOP());
        vm->pc += 1;
        break;
      case k_PRINTNL:
        PRINTF("\n");
        vm->pc += 1;
        break;
      case k_PRINTT:
        PRINTF("\t");
        vm->pc += 1;
        break;
      case k_STRING:
        tmp1 = pu8_programm[vm->pc + 1]; // string length
        DPUSH(vm->pc + 2);  // push string address
        vm->pc += tmp1 + 2;
        break;
      case k_NUMBER:
        DPUSH(*(IXX_U32*)&pu8_programm[vm->pc + 1]);
        vm->pc += 5;
        break;
      case k_BYTENUM:
        DPUSH(pu8_programm[vm->pc + 1]);
        vm->pc += 2;
        break;
      case k_VAR:
        var = pu8_programm[vm->pc + 1];
        DPUSH(vm->variables[var]);
        vm->pc += 2;
        break;
      case k_LET:
        var = pu8_programm[vm->pc + 1];
        vm->variables[var] = DPOP();
        vm->pc += 2;
        break;
      case k_ADD:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 + tmp2);
        vm->pc += 1;
        break;
      case k_SUB:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 - tmp2);
        vm->pc += 1;
        break;
      case k_MUL:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 * tmp2);
        vm->pc += 1;
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
        vm->pc += 1;
        break;
      case k_MOD:
        tmp2 = DPOP();
        tmp1 = DPOP();
        if(tmp2 == 0)
        {
          DPUSH(0);
        }
        else
        {
          DPUSH(tmp1 % tmp2);
        }
        vm->pc += 1;
        break;
      case k_AND:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 && tmp2);
        vm->pc += 1;
        break;
      case k_OR:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 || tmp2);
        vm->pc += 1;
        break;
      case k_NOT:
        DPUSH(!DPOP());
        vm->pc += 1;
        break;
      case k_EQUAL:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 == tmp2);
        vm->pc += 1;
        break;
      case k_NEQUAL :
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 != tmp2);
        vm->pc += 1;
        break;
      case k_LESS:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 < tmp2);
        vm->pc += 1;
        break;
      case k_LESSEQUAL:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 <= tmp2);
        vm->pc += 1;
        break;
      case k_GREATER:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 > tmp2);
        vm->pc += 1;
        break;
      case k_GREATEREQUAL:
        tmp2 = DPOP();
        tmp1 = DPOP();
        DPUSH(tmp1 >= tmp2);
        vm->pc += 1;
        break;
      case k_GOTO:
        vm->pc = *(IXX_U16*)&pu8_programm[vm->pc + 1];
        break;
      case k_GOSUB:
        CPUSH(vm->pc + 3);
        vm->pc = *(IXX_U16*)&pu8_programm[vm->pc + 1];
        break;
      case k_RETURN:
        vm->pc = (IXX_U16)CPOP();
        break;
      case k_NEXT:
        // ID = ID + stack[-1]
        // IF ID <= stack[-2] GOTO start
        tmp1 = *(IXX_U16*)&pu8_programm[vm->pc + 1];
        var = pu8_programm[vm->pc + 3];
        vm->variables[var] = vm->variables[var] + DSTACK(-1);
        if(vm->variables[var] <= DSTACK(-2))
        {
          vm->pc = tmp1;
        }
        else
        {
          vm->pc += 4;
          DPOP();  // remove step value
          DPOP();  // remove end condition
        }
        break;
      case k_IF:
        if(DPOP() == 0)
        {
          vm->pc = *(IXX_U16*)&pu8_programm[vm->pc + 1];
        }
        else
        {
          vm->pc += 3;
        }
        break;
      case k_FUNC:
        switch(pu8_programm[vm->pc + 1])
        {
          case k_PEEK: DPUSH(peek(DPOP())); vm->pc += 2; break;
          default:
            PRINTF("Error: unknown function '%u'\n", pu8_programm[vm->pc + 1]);
            vm->pc += 2;
            break;
        }
        break;
      case k_VECTS1:
        var = pu8_programm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &au8_TestBuffer[idx];
        tmp1 = DPOP();
        ACS8(ptr[DPOP()]) = tmp1;
        vm->pc += 2;
        break;
      case k_VECTG1:
        var = pu8_programm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &au8_TestBuffer[idx];
        DPUSH(ACS8(ptr[DPOP()]));
        vm->pc += 2;
        break;
      case k_VECTS2:
        var = pu8_programm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &au8_TestBuffer[idx];
        tmp1 = DPOP();
        ACS16(ptr[DPOP()]) = tmp1;
        vm->pc += 2;
        break;
      case k_VECTG2:
        var = pu8_programm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &au8_TestBuffer[idx];
        DPUSH(ACS16(ptr[DPOP()]));
        vm->pc += 2;
        break;
      case k_VECTS4:
        var = pu8_programm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &au8_TestBuffer[idx];
        tmp1 = DPOP();
        ACS32(ptr[DPOP()]) = tmp1;
        vm->pc += 2;
        break;
      case k_VECTG4:
        var = pu8_programm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &au8_TestBuffer[idx];
        DPUSH(ACS32(ptr[DPOP()]));
        vm->pc += 2;
        break;
      default:
        return cnt;
    }
    cnt++;
  }
  return cnt;
}

void TINY_Destroy(void * pv_vm)
{
  free(pv_vm);
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


/***************************************************************************************************
* Static functions
***************************************************************************************************/
static IXX_U32 peek(IXX_U32 addr)
{
  return addr * 2;
}