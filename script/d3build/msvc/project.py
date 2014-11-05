# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import uuid as uuid_module
import xml.etree.ElementTree as etree
from ..util import indent_xml
from ..error import ConfigError
import io
import os
Element = etree.Element
SubElement = etree.SubElement

XMLNS = 'http://schemas.microsoft.com/developer/msbuild/2003'

def condition(variant):
    return "'$(Configuration)|$(Platform)'=='{}'".format(variant)

SOURCE_TYPES = {kk: (n, v) for n, (k, v) in enumerate([
    ('c c++', 'ClCompile'),
    ('h h++', 'ClInclude'),
    ('rc', 'ResourceCompile'),
    ('vcxproj', 'ProjectReference'),
]) for kk in k.split()}

def proj_import(root, path):
    SubElement(root, 'Import', {'Project': path})

def emit_properties(*, element, props, var=None):
    for k, v in sorted(props.items()):
        if isinstance(v, list):
            assert var is not None
            vs = '{};{}({})'.format(';'.join(v), var, k)
        elif isinstance(v, bool):
            vs = str(v).lower()
        elif isinstance(v, str):
            vs = v
        else:
            raise TypeError('unexpected property type: {}'.format(type(v)))
        SubElement(element, k).text = vs

TYPE_CPP = uuid_module.UUID('8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942')

class Project(object):
    """A Visual Studio project."""
    __slots__ = [
        # The project name.
        'name',
        # Path to the project file.
        'path',
        # The project type UUID.
        'type',
        # The project UUID.
        'uuid',
        # Map from solution (config, platform) to project (config, platform).
        'configs',
        # List of project dependencies (other projects).
        'dependencies',
    ]

    @property
    def sourcetype(self):
        return 'vcxproj'

    def emit(self):
        """Emit project files if necessary."""

class UserProject(Project):
    """A user-generated Visual Studio project."""
    __slots__ = [
        # Contents of the project file.
        '_data_project',
        # Contents of the filter file.
        '_data_filter',
        # Contents of the user file.
        '_data_user',
    ]

    def emit(self):
        """Emit project files if necessary."""
        with open(self.name + '.vcxproj', 'wb') as fp:
            fp.write(self._data_project)
        with open(self.name + '.vcxproj.filters', 'wb') as fp:
            fp.write(self._data_filter)
        with open(self.name + '.vcxproj.user', 'wb') as fp:
            fp.write(self._data_user)

def read_project(*, path, configs):
    """Read a Visual Studio project."""
    if os.path.splitext(path)[1] != '.vcxproj':
        raise UserError('invalid Visual Studio project extension')
    with open(path, 'rb') as fp:
        doc = etree.parse(fp)

    def get_uuid():
        gtag = etree.QName(XMLNS, 'PropertyGroup')
        ptag = etree.QName(XMLNS, 'ProjectGuid')
        for gelem in doc.getroot():
            if gelem.tag == gtag:
                for pelem in gelem:
                    if pelem.tag == ptag:
                        return uuid_module.UUID(pelem.text)
        raise ConfigError('could not detect project UUID: {}'.format(path))

    def get_configs():
        gtag = etree.QName(XMLNS, 'ItemGroup')
        itag = etree.QName(XMLNS, 'ProjectConfiguration')
        ctag = etree.QName(XMLNS, 'Configuration')
        ptag = etree.QName(XMLNS, 'Platform')
        configs = []
        for gelem in doc.getroot():
            if (gelem.tag != gtag or
                gelem.attrib.get('Label') != 'ProjectConfigurations'):
                continue
            for ielem in gelem:
                if ielem.tag != itag:
                    continue
                cfg = None
                plat = None
                for pelem in ielem:
                    if pelem.tag == ctag:
                        cfg = pelem.text
                    elif pelem.tag == ptag:
                        plat = pelem.text
                if cfg is None or plat is None:
                    raise ConfigError(
                        'could not parse project configurations')
                configs.append((cfg, plat))
        return configs

    obj = Project()
    obj.name = os.path.splitext(os.path.basename(path))[0]
    obj.path = path
    obj.type = TYPE_CPP
    obj.uuid = get_uuid()
    obj.configs = configs
    obj.dependencies = []
    return obj

def xml_data(root):
    indent_xml(root)
    return etree.tostring(root, encoding='UTF-8')

