NanoBasic
=========

A small BASIC compiler with virtual machine.
This software is written from scratch in C.

Most of the common BASIC keywords are supported:

```bnf
    PRINT expression-list [ ; | , ]
    FOR numeric_variable '=' numeric_expression TO numeric_expression [ STEP number ]
    IF relation-expression THEN statement-list [ ELSE statement-list ]
    IF relation-expression GOTO line-number [ ELSE statement-list ]
    GOTO line-number
    GOSUB line-number
    ON numeric_expression GOSUB line-number-list
    ON numeric_expression GOTO line-number-list
    LET variable = expression
    LET string-variable$ = string-expression$
    DIM array-variable "(" numeric_expression ")"
    ERASE ( array-variable | string-variable$ )
    READ variable-list
    DATA ( constant-list | string-list )       ; Only at the end of the program
    RESTORE "(" number ")"                     ; Number is offset (0..n), not line number
    RETURN
    END
    BREAK
    TRON, TROFF
    FREE
    AND, NOT, OR, RND, MOD, LEN, VAL, SPC, NIL
    LEN, CHR$, MID$, LEFT$, RIGHT$, STR$, HEX$, STRING$
```

Basic V2 features (optional):

```bnf
    CONST variable = number

    IF relation-expression THEN 
        statements...
    [ ELSE
        statements... ]
    END

    WHILE relation-expression
        statements...
    END

    variable = expression                                  ; without LET
    string-variable$ = string-expression$                  ; without LET

    EXIT                                                   ; instead of END
```

Supported data types are:

- Unsigned Integer, 32 bit (0 to 4294967295)
- String (up to 120 characters)
- Array (one dimension, up to 128 elements)
- Constant (numeric only)

The compiler is able to generate a binary file that can be executed by the virtual machine.
The goal of NanoBasic was to be a small and fast, due to compiler and VM combination.
The main purpose of NanoBasic is to be embedded in other applications, to provide a simple scripting language
for configuration and as glue code.

The Basic language is inspired by the original Microsoft Basic known from Home Computers in the 80s.
