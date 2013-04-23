"""Build objects.

The project data (from the 'data' module), which is generic, is
converted into specific build objects, which are then emitted by the
backend.
"""
import inspect
import uuid

class Object(object):
    """Abstract base class for targets, modules, and generated sources."""
    __slots__ = []

    def __init__(self, **kw):
        for class_ in inspect.getmro(self.__class__):
            try:
                slots = class_.__slots__
            except AttributeError:
                continue
            for slot in slots:
                try:
                    val = kw[slot]
                except KeyError:
                    raise TypeError(
                        'required argument missing: {}'.format(slot))
                setattr(self, slot, val)

class Target(Object):
    __slots__ = ['uuid']

    def get_uuid(self, cfg):
        if self.uuid is None:
            self.uuid = uuid.uuid4()
            cfg.warn('target has no UUID, generated: {}'.format(self.uuid))
        return self.uuid

    @classmethod
    def build(class_, build, mod, name, external):
        if name is not None:
            build.cfg.warn('target has name: {}'.format(name))
        build.add_target(class_.parse(build, mod, external))

class GeneratedSource(Object):
    __slots__ = ['target']
    deps = ()
    is_regenerated = True
    is_regenerated_always = False
    is_regenerated_only = False
    HEADER = 'This file automatically generated by the build system.'

    @classmethod
    def build(class_, build, mod, name, external):
        if name is not None:
            build.cfg.warn(
                'generated source has name: {}'.format(name))
        build.add_generated_source(class_.parse(build, mod, external))