def create_project(*, name, sources, uuid, variants, props, arguments):
    """Create a Visual Studio project.

    name: the project name.
    sources: list of source files in the project.
    uuid: the project UUID.
    variants: list of "config|arch" variants.
    props: map from "config|arch" to map from group to prop dict.
    arguments: default arguments for debugging.
    """

    def create_project():
        root = Element('Project', {
            'xmlns': XMLNS,
            'ToolsVersion': '12.0',
            'DefaultTargets': 'Build',
        })

        cfgs = SubElement(
            root, 'ItemGroup', {'Label': 'ProjectConfigurations'})
        for variant in variants:
            pc = SubElement(
                cfgs, 'ProjectConfiguration', {'Include': variant})
            configuration, platform = variant.split('|')
            SubElement(pc, 'Configuration').text = configuration
            SubElement(pc, 'Platform').text = platform
        del cfgs, variant, configuration, platform, pc

        pg = SubElement(root, 'PropertyGroup', {'Label': 'Globals'})
        SubElement(pg, 'Keyword').text = 'Win32Proj'
        SubElement(pg, 'ProjectGuid').text = \
            '{{{}}}'.format(str(uuid).upper())
        # RootNamespace
        del pg

        proj_import(root, '$(VCTargetsPath)\\Microsoft.Cpp.Default.props')

        for variant in variants:
            emit_properties(
                element=SubElement(root, 'PropertyGroup', {
                    'Condition': condition(variant),
                    'Label': 'Configuration',
                }),
                props=props[variant]['Config'])
        del variant

        proj_import(root, '$(VCTargetsPath)\\Microsoft.Cpp.props')

        SubElement(root, 'ImportGroup', {'Label': 'ExtensionSettings'})

        for variant in variants:
            ig = SubElement(root, 'ImportGroup', {
                'Label': 'PropertySheets',
                'Condition': condition(variant),
            })
            path = '$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props'
            i = SubElement(ig, 'Import', {
                'Project': path,
                'Condition': "exists('{}')".format(path),
                'Label': 'LocalAppDataPlatform',
            })
        del variant, ig, path, i

        SubElement(root, 'PropertyGroup', {'Label': 'UserMacros'})

        for variant in variants:
            emit_properties(
                element=SubElement(root, 'PropertyGroup', {
                    'Condition': condition(variant),
                }),
                props=props[variant]['VC'],
                var='$')
        del variant

        for variant in variants:
            ig = SubElement(root, 'ItemDefinitionGroup', {
                'Condition': condition(variant),
            })
            for group in ('ClCompile', 'Link'):
                emit_properties(
                    element=SubElement(ig, group),
                    props=props[variant][group],
                    var='%')
        del variant, ig, group

        groups = {}
        for source in sources:
            try:
                index, tag = SOURCE_TYPES[source.sourcetype]
            except KeyError:
                raise ConfigError(
                    'cannot add file to executable: {}'.format(source.path))
            try:
                group = groups[index]
            except KeyError:
                group = Element('ItemGroup')
                groups[index] = group
            src = SubElement(group, tag, {'Include': source.path})
            if tag == 'ProjectReference':
                SubElement(src, 'Project').text = \
                    '{{{}}}'.format(str(source.uuid).upper())
        del source, index, tag, group, src
        for n, elt in sorted(groups.items()):
            root.append(elt)
        del n, elt

        proj_import(root, '$(VCTargetsPath)\\Microsoft.Cpp.targets')

        SubElement(root, 'ImportGroup', {'Label': 'ExtensionTargets'})

        return root

    def create_filter():
        filters = set()
        root = Element('Project', {
            'xmlns': XMLNS,
            'ToolsVersion': '12.0',
        })
        groups = {}
        for source in sources:
            index, tag = SOURCE_TYPES[source.sourcetype]
            if tag == 'ProjectReference':
                continue
            try:
                group = groups[index]
            except KeyError:
                group = Element('ItemGroup')
                groups[index] = group
            elt = SubElement(group, tag, {'Include': source.path})
            dirname, basename = os.path.split(source.path)
            if not dirname:
                continue
            filter = dirname
            SubElement(elt, 'Filter').text = filter
            while filter and filter not in filters:
                filters.add(filter)
                filter = os.path.dirname(filter)
        for n, elt in sorted(groups.items()):
            root.append(elt)
        fgroup = SubElement(root, 'ItemGroup')
        for filter in sorted(filters):
            elt = SubElement(fgroup, 'Filter', {'Include': filter})
            SubElement(elt, 'UniqueIdentifier').text = \
                '{{{}}}'.format(str(uuid_module.uuid4()).upper())
        return root

    def convert_arg(arg):
        return '"{}"'.format(arg)

    def create_user():
        root = Element('Project', {
            'xmlns': XMLNS,
            'ToolsVersion': '12.0',
        })
        args = ' '.join(convert_arg(arg) for arg in arguments)
        for variant in variants:
            pg = SubElement(root, 'PropertyGroup', {
                'Condition': condition(variant),
            })
            SubElement(pg, 'LocalDebuggerCommandArguments').text = args
            SubElement(pg, 'DebuggerFlavor').text = 'WindowsLocalDebugger'
            epath = props[variant]['Debug'].get('Path', ())
            if epath:
                SubElement(pg, 'LocalDebuggerEnvironment').text = \
                    'PATH=%PATH%;' + ';'.join(epath)
        return root

    def create_object():
        configs = set(x.split('|')[0] for x in variants)
        obj = UserProject()
        obj.name = name
        obj.path = name + '.vcxproj'
        obj.type = TYPE_CPP
        obj.uuid = uuid
        obj.configs = {c: c for c in configs}
        obj.dependencies = [source for source in sources
                            if source.sourcetype == 'vcxproj']
        obj._data_project = xml_data(create_project())
        obj._data_filter = xml_data(create_filter())
        obj._data_user = xml_data(create_user())
        return obj

    return create_object()
