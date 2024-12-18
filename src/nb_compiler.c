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
#include <setjmp.h>
#include "nb.h"
#include "nb_int.h"

#define MAX_LINE_LEN        128
#define MAX_SYM_LEN         8
#define MAX_XFUNC_PARAMS    8
#define MAX_CODE_PER_LINE   50 // aprox. max. 50 bytes per line

// Token types
enum {
    LET = 128, DIM, FOR, TO,    // 128 - 131
    STEP, NEXT, IF, THEN,       // 132 - 135
    PRINT, GOTO, GOSUB, RETURN, // 136 - 139
    END, REM, AND, OR,          // 140 - 143
    NOT, MOD, NUM, STR,         // 144 - 147
    ID, SID, EQ, NQ,            // 148 - 151
    LE, LQ, GR, GQ,             // 152 - 155
    XFUNC, ARR, BREAK, LABEL,   // 156 - 159
    SET1, SET2, SET4, GET1,     // 160 - 163    
    GET2, GET4, LEFTS, RIGHTS,  // 164 - 167
    MIDS, LEN, VAL, STRS,       // 168 - 171
    SPC, PARAM, COPY, CONST,    // 172 - 175
    ERASE, ELSE, HEXS,     // 176 - 179
    INSTR, ON, TRON, TROFF,     // 180 - 183
    FREE, RND, PARAMS, STRINGS, // 184 - 187
    WHILE, EXIT, DATA, READ,    // 188 - 191
    RESTORE,                    // 192
};

// Expression result types
typedef enum type_t {
    e_ANY = 0,
    e_NUM = NB_NUM,
    e_STR = NB_STR,
    e_ARR = NB_ARR,
    e_CNST,
} type_t;

// Symbol table
typedef struct {
    char name[MAX_SYM_LEN];
    uint8_t  type;   // Token type
    uint8_t  res;    // Reserved for future use
    uint16_t value;  // Variable index (0..n) or label address
} sym_t;

// Define external function
typedef struct {
    uint8_t num_params;
    uint8_t return_type;
    uint8_t type[MAX_XFUNC_PARAMS];
} xfunc_t;

typedef struct {
    uint8_t idx;
    uint16_t pos;
} fwdecl_t;

static void *FilePtr = NULL;
static fwdecl_t a_ForwardDeclaration[cfg_MAX_FW_DECL] = {0};
static uint8_t NumFwDecls = 0;
static xfunc_t a_XFuncs[cfg_MAX_NUM_XFUNC] = {0};
static uint8_t NumXFuncs = 0;
static sym_t a_Symbol[cfg_MAX_NUM_SYM] = {0};
static uint8_t CurrVarIdx = 0;
static uint8_t *p_Code;
static uint16_t Pc = 0;
static uint16_t Linenum = 0;
static uint16_t ErrCount = 0;
static uint16_t SymIdx = 0;
static uint16_t StartOfVars = 0;
static char a_Line[MAX_LINE_LEN];
static char a_Buff[MAX_LINE_LEN];
static char *p_pos = NULL;
static char *p_next = NULL;
static uint32_t Value;
static uint8_t NextTok;
static uint8_t NestedLoopIdx = 0;
static bool TraceOn = false;
static bool FirstDataDeclaration = true;
static jmp_buf  JmpBuf;

