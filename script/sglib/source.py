# Copyright 2014 Dietrich Epp.
# This file is part of SGLib.  SGLib is licensed under the terms of the
# 2-clause BSD license.  For more information, see LICENSE.txt.
from d3build.source import SourceList

src = SourceList(base=__file__, path='../..')

src.add(path='include/sg', sources='''
arch.h
atomic.h
attribute.h
audio_buffer.h
audio_file.h
binary.h
byteorder.h
clock.h
cpu.h
cvar.h
defs.h
entry.h
error.h
event.h
file.h
hash.h
hashtable.h
key.h
keycode.h
log.h
mixer.h
net.h
opengl.h
pack.h
pixbuf.h
rand.h
record.h
strbuf.h
thread.h
type.h
util.h
version.h
''')

src.add('include/config.h')

src.add(path='src/audio', sources='''
buffer.c
convert.c
file.c
ogg.c ogg
ogg.h ogg
opus.c ogg opus
resample.c
vorbis.c ogg vorbis
wav.c
writer.c
''')

src.add(path='src/core', sources='''
clock.c
clock_impl.h
cvar.c
cvartable.c
cvartable.h
error.c
file_impl.h
file_posix.c posix
file_read.c
file_win.c windows
keyid.c
keytable_evdev.c linux
keytable_mac.c osx
keytable_win.c windows
log.c
log_console.c
log_impl.h
log_network.c
logtest.c exclude
net.c
opengl.c
pack.c
path.c
path_norm.c
path_posix.c posix
path_win.c windows
private.h
rand.c
sys.c
version.c
version_const.c
winutf8.c windows
''')

src.add(path='src/mixer', sources='''
channel.c
mixdown.c
mixer.c
mixer.h
queue.c
sound.c
sound.h
time.c
time.h
timeexact.c
system_alsa.c audio_alsa
system_coreaudio.c audio_coreaudio
system_directsound8.c audio_directsound
system_sdl.c audio_sdl
''')

src.add(path='src/pixbuf', sources='''
coregraphics.c image_coregraphics
image.c
libjpeg.c image_libjpeg
libpng.c image_libpng
pixbuf.c
premultiply_alpha.c
private.h
texture.c
wincodec.c image_wincodec
''')

src.add(path='src/record', tags=['video_recording'], sources='''
cmdargs.c
cmdargs.h
record.c !video_recording
record.h !video_recording
screenshot.c !video_recording
screenshot.h !video_recording
videoio.c
videoio.h
videoparam.c
videoparam.h
videoproc.c
videoproc.h
videotime.c
videotime.h
''')

src.add(path='src/type', tags=['freetype'], sources='''
font.c
freetype_error.c
private.h
textflow.c
textlayout.c
typeface.c
''')

src.add(path='src/util', sources='''
cpu.c
hash.c
hashtable.c
strbuf.c
thread_pthread.c posix
thread_windows.c windows
''')

src.add(path='src/core/osx', tags=['frontend_cocoa'], sources='''
GApplication.h
GApplication.m
GController.h
GController.m
GDisplay.h
GDisplay.m
GMain.m
GPrefix.h
GView.h
GView.m
GWindow.h
GWindow.m
''')

src.add(path='src/core/gtk', tags=['frontend_gtk'], sources='''
gtk.c
''')

src.add(path='src/core/sdl', tags=['frontend_sdl'], sources='''
sdl.c
''')

src.add(path='src/core/windows', tags=['frontend_windows'], sources='''
windows.c
''')

src = src.sources
