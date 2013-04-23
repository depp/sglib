from . import XMLNS
from build.error import ProjectError, ConfigError
import build.object.source as source
import xml.etree.ElementTree as etree
import uuid

def get_uuid(doc):
    gtag = etree.QName(XMLNS, 'PropertyGroup')
    ptag = etree.QName(XMLNS, 'ProjectGuid')
    for gelem in doc.getroot():
        if gelem.tag == gtag and gelem.attrib.get('Label') == 'Globals':
            for pelem in gelem:
                if pelem.tag == ptag:
                    return uuid.UUID(pelem.text)
    return None

def get_configs(doc):
    gtag = etree.QName(XMLNS, 'ItemGroup')
    itag = etree.QName(XMLNS, 'ProjectConfiguration')
    ctag = etree.QName(XMLNS, 'Configuration')
    ptag = etree.QName(XMLNS, 'Platform')
    configs = []
    for gelem in doc.getroot():
        if (gelem.tag != gtag or
            gelem.attrib.get('Label') != 'ProjectConfigurations'):
            continue
        for ielem in gelem:
            if ielem.tag != itag:
                continue
            cfg = None
            plat = None
            for pelem in ielem:
                if pelem.tag == ctag:
                    cfg = pelem.text
                elif pelem.tag == ptag:
                    plat = pelem.text
            if cfg is None or plat is None:
                raise ConfigError('could not parse project configurations')
            configs.append((cfg, plat))
    return configs

class MSVCModule(object):
    """A module which is built by an MSVC project."""
    __slots__ = ['project', 'uuid', 'configs']

    def get_uuid(self, cfg):
        if self.uuid is None:
            self.uuid = uuid.uuid4()
        return self.uuid

def build_msvc(build, mod, name, external):
    sources = []
    for group in mod.group.all_groups():
        sources.extend(group.sources)
    deps = []
    for src in sources:
        pname = build.gen_name()
        obj = MSVCModule()
        obj.project = src.path
        if obj.project.splitext()[1] != '.vcxproj':
            raise ProjectError('expected .vcxproj source in MSVC module')
        try:
            with open(build.cfg.native_path(obj.project), 'rb') as fp:
                doc = etree.parse(fp)
        except IOError:
            raise ConfigError('project not found')
        obj.uuid = get_uuid(doc)
        if obj.uuid is None:
            raise ConfigError('could not find project uuid')
        obj.configs = get_configs(doc)
        if not obj.configs:
            raise ConfigError('could not find project configurations')
        build.add(pname, obj)
        deps.append(pname)
    obj = source.SourceModule()
    obj.private_modules.update(deps)
    build.add(name, obj)

BUILDERS = {
    'msvc': build_msvc,
}
