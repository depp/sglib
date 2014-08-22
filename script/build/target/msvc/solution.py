# Copyright 2013 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from . import CONFIGS, PLATS
from .module import MSVCModule
from build.object.literalfile import LiteralFile
from build.path import Path
from build.error import ConfigError
import collections
import io
import uuid

Project = collections.namedtuple('Project', 'path uuid configs')
TYPE_UUID = uuid.UUID("8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942")

def make_solution(build, proj):
    projects = []
    configs = [(c, p) for c in CONFIGS for p in PLATS]
    for target in build.targets():
        projects.append(Project(
            Path('/', 'builddir').join1(target.filename, '.vcxproj'),
            target.get_uuid(build.cfg),
            configs))
    for module in build.modules.values():
        if not isinstance(module, MSVCModule):
            continue
        projects.append(Project(
            module.project, module.get_uuid(build.cfg), module.configs))

    f = io.StringIO()

    f.write(
        '\ufeff\n'
        'Microsoft Visual Studio Solution File, Format Version 11.00\n'
        '# Visual C++ Express 2010\n'
    )

    for project in projects:
        f.write(
            'Project("{{{type_guid}}}") = '
            '"{proj_name}", "{proj_path}", "{{{proj_uuid}}}"\n'
            'EndProject\n'
            .format(type_guid=str(TYPE_UUID).upper(),
                    proj_name=project.path.splitext()[0].basename(),
                    proj_path=build.cfg.target_path(project.path),
                    proj_uuid=str(project.uuid).upper()))

    f.write('Global\n')

    f.write('\tGlobalSection(SolutionConfigurationPlatforms) = '
            'preSolution\n')
    for config, plat in configs:
        f.write('\t\t{config}|{plat} = {config}|{plat}\n'
                .format(config=config, plat=plat))
    f.write('\tEndGlobalSection\n')

    f.write('\tGlobalSection(ProjectConfigurationPlatforms) = '
            'postSolution\n')
    for project in projects:
        for config, plat in configs:
            cp = '{}|{}'.format(config, plat)
            if (config, plat) in project.configs:
                f.write(
                    '\t\t{{{uuid}}}.{cp}.ActiveCfg = {cp}\n'
                    '\t\t{{{uuid}}}.{cp}.Build.0 = {cp}\n'
                    .format(uuid=str(project.uuid).upper(), cp=cp))
            else:
                fallback = list(project.configs)
                if not fallback:
                    raise ConfigError('project has no configurations: {}'
                                      .format(project.path))
                build.cfg.warn(
                    'project does not support config {}: {}'
                    .format(cp, project.path))
                def key(cp):
                    if cp[0] == config: return 0
                    if cp[1] == plat: return 1
                    return 2
                fallback.sort(key=key)
                cp2 = '{}|{}'.format(*fallback[0])
                f.write(
                    '\t\t{{{uuid}}}.{cp}.ActiveCfg = {cp2}\n'
                    .format(uuid=str(target.uuid).upper(), cp=cp, cp2=cp2))

    f.write('\tEndGlobalSection\n')

    f.write(
        '\tGlobalSection(SolutionProperties) = preSolution\n'
        '\t\tHideSolutionNode = FALSE\n'
        '\tEndGlobalSection\n')

    f.write('EndGlobal\n')

    build.add(None, LiteralFile(
        target= Path('/', 'builddir').join1(proj.filename, '.sln'),
        contents=f.getvalue()))
