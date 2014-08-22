Portable C Extension library: LibPCE
====================================

This is a library of simple functions and macros that are not part of
the C standard library, but "should be".  It is similar to existing
projects such as APR and GLib.

Compared to APR and GLib:

* LibPCE is smaller.  It is much smaller than GLib, and also smaller
  than APR.  LibPCE does not provied abstractions for IO -- if the
  standard C library isn't good enough, you probably want to do it
  yourself.

* LibPCE has a more permissive license, the simplified BSD license.
  You can include LibPCE source code directly in your project, or
  statically linke LibPCE without worries.

Platform assumptions
--------------------

* char is 8 bits
* short is 16 bits
* int is 32 bits
* long long is 64 bits
* byte order is big or little endian
* integers are two's complement

These assumptions are true on a wide range of commodity architectures
available today.  We don't care about bizarre old architectures with
funny word lengths or mixed endian, or 16-bit architectures from the
early 1990s.  Likewise, there are no 128-bit architectures on the
horizon.

Library headers
---------------

* libpce/arch.h: Macros for determining CPU architecture, e.g.,
  defines PCE_CPU_X86 on x86 and x86_64 systems.

* libpce/attribute.h: Macros for function attributes, e.g.,
  PCE_ATTR_NORETURN for functions that don't return.

* libpce/byteorder.h: Macros for determining byte order.

* libpce/byteswap.h: Byte swapping functions.

* libpce/cpu.h: Functions and macros for CPU feature availability,
  e.g., determine at runtiime if MMX is supported.

* libpce/strbuf.h: String buffers, for constructing strings without
  worrying about allocation.

Planned additions
-----------------

* Atomic operations.

* Synchronization primitives: locks, read/write locks, events,
  semaphores, something like that.

* Hash function and hash table.

* Pseudorandom numbers.

* String formatting functions, using modern "Hello, {0}" notation
  instead of printf format strings.

* Simple Unicode functions: minimally, conversion between UTF
  encodings.

* Base 64, base 16 encoding / decoding.
