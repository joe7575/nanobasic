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
  LET = 128, IF, THEN, PRINT, GOTO, END, REM, 
  NUM, STR, ID, EQ, NQ, LE, LQ, GR, GQ
};

// Symbol table
typedef struct {
  char name[MAX_SYM_LEN];
  IXX_U8 type;    // ID, NUM, IF,...
  IXX_U32 value;  // Variable index (0..n) or label address
} t_SYM;

static t_SYM as_Symbol[MAX_NUM_SYM] = {0};
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
static void compile_stmt(IXX_U8 tok);
static void compile_if(void);
static void compile_goto(void);
static void compile_var(void);
static void compile_comment(void);
static void compile_print(void);
static void compile_expression(IXX_U8 tok);
static void compile_term(IXX_U8 tok);
static void compile_factor(IXX_U8 tok);
static void compile_return(void);
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

  u16_Linenum = 0;
  au8_Code[u16_Pc++] = k_TAG;
  au8_Code[u16_Pc++] = k_VERSION;
  // Add keywords
  sym_add("LET", 0, LET);
  sym_add("IF", 0, IF);
  sym_add("THEN", 0, THEN);
  sym_add("PRINT", 0, PRINT);
  sym_add("GOTO", 0, GOTO);
  sym_add("END", 0, END);
  sym_add("REM", 0, REM);

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
  printf("Token: %s\n", ac8_Buff);
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
    u16_SymIdx = sym_add(ac8_Buff, 0, ID);
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
  u8_NextTok = next_token();
  return u8_NextTok;
}

static IXX_U8 next(void)
{
  IXX_U8 tok = u8_NextTok;
  if(pc8_pos == pc8_next)
  {
    tok = lookahead();
  }
  pc8_pos = pc8_next;
  return tok;
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
  IXX_U8 tok = next();
  if(tok == ID)
  {
    IXX_U16 idx = u16_SymIdx;
    tok = next();
    if(tok == ':')
    {
      as_Symbol[idx].value = u16_Pc;
      tok = next();
    }
  }
  if(tok)
  {
    compile_stmt(tok);
  }
}

static void compile_stmt(IXX_U8 tok)
{
  switch(tok)
  {
    case IF: compile_if(); break;
    case LET: compile_var(); break;
    case REM: break;
    case GOTO: compile_goto(); break;
    case PRINT: compile_print(); break;
    case END: compile_end(); break;
    default: printf("Error: unknown statement\n"); break;
  }
}

static void compile_if(void)
{
  IXX_U8 tok, op;

  compile_expression(next());
  op = next();
  compile_expression(next());
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
  compile_stmt(next());
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

static void compile_var(void)
{
  IXX_U8 tok = next();
  IXX_U16 idx = u16_SymIdx;
  match('=');
  compile_expression(next());
  au8_Code[u16_Pc++] = k_VAR;
  au8_Code[u16_Pc++] = as_Symbol[idx].value;
}

static void compile_print(void)
{
  IXX_U8 tok = next();
  while(tok)
  {
    if(tok == STR)
    {
      IXX_U16 len = strlen(ac8_Buff);
      ac8_Buff[len - 1] = '\0';
      au8_Code[u16_Pc++] = k_PRINTS;
      strcpy((char*)&au8_Code[u16_Pc], ac8_Buff + 1);
      u16_Pc += len - 1;
    }
    else
    {
      compile_expression(tok);
      au8_Code[u16_Pc++] = k_PRINTV;
    }
    tok = next();
    if(tok == ',')
    {
      tok = next();
    }
  }
  au8_Code[u16_Pc++] = k_PRINTNL;
}

static void compile_expression(IXX_U8 tok)
{
  compile_term(tok);
  IXX_U8 op = next();
  while(op == '+' || op == '-')
  {
    compile_term(next());
    if(op == '+')
    {
      au8_Code[u16_Pc++] = k_ADD;
    }
    else
    {
      au8_Code[u16_Pc++] = k_SUB;
    }
    op = next();
  }
}

static void compile_term(IXX_U8 tok)
{
  compile_factor(tok);
  IXX_U8 op = next();
  while(op == '*' || op == '/')
  {
    compile_factor(next());
    if(op == '*')
    {
      au8_Code[u16_Pc++] = k_MUL;
    }
    else
    {
      au8_Code[u16_Pc++] = k_DIV;
    }
    op = next();
  }
}

static void compile_factor(IXX_U8 tok)
{
  if(tok == '(')
  {
    compile_expression(next());
    match(')');
  }
  else if(tok == NUM)
  {
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
    au8_Code[u16_Pc++] = k_VAR;
    au8_Code[u16_Pc++] = as_Symbol[u16_SymIdx].value;
  }
}

static void compile_return(void)
{
  au8_Code[u16_Pc++] = k_RETURN;
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
      return i;
    }
  }
  return -1;
}
