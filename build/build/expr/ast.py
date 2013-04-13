import re

class EvalError(ValueError):
    pass

def to_string(environ, value):
    if isinstance(value, str):
        return value
    raise EvalError('cannot convert type to string: {}'
                    .format(type(value).__name__))

def to_bool(value):
    if not isinstance(value, bool):
        raise EvalError('cannot convert type to boolean: {}'
                        .format(type(value).__name__))
    return value

def compare(x, y):
    if type(x) != type(y):
        raise EvalError('cannot compare types: {}, {}'
                        .format(type(x).__name__, type(y).__name__))
    if not isinstance(x, (bool, str)):
        raise EvalError('uncomparable type: {}'
                        .format(type(x).__name__))
    return x == y

class Group(object):
    __slots__ = ['val']

    def __init__(self, val):
        self.val = val

    def eval(self, environ):
        return self.val.eval(environ)

class OpExpr(object):
    __slots__ = ['args']

    def __init__(self, *args):
        self.args = tuple(args)

########################################
# Logical operators

class LogOp(OpExpr):
    __slots__ = []

    def eval(self, environ):
        v = self.is_and
        for arg in self.args:
            v = arg.eval(environ)
            if type(v) != bool:
                raise EvalError(
                    'expected bool operand for "{}" operator'
                    .format('&&' if self.is_and else '||'))
            if v != self.is_and:
                return v
        return v

class LogAnd(LogOp):
    __slots__ = []
    is_and = True

class LogOr(LogOp):
    __slots__ = []
    is_and = False

def cmp_flatten(arr, expr):
    if isinstance(expr, CmpOp):
        cmp_flatten(arr, expr.args[0])
        arr.append((expr.func, expr.oper))
        cmp_flatten(arr, expr.args[1])
    else:
        arr.append(expr)

########################################
# Comparison operators

class CmpOp(OpExpr):
    __slots__ = []

    def eval(self, environ):
        arr = []
        cmp_flatten(arr, self)
        n = len(arr) // 2
        lhs = arr[0].eval(environ)
        for i in range(n):
            func, oper = arr[i*2+1]
            rhs = arr[i*2+2].eval(environ)
            if type(lhs) != type(rhs):
                raise EvalError(
                    'type mismatch for "{}" operator' .format(oper))
            if not func(lhs, rhs):
                return False
            lhs = rhs
        return True

def cmpop(oper):
    def wrap(func):
        clsdict = {
            'func': staticmethod(func),
            'oper': oper,
            '__slots__': [],
        }
        return type(func.__name__, (CmpOp,), clsdict)
    return wrap

@cmpop('==')
def Equals(x, y):
    return x == y

@cmpop('!=')
def NotEquals(x, y):
    return x != y

########################################
# Simple binary expressions

class BinOp(OpExpr):
    __slots__ = []

    def eval(self, environ):
        return self.func(
            environ,
            self.args[0].eval(environ),
            self.args[1].eval(environ))

def binop(func):
    clsdict = {
        'func': staticmethod(func),
        '__slots__': [],
    }
    return type(func.__name__, (BinOp,), clsdict)

@binop
def Field(environ, x, y):
    k = to_string(environ, y)
    try:
        return x[k]
    except TypeError:
        raise EvalError('{} object has no fields'
                        .format(type(x).__name__))
    except KeyError:
        raise EvalError(
            '{} object does not have {!r} field'
            .format(type(x).__name__, str(k)))

########################################
# Functions

class Function(object):
    __slots__ = ['func', 'args']

    def __init__(self, func, args):
        self.func = func
        self.args = tuple(args)

    def eval(self, environ):
        args = tuple(arg.eval(environ) for arg in self.args)
        try:
            return self.func(environ, *args)
        except EvalError as ex:
            raise EvalError(
                '{}: {}'.format(ex.desc, self.func.__name__),
                ex.details)

def string_func(func):
    def new_func(environ, *arg):
        if len(arg) != 1:
            raise ValueError('one argument expected')
        return func(environ, to_string(environ, arg[0]))
    return new_func

UNDERSCORE = re.compile('[^A-Za-z0-9]+')

class Functions(object):
    def concat(environ, *args):
        return ''.join(to_string(environ, arg) for arg in args)

    @string_func
    def underscore(environ, arg):
        return UNDERSCORE.sub('_', arg).strip('_')

########################################
# Miscellaneous

class VarExpr(object):
    __slots__ = ['var']

    def __init__(self, var):
        self.var = var

    def eval(self, environ):
        try:
            return environ[self.var]
        except KeyError:
            raise EvalError('undefined variable: ${}'.format(self.var))

class StringExpr(object):
    __slots__ = ['value']

    def __init__(self, value):
        self.value = value

    def eval(self, environ):
        return self.value
