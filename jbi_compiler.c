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
#include <stdarg.h>
#include "jbi.h"
#include "jbi_int.h"

//#define DEBUG

#define MAX_STR_LEN     256
#define MAX_NUM_SYM     256
#define MAX_SYM_LEN     9
#define MAX_CODE_LEN    1024
#define MAX_FOR_LOOPS   8

#define MIN(a,b)        ((a) < (b) ? (a) : (b))

// Token types
enum {
  LET = 128, FOR, TO, STEP, NEXT, IF, THEN, PRINT, GOTO, GOSUB, RETURN, END, REM, AND, OR, NOT,
  NUM, STR, ID, SID, EQ, NQ, LE, LQ, GR, GQ,
  PEEK, VECT,
  LABEL,
};

// Keywords
static char *Keywords[] = {
  "let", "for", "to", "step", "next", "if", "then", "print", "goto", "gosub", "return", "end", "rem", "and", "or", "not",
  "number", "string", "variable", "string variable", "=", "<>", "<", "<=", ">", ">=",
  "peek", "vector",
  "label",
};

// Symbol table
typedef struct {
  char name[MAX_SYM_LEN];
  uint8_t  type;   // ID, NUM, IF,...
  uint32_t value;  // Variable index (0..n) or label address
} sym_t;

static sym_t a_Symbol[MAX_NUM_SYM] = {0};
static uint8_t CurrVarIdx = 0;
static uint8_t a_Code[MAX_CODE_LEN];
static uint16_t Pc = 0;
static uint16_t Linenum = 0;
static uint16_t ErrCount = 0;
static uint16_t SymIdx = 0;
static uint16_t StartOfVars = 0;
static uint16_t a_ForLoopStartAddr[MAX_FOR_LOOPS] = {0};
static char a_Line[MAX_STR_LEN];
static char a_Buff[MAX_STR_LEN];
static char *p_pos = NULL;
static char *p_next = NULL;
static uint32_t Value;
static uint8_t NextTok;
static uint8_t ForLoopIdx = 0;

static void compile(FILE *fp);
static uint8_t next_token(void);
static uint8_t lookahead(void);
static uint8_t next(void);
static void match(uint8_t expected);
static void label(void);
static void compile_line(void);
static void compile_stmt(void);
static void compile_for(void);
static void compile_next(void);
static void compile_if(void);
static void compile_goto(void);
static void compile_gosub(void);
static void compile_return(void);
static void compile_var(void);
static void remark(void);
static void compile_comment(void);
static void compile_print(void);
static void compile_string(void);
static void compile_expression(void);
static void compile_and_expr(void);
static void compile_not_expr(void);
static void compile_comp_expr(void);
static void compile_add_expr(void);
static void compile_term(void);
static void compile_factor(void);
static void compile_end(void);
static uint8_t compile_width(bool bSet);
static uint16_t sym_add(char *id, uint32_t val, uint8_t type);
static void error(char *fmt, ...);

/*************************************************************************************************
** API functions
*************************************************************************************************/
void jbi_dump_code(uint8_t *code, uint16_t size)
{
  for(uint16_t i = 0; i < size; i++)
  {
    printf("%02X ", code[i]);
    if((i % 32) == 31)
    {
      printf("\n");
    }
  }
  printf("\n");
}

uint8_t *jbi_compiler(char *filename, uint16_t *p_len, uint8_t *p_num_vars)
{
  // Add keywords
  sym_add("let", 0, LET);
  sym_add("for", 0, FOR);
  sym_add("to", 0, TO);
  sym_add("step", 0, STEP);
  sym_add("next", 0, NEXT);
  sym_add("if", 0, IF);
  sym_add("then", 0, THEN);
  sym_add("print", 0, PRINT);
  sym_add("goto", 0, GOTO);
  sym_add("gosub", 0, GOSUB);
  sym_add("return", 0, RETURN);
  sym_add("end", 0, END);
  sym_add("rem", 0, REM);
  sym_add("and", 0, AND);
  sym_add("or", 0, OR);
  sym_add("not", 0, NOT);
  sym_add("peek", 0, PEEK);
  StartOfVars = CurrVarIdx;

  FILE *fp = fopen(filename, "r");
  if(fp == NULL)
  {
    printf("Error: could not open file %s\n", filename);
    return NULL;
  }

  CurrVarIdx = 0;

  printf("- pass1\n");
  compile(fp);

  if(ErrCount > 0)
  {
    *p_len = 0;
    *p_num_vars = 0;
    return NULL;
  }

  printf("- pass2\n");
  compile(fp);

  fclose(fp);

  *p_len = Pc;
  *p_num_vars = CurrVarIdx;
  return a_Code;
}

