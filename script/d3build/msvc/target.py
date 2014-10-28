# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from ..target import BaseTarget
from ..error import ConfigError, UserError
from .project import create_project
from .solution import solution_data
import uuid as uuid_module

def group_vars(varset):
    """Group variables by section."""
    sections = {k: {} for k in ('Config', 'VC', 'ClCompile', 'Link')}
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
    """Object for creating an Xcode project."""
    __slots__ = [
        # Name for the solution.
        '_name',
        # Map from project name to project object.
        '_projects',
    ]

    def __init__(self, name, script, config, env):
        super(VisualStudioTarget, self).__init__(name, script, config, env)
        self._name = name
        self._projects = {}

    def finalize(self):
        super(VisualStudioTarget, self).finalize()
        sln_data = solution_data(
            configs=self.env.configs,
            projects=sorted(self._projects.values(), key=lambda x: x.name))
        with open(self._name + '.sln', 'wb') as fp:
            fp.write(sln_data)
        for project in self._projects.values():
            project.emit()

    @property
    def run_srcroot(self):
        """The path root of the source tree, at runtime."""
        return ''

    def add_executable(self, *, name, module, uuid=None, arguments=[]):
        """Create an executable target.

        Returns the path to the executable.
        """
        if module is None:
            return ''
        if uuid is None:
            uuid = uuid_module.uuid4()
        else:
            uuid = uuid_module.UUID(uuid)
        varset = self.env.schema.merge(module.varsets)
        varset['Config.ConfigurationType'] = 'Application'
        project_refs = varset.get('.ProjectReferences', [])
        for project in project_refs:
            self._add_project(project)
        self._add_project(create_project(
            name=name,
            sources=module.sources,
            uuid=uuid,
            configs=[(c, p) for c in self.env.configurations
                     for p in self.env.platforms],
            props={
                k: group_vars(self.env.schema.merge([varset, v]))
                for k, v in self.env.base_vars.items()
            },
            project_refs=project_refs,
        ))

    def _add_project(self, project):
        """Add a project to the solution."""
        try:
            existing_project = self._projects[project.name]
        except KeyError:
            self._projects[project.name] = project
            return
        if project is not existing_project:
            raise UserError('duplicate projects: {}'.format(project.name))
