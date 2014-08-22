#!/usr/bin/env python
# Copyright 2011-2014 Dietrich Epp <depp@zdome.net>
# See LICENSE.txt for details.
import re, os, sys, cStringIO

WARNING = "/* This file is automatically generated by keycode.py.  */"
SRC_DIR = '../../src/core'
INCLUDE_DIR = '../../include/sg'

def readTable(path):
    v = []
    for lineno, line in enumerate(open(path, 'r')):
        if line.startswith(';'):
            continue
        line = line.strip()
        if not line:
            continue
        l = line.split('\t')
        if len(l) == 2:
            i, n = l
            nn = None
        elif len(l) == 3:
            i, n, nn = l
        else:
            raise Exception("Wrong number of columns: %s:%i" % (path, lineno+1))
        if not i or not n or (nn is not None and not nn):
            raise Exception("Empty column: %s:%i" % (path, lineno+1))
        v.append((int(i), n, nn))
    return v

USED = set()
HID_NAMES = readTable('keycode.txt')

def ptable(f, table, name, ttype):
    print >>f, '%s %s[%d] = {' % (ttype, name, len(table))
    rows = [','.join(['%3d' % x for x in table[i:i+16]]) for i in range(0, len(table), 16)]
    print >>f, ',\n'.join(rows)
    print >>f, '};'

def pdata(f, data, name, ttype):
    print >>f, '%s %s[%d] =' % (ttype, name, len(data))
    io = cStringIO.StringIO()
    for i, c in enumerate(data[:-1]):
        n = ord(c)
        if n < 32 or n >= 127:
            if n == 0 and not data[i+1].isdigit():
                t = '\\0'
            else:
                t = '\\%03o' % n
        else:
            t = c
        if io.tell() + len(t) + 3 > 78:
            print >>f, '"%s"' % (io.getvalue(),)
            io = cStringIO.StringIO()
        io.write(t)
    print >>f, '"%s";' % (io.getvalue(),)

def gencodemap(table1, table2, out, size, name):
    out = os.path.join(SRC_DIR, out)
    t1 = readTable(table1)
    t2 = readTable(table2)
    tmp = out + '.tmp'
    names = dict([(n, i) for (i, n, nn) in t2])
    if len(names) != len(t2):
        raise Exception("%s: contains duplicate name" % (table2,))
    out1 = [255] * 256
    out2 = [255] * size
    for i, n, nn in t1:
        USED.add(i)
        if i < 0 or i > 255:
            raise Exception("HID Key code out of range: %d" % i)
        if n[0].isdigit():
            ii = int(n, 0)
        else:
            ii = names[n]
        if ii < 0 or ii >= size:
            raise Exception("Platform Key code out of range: %d" % ii)
        if out1[i] != 255 or out2[ii] != 255:
            print >>sys.stderr, '%d -> %d' % (i, ii)
            print >>sys.stderr, '%d -> %d' % (i, out1[i])
            print >>sys.stderr, '%d -> %d' % (out2[ii], ii)
            raise Exception("Collision")
        out1[i] = ii
        out2[ii] = i
    f = open(tmp, 'w')
    try:
        print >>f, WARNING
        ptable(f, out1, 'SG_%s_HID_TO_NATIVE' % name, 'const unsigned char')
        ptable(f, out2, 'SG_%s_NATIVE_TO_HID' % name,  'const unsigned char')
        f.close()
        os.rename(tmp, out)
    except:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise

IDENT = re.compile('^[A-Za-z][A-Za-z0-9_]*$')

def mkid(n):
    if n.startswith('KP '):
        pfx = 'KP_'
        sfx = n[3:]
    else:
        pfx = 'KEY_'
        sfx = n
    i = pfx + sfx.replace(' ', '')
    if not IDENT.match(i):
        raise Exception("Invalid name: %s" % repr(n))
    return i

def idents():
    # Call this AFTER calling all the others, so we know which codes
    # are actually used
    names = [x for x in HID_NAMES if x[0] in USED]
    names.sort()
    out = os.path.join(INCLUDE_DIR, 'keycode.h')
    tmp = out + '.tmp'
    f = open(tmp, 'w')
    try:
        print >>f, WARNING
        print >>f, "#ifndef SG_KEYCODE_H"
        print >>f, "#define SG_KEYCODE_H"
        print >>f, 'enum {'
        print >>f, ',\n'.join(['    %s = %d' % (mkid(nn or n), i) for (i, n, nn) in names])
        print >>f, '};'
        print >>f, "#endif"
        f.close()
        os.rename(tmp, out)
    except:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise

def mkid2(n):
    return n.replace(' ', '')

KEYID_HEADER = '''\
#include "sg/key.h"
#include <string.h>
'''

KEYID_CODE = '''\
int
sg_keyid_code_from_name(const char *name)
{
    unsigned l = 0, r = %(count)d, m;
    int x, c;
    while (l < r) {
        m = (l + r) / 2;
        x = KEYID_ORDER[m];
        c = strcmp(name, KEYID_NAME + KEYID_OFF[x]);
        if (c < 0)
            r = m;
        else if (c > 0)
            l = m + 1;
        else
            return x;
    }
    return -1;
}

const char *
sg_keyid_name_from_code(int code)
{
    int off;
    if (code < 0 || code > 255)
        return 0;
    off = KEYID_OFF[code];
    if (off == (unsigned short) -1)
        return 0;
    return KEYID_NAME + off;
}\
'''

def names():
    names = [(mkid2(nn or n), i) for (i, n, nn) in HID_NAMES if i in USED]
    names.sort()
    data = cStringIO.StringIO()
    offsets = [-1] * 256
    order = []
    for n, i in names:
        order.append(i)
        offsets[i] = data.tell()
        data.write(n)
        data.write('\0')
    data = data.getvalue()
    out = os.path.join(SRC_DIR, 'keyid.c')
    tmp = out + '.tmp'
    f = open(tmp, 'w')
    try:
        print >>f, WARNING
        print >>f, KEYID_HEADER
        pdata(f, data, 'KEYID_NAME', 'static const char')
        print >>f
        ptable(f, offsets, 'KEYID_OFF', 'static const unsigned short')
        print >>f
        ptable(f, order, 'KEYID_ORDER', 'static const unsigned char')
        print >>f
        print >>f, KEYID_CODE % { 'count': len(names) }
        f.close()
        os.rename(tmp, out)
    except:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise

gencodemap('mac.txt', 'mac2.txt', 'keytable_mac.c', 128, 'MAC')
gencodemap('win.txt', 'win2.txt', 'keytable_win.c', 256, 'WIN')
gencodemap('xkb.txt', 'evdev.txt', 'keytable_evdev.c', 256, 'EVDEV')
idents()
names()
