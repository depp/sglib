# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import re
import collections

token = collections.namedtuple('token', 'name')

ROOT  = token('root object')
KEY   = token('dict key')
VALUE = token('dict value')
LIST  = token('list item')
END   = token('end of file')

UNQUOT = re.compile('^[A-Za-z0-9]+$')
ESCAPE = re.compile('[\x00-\x1f"\\\x7f]')

class Context(object):
    __slots__ = ['type', 'indent', 'compact', 'parent', 'name']
    def __init__(self, type, indent, compact, parent, name):
        self.type = type
        self.indent = indent
        self.compact = compact
        self.parent = parent
        self.name = name

class PlistError(Exception):
    pass

class Writer(object):
    __slots__ = ['_file', '_cxt', '_name']
    def __init__(self, f):
        self._file = f
        self._cxt = Context(ROOT, 0, False, None, 'root')
        self._name = None
        f.write('// !$*UTF8*$!\n')

    def _write_str(self, x):
        if not UNQUOT.match(x):
            x = '"' + ESCAPE.sub(lambda y: '\\' + y.group(0), x) + '"'
        self._file.write(x)

    def _fail(self, why):
        raise PlistError('%s (state: %s)' % (why, self._cxt.type._name))

    def write_key(self, key):
        if not isinstance(key, str):
            raise TypeError('Key must be a string')
        if self._cxt.type != KEY:
            self._fail('Cannot emit key here')
        self._name = key
        self._startobj()
        self._write_str(key)
        self._file.write(' =')
        self._cxt.type = VALUE

    def _check_value(self):
        if self._cxt.type not in (ROOT, VALUE, LIST):
            self._fail('Cannot emit object here')

    def newline(self, indent=True):
        self._file.write('\n')
        if indent:
            self._file.write('\t' * self._cxt.indent)

    def _startobj(self):
        c = self._cxt.type
        if c == VALUE or self._cxt.compact:
            self._file.write(' ')
        elif c != ROOT:
            self.newline()

    def _endobj(self):
        c = self._cxt.type
        if c == ROOT:
            self._file.write('\n')
            self._cxt.type = END
        elif c == VALUE:
            self._cxt.type = KEY
            self._file.write(';')
        elif c == LIST:
            self._file.write(',')
        else:
            self._fail('Invalid state')

    def _close(self, c):
        if self._cxt.compact:
            self._file.write(' ' + c)
            self._cxt = self._cxt.parent
        else:
            self._cxt = self._cxt.parent
            self.newline()
            self._file.write(c)
        self._endobj()

    def _push(self, type, compact):
        self._cxt = Context(
            type, self._cxt.indent + 1,
            self._cxt.compact or compact, self._cxt,
            self._name)
        self._name = None

    def start_dict(self, compact=False):
        self._check_value()
        self._startobj()
        self._push(KEY, compact)
        self._name = None
        self._file.write('{')

    def end_dict(self):
        if self._cxt.type != KEY:
            self._fail('Cannot close dict here')
        self._close('}')

    def start_list(self, compact=False):
        self._check_value()
        self._startobj()
        self._push(LIST, compact)
        self._name = 0
        self._file.write('(')

    def end_list(self):
        if self._cxt.type != LIST:
            self._fail('Cannot close list here')
        self._close(')')

    def write_object(self, obj):
        if isinstance(self._name, int):
            newname = self._name + 1
        else:
            newname = None
        if isinstance(obj, bool):
            obj = 'YES' if obj else 'NO'
        elif isinstance(obj, int):
            obj = str(obj)
        elif isinstance(obj, str):
            pass
        elif isinstance(obj, dict):
            self.start_dict()
            for k, v in sorted(obj.items()):
                self.write_pair(k, v)
            self.end_dict()
            self._name = newname
            return
        elif isinstance(obj, list):
            self.start_list()
            for v in obj:
                self.write_object(v)
            self.end_list()
            self._name = newname
            return
        else:
            raise TypeError('Not a str, bool, or int: {!r} (path={})'
                            .format(obj, self._context_name()))
        self._check_value()
        self._startobj()
        self._write_str(obj)
        self._endobj()
        self._name = newname

    def write_pair(self, key, value):
        self.write_key(key)
        self.write_object(value)

    def _context_name(self):
        names = [self._name]
        cxt = self._cxt
        while cxt is not None:
            names.append(str(cxt.name))
            cxt = cxt.parent
        names.reverse()
        return ':'.join(names)