static bool get_line(void);
static uint8_t next_token(void);
static uint8_t lookahead(void);
static uint8_t next(void);
static void match(uint8_t expected);
#ifndef cfg_LINE_NUMBERS
static void label(void);
#endif
static void compile_line(void);
static void compile_stmts(void);
static void compile_stmt(void);
static void compile_for(void);
static void compile_if(void);
static void compile_goto(void);
static void compile_gosub(void);
static void compile_return(void);
static void compile_var(uint8_t tok);
static void compile_dim(void);
static void remark(void);
static void compile_print(void);
static void debug_print(uint16_t lineno);
static void compile_string(void);
#ifdef cfg_BASIC_V2
static void compile_exit(void);
#else
static void compile_end(void);
#endif
static type_t compile_xfunc(void);
static void compile_break(void);
#ifdef cfg_BYTE_ACCESS
static void compile_set1(void);
static void compile_set2(void);
static void compile_set4(void);
static void compile_copy(void);
static void compile_set(uint8_t instr);
static void compile_get(uint8_t tok, uint8_t instr);
#endif
static void compile_erase(void);
static void compile_on(void);
static uint8_t list_of_numbers(void);
static void compile_data(void);
static void compile_read(void);
static void compile_restore(void);
#ifdef cfg_BASIC_V2
static void compile_const(void);
static void compile_while(void);
#endif
static void compile_tron(void);
static void compile_troff(void);
static void compile_free(void);
static uint16_t sym_add(char *id, uint32_t val, uint8_t type);
static uint16_t sym_get(char *id);
static void error(char *err, char *id);
static uint8_t get_num_vars(void);
static void forward_declaration(uint16_t idx, uint16_t pos);
static void resolve_forward_declarations(void);
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
void nb_init(void) {
    // Add keywords
    sym_add("let", 0, LET);
    sym_add("dim", 0, DIM);
    sym_add("for", 0, FOR);
    sym_add("to", 0, TO);
    sym_add("step", 0, STEP);
    sym_add("next", 0, NEXT);
    sym_add("if", 0, IF);
    sym_add("then", 0, THEN);
    sym_add("else", 0, ELSE);
#ifdef cfg_BASIC_V2    
    sym_add("end", 0, END);
    sym_add("while", 0, WHILE);
    sym_add("exit", 0, EXIT);
#endif
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
    sym_add("break", 0, BREAK);
    sym_add("data", 0, DATA);
    sym_add("read", 0, READ);
    sym_add("restore", 0, RESTORE);
#ifdef cfg_BYTE_ACCESS
    sym_add("set1", 0, SET1);
    sym_add("set2", 0, SET2);
    sym_add("set4", 0, SET4);
    sym_add("get1", 0, GET1);
    sym_add("get2", 0, GET2);
    sym_add("get4", 0, GET4);
    sym_add("copy", 0, COPY);
#endif
#ifdef cfg_STRING_SUPPORT    
    sym_add("left$", 0, LEFTS);
    sym_add("right$", 0, RIGHTS);
    sym_add("mid$", 0, MIDS);
    sym_add("len", 0, LEN);
    sym_add("val", 0, VAL);
    sym_add("str$", 0, STRS);
    sym_add("spc", 0, SPC);
    sym_add("hex$", 0, HEXS);
    sym_add("param$", 0, PARAMS);
#endif
#if defined(cfg_BASIC_V2) || defined(cfg_STRING_SUPPORT)
    sym_add("string$", 0, STRINGS);
#endif
    sym_add("param", 0, PARAM);
    sym_add("const", 0, CONST);
    sym_add("erase", 0, ERASE);
    sym_add("instr", 0, INSTR);
#ifdef cfg_ON_COMMANDS
    sym_add("on", 0, ON);
#endif     
    sym_add("tron", 0, TRON);
    sym_add("troff", 0, TROFF);
    sym_add("free", 0, FREE);
    sym_add("rnd", 0, RND);
}

uint8_t nb_define_external_function(char *name, uint8_t num_params, uint8_t *types, uint8_t return_type) {
    if(NumXFuncs >= cfg_MAX_NUM_XFUNC) {
        nb_print("Error: too many external functions\n");
        return 0;
    }
    if(num_params > MAX_XFUNC_PARAMS) {
        nb_print("Error: too many parameters\n");
        return 0;
    }
    sym_add(name, NumXFuncs, XFUNC);
    a_XFuncs[NumXFuncs].num_params = num_params;
    a_XFuncs[NumXFuncs].return_type = return_type;
    for(uint8_t i = 0; i < num_params; i++) {
        a_XFuncs[NumXFuncs].type[i] = types[i];
    }
    return NB_XFUNC + NumXFuncs++;
}

void *nb_create(void) {
    t_VM *vm = malloc(sizeof(t_VM));
    if(vm != NULL) {
        memset(vm, 0, sizeof(t_VM));
        nb_mem_init(vm);
        //srand(time(NULL));
    }
    return vm;
}

uint16_t nb_compile(void *pv_vm, void *fp) {
    t_VM *vm = pv_vm;
    p_Code = vm->code;
    StartOfVars = CurrVarIdx;
    CurrVarIdx = 0;
    Pc = 0;
    FilePtr = fp;
    Linenum = 0;
    ErrCount = 0;
    TraceOn = false;
    FirstDataDeclaration = true;

    setjmp(JmpBuf);
    while(get_line()) {
        compile_line();
    }

    if(ErrCount > 0) {
        vm->code_size = 0;
        return ErrCount;
    }

    resolve_forward_declarations();

    vm->code_size = Pc;
    vm->num_vars = get_num_vars();
    return ErrCount;
}

void nb_dump_code(void *pv_vm) {
    t_VM *vm = pv_vm;
    for(uint16_t i = 0; i < vm->code_size; i++) {
        nb_print("%02X ", vm->code[i]);
        if((i % 32) == 31) {
            nb_print("\n");
        } 
    }
    nb_print("\n");
}

void nb_output_symbol_table(void *pv_vm) {
    (void)pv_vm;
    uint8_t idx = 0;

    nb_print("#### Symbol table ####\n");
    nb_print("Variables:\n");
    for(uint16_t i = StartOfVars; i < cfg_MAX_NUM_SYM; i++) {
        if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type != LABEL)
        {
            nb_print("%2u: %8s\n", idx++, a_Symbol[i].name);
        }
    }
#ifndef cfg_LINE_NUMBERS    
    nb_print("Labels:\n");
    for(uint16_t i = StartOfVars; i < cfg_MAX_NUM_SYM; i++) {
        if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type == LABEL)
        {
            nb_print("%16s: %u\n", a_Symbol[i].name, a_Symbol[i].value);
        }
    }
#endif
}

// return 0 if not found
uint16_t nb_get_label_address(void *pv_vm, char *name) {
    (void)pv_vm;
    char str[MAX_SYM_LEN];
    // Convert to lower case
    for(uint16_t i = 0; i < MAX_SYM_LEN; i++) {
        str[i] = tolower(name[i]);
        if(name[i] == '\0') {
            break;
        }
    }

    for(uint16_t i = StartOfVars; i < cfg_MAX_NUM_SYM; i++) {
        if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type == LABEL && strcmp(a_Symbol[i].name, str) == 0)
        {
            return a_Symbol[i].value;
        }
    }
    return 0;
}   

