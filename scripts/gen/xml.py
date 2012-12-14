from gen.source import Source
from gen.path import Path
from gen.project import *
from gen.util import *
from xml.dom import Node
import xml.dom.minidom

def unexpected(node, c):
    """Raise an exception for an unexpected child of the given element."""
    if c.nodeType == Node.ELEMENT_NODE:
        v = '{} element'.format(c.tagName)
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
    raise ValueError('unexpected {} in {} element'.format(v, node.tagName))

def unexpected_attr(node, attr):
    raise ValueError('unexpected {} attribute on {} element'
                     .format(attr.name, node.tagName))

def missing_attr(node, attr):
    raise ValueError('missing {} attribute on {} element'
                     .format(attr, node.tagName))

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
    return ''.join(a)

def node_noattr(node):
    """Raise an exception if an element has any attributes."""
    if node.attributes.length:
        unexpected_attr(node, node.attributes.item(0))

####################

def attr_bool(attr):
    if attr.value == 'true':
        return True
    elif attr.value == 'false':
        return False
    raise ValueError('invalid boolean: {!r}'.format(attr.value))

def attr_cvar(attr):
    if not is_cvar(attr.value):
        raise ValueError('invalid cvar name: {!r}'.format(attr.value))
    return attr.value

def attr_enable(attr):
    enable = []
    for tag in attr.value.split():
        if not is_flat_tag(tag):
            raise ValueError('invalid enable flag: {!r}'.format(tag))
        enable.append(tag)
    return tuple(enable)

def attr_flag(attr):
    if not is_flat_tag(attr.value):
        raise ValueError('invalid enable flag: {!r}'.format(attr.value))
    return attr.value

def attr_modid(attr):
    if not is_hier_tag(attr.value):
        raise ValueError('invalid module name: {!r}'.format(attr.value))
    return attr.value

def attr_module(attr):
    modules = []
    for tag in attr.value.split():
        if not is_hier_tag(tag):
            raise ValueError('invalid module name: {!r}'.format(tag))
        modules.append(tag)
    return tuple(modules)

def attr_os(attr):
    if attr.value not in OS:
        raise ValueError('unknown OS: {!r}'.format(attr.value))
    return attr.value

def attr_path(attr, path):
    try:
        v = Path(attr.value)
    except ValueError as ex:
        raise ValueError('invalid path: {!r}'.format(ex))
    return Path(path, v)

####################

def cvar_path(node, path):
    npath = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            npath = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    if npath is None:
        missing_attr(node, 'path')
    return npath

def parse_cvar(node, path):
    name = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'name':
            name = attr_cvar(attr)
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

def parse_defaults(node):
    os = None
    enable = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'os':
            os = attr_os(attr)
        elif attr.name == 'enable':
            enable = attr_enable(attr)
        else:
            unexpected_attr(node, attr)
    if enable is None:
        expected_attr(node, 'enable')
    node_empty(node)
    return Defaults(os, enable)

####################
# Source tags

def src_group(mod, node, path, enable, module):
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            path = attr_path(attr, path)
        elif attr.name == 'enable':
            enable = enable + attr_enable(attr)
        elif attr.name == 'module':
            module = module + attr_module(attr)
        else:
            unexpected_attr(node, attr)
    for c in node_elements(node):
        try:
            func = SRC_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(mod, c, path, enable, module)

def src_src(mod, node, path, enable, module):
    spath = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            spath = attr_path(attr, path)
        elif attr.name == 'enable':
            enable = enable + attr_enable(attr)
        elif attr.name == 'module':
            module = module + attr_module(attr)
        else:
            unexpected_attr(node, attr)
    if spath is None:
        missing_attr(node, 'path')
    node_empty(node)
    mod.add_source(Source(spath, enable, module))

SRC_ELEM = {
    'group': src_group,
    'src': src_src,
}

####################
# Module tags

def mod_name(mod, node, path=None):
    node_noattr(node)
    name = node_text(node)
    if mod.name is not None:
        raise ValueError('duplicate name tag')
    mod.name = name

def mod_header_path(mod, node, path):
    ipath = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            ipath = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    if not ipath:
        expected_attr(node, 'path')
    node_empty(node)
    mod.header_path.append(ipath)

