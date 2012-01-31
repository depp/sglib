import os
import buildtool.path as path
import buildtool.shell as shell
import re

def objs(paths):
    return ' '.join(path.withext(path.sources(paths), '.o'))

def run(obj):
    base_src = obj.get_atoms(None, 'LINUX')
    gtk_src = obj.get_atoms('GTK')
    sdl_src = obj.get_atoms('SDL')
    libpng_src = obj.get_atoms('LIBPNG')
    libjpeg_src = obj.get_atoms('LIBJPEG')
    pango_src = obj.get_atoms('PANGO')

    all_src = (base_src + gtk_src + sdl_src +
               libpng_src + libjpeg_src + pango_src)

    subs = {
        'BASE_OBJS': objs(base_src),
        'GTK_OBJS': objs(gtk_src),
        'SDL_OBJS': objs(sdl_src),
        'LIBPNG_OBJS': objs(libpng_src),
        'LIBJPEG_OBJS': objs(libjpeg_src),
        'PANGO_OBJS': objs(pango_src),
        'C_OBJS': objs(path.c_sources(all_src)),
        'CXX_OBJS': objs(path.cxx_sources(all_src)),

        'CPPFLAGS': ' '.join(['-I' + p for p in obj.incldirs]),
        'CFLAGS': '',
        'CXXFLAGS': '',
        'LIBS': '',

        'SRCFILE': min(src for src in obj.all_sources()
                       if src != 'version.c'),
        'VERSION': obj.version,
    }

    def repl(m):
        what = m.group(1)
        try:
            return subs[what]
        except KeyError:
            pass
        try:
            return obj.env[what]
        except KeyError:
            pass
        return m.group(0)

    dirpath = os.path.dirname(__file__)
    files = [('Makefile', '#'),
             ('configure.ac', 'dnl')]
    for fname, cm in files:
        text = open(os.path.join(dirpath, fname), 'r').read()
        text = (cm + ' ' + obj.warning + '\n' +
                re.sub(r'@(\w+)@', repl, text))
        obj.write_file(fname, text)

    text = open(os.path.join(dirpath, 'config.mak.in'), 'r').read()
    obj.write_file('config.mak.in', text)

    shell.run(obj, ['aclocal'])
    shell.run(obj, ['autoheader'])
    shell.run(obj, ['autoconf'])
    shell.run(obj, ['./configure', '--enable-warnings=error'])
