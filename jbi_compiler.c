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

#define MAX_LINE_LEN        256
#define MAX_SYM_LEN         8

#define MIN(a,b)        ((a) < (b) ? (a) : (b))

// Token types
enum {
    LET = 128, DIM, FOR, TO,    // 128 - 131
    STEP, NEXT, IF, THEN,       // 132 - 135
    PRINT, GOTO, GOSUB, RETURN, // 136 - 139
    END, REM, AND, OR,          // 140 - 143
    NOT, MOD, NUM, STR,         // 144 - 147
    ID, SID, EQ, NQ,            // 148 - 151
    LE, LQ, GR, GQ,             // 152 - 155
    CMD, ARR, BREAK, LABEL,     // 156 - 159
    SET1, SET2, SET4, GET1,     // 160 - 163    
    GET2, GET4, LEFTS, RIGHTS,  // 164 - 167
    MIDS, LEN, VAL, STRS,       // 168 - 171
    SPC, STACK, COPY, CONST,    // 172 - 175
};

typedef enum type_t {
    e_ANY,
    e_NUM,
    e_STR,
    e_ARR,
    e_CNST,
} type_t;

// Keywords
static char *Keywords[] = {
    "let", "dim", "for", "to",
    "step", "next", "if", "then",
    "print", "goto", "gosub", "return",
    "end", "rem", "and", "or",
    "not", "mod", "number", "string",
    "variable", "string variable", "=", "<>",
    "<", "<=", ">", ">=",
    "cmd", "array", "break", "label",
    "set1", "set2", "set4", "get1",
    "get2", "get4", "left$", "right$",
    "mid$", "len", "val", "str$",
    "spc", "stack", "copy", "const",
};

// Symbol table
typedef struct {
    char name[MAX_SYM_LEN];
    uint8_t  type;   // ID, NUM, IF,...
    uint8_t  res;    // Reserved for future use
    uint16_t value;  // Variable index (0..n) or label address
} sym_t;

static sym_t a_Symbol[cfg_MAX_NUM_SYM] = {0};
static uint8_t CurrVarIdx = 0;
static uint8_t a_Code[cfg_MAX_CODE_LEN];
static uint16_t Pc = 0;
static uint16_t PcStart = 0;
static uint16_t Linenum = 0;
static uint16_t ErrCount = 0;
static uint16_t SymIdx = 0;
static uint16_t StartOfVars = 0;
static uint16_t a_ForLoopStartAddr[cfg_MAX_FOR_LOOPS] = {0};
static char a_Line[MAX_LINE_LEN];
static char a_Buff[MAX_LINE_LEN];
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
static void compile_dim(void);
static void remark(void);
static void compile_comment(void);
static void compile_print(void);
static void compile_string(void);
static void compile_end(void);
static void compile_cmd(void);
static void compile_break(void);
static void compile_set1(void);
static void compile_set2(void);
static void compile_set4(void);
static void compile_copy(void);
static void compile_const(void);
static uint16_t sym_add(char *id, uint32_t val, uint8_t type);
static uint16_t sym_get(char *id);
static void error(char *fmt, ...);
static char *token(uint8_t tok);
static void compile_set(uint8_t instr);
static void compile_get(uint8_t tok, uint8_t instr);
static type_t compile_expression(type_t type);
static type_t compile_and_expr(void);
static type_t compile_not_expr(void);
static type_t compile_comp_expr(void);
static type_t compile_add_expr(void);
static type_t compile_term(void);
static type_t compile_factor(void);

