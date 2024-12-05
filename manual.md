# Tiny Basic

Die Syntax von TinyBasic ist durchaus überschaubar, deckt aber alles ab, was dringend benötigt
wird (bereits bereinigt, was nicht benötigt wird).

```bnf
    line ::= number statement CR | statement CR
    statement ::= PRINT expr-list
                  IF expression relop expression THEN statement
                  GOTO number
                  LET var = expression
                  GOSUB number
                  RETURN
                  END
    expr-list ::= (string|expression) (, (string|expression) )*
    var-list ::= var (, var)*
    expression ::= (+|-|ε) term ((+|-) term)*
    term ::= factor ((*|/) factor)*
    factor ::= var | number | (expression)
    var ::= A | B | C ... | Y | Z
    number ::= digit digit*
    digit ::= 0 | 1 | 2 | 3 | ... | 8 | 9
    relop ::= < (>|=|ε) | > (<|=|ε) | =
    string ::= " ( |!|#|$ ... -|.|/|digit|: ... @|A|B|C ... |X|Y|Z)* "
```

Dazu gibt es eine Python Implementierung:
GitHub - aleozlx/tinybasic: tiny BASIC parser, interpreter & compiler

Diese stellt die ideale Ausgangsbasis für einen kleinen Basic Compiler als Teil des ConfigTools
dar. Die Idee ist hier, dass auf der Hardware (da wenig Platz) nut die VM dazu läuft, die einen
ByteCode interpretiert und der Compiler nur auf em PC läuft. Denkbar ist aber auch, dass der
Compiler im Flash auf der Hardware liegt und von dort ausgeführt wird.

Virtual Machine
Die VM soll als einfacher switch/case implementiert werden, so dass nur ca. 1 KByte an
Code Speicher benötigt wird.

Folgende Tabelle zeigt die Instruktionen und den Byte Code dazu:

```text
k_END           // ii
k_PRINT_STR_N1        // ii (print string from stack)
k_PRINT_VAL_N1        // ii (print value from stack)
k_PRINT_NEWL_N1       // ii
k_PRINT_TAB_N1        // ii (print tab)
k_PUSH_STR_Nx        // ii nn S T R I N G 00 (push string address (16 bit))
k_PUSH_NUM_N5        // ii xx xx xx xx (push const value)
k_PUSH_NUM_N2       // ii xx (push const value)
k_PUSH_VAR_N2           // ii idx (push variable)
k_POP_VAR_N2           // ii idx (pop variable)
k_ADD_N1           // ii (add two values from stack)
k_SUB_N1           // ii (sub ftwo values rom stack)
k_MUL_N1           // ii (mul two values from stack)
k_DIV_N1           // ii (div two values from stack)
k_EQUAL_N1         // ii (compare two values from stack)
k_NOT_EQUAL_N1        // ii (compare two values from stack)
k_LESS_N1          // ii (compare two values from stack)
k_LESS_EQU_N1     // ii (compare two values from stack)
k_GREATER_N1       // ii (compare two values from stack)
k_GREATER_EQU_N1  // ii (compare two values from stack)
k_GOTO_N2          // ii xx xx (16 bit programm address) 
k_GOSUB_N2         // ii xx xx (16 bit programm address) 
k_RETURN_N1        // ii (pop return address)
k_IF_N2            // ii xx xx (pop val, END address)

ii...instruction
xx...8 bit value
xx xx xx xx...32 bit value
nn...8 bit string length
```

Damit ist auch schon der komplette Kern der VM beschrieben. Erweiterungen sind denkbar (call extern, …)

## Compiler

```basic
PRINT "Hello"
    01 H e l l o 0
PRINT A
    02 00 (variable A liegt auf Pasition 0)
IF expression relop expression THEN statement
    - expression
    - expression
    - relop
    20 xx xx
    - statement
GOTO number
    17 xx xx (Nummer muss in Adresse umgesetzt werden)
LET var = expression
    - expression
    06 idx
GOSUB number
    18 xx xx (Nummer muss in Adresse umgesetzt werden)
RETURN
    19
END
    00
```

## Mögliche Erweiterungen

Um TinyBasic als Scriptsprache auf dem Gerät sinnvoll nutzen zu können, sind folgende
Erweiterungen vorstellbar:

- Labels statt Zeilennummern (damit GOTO label) quasi als Funktionen, auch von außen erreichbar
- Externe Variablen, die beim “Aufruf” eines Labels “übergeben” werden
- String Variablen ($), die auch für CAN-Nachrichten genutzt werden (Byte Null ist die Größe)
- Externe Funktionen wie alloc, free, send_msg
- get/get Funktionen, um Strings ändern zu können (evtl. auch str[idx])

```text
init():
  LET A = 0
  LET B = 1000
  register_msg(1, 1, "STD", 100)
  RETURN
loop(ticks):
  PRINT "Hello world"
  RETURN
on_can(topic, port, msg$):
  LET tx_msg$ = ALLOC(88)
  SETLONG(tx_msg$, 4, 0x100)
  SETBYTE(tx_msg$, 8, 0x)
  SETBYTE(tx_msg$, 9,GETBYTE(msg$, 10))
  SEND_MSG(1, tx_msg$)
  FREE(msg$)
  RETURN
```

Damit wäre dann Lua weitgehend ersetzbar…

Vorstellbar wäre auch:

- peek/poke/sys
- FOR loop
- Returnwerte aus C-Funktionen
- ???
