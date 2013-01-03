from gen.path import Path
import gen.project.config as config
import gen.project.source as source
import gen.project.module as module
import gen.project.project as project
import gen.os as os
import gen.util as util
from xml.dom import Node
import xml.dom.minidom

def unexpected(node, c, *, allowed=None):
    """Raise an exception for an unexpected child of the given element."""
    if c.nodeType == Node.ELEMENT_NODE:
        v = '<{}> element'.format(c.tagName)
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
    if allowed is None:
        raise ValueError('unexpected {} in <{}> element'
                         .format(v, node.tagName))
    else:
        raise ValueError(
            'unexpected {} in <{}> element, allowed elements are: {}'
            .format(v, node.tagName,
                    ' '.join('<{}>'.format(tagname)
                             for tagname in sorted(allowed))))

def unexpected_attr(node, attr):
    raise ValueError('unexpected "{}" attribute on <{}> element'
                     .format(attr.name, node.tagName))

def missing_attr(node, attr):
    raise ValueError('missing "{}" attribute on <{}> element'
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

def node_elements_apply(node, *args):
    for c in node_elements(node):
        for d, a in args:
            try:
                func = d[c.tagName]
                break
            except KeyError:
                pass
        else:
            unexpected(node, c, allowed=(k for d, a in args for k in d))
        func(c, *a)

####################

def attr_bool(attr):
    if attr.value == 'true':
        return True
    elif attr.value == 'false':
        return False
    raise ValueError('invalid boolean: {!r}'.format(attr.value))

def attr_cvar(attr):
    if not util.is_cvar(attr.value):
        raise ValueError('invalid cvar name: {!r}'.format(attr.value))
    return attr.value

def attr_enable(attr):
    enable = []
    for tag in attr.value.split():
        if not util.is_flat_tag(tag):
            raise ValueError('invalid enable flag: {!r}'.format(tag))
        enable.append(tag)
    return tuple(enable)

def attr_flag(attr):
    if not util.is_flat_tag(attr.value):
        raise ValueError('invalid enable flag: {!r}'.format(attr.value))
    return attr.value

def attr_modid(attr):
    if not util.is_hier_tag(attr.value):
        raise ValueError('invalid module name: {!r}'.format(attr.value))
    return attr.value

def attr_module(attr):
    modules = []
    for tag in attr.value.split():
        if not util.is_hier_tag(tag):
            raise ValueError('invalid module name: {!r}'.format(tag))
        modules.append(tag)
    return tuple(modules)

def attr_os(attr):
    if attr.value not in os.OS:
        raise ValueError('unknown OS: {!r}'.format(attr.value))
    return attr.value

def attr_path(attr, path):
    try:
        v = Path(attr.value)
    except ValueError as ex:
        raise ValueError('invalid path: {!r}'.format(ex))
    return Path(path, v)

####################

def parse_path(node, path):
    """Parse a 'path' tag."""
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

def generic_name(node, obj):
    """Parse a 'name' tag.  Add it to the object."""
    node_noattr(node)
    v = node_text(node)
    if obj.name is not None:
        raise ValueError('duplicate name')
    obj.name = v

NAME_ELEM = { 'name': generic_name }

def generic_filename(node, obj):
    """Parse a 'filename' tag.  Add it to the object."""
    os = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'os':
            os = attr_os(attr)
        else:
            unexpected_attr(node, attr)
    v = node_text(node)
    if os in obj.filename:
        raise ValueError('duplicate filename')
    obj.filename[os] = v

FILENAME_ELEM = { 'filename': generic_filename }

####################
# Config tags

def config_feature(node, obj, path):
    flagid = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'flag':
            flagid = attr_flag(attr)
        else:
            unexpected_attr(node, attr)
    if flagid is None:
        expected_attr(node, 'flag')
    feature = config.Feature(flagid)
    node_elements_apply(
        node,
        (CONFIG_ELEM, (feature, path)),
        (NAME_ELEM, (feature,)))
    obj.add_config(feature)

def config_alternatives(node, obj, path):
    node_noattr(node)
    alts = config.Alternatives()
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
            alt = config.Alternative(flagid)
            node_elements_apply(
                c,
                (CONFIG_ELEM, (alt, path)),
                (NAME_ELEM, (alt,)))
            alts.add_config(alt)
        else:
            unexpected(node, c)
    obj.add_config(alts)

def config_public(node, obj, path):
    node_noattr(node)
    public = config.Public()
    node_elements_apply(
        node,
        (CONFIG_ELEM, (public, path)))
    obj.add_config(public)

def config_variant(node, obj, path):
    flagid = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'flag':
            flagid = attr_flag(attr)
        else:
            unexpected_attr(node, attr)
    if flagid is None:
        expected_attr(node, 'flag')
    var = config.Variant(flagid)
    node_elements_apply(
        node,
        (CONFIG_ELEM, (var, path)),
        (NAME_ELEM, (var,)),
        (FILENAME_ELEM, (var,)))
    obj.add_config(var)

def config_require(node, obj, path):
    modules = ()
    enable = ()
    is_global = False
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'module':
            modules = attr_module(attr)
        elif attr.name == 'enable':
            enable = attr_enable(attr)
        elif attr.name == 'global':
            is_global = attr_bool(attr)
        else:
            unexpected_attr(node, attr)
    if modules is None:
        missing_attr(node, 'module')
    node_empty(node)
    obj.add_config(config.Require(modules, enable, is_global))

def config_defaults(node, obj, path):
    """Parse a 'defaults' config object."""
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
    obj.add_config(config.Defaults(os, enable))

def config_cvar(node, obj, path):
    """Parse a 'cvar' config object."""
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
                v.append(parse_path(c, path))
            else:
                unexpected(node, c)
        elif c.nodeType == Node.COMMENT_NODE:
            pass
        else:
            unexpected(node, c)
    obj.add_config(config.CVar(name, v))

def config_header_path(node, obj, path):
    """Parse a 'header-path' config object."""
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
    obj.add_config(config.HeaderPath(ipath))

def config_define(node, obj, path):
    name = None
    value = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'name':
            name = attr.value
            if not util.is_ident(name):
                raise ValueError(
                    'invalid definition name: {}'.format(name))
        elif attr.name == 'value':
            value = attr.value
        else:
            unexpected_attr(node, attr)
    node_empty(node)
    obj.add_config(config.Define(name, value))

def config_pkgconfig(node, obj, path):
    node_noattr(node)
    spec = node_text(node)
    obj.add_config(config.PkgConfig(spec))

def config_framework(node, obj, path):
    node_noattr(node)
    name = node_text(node)
    if not util.is_ident(name):
        raise ValueError('invalid framework name: {}'.format(name))
    obj.add_config(config.Framework(name))

def config_sdlconfig(node, obj, path):
    node_noattr(node)
    version = node_text(node)
    obj.add_config(config.SdlConfig(version))

def config_library_search(node, obj, path):
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
            source = config.TestSource(prologue, body)
        elif c.tagName == 'flags':
            node_empty(c)
            d = {}
            for i in range(c.attributes.length):
                attr = c.attributes.item(i)
                if attr.name not in ('LIBS', 'CFLAGS', 'LDFLAGS'):
                    unexpected_attr(c, attrr)
                d[attr.name] = tuple(attr.value.split())
            flags.append(d)
    obj.add_config(config.Search(source, flags))

def config_group(node, obj, path):
    node_noattr(node)
    cfg = config.ConfigSet()
    node_elements_apply(
        node,
        (CONFIG_ELEM, (cfg, path)))
    obj.add_config(cfg)

CONFIG_ELEM = {
    'feature': config_feature,
    'alternatives': config_alternatives,
    'public': config_public,
    'variant': config_variant,
    'require': config_require,
    'defaults': config_defaults,
    'cvar': config_cvar,
    'header-path': config_header_path,
    'define': config_define,

    'pkg-config': config_pkgconfig,
    'framework': config_framework,
    'sdl-config': config_sdlconfig,
    'library-search': config_library_search,
}

####################
# Source tags

def src_group(node, obj, path, enable, module):
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
    node_elements_apply(
        node,
        (SRC_ELEM, (obj, path, enable, module)))

def src_src(node, obj, path, enable, module):
    spath = None
    generator = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            spath = attr_path(attr, path)
        elif attr.name == 'enable':
            enable = enable + attr_enable(attr)
        elif attr.name == 'module':
            module = module + attr_module(attr)
        elif attr.name == 'generator':
            generator = attr.value
        else:
            unexpected_attr(node, attr)
    if spath is None:
        missing_attr(node, 'path')
    node_empty(node)
    obj.add_source(source.Source(spath, enable, module, generator))

SRC_ELEM = {
    'group': src_group,
    'src': src_src,
}

####################
# Executable tags

def exe_icon(node, obj, path):
    spath = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            spath = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    if spath is None:
        raise ValueError('missing path attribute on exe-icon')
    obj.exe_icon.append(spath)

def exe_apple_category(node, obj, path):
    node_noattr(node)
    val = node_text(node)
    if not util.is_domain(val):
        raise ValueError('invalid apple-category')
    obj.apple_category = val

EXE_ELEM = {
    'exe-icon': exe_icon,
    'apple-category': exe_apple_category,
}

####################
# Executable and Module

def parse_module(node, path):
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            path = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    mod = module.Module()
    node_elements_apply(
        node,
        (CONFIG_ELEM, (mod, path)),
        (SRC_ELEM, (mod, path, (), ())),
        (NAME_ELEM, (mod,)))
    return mod

def parse_executable(node, path):
    modid = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            path = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    exe = module.Executable()
    node_elements_apply(
        node,
        (CONFIG_ELEM, (exe, path)),
        (SRC_ELEM, (exe, path, (), ())),
        (EXE_ELEM, (exe, path)),
        (NAME_ELEM, (exe,)),
        (FILENAME_ELEM, (exe,)))
    return exe

def parse_libname(node, obj):
    pattern = None
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'pattern':
            pattern = attr.value
        else:
            unexpected_attr(node, attr)
    if pattern is None:
        missing_attr(node, 'pattern')
    obj.libnames.append(pattern)

BUNDLED_LIBRARY_ELEM = {
    'libname': parse_libname,
}

def parse_bundled_library(node, obj, path):
    libname = None
    path = Path()
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            path = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    bundled_library = module.BundledLibrary()
    node_elements_apply(
        node,
        (CONFIG_ELEM, (bundled_library, path)),
        (SRC_ELEM, (bundled_library, path, (), ())),
        (CONFIG_ELEM, (bundled_library, path)),
        (BUNDLED_LIBRARY_ELEM, (bundled_library,)))
    obj.bundles.append(bundled_library)

EXTERNAL_LIBRARY_ELEM = {
    'bundled-library': parse_bundled_library,
}

def parse_external_library(node, path):
    node_noattr(node)
    lib = module.ExternalLibrary()
    node_elements_apply(
        node,
        (CONFIG_ELEM, (lib, path)),
        (NAME_ELEM, (lib,)),
        (EXTERNAL_LIBRARY_ELEM, (lib, path)))
    return lib

####################
# Project

def proj_simple_prop(propname, check=None):
    def parse(node, proj, path):
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

def proj_pathlist_prop(propname):
    def parse(node, proj, path):
        apatth = None
        for i in range(node.attributes.length):
            attr = node.attributes.item(i)
            if attr.name == 'path':
                apath = attr_path(attr, path)
            else:
                unexpected_attr(node, attr)
        if apath is None:
            missing_attr(node, 'path')
        node_empty(node)
        getattr(proj, propname).append(Path(apath))
    return parse

def proj_target(func):
    def parse(node, proj, path):
        proj.targets.append(func(node, path))
    return parse

MODULE_TAG = {
    'executable': parse_executable,
    'module': parse_module,
    'external-library': parse_external_library,
}

PROJ_ELEM = {
    'ident': proj_simple_prop('ident', util.is_domain),
    'url': proj_simple_prop('url'),
    'email': proj_simple_prop('email'),
    'copyright': proj_simple_prop('copyright'),
    'module-path': proj_pathlist_prop('module_path'),
    'lib-path': proj_pathlist_prop('lib_path'),
    'executable': proj_target(parse_executable),
}

def parse_project(node, path):
    proj = project.Project()
    for i in range(node.attributes.length):
        attr = node.attributes.item(i)
        if attr.name == 'path':
            path = attr_path(attr, path)
        else:
            unexpected_attr(node, attr)
    node_elements_apply(
        node,
        (CONFIG_ELEM, (proj, path)),
        (PROJ_ELEM, (proj, path)),
        (NAME_ELEM, (proj,)),
        (FILENAME_ELEM, (proj,)))
    return proj

########################################
# Top level functions

ROOT_ELEM = {
    'project': parse_project,
    'module': parse_module,
    'external-library': parse_external_library,
}

def load(fp, base):
    """Load the project or module XML file from the given file."""
    doc = xml.dom.minidom.parse(fp)
    root = doc.documentElement
    try:
        func = ROOT_ELEM[root.tagName]
    except KeyError:
        raise ValueError('unexpected root element')
    return func(root, base)