/*************************************************************************************************
** API functions
*************************************************************************************************/
void jbi_dump_code(uint8_t *code, uint16_t size) {
    for(uint16_t i = 0; i < size; i++) {
        printf("%02X ", code[i]);
        if((i % 32) == 31) {
            printf("\n");
        }
    }
    printf("\n");
}
void jbi_init(void) {
    // Add keywords
    sym_add("let", 0, LET);
    sym_add("dim", 0, DIM);
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
    sym_add("mod", 0, MOD);
    sym_add("cmd", 0, CMD);
    sym_add("break", 0, BREAK);
#ifdef cfg_BYTE_ACCESS
    sym_add("set1", 0, SET1);
    sym_add("set2", 0, SET2);
    sym_add("set4", 0, SET4);
    sym_add("get1", 0, GET1);
    sym_add("get2", 0, GET2);
    sym_add("get4", 0, GET4);
#endif
#ifdef cfg_STRING_SUPPORT    
    sym_add("left$", 0, LEFTS);
    sym_add("right$", 0, RIGHTS);
    sym_add("mid$", 0, MIDS);
    sym_add("len", 0, LEN);
    sym_add("val", 0, VAL);
    sym_add("str$", 0, STRS);
    sym_add("spc", 0, SPC);
#endif
    sym_add("stack", 0, STACK);
    sym_add("copy", 0, COPY);
    sym_add("const", 0, CONST);
    StartOfVars = CurrVarIdx;
    CurrVarIdx = 0;
    Pc = 0;
    a_Code[Pc++] = k_TAG;
    a_Code[Pc++] = k_VERSION;
    PcStart = Pc;
}

uint8_t *jbi_compiler(char *filename, uint16_t *p_len, uint8_t *p_num_vars) {
    FILE *fp = fopen(filename, "r");
    if(fp == NULL) {
        printf("Error: could not open file %s\n", filename);
        return NULL;
    }

    printf("- pass1\n");
    compile(fp);

    if(ErrCount > 0) {
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

void jbi_output_symbol_table(void) {
    printf("#### Symbol table ####\n");
    printf("Variables:\n");
    for(uint16_t i = StartOfVars; i < cfg_MAX_NUM_SYM; i++) {
        if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type != LABEL)
        {
            printf("%16s: %u\n", a_Symbol[i].name, a_Symbol[i].value);
        }
    }
#ifndef cfg_LINE_NUMBERS    
    printf("Labels:\n");
    for(uint16_t i = StartOfVars; i < cfg_MAX_NUM_SYM; i++) {
        if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type == LABEL)
        {
            printf("%16s: %u\n", a_Symbol[i].name, a_Symbol[i].value);
        }
    }
#endif
}

uint16_t jbi_get_label_address(char *name) {
    for(uint16_t i = StartOfVars; i < cfg_MAX_NUM_SYM; i++) {
        if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type == LABEL && strcmp(a_Symbol[i].name, name) == 0)
        {
            return a_Symbol[i].value;
        }
    }
    return 0;
}   

uint16_t jbi_get_var_num(char *name) {
    for(uint16_t i = StartOfVars; i < cfg_MAX_NUM_SYM; i++) {
        if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type != LABEL && strcmp(a_Symbol[i].name, name) == 0)
        {
            return a_Symbol[i].value;
        }
    }
    return -1;
}   

/*************************************************************************************************
** Static functions
*************************************************************************************************/
static void compile(FILE *fp) {
    Linenum = 0;
    ErrCount = 0;
    Pc = PcStart;

    fseek(fp, 0, SEEK_SET);
    while(fgets(a_Line, MAX_LINE_LEN, fp) != NULL) {
        Linenum++;
        p_pos = p_next = a_Line;
        compile_line();
    }
}

static uint8_t next_token(void) {
    if(*p_pos == '\0') {
        return 0; // End of line
    }
    p_next = jbi_scanner(p_pos, a_Buff);
    if(a_Buff[0] == '\0') {
       return 0; // End of line
    }
    if(a_Buff[0] == '\n') {
       return 0; // End of line
    }
    if(a_Buff[0] == '\"') {
        return STR;
    }
    if(isdigit(a_Buff[0])) {
        Value = atoi(a_Buff);
       return NUM;
    }
    if(isalpha(a_Buff[0])) {
        uint16_t len = strlen(a_Buff);
        uint8_t type = a_Buff[len - 1] == '$' ? SID : ID;

        SymIdx = sym_add(a_Buff, CurrVarIdx, type);
        return a_Symbol[SymIdx].type;
    }
    if(a_Buff[0] == '<') {
        // parse '<=' or '<'
        if (a_Buff[1] == '=') {
            return LQ;
        }
        return LE;
    }
    if(a_Buff[0] == '>') {
        // parse '>=' or '>'
        if (a_Buff[0] == '=') {
            return GQ;
        }
        return GR;
    }
    if(strlen(a_Buff) == 1) {
        return a_Buff[0]; // Single character
    }
    error("unknown token %c", a_Buff[0]);
    return 0;
}