/*************************************************************************************************
** Static functions
*************************************************************************************************/
static bool get_line(void) {
    if(nb_get_code_line(FilePtr, a_Line, MAX_LINE_LEN) != NULL) {
        if(strlen(a_Line) > (MAX_LINE_LEN - 2)) {
            error("line too long", NULL);
        }
        p_pos = p_next = a_Line;

#ifndef cfg_LINE_NUMBERS        
        Linenum++;
#else
        uint8_t tok = lookahead();
        if(tok == NUM) {
            match(NUM);
            if(Value > 0 && Value < 65536) {
                if(Value > Linenum) {
                    Linenum = Value;
                    SymIdx = sym_add(a_Buff, Pc, LABEL);
                } else {
                    error("line number out of order", NULL);
                }
            } else {
                error("line number out of range", NULL);
            }
        }
#endif
        return true;
    }
    return false;
}

static uint8_t next_token(void) {
    if(p_pos == NULL || *p_pos == '\0') {
        return 0; // End of line
    }
    p_next = nb_scanner(p_pos, a_Buff);
    if(a_Buff[0] == '\0') {
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
    if(a_Buff[0] == '=') {
        return EQ;
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
    error("unknown character", a_Buff);
    return 0;
}

static uint8_t lookahead(void) {
    if(p_pos == p_next) {
       NextTok = next_token();
    }
    //nb_print("lookahead: %s\n", a_Buff);
    return NextTok;
}

static uint8_t next(void) {
    if(p_pos == p_next) {
       NextTok = next_token();
    }
    p_pos = p_next;
    return NextTok;
}

static void match(uint8_t expected) {
    uint8_t tok = next();
    if (tok == expected) {
    } else {
        error("syntax error", a_Buff);
    }
}

#ifndef cfg_LINE_NUMBERS
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
    error("label expected", a_Buff);
  }
  match(LABEL);
}
#endif

static void compile_line(void) {
#ifndef cfg_LINE_NUMBERS    
    uint8_t tok = lookahead();
    if(tok == ID || tok == LABEL) {
        uint16_t idx = SymIdx;
        tok = lookahead();
        if(tok == ':') {
            label();
            match(':');
            a_Symbol[idx].value = Pc;
        }
    }
#endif
    if(TraceOn) {
        debug_print(Linenum);
    }
    compile_stmts();
}

static void compile_stmts(void) {
    uint8_t tok = lookahead();
    while(tok && tok != ELSE) {
        compile_stmt();
        tok = lookahead();
        if(tok == ':') {
            match(':');
            tok = lookahead();
        }
        if(Pc >= cfg_MAX_CODE_SIZE - MAX_CODE_PER_LINE) {
            error("code size exceeded", NULL);
            break;
        }
    }
}

static void compile_stmt(void) {
    uint8_t tok = next();
    if(FirstDataDeclaration == false && tok != DATA) {
        error("data statement expected", NULL);
    }
    switch(tok) {
    case FOR: compile_for(); break;
    case IF: compile_if(); break;
    case LET: tok = next(); compile_var(tok); break;
    case ID: compile_var(tok); break;
    case SID: compile_var(tok); break;
    case ARR: compile_var(tok); break;
    case DIM: compile_dim(); break;
    case REM: remark(); break;
    case GOTO: compile_goto(); break;
    case GOSUB: compile_gosub(); break;
    case RETURN: compile_return(); break;
    case PRINT: compile_print(); break;
    case READ: compile_read(); break;
    case DATA: compile_data(); break;
    case RESTORE: compile_restore(); break;
#ifdef cfg_BASIC_V2
    case EXIT: compile_exit(); break;
    case CONST: compile_const(); break;
    case WHILE: compile_while(); break;
#else    
    case END: compile_end(); break;
#endif
    case XFUNC: compile_xfunc(); break;
    case BREAK: compile_break(); break;
#ifdef cfg_BYTE_ACCESS    
    case SET1: compile_set1(); break;
    case SET2: compile_set2(); break;
    case SET4: compile_set4(); break;
    case COPY: compile_copy(); break;
#endif
    case ERASE: compile_erase(); break;
#ifdef cfg_ON_COMMANDS
    case ON: compile_on(); break;
#endif
    case TRON: compile_tron(); break;
    case TROFF: compile_troff(); break;
    case FREE: compile_free(); break;
    default: error("syntax error", a_Buff); break;
    }
}

