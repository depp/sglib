from build.path import Href
import uuid

class Project(object):
    __slots__ = ['modules', 'info', '_counter', 'files', 'uuid']

    def __init__(self):
        self.modules = []
        self.info = None
        self._counter = 0
        self.files = []
        self.uuid = None

    def get_uuid(self, cfg):
        if self.uuid is None:
            self.uuid = uuid.uuid4()
            cfg.warn('project has no UUID, generated: {}'.format(self.uuid))
        return self.uuid

    @classmethod
    def load_xml(class_, cfg, ipaths):
        """Load a project for the given configuration."""
        proj = class_()
        from . import xml

        basename, ext = cfg.projfile.splitext()
        if ext != '.xml':
            raise ValueError('expected .xml extension for project file')
        first = True

        q = [basename]
        paths = set(q)
        refmap = {}
        while q:
            xmlpath = q.pop()
            proj.files.append(xmlpath.addext('.xml'))
            buildfile = xml.parse_file(cfg, xmlpath, ipaths)
            ref = Href(xmlpath, None)
            if buildfile.default is not None and buildfile.default != ref:
                refmap[ref] = buildfile.default
            buildfile.expand_templates(cfg, proj)
            if xmlpath == basename:
                proj.info = buildfile.info
            npaths = set()
            for m in buildfile.modules:
                npaths.update(ref.path for ref in m.module_refs())
                proj.modules.append(m)
            npaths.discard(None)
            npaths.difference_update(paths)
            q.extend(npaths)
            paths.update(npaths)

        for m in proj.modules:
            m.update_refs(refmap)

        modnames = set()
        for m in proj.modules:
            if m.name is not None:
                if m.name in modnames:
                    raise ValueError('duplicate module name: {}'
                                     .format(m.name))
                modnames.add(m.name)

        for m in proj.modules:
            for ref in m.module_refs():
                if ref not in modnames:
                    raise ValueError('undefined module ref: {}'
                                     .format(ref))

        proj.uuid = proj.info.get_uuid('uuid', None)

        return proj

    def dump_xml(self, fp):
        """Dump a project as XML."""
        from . import xml
        xml.dump(self.modules, self.info, fp)

    def gen_name(self):
        """Generate a unique module name."""
        self._counter += 1
        return Href(None, 'proj-{}'.format(str(self._counter)))

    @property
    def filename(self):
        """The filename to use for project files."""
        info = self.info
        if 'filename' in info:
            return info.get_string('filename')
        elif 'name' in info:
            return info.get_string('name')
        raise ConfigError('project lacks name or filename attribute')
