# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import collections
import re
from build.expr import ast

class ParseError(ValueError):
    pass

Token = collections.namedtuple('Token', 'typ value')
EOF = Token('EOF', None)

TEXT_RE = re.compile(r'[-\w]+')
SSTR_RE = re.compile(r"'((?:[^\\']|\\.)*)'")
DSTR_RE = re.compile(r'"((?:[^\\"]|\\.)*)"')

ESCAPE = {
    'a': '\a',
    'b': '\b',
    'f': '\f',
    'n': '\n',
    'r': '\r',
    'v': '\v',
    't': '\t',
    "'": "'",
    '"': '"',
    '\\': '\\',
}

def unescape(x):
    parts = []
    pos = 0
    while True:
        npos = x.find('\\', pos)
        if npos < 0:
            parts.append(x[pos:])
            break
        if pos != npos:
            parts.append(x[pos:npos])
        pos = npos
        c = x[pos + 1]
        try:
            c = ESCAPE[c]
        except KeyError:
            pass
        else:
            parts.append(c)
            pos += 2
            continue
        if c == 'x':
            hexlen = 2
        elif c == 'u':
            hexlen = 4
        elif c == 'U':
            hexlen = 8
        else:
            raise ParseError('invalid escape sequence: {!r}'.format(c))
        h = x[pos+2:pos+2+hexlen]
        if len(h) != hexlen:
            raise ParseError('invalid hex escape sequence')
        try:
            c = int(h, 16)
        except ParseError:
            raise ParseError('invalid hex escape sequence')
        if (c & ~0x7ff) == 0xd800 or c >= 0x110000:
            raise ParseError('hex escape sequence out of range: {:x}'
                             .format(c))
        c = chr(c)
        parts.append(c)
    return ''.join(parts)

Oper = collections.namedtuple('Oper', 'prec node')

OPER_PREC = {
    '||': Oper(1, ast.LogOr),
    '&&': Oper(2, ast.LogAnd),
    '!=': Oper(3, ast.NotEquals),
    '==': Oper(3, ast.Equals),
    # '.':  Oper(4, expr.Field),
}

OPER_SET = frozenset(OPER_PREC).union('( ) | .'.split())

def tokenize(s):
    while True:
        s = s.lstrip()
        if not s:
            break
        v = s[:2]
        if v in OPER_SET:
            yield Token(v, None)
            s = s[2:]
            continue
        v = s[:1]
        if v in OPER_SET:
            yield Token(v, None)
            s = s[1:]
            continue
        if s[0] == '$':
            typ = 'VAR'
            s = s[1:]
            if not s:
                raise ParseError('expected variable name after "$"')
        else:
            typ = 'TEXT'
        c = s[0]
        if c == '"':
            m = DSTR_RE.match(s)
            if not m:
                raise ParseError('invalid string')
            v = unescape(m.group(1))
        elif c == "'":
            m = SSTR_RE.match(s)
            if not m:
                raise ParseError('invalid string')
            v = unescape(m.group(1))
        else:
            m = TEXT_RE.match(s)
            if not m:
                raise ParseError('invalid character in expression: {!r}'
                                 .format(v[0]))
            v = m.group(0)
        yield Token(typ, v)
        s = s[m.end():]

class Tokens(object):
    __slots__ = ['_pos', '_tokens', 'tok']

    def __init__(self, s):
        self._pos = 0
        self._tokens = list(tokenize(s))
        self._tokens.append(EOF)
        self.tok = self._tokens[0]

    def next(self):
        self._pos += 1
        tok = self._tokens[self._pos]
        self.tok = tok
        return tok

def parse_prim(toks):
    base = None
    while True:
        tok = toks.tok
        typ = tok.typ
        if typ == 'TEXT':
            e = ast.StringExpr(tok.value)
            toks.next()
        elif typ == 'VAR':
            e = ast.VarExpr(tok.value)
            toks.next()
        elif typ == '(':
            toks.next()
            e = parse_func(toks)
            e = parse_op(toks, e, 0)
            tok = toks.tok
            if tok.typ != ')':
                raise ParseError('expected ")"')
            toks.next()
        else:
            if base is None:
                return None
            raise ParseError('expected primary expression')
        if base is not None:
            e = ast.Field(base, e)
        if toks.tok.typ != '.':
            return e
        base = e
        toks.next()

def parse_func(toks):
    parts = []
    while True:
        e = parse_prim(toks)
        if e is None:
            break
        parts.append(e)
    if not parts:
        raise ParseError('unexpected "{}"'.format(toks.tok.typ))
    if len(parts) == 1:
        return parts[0]
    func = parts[0]
    if not isinstance(func, ast.StringExpr):
        raise ParseError('function name must be a string')
    funcname = func.value
    m = TEXT_RE.match(funcname)
    if not m or m.end() != len(funcname):
        raise ParseError('invalid function name: {!r}'.format(funcname))
    try:
        func = getattr(ast.Functions, funcname)
    except AttributeError:
        raise ParseError('unknown function: {!r}'.format(funcname))
    return ast.Function(func, parts[1:])

def parse_op(toks, lhs, minprec):
    while True:
        oper = OPER_PREC.get(toks.tok.typ, None)
        if oper is None or oper.prec < minprec:
            return lhs
        toks.next()
        rhs = parse_prim(toks)
        while True:
            oper2 = OPER_PREC.get(toks.tok.typ, None)
            if oper2 is None or oper2.prec <= oper.prec:
                break
            rhs = parse_op(toks, rhs, oper2.prec)
        lhs = oper.node(lhs, rhs)

def parse(s):
    toks = Tokens(s)
    exprs = []
    while True:
        e = parse_func(toks)
        e = parse_op(toks, e, 0)
        exprs.append(e)
        if toks.tok.typ == 'EOF':
            break
        if toks.tok.typ != '|':
            raise ParseError('unexpected token: "{}"'.format(toks.tok.typ))
        toks.next()
    return exprs