def mod_define(mod, node, path):
    name = None
    value = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'name':
            name = attr.value
            if not is_ident(name):
                raise ValueError(
                    'invalid definition name: {}'.format(name))
        elif attr.name == 'value':
            value = attr.value
        else:
            unexpected_attr(node, attr)
    node_empty(node)
    mod.define.append((name, value))

def mod_cvar(mod, node, path):
    name, value = parse_cvar(node, path)
    mod.cvar.append((name, value))

def mod_defaults(mod, node, path):
    mod.defaults.append(parse_defaults(node))

####################
# Config tags

def config_require(mod, node):
    modules = ()
    enable = ()
    public = False
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'module':
            modules = attr_module(attr)
        elif attr.name == 'enable':
            enable = attr_enable(attr)
        elif attr.name == 'public':
            public = attr_bool(attr)
        else:
            unexpected_attr(node, attr)
    if modules is None:
        missing_attr(node, 'module')
    node_empty(node)
    mod.config.append(Require(modules, enable, public))

def feat_name(feat, node):
    node_noattr(node)
    v = node_text(node)
    if feat.name is not None:
        raise ValueError('feature has duplicate name')
    feat.name = v

def config_feature(mod, node):
    flagid = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'flag':
            flagid = attr_flag(attr)
        else:
            unexpected_attr(node, attr)
    if flagid is None:
        expected_attr(node, 'flag')
    feature = Feature(flagid)
    for c in node_elements(node):
        try:
            func = FEAT_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(feature, c)
    mod.config.append(feature)

def alt_name(obj, node):
    node_noattr(node)
    obj.name = node_text(node)

def config_alternatives(obj, node):
    node_noattr(node)
    alts = Alternatives()
    for c in node_elements(node):
        if c.tagName == 'alt':
            flagid = None
            for i in range(c.attributes.length):
                attr = c.attributes.item(i)
                if attr.name == 'flag':
                    flagid = attr_flag(attr)
                else:
                    unexpected_attr(c, attr)
            if flagid is None:
                expected_attr(c, 'flag')
            alt = Alternative(flagid)
            for cc in node_elements(c):
                try:
                    func = ALT_ELEMS[cc.tagName]
                except KeyError:
                    unexpected(c, cc)
                func(alt, cc)
            alts.alternatives.append(alt)
        else:
            unexpected(node, c)
    obj.config.append(alts)

def var_name(var, node):
    node_noattr(node)
    v = node_text(node)
    if var.name is not None:
        raise ValueError('variant has duplicate name')
    var.name = v

def var_exe_suffix(var, node):
    os = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'os':
            os = attr_os(attr)
        else:
            unexpected_attr(node, attr)
    v = node_text(node)
    if os in var.exe_suffix:
        raise ValueError('variant has conflicting exe-suffix')
    var.exe_suffix[os] = v

VAR_NAME = {
    'linux': lambda x: x.lower(),
    'osx': lambda x: x,
    'windows': lambda x: x,
}

def config_variant(obj, node):
    flagid = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'flag':
            flagid = attr_flag(attr)
        else:
            unexpected_attr(node, attr)
    if flagid is None:
        expected_attr(node, 'flag')
    var = Variant(flagid)
    for c in node_elements(node):
        try:
            func = VAR_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(var, c)
    for os, func in VAR_NAME.items():
        try:
            var.exe_suffix[os]
        except KeyError:
            try:
                base = var.exe_suffix[None]
            except KeyError:
                raise ValueError('missing variant exe-suffix')
            var.exe_suffix[os] = func(base)
    obj.config.append(var)

CONFIG_ELEM = {
    'require': config_require,
    'feature': config_feature,
    'alternatives': config_alternatives,
}

FEAT_ELEM = {
    'name': feat_name,
}
FEAT_ELEM.update(CONFIG_ELEM)

ALT_ELEMS = {
    'name': alt_name,
}
ALT_ELEMS.update(CONFIG_ELEM)

VAR_ELEM = {
    'name': var_name,
    'exe-suffix': var_exe_suffix,
}
VAR_ELEM.update(CONFIG_ELEM)

MCONFIG_ELEM = {
    'variant': config_variant,
}
MCONFIG_ELEM.update(CONFIG_ELEM)

####################
# Executable tags