void jbi_output_symbol_table(void)
{
  printf("#### Symbol table ####\n");
  printf("Variables:\n");
  for(uint16_t i = StartOfVars; i < MAX_NUM_SYM; i++)
  {
    if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type != LABEL)
    {
      printf("%16s: %u\n", a_Symbol[i].name, a_Symbol[i].value);
    }
  }
  printf("Labels:\n");
  for(uint16_t i = StartOfVars; i < MAX_NUM_SYM; i++)
  {
    if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type == LABEL)
    {
      printf("%16s: %u\n", a_Symbol[i].name, a_Symbol[i].value);
    }
  }
}

/*************************************************************************************************
** Static functions
*************************************************************************************************/
static void compile(FILE *fp)
{
  Linenum = 0;
  ErrCount = 0;
  Pc = 0;
  a_Code[Pc++] = k_TAG;
  a_Code[Pc++] = k_VERSION;
  //sym_add("rx", CurrVarIdx, VECT);
  //sym_add("tx", CurrVarIdx, VECT);

  fseek(fp, 0, SEEK_SET);
  while(fgets(a_Line, MAX_STR_LEN, fp) != NULL)
  {
    Linenum++;
    p_pos = p_next = a_Line;
    compile_line();
  }
}

static uint8_t next_token(void)
{
  if(*p_pos == '\0')
  {
    return 0; // End of line
  }
  p_next = jbi_scanner(p_pos, a_Buff);
  if(a_Buff[0] == '\0')
  {
    return 0; // End of line
  }
  if(a_Buff[0] == '\n')
  {
    return 0; // End of line
  }
  if(a_Buff[0] == '\"')
  {
    return STR;
  }
  if(isdigit(a_Buff[0]))
  {
    Value = atoi(a_Buff);
    return NUM;
  }
  if(isalpha(a_Buff[0]))
  {
    uint16_t len = strlen(a_Buff);
    uint8_t type = a_Buff[len - 1] == '$' ? SID : ID;

    SymIdx = sym_add(a_Buff, CurrVarIdx, type);
    return a_Symbol[SymIdx].type;
  }
  if(a_Buff[0] == '<')
  {
    // parse '<=' or '<'
    if (a_Buff[1] == '=') {
        return LQ;
    }
    return LE;
  }
  if(a_Buff[0] == '>')
  {
    // parse '>=' or '>'
    if (a_Buff[0] == '=') {
        return GQ;
    }
    return GR;
  }
  if(strlen(a_Buff) == 1)
  {
    return a_Buff[0]; // Single character
  }
  error("unknown token %c", a_Buff[0]);
  return 0;
}

static uint8_t lookahead(void)
{
  if(p_pos == p_next)
  {
    NextTok = next_token();
  }
  //printf("lookahead: %s\n", a_Buff);
  return NextTok;
}

static uint8_t next(void)
{
  if(p_pos == p_next)
  {
    NextTok = next_token();
  }
  p_pos = p_next;
#ifdef DEBUG
  printf("next: %s\n", a_Buff);
#endif
  return NextTok;
}

static void match(uint8_t expected) {
  uint8_t tok = next();
  if (tok == expected) {
  } else {
    if (expected >= LET && expected <= VECT) {
      error("%s expected instead of '%s'", Keywords[expected - LET], a_Buff);
    } else {
      error("'%c' expected instead of '%s'", expected, a_Buff);
    }
  }
}

