#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include "TINY.h"
#include "TINYint.h"

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
  IXX_U8  type;   // ID, NUM, IF,...
  IXX_U32 value;  // Variable index (0..n) or label address
} t_SYM;

static t_SYM as_Symbol[MAX_NUM_SYM] = {0};
static IXX_U8 u8_CurrVarIdx = 0;
static IXX_U8 au8_Code[MAX_CODE_LEN];
static IXX_U16 u16_Pc = 0;
static IXX_U16 u16_Linenum = 0;
static IXX_U16 u16_ErrCount = 0;
static IXX_U16 u16_SymIdx = 0;
static IXX_U16 u16_StartOfVars = 0;
static IXX_U16 au16_ForLoopStartAddr[MAX_FOR_LOOPS] = {0};
static char ac8_Line[MAX_STR_LEN];
static char ac8_Buff[MAX_STR_LEN];
static char *pc8_pos = NULL;
static char *pc8_next = NULL;
static IXX_U32 u32_Value;
static IXX_U8 u8_NextTok;
static IXX_U8 u8_ForLoopIdx = 0;

static void compile(FILE *fp);
static IXX_U8 next_token(void);
static IXX_U8 lookahead(void);
static IXX_U8 next(void);
static void match(IXX_U8 expected);
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
static IXX_U8 compile_width(IXX_BOOL bSet);
static IXX_U16 sym_add(char *id, IXX_U32 val, IXX_U8 type);
static void error(char *fmt, ...);

void TINY_DumpCode(IXX_U8 *code, IXX_U16 size)
{
  for(IXX_U16 i = 0; i < size; i++)
  {
    printf("%02X ", code[i]);
    if((i % 32) == 31)
    {
      printf("\n");
    }
  }
  printf("\n");
}

IXX_U8 *TINY_Compiler(char *filename, IXX_U16 *pu16_len, IXX_U8 *pu8_num_vars)
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
  u16_StartOfVars = u8_CurrVarIdx;

  FILE *fp = fopen(filename, "r");
  if(fp == NULL)
  {
    printf("Error: could not open file %s\n", filename);
    return NULL;
  }

  u8_CurrVarIdx = 0;

  printf("- pass1\n");
  compile(fp);

  if(u16_ErrCount > 0)
  {
    *pu16_len = 0;
    *pu8_num_vars = 0;
    return NULL;
  }

  printf("- pass2\n");
  compile(fp);

  fclose(fp);

  *pu16_len = u16_Pc;
  *pu8_num_vars = u8_CurrVarIdx;
  return au8_Code;
}

void TINY_OutputSymbolTable(void)
{
  printf("#### Symbol table ####\n");
  printf("Variables:\n");
  for(IXX_U16 i = u16_StartOfVars; i < MAX_NUM_SYM; i++)
  {
    if(as_Symbol[i].name[0] != '\0' && as_Symbol[i].type != LABEL)
    {
      printf("%16s: %u\n", as_Symbol[i].name, as_Symbol[i].value);
    }
  }
  printf("Labels:\n");
  for(IXX_U16 i = u16_StartOfVars; i < MAX_NUM_SYM; i++)
  {
    if(as_Symbol[i].name[0] != '\0' && as_Symbol[i].type == LABEL)
    {
      printf("%16s: %u\n", as_Symbol[i].name, as_Symbol[i].value);
    }
  }
}

static void compile(FILE *fp)
{
  u16_Linenum = 0;
  u16_ErrCount = 0;
  u16_Pc = 0;
  au8_Code[u16_Pc++] = k_TAG;
  au8_Code[u16_Pc++] = k_VERSION;
  //sym_add("rx", u8_CurrVarIdx, VECT);
  //sym_add("tx", u8_CurrVarIdx, VECT);

  fseek(fp, 0, SEEK_SET);
  while(fgets(ac8_Line, MAX_STR_LEN, fp) != NULL)
  {
    u16_Linenum++;
    pc8_pos = pc8_next = ac8_Line;
    compile_line();
  }
}