EXENAME = {
    None: (is_title, None),
    'linux': (is_filename, make_filename),
    'osx': (is_title, None),
    'windows': (is_title, None),
}

def exe_name(mod, node, path):
    osval = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'os':
            osval = attr_os(attr)
        else:
            unexpected_attr(node, attr)
    if osval in mod.exe_name:
        raise ValueError('duplicate exe-name with same "os" attribute')
    val = node_text(node)
    func = EXENAME[osval][0]
    if not func(val):
        raise ValueError('invalid exe-name')
    mod.exe_name[osval] = val

def exe_icon(mod, node, path):
    osval = None
    spath = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'os':
            osval = attr_os(attr)
        elif attr.name == 'path':
            spath = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    if spath is None:
        raise ValueError('missing path attribute on exe-icon')
    if osval in mod.exe_icon:
        raise ValueError('duplicate exe-icon')
    mod.exe_icon[osval] = spath

def exe_category(mod, node, path):
    node_noattr(node)
    val = node_text(node)
    if not is_domain(val):
        raise ValueError('invalid apple-category')
    mod.apple_category = val

####################
# Executable and Module

def mod_config(func):
    def wrap(mod, node, path):
        func(mod, node)
    return wrap

def mod_src(func):
    def wrap(mod, node, path):
        func(mod, node, path, (), ())
    return wrap

MOD_ELEM = {
    'name': mod_name,
    'header-path': mod_header_path,
    'define': mod_define,
    'cvar': mod_cvar,
    'defaults': mod_defaults,
}
MOD_ELEM.update({k: mod_config(v) for k, v in MCONFIG_ELEM.items()})
MOD_ELEM.update({k: mod_src(v) for k, v in SRC_ELEM.items()})

EXE_ELEM = {
    'exe-name': exe_name,
    'exe-icon': exe_icon,
    'apple-category': exe_category,
}
EXE_ELEM.update(MOD_ELEM)

def parse_module(node, path):
    modid = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            path = attr_path(attr, path)
        elif attr.name == 'modid':
            modid = attr_modid(attr)
        else:
            unexpected_attr(node, attr)
    if modid is None:
        missing_attr(node, 'modid')
    mod = Module(modid)
    for c in node_elements(node):
        try:
            func = MOD_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(mod, c, path)
    return mod

def parse_executable(node, path):
    modid = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            path = attr_path(attr, path)
        elif attr.name == 'modid':
            modid = attr_modid(attr)
        else:
            unexpected_attr(node, attr)
    mod = Executable(modid)
    for c in node_elements(node):
        try:
            func = EXE_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(mod, c, path)
    for osval in OS:
        if osval in mod.exe_name:
            continue
        if None in mod.exe_name:
            base = mod.exe_name[None]
        else:
            base = mod.name
        if base is None:
            raise ValueError(
                'no exe-name for os {} and no default'.format(osval))
        check_func, make_func = EXENAME[osval]
        val = base if make_func is None else make_func(base)
        if not check_func(val):
            raise ValueError(
                'bad default exe-name for {}'.format(osval))
        mod.exe_name[osval] = val
    return mod

####################
# External library

def lib_framework(mod, node):
    node_noattr(node)
    name = node_text(node)
    if not is_ident(name):
        raise ValueError('invalid framework name: {}'.format(name))
    mod.add_libsource(Framework(name))

def lib_pkgconfig(mod, node):
    node_noattr(node)
    spec = node_text(node)
    mod.add_libsource(PkgConfig(spec))

def lib_sdlconfig(mod, node):
    node_noattr(node)
    version = node_text(node)
    mod.add_libsource(SdlConfig(version))

def lib_libsearch(mod, node):
    node_noattr(node)
    source = None
    flags = []
    for c in node_elements(node):
        if c.tagName == 'test-source':
            if source is not None:
                raise ValueError('duplicate test-source')
            node_noattr(c)
            prologue = None
            body = None
            for cc in node_elements(c):
                if cc.tagName == 'test-prologue':
                    if prologue is not None:
                        raise ValueError('duplicate prologue')
                    node_noattr(cc)
                    prologue = node_text(cc)
                elif cc.tagName == 'test-body':
                    if body is not None:
                        raise ValueError('duplicate body')
                    node_noattr(cc)
                    body = node_text(cc)
                else:
                    unexpected(c, cc)
            source = TestSource(prologue, body)
        elif c.tagName == 'flags':
            node_empty(c)
            d = {}
            for i in range(c.attributes.length):
                attr = c.attributes.item(i)
                if attr.name not in ('LIBS', 'CFLAGS', 'LDFLAGS'):
                    unexpected_attr(c, attrr)
                d[attr.name] = tuple(attr.value.split())
            flags.append(d)
    mod.add_libsource(LibrarySearch(source, flags))