static void label(void)
{
  uint8_t tok = lookahead();
  if(tok == ID) // Token recognized as variable?
  {
    // Convert to label
    a_Symbol[SymIdx].type = LABEL;
    NextTok = LABEL;
    CurrVarIdx--;
  }
  match(LABEL);
}

static void compile_line(void)
{
  uint8_t tok = lookahead();
  if(tok == ID || tok == LABEL)
  {
    uint16_t idx = SymIdx;
    label();
    tok = lookahead();
    if(tok == ':')
    {
      match(':');
      a_Symbol[idx].value = Pc;
      tok = lookahead();
    }
  }
  while(tok)
  {
    compile_stmt();
    tok = lookahead();
    if(tok == ';')
    {
      match(';');
      tok = lookahead();
    }
  }
}

static void compile_stmt(void)
{
  uint8_t tok = next();
  switch(tok)
  {
    case FOR: compile_for(); break;
    case IF: compile_if(); break;
    case LET: compile_var(); break;
    case REM: remark(); break;
    case GOTO: compile_goto(); break;
    case GOSUB: compile_gosub(); break;
    case RETURN: compile_return(); break;
    case PRINT: compile_print(); break;
    case NEXT: compile_next(); break;
    case END: compile_end(); break;
    default: error("unknown statement '%u'", tok); break;
  }
}

// FOR ID '=' <Expression1> TO <Expression2> [STEP <Expression3>]
// <Statement>...
// NEXT [ID]
//
// ID1 = <Expression1>            k_NUMBER number / k_VAR var (push), k_LET var (pop)
// ID2 = <Expression2>            k_NUMBER number / k_VAR var (push)
// ID3 = <Expression3> or 1       k_NUMBER number / k_VAR var (push)
// start:
static void compile_for(void)
{
  uint8_t tok;
  match(ID);
  uint16_t idx = SymIdx;
  match('=');
  compile_expression();
  a_Code[Pc++] = k_LET;
  a_Code[Pc++] = a_Symbol[idx].value;
  match(TO);
  compile_expression();
  tok = lookahead();
  if(tok == STEP)
  {
    match(STEP);
    compile_expression();
  }
  else
  {
    a_Code[Pc++] = k_BYTENUM;
    a_Code[Pc++] = 1;
  }
  // Save start address
  if(ForLoopIdx >= MAX_FOR_LOOPS)
  {
    error("too many nested 'for' loops");
    return;
  }
  a_ForLoopStartAddr[ForLoopIdx++] = Pc;
}

// ID =  ID + Step ID3            k_NEXT start var
// ID <= ID2 GOTO start           
static void compile_next(void)
{
  match(ID);
  uint16_t idx = SymIdx;
  if(ForLoopIdx == 0)
  {
    error("'next' without 'for'");
    return;
  }
  uint16_t addr = a_ForLoopStartAddr[--ForLoopIdx];
  a_Code[Pc++] = k_NEXT;
  a_Code[Pc++] = addr & 0xFF;
  a_Code[Pc++] = (addr >> 8) & 0xFF;
  a_Code[Pc++] = a_Symbol[idx].value;
}

static void compile_if(void)
{
  uint8_t tok, op;

  compile_expression();
  a_Code[Pc++] = k_IF;
  uint16_t pos = Pc;
  Pc += 2;
  match(THEN);
  compile_stmt();
  a_Code[pos] = Pc & 0xFF;
  a_Code[pos + 1] = (Pc >> 8) & 0xFF;
}

static void compile_goto(void)
{
  label();
  uint16_t addr = a_Symbol[SymIdx].value;
  a_Code[Pc++] = k_GOTO;
  a_Code[Pc++] = addr & 0xFF;
  a_Code[Pc++] = (addr >> 8) & 0xFF;
}

static void compile_gosub(void)
{
  label();
  uint16_t addr = a_Symbol[SymIdx].value;
  a_Code[Pc++] = k_GOSUB;
  a_Code[Pc++] = addr & 0xFF;
  a_Code[Pc++] = (addr >> 8) & 0xFF;
}

