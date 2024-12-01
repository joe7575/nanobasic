#
# Tiny BASIC interpreter and compiler
# pip install peglet
# Debug: for item in self.programm[22:]: print(hex(item), end=" ")
#

import os
import io
import re
import sys
import argparse
import peglet

k_TAG           = 0xBC
k_VERSION       = 0x01

k_END           = 0
k_PRINTS        = 1
k_PRINTV        = 2
k_PRINTNL       = 3
k_NUMBER        = 4
k_BYTENUM       = 5
k_VAR           = 6
k_LET           = 7
k_ADD           = 8
k_SUB           = 9
k_MUL           = 10
k_DIV           = 11
k_EQU         = 12
k_NEQU        = 13
k_LE          = 14
k_LEEQU     = 15
k_GR       = 16
k_GREQU  = 17
k_GOTO          = 18
k_GOSUB         = 19
k_RETURN        = 20
k_IF            = 21

class Parser(object):

    grammar = r"""
    lines       = _ line _ lines
                | _ line
    line        = label (\:) _ stmt                 hug
                | stmt                              hug
                | label (\:)                        hug
    stmt        = print_stmt
                | let_stmt
                | if_stmt
                | goto_stmt
                | gosub_stmt
                | end_stmt
                | rem_stmt

    print_stmt  = (PRINT\b) _ expr_list
    let_stmt    = (LET\b) _ var _ (?:=) _ expr
                | (LET\b) _ var _ (?:=) _ str
    if_stmt     = (IF\b) _ expr _ relop _ expr _ (THEN\b) _ stmt
    goto_stmt   = (GOTO\b) _ label
    gosub_stmt  = (GOSUB\b) _ label
    end_stmt    = (END\b)
    rem_stmt    = (REM\b) _ str

    expr_list   = expr _ (,) _ expr_list 
                | expr 
                | str _ (,) _ expr_list
                | str

    expr        = term _ (\+|\-) _ expr
                | term

    term        = factor _ (\*|\/) _ term
                | factor

    factor      = var
                | num
                | l_paren _ expr _ r_paren          join

    var_list    = var _ , _ var_list
                | var
    var         = ([A-Za-z_][A-Za-z0-9_]*)
    label       = ([A-Za-z_][A-Za-z0-9_]*)

    str         = " chars " _                       join quote
                | ' sqchars ' _                     join
    chars       = char chars 
                |
    char        = ([^\x00-\x1f"\\]) 
                | esc_char
    sqchars     = sqchar sqchars 
                |
    sqchar      = ([^\x00-\x1f'\\]) 
                | esc_char
    esc_char    = \\(['"/\\])
                | \\([bfnrt])                       escape
    num         = (\-) num
                | (\d+)
    relop       = (<>|<=|<|>=|>|=)
    binop       = (\+|\-|\*|\/)
    l_paren     = (\()
    r_paren     = (\)) 
    _           = \s*
    """

    def __init__(self):
        kwargs = {"hug"     : peglet.hug,
                  "join"    : peglet.join,
                  "quote"   : self.quote,
                  "escape"  : re.escape}
        self.parser = peglet.Parser(self.grammar, **kwargs)
    
    def __call__(self, program):
        return self.parser(program)

    def quote(self, token):
        return '"%s"' % token