static uint8_t lookahead(void) {
    if(p_pos == p_next) {
       NextTok = next_token();
    }
    //printf("lookahead: %s\n", a_Buff);
    return NextTok;
}

static uint8_t next(void) {
    if(p_pos == p_next) {
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
        error("%s expected instead of '%s'", token(expected), a_Buff);
    }
}

static void label(void) {
  uint8_t tok = lookahead();
  if(tok == ID) { // Token recognized as variable?
    // Convert to label
    a_Symbol[SymIdx].type = LABEL;
    NextTok = LABEL;
    CurrVarIdx--;
  } else if(tok == LABEL) {
    // Already a label
  } else {
    error("label expected instead of '%s'", a_Buff);
  }
  match(LABEL);
}

static void compile_line(void) {
    uint8_t tok = lookahead();
    if(tok == NUM) {
        match(NUM);
        Linenum = Value;
        sym_add(a_Buff, Pc, LABEL);
        tok = lookahead();
    } else if(tok == ID || tok == LABEL) {
        uint16_t idx = SymIdx;
        label();
        tok = lookahead();
        if(tok == ':') {
            match(':');
            a_Symbol[idx].value = Pc;
            tok = lookahead();
        }
    }
    while(tok) {
        compile_stmt();
        tok = lookahead();
        if(tok == ';') {
            match(';');
            tok = lookahead();
        }
    }
}

static void compile_stmt(void) {
    uint8_t tok = next();
    switch(tok) {
    case FOR: compile_for(); break;
    case IF: compile_if(); break;
    case LET: compile_var(); break;
    case DIM: compile_dim(); break;
    case REM: remark(); break;
    case GOTO: compile_goto(); break;
    case GOSUB: compile_gosub(); break;
    case RETURN: compile_return(); break;
    case PRINT: compile_print(); break;
    case NEXT: compile_next(); break;
    case END: compile_end(); break;
    case CMD: compile_cmd(); break;
    case BREAK: compile_break(); break;
#ifdef cfg_BYTE_ACCESS    
    case SET1: compile_set1(); break;
    case SET2: compile_set2(); break;
    case SET4: compile_set4(); break;
    case COPY: compile_copy(); break;
#endif
    case CONST: compile_const(); break;
    default: error("unknown statement %s", token(tok)); break;
    }
}

// FOR ID '=' <Expression1> TO <Expression2> [STEP <Expression3>]
// <Statement>...
// NEXT [ID]
//
// <Expression1>            (set variable ID)
// <Expression2>            (push value)
// <Expression3> or 1       (push value)
// start:
static void compile_for(void) {
    uint8_t tok;
    match(ID);
    uint16_t idx = SymIdx;
    match('=');
    compile_expression(e_NUM);
    a_Code[Pc++] = k_POP_VAR_N2;
    a_Code[Pc++] = a_Symbol[idx].value;
    match(TO);
    compile_expression(e_NUM);
    tok = lookahead();
    if(tok == STEP) {
        match(STEP);
        compile_expression(e_NUM);
    } else {
        a_Code[Pc++] = k_PUSH_NUM_N2;
        a_Code[Pc++] = 1;
    }
    // Save start address
    if(ForLoopIdx >= cfg_MAX_FOR_LOOPS) {
        error("too many nested 'for' loops");
        return;
    }
    a_ForLoopStartAddr[ForLoopIdx++] = Pc;
}

