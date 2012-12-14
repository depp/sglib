import gen.atom as atom
from gen.path import Path
import gen.info as info
import re

CONFIGS = ('Debug', 'Release')
PLATS = ('Win32', 'x64')

def condition(config, plat):
    assert config in CONFIGS
    assert plat in PLATS
    return "'$(Configuration)|$(Platform)'=='%s|%s'" % (config, plat)

def _textelt(doc, name, content):
    n = doc.createElement(name)
    n.appendChild(doc.createTextNode(content))
    return n

def mkdef(k, v):
    if v is None:
        return k
    return '%s=%s' % (k, v)

class Sources(object):
    """Sources for a project."""

    def __init__(self, targenv):
        compiles = []
        includes = []
        types_c = 'c', 'cxx'
        types_h = 'h', 'hxx'
        def handle_c(source, env):
            compiles.append(source)
        def handle_h(source, env):
            includes.append(source)
        handlers = {}
        for t in types_c: handlers[t] = handle_c
        for t in types_h: handlers[t] = handle_h
        targenv.apply(handlers)
        compiles.sort()
        includes.sort()
        self.compiles = compiles
        self.includes = includes

    def groups(self):
        yield 'ClCompile', self.compiles
        yield 'ClInclude', self.includes

class Project(object):
    """A project file.

    This is for either the vcxproj, vcxproj.user, or vcxproj.filters.
    """
    __slots__ = ['doc']
    def __init__(self):
        import xml.dom.minidom
        impl = xml.dom.minidom.getDOMImplementation()
        doc = impl.createDocument(None, 'Project', None)
        root = doc.documentElement
        root.setAttribute('ToolsVersion', '4.0')
        root.setAttribute(
            'xmlns', 'http://schemas.microsoft.com/developer/msbuild/2003')
        self.doc = doc

    @property
    def root(self):
        return self.doc.documentElement

    def write(self, fname):
        print('FILE', fname)
        with open(fname, 'wb') as f:
            self.doc.writexml(f, encoding='UTF-8')

