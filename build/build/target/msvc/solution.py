from . import CONFIGS, PLATS
from build.object.literalfile import LiteralFile
from build.path import Path
import io

def make_solution(build, proj):
    f = io.StringIO()

    f.write(
        '\ufeff\n'
        'Microsoft Visual Studio Solution File, Format Version 11.00\n'
        '# Visual C++ Express 2010\n'
    )

    for target in build.targets:
        f.write(
            'Project("{{{sln_guid}}}") = '
            '"{proj_name}", "{proj_path}", "{{{proj_uuid}}}"\n'
            'EndProject\n'
            .format(sln_guid=str(proj.get_uuid(build.cfg)).upper(),
                    proj_name=target.filename,
                    proj_path=build.cfg.target_path(
                        Path('/', 'builddir').join1(
                            target.filename, '.vcxproj')),
                    proj_uuid=str(target.get_uuid(build.cfg)).upper()))

    f.write('Global\n')

    f.write('\tGlobalSection(SolutionConfigurationPlatforms) = '
            'preSolution\n')
    for config in CONFIGS:
        for plat in PLATS:
            f.write('\t\t{config}|{plat} = {config}|{plat}\n'
                    .format(config=config, plat=plat))
    f.write('\tEndGlobalSection\n')

    f.write('\tGlobalSection(ProjectConfigurationPlatforms) = '
            'postSolution\n')
    for target in build.targets:
        for config in CONFIGS:
            for plat in PLATS:
                f.write(
                    '\t\t{{{uuid}}}.{cp}.ActiveCfg = {cp}\n'
                    '\t\t{{{uuid}}}.{cp}.Build.0 = {cp}\n'
                    .format(uuid=str(target.uuid).upper(),
                            cp='{}|{}'.format(config, plat)))
    f.write('\tEndGlobalSection\n')

    f.write(
        '\tGlobalSection(SolutionProperties) = preSolution\n'
        '\t\tHideSolutionNode = FALSE\n'
        '\tEndGlobalSection\n')

    f.write('EndGlobal\n')

    build.add_generated_source(LiteralFile(
        target= Path('/', 'builddir').join1(proj.filename, '.sln'),
        contents=f.getvalue()))