class Compiler(object):
    
    def __init__(self):
        self.parser = Parser()
        self.parse_tree = None
        self.symbols = []
        self.labels = {}
        self.programm = None

    def __call__(self, program):
        self.parse_tree = self.parser(program)
        self.programm = bytearray([k_TAG, k_VERSION, k_END])
        for line in self.parse_tree:
            if "LET" in line:
                id = line[1]
                if id not in self.symbols:
                    self.symbols.append(id)
        for line in self.parse_tree:
            self.compile_line(line)
        self.programm.append(k_END)
        tbl = []
        for c in self.programm:
            tbl.append("%02X" % c)
            print("%02X" % c, end="")
        return "".join(tbl)
    
    def compile_line(self, line):
        if len(line) > 1 and line[1] == ":":
            self.compile_label(line[0])
            line = line[2:]
        if line:
            self.compile_stmt(line)

    def compile_stmt(self, stmt):
        head, tail = stmt[0], stmt[1:]
        if tail:
            if head == "IF":
                self.compile_if(tail)
            elif head == "LET":
                self.compile_var(tail)
            elif head == "REM":
                self.compile_comment(tail)
            elif head == "GOTO":
                self.compile_goto(tail)
            elif head == "PRINT":
                self.compile_printf(tail)
            else:
                self.compile_stmt(tail)
        else:
            if head == "END":
                self.compile_return()
    
    def compile_if(self, tokens):
        tokens = self.compile_expression(tokens)
        relop = {"=": k_EQU, "<>": k_NEQU, "<": k_LE, "<=": k_LEEQU, ">": k_GR, ">=": k_GREQU}[tokens[0]]
        tokens = self.compile_expression(tokens[1:])
        self.programm.append(relop)
        self.programm.append(k_IF)
        idx = len(self.programm)
        self.programm.extend([0, 0])
        if tokens[0] == "THEN":
            tokens = tokens[1:]
        self.compile_stmt(tokens)
        addr = len(self.programm)
        self.programm[idx] = addr & 0xFF
        self.programm[idx + 1] = (addr >> 8) & 0xFF
        pass

    def compile_goto(self, tokens):
        addr = self.labels[tokens[0]]
        self.programm.append(k_GOTO)
        self.programm.extend(addr.to_bytes(2, byteorder='little'))

    def compile_var(self, tokens):
        var, tail =  tokens[0], tokens[1:]
        if self.is_quoted(var):
            pass
        tail = self.compile_expression(tail)
        self.programm.append(k_LET)
        self.programm.append(self.symbols.index(var))
        return tail

    def compile_comment(self, xs):
        pass

    def compile_label(self, n):
        self.labels[n] = len(self.programm)
    
    def compile_printf(self, tokens):
        while tokens:
            if tokens[0][0] == '"':
                self.programm.append(k_PRINTS)
                tokens = self.compile_string(tokens)
            else:
                tokens = self.compile_expression(tokens)
                self.programm.append(k_PRINTV)
            if tokens and tokens[0] == ",":
                tokens = tokens[1:]
        self.programm.append(k_PRINTNL)

    def compile_expression(self, tokens):
        tail = self.compile_term(tokens)
        if len(tail) > 0:
            if tail[0] == "+":
                tail = self.compile_expression(tail[1:])
                self.programm.append(k_ADD)
                return tail
            elif tail[0] == "-":
                tail = self.compile_expression(tail[1:])
                self.programm.append(k_SUB)
                return tail
        return tail
            
    def compile_string(self, tokens):
        s = tokens[0]
        self.programm.extend(s[1:-1].encode("ascii"))
        self.programm.append(0)
        return tokens[1:]
    
    def compile_term(self, tokens):
        tail = self.compile_factor(tokens)
        if len(tail) > 0:
            if tail[0] == "*":
                tail = self.compile_term(tail[1:])
                self.programm.append(k_MUL)
                return tail
            elif tail[0] == "/":
                tail = self.compile_term(tail[1:])
                self.programm.append(k_DIV)
                return tail
        return tail
    
    def compile_factor(self, tokens):
        if tokens[0] == "(":
            return self.compile_expression(tokens[1:])
        elif tokens[0] in self.symbols:
            self.programm.append(k_VAR)
            self.programm.append(self.symbols.index(tokens[0]))
            return tokens[1:]
        else:
            num = int(tokens[0])
            if num < 256:
                self.programm.append(k_BYTENUM)
                self.programm.append(num)
            else:
                self.programm.append(k_NUMBER)
                self.programm.extend(num.to_bytes(4, byteorder='little'))
            return tokens[1:]
        
    def compile_return(self):
        self.programm.append(k_END)

    def is_quoted(self, s):
        return re.match('^".*"$', s)

class TinyBasic(object):
    
    def __init__(self):
        self.parser = Parser()
        self.compiler = Compiler()
    
    def parse(self, program):
        return self.parser(program)
    
    def compile(self, program):
        return self.compiler(program)

if __name__ == "__main__":
    sys.argv = ["tinybasic.py", "./test.bas"]
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("path", nargs='?')
    arg_parser.add_argument("-p", "--parse", action="store_true")
    args = arg_parser.parse_args()
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    tiny_basic = TinyBasic()

    if args.path:
        if os.path.isfile(args.path):
            with io.open(args.path, "r", encoding="ascii") as f:
                program = "".join(f.readlines())
                if args.parse:
                    for line in tiny_basic.parse(program):
                        print(line)
                else:
                    s = tiny_basic.compile(program)
                    os.system(".\\Debug\\demo.exe %s" % s)
