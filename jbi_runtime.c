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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "jbi.h"
#include "jbi_int.h"

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

#define ACS8(x)   *(uint8_t*)&(x)
#define ACS16(x)  *(uint16_t*)&(x)
#define ACS32(x)  *(uint32_t*)&(x)

#define DPUSH(x) vm->datastack[vm->dsp++ % k_STACK_SIZE] = x
#define DPOP() vm->datastack[--vm->dsp % k_STACK_SIZE]
#define DSTACK(x) vm->datastack[(vm->dsp + x) % k_STACK_SIZE]
#define CPUSH(x) vm->callstack[vm->csp++ % k_STACK_SIZE] = x
#define CPOP() vm->callstack[--vm->csp % k_STACK_SIZE]

#define CHAR(x) (((x) - 0x30) & 0x1F)

/***************************************************************************************************
**    static function-prototypes
***************************************************************************************************/
static uint32_t peek(uint32_t addr);

static uint8_t aTestBuffer[256] = {0}; // TODO

/***************************************************************************************************
**    global functions
***************************************************************************************************/
void *jbi_create(uint8_t num_vars)
{
  uint16_t size = sizeof(t_VM) + num_vars * sizeof(uint32_t);
  t_VM *vm = malloc(size);
  memset(vm, 0, size);
  return vm;
}

void jbi_SetVar(void *pv_vm, uint8_t var, uint32_t val)
{
  t_VM *vm = pv_vm;
  vm->variables[var] = val;
}

/***************************************************************************************************
  Function:
    jbi_run

  Description:
    Run the Tiny Basic Interpreter with the given program.
    Some measures:
    - run jbi_run with an empty program: 680 ticks
    - run jbi_run with a program with 20 instructions: ~1200 ticks
    => Each instruction needs 26 ticks or 16 ns (for 160 MHz)
    => Currently needs 0x350 bytes of code space

  Parameters:
    program  (IN) - pointer to the program

  Return value:
    Number of executed instructions

***************************************************************************************************/
uint16_t jbi_run(void *pv_vm, uint8_t* pprogramm, uint16_t len, uint16_t cycles)
{
  uint32_t tmp1, tmp2;
  uint16_t idx;
  uint8_t  var;
  uint8_t  *ptr;
  t_VM *vm = pv_vm;

  assert(pprogramm[0] == k_TAG);
  assert(pprogramm[1] == k_VERSION);
  vm->pc = 2;

  for(uint16_t i = 0; i < cycles; i++)
  {
#ifdef DEBUG    
    printf("%04X: %02X %s\n", vm->pc, pprogramm[vm->pc], Opcodes[pprogramm[vm->pc]]);
#endif
    switch (pprogramm[vm->pc])
    {
      case k_END:
        return JBI_END;
      case k_PRINTS:
        PRINTF("%s", &pprogramm[DPOP() % len] );
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
        tmp1 = pprogramm[vm->pc + 1]; // string length
        DPUSH(vm->pc + 2);  // push string address
        vm->pc += tmp1 + 2;
        break;
      case k_NUMBER:
        DPUSH(*(uint32_t*)&pprogramm[vm->pc + 1]);
        vm->pc += 5;
        break;
      case k_BYTENUM:
        DPUSH(pprogramm[vm->pc + 1]);
        vm->pc += 2;
        break;
      case k_VAR:
        var = pprogramm[vm->pc + 1];
        DPUSH(vm->variables[var]);
        vm->pc += 2;
        break;
      case k_LET:
        var = pprogramm[vm->pc + 1];
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
        vm->pc = *(uint16_t*)&pprogramm[vm->pc + 1];
        break;
      case k_GOSUB:
        CPUSH(vm->pc + 3);
        vm->pc = *(uint16_t*)&pprogramm[vm->pc + 1];
        break;
      case k_RETURN:
        vm->pc = (uint16_t)CPOP();
        break;
      case k_NEXT:
        // ID = ID + stack[-1]
        // IF ID <= stack[-2] GOTO start
        tmp1 = *(uint16_t*)&pprogramm[vm->pc + 1];
        var = pprogramm[vm->pc + 3];
        vm->variables[var] = vm->variables[var] + DSTACK(-1);
        if(vm->variables[var] <= DSTACK(-2))
        {
          vm->pc = tmp1;
        }
        else
        {
          vm->pc += 4;
          DPOP();  // remove step value
          DPOP();  // remove loop end value
        }
        break;
      case k_IF:
        if(DPOP() == 0)
        {
          vm->pc = *(uint16_t*)&pprogramm[vm->pc + 1];
        }
        else
        {
          vm->pc += 3;
        }
        break;
      case k_FUNC:
        switch(pprogramm[vm->pc + 1])
        {
          case k_PEEK: DPUSH(peek(DPOP())); vm->pc += 2; break;
          default:
            PRINTF("Error: unknown function '%u'\n", pprogramm[vm->pc + 1]);
            vm->pc += 2;
            break;
        }
        break;
      case k_VECTS1:
        var = pprogramm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &aTestBuffer[idx];
        tmp1 = DPOP();
        ACS8(ptr[DPOP()]) = tmp1;
        vm->pc += 2;
        break;
      case k_VECTG1:
        var = pprogramm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &aTestBuffer[idx];
        DPUSH(ACS8(ptr[DPOP()]));
        vm->pc += 2;
        break;
      case k_VECTS2:
        var = pprogramm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &aTestBuffer[idx];
        tmp1 = DPOP();
        ACS16(ptr[DPOP()]) = tmp1;
        vm->pc += 2;
        break;
      case k_VECTG2:
        var = pprogramm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &aTestBuffer[idx];
        DPUSH(ACS16(ptr[DPOP()]));
        vm->pc += 2;
        break;
      case k_VECTS4:
        var = pprogramm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &aTestBuffer[idx];
        tmp1 = DPOP();
        ACS32(ptr[DPOP()]) = tmp1;
        vm->pc += 2;
        break;
      case k_VECTG4:
        var = pprogramm[vm->pc + 1];
        idx = vm->variables[var];
        ptr = &aTestBuffer[idx];
        DPUSH(ACS32(ptr[DPOP()]));
        vm->pc += 2;
        break;
      default:
        return JBI_ERROR;
    }
  }
  return JBI_BUSY;
}

void jbi_destroy(void * pv_vm)
{
  free(pv_vm);
}

/***************************************************************************************************
  Function:
    jbi_Hex2Bin

  Description:
    Convert the HEX string to a binary string.
    The HEX string must have an even number of characters and the characters A - F must
    be in upper case.

  Parameters:
    p_in  (IN)  - pointer to the HEX string
    len (IN)  - length of the HEX string in bytes
    pout (OUT) - pointer to the binary string

  Return value:
    -

***************************************************************************************************/
void jbi_Hex2Bin(const char* p_in, uint16_t len, uint8_t* pout){

  static const unsigned char TBL[] = {
     0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   0,   0,   0,   0,
     0,   0,  10,  11,  12,  13,  14,  15,   0,   0,   0,   0,   0,   0,   0,
  };

  const char* end = p_in + len;

  while (p_in < end)
  {
    *(pout++) = TBL[CHAR(*p_in)] << 4 | TBL[CHAR(*(p_in + 1))];
    p_in += 2;
  }
}


/***************************************************************************************************
* Static functions
***************************************************************************************************/
static uint32_t peek(uint32_t addr)
{
  return addr * 2;
}