static IXX_U8 next_token(void)
{
  if(*pc8_pos == '\0')
  {
    return 0; // End of line
  }
  pc8_next = TINY_Scanner(pc8_pos, ac8_Buff);
  if(ac8_Buff[0] == '\0')
  {
    return 0; // End of line
  }
  if(ac8_Buff[0] == '\n')
  {
    return 0; // End of line
  }
  if(ac8_Buff[0] == '\"')
  {
    return STR;
  }
  if(isdigit(ac8_Buff[0]))
  {
    u32_Value = atoi(ac8_Buff);
    return NUM;
  }
  if(isalpha(ac8_Buff[0]))
  {
    IXX_U16 len = strlen(ac8_Buff);
    IXX_U8 type = ac8_Buff[len - 1] == '$' ? SID : ID;

    u16_SymIdx = sym_add(ac8_Buff, u8_CurrVarIdx, type);
    return as_Symbol[u16_SymIdx].type;
  }
  if(ac8_Buff[0] == '<')
  {
    // parse '<=' or '<'
    if (ac8_Buff[1] == '=') {
        return LQ;
    }
    return LE;
  }
  if(ac8_Buff[0] == '>')
  {
    // parse '>=' or '>'
    if (ac8_Buff[0] == '=') {
        return GQ;
    }
    return GR;
  }
  if(strlen(ac8_Buff) == 1)
  {
    return ac8_Buff[0]; // Single character
  }
  error("unknown token %c", ac8_Buff[0]);
  return 0;
}

static IXX_U8 lookahead(void)
{
  if(pc8_pos == pc8_next)
  {
    u8_NextTok = next_token();
  }
  //printf("lookahead: %s\n", ac8_Buff);
  return u8_NextTok;
}

static IXX_U8 next(void)
{
  if(pc8_pos == pc8_next)
  {
    u8_NextTok = next_token();
  }
  pc8_pos = pc8_next;
#ifdef DEBUG
  printf("next: %s\n", ac8_Buff);
#endif
  return u8_NextTok;
}

static void match(IXX_U8 expected) {
  IXX_U8 tok = next();
  if (tok == expected) {
  } else {
    if (expected >= LET && expected <= VECT) {
      error("%s expected instead of '%s'", Keywords[expected - LET], ac8_Buff);
    } else {
      error("'%c' expected instead of '%s'", expected, ac8_Buff);
    }
  }
}

static void label(void)
{
  IXX_U8 tok = lookahead();
  if(tok == ID) // Token recognized as variable?
  {
    // Convert to label
    as_Symbol[u16_SymIdx].type = LABEL;
    u8_NextTok = LABEL;
    u8_CurrVarIdx--;
  }
  match(LABEL);
}