def vcxproj_emit(pname, targenv, sources):
    # All the local functions make it so we don't accidentally
    # reuse a variable we don't want.
    vproj = Project()
    env = targenv.unionenv()
    root = vproj.root
    doc = vproj.doc

    def textelt(name, context):
        return _textelt(doc, name, context)

    root.setAttribute('DefaultTargets', 'Build')
    def radd(x):
        root.appendChild(x)

    def pimport(proj):
        n = doc.createElement('Import')
        n.setAttribute('Project', proj)
        return n

    def configs():
        configs = doc.createElement('ItemGroup')
        configs.setAttribute('Label', 'ProjectConfigurations')
        for c in CONFIGS:
            for p in PLATS:
                pc = doc.createElement('ProjectConfiguration')
                pc.setAttribute('Include', '%s|%s' % (c, p))
                pc.appendChild(textelt('Configuration', c))
                pc.appendChild(textelt('Platform', p))
                configs.appendChild(pc)
        radd(configs)
    configs()

    def mglobals():
        pg = doc.createElement('PropertyGroup')
        pg.setAttribute('Label', 'Globals')
        pg.appendChild(textelt('Keyword', 'Win32Proj'))
        radd(pg)
    mglobals()

    radd(pimport('$(VCTargetsPath)\\Microsoft.Cpp.Default.props'))

    def configs2():
        usedebugs = { 'Debug': 'true', 'Release': 'false' }
        toolsets = { 'Win32': None, 'x64': 'Windows7.1SDK' }
        for c in CONFIGS:
            usedebug = usedebugs[c]
            for p in PLATS:
                pg = doc.createElement('PropertyGroup')
                pg.setAttribute('Condition', condition(c, p))
                pg.setAttribute('Label', 'Configuration')
                pg.appendChild(
                    textelt('ConfigurationType', 'Application'))
                pg.appendChild(textelt('UseDebugLibraries', usedebug))
                toolset = toolsets[p]
                if toolset is not None:
                    pg.appendChild(textelt('PlatformToolset', toolset))
                radd(pg)
    configs2()

    radd(pimport('$(VCTargetsPath)\\Microsoft.Cpp.props'))

    def extsettings():
        n = doc.createElement('ImportGroup')
        n.setAttribute('Label', 'ExtensionSettings')
        radd(n)
    extsettings()

    def igroups():
        for c in CONFIGS:
            for p in PLATS:
                ig = doc.createElement('ImportGroup')
                ig.setAttribute('Label', 'PropertySheets')
                ig.setAttribute('Condition', condition(c, p))
                i = doc.createElement('Import')
                f = '$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props'
                i.setAttribute('Project', f)
                i.setAttribute('Condition', "exists('%s')" % (f,))
                i.setAttribute('Label', 'LocalAppDataPlatform')
                ig.appendChild(i)
                radd(ig)
    igroups()

    def umacros():
        n = doc.createElement('PropertyGroup')
        n.setAttribute('Label', 'UserMacros')
        radd(n)
    umacros()

    def configs3():
        for c in CONFIGS:
            for p in PLATS:
                pg = doc.createElement('PropertyGroup')
                pg.setAttribute('Condition', condition(c, p))
                pg.appendChild(textelt('LinkIncremental', 'true'))
                radd(pg)
    configs3()

    def configs4():
        cdefs = { 'Debug': '_DEBUG', 'Release': 'NDEBUG' }
        cattr = {
            None: [
                ('AdditionalIncludeDirectories',
                 ';'.join(x.windows for x in env.CPPPATH)),
                ('DebugInformationFormat', 'ProgramDatabase'),
                ('ObjectFileName', '$(IntDir)/%(RelativeDir)'),
                ('WarningLevel', 'Level3'),
            ],
            'Debug': [
                ('RuntimeLibrary', 'MultiThreadedDebugDLL'),
                ('Optimization', 'Disabled'),
            ],
            'Release': [
                ('RuntimeLibrary', 'MultiThreaded'),
            ],
            'Win32': [],
            'x64': [],
        }
        lattr = {
            None: [
                ('Subsystem', 'Windows'),
                ('GenerateDebugInformation', 'true'),
            ],
            'Debug': [],
            'Release': [],
            'Win32': [
                # ('TargetMachine', 'MachineX86'),
            ],
            'x64': [
                ('EnableCOMDATFolding', 'true'),
                ('OptimizeReferences', 'true'),
            ],
        }
        for c in CONFIGS:
            cdef = cdefs[c]
            for p in PLATS:
                dg = doc.createElement('ItemDefinitionGroup')
                dg.setAttribute('Condition', condition(c, p))

                def sgadd(n, t):
                    if t:
                        sg.appendChild(textelt(n, t))

                sg = doc.createElement('ClCompile')
                dg.appendChild(sg)
                defs = [mkdef(k, v) for k, v in env.DEFS] + [
                    '_CRT_SECURE_NO_DEPRECATE',
                    'WIN32',
                    cdef,
                    '_WINDOWS',
                    '%(PreprocessorDefinitions)'
                ]
                sgadd('PreprocessorDefinitions', ';'.join(defs))
                for g in (None, c, p):
                    for k, v in cattr[g]:
                        sgadd(k, v)

                sg = doc.createElement('Link')
                dg.appendChild(sg)
                for g in (None, c, p):
                    for k, v in lattr[g]:
                        sgadd(k, v)

                radd(dg)
    configs4()

    def files():
        for tag, srclist in sources.groups():
            ig = doc.createElement('ItemGroup')
            for src in srclist:
                n = doc.createElement(tag)
                n.setAttribute('Include', src.relpath.windows)
                ig.appendChild(n)
            radd(ig)
    files()

    radd(pimport('$(VCTargetsPath)\\Microsoft.Cpp.targets'))

    def exttargets():
        ig = doc.createElement('ImportGroup')
        ig.setAttribute('Label', 'ExtensionTargets')
        radd(ig)
    exttargets()

    vproj.write(pname + '.vcxproj')

def vcxproj_filters_emit(pname, targenv, sources):
    import uuid
    vproj = Project()
    env = targenv.unionenv()
    root = vproj.root
    doc = vproj.doc

    fnames = set()
    def add_fname(fname):
        while fname:
            if fname in fnames:
                return
            fnames.add(fname)
            i = fname.rfind('\\')
            if i < 0:
                return
            fname = fname[:i]

    for tag, srclist in sources.groups():
        ig = doc.createElement('ItemGroup')
        for src in srclist:
            n = doc.createElement(tag)
            n.setAttribute('Include', src.relpath.windows)
            gpath = src.grouppath.windows
            gname = src.group.name
            i = gpath.rfind('\\')
            if i < 0:
                fname = gname
            else:
                fname = gname + '\\' + gpath[:i]
            add_fname(fname)
            f = doc.createElement('Filter')
            f.appendChild(doc.createTextNode(fname))
            n.appendChild(f)
            ig.appendChild(n)
        root.appendChild(ig)

    fnames = list(fnames)
    fnames.sort()
    ig = doc.createElement('ItemGroup')
    for fname in fnames:
        f = doc.createElement('Filter')
        f.setAttribute('Include', fname)
        n = doc.createElement('UniqueIdentifier')
        u = '{%s}' % (str(uuid.uuid4()),)
        n.appendChild(doc.createTextNode(u))
        f.appendChild(n)
        ig.appendChild(f)
    root.appendChild(ig)

    vproj.write(pname + '.vcxproj.filters')

