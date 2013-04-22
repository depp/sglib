from . import CONFIGS, PLATS, XMLNS
from build.error import ConfigError
from build.path import Path
from build.object.literalfile import LiteralFile
import xml.etree.ElementTree as etree
import uuid
Element = etree.Element
SubElement = etree.SubElement

def condition(config, plat):
    assert config in CONFIGS
    assert plat in PLATS
    return "'$(Configuration)|$(Platform)'=='%s|%s'" % (config, plat)

SOURCE_TYPES = {kk: (n, v) for n, (k, v) in enumerate([
    ('c c++', 'ClCompile'),
    ('h h++', 'ClInclude'),
    ('rc', 'ResourceCompile'),
]) for kk in k.split()}

def proj_import(root, path):
    SubElement(root, 'Import', {'Project': path})

def merge_defs(newval, oldval):
    if newval and oldval:
        return '{};{}'.format(newval, oldval)
    return newval or oldval

MERGE_FUNC = {
    'PreprocessorDefinitions': merge_defs,
}

def merge_attr(attrsets):
    result = {}
    for attrset in attrsets:
        for gname, group in attrset.items():
            rgroup = result.setdefault(gname, {})
            for aname, newval in group.items():
                try:
                    oldval = rgroup[aname]
                except KeyError:
                    rgroup[aname] = newval
                else:
                    try:
                        merge_func = MERGE_FUNC[aname]
                    except KeyError:
                        rgroup[aname] = newval
                    else:
                        rgroup[aname] = merge_func(newval, oldval)
    return result

BASE_ATTR = {
    'ClCompile': {
        'DebugInformationFormat': 'ProgramDatabase',
        'ObjectFileName': '$(IntDir)/%(RelativeDir)',
        'WarningLevel': 'Level3',
        'PreprocessorDefinitions':
        '_CRT_SECURE_NO_DEPRECATE;WIN32;_WINDOWS;'
        '%(PreprocessorDefinitions)',
    },
    'Link': {
        'Subsystem': 'Windows',
        'GenerateDebugInformation': 'true',
    },
}
CONFIG_ATTR = {
    'Debug': {
        'ClCompile': {
            'RuntimeLibrary': 'MultiThreadedDebugDLL',
            'Optimization': 'Disabled',
        },
    },
    'Release': {
        'ClCompile': {
            'RuntimeLibrary': 'MultiThreaded',
        },
    },
}
PLATFORM_ATTR = {
    'Win32': {
        'Link': {
            # 'TargetMachine': 'MachineX86',
        },
    },
    'x64': {
        'Link': {
            'EnableCOMDATFolding': 'true',
            'OptimizeReferences': 'true',
        }
    }
}

def make_executable_project(build, target):
    source = build.sourcemodules[target.source]

    root = Element('Project', {
        'xmlns': XMLNS,
        'ToolsVersion': '4.0',
        'DefaultTargets': 'Build',
    })

    cfgs = SubElement(root, 'ItemGroup', {'Label': 'ProjectConfigurations'})
    for c in CONFIGS:
        for p in PLATS:
            pc = SubElement(cfgs, 'ProjectConfiguration', {
                'Include': '{}|{}'.format(c, p),
            })
            SubElement(pc, 'Configuration').text = c
            SubElement(pc, 'Platform').text = p
    del cfgs, pc

    pg = SubElement(root, 'PropertyGroup', {'Label': 'Globals'})
    SubElement(pg, 'Keywords').text = 'Win32Proj'
    SubElement(pg, 'ProjectGuid').text = \
        '{{{}}}'.format(str(target.get_uuid(build.cfg)).upper())
    del pg

    proj_import(root, '$(VCTargetsPath)\\Microsoft.Cpp.Default.props')

    usedebugs = { 'Debug': 'true', 'Release': 'false' }
    toolsets = { 'Win32': None, 'x64': 'Windows7.1SDK' }
    for c in CONFIGS:
        for p in PLATS:
            pg = SubElement(root, 'PropertyGroup', {
                'Condition': condition(c, p),
                'Label': 'Configuration',
            })
            SubElement(pg, 'ConfigurationType').text = 'Application'
            SubElement(pg, 'UseDebugLibraries').text = usedebugs[c]
            if toolsets[p] is not None:
                SubElement(pg, 'PlatformToolset').text = toolsets[p]
    del pg

    proj_import(root, '$(VCTargetsPath)\\Microsoft.Cpp.props')

    SubElement(root, 'ImportGroup', {'Label': 'ExtensionSettings'})

    for c in CONFIGS:
        for p in PLATS:
            ig = SubElement(root, 'ImportGroup', {
                'Label': 'PropertySheets',
                'Condition': condition(c, p),
            })
            path = '$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props'
            i = SubElement(ig, 'Import', {
                'Project': path,
                'Condition': "exists('{}')".format(path),
                'Label': 'LocalAppDataPlatform',
            })
    del ig, i

    SubElement(root, 'PropertyGroup', {'Label': 'UserMacros'})

    for c in CONFIGS:
        for p in PLATS:
            pg = SubElement(root, 'PropertyGroup', {
                'Condition': condition(c, p),
            })
            SubElement(pg, 'LinkIncremental').text = 'true'
    del pg

    target_attr = {
        'ClCompile': {
            'AdditionalIncludeDirectories':
            ';'.join(build.cfg.target_path(ipath)
                     for ipath in source.header_paths),
            'PreprocessorDefinitions':
            ';'.join('{}={}'.format(k, v) if v is not None else k
                     for k, v in sorted(source.defs.items()))
        },
    }

    for c in CONFIGS:
        for p in PLATS:
            dg = SubElement(root, 'ItemDefinitionGroup', {
                'Condition': condition(c, p),
            })
            attrs = merge_attr([
                BASE_ATTR, target_attr, CONFIG_ATTR[c], PLATFORM_ATTR[p]])
            for gname, group in attrs.items():
                gelt = SubElement(dg, gname)
                for aname, aval in group.items():
                    SubElement(gelt, aname).text = aval

    groups = {}
    for src in source.sources:
        try:
            index, tag = SOURCE_TYPES[src.type]
        except KeyError:
            build.cfg.warn(
                'cannot add file to executable: {}'.format(src.path))
            continue
        try:
            group = groups[index]
        except KeyError:
            group = Element('ItemGroup')
            groups[index] = group
        SubElement(group, tag, {'Include': build.cfg.target_path(src.path)})
    for n, elt in sorted(groups.items()):
        root.append(elt)

    group = None
    for dep in source.deps:
        mod = build.modules[dep]
        if group is None:
            group = SubElement(root, 'ItemGroup')
        pref = SubElement(group, 'ProjectReference', {
            'Include': build.cfg.target_path(mod.project)})
        SubElement(pref, 'Project').text = \
            '{{{}}}'.format(mod.get_uuid(build.cfg))

    proj_import(root, '$(VCTargetsPath)\\Microsoft.Cpp.targets')

    SubElement(root, 'ImportGroup', {'Label': 'ExtensionTargets'})

    return etree.tostring(root, encoding='unicode')