/* FOR ID '=' <Expression1> TO <Expression2> [STEP <Expression3>]
**   <Statement>...
** NEXT [ID]
**
** <Expression2>  and step counter are pushed on the data stack
*/
static void compile_for(void) {
    uint16_t pc;
    uint8_t tok;
    uint16_t idx;

    if(++NestedLoopIdx >= cfg_MAX_FOR_LOOPS) {
        error("too many nested 'for/while' loops", NULL);
    }

    // FOR ID
    match(ID);
    idx = SymIdx;
    match(EQ);
    compile_expression(e_NUM);
    p_Code[Pc++] = k_POP_VAR_N2;
    p_Code[Pc++] = a_Symbol[idx].value;
    match(TO);
    compile_expression(e_NUM);
    tok = lookahead();
    if(tok == STEP) {
        match(STEP);
        compile_expression(e_NUM);
    } else {
        p_Code[Pc++] = k_PUSH_NUM_N2;
        p_Code[Pc++] = 1;
    }

    pc = Pc;
    while(get_line()) {
        tok = lookahead();
        if(tok == NEXT) {
            break;
        }
        compile_line();
    }

    // NEXT [ID]
    match(NEXT);
    tok = lookahead();
    if(tok == ID) {
        match(ID);
        if(idx != SymIdx) {
            error("mismatched 'for' and 'next'", NULL);
        }
    }
    p_Code[Pc++] = k_NEXT_N4;
    p_Code[Pc++] = pc & 0xFF;
    p_Code[Pc++] = (pc >> 8) & 0xFF;
    p_Code[Pc++] = a_Symbol[idx].value;
    NestedLoopIdx--;
}

#ifdef cfg_BASIC_V2
/*
** WHILE <Expression>
** (not: goto end)
**    <Statement>...
** END
*/
static void compile_while(void) {
    uint16_t pos1, pos2;
    uint8_t tok;

    if(++NestedLoopIdx >= cfg_MAX_FOR_LOOPS) {
        error("too many nested 'for/while' loops", NULL);
    }

    pos1 = Pc;
    compile_expression(e_NUM);
    p_Code[Pc++] = k_IF_N3;
    pos2 = Pc;
    Pc += 2;
    while(get_line()) {
        tok = lookahead();
        if(tok == END) {
            break;
        }
        compile_line();
    }
    match(END);
    p_Code[Pc++] = k_GOTO_N3;
    p_Code[Pc++] = pos1 & 0xFF;
    p_Code[Pc++] = (pos1 >> 8) & 0xFF;
    ACS16(p_Code[pos2]) = Pc;
    NestedLoopIdx--;
}

/*
** IF <Expression> THEN
** (not: goto else)
**    <Statement>...
** (goto end)
** [ELSE
**    <Statement>...]
** END
*/
static void compile_if(void) {
    uint8_t tok;
    uint16_t pos1; // for else address
    uint16_t pos2; // ifend

    compile_expression(e_NUM);
    p_Code[Pc++] = k_IF_N3;
    pos1 = Pc;
    Pc += 2;
    tok = lookahead();
    match(THEN);

    while(get_line()) {
        tok = lookahead();
        if(tok == ELSE || tok == END) {
            break;
        }
        compile_line();
    }

    if(tok == ELSE) {
        p_Code[Pc++] = k_GOTO_N3;
        pos2 = Pc;
        Pc += 2;
        ACS16(p_Code[pos1]) = Pc;

        while(get_line()) {
            tok = lookahead();
            if(tok == END) {
                break;
            }
            compile_line();
        }

        ACS16(p_Code[pos2]) = Pc;
    } else {
        ACS16(p_Code[pos1]) = Pc;
    }
    match(END);
}
#else
static void compile_if(void) {
    uint8_t tok;

    compile_expression(e_NUM);
    p_Code[Pc++] = k_IF_N3;
    uint16_t pos = Pc;
    Pc += 2;
    tok = lookahead();
    if(tok == THEN) {
        match(THEN);
        compile_stmts();
        ACS16(p_Code[pos]) = Pc;
    } else if(tok == GOTO) {
        match(GOTO);
        compile_goto();
        ACS16(p_Code[pos]) = Pc;
    } else {
        error("THEN or GOTO expected", a_Buff);
    }
    tok = lookahead();
    if(tok == ELSE) {
        match(ELSE);
        p_Code[Pc++] = k_GOTO_N3; // goto END
        ACS16(p_Code[pos]) = Pc + 2;
        pos = Pc;
        Pc += 2;
        compile_stmts();
        ACS16(p_Code[pos]) = Pc;
    }
}
#endif

static void compile_goto(void) {
    uint16_t addr;
#ifdef cfg_LINE_NUMBERS
    match(NUM);
    SymIdx = sym_add(a_Buff, 0, LABEL);
    addr = 0;
#else
    label();
    addr = a_Symbol[SymIdx].value;
#endif
    forward_declaration(SymIdx, Pc + 1);
    p_Code[Pc++] = k_GOTO_N3;
    p_Code[Pc++] = addr & 0xFF;
    p_Code[Pc++] = (addr >> 8) & 0xFF;
}

static void compile_gosub(void) {
    uint16_t addr;
#ifdef cfg_LINE_NUMBERS
    match(NUM);
    SymIdx = sym_add(a_Buff, 0, LABEL);
    addr = 0;
#else
    label();
    addr = a_Symbol[SymIdx].value;
#endif
    forward_declaration(SymIdx, Pc + 1);
    p_Code[Pc++] = k_GOSUB_N3;
    p_Code[Pc++] = addr & 0xFF;
    p_Code[Pc++] = (addr >> 8) & 0xFF;
}

static void compile_return(void) {
    p_Code[Pc++] = k_RETURN_N1;
}

