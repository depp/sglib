try:
    from lxml import etree
    is_lxml = True
except ImportError:
    from xml.etree import ElementTree as etree
    is_lxml = False

from build.path import Href, Path

def parse_file(path, rootenv):
    with open(path, 'rb') as fp:
        doc = etree.parse(fp)
    q = [(0, rootenv, doc.getroot())]
    while q:
        state, env, node = q.pop()
        if state == 0:
            if node.tail:
                q.append((1, env, (node.tail, 'tail', node)))
            loc = '{}:{}'.format(path, node.sourceline) if is_lxml else path
            try:
                subenv = env.add_node(node.tag, node.attrib, loc)
            except ValueError as ex:
                raise ValueError('{}: <{}>: {}'.format(loc, node.tag, ex))
            if subenv is not None:
                q.append((2, subenv, node))
                for subnode in reversed(node):
                    q.append((0, subenv, subnode))
                if node.text:
                    q.append((1, subenv, (node.text, 'text', node)))
        elif state == 1:
            try:
                env.add_str(node[0])
            except ValueError as ex:
                tag = node[2].tag
                if node[1] == 'tail':
                    tag = '/' + tag
                loc = ('{}:{}'.format(path, node[2].sourceline)
                       if is_lxml else path)
                raise ValueError('{}: after {}: {}'
                                 .format(loc, tag, ex))
        else:
            try:
                env.finish()
            except ValueError as ex:
                loc = ('{}:{}'.format(path, node.sourceline)
                       if is_lxml else path)
                raise ValueError('{}: <{}>: {}'
                                 .format(loc, node.tag, ex))

def dump_info(parent, info):
    for key, value in sorted(info.items()):
        elt = etree.SubElement(parent, 'key')
        elt.attrib['name'] = key
        last = None
        for item in value:
            if isinstance(item, str):
                if last is None:
                    nc = item if elt.text is None else elt.text + item
                    elt.text = nc or None
                else:
                    nc = item if last.tail is None else last.tail + item
                    last.tail = nc or None
            elif isinstance(item, Path):
                last = etree.SubElement(elt, 'path')
                last.attrib['path'] = item.path
            else:
                raise TypeError('invalid info type: {}'
                                .format(type(item).__name__))

def dump_group(parent, group):
    for req in group.requirements:
        elt = etree.SubElement(parent, 'require')
        elt.attrib['module'] = str(Href(None, str(req.module)))
        if req.public:
            elt.attrib['public'] = 'true'
    for ipath in group.header_paths:
        elt = etree.SubElement(parent, 'header-path')
        elt.attrib['path'] = ipath.path.path
        if ipath.public:
            elt.attrib['public'] = 'true'
    for src in group.sources:
        elt = etree.SubElement(parent, 'src')
        elt.attrib['path'] = src.path.path
        if src.type is not None:
            elt.attrib['type'] = src.type
    for subgroup in group.groups:
        elt = etree.SubElement(parent, 'group')
        dump_group(elt, subgroup)

def dump_module(parent, module):
    elt = etree.SubElement(parent, 'module')
    elt.attrib['type'] = module.type
    if module.name is not None:
        elt.attrib['name'] = str(module.name)
    dump_info(elt, module.info)
    dump_group(elt, module.group)
    for mod in module.modules:
        dump_module(elt, mod)

def dump(modules, info, fp):
    build = etree.Element('build')
    dump_info(build, info)
    for mod in modules:
        dump_module(build, mod)
    tree = etree.ElementTree(build)
    tree.write(fp, pretty_print=True, encoding='UTF-8')