def lib_group(mod, node):
    node_noattr(node)
    src = LibraryGroup()
    has_elem = False
    for c in node_elements(node):
        has_elem = True
        try:
            func = LIB_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(src, c)
    if not has_elem:
        raise ValueError('empty library-group')
    mod.add_libsource(src)

LIB_ELEM = {
    'name': mod_name,
    'framework': lib_framework,
    'pkg-config': lib_pkgconfig,
    'sdl-config': lib_sdlconfig,
    'library-search': lib_libsearch,
}

BUNDLE_ELEM = {
    'header-path': mod_header_path,
    'define': mod_define,
    'require': mod_config(config_require),
    'cvar': mod_cvar,
}
BUNDLE_ELEM.update({k: mod_src(v) for k, v in SRC_ELEM.items()})

def lib_bundled(mod, node):
    libname = None
    path = Path()
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            path = attr_path(attr, path)
        elif attr.name == 'libname':
            libname = attr.value
        else:
            unexpected_attr(node, attr)
    if libname is None:
        missing_attr(node, 'libname')
    src = BundledLibrary(libname)
    for c in node_elements(node):
        try:
            func = BUNDLE_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(src, c, path)
    mod.bundled_versions.append(src)

EXTLIB_ELEM = {
    'library-group': lib_group,
    'bundled-library': lib_bundled,
}
EXTLIB_ELEM.update(LIB_ELEM)

def parse_external_library(node, path):
    modid = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'modid':
            modid = attr_modid(attr)
        else:
            unexpected_attr(node, attr)
    if modid is None:
        missing_attr(node, 'modid')
    mod = ExternalLibrary(modid)
    for c in node_elements(node):
        try:
            func = EXTLIB_ELEM[c.tagName]
        except KeyError:
            unexpected(node, c)
        func(mod, c)
    return mod

def parse_bundled_library(node, path):
    name = None
    libname = None

####################
# Project

def proj_prop(propname, check=None):
    def parse(proj, node, path):
        oldval = getattr(proj, propname)
        if oldval is not None:
            raise ValueError('project has duplicate {} element'
                             .format(node.tagName))
        node_noattr(node)
        val = node_text(node)
        if check is not None and not check(val):
            raise ValueError('invalid {} vaue'.format(node.tagName))
        setattr(proj, propname, val)
    return parse

def proj_cvar(proj, node, path):
    name, value = parse_cvar(node, path)
    proj.cvar.append((name, value))

def proj_modpath(proj, node, path):
    mpath = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            mpath = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    if mpath is None:
        missing_attr(node, 'path')
    node_empty(node)
    proj.module_path.append(Path(path, mpath))

def proj_libpath(proj, node, path):
    mpath = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            mpath = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    if mpath is None:
        missing_attr(node, 'path')
    node_empty(node)
    proj.lib_path.append(mpath)

def proj_defaults(proj, node, path):
    proj.defaults.append(parse_defaults(node))

MODULE_TAG = {
    'executable': parse_executable,
    'module': parse_module,
    'external-library': parse_external_library,
}

PROJ_ELEM = {
    'name': proj_prop('name', is_title),
    'ident': proj_prop('ident', is_domain),
    'filename': proj_prop('filename', is_filename),
    'url': proj_prop('url'),
    'email': proj_prop('email'),
    'copyright': proj_prop('copyright'),
    'cvar': proj_cvar,
    'module-path': proj_modpath,
    'lib-path': proj_libpath,
    'defaults': proj_defaults,
}

def parse_project(node, path):
    proj = Project()
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            path = attr_path(attr, path)
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
        except ValueError as ex:
            raise ValueError(
                'project has no filename element, cannot use default')
    return proj

########################################
# Top level functions

def load(path, base):
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
    return func(root, base)