def convert_arg1(arg):
    if isinstance(arg, Path):
        p = arg.windows
        if p == '.':
            p = ''
        return '$(ProjectDir)%s' % (p,)
    elif isinstance(arg, str):
        return arg
    else:
        raise TypeError('invalid argument type: %r' % (arg,))

def convert_arg(arg):
    if isinstance(arg, Path):
        x = convert_arg1(arg)
    elif isinstance(arg, str):
        x = convert_arg1(arg)
    elif isinstance(arg, tuple):
        x = ''
        for i in arg:
            x += convert_arg1(i)
    else:
        raise TypeError('invalid argument type: %r' % (arg,))
    return '"%s"' % (x,)

def vcxproj_user_emit(pname, targenv, sources):
    import uuid
    vproj = Project()
    env = targenv.unionenv()
    root = vproj.root
    doc = vproj.doc

    args = []
    for k, v in targenv.DEFAULT_CVARS:
        args.append('/D%s=%s' % (k, convert_arg(v)))
    args = ' '.join(args)

    for c in CONFIGS:
        for p in PLATS:
            pg = doc.createElement('PropertyGroup')
            pg.setAttribute('Condition', condition(c, p))
            if args:
                n = doc.createElement('LocalDebuggerCommandArguments')
                n.appendChild(doc.createTextNode(args))
                pg.appendChild(n)
            n = doc.createElement('DebuggerFlavor')
            n.appendChild(doc.createTextNode('WindowsLocalDebugger'))
            pg.appendChild(n)
            root.appendChild(pg)

    vproj.write(pname + '.vcxproj.user')

def sln_emit(proj, pnames):
    import io, uuid
    x = io.StringIO()
    x.write('\uFEFF\n'.encode('utf-8'))
    x.write('Microsoft Visual Studio Solution File, Format Version 11.00\n')
    x.write('# Visual C++ Express 2010\n')
    uuids = {}
    for pname in pnames:
        uuids[pname] = str(uuid.uuid4()).upper()
    su = str(uuid.uuid4()).upper()
    for pname in pnames:
        x.write(
            'Project("{%s}") = "%s", "%s.vcxproj", "{%s}"\nEndProject\n' %
            (su, pname, pname, uuids[pname]))
    x.write('Global\n')

    x.write('\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n')
    for c in CONFIGS:
        for p in PLATS:
            x.write('\t\t%s|%s = %s|%s\n' % (c, p, c, p))
    x.write('\tEndGlobalSection\n')

    x.write('\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n')
    for pname in pnames:
        for c in CONFIGS:
            for p in PLATS:
                for s in ('ActiveCfg', 'Build.0'):
                    x.write('\t\t{%s}.%s|%s.%s = %s|%s\n' %
                            (uuids[pname], c, p, s, c, p))
    x.write('\tEndGlobalSection\n')

    x.write('\tGlobalSection(SolutionProperties) = preSolution\n')
    x.write('\t\tHideSolutionNode = FALSE\n')
    x.write('\tEndGlobalSection\n')

    x.write('EndGlobal\n')

    path = proj.info.PKG_FILENAME + '.sln'
    print('FILE', path)
    with open(path, 'wb') as f:
        f.write(x.getvalue().replace('\n', '\r\n'))

def files_msvc(proj):
    yield Path(proj.info.PKG_FILENAME + '.sln')
    for module in proj.targets():
        pname = info.make_filename(module.info.EXE_WINDOWS)
        for ext in ('.vcxproj', '.vcxproj.filters', '.vcxproj.user'):
            yield Path(pname + ext)

def write_msvc(proj):
    projenv = atom.ProjectEnv(proj)
    pnames = []
    for targenv in projenv.targets('WINDOWS'):
        pname = info.make_filename(targenv.EXE_WINDOWS)
        pnames.append(pname)
        srcs = Sources(targenv)
        vcxproj_emit(pname, targenv, srcs)
        vcxproj_filters_emit(pname, targenv, srcs)
        vcxproj_user_emit(pname, targenv, srcs)
    sln_emit(proj, pnames)