def make_executable_filters(build, target):
    source = build.sourcemodules[target.source]
    filters = set()
    root = Element('Project', {
        'xmlns': XMLNS,
        'ToolsVersion': '4.0',
    })
    groups = {}
    for src in source.sources:
        try:
            index, tag = SOURCE_TYPES[src.type]
        except KeyError:
            build.cfg.warn(
                'cannot add file to executable: {}'.format(src.path))
            continue
        try:
            group = groups[index]
        except KeyError:
            group = Element('ItemGroup')
            groups[index] = group
        basename, filename = src.path.split()
        filter = basename.path[1:].replace('/', '\\') or 'Other Sources'
        elt = SubElement(group, tag, {
            'Include': build.cfg.target_path(src.path)})
        SubElement(elt, 'Filter').text = filter
        while filter not in filters:
            filters.add(filter)
            i = filter.rfind('\\')
            if i < 0:
                break
            filter = filter[:i]
        filters.add(filter)
    for n, elt in sorted(groups.items()):
        root.append(elt)
    fgroup = SubElement(root, 'ItemGroup')
    for filter in sorted(filters):
        elt = SubElement(fgroup, 'Filter', {'Include': filter})
        SubElement(elt, 'UniqueIdentifier').text = \
            '{{{}}}'.format(str(uuid.uuid4()))

    return etree.tostring(root, encoding='unicode')

def convert_arg(cfg, arg):
    parts = ['"']
    for part in arg:
        if isinstance(part, str):
            parts.append(part)
        elif isinstance(part, Path):
            parts.append('$(ProjectDir)')
            p = cfg.target_path(part)
            if p != '.':
                parts.append(p)
        else:
            raise TypeError('invalid argument type')
    parts.append('"')
    return ''.join(parts)

def make_executable_user(build, target):
    root = Element('Project', {
        'xmlns': XMLNS,
        'ToolsVersion': '4.0',
    })
    args = ' '.join(convert_arg(build.cfg, arg) for arg in target.args)
    for c in CONFIGS:
        for p in PLATS:
            pg = SubElement(root, 'PropertyGroup', {
                'Condition': condition(c, p),
            })
            SubElement(pg, 'LocalDebuggerCommandArguments').text = args
            SubElement(pg, 'DebuggerFlavor').text = 'WindowsLocalDebugger'

    return etree.tostring(root, encoding='unicode')

EXECUTABLE_FILES = [
    ('.vcxproj', make_executable_project),
    ('.vcxproj.filters', make_executable_filters),
    ('.vcxproj.user', make_executable_user),
]

def make_executable(build, target):
    path = Path('/', 'builddir').join1(target.filename)
    for ext, func in EXECUTABLE_FILES:
        build.add_generated_source(LiteralFile(
            target=path.addext(ext),
            contents=func(build, target),
        ))

TARGET_TYPES = {
    'executable': make_executable,
}

def make_target(build, target):
    target_type = target.target_type
    try:
        func = TARGET_TYPES[target_type]
    except KeyError:
        raise ConfigError('unsupported target type: {}'.format(target_type))
    func(build, target)