static void compile_var(uint8_t tok) {
    uint16_t idx = SymIdx;
    type_t type;

    if(tok == SID) { // let a$ = "string"
        match(EQ);
        compile_expression(e_STR);
        // Var[value] = pop()
        p_Code[Pc++] = k_POP_STR_N2;
        p_Code[Pc++] = a_Symbol[idx].value;
    } else if(tok == ID) { // let a = expression
        match(EQ);
        type = compile_expression(e_ANY);
        if(type == e_NUM) {
            // Var[value] = pop()
            p_Code[Pc++] = k_POP_VAR_N2;
            p_Code[Pc++] = a_Symbol[idx].value;
        } else if(type == e_STR) {
            // Var[value] = pop()
            p_Code[Pc++] = k_POP_STR_N2;
            p_Code[Pc++] = a_Symbol[idx].value;
        } else {
            error("type mismatch", a_Buff);
        }
    } else if(tok == ARR) { // let rx(0) = 1
        match('(');
        compile_expression(e_NUM);
        match(')');
        match(EQ);
        compile_expression(e_NUM);
        p_Code[Pc++] = k_SET_ARR_ELEM_N2;
        p_Code[Pc++] = a_Symbol[idx].value;
    } else {
        error("unknown variable type", a_Buff);
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
        p_Code[Pc++] = k_DIM_ARR_N2;
        p_Code[Pc++] = a_Symbol[idx].value;
    } else {
        error("unknown variable type", a_Buff);
    }
}

static void remark(void) {
    // Skip to end of line
    p_pos[0] = '\0';
}

static void compile_print(void) {
    type_t type;
    bool add_newline = true;
    uint8_t tok = lookahead();
    if(tok == 0) {
        p_Code[Pc++] = k_PRINT_NEWL_N1;
        return;
    }
    while(tok && tok != ELSE) {
        add_newline = true;
        if(tok == STR) {
            compile_string();
            p_Code[Pc++] = k_PRINT_STR_N1;
        } else if(tok == SID) {
            p_Code[Pc++] = k_PUSH_VAR_N2;
            p_Code[Pc++] = a_Symbol[SymIdx].value;
            p_Code[Pc++] = k_PRINT_STR_N1;
            match(SID);
        } else if(tok == SPC) { // spc function
            match('(');
            compile_expression(e_NUM);
            match(')');
            p_Code[Pc++] = k_PRINT_BLANKS_N1;
        } else {
            type = compile_expression(e_ANY);
            if(type == e_NUM) {
                p_Code[Pc++] = k_PRINT_VAL_N1;
            } else if(type == e_STR) {
                p_Code[Pc++] = k_PRINT_STR_N1;
            } else {
                error("type mismatch", a_Buff);
            }
        }
        tok = lookahead();
        if(tok == ',') {
            match(',');
            p_Code[Pc++] = k_PRINT_TAB_N1;
            add_newline = false;
            tok = lookahead();
        } else if(tok == ';') {
            match(';');
            tok = lookahead();
            add_newline = false;
        } else if(tok && tok != ELSE) {
            p_Code[Pc++] = k_PRINT_SPACE_N1;
        }
    }
    if(add_newline) {
        p_Code[Pc++] = k_PRINT_NEWL_N1;
    }
}

static void debug_print(uint16_t lineno) {
    p_Code[Pc++] = k_PRINT_LINENO_N3;
    p_Code[Pc++] = lineno & 0xFF;
    p_Code[Pc++] = (lineno >> 8) & 0xFF;
}

static void compile_string(void) {
    match(STR);
    // push string address
    uint16_t len = strlen(a_Buff);
    a_Buff[len - 1] = '\0';
    p_Code[Pc++] = k_PUSH_STR_Nx;
    p_Code[Pc++] = len - 1; // without quotes but with 0
    strcpy((char*)&p_Code[Pc], a_Buff + 1);
    Pc += len - 1;
}

static void compile_data(void) {
    uint8_t tok;
    if(FirstDataDeclaration) {
        FirstDataDeclaration = false;
        uint16_t idx = sym_add("@data", CurrVarIdx, ID);
        a_Symbol[idx].value = Pc;
    }
    
    while(1) {
        tok = next();
        if(tok == NUM) {
            ACS32(p_Code[Pc]) = Value;
            Pc += 4;
        } else if(tok == STR) {
            uint16_t len = strlen(a_Buff);
            a_Buff[len - 1] = '\0';
            p_Code[Pc++] = len - 1; // without quotes but with 0
            strcpy((char*)&p_Code[Pc], a_Buff + 1);
            Pc += len - 1;
        } else {
            error("syntax error", a_Buff);
        }
        tok = lookahead();
        if(tok == ',') {
            match(',');
        } else {
            break;
        }
    }
}

static void compile_read(void) {
    uint8_t tok;
    uint16_t idx1, idx2;

    while(1) {
        match(ID);
        idx1 = SymIdx;
        idx2 = sym_add("@data", CurrVarIdx, ID);
        p_Code[Pc++] = k_READ_NUM_N4;
        p_Code[Pc++] = a_Symbol[idx2].value;
        forward_declaration(idx2, Pc);
        Pc += 2;
        p_Code[Pc++] = k_POP_VAR_N2;
        p_Code[Pc++] = a_Symbol[idx1].value;
        tok = lookahead();
        if(tok == ',') {
            match(',');
        } else {
            break;
        }
    }
}

