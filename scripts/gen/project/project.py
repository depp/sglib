__all__ = ['Project']
from gen.project.config import ConfigSet
from gen.error import ConfigError
from gen.os import INTRINSICS

class Project(object):
    """A project is a set of modules and project-wide settings."""

    __slots__ = [
        'name', 'ident', 'filename', 'url', 'email', 'copyright',
        'config', 'module_path', 'lib_path', 'modules', 'targets',
    ]

    def __init__(self):
        self.name = None
        self.ident = None
        self.filename = None
        self.url = None
        self.email = None
        self.copyright = None
        self.config = ConfigSet()
        self.module_path = []
        self.lib_path = []
        self.modules = {}
        self.targets = []

    def add_config(self, config):
        self.config.add_config(config)

    def validate(self):
        errors = []
        objs = []
        for target in self.targets:
            objs.append((target, 'target {}'.format(target.name)))
        for modid, module in self.modules.items():
            objs.append((module, 'module {}'.format(modid)))
        enable = set()
        enable.update(INTRINSICS)
        for obj, name in objs:
            enable.update(obj.enable_flags())
        enable = frozenset(enable)
        for obj, name in objs:
            try:
                obj.validate(enable)
            except ConfigError as ex:
                ex.reason = '{} (in {})'.format(ex.reason, name)
                errors.append(ex)
        if errors:
            raise ConfigError('error validating project',
                              suberrors=errors)
