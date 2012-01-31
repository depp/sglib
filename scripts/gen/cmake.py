import string

TEMPL = """\
# $WARNING
# Only Windows builds supported with CMake
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project($PROJNAME)
if(NOT DEFINED WIN32)
  message(FATAL_ERROR "Only Windows supported with CMake")
endif()
include_directories($INCDIRS)
add_executable($EXENAME WIN32 $SOURCES)
"""

def run(obj):
    t = string.Template(TEMPL).substitute(
        WARNING=obj.warning,
        PROJNAME=obj.env.PKG_FILENAME,
        EXENAME=obj.env.EXE_WINDOWS,
        INCDIRS=' '.join(obj.incldirs),
        SOURCES=' '.join(obj.get_atoms(None, 'WINDOWS')))
    obj.write_file('CMakeLists.txt', t)
