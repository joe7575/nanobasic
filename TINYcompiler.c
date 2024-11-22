#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "TINY.h"
#include "TINYint.h"

#define MAX_STR_LEN     256
#define MAX_NUM_SYM     64
#define MAX_SYM_LEN     9
#define MAX_CODE_LEN    1024

#define MIN(a,b)        ((a) < (b) ? (a) : (b))

// Token types
enum {
  LET = 128, IF, THEN, PRINT, GOTO, GOSUB, RETURN, END, REM, 
  NUM, STR, ID, EQ, NQ, LE, LQ, GR, GQ
};

// Symbol table
typedef struct {
  char name[MAX_SYM_LEN];
  IXX_U8 type;    // ID, NUM, IF,...
  IXX_U32 value;  // Variable index (0..n) or label address
} t_SYM;

static t_SYM as_Symbol[MAX_NUM_SYM] = {0};
static IXX_U8 u8_NumSymbols = 0;
static IXX_U8 au8_Code[MAX_CODE_LEN];
static IXX_U16 u16_Pc = 0;
static IXX_U16 u16_Linenum = 0;
static IXX_U16 u16_SymIdx = 0;
static char ac8_Line[MAX_STR_LEN];
static char ac8_Buff[MAX_STR_LEN];
static char *pc8_pos = NULL;
static char *pc8_next = NULL;
static IXX_U32 u32_Value;
static IXX_U8 u8_NextTok;

static IXX_U8 next_token(void);
static IXX_U8 lookahead(void);
static IXX_U8 next(void);
static void match(IXX_U8 expected);
static void compile_line(void);
static void compile_stmt(void);
static void compile_if(void);
static void compile_goto(void);
static void compile_gosub(void);
static void compile_return(void);
static void compile_var(void);
static void compile_comment(void);
static void compile_print(void);
static void compile_expression(void);
static void compile_term(void);
static void compile_factor(void);
static void compile_end(void);
static IXX_U16 sym_add(char *id, IXX_U32 val, IXX_U8 type);

IXX_U8 *TINY_Compiler(char *filename, IXX_U16 *pu16_len)
{
  FILE *fp = fopen(filename, "r");
  if(fp == NULL)
  {
    printf("Error: could not open file %s\n", filename);
    return NULL;
  }

  // Add keywords
  sym_add("let", 0, LET);
  sym_add("if", 0, IF);
  sym_add("then", 0, THEN);
  sym_add("print", 0, PRINT);
  sym_add("goto", 0, GOTO);
  sym_add("gosub", 0, GOSUB);
  sym_add("return", 0, RETURN);
  sym_add("end", 0, END);
  sym_add("rem", 0, REM);

  printf("- pass1\n");
  u16_Linenum = 0;
  u16_Pc = 0;
  au8_Code[u16_Pc++] = k_TAG;
  au8_Code[u16_Pc++] = k_VERSION;
  u8_NumSymbols = 0;
  while(fgets(ac8_Line, MAX_STR_LEN, fp) != NULL)
  {
    u16_Linenum++;
    pc8_pos = pc8_next = ac8_Line;
    compile_line();
  }

  printf("- pass2\n");
  fseek(fp, 0, SEEK_SET);
  u16_Linenum = 0;
  u16_Pc = 2;
  while(fgets(ac8_Line, MAX_STR_LEN, fp) != NULL)
  {
    u16_Linenum++;
    pc8_pos = pc8_next = ac8_Line;
    compile_line();
  }

  fclose(fp);
  *pu16_len = u16_Pc;
  return au8_Code;
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
    u16_SymIdx = sym_add(ac8_Buff, u8_NumSymbols, ID);
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
  printf("Error: unknown token %s\n", ac8_Buff);
  exit(-1);
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
  //printf("next: %s\n", ac8_Buff);
  return u8_NextTok;
}

static void match(IXX_U8 expected) {
  IXX_U8 tok = next();
  if (tok == expected) {
      return;
  } else {
      printf("%d: expected token: %d\n", u16_Linenum, tok);
      exit(-1);
  }
}

static void compile_line(void)
{
  IXX_U8 tok = lookahead();
  if(tok == ID)
  {
    match(ID);
    IXX_U16 idx = u16_SymIdx;
    tok = lookahead();
    if(tok == ':')
    {
      match(':');
      as_Symbol[idx].value = u16_Pc;
      tok = lookahead();
    }
  }
  if(tok)
  {
    compile_stmt();
  }
}

static void compile_stmt(void)
{
  IXX_U8 tok = next();
  switch(tok)
  {
    case IF: compile_if(); break;
    case LET: compile_var(); break;
    case REM: break;
    case GOTO: compile_goto(); break;
    case GOSUB: compile_gosub(); break;
    case RETURN: compile_return(); break;
    case PRINT: compile_print(); break;
    case END: compile_end(); break;
    default: printf("Error: unknown statement\n"); break;
  }
}

static void compile_if(void)
{
  IXX_U8 tok, op;

  compile_expression();
  op = next();
  compile_expression();
  switch(op)
  {
    case EQ: au8_Code[u16_Pc++] = k_EQUAL; break;
    case NQ: au8_Code[u16_Pc++] = k_NEQUAL; break;
    case LE: au8_Code[u16_Pc++] = k_LESS; break;
    case LQ: au8_Code[u16_Pc++] = k_LESSEQUAL; break;
    case GR: au8_Code[u16_Pc++] = k_GREATER; break;
    case GQ: au8_Code[u16_Pc++] = k_GREATEREQUAL; break;
  }
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
  match(ID);
  IXX_U16 addr = as_Symbol[u16_SymIdx].value;
  au8_Code[u16_Pc++] = k_GOTO;
  au8_Code[u16_Pc++] = addr & 0xFF;
  au8_Code[u16_Pc++] = (addr >> 8) & 0xFF;
}

static void compile_gosub(void)
{
  match(ID);
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
  match('=');
  compile_expression();
  au8_Code[u16_Pc++] = k_LET;
  au8_Code[u16_Pc++] = as_Symbol[idx].value;
}

static void compile_print(void)
{
  IXX_U8 tok = lookahead();
  while(tok)
  {
    if(tok == STR)
    {
      match(STR);
      IXX_U16 len = strlen(ac8_Buff);
      ac8_Buff[len - 1] = '\0';
      au8_Code[u16_Pc++] = k_PRINTS;
      strcpy((char*)&au8_Code[u16_Pc], ac8_Buff + 1);
      u16_Pc += len - 1;
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
      tok = lookahead();
    }
  }
  au8_Code[u16_Pc++] = k_PRINTNL;
}

static void compile_expression(void)
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
  while(op == '*' || op == '/')
  {
    match(op);
    compile_factor();
    if(op == '*')
    {
      au8_Code[u16_Pc++] = k_MUL;
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
  if(tok == '(')
  {
    match('(');
    compile_expression();
    match(')');
  }
  else if(tok == NUM)
  {
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
  }
  else
  {
    match(ID);
    au8_Code[u16_Pc++] = k_VAR;
    au8_Code[u16_Pc++] = as_Symbol[u16_SymIdx].value;
  }
}

static void compile_end(void)
{
  au8_Code[u16_Pc++] = k_END;
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
      u8_NumSymbols++;
      return i;
    }
  }
  return -1;
}
