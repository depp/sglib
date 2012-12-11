from __future__ import absolute_import

from gen.source import Source
from gen.path import Path
from gen.project import OS, Project, Module, BundledLibrary, Executable
from gen.util import *
from xml.dom import Node
import xml.dom.minidom

def unexpected(node, c):
    """Raise an exception for an unexpected child of the given element."""
    if c.nodeType == Node.ELEMENT_NODE:
        v = '%s element' % c.tagName
    elif c.nodeType == Node.TEXT_NODE:
        v = 'text'
    elif c.nodeType == Node.CDATA_SECTION_NODE:
        v = 'CDATA section'
    elif c.nodeType == Node.ENTITY_NODE:
        v = 'entity'
    elif c.nodeType == Node.PROCESSING_INSTRUCTION_NODE:
        v = 'processing instruction'
    elif c.nodeType == Node.COMMENT_NODE:
        v = 'comment'
    else:
        v = 'node'
    raise ValueError('unexpected %s in %s element' % (v, node.tagName))

def unexpected_attr(node, attr):
    raise ValueError('unexpected %s attribute on %s element' %
                     (attr.name, node.tagName))

def missing_attr(node, attr):
    raise ValueError('missing %s attribute on %s element' %
                     (attr, node.tagName))

def node_elements(node):
    """Iterate over the children of a node that are element nodes.

    Raise an exception if the node contains any other content.
    """
    for c in node.childNodes:
        if c.nodeType == Node.ELEMENT_NODE:
            yield c
        elif c.nodeType == Node.TEXT_NODE:
            if not c.data.isspace():
                unexpected(node, c)
        elif c.nodeType == Node.COMMENT_NODE:
            pass
        else:
            unexpected(node, c)

def node_empty(node):
    """Raise an exception if the node is not empty."""
    for c in node.childNodes:
        if c.nodeType == Node.TEXT_NODE:
            if not c.data.isspace():
                unexpected(node, c)
        elif c.nodeType == Node.COMMENT_NODE:
            pass
        else:
            unexpected(node, c)

def node_text(node):
    """Return the text contained in a node.

    Raise an exception if the node contains any other content.
    """
    a = []
    for c in node.childNodes:
        if (c.nodeType == Node.TEXT_NODE or
            c.nodeType == Node.CDATA_SECTION_NODE):
            a.append(c.data)
        elif c.nodeType == Node.COMMENT_NODE:
            pass
        else:
            unexpected(node, c)
    return u''.join(a)

def node_noattr(node):
    """Raise an exception if an element has any attributes."""
    if node.attributes.length:
        unexpected_attr(node, node.attributes.item(0))

####################

def cvar_path(node, path):
    npath = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            npath = Path(attr.value)
        else:
            unexpected_attr(node, attr)
    if npath is None:
        missing_attr(node, 'path')
    return Path(path, npath)

def parse_cvar(node, path):
    name = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'name':
            name = attr.value
            if not is_cvar(name):
                raise ValueError('invalid cvar name: %s' % name)
        else:
            unexpected_attr(node, attr)
    if name is None:
        missing_attr(node, 'name')
    v = []
    for c in node.childNodes:
        if (c.nodeType == Node.TEXT_NODE or
            c.nodeType == Node.CDATA_SECTION_NODE):
            v.append(c.data)
        elif c.nodeType == Node.ELEMENT_NODE:
            if c.tagName == 'path':
                v.append(cvar_path(c, path))
            else:
                unexpected(node, c)
        elif c.nodeType == Node.COMMENT_NODE:
            pass
        else:
            unexpected(node, c)
    return name, v

def parse_tags(attr):
    tags = []
    for tag in attr.value.split():
        if not is_ident(tag):
            raise ValueError('invalid tag: %s' % (tag,))
        tags.append(tag)
    return tuple(tags)

####################

