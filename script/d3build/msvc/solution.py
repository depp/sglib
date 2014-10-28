# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
import io

SOLUTION = """\
\ufeff
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio 2013
VisualStudioVersion = 12.0.21005.1
MinimumVisualStudioVersion = 10.0.40219.1
{project_refs}\
Global
\tGlobalSection(SolutionConfigurationPlatforms) = preSolution
{configurations}\
\tEndGlobalSection
\tGlobalSection(ProjectConfigurationPlatforms) = postSolution
{project_configs}\
\tEndGlobalSection
\tGlobalSection(SolutionProperties) = preSolution
\t\tHideSolutionNode = FALSE
\tEndGlobalSection
EndGlobal
"""

def solution_data(*, configs, projects):
    configurations = io.StringIO()
    project_refs = io.StringIO()
    project_configs = io.StringIO()
    for config, platform in configs:
        print('\t\t{0}|{1} = {0}|{1}'.format(config, platform),
              file=configurations)
    for project in projects:
        uuid = str(project.uuid).upper()
        print('Project("{}") = "{}", "{}", "{}"'
              .format(project.type, project.name, project.path, uuid),
              file=project_refs)
        print('EndProject', file=project_refs)
        for config1 in configs:
            config2 = project.configs.get(config1, config1)
            x = '{}|{}'.format(*config1)
            y = '{}|{}'.format(*config2)
            print('\t\t{}.{}.ActiveCfg = {}'.format(uuid, x, y),
                  file=project_configs)
            print('\t\t{}.{}.Build.0 = {}'.format(uuid, x, y),
                  file=project_configs)
    text = SOLUTION.format(
        project_refs=project_refs.getvalue(),
        project_configs=project_configs.getvalue(),
        configurations=configurations.getvalue())
    return text.encode('UTF-8').replace(b'\n', b'\r\n')
