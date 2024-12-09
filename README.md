NanoBasic
=========

A small BASIC compiler with virtual machine implemented in C.

```bnf
    line = number statements CR | statements CR | label statements CR
    statements = statement (: statement)*
    statement = FOR var '=' expression TO expression (STEP expression)?
                NEXT (var)?
                IF expression (THEN statements | GOTO number) (ELSE statements)?
                GOTO (number|label)
                GOSUB (number|label)
                RETURN
                ON expression GOSUB expr-list
                ON expression GOTO expr-list
                LET (var '=' expression | var '(' expression ')' '=' expression )
                DIM var '(' expression ')'
                PRINT print-list
                END
                BREAK
                SET1 '(' arr ',' expression ',' expression ')'
                SET2 '(' arr ',' expression ',' expression ')'
                SET4 '(' arr ',' expression ',' expression ')'
                COPY '(' arr ',' expression ',' arr ',' expression ',' expression ')'
                CONST var '=' number
                ERASE arr
                FREE '(' ')'
                TRON
                TROFF

    print-list = ( string | expression | SPC ) ';' print-list
                 ( string | expression | SPC ) ',' print-list
                 ( string | expression | SPC ) ' ' print-list
                 ( string | expression | SPC ) ';'
                 ( string | expression | SPC ) ','

    expression = and_expr (OR and_expr)*
    and_expr = not_expr (AND not_expr)*
    not_expr = NOT comp_expr
               comp_expr
    comp_expr = add_expr (relop add_expr)*
    add_expr = term ((+|-) term)*
    (+|-|ε) term ((+|-) term)*
    term ::= factor ((*|/|MOD) factor)*
    factor ::= const | var | number | '(' expression ')'
               GET1 '(' arr ',' expression ')'
               GET2 '(' arr ',' expression ')'
               GET4 '(' arr ',' expression ')'
    var ::= A | B | C ... | Y | Z
    number ::= digit digit*
    digit ::= 0 | 1 | 2 | 3 | ... | 8 | 9
    relop ::= < (>|=|ε) | > (<|=|ε) | =
    string ::= " ( |!|#|$ ... -|.|/|digit|: ... @|A|B|C ... |X|Y|Z)* "
```