// ID =  ID + Step ID3            k_NEXT_N4 start var
// ID <= ID2 GOTO start           
static void compile_next(void) {
    match(ID);
    uint16_t idx = SymIdx;
    if(ForLoopIdx == 0) {
        error("'next' without 'for'");
        return;
    }
    uint16_t addr = a_ForLoopStartAddr[--ForLoopIdx];
    a_Code[Pc++] = k_NEXT_N4;
    a_Code[Pc++] = addr & 0xFF;
    a_Code[Pc++] = (addr >> 8) & 0xFF;
    a_Code[Pc++] = a_Symbol[idx].value;
}

static void compile_if(void) {
    uint8_t tok, op;

    compile_expression(e_NUM);
    a_Code[Pc++] = k_IF_N2;
    uint16_t pos = Pc;
    Pc += 2;
    match(THEN);
    compile_stmt();
    a_Code[pos] = Pc & 0xFF;
    a_Code[pos + 1] = (Pc >> 8) & 0xFF;
}

static void compile_goto(void) {
    uint8_t tok = lookahead();
    uint16_t addr;
    if(tok == LABEL) {
        label();
        addr = a_Symbol[SymIdx].value;
    }else if(tok == NUM) {
        match(NUM);
        addr = sym_get(a_Buff);
    } else {
        error("label or number expected instead of '%s'", a_Buff);
        return;
    }
    a_Code[Pc++] = k_GOTO_N2;
    a_Code[Pc++] = addr & 0xFF;
    a_Code[Pc++] = (addr >> 8) & 0xFF;
}

static void compile_gosub(void) {
    label();
    uint16_t addr = a_Symbol[SymIdx].value;
    a_Code[Pc++] = k_GOSUB_N2;
    a_Code[Pc++] = addr & 0xFF;
    a_Code[Pc++] = (addr >> 8) & 0xFF;
}

static void compile_return(void) {
    a_Code[Pc++] = k_RETURN_N1;
}

static void compile_var(void) {
    uint8_t tok = next();
    uint16_t idx = SymIdx;
    type_t type;

    if(tok == SID) { // let a$ = "string"
        match('=');
        compile_expression(e_STR);
        // Var[value] = pop()
        a_Code[Pc++] = k_POP_STR_N2;
        a_Code[Pc++] = a_Symbol[idx].value;
    } else if(tok == ID) { // let a = expression
        match('=');
        type = compile_expression(e_ANY);
        if(type == e_NUM) {
            // Var[value] = pop()
            a_Code[Pc++] = k_POP_VAR_N2;
            a_Code[Pc++] = a_Symbol[idx].value;
        } else if(type == e_STR) {
            // Var[value] = pop()
            a_Code[Pc++] = k_POP_STR_N2;
            a_Code[Pc++] = a_Symbol[idx].value;
        } else {
            error("type mismatch");
            return;
        }
    } else if(tok == ARR) { // let rx(0) = 1
        match('(');
        compile_expression(e_NUM);
        match(')');
        match('=');
        compile_expression(e_NUM);
        a_Code[Pc++] = k_SET_ARR_ELEM_N2;
        a_Code[Pc++] = a_Symbol[idx].value;
    } else {
        error("unknown variable type '%s'", token(tok));
    }
}

static void compile_dim(void) {
    uint8_t tok = next();
    if(tok == ID || tok == ARR) {
        uint16_t idx = SymIdx;
        a_Symbol[idx].type = ARR;
        match('(');
        compile_expression(e_NUM);        
        match(')');
        a_Code[Pc++] = k_DIM_ARR_N2;
        a_Code[Pc++] = a_Symbol[idx].value;
    } else {
        error("unknown variable type '%u'", tok);
    }
}

static void remark(void) {
    // Skip to end of line
    p_pos[0] = '\0';
}

