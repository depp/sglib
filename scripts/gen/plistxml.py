import xml.dom.minidom
import xml.dom as dom
# The methods you want to use are 'load' and 'dump'
# All keys and strings will be unicode
# this will FAIL with non Unicode strings
# this is intentional

class PlistError(ValueError):
    pass

def checkwhite(obj):
    if obj.nodeType == dom.Node.TEXT_NODE:
        if obj.data.strip():
            raise PlistError('expected whitespace')
    else:
        raise PlistError('expected whitespace')

def loadString(n):
    text = u''
    for child in n.childNodes:
        if child.nodeType == dom.Node.TEXT_NODE:
            text += child.data
        else:
            raise PlistError('invalid node in string')
    return text

def loadDict(n):
    d = {}
    key = None
    for child in n.childNodes:
        if child.nodeType == dom.Node.ELEMENT_NODE:
            if child.tagName == 'key':
                if key is not None:
                    raise PlistError('expcted value')
                key = loadString(child)
                if key in d:
                    raise PlistError('duplicate key %r' % key)
            else:
                if key is None:
                    raise PlistError('expected key')
                d[key] = loadValue(child)
                key = None
        else:
            checkwhite(child)
    if key is not None:
        raise PlistError('expected value')
    return d

def loadArray(n):
    a = []
    for child in n.childNodes:
        if child.nodeType == dom.Node.ELEMENT_NODE:
            a.append(loadValue(child))
        else:
            checkwhite(child)
    return a

def loadReal(n):
    x = loadString(n)
    try:
        return float(x)
    except ValueError:
        raise PlistError('invalid real')

def loadInteger(n):
    x = loadString(n)
    try:
        return int(x)
    except ValueError:
        raise PlistError('invalid integer')

def checknone(x):
    for c in x.childNodes:
        checkwhite(c)

def loadFalse(x):
    checknone(x)
    return False

def loadTrue(x):
    checknone(x)
    return True

LOADERS = {
    'dict': loadDict,
    'array': loadArray,
    'string': loadString,
    'real': loadReal,
    'integer': loadInteger,
    'true': loadTrue,
    'false': loadFalse,
}

def loadValue(n):
    k = n.tagName
    try:
        l = LOADERS[k]
    except KeyError:
        raise PlistError('invalid tag: %r' % (k,))
    return l(n)

def load(data):
    doc = xml.dom.minidom.parseString(data)
    root = doc.documentElement
    if root.nodeType != dom.Node.ELEMENT_NODE or root.tagName != 'plist':
        raise PlistError('not a plist')
    hasroot = False
    for child in root.childNodes:
        if child.nodeType == dom.Node.ELEMENT_NODE:
            if hasroot:
                raise PlistError('multiple roots')
            hasroot = True
            obj = loadValue(child)
        else:
            checkwhite(child)
    if not hasroot:
        raise PlistError('no root object')
    return obj

def dumpValue(doc, obj):
    if isinstance(obj, dict):
        n = doc.createElement('dict')
        for k, v in sorted(obj.iteritems()):
            c = doc.createElement('key')
            c.appendChild(doc.createTextNode(k))
            n.appendChild(c)
            c = dumpValue(doc, v)
            n.appendChild(c)
    elif isinstance(obj, unicode):
        n = doc.createElement('string')
        n.appendChild(doc.createTextNode(obj))
    elif isinstance(obj, bool):
        n = doc.createElement(['false', 'true'][obj])
    elif isinstance(obj, int):
        n = doc.createElement('integer')
        n.appendChild(doc.createTextNode(str(obj)))
    elif isinstance(obj, float):
        n = doc.createElement('real')
        n.appendChild(doc.createTextNode(str(obj)))
    elif isinstance(obj, list) or isinstance(obj, tuple):
        n = doc.createElement('array')
        for v in obj:
            n.appendChild(dumpValue(doc, v))
    else:
        raise TypeErorr('unknown type: %r' % obj)
    return n

def dump(obj):
    impl = xml.dom.minidom.getDOMImplementation()
    dt = impl.createDocumentType(
        'plist', "-//Apple//DTD PLIST 1.0//EN",
        "http://www.apple.com/DTDs/PropertyList-1.0.dtd")
    doc = impl.createDocument(None, 'plist', dt)
    doc.documentElement.appendChild(dumpValue(doc, obj))
    return doc.toxml('UTF-8')
