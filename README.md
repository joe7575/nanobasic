NanoBasic
=========

A small BASIC compiler with virtual machine implemented.
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
    CONST variable = number
    LET variable = expression
    LET string-variable$ = string-expression$
    DIM array-variable "(" numeric_expression ")"
    ERASE array-variable
    RETURN
    END
    BREAK
    TRON, TROFF
    FREE
    AND, NOT, OR, RND, MOD, LEN, VAL, SPC
    LEN, CHR$, MID$, LEFT$, RIGHT$, STR$, HEX$
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