static void compile_return(void)
{
  a_Code[Pc++] = k_RETURN;
}

static void compile_var(void)
{
  uint8_t tok = next();
  uint16_t idx = SymIdx;

  if(tok == SID) // let a$ = "string"
  {
    match('=');
    compile_string();
    // Var[value] = pop()
    a_Code[Pc++] = k_LET;
    a_Code[Pc++] = a_Symbol[idx].value;
  }
  else if(tok == ID) // let a = expression
  {
    match('=');
    compile_expression();
    // Var[value] = pop()
    a_Code[Pc++] = k_LET;
    a_Code[Pc++] = a_Symbol[idx].value;
  }
  else if(tok == VECT) // let rx[0] = 1
  {
    match('[');
    compile_expression();
    uint8_t instr = compile_width(1);
    match(']');
    match('=');
    compile_expression();
    a_Code[Pc++] = instr;
    a_Code[Pc++] = a_Symbol[idx].value;
  }
  else
  {
    error("unknown variable type '%u'", tok);
  }
}

static void remark(void)
{
  // Skip to end of line
  p_pos[0] = '\0';
}

static void compile_print(void)
{
  uint8_t tok = lookahead();
  if(tok == 0)
  {
    a_Code[Pc++] = k_PRINTNL;
    return;
  }
  while(tok)
  {
    if(tok == STR)
    {
      compile_string();
      a_Code[Pc++] = k_PRINTS;
    }
    else if(tok == SID)
    {
      a_Code[Pc++] = k_VAR;
      a_Code[Pc++] = a_Symbol[SymIdx].value;
      a_Code[Pc++] = k_PRINTS;
      match(SID);
    }
    else
    {
      compile_expression();
      a_Code[Pc++] = k_PRINTV;
    }
    tok = lookahead();
    if(tok == ',')
    {
      match(',');
      a_Code[Pc++] = k_PRINTT;
      tok = lookahead();
    }
    else if(tok == ';')
    {
      match(';');
       tok = lookahead();
    }
    else
    {
      a_Code[Pc++] = k_PRINTNL;
    }
  }
}

static void compile_string(void)
{
  match(STR);
  // push string address
  uint16_t len = strlen(a_Buff);
  a_Buff[len - 1] = '\0';
  a_Code[Pc++] = k_STRING;
  a_Code[Pc++] = len - 1; // without quotes but with 0
  strcpy((char*)&a_Code[Pc], a_Buff + 1);
  Pc += len - 1;
}

static void compile_expression(void)
{
  compile_and_expr();
  uint8_t op = lookahead();
  while(op == OR)
  {
    match(op);
    a_Code[Pc++] = k_OR;
    op = lookahead();
  }
}

static void compile_and_expr(void)
{
  compile_not_expr();
  uint8_t op = lookahead();
  while(op == AND)
  {
    match(op);
    a_Code[Pc++] = k_AND;
    op = lookahead();
  }
}

static void compile_not_expr(void)
{
  uint8_t op = lookahead();
  if(op == NOT)
  {
    match(op);
    compile_comp_expr();
    a_Code[Pc++] = k_NOT;
  }
  else
  {
    compile_comp_expr();
  }
}

static void compile_comp_expr(void)
{
  compile_add_expr();
  uint8_t op = lookahead();
  while(op == EQ || op == NQ || op == LE || op == LQ || op == GR || op == GQ)
  {
    match(op);
    compile_add_expr();
    switch(op)
    {
      case EQ: a_Code[Pc++] = k_EQUAL; break;
      case NQ: a_Code[Pc++] = k_NEQUAL; break;
      case LE: a_Code[Pc++] = k_LESS; break;
      case LQ: a_Code[Pc++] = k_LESSEQUAL; break;
      case GR: a_Code[Pc++] = k_GREATER; break;
      case GQ: a_Code[Pc++] = k_GREATEREQUAL; break;
    }
    op = lookahead();
  }
}

