#!/usr/bin/env python
import re, os, sys

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

def ptable(f, table, name):
    print >>f, 'const unsigned char %s[%d] = {' % (name, len(table))
    rows = [','.join(['%3d' % x for x in table[i:i+16]]) for i in range(0, len(table), 16)]
    print >>f, ',\n'.join(rows)
    print >>f, '};'

def macwin(table1, table2, out):
    t1 = readTable(table1)
    t2 = readTable(table2)
    tmp = out + '.tmp'
    names = dict([(n, i) for (i, n, nn) in t2])
    if len(names) != len(t2):
        raise Exception("%s: contains duplicate name" % (table2,))
    out1 = [255] * 256
    out2 = [255] * 256
    for i, n, nn in t1:
        USED.add(i)
        if i < 0 or i > 255:
            raise Exception("HID Key code out of range: %d" % i)
        if n[0].isdigit():
            ii = int(n, 0)
        else:
            ii = names[n]
        if ii < 0 or ii > 255:
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
        ptable(f, out1, 'KBD_HID_TO_NATIVE')
        ptable(f, out2, 'KBD_NATIVE_TO_HID')
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
    hid_names = [x for x in readTable('keycodes.txt') if x[0] in USED]
    hid_names.sort()
    out = 'keycode.h'
    tmp = out + '.tmp'
    f = open(tmp, 'w')
    try:
        print >>f, 'enum {'
        print >>f, ',\n'.join(['    %s = %d' % (mkid(nn or n), i) for (i, n, nn) in hid_names])
        print >>f, '};'
        f.close()
        os.rename(tmp, out)
    except:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise

macwin('mac.txt', 'mac2.txt', 'keytable_mac.c')
idents()