def mod_group(mod, node, path, tags):
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            val = Path(attr.value)
            path = Path(path, val)
        elif attr.name == 'tag':
            ntags = parse_tags(attr)
            tags = tags + ntags
        else:
            unexpected_attr(node, attr)
    for c in node_elements(node):
        try:
            func = GROUP_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(mod, c, path, tags)

def mod_src(mod, node, path, tags):
    spath = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            spath = Path(attr.value)
        elif attr.name == 'tag':
            ntags = parse_tags(attr)
            tags = tags + ntags
        else:
            unexpected_attr(node, attr)
    if spath is None:
        missing_attr(node, path)
    node_empty(node)
    mod.add_source(Source(Path(path, spath), tags))

GROUP_ELEM = {
    'group': mod_group,
    'src': mod_src,
}

def mod_name(mod, node, path, tags):
    node_noattr(node)
    name = node_text(node)
    if mod.name is not None:
        raise ValueError('duplicate name tag')
    mod.name = name

def mod_header_path(mod, node, path, tags):
    ipath = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            ipath = Path(attr.value)
        else:
            unexpected_attr(node, attr)
    ipath = Path(path, ipath)
    node_empty(node)
    mod.header_path.append(ipath)

def mod_define(mod, node, path, tags):
    name = None
    value = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'name':
            name = attr.value
            if not is_ident(name):
                raise ValueError('invalid definition name: %s' % name)
        elif attr.name == 'value':
            value = attr.value
        else:
            unexpected_attr(node, attr)
    node_empty(node)
    mod.define.append((name, value))

def mod_require(mod, node, path, tags):
    name = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'name':
            name = attr.value
            if not is_ident(name):
                raise ValueError('invalid module requirement: %s' % name)
        else:
            unexpected_attr(node, attr)
    node_empty(node)
    mod.require.append(name)

def mod_cvar(mod, node, path, tags):
    name, value = parse_cvar(node, path)
    mod.cvar.append((name, value))

MOD_ELEM = {
    'name': mod_name,
    'header-path': mod_header_path,
    'define': mod_define,
    'require': mod_require,
    'cvar': mod_cvar,
}
MOD_ELEM.update(GROUP_ELEM)

EXENAME = {
    None: (is_title, None),
    'linux': (is_filename, make_filename),
    'osx': (is_title, None),
    'windows': (is_title, None),
}

def exe_name(mod, node, path, tags):
    osval = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'os':
            if attr.value not in OS:
                raise ValueError('unknown os: %s' % attr.value)
            osval = attr.value
        else:
            unexpected_attr(node, attr)
    if osval in mod.exe_name:
        raise ValueError('duplicate exe-name with same "os" attribute')
    val = node_text(node)
    func = EXENAME[osval][0]
    if not func(val):
        raise ValueError('invalid exe-name')
    mod.exe_name[osval] = val

def exe_mac_icon(mod, node, path, tags):
    osval = None
    spath = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'os':
            if attr.value not in OS:
                raise ValueError('unknown os: %s' % attr.value)
            osval = attr.value
        elif attr.name == 'path':
            spath = Path(attr.value)
            spath = Path(path, spath)
        else:
            unexpected_attr(node, attr)
    if spath is None:
        raise ValueError('missing path attribute on exe-icon')
    if osval in mod.exe_icon:
        raise ValueError('duplicate exe-icon')
    mod.exe_icon[osval] = spath

def exe_category(mod, node, path, tags):
    node_noattr(node)
    val = node_text(node)
    if not is_domain(val):
        raise ValueError('invalid apple-category')
    mod.apple_category = val

EXE_ELEM = {
    'exe-name': exe_name,
    'exe-icon': exe_mac_icon,
    'apple-category': exe_category,
}
EXE_ELEM.update(MOD_ELEM)

