__all__ = ['Project']
from gen.project.config import ConfigSet

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