static void compile_add_expr(void)
{
  compile_term();
  uint8_t op = lookahead();
  while(op == '+' || op == '-')
  {
    match(op);
    compile_term();
    if(op == '+')
    {
      a_Code[Pc++] = k_ADD;
    }
    else
    {
      a_Code[Pc++] = k_SUB;
    }
    op = lookahead();
  }
}

static void compile_term(void)
{
  compile_factor();
  uint8_t op = lookahead();
  while(op == '*' || op == '/' || op == '%')
  {
    match(op);
    compile_factor();
    if(op == '*')
    {
      a_Code[Pc++] = k_MUL;
    }
    else if(op == '%')
    {
      a_Code[Pc++] = k_MOD;
    }
    else
    {
      a_Code[Pc++] = k_DIV;
    }
    op = lookahead();
  }
}

static void compile_factor(void)
{
  uint8_t tok = lookahead();
  switch(tok)
  {
    case '(':
      match('(');
      compile_expression();
      match(')');
      break;
    case NUM:
      match(NUM);
      if(Value < 256)
      {
        a_Code[Pc++] = k_BYTENUM;
        a_Code[Pc++] = Value;
      }
      else
      {
        a_Code[Pc++] = k_NUMBER;
        a_Code[Pc++] = Value & 0xFF;
        a_Code[Pc++] = (Value >> 8) & 0xFF;
        a_Code[Pc++] = (Value >> 16) & 0xFF;
        a_Code[Pc++] = (Value >> 24) & 0xFF;
      }
      break;
    case ID:
      match(ID);
      a_Code[Pc++] = k_VAR;
      a_Code[Pc++] = a_Symbol[SymIdx].value;
      break;
    case PEEK:
      match(PEEK);
      match('(');
      compile_expression();
      match(')');
      a_Code[Pc++] = k_FUNC;
      a_Code[Pc++] = k_PEEK;
      break;
    case VECT:
      uint8_t val = a_Symbol[SymIdx].value;
      match(VECT);
      match('[');
      compile_expression();
      uint8_t instr = compile_width(0);
      match(']');
      a_Code[Pc++] = instr;
      a_Code[Pc++] = val;
      break;
    default:
      error("unknown factor '%u'", tok);
      break;
  }
}

static void compile_end(void)
{
  a_Code[Pc++] = k_END;
}

/*
 Return instructions for vector set operations.
 To get get instructions, add 1 to the instruction.
*/
static uint8_t compile_width(bool bSet)
{
  uint8_t tok = lookahead();
  if(tok == ',')
  {
    match(',');
    tok = lookahead();
    if(tok == NUM)
    {
      uint8_t val = Value;
      match(NUM);
      if(val == 1) return bSet ? k_VECTS1 : k_VECTG1;
      if(val == 2) return bSet ? k_VECTS2 : k_VECTG2;
      if(val == 4) return bSet ? k_VECTS4 : k_VECTG4;
      error("unknown width '%u'", val);
      return 0;
    }
    error("number expected");
  }
  return bSet ? k_VECTS1 : k_VECTG1;
}

static uint16_t sym_add(char *id, uint32_t val, uint8_t type)
{
  // Search for existing symbol
  for(uint16_t i = 0; i < MAX_NUM_SYM; i++)
  {
    if(strcmp(a_Symbol[i].name, id) == 0)
    {
      return i;
    }
  }

  // Add new symbol
  for(uint16_t i = 0; i < MAX_NUM_SYM; i++)
  {
    if(a_Symbol[i].name[0] == '\0')
    {
      memcpy(a_Symbol[i].name, id, MIN(strlen(id), MAX_SYM_LEN));
      a_Symbol[i].value = val;
      a_Symbol[i].type = type;
      CurrVarIdx++;
      return i;
    }
  }
  return -1;
}

static void error(char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  printf("Error in line %u: ", Linenum);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
  ErrCount++;
  p_pos = p_next;
  p_pos[0] = '\0';
}