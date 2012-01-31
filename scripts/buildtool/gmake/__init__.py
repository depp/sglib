import os
import buildtool.path as path
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

        'EXENAME': obj.env.EXE_LINUX,
    }

    def repl(m):
        what = m.group(1)
        try:
            return subs[what]
        except KeyError:
            return m.group(0)

    dirpath = os.path.dirname(__file__)
    files = [('Makefile', '#'),
             ('config.mak.in', '#'),
             ('configure.ac', 'dnl')]
    for fname, cm in files:
        text = open(os.path.join(dirpath, fname), 'r').read()
        text = (cm + ' ' + obj.warning + '\n' +
                re.sub(r'@(\w+)@', repl, text))
        obj.write_file(fname, text)