static void compile_line(void)
{
  IXX_U8 tok = lookahead();
  if(tok == ID || tok == LABEL)
  {
    IXX_U16 idx = u16_SymIdx;
    label();
    tok = lookahead();
    if(tok == ':')
    {
      match(':');
      as_Symbol[idx].value = u16_Pc;
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
  IXX_U8 tok = next();
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
  IXX_U8 tok;
  match(ID);
  IXX_U16 idx = u16_SymIdx;
  match('=');
  compile_expression();
  au8_Code[u16_Pc++] = k_LET;
  au8_Code[u16_Pc++] = as_Symbol[idx].value;
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
    au8_Code[u16_Pc++] = k_BYTENUM;
    au8_Code[u16_Pc++] = 1;
  }
  // Save start address
  if(u8_ForLoopIdx >= MAX_FOR_LOOPS)
  {
    error("too many nested 'for' loops");
    return;
  }
  au16_ForLoopStartAddr[u8_ForLoopIdx++] = u16_Pc;
}

// ID =  ID + Step ID3            k_NEXT start var
// ID <= ID2 GOTO start           
static void compile_next(void)
{
  match(ID);
  IXX_U16 idx = u16_SymIdx;
  if(u8_ForLoopIdx == 0)
  {
    error("'next' without 'for'");
    return;
  }
  IXX_U16 addr = au16_ForLoopStartAddr[--u8_ForLoopIdx];
  au8_Code[u16_Pc++] = k_NEXT;
  au8_Code[u16_Pc++] = addr & 0xFF;
  au8_Code[u16_Pc++] = (addr >> 8) & 0xFF;
  au8_Code[u16_Pc++] = as_Symbol[idx].value;
}

static void compile_if(void)
{
  IXX_U8 tok, op;

  compile_expression();
  au8_Code[u16_Pc++] = k_IF;
  IXX_U16 pos = u16_Pc;
  u16_Pc += 2;
  match(THEN);
  compile_stmt();
  au8_Code[pos] = u16_Pc & 0xFF;
  au8_Code[pos + 1] = (u16_Pc >> 8) & 0xFF;
}

static void compile_goto(void)
{
  label();
  IXX_U16 addr = as_Symbol[u16_SymIdx].value;
  au8_Code[u16_Pc++] = k_GOTO;
  au8_Code[u16_Pc++] = addr & 0xFF;
  au8_Code[u16_Pc++] = (addr >> 8) & 0xFF;
}

static void compile_gosub(void)
{
  label();
  IXX_U16 addr = as_Symbol[u16_SymIdx].value;
  au8_Code[u16_Pc++] = k_GOSUB;
  au8_Code[u16_Pc++] = addr & 0xFF;
  au8_Code[u16_Pc++] = (addr >> 8) & 0xFF;
}

static void compile_return(void)
{
  au8_Code[u16_Pc++] = k_RETURN;
}

static void compile_var(void)
{
  IXX_U8 tok = next();
  IXX_U16 idx = u16_SymIdx;

  if(tok == SID) // let a$ = "string"
  {
    match('=');
    compile_string();
    // Var[value] = pop()
    au8_Code[u16_Pc++] = k_LET;
    au8_Code[u16_Pc++] = as_Symbol[idx].value;
  }
  else if(tok == ID) // let a = expression
  {
    match('=');
    compile_expression();
    // Var[value] = pop()
    au8_Code[u16_Pc++] = k_LET;
    au8_Code[u16_Pc++] = as_Symbol[idx].value;
  }
  else if(tok == VECT) // let rx[0] = 1
  {
    match('[');
    compile_expression();
    IXX_U8 instr = compile_width(1);
    match(']');
    match('=');
    compile_expression();
    au8_Code[u16_Pc++] = instr;
    au8_Code[u16_Pc++] = as_Symbol[idx].value;
  }
  else
  {
    error("unknown variable type '%u'", tok);
  }
}

static void remark(void)
{
  // Skip to end of line
  pc8_pos[0] = '\0';
}

static void compile_print(void)
{
  IXX_U8 tok = lookahead();
  if(tok == 0)
  {
    au8_Code[u16_Pc++] = k_PRINTNL;
    return;
  }
  while(tok)
  {
    if(tok == STR)
    {
      compile_string();
      au8_Code[u16_Pc++] = k_PRINTS;
    }
    else if(tok == SID)
    {
      au8_Code[u16_Pc++] = k_VAR;
      au8_Code[u16_Pc++] = as_Symbol[u16_SymIdx].value;
      au8_Code[u16_Pc++] = k_PRINTS;
      match(SID);
    }
    else
    {
      compile_expression();
      au8_Code[u16_Pc++] = k_PRINTV;
    }
    tok = lookahead();
    if(tok == ',')
    {
      match(',');
      au8_Code[u16_Pc++] = k_PRINTT;
      tok = lookahead();
    }
    else if(tok == ';')
    {
      match(';');
       tok = lookahead();
    }
    else
    {
      au8_Code[u16_Pc++] = k_PRINTNL;
    }
  }
}

static void compile_string(void)
{
  match(STR);
  // push string address
  IXX_U16 len = strlen(ac8_Buff);
  ac8_Buff[len - 1] = '\0';
  au8_Code[u16_Pc++] = k_STRING;
  au8_Code[u16_Pc++] = len - 1; // without quotes but with 0
  strcpy((char*)&au8_Code[u16_Pc], ac8_Buff + 1);
  u16_Pc += len - 1;
}

static void compile_expression(void)
{
  compile_and_expr();
  IXX_U8 op = lookahead();
  while(op == OR)
  {
    match(op);
    au8_Code[u16_Pc++] = k_OR;
    op = lookahead();
  }
}

static void compile_and_expr(void)
{
  compile_not_expr();
  IXX_U8 op = lookahead();
  while(op == AND)
  {
    match(op);
    au8_Code[u16_Pc++] = k_AND;
    op = lookahead();
  }
}

static void compile_not_expr(void)
{
  IXX_U8 op = lookahead();
  if(op == NOT)
  {
    match(op);
    compile_comp_expr();
    au8_Code[u16_Pc++] = k_NOT;
  }
  else
  {
    compile_comp_expr();
  }
}

static void compile_comp_expr(void)
{
  compile_add_expr();
  IXX_U8 op = lookahead();
  while(op == EQ || op == NQ || op == LE || op == LQ || op == GR || op == GQ)
  {
    match(op);
    compile_add_expr();
    switch(op)
    {
      case EQ: au8_Code[u16_Pc++] = k_EQUAL; break;
      case NQ: au8_Code[u16_Pc++] = k_NEQUAL; break;
      case LE: au8_Code[u16_Pc++] = k_LESS; break;
      case LQ: au8_Code[u16_Pc++] = k_LESSEQUAL; break;
      case GR: au8_Code[u16_Pc++] = k_GREATER; break;
      case GQ: au8_Code[u16_Pc++] = k_GREATEREQUAL; break;
    }
    op = lookahead();
  }
}

static void compile_add_expr(void)
{
  compile_term();
  IXX_U8 op = lookahead();
  while(op == '+' || op == '-')
  {
    match(op);
    compile_term();
    if(op == '+')
    {
      au8_Code[u16_Pc++] = k_ADD;
    }
    else
    {
      au8_Code[u16_Pc++] = k_SUB;
    }
    op = lookahead();
  }
}

static void compile_term(void)
{
  compile_factor();
  IXX_U8 op = lookahead();
  while(op == '*' || op == '/' || op == '%')
  {
    match(op);
    compile_factor();
    if(op == '*')
    {
      au8_Code[u16_Pc++] = k_MUL;
    }
    else if(op == '%')
    {
      au8_Code[u16_Pc++] = k_MOD;
    }
    else
    {
      au8_Code[u16_Pc++] = k_DIV;
    }
    op = lookahead();
  }
}

static void compile_factor(void)
{
  IXX_U8 tok = lookahead();
  switch(tok)
  {
    case '(':
      match('(');
      compile_expression();
      match(')');
      break;
    case NUM:
      match(NUM);
      if(u32_Value < 256)
      {
        au8_Code[u16_Pc++] = k_BYTENUM;
        au8_Code[u16_Pc++] = u32_Value;
      }
      else
      {
        au8_Code[u16_Pc++] = k_NUMBER;
        au8_Code[u16_Pc++] = u32_Value & 0xFF;
        au8_Code[u16_Pc++] = (u32_Value >> 8) & 0xFF;
        au8_Code[u16_Pc++] = (u32_Value >> 16) & 0xFF;
        au8_Code[u16_Pc++] = (u32_Value >> 24) & 0xFF;
      }
      break;
    case ID:
      match(ID);
      au8_Code[u16_Pc++] = k_VAR;
      au8_Code[u16_Pc++] = as_Symbol[u16_SymIdx].value;
      break;
    case PEEK:
      match(PEEK);
      match('(');
      compile_expression();
      match(')');
      au8_Code[u16_Pc++] = k_FUNC;
      au8_Code[u16_Pc++] = k_PEEK;
      break;
    case VECT:
      IXX_U8 val = as_Symbol[u16_SymIdx].value;
      match(VECT);
      match('[');
      compile_expression();
      IXX_U8 instr = compile_width(0);
      match(']');
      au8_Code[u16_Pc++] = instr;
      au8_Code[u16_Pc++] = val;
      break;
    default:
      error("unknown factor '%u'", tok);
      break;
  }
}

static void compile_end(void)
{
  au8_Code[u16_Pc++] = k_END;
}

/*
 Return instructions for vector set operations.
 To get get instructions, add 1 to the instruction.
*/
static IXX_U8 compile_width(IXX_BOOL bSet)
{
  IXX_U8 tok = lookahead();
  if(tok == ',')
  {
    match(',');
    tok = lookahead();
    if(tok == NUM)
    {
      IXX_U8 val = u32_Value;
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

static IXX_U16 sym_add(char *id, IXX_U32 val, IXX_U8 type)
{
  // Search for existing symbol
  for(IXX_U16 i = 0; i < MAX_NUM_SYM; i++)
  {
    if(strcmp(as_Symbol[i].name, id) == 0)
    {
      return i;
    }
  }

  // Add new symbol
  for(IXX_U16 i = 0; i < MAX_NUM_SYM; i++)
  {
    if(as_Symbol[i].name[0] == '\0')
    {
      memcpy(as_Symbol[i].name, id, MIN(strlen(id), MAX_SYM_LEN));
      as_Symbol[i].value = val;
      as_Symbol[i].type = type;
      u8_CurrVarIdx++;
      return i;
    }
  }
  return -1;
}

static void error(char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  printf("Error in line %u: ", u16_Linenum);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
  u16_ErrCount++;
  pc8_pos = pc8_next;
  pc8_pos[0] = '\0';
}