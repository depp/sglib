from lxml import etree

def parse_file(path, rootenv):
    xml_parser = etree.XMLParser(remove_comments=True)
    with open(path, 'rb') as fp:
        doc = etree.parse(fp, xml_parser)
    q = [(0, rootenv, doc.getroot())]
    while q:
        state, env, node = q.pop()
        if state == 0:
            if node.tail:
                q.append((1, env, (node.tail, 'tail', node)))
            loc = '{}:{}'.format(path, node.sourceline)
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
                raise ValueError('{}: after {}, line {}: {}'
                                 .format(path, tag, node[2].sourceline, ex))
        else:
            try:
                env.finish()
            except ValueError as ex:
                raise ValueError('{}:{}: <{}>: {}'
                                 .format(path, node.sourceline, node.tag, ex))