static void compile_restore(void) {
    uint16_t idx = sym_add("@data", CurrVarIdx, ID);
    match('(');
    compile_expression(e_NUM);
    match(')');
    p_Code[Pc++] = k_RESTORE_N2;
    p_Code[Pc++] = a_Symbol[idx].value;
}

#ifdef cfg_BASIC_V2
static void compile_exit(void) {
    p_Code[Pc++] = k_END;
}
#else
static void compile_end(void) {
    p_Code[Pc++] = k_END;
}
#endif

static type_t compile_xfunc(void) {
    uint8_t idx = sym_get(a_Buff);
    uint8_t tok;
    if(idx >= NumXFuncs) {
        error("unknown external function", a_Buff);
    }
    match('(');
    for(uint8_t i = 0; i < a_XFuncs[idx].num_params; i++) {
        compile_expression(a_XFuncs[idx].type[i]);
        p_Code[Pc++] = k_PUSH_PARAM_N1;
        tok = lookahead();
        if(tok == ',') {
            match(',');
        }
    }
    p_Code[Pc++] = k_XFUNC_N2;
    p_Code[Pc++] = idx;
    match(')');
    return a_XFuncs[idx].return_type;
}

static void compile_break(void) {
    p_Code[Pc++] = k_BREAK_INSTR_N1;
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
    compile_expression(e_STR);
    match(',');
    compile_expression(e_NUM);
    match(',');
    compile_expression(e_STR);
    match(',');
    compile_expression(e_NUM);
    match(',');
    compile_expression(e_NUM);
    match(')');
    p_Code[Pc++] = k_COPY_N1;
}

static void compile_set(uint8_t instr) {
    uint8_t idx;
    match('(');
    match(SID);
    idx = SymIdx;
    match(',');
    compile_expression(e_NUM);
    match(',');
    compile_expression(e_NUM);
    match(')');
    p_Code[Pc++] = instr;
    p_Code[Pc++] = a_Symbol[idx].value;
}

static void compile_get(uint8_t tok, uint8_t instr) {
    uint8_t idx;
    match(tok);
    match('(');
    match(SID);
    idx = SymIdx;
    match(',');
    compile_expression(e_NUM);
    match(')');
    p_Code[Pc++] = instr;
    p_Code[Pc++] = a_Symbol[idx].value;
}
#endif

#ifdef cfg_BASIC_V2
static void compile_const(void) {
    next();
    uint16_t idx = SymIdx;
    match(EQ);
    match(NUM);
    a_Symbol[idx].type = e_CNST;
    a_Symbol[idx].value = Value;
}
#endif

static void compile_erase(void) {
    uint8_t tok = next();
    if(tok == SID || tok == ARR) {
        p_Code[Pc++] = k_ERASE_ARR_N2;
        p_Code[Pc++] = a_Symbol[SymIdx].value;
    } else {
        error("unknown variable type", a_Buff);
    }
}

#ifdef cfg_ON_COMMANDS
static void compile_on(void) {
    uint16_t pos;
    uint8_t num;

    compile_expression(e_NUM);
    uint8_t tok = lookahead();
    if(tok == GOSUB) {
        match(GOSUB);
        p_Code[Pc++] = k_ON_GOSUB_N2;
    } else if(tok == GOTO) {
        match(GOTO);
        p_Code[Pc++] = k_ON_GOTO_N2;
    } else {
        error("GOSUB or GOTO expected", a_Buff);
    }
    pos = Pc;
    p_Code[Pc++] = 0; // number of elements
    num = list_of_numbers();
    p_Code[pos] = num;
}

static uint8_t list_of_numbers(void) {
    uint8_t num = 0;

    while(1) {
        compile_goto(); 
        num++;
        uint8_t tok = lookahead();
        if(tok == ',') {
            match(',');
        } else {
            break;
        }
    }
    return num;
}
#endif

static void compile_tron(void) {
    TraceOn = true;
}

static void compile_troff(void) {
    TraceOn = false;
}

static void compile_free(void) {
    match('(');
    match(')');
    p_Code[Pc++] = k_FREE_N1;
}

/**************************************************************************************************
 * Symbol table and other helper functions
 *************************************************************************************************/

