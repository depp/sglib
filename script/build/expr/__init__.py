# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from .ast import to_string, to_bool, compare, EvalError
from .parse import parse, ParseError

def evaluate(environ, value):
    ast = parse(value)
    if len(ast) > 1:
        raise ValueError('too many expressions')
    if not ast:
        raise ValueError('expected expression')
    return ast[0].eval(environ)

def evaluate_many(environ, value):
    return [x.eval(environ) for x in parse(value)]