static void compile_print(void) {
    type_t type;
    uint8_t tok = lookahead();
    if(tok == 0) {
        a_Code[Pc++] = k_PRINT_NEWL_N1;
        return;
    }
    while(tok) {
        if(tok == STR) {
            compile_string();
            a_Code[Pc++] = k_PRINT_STR_N1;
        } else if(tok == SID) {
            a_Code[Pc++] = k_PUSH_VAR_N2;
            a_Code[Pc++] = a_Symbol[SymIdx].value;
            a_Code[Pc++] = k_PRINT_STR_N1;
            match(SID);
        } else if(tok == SPC) { // spc function
            match('(');
            compile_expression(e_NUM);
            match(')');
            a_Code[Pc++] = k_PRINT_BLANKS_N1;
        } else {
            type = compile_expression(e_ANY);
            if(type == e_NUM) {
                a_Code[Pc++] = k_PRINT_VAL_N1;
            } else if(type == e_STR) {
                a_Code[Pc++] = k_PRINT_STR_N1;
            } else {
                error("type mismatch");
                return;
            }
        }
        tok = lookahead();
        if(tok == ',') {
            match(',');
            a_Code[Pc++] = k_PRINT_TAB_N1;
            tok = lookahead();
        } else if(tok == ';') {
            match(';');
            tok = lookahead();
        } else {
            a_Code[Pc++] = k_PRINT_NEWL_N1;
        }
    }
}

static void compile_string(void) {
    match(STR);
    // push string address
    uint16_t len = strlen(a_Buff);
    a_Buff[len - 1] = '\0';
    a_Code[Pc++] = k_PUSH_STR_Nx;
    a_Code[Pc++] = len - 1; // without quotes but with 0
    strcpy((char*)&a_Code[Pc], a_Buff + 1);
    Pc += len - 1;
}

static void compile_end(void) {
    a_Code[Pc++] = k_END;
}

static void compile_cmd(void) {
    match('(');
    compile_expression(e_NUM);
    match(')');
    a_Code[Pc++] = k_FUNC_CALL;
    a_Code[Pc++] = k_CALL_CMD_N2;
}

static void compile_break(void) {
    a_Code[Pc++] = k_BREAK_INSTR_N1;
}

#ifdef cfg_BYTE_ACCESS
static void compile_set1(void) {
    compile_set(k_SET_ARR_1BYTE_N2);
}

static void compile_set2(void) {
    compile_set(k_SET_ARR_2BYTE_N2);
}

static void compile_set4(void) {
    compile_set(k_SET_ARR_4BYTE_N2);
}

static void compile_copy(void) {
    match('(');
    compile_expression(e_ARR);
    match(',');
    compile_expression(e_NUM);
    match(',');
    compile_expression(e_ARR);
    match(',');
    compile_expression(e_NUM);
    match(',');
    compile_expression(e_NUM);
    match(')');
    a_Code[Pc++] = k_COPY_N1;
}
#endif

static void compile_const(void) {
    uint8_t tok = next();
    uint16_t idx = SymIdx;
    match('=');
    match(NUM);
    a_Symbol[idx].type = e_CNST;
    a_Symbol[idx].value = Value;
}

/**************************************************************************************************
 * Symbol table and other helper functions
 *************************************************************************************************/
static uint16_t sym_add(char *id, uint32_t val, uint8_t type) {
    uint16_t start = 0;
    char sym[MAX_SYM_LEN];

    // Convert to lower case
    for(uint16_t i = 0; i < MAX_SYM_LEN; i++) {
        sym[i] = tolower(id[i]);
        if(sym[i] == '\0') {
            break;
        }
    }
    // Search for existing symbol
    for(uint16_t i = 0; i < cfg_MAX_NUM_SYM; i++) {
        if(strcmp(a_Symbol[i].name, sym) == 0) {
            return i;
        }
        if(a_Symbol[i].name[0] == '\0') {
            start = i;
            break;
        }
    }

    // Add new symbol
    for(uint16_t i = start; i < cfg_MAX_NUM_SYM; i++) {
        if(a_Symbol[i].name[0] == '\0') {
            memcpy(a_Symbol[i].name, sym, MIN(strlen(sym), MAX_SYM_LEN));
            a_Symbol[i].value = val;
            a_Symbol[i].type = type;
            CurrVarIdx++;
            return i;
        }
    }
    error("symbol table full");
}