/*
** Add symbol to symbol table
** id = symbol name
** val = value (in case of variable the index to vm->variables)
** type = type of symbol (ID, SID, ARR, LABEL)
*/
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
    sym[MAX_SYM_LEN - 1] = '\0';

    // Search for existing symbol
    for(uint16_t i = 0; i < cfg_MAX_NUM_SYM; i++) {
        if(strcmp(a_Symbol[i].name, sym) == 0) {
            if(a_Symbol[i].value == 0 && a_Symbol[i].type == LABEL && val > 0) {
                a_Symbol[i].value = val;
            }
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
    error("symbol table full", NULL);
    return 0;
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
    sym[MAX_SYM_LEN - 1] = '\0';

    // Search for existing symbol
    for(uint16_t i = 0; i < cfg_MAX_NUM_SYM; i++) {
        if(strcmp(a_Symbol[i].name, sym) == 0) {
            return a_Symbol[i].value;
        }
        if(a_Symbol[i].name[0] == '\0') {
            break;
        }
    }
    error("unknown symbol", id);
    return 0;
}

static void error(char *err, char *id) {
    nb_print("Error in line %u: ", Linenum);
    if(id != NULL && id[0] != '\0') {
        nb_print("%s at '%s'\n", err, id);
    } else {
        nb_print("%s\n", err);
    }
    ErrCount++;
    p_pos = p_next;
    if(p_pos != NULL) {
        p_pos[0] = '\0';
    }
    longjmp(JmpBuf, 0);
}

static uint8_t get_num_vars(void) {
    uint8_t idx = 0;

    for(uint16_t i = StartOfVars; i < cfg_MAX_NUM_SYM; i++) {
        if(a_Symbol[i].name[0] != '\0' && a_Symbol[i].type != LABEL)
        {
            idx++;
        }
    }
    return idx;
}

// idx = index of symbol (SmyIdx)
// pos = position in code array
static void forward_declaration(uint16_t idx, uint16_t pos) {
    if(NumFwDecls < cfg_MAX_FW_DECL) {
        a_ForwardDeclaration[NumFwDecls].idx = idx;
        a_ForwardDeclaration[NumFwDecls].pos = pos;
        NumFwDecls++;
    } else {
        error("too many forward declarations", NULL);
    }
}

static void resolve_forward_declarations(void) {
    uint16_t idx, pos, addr;
    for(uint8_t i = 0; i < NumFwDecls; i++) {
        idx = a_ForwardDeclaration[i].idx;
        pos = a_ForwardDeclaration[i].pos;
        addr = a_Symbol[idx].value;
        p_Code[pos + 0] = addr & 0xFF;
        p_Code[pos + 1] = (addr >> 8) & 0xFF;
    }
    NumFwDecls = 0;
}

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
            error("type mismatch", NULL);
        }
        p_Code[Pc++] = k_OR_N1;
        op = lookahead();
    }
    if(type != e_ANY && type1 != type) {
        error("type mismatch", a_Buff);
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
            error("type mismatch", a_Buff);
        }
        p_Code[Pc++] = k_AND_N1;
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
            error("type mismatch", a_Buff);
        }
          p_Code[Pc++] = k_NOT_N1;
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
            error("type mismatch", a_Buff);
        }
#ifdef cfg_STRING_SUPPORT        
        if(type1 == e_STR) {
            switch(op) {
            case EQ: p_Code[Pc++] = k_STR_EQUAL_N1; break;
            case NQ: p_Code[Pc++] = k_STR_NOT_EQU_N1; break;
            case LE: p_Code[Pc++] = k_STR_LESS_N1; break;
            case LQ: p_Code[Pc++] = k_STR_LESS_EQU_N1; break;
            case GR: p_Code[Pc++] = k_STR_GREATER_N1; break;
            case GQ: p_Code[Pc++] = k_STR_GREATER_EQU_N1; break;
            default: error("unknown operator", a_Buff); break;
            }
        } else {
#else
        { 
#endif
            switch(op) {
            case EQ: p_Code[Pc++] = k_EQUAL_N1; break;
            case NQ: p_Code[Pc++] = k_NOT_EQUAL_N1; break;
            case LE: p_Code[Pc++] = k_LESS_N1; break;
            case LQ: p_Code[Pc++] = k_LESS_EQU_N1; break;
            case GR: p_Code[Pc++] = k_GREATER_N1; break;
            case GQ: p_Code[Pc++] = k_GREATER_EQU_N1; break;
            default: error("unknown operator", a_Buff); break;
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
            error("type mismatch", a_Buff);
        }
        if(op == '+') {
            if(type1 == e_NUM) {
              p_Code[Pc++] = k_ADD_N1;
            } else {
#ifdef cfg_STRING_SUPPORT                
                p_Code[Pc++] = k_ADD_STR_N1;
#else
                error("type mismatch", a_Buff);
#endif
            }
        } else {
            if(type1 == e_NUM) {
              p_Code[Pc++] = k_SUB_N1;
            } else {
              error("type mismatch", a_Buff);
            }
        }
        op = lookahead();
    }
    return type1;
}

static type_t compile_term(void) {
    type_t type1 = compile_factor();
    uint8_t op = lookahead();
    while(op == '*' || op == '/' || op == MOD) {
        match(op);
        type_t type2 = compile_factor();
        if(type1 != e_NUM || type2 != e_NUM) {
            error("type mismatch", a_Buff);
        }
        if(op == '*') {
          p_Code[Pc++] = k_MUL_N1;
        } else if(op == MOD) {
          p_Code[Pc++] = k_MOD_N1;
        } else {
          p_Code[Pc++] = k_DIV_N1;
        }
        op = lookahead();
    }
    return type1;
}