def parse_module(node, path):
    name = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            val = Path(attr.value)
            path = Path(path, val)
        elif attr.name == 'id':
            name = attr.value
            if not is_ident(name):
                raise ValueError('invalid id: %s' % name)
        else:
            unexpected_attr(node, attr)
    if name is None:
        missing_attr(node, 'id')
    mod = Module(name)
    for c in node_elements(node):
        try:
            func = MOD_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(mod, c, path, ())
    return mod

def parse_executable(node, path):
    name = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            val = Path(attr.value)
            path = Path(path, val)
        elif attr.name == 'id':
            name = attr.value
            if not is_ident(name):
                raise ValueError('invalid id: %s' % name)
        else:
            unexpected_attr(node, attr)
    mod = Executable(name)
    for c in node_elements(node):
        try:
            func = EXE_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(mod, c, path, ())
    for osval in OS:
        if osval in mod.exe_name:
            continue
        if None in mod.exe_name:
            base = mod.exe_name[None]
        else:
            base = mod.name
        if base is None:
            raise ValueError(
                'no exe-name for os %s and no default' % osval)
        check_func, make_func = EXENAME[osval]
        val = base if make_func is None else make_func(val)
        if not check_func(val):
            raise ValueError(
                'bad default exe-name for %s' % osval)
        mod.exe_name[osval] = val
    return mod

def parse_bundled_library(node, path):
    name = None
    libname = None
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            val = Path(attr.value)
            path = Path(path, val)
        elif attr.name == 'id':
            name = attr.value
            if not is_ident(name):
                raise ValueError('invalid id: %s' % name)
        elif attr.name == 'libname':
            libname = attr.value
        else:
            unexpected_attr(node, attr)
    if name is None:
        missing_attr(node, 'id')
    if libname is None:
        missing_attr(node, 'libname')
    mod = BundledLibrary(name, libname)
    for c in node_elements(node):
        try:
            func = MOD_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(mod, c, path, ())
    return mod

####################

def proj_prop(propname, check=None):
    def parse(proj, node, path):
        oldval = getattr(proj, propname)
        if oldval is not None:
            raise ValueError('project has duplicate %s element' %
                             node.tagName)
        node_noattr(node)
        val = node_text(node)
        if check is not None and not check(val):
            raise ValueError('invalid %s vaue' % node.tagName)
        setattr(proj, propname, val)
    return parse

def proj_cvar(proj, node, path):
    name, value = parse_cvar(node, path)
    proj.cvar.append((name, value))

MODULE_TAG = {
    'executable': parse_executable,
    'module': parse_module,
    'bundled-library': parse_bundled_library,
}

PROJ_ELEM = {
    'name': proj_prop('name', is_title),
    'ident': proj_prop('ident', is_domain),
    'filename': proj_prop('filename', is_filename),
    'url': proj_prop('url'),
    'email': proj_prop('email'),
    'copyright': proj_prop('copyright'),
    'cvar': proj_cvar,
}

def parse_project(node, path):
    proj = Project()
    for i in xrange(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            val = Path(attr.value)
            path = Path(path, val)
        else:
            unexpected_attr(node, attr)
    for c in node_elements(node):
        try:
            func = PROJ_ELEM[c.tagName]
        except KeyError:
            try:
                func = MODULE_TAG[c.tagName]
            except KeyError:
                unexpected(node, c)
            mod = func(c, path)
            proj.add_module(mod)
        else:
            func(proj, c, path)
    if proj.name is None:
        raise ValueError('project has no name element')
    if proj.ident is None:
        raise ValueError('project has no ident element')
    if proj.filename is None:
        try:
            proj.filename = make_filename(proj.name)
        except ValueError, ex:
            raise ValueError(
                'project has no filename element, cannot use default')
    return proj

########################################

def load(path):
    """Load the project or module XML file at the given path."""
    doc = xml.dom.minidom.parse(open(path, 'rb'))
    root = doc.documentElement
    if root.tagName == 'project':
        func = parse_project
    else:
        try:
            func = MODULE_TAG[root.tagName]
        except KeyError:
            raise ValueError('unexpected root element')
    return func(root, Path())
