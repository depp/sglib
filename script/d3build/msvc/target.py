# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..target import BaseTarget
from ..error import ConfigError, UserError
from .project import create_project
from .solution import solution_data
from .schema import VisualStudioSchema
from .module import VisualStudioModule
from . import base
import uuid as uuid_module

GROUPS = 'Config', 'VC', 'ClCompile', 'Link', 'Debug'

def group_vars(varset):
    """Group variables by section."""
    sections = {k: {} for k in GROUPS}
    for k, v in varset.items():
        i = k.find('.')
        assert i >= 0
        if i == 0:
            continue
        section = k[:i]
        key = k[i+1:]
        sections[section][key] = v
    return sections

class VisualStudioTarget(BaseTarget):
    """Object for creating a Visual Studio solution."""
    __slots__ = [
        # The schema for build variables.
        'schema',
        # The base build variables, a module.
        'base',
        # Name for the solution.
        '_name',
        # Map from project name to project object.
        '_projects',
    ]

    def __init__(self, build, name):
        super(VisualStudioTarget, self).__init__()
        self.schema = VisualStudioSchema()
        self.base = VisualStudioModule(self.schema)
        self.base.add_variables(base.BASE_CONFIG)
        self.base.add_variables(base.DEBUG_CONFIG, configs=['Debug'])
        self.base.add_variables(base.RELEASE_CONFIG, configs=['Release'])
        self._name = name
        self._projects = {}

    def finalize(self):
        super(VisualStudioTarget, self).finalize()
        sln_data = solution_data(
            variants=self.schema.variants,
            projects=sorted(self._projects.values(), key=lambda x: x.name))
        with open(self._name + '.sln', 'wb') as fp:
            fp.write(sln_data)
        for project in self._projects.values():
            project.emit()

    @property
    def run_srcroot(self):
        """The path root of the source tree, at runtime."""
        return '$(SolutionDir)'

    def module(self):
        return VisualStudioModule(self.schema).add_module(self.base)

    def add_executable(self, *, name, module, uuid=None, arguments=[]):
        """Create an executable target.

        Returns the path to the executable.
        """
        if not self._add_module(module):
            return
        if uuid is None:
            uuid = uuid_module.uuid4()
        else:
            uuid = uuid_module.UUID(uuid)

        varset = module.variables().get_all()
        settings = {
            'Config.ConfigurationType': 'Application',
            'Link.SubSystem': 'Windows',
            'ClCompile.ObjectFileName': '$(IntDir)\\%(RelativeDir)',
        }
        for varname, value in settings.items():
            for variant in self.schema.variants:
                varset[variant][varname] = value
        props = {k: group_vars(v) for k, v in varset.items()}

        self._add_project(create_project(
            name=name,
            sources=module.sources,
            uuid=uuid,
            variants=self.schema.variants,
            props=props,
            arguments=arguments,
        ))

    def _add_project(self, project):
        """Add a project to the solution."""
        try:
            existing_project = self._projects[project.name]
        except KeyError:
            pass
        else:
            if project is not existing_project:
                raise UserError('duplicate projects: {}'.format(project.name))
            return
        self._projects[project.name] = project
        for dep in project.dependencies:
            self._add_project(dep)