static uint16_t sym_get(char *id) {
    char sym[MAX_SYM_LEN];

    // Convert to lower case
    for(uint16_t i = 0; i < MAX_SYM_LEN; i++) {
        sym[i] = tolower(id[i]);
        if(sym[i] == '\0') {
            break;
        }
    }
    // Search for existing symbol
    for(uint16_t i = 0; i < cfg_MAX_NUM_SYM; i++) {
        if(strcmp(a_Symbol[i].name, sym) == 0) {
            return a_Symbol[i].value;
        }
        if(a_Symbol[i].name[0] == '\0') {
            break;
        }
    }
    return 0;
}

static void error(char *fmt, ...) {
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

static char *token(uint8_t tok) {
    static char s[5];
    if(tok >= LET && tok <= STACK) {
        return Keywords[tok - LET];
    } else {
        s[0] = '\'';
        s[1] = tok;
        s[2] = '\'';
        s[3] = '\0';
        return s;
    }
} 

#ifdef cfg_BYTE_ACCESS    
static void compile_set(uint8_t instr) {
    uint8_t idx;
    match('(');
    match(ARR);
    idx = SymIdx;
    match(',');
    compile_expression(e_NUM);
    match(',');
    compile_expression(e_NUM);
    match(')');
    a_Code[Pc++] = instr;
    a_Code[Pc++] = a_Symbol[idx].value;
}

static void compile_get(uint8_t tok, uint8_t instr) {
    uint8_t idx, type;
    match(tok);
    match('(');
    match(ARR);
    idx = SymIdx;
    match(',');
    type = compile_expression(e_NUM);
    match(')');
    a_Code[Pc++] = instr;
    a_Code[Pc++] = a_Symbol[idx].value;
}
#endif

/**************************************************************************************************
 * Expression compiler
 *************************************************************************************************/
static type_t compile_expression(type_t type) {
    type_t type1 = compile_and_expr();
    uint8_t op = lookahead();
    while(op == OR) {
        match(op);
        type_t type2 = compile_and_expr();
        if(type1 != e_NUM || type2 != e_NUM) {
            error("type mismatch");
            return type1;
        }
        a_Code[Pc++] = k_OR_N1;
        op = lookahead();
    }
    if(type != e_ANY && type1 != type) {
        error("type mismatch");
    }
    return type1;
}

static type_t compile_and_expr(void) {
    type_t type1 = compile_not_expr();
    uint8_t op = lookahead();
    while(op == AND) {
        match(op);
        type_t type2 = compile_not_expr();
        if(type1 != e_NUM || type2 != e_NUM) {
            error("type mismatch");
            return type2;
        }
        a_Code[Pc++] = k_AND_N1;
        op = lookahead();
    }
    return type1;
}

static type_t compile_not_expr(void) {
    type_t type;
    uint8_t op = lookahead();
    if(op == NOT) {
        match(op);
        type = compile_comp_expr();
        if(type != e_NUM) {
            error("type mismatch");
            return type;
        }
          a_Code[Pc++] = k_NOT_N1;
    } else {
        type = compile_comp_expr();
    }
    return type;
}

static type_t compile_comp_expr(void) {
    type_t type1 = compile_add_expr();
    uint8_t op = lookahead();
    while(op == EQ || op == NQ || op == LE || op == LQ || op == GR || op == GQ) {
        match(op);
        type_t type2 = compile_add_expr();
        if(type1 != type2) {
            error("type mismatch");
            return type1;
        }
#ifdef cfg_STRING_SUPPORT        
        if(type1 == e_STR) {
            switch(op) {
            case EQ: a_Code[Pc++] = k_STR_EQUAL_N1; break;
            case NQ: a_Code[Pc++] = k_STR_NOT_EQU_N1; break;
            case LE: a_Code[Pc++] = k_STR_LESS_N1; break;
            case LQ: a_Code[Pc++] = k_STR_LESS_EQU_N1; break;
            case GR: a_Code[Pc++] = k_STR_GREATER_N1; break;
            case GQ: a_Code[Pc++] = k_STR_GREATER_EQU_N1; break;
            default: error("unknown operator '%u'", op); break;
            }
        } else {
#else
        { 
#endif
            switch(op) {
            case EQ: a_Code[Pc++] = k_EQUAL_N1; break;
            case NQ: a_Code[Pc++] = k_NOT_EQUAL_N1; break;
            case LE: a_Code[Pc++] = k_LESS_N1; break;
            case LQ: a_Code[Pc++] = k_LESS_EQU_N1; break;
            case GR: a_Code[Pc++] = k_GREATER_N1; break;
            case GQ: a_Code[Pc++] = k_GREATER_EQU_N1; break;
            default: error("unknown operator '%u'", op); break;
            }
        }
        op = lookahead();
    }
    return type1;
}

static type_t compile_add_expr(void) {
    type_t type1 = compile_term();
    uint8_t op = lookahead();
    while(op == '+' || op == '-') {
        match(op);
        type_t type2 = compile_term();
        if(type1 != type2) {
            error("type mismatch");
            return type1;
        }
        if(op == '+') {
            if(type1 == e_NUM) {
              a_Code[Pc++] = k_ADD_N1;
            } else {
#ifdef cfg_STRING_SUPPORT                
                a_Code[Pc++] = k_ADD_STR_N1;
#else
                error("type mismatch");
                return type1;
#endif
            }
        } else {
            if(type1 == e_NUM) {
              a_Code[Pc++] = k_SUB_N1;
            } else {
              error("type mismatch");
              return type1;
            }
        }
        op = lookahead();
    }
    return type1;
}

static type_t compile_term(void) {
    type_t type1 = compile_factor();
    uint8_t op = lookahead();
    while(op == '*' || op == '/' || op == '%') {
        match(op);
        type_t type2 = compile_factor();
        if(type1 != e_NUM || type2 != e_NUM) {
            error("type mismatch");
            return type2;
        }
        if(op == '*') {
          a_Code[Pc++] = k_MUL_N1;
        } else if(op == MOD) {
          a_Code[Pc++] = k_MOD_N1;
        } else {
          a_Code[Pc++] = k_DIV_N1;
        }
        op = lookahead();
    }
    return type1;
}

static type_t compile_factor(void) {
    type_t type = 0;
    uint8_t idx;
    uint8_t tok = lookahead();
    switch(tok) {
    case '(':
        match('(');
        compile_expression(e_NUM);
        match(')');
        break;
    case e_CNST:
        Value = a_Symbol[SymIdx].value;
        match(e_CNST);
        if(Value < 256)
        {
          a_Code[Pc++] = k_PUSH_NUM_N2;
          a_Code[Pc++] = Value;
        }
        else
        {
          a_Code[Pc++] = k_PUSH_NUM_N5;
          a_Code[Pc++] = Value & 0xFF;
          a_Code[Pc++] = (Value >> 8) & 0xFF;
          a_Code[Pc++] = (Value >> 16) & 0xFF;
          a_Code[Pc++] = (Value >> 24) & 0xFF;
        }
        type = e_NUM;
        break;
    case NUM: // number, like 1234
        match(NUM);
        if(Value < 256)
        {
          a_Code[Pc++] = k_PUSH_NUM_N2;
          a_Code[Pc++] = Value;
        }
        else
        {
          a_Code[Pc++] = k_PUSH_NUM_N5;
          a_Code[Pc++] = Value & 0xFF;
          a_Code[Pc++] = (Value >> 8) & 0xFF;
          a_Code[Pc++] = (Value >> 16) & 0xFF;
          a_Code[Pc++] = (Value >> 24) & 0xFF;
        }
        type = e_NUM;
        break;
    case ID: // variable, like var1
        match(ID);
        a_Code[Pc++] = k_PUSH_VAR_N2;
        a_Code[Pc++] = a_Symbol[SymIdx].value;
        type = e_NUM;
        break;
    case ARR: 
        uint8_t val = a_Symbol[SymIdx].value;
        match(ARR);
        tok = lookahead();
        if(tok == '(') { // like A(0)
            match('(');
            compile_expression(e_NUM);
            match(')');
            a_Code[Pc++] = k_GET_ARR_ELEM_N2;
            a_Code[Pc++] = val;
            type = e_NUM;
        } else { // left$(A,8) or copy(A+0,...)
            a_Code[Pc++] = k_PUSH_VAR_N2;
            a_Code[Pc++] = a_Symbol[SymIdx].value;
            type = e_ARR;
        }
        break;
#ifdef cfg_BYTE_ACCESS        
    case GET1: // get1 function
        compile_get(GET1, k_GET_ARR_1BYTE_N2);
        type = e_NUM;
        break;
    case GET2: // get2 function
        compile_get(GET2, k_GET_ARR_2BYTE_N2);
        type = e_NUM;
        break;
    case GET4: // get4 function
        compile_get(GET4, k_GET_ARR_4BYTE_N2);
        type = e_NUM;
        break;
#endif
    case STR: // string, like "Hello"
        match(STR);
        // push string address
        uint16_t len = strlen(a_Buff);
        a_Buff[len - 1] = '\0';
        a_Code[Pc++] = k_PUSH_STR_Nx;
        a_Code[Pc++] = len - 1; // without quotes but with 0
        strcpy((char*)&a_Code[Pc], a_Buff + 1);
        Pc += len - 1;
        type = e_STR;
        break;
    case SID: // string variable, like A$
        match(SID);
        a_Code[Pc++] = k_PUSH_VAR_N2;
        a_Code[Pc++] = a_Symbol[SymIdx].value;
        type = e_STR;
        break;
#ifdef cfg_STRING_SUPPORT        
    case LEFTS: // left function
        match(LEFTS);
        match('(');
        compile_expression(e_STR);
        match(',');
        compile_expression(e_NUM);
        match(')');
        a_Code[Pc++] = k_FUNC_CALL;
        a_Code[Pc++] = k_LEFT_STR_N2;
        type = e_STR;
        break;
    case RIGHTS: // right function
        match(RIGHTS);
        match('(');
        compile_expression(e_STR);
        match(',');
        compile_expression(e_NUM);
        match(')');
        a_Code[Pc++] = k_FUNC_CALL;
        a_Code[Pc++] = k_RIGHT_STR_N2;
        type = e_STR;
        break;
    case MIDS: // mid function
        match(MIDS);
        match('(');
        compile_expression(e_STR);
        match(',');
        compile_expression(e_NUM);
        match(',');
        type = compile_expression(e_NUM);
        match(')');
        a_Code[Pc++] = k_FUNC_CALL;
        a_Code[Pc++] = k_MID_STR_N2;
        type = e_STR;
        break;
    case LEN: // len function
        match(LEN);
        match('(');
        compile_expression(e_STR);
        match(')');
        a_Code[Pc++] = k_FUNC_CALL;
        a_Code[Pc++] = k_STR_LEN_N2;
        type = e_NUM;
        break;
    case VAL: // val function
        match(VAL);
        match('(');
        compile_expression(e_STR);
        match(')');
        a_Code[Pc++] = k_FUNC_CALL;
        a_Code[Pc++] = k_STR_TO_VAL_N2;
        type = e_NUM;
        break;
    case STRS: // str$ function
        match(STRS);
        match('(');
        compile_expression(e_NUM);
        match(')');
        a_Code[Pc++] = k_FUNC_CALL;
        a_Code[Pc++] = k_VAL_TO_STR_N2;
        type = e_STR;
        break;
#endif        
    case STACK: // Move value from (external) parameter stack to the data stack
        match(STACK);
        match('(');
        match(')');
        a_Code[Pc++] = k_POP_PUSH_N1;
        type = e_NUM;
        break;
    default:
        error("unknown factor '%u'", tok);
        break;
    }
    return type;
}