static type_t compile_factor(void) {
    type_t type = 0;
    uint8_t val;
    uint8_t tok = lookahead();
    switch(tok) {
    case '(':
        match('(');
        type = compile_expression(e_NUM);
        match(')');
        break;
    case e_CNST:
        Value = a_Symbol[SymIdx].value;
        match(e_CNST);
        if(Value < 256)
        {
          p_Code[Pc++] = k_PUSH_NUM_N2;
          p_Code[Pc++] = Value;
        }
        else
        {
          p_Code[Pc++] = k_PUSH_NUM_N5;
          p_Code[Pc++] = Value & 0xFF;
          p_Code[Pc++] = (Value >> 8) & 0xFF;
          p_Code[Pc++] = (Value >> 16) & 0xFF;
          p_Code[Pc++] = (Value >> 24) & 0xFF;
        }
        type = e_NUM;
        break;
    case NUM: // number, like 1234
        match(NUM);
        if(Value < 256)
        {
          p_Code[Pc++] = k_PUSH_NUM_N2;
          p_Code[Pc++] = Value;
        }
        else
        {
          p_Code[Pc++] = k_PUSH_NUM_N5;
          p_Code[Pc++] = Value & 0xFF;
          p_Code[Pc++] = (Value >> 8) & 0xFF;
          p_Code[Pc++] = (Value >> 16) & 0xFF;
          p_Code[Pc++] = (Value >> 24) & 0xFF;
        }
        type = e_NUM;
        break;
    case ID: // variable, like var1
        match(ID);
        p_Code[Pc++] = k_PUSH_VAR_N2;
        p_Code[Pc++] = a_Symbol[SymIdx].value;
        type = e_NUM;
        break;
    case ARR: 
        val = a_Symbol[SymIdx].value;
        match(ARR);
        tok = lookahead();
        if(tok == '(') { // like A(0)
            match('(');
            compile_expression(e_NUM);
            match(')');
            p_Code[Pc++] = k_GET_ARR_ELEM_N2;
            p_Code[Pc++] = val;
            type = e_NUM;
        } else { // left$(A,8) or copy(A, 0,...)
            p_Code[Pc++] = k_PUSH_VAR_N2;
            p_Code[Pc++] = a_Symbol[SymIdx].value;
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
        p_Code[Pc++] = k_PUSH_STR_Nx;
        p_Code[Pc++] = len - 1; // without quotes but with 0
        strcpy((char*)&p_Code[Pc], a_Buff + 1);
        Pc += len - 1;
        type = e_STR;
        break;
    case SID: // string variable, like A$
        match(SID);
        p_Code[Pc++] = k_PUSH_VAR_N2;
        p_Code[Pc++] = a_Symbol[SymIdx].value;
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
        p_Code[Pc++] = k_LEFT_STR_N1;
        type = e_STR;
        break;
    case RIGHTS: // right function
        match(RIGHTS);
        match('(');
        compile_expression(e_STR);
        match(',');
        compile_expression(e_NUM);
        match(')');
        p_Code[Pc++] = k_RIGHT_STR_N1;
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
        p_Code[Pc++] = k_MID_STR_N1;
        type = e_STR;
        break;
    case LEN: // len function
        match(LEN);
        match('(');
        compile_expression(e_STR);
        match(')');
        p_Code[Pc++] = k_STR_LEN_N1;
        type = e_NUM;
        break;
    case VAL: // val function
        match(VAL);
        match('(');
        compile_expression(e_STR);
        match(')');
        p_Code[Pc++] = k_STR_TO_VAL_N1;
        type = e_NUM;
        break;
    case STRS: // str$ function
        match(STRS);
        match('(');
        compile_expression(e_NUM);
        match(')');
        p_Code[Pc++] = k_VAL_TO_STR_N1;
        type = e_STR;
        break;
    case HEXS: // hex function
        match(HEXS);
        match('(');
        compile_expression(e_NUM);
        match(')');
        p_Code[Pc++] = k_VAL_TO_HEX_N1;
        type = e_STR;
        break;
    case INSTR: // instr function
        match(INSTR);
        match('(');
        compile_expression(e_NUM);
        match(',');
        compile_expression(e_STR);
        match(',');
        compile_expression(e_STR);
        match(')');
        p_Code[Pc++] = k_INSTR_N1;
        type = e_NUM;
        break;
    case PARAMS: // Move value from (external) parameter stack to the data stack
        match(PARAMS);
        match('(');
        match(')');
        p_Code[Pc++] = k_PARAMS_N1;
        type = e_STR;
        break;
#endif
#if defined(cfg_BASIC_V2) || defined(cfg_STRING_SUPPORT)        
    case STRINGS: // string$ function
        match(STRINGS);
        match('(');
        compile_expression(e_NUM);
        match(',');
        compile_expression(e_NUM);
        match(')');
        p_Code[Pc++] = k_ALLOC_STR_N1;
        type = e_STR;
        break;
#endif        
    case PARAM: // Move value from (external) parameter stack to the data stack
        match(PARAM);
        match('(');
        match(')');
        p_Code[Pc++] = k_PARAM_N1;
        type = e_NUM;
        break;
    case RND: // Random number
        match(RND);
        match('(');
        compile_expression(e_NUM);
        match(')');
        p_Code[Pc++] = k_RND_N1;
        type = e_NUM;
        break;
    case XFUNC:
        match(XFUNC);
        type = compile_xfunc();
        p_Code[Pc++] = k_PARAM_N1;
        break;
    default:
        error("syntax error", a_Buff);
        break;
    }
    return type;
}
