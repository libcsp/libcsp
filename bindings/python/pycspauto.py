'''Wrapper for csp.h

Generated with:
/usr/local/bin/ctypesgen.py --all-headers -I../../build/include -I../../include ../../include/csp/csp.h ../../include/csp/drivers/i2c.h ../../include/csp/drivers/usart.h ../../include/csp/interfaces/csp_if_can.h ../../include/csp/interfaces/csp_if_i2c.h ../../include/csp/interfaces/csp_if_kiss.h ../../include/csp/interfaces/csp_if_lo.h ../../include/csp/interfaces/csp_if_zmqhub.h -lcsp -o pycspauto.py

Do not modify this file.
'''

__docformat__ =  'restructuredtext'

# Begin preamble

import ctypes, os, sys
from ctypes import *

_int_types = (c_int16, c_int32)
if hasattr(ctypes, 'c_int64'):
    # Some builds of ctypes apparently do not have c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (c_int64,)
for t in _int_types:
    if sizeof(t) == sizeof(c_size_t):
        c_ptrdiff_t = t
del t
del _int_types

class c_void(Structure):
    # c_void_p is a buggy return type, converting to int, so
    # POINTER(None) == c_void_p is actually written as
    # POINTER(c_void), so it can be treated as a real pointer.
    _fields_ = [('dummy', c_int)]

def POINTER(obj):
    p = ctypes.POINTER(obj)

    # Convert None to a real NULL pointer to work around bugs
    # in how ctypes handles None on 64-bit platforms
    if not isinstance(p.from_param, classmethod):
        def from_param(cls, x):
            if x is None:
                return cls()
            else:
                return x
        p.from_param = classmethod(from_param)

    return p

class UserString:
    def __init__(self, seq):
        if isinstance(seq, basestring):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq)
    def __str__(self): return str(self.data)
    def __repr__(self): return repr(self.data)
    def __int__(self): return int(self.data)
    def __long__(self): return long(self.data)
    def __float__(self): return float(self.data)
    def __complex__(self): return complex(self.data)
    def __hash__(self): return hash(self.data)

    def __cmp__(self, string):
        if isinstance(string, UserString):
            return cmp(self.data, string.data)
        else:
            return cmp(self.data, string)
    def __contains__(self, char):
        return char in self.data

    def __len__(self): return len(self.data)
    def __getitem__(self, index): return self.__class__(self.data[index])
    def __getslice__(self, start, end):
        start = max(start, 0); end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, basestring):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other))
    def __radd__(self, other):
        if isinstance(other, basestring):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other) + self.data)
    def __mul__(self, n):
        return self.__class__(self.data*n)
    __rmul__ = __mul__
    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self): return self.__class__(self.data.capitalize())
    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))
    def count(self, sub, start=0, end=sys.maxint):
        return self.data.count(sub, start, end)
    def decode(self, encoding=None, errors=None): # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())
    def encode(self, encoding=None, errors=None): # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())
    def endswith(self, suffix, start=0, end=sys.maxint):
        return self.data.endswith(suffix, start, end)
    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))
    def find(self, sub, start=0, end=sys.maxint):
        return self.data.find(sub, start, end)
    def index(self, sub, start=0, end=sys.maxint):
        return self.data.index(sub, start, end)
    def isalpha(self): return self.data.isalpha()
    def isalnum(self): return self.data.isalnum()
    def isdecimal(self): return self.data.isdecimal()
    def isdigit(self): return self.data.isdigit()
    def islower(self): return self.data.islower()
    def isnumeric(self): return self.data.isnumeric()
    def isspace(self): return self.data.isspace()
    def istitle(self): return self.data.istitle()
    def isupper(self): return self.data.isupper()
    def join(self, seq): return self.data.join(seq)
    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))
    def lower(self): return self.__class__(self.data.lower())
    def lstrip(self, chars=None): return self.__class__(self.data.lstrip(chars))
    def partition(self, sep):
        return self.data.partition(sep)
    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))
    def rfind(self, sub, start=0, end=sys.maxint):
        return self.data.rfind(sub, start, end)
    def rindex(self, sub, start=0, end=sys.maxint):
        return self.data.rindex(sub, start, end)
    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))
    def rpartition(self, sep):
        return self.data.rpartition(sep)
    def rstrip(self, chars=None): return self.__class__(self.data.rstrip(chars))
    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)
    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)
    def splitlines(self, keepends=0): return self.data.splitlines(keepends)
    def startswith(self, prefix, start=0, end=sys.maxint):
        return self.data.startswith(prefix, start, end)
    def strip(self, chars=None): return self.__class__(self.data.strip(chars))
    def swapcase(self): return self.__class__(self.data.swapcase())
    def title(self): return self.__class__(self.data.title())
    def translate(self, *args):
        return self.__class__(self.data.translate(*args))
    def upper(self): return self.__class__(self.data.upper())
    def zfill(self, width): return self.__class__(self.data.zfill(width))

class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""
    def __init__(self, string=""):
        self.data = string
    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")
    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data): raise IndexError
        self.data = self.data[:index] + sub + self.data[index+1:]
    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data): raise IndexError
        self.data = self.data[:index] + self.data[index+1:]
    def __setslice__(self, start, end, sub):
        start = max(start, 0); end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start]+sub.data+self.data[end:]
        elif isinstance(sub, basestring):
            self.data = self.data[:start]+sub+self.data[end:]
        else:
            self.data =  self.data[:start]+str(sub)+self.data[end:]
    def __delslice__(self, start, end):
        start = max(start, 0); end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]
    def immutable(self):
        return UserString(self.data)
    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, basestring):
            self.data += other
        else:
            self.data += str(other)
        return self
    def __imul__(self, n):
        self.data *= n
        return self

class String(MutableString, Union):

    _fields_ = [('raw', POINTER(c_char)),
                ('data', c_char_p)]

    def __init__(self, obj=""):
        if isinstance(obj, (str, unicode, UserString)):
            self.data = str(obj)
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(POINTER(c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj)

        # Convert from c_char_p
        elif isinstance(obj, c_char_p):
            return obj

        # Convert from POINTER(c_char)
        elif isinstance(obj, POINTER(c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(cast(obj, POINTER(c_char)))

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)
    from_param = classmethod(from_param)

def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)

# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to c_void_p.
def UNCHECKED(type):
    if (hasattr(type, "_type_") and isinstance(type._type_, str)
        and type._type_ != "P"):
        return type
    else:
        return c_void_p

# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self,func,restype,argtypes):
        self.func=func
        self.func.restype=restype
        self.argtypes=argtypes
    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func
    def __call__(self,*args):
        fixed_args=[]
        i=0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i+=1
        return self.func(*fixed_args+list(args[i:]))

# End preamble

_libs = {}
_libdirs = []

# Begin loader

# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

import os.path, re, sys, glob
import platform
import ctypes
import ctypes.util

def _environ_path(name):
    if name in os.environ:
        return os.environ[name].split(":")
    else:
        return []

class LibraryLoader(object):
    def __init__(self):
        self.other_dirs=[]

    def load_library(self,libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            if os.path.exists(path):
                return self.load(path)

        raise ImportError("%s not found." % libname)

    def load(self,path):
        """Given a path to a library, load it."""
        try:
            # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
            # of the default RTLD_LOCAL.  Without this, you end up with
            # libraries not being loadable, resulting in "Symbol not found"
            # errors
            if sys.platform == 'darwin':
                return ctypes.CDLL(path, ctypes.RTLD_GLOBAL)
            else:
                return ctypes.cdll.LoadLibrary(path)
        except OSError,e:
            raise ImportError(e)

    def getpaths(self,libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # FIXME / TODO return '.' and os.path.dirname(__file__)
            for path in self.getplatformpaths(libname):
                yield path

            path = ctypes.util.find_library(libname)
            if path: yield path

    def getplatformpaths(self, libname):
        return []

# Darwin (Mac OS X)

class DarwinLibraryLoader(LibraryLoader):
    name_formats = ["lib%s.dylib", "lib%s.so", "lib%s.bundle", "%s.dylib",
                "%s.so", "%s.bundle", "%s"]

    def getplatformpaths(self,libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [format % libname for format in self.name_formats]

        for dir in self.getdirs(libname):
            for name in names:
                yield os.path.join(dir,name)

    def getdirs(self,libname):
        '''Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        '''

        dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [os.path.expanduser('~/lib'),
                                          '/usr/local/lib', '/usr/lib']

        dirs = []

        if '/' in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))

        dirs.extend(self.other_dirs)
        dirs.append(".")
        dirs.append(os.path.dirname(__file__))

        if hasattr(sys, 'frozen') and sys.frozen == 'macosx_app':
            dirs.append(os.path.join(
                os.environ['RESOURCEPATH'],
                '..',
                'Frameworks'))

        dirs.extend(dyld_fallback_library_path)

        return dirs

# Posix

class PosixLibraryLoader(LibraryLoader):
    _ld_so_cache = None

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = []
        for name in ("LD_LIBRARY_PATH",
                     "SHLIB_PATH", # HPUX
                     "LIBPATH", # OS/2, AIX
                     "LIBRARY_PATH", # BE/OS
                    ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))
        directories.extend(self.other_dirs)
        directories.append(".")
        directories.append(os.path.dirname(__file__))

        try: directories.extend([dir.strip() for dir in open('/etc/ld.so.conf')])
        except IOError: pass

        unix_lib_dirs_list = ['/lib', '/usr/lib', '/lib64', '/usr/lib64']
        if sys.platform.startswith('linux'):
            # Try and support multiarch work in Ubuntu
            # https://wiki.ubuntu.com/MultiarchSpec
            bitage = platform.architecture()[0]
            if bitage.startswith('32'):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ['/lib/i386-linux-gnu', '/usr/lib/i386-linux-gnu']
            elif bitage.startswith('64'):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ['/lib/x86_64-linux-gnu', '/usr/lib/x86_64-linux-gnu']
            else:
                # guess...
                unix_lib_dirs_list += glob.glob('/lib/*linux-gnu')
        directories.extend(unix_lib_dirs_list)

        cache = {}
        lib_re = re.compile(r'lib(.*)\.s[ol]')
        ext_re = re.compile(r'\.s[ol]$')
        for dir in directories:
            try:
                for path in glob.glob("%s/*.s[ol]*" % dir):
                    file = os.path.basename(path)

                    # Index by filename
                    if file not in cache:
                        cache[file] = path

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        if library not in cache:
                            cache[library] = path
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname)
        if result: yield result

        path = ctypes.util.find_library(libname)
        if path: yield os.path.join("/lib",path)

# Windows

class _WindowsLibrary(object):
    def __init__(self, path):
        self.cdll = ctypes.cdll.LoadLibrary(path)
        self.windll = ctypes.windll.LoadLibrary(path)

    def __getattr__(self, name):
        try: return getattr(self.cdll,name)
        except AttributeError:
            try: return getattr(self.windll,name)
            except AttributeError:
                raise

class WindowsLibraryLoader(LibraryLoader):
    name_formats = ["%s.dll", "lib%s.dll", "%slib.dll"]

    def load_library(self, libname):
        try:
            result = LibraryLoader.load_library(self, libname)
        except ImportError:
            result = None
            if os.path.sep not in libname:
                for name in self.name_formats:
                    try:
                        result = getattr(ctypes.cdll, name % libname)
                        if result:
                            break
                    except WindowsError:
                        result = None
            if result is None:
                try:
                    result = getattr(ctypes.cdll, libname)
                except WindowsError:
                    result = None
            if result is None:
                raise ImportError("%s not found." % libname)
        return result

    def load(self, path):
        return _WindowsLibrary(path)

    def getplatformpaths(self, libname):
        if os.path.sep not in libname:
            for name in self.name_formats:
                dll_in_current_dir = os.path.abspath(name % libname)
                if os.path.exists(dll_in_current_dir):
                    yield dll_in_current_dir
                path = ctypes.util.find_library(name % libname)
                if path:
                    yield path

# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin":   DarwinLibraryLoader,
    "cygwin":   WindowsLibraryLoader,
    "win32":    WindowsLibraryLoader
}

loader = loaderclass.get(sys.platform, PosixLibraryLoader)()

def add_library_search_dirs(other_dirs):
    loader.other_dirs = other_dirs

load_library = loader.load_library

del loaderclass

# End loader

add_library_search_dirs([])

# Begin libraries

_libs["csp"] = load_library("csp")

# 1 libraries
# End libraries

# No modules

int8_t = c_char # /usr/include/stdint.h: 36

int16_t = c_int # /usr/include/stdint.h: 37

int32_t = c_int # /usr/include/stdint.h: 38

int64_t = c_long # /usr/include/stdint.h: 40

uint8_t = c_ubyte # /usr/include/stdint.h: 48

uint16_t = c_uint # /usr/include/stdint.h: 49

uint32_t = c_uint # /usr/include/stdint.h: 51

uint64_t = c_ulong # /usr/include/stdint.h: 55

int_least8_t = c_char # /usr/include/stdint.h: 65

int_least16_t = c_int # /usr/include/stdint.h: 66

int_least32_t = c_int # /usr/include/stdint.h: 67

int_least64_t = c_long # /usr/include/stdint.h: 69

uint_least8_t = c_ubyte # /usr/include/stdint.h: 76

uint_least16_t = c_uint # /usr/include/stdint.h: 77

uint_least32_t = c_uint # /usr/include/stdint.h: 78

uint_least64_t = c_ulong # /usr/include/stdint.h: 80

int_fast8_t = c_char # /usr/include/stdint.h: 90

int_fast16_t = c_long # /usr/include/stdint.h: 92

int_fast32_t = c_long # /usr/include/stdint.h: 93

int_fast64_t = c_long # /usr/include/stdint.h: 94

uint_fast8_t = c_ubyte # /usr/include/stdint.h: 103

uint_fast16_t = c_ulong # /usr/include/stdint.h: 105

uint_fast32_t = c_ulong # /usr/include/stdint.h: 106

uint_fast64_t = c_ulong # /usr/include/stdint.h: 107

intptr_t = c_long # /usr/include/stdint.h: 119

uintptr_t = c_ulong # /usr/include/stdint.h: 122

intmax_t = c_long # /usr/include/stdint.h: 134

uintmax_t = c_ulong # /usr/include/stdint.h: 135

enum_csp_reserved_ports_e = c_int # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

CSP_CMP = 0 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

CSP_PING = 1 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

CSP_PS = 2 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

CSP_MEMFREE = 3 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

CSP_REBOOT = 4 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

CSP_BUF_FREE = 5 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

CSP_UPTIME = 6 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

CSP_ANY = (31 + 1) # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

CSP_PROMISC = (31 + 2) # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 39

enum_anon_1 = c_int # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 56

CSP_PRIO_CRITICAL = 0 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 56

CSP_PRIO_HIGH = 1 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 56

CSP_PRIO_NORM = 2 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 56

CSP_PRIO_LOW = 3 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 56

csp_prio_t = enum_anon_1 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 56

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 102
class struct_anon_2(Structure):
    _pack_ = 1
    pass

struct_anon_2.__slots__ = [
    'flags',
    'sport',
    'dport',
    'dst',
    'src',
    'pri',
]
struct_anon_2._fields_ = [
    ('flags', c_uint, 8),
    ('sport', c_uint, 6),
    ('dport', c_uint, 6),
    ('dst', c_uint, 5),
    ('src', c_uint, 5),
    ('pri', c_uint, 2),
]

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 121
class union_anon_3(Union):
    pass

union_anon_3.__slots__ = [
    'ext',
    'unnamed_1',
]
union_anon_3._anonymous_ = [
    'unnamed_1',
]
union_anon_3._fields_ = [
    ('ext', c_uint32),
    ('unnamed_1', struct_anon_2),
]

csp_id_t = union_anon_3 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 121

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 172
class union_anon_4(Union):
    pass

union_anon_4.__slots__ = [
    'data',
    'data16',
    'data32',
]
union_anon_4._fields_ = [
    ('data', c_uint8 * 1024),
    ('data16', c_uint16 * 0),
    ('data32', c_uint32 * 0),
]

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 177
class struct_anon_5(Structure):
    _pack_ = 1
    pass

struct_anon_5.__slots__ = [
    'padding',
    'length',
    'id',
    'unnamed_1',
]
struct_anon_5._anonymous_ = [
    'unnamed_1',
]
struct_anon_5._fields_ = [
    ('padding', c_uint8 * 8),
    ('length', c_uint16),
    ('id', csp_id_t),
    ('unnamed_1', union_anon_4),
]

csp_packet_t = struct_anon_5 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 177

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 184
class struct_csp_iface_s(Structure):
    pass

nexthop_t = CFUNCTYPE(UNCHECKED(c_int), POINTER(struct_csp_iface_s), POINTER(csp_packet_t), c_uint32) # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 181

struct_csp_iface_s.__slots__ = [
    'name',
    'driver',
    'nexthop',
    'promisc',
    'mtu',
    'split_horizon_off',
    'tx',
    'rx',
    'tx_error',
    'rx_error',
    'drop',
    'autherr',
    'frame',
    'txbytes',
    'rxbytes',
    'irq',
    'next',
]
struct_csp_iface_s._fields_ = [
    ('name', String),
    ('driver', POINTER(None)),
    ('nexthop', nexthop_t),
    ('promisc', c_uint8),
    ('mtu', c_uint16),
    ('split_horizon_off', c_uint8),
    ('tx', c_uint32),
    ('rx', c_uint32),
    ('tx_error', c_uint32),
    ('rx_error', c_uint32),
    ('drop', c_uint32),
    ('autherr', c_uint32),
    ('frame', c_uint32),
    ('txbytes', c_uint32),
    ('rxbytes', c_uint32),
    ('irq', c_uint32),
    ('next', POINTER(struct_csp_iface_s)),
]

csp_iface_t = struct_csp_iface_s # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 202

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 212
class struct_csp_conn_s(Structure):
    pass

csp_socket_t = struct_csp_conn_s # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 212

csp_conn_t = struct_csp_conn_s # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 213

__gwchar_t = c_int # /usr/include/inttypes.h: 34

# /usr/include/inttypes.h: 275
class struct_anon_6(Structure):
    pass

struct_anon_6.__slots__ = [
    'quot',
    'rem',
]
struct_anon_6._fields_ = [
    ('quot', c_long),
    ('rem', c_long),
]

imaxdiv_t = struct_anon_6 # /usr/include/inttypes.h: 275

# /usr/include/inttypes.h: 290
if hasattr(_libs['csp'], 'imaxabs'):
    imaxabs = _libs['csp'].imaxabs
    imaxabs.argtypes = [intmax_t]
    imaxabs.restype = intmax_t

# /usr/include/inttypes.h: 293
if hasattr(_libs['csp'], 'imaxdiv'):
    imaxdiv = _libs['csp'].imaxdiv
    imaxdiv.argtypes = [intmax_t, intmax_t]
    imaxdiv.restype = imaxdiv_t

# /usr/include/inttypes.h: 297
if hasattr(_libs['csp'], 'strtoimax'):
    strtoimax = _libs['csp'].strtoimax
    strtoimax.argtypes = [String, POINTER(POINTER(c_char)), c_int]
    strtoimax.restype = intmax_t

# /usr/include/inttypes.h: 301
if hasattr(_libs['csp'], 'strtoumax'):
    strtoumax = _libs['csp'].strtoumax
    strtoumax.argtypes = [String, POINTER(POINTER(c_char)), c_int]
    strtoumax.restype = uintmax_t

# /usr/include/inttypes.h: 305
if hasattr(_libs['csp'], 'wcstoimax'):
    wcstoimax = _libs['csp'].wcstoimax
    wcstoimax.argtypes = [POINTER(__gwchar_t), POINTER(POINTER(__gwchar_t)), c_int]
    wcstoimax.restype = intmax_t

# /usr/include/inttypes.h: 310
if hasattr(_libs['csp'], 'wcstoumax'):
    wcstoumax = _libs['csp'].wcstoumax
    wcstoumax.argtypes = [POINTER(__gwchar_t), POINTER(POINTER(__gwchar_t)), c_int]
    wcstoumax.restype = uintmax_t

# /usr/include/string.h: 46
if hasattr(_libs['csp'], 'memcpy'):
    memcpy = _libs['csp'].memcpy
    memcpy.argtypes = [POINTER(None), POINTER(None), c_size_t]
    memcpy.restype = POINTER(None)

# /usr/include/string.h: 50
if hasattr(_libs['csp'], 'memmove'):
    memmove = _libs['csp'].memmove
    memmove.argtypes = [POINTER(None), POINTER(None), c_size_t]
    memmove.restype = POINTER(None)

# /usr/include/string.h: 58
if hasattr(_libs['csp'], 'memccpy'):
    memccpy = _libs['csp'].memccpy
    memccpy.argtypes = [POINTER(None), POINTER(None), c_int, c_size_t]
    memccpy.restype = POINTER(None)

# /usr/include/string.h: 66
if hasattr(_libs['csp'], 'memset'):
    memset = _libs['csp'].memset
    memset.argtypes = [POINTER(None), c_int, c_size_t]
    memset.restype = POINTER(None)

# /usr/include/string.h: 69
if hasattr(_libs['csp'], 'memcmp'):
    memcmp = _libs['csp'].memcmp
    memcmp.argtypes = [POINTER(None), POINTER(None), c_size_t]
    memcmp.restype = c_int

# /usr/include/string.h: 96
if hasattr(_libs['csp'], 'memchr'):
    memchr = _libs['csp'].memchr
    memchr.argtypes = [POINTER(None), c_int, c_size_t]
    memchr.restype = POINTER(None)

# /usr/include/string.h: 129
if hasattr(_libs['csp'], 'strcpy'):
    strcpy = _libs['csp'].strcpy
    strcpy.argtypes = [String, String]
    if sizeof(c_int) == sizeof(c_void_p):
        strcpy.restype = ReturnString
    else:
        strcpy.restype = String
        strcpy.errcheck = ReturnString

# /usr/include/string.h: 132
if hasattr(_libs['csp'], 'strncpy'):
    strncpy = _libs['csp'].strncpy
    strncpy.argtypes = [String, String, c_size_t]
    if sizeof(c_int) == sizeof(c_void_p):
        strncpy.restype = ReturnString
    else:
        strncpy.restype = String
        strncpy.errcheck = ReturnString

# /usr/include/string.h: 137
if hasattr(_libs['csp'], 'strcat'):
    strcat = _libs['csp'].strcat
    strcat.argtypes = [String, String]
    if sizeof(c_int) == sizeof(c_void_p):
        strcat.restype = ReturnString
    else:
        strcat.restype = String
        strcat.errcheck = ReturnString

# /usr/include/string.h: 140
if hasattr(_libs['csp'], 'strncat'):
    strncat = _libs['csp'].strncat
    strncat.argtypes = [String, String, c_size_t]
    if sizeof(c_int) == sizeof(c_void_p):
        strncat.restype = ReturnString
    else:
        strncat.restype = String
        strncat.errcheck = ReturnString

# /usr/include/string.h: 144
if hasattr(_libs['csp'], 'strcmp'):
    strcmp = _libs['csp'].strcmp
    strcmp.argtypes = [String, String]
    strcmp.restype = c_int

# /usr/include/string.h: 147
if hasattr(_libs['csp'], 'strncmp'):
    strncmp = _libs['csp'].strncmp
    strncmp.argtypes = [String, String, c_size_t]
    strncmp.restype = c_int

# /usr/include/string.h: 151
if hasattr(_libs['csp'], 'strcoll'):
    strcoll = _libs['csp'].strcoll
    strcoll.argtypes = [String, String]
    strcoll.restype = c_int

# /usr/include/string.h: 154
if hasattr(_libs['csp'], 'strxfrm'):
    strxfrm = _libs['csp'].strxfrm
    strxfrm.argtypes = [String, String, c_size_t]
    strxfrm.restype = c_size_t

# /usr/include/xlocale.h: 30
class struct___locale_data(Structure):
    pass

# /usr/include/xlocale.h: 27
class struct___locale_struct(Structure):
    pass

struct___locale_struct.__slots__ = [
    '__locales',
    '__ctype_b',
    '__ctype_tolower',
    '__ctype_toupper',
    '__names',
]
struct___locale_struct._fields_ = [
    ('__locales', POINTER(struct___locale_data) * 13),
    ('__ctype_b', POINTER(c_uint)),
    ('__ctype_tolower', POINTER(c_int)),
    ('__ctype_toupper', POINTER(c_int)),
    ('__names', POINTER(c_char) * 13),
]

__locale_t = POINTER(struct___locale_struct) # /usr/include/xlocale.h: 39

locale_t = __locale_t # /usr/include/xlocale.h: 42

# /usr/include/string.h: 166
if hasattr(_libs['csp'], 'strcoll_l'):
    strcoll_l = _libs['csp'].strcoll_l
    strcoll_l.argtypes = [String, String, __locale_t]
    strcoll_l.restype = c_int

# /usr/include/string.h: 169
if hasattr(_libs['csp'], 'strxfrm_l'):
    strxfrm_l = _libs['csp'].strxfrm_l
    strxfrm_l.argtypes = [String, String, c_size_t, __locale_t]
    strxfrm_l.restype = c_size_t

# /usr/include/string.h: 176
if hasattr(_libs['csp'], 'strdup'):
    strdup = _libs['csp'].strdup
    strdup.argtypes = [String]
    if sizeof(c_int) == sizeof(c_void_p):
        strdup.restype = ReturnString
    else:
        strdup.restype = String
        strdup.errcheck = ReturnString

# /usr/include/string.h: 184
if hasattr(_libs['csp'], 'strndup'):
    strndup = _libs['csp'].strndup
    strndup.argtypes = [String, c_size_t]
    if sizeof(c_int) == sizeof(c_void_p):
        strndup.restype = ReturnString
    else:
        strndup.restype = String
        strndup.errcheck = ReturnString

# /usr/include/string.h: 236
if hasattr(_libs['csp'], 'strchr'):
    strchr = _libs['csp'].strchr
    strchr.argtypes = [String, c_int]
    if sizeof(c_int) == sizeof(c_void_p):
        strchr.restype = ReturnString
    else:
        strchr.restype = String
        strchr.errcheck = ReturnString

# /usr/include/string.h: 263
if hasattr(_libs['csp'], 'strrchr'):
    strrchr = _libs['csp'].strrchr
    strrchr.argtypes = [String, c_int]
    if sizeof(c_int) == sizeof(c_void_p):
        strrchr.restype = ReturnString
    else:
        strrchr.restype = String
        strrchr.errcheck = ReturnString

# /usr/include/string.h: 285
if hasattr(_libs['csp'], 'strcspn'):
    strcspn = _libs['csp'].strcspn
    strcspn.argtypes = [String, String]
    strcspn.restype = c_size_t

# /usr/include/string.h: 289
if hasattr(_libs['csp'], 'strspn'):
    strspn = _libs['csp'].strspn
    strspn.argtypes = [String, String]
    strspn.restype = c_size_t

# /usr/include/string.h: 315
if hasattr(_libs['csp'], 'strpbrk'):
    strpbrk = _libs['csp'].strpbrk
    strpbrk.argtypes = [String, String]
    if sizeof(c_int) == sizeof(c_void_p):
        strpbrk.restype = ReturnString
    else:
        strpbrk.restype = String
        strpbrk.errcheck = ReturnString

# /usr/include/string.h: 342
if hasattr(_libs['csp'], 'strstr'):
    strstr = _libs['csp'].strstr
    strstr.argtypes = [String, String]
    if sizeof(c_int) == sizeof(c_void_p):
        strstr.restype = ReturnString
    else:
        strstr.restype = String
        strstr.errcheck = ReturnString

# /usr/include/string.h: 348
if hasattr(_libs['csp'], 'strtok'):
    strtok = _libs['csp'].strtok
    strtok.argtypes = [String, String]
    if sizeof(c_int) == sizeof(c_void_p):
        strtok.restype = ReturnString
    else:
        strtok.restype = String
        strtok.errcheck = ReturnString

# /usr/include/string.h: 354
if hasattr(_libs['csp'], '__strtok_r'):
    __strtok_r = _libs['csp'].__strtok_r
    __strtok_r.argtypes = [String, String, POINTER(POINTER(c_char))]
    if sizeof(c_int) == sizeof(c_void_p):
        __strtok_r.restype = ReturnString
    else:
        __strtok_r.restype = String
        __strtok_r.errcheck = ReturnString

# /usr/include/string.h: 359
if hasattr(_libs['csp'], 'strtok_r'):
    strtok_r = _libs['csp'].strtok_r
    strtok_r.argtypes = [String, String, POINTER(POINTER(c_char))]
    if sizeof(c_int) == sizeof(c_void_p):
        strtok_r.restype = ReturnString
    else:
        strtok_r.restype = String
        strtok_r.errcheck = ReturnString

# /usr/include/string.h: 399
if hasattr(_libs['csp'], 'strlen'):
    strlen = _libs['csp'].strlen
    strlen.argtypes = [String]
    strlen.restype = c_size_t

# /usr/include/string.h: 406
if hasattr(_libs['csp'], 'strnlen'):
    strnlen = _libs['csp'].strnlen
    strnlen.argtypes = [String, c_size_t]
    strnlen.restype = c_size_t

# /usr/include/string.h: 413
if hasattr(_libs['csp'], 'strerror'):
    strerror = _libs['csp'].strerror
    strerror.argtypes = [c_int]
    if sizeof(c_int) == sizeof(c_void_p):
        strerror.restype = ReturnString
    else:
        strerror.restype = String
        strerror.errcheck = ReturnString

# /usr/include/string.h: 431
if hasattr(_libs['csp'], '__xpg_strerror_r'):
    __xpg_strerror_r = _libs['csp'].__xpg_strerror_r
    __xpg_strerror_r.argtypes = [c_int, String, c_size_t]
    __xpg_strerror_r.restype = c_int

# /usr/include/string.h: 445
if hasattr(_libs['csp'], 'strerror_l'):
    strerror_l = _libs['csp'].strerror_l
    strerror_l.argtypes = [c_int, __locale_t]
    if sizeof(c_int) == sizeof(c_void_p):
        strerror_l.restype = ReturnString
    else:
        strerror_l.restype = String
        strerror_l.errcheck = ReturnString

# /usr/include/string.h: 451
if hasattr(_libs['csp'], '__bzero'):
    __bzero = _libs['csp'].__bzero
    __bzero.argtypes = [POINTER(None), c_size_t]
    __bzero.restype = None

# /usr/include/string.h: 455
if hasattr(_libs['csp'], 'bcopy'):
    bcopy = _libs['csp'].bcopy
    bcopy.argtypes = [POINTER(None), POINTER(None), c_size_t]
    bcopy.restype = None

# /usr/include/string.h: 459
if hasattr(_libs['csp'], 'bzero'):
    bzero = _libs['csp'].bzero
    bzero.argtypes = [POINTER(None), c_size_t]
    bzero.restype = None

# /usr/include/string.h: 462
if hasattr(_libs['csp'], 'bcmp'):
    bcmp = _libs['csp'].bcmp
    bcmp.argtypes = [POINTER(None), POINTER(None), c_size_t]
    bcmp.restype = c_int

# /usr/include/string.h: 489
if hasattr(_libs['csp'], 'index'):
    index = _libs['csp'].index
    index.argtypes = [String, c_int]
    if sizeof(c_int) == sizeof(c_void_p):
        index.restype = ReturnString
    else:
        index.restype = String
        index.errcheck = ReturnString

# /usr/include/string.h: 517
if hasattr(_libs['csp'], 'rindex'):
    rindex = _libs['csp'].rindex
    rindex.argtypes = [String, c_int]
    if sizeof(c_int) == sizeof(c_void_p):
        rindex.restype = ReturnString
    else:
        rindex.restype = String
        rindex.errcheck = ReturnString

# /usr/include/string.h: 523
if hasattr(_libs['csp'], 'ffs'):
    ffs = _libs['csp'].ffs
    ffs.argtypes = [c_int]
    ffs.restype = c_int

# /usr/include/string.h: 534
if hasattr(_libs['csp'], 'strcasecmp'):
    strcasecmp = _libs['csp'].strcasecmp
    strcasecmp.argtypes = [String, String]
    strcasecmp.restype = c_int

# /usr/include/string.h: 538
if hasattr(_libs['csp'], 'strncasecmp'):
    strncasecmp = _libs['csp'].strncasecmp
    strncasecmp.argtypes = [String, String, c_size_t]
    strncasecmp.restype = c_int

# /usr/include/string.h: 557
if hasattr(_libs['csp'], 'strsep'):
    strsep = _libs['csp'].strsep
    strsep.argtypes = [POINTER(POINTER(c_char)), String]
    if sizeof(c_int) == sizeof(c_void_p):
        strsep.restype = ReturnString
    else:
        strsep.restype = String
        strsep.errcheck = ReturnString

# /usr/include/string.h: 564
if hasattr(_libs['csp'], 'strsignal'):
    strsignal = _libs['csp'].strsignal
    strsignal.argtypes = [c_int]
    if sizeof(c_int) == sizeof(c_void_p):
        strsignal.restype = ReturnString
    else:
        strsignal.restype = String
        strsignal.errcheck = ReturnString

# /usr/include/string.h: 567
if hasattr(_libs['csp'], '__stpcpy'):
    __stpcpy = _libs['csp'].__stpcpy
    __stpcpy.argtypes = [String, String]
    if sizeof(c_int) == sizeof(c_void_p):
        __stpcpy.restype = ReturnString
    else:
        __stpcpy.restype = String
        __stpcpy.errcheck = ReturnString

# /usr/include/string.h: 569
if hasattr(_libs['csp'], 'stpcpy'):
    stpcpy = _libs['csp'].stpcpy
    stpcpy.argtypes = [String, String]
    if sizeof(c_int) == sizeof(c_void_p):
        stpcpy.restype = ReturnString
    else:
        stpcpy.restype = String
        stpcpy.errcheck = ReturnString

# /usr/include/string.h: 574
if hasattr(_libs['csp'], '__stpncpy'):
    __stpncpy = _libs['csp'].__stpncpy
    __stpncpy.argtypes = [String, String, c_size_t]
    if sizeof(c_int) == sizeof(c_void_p):
        __stpncpy.restype = ReturnString
    else:
        __stpncpy.restype = String
        __stpncpy.errcheck = ReturnString

# /usr/include/string.h: 577
if hasattr(_libs['csp'], 'stpncpy'):
    stpncpy = _libs['csp'].stpncpy
    stpncpy.argtypes = [String, String, c_size_t]
    if sizeof(c_int) == sizeof(c_void_p):
        stpncpy.restype = ReturnString
    else:
        stpncpy.restype = String
        stpncpy.errcheck = ReturnString

enum_anon_7 = c_int # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 40

CSP_ERROR = 0 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 40

CSP_WARN = 1 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 40

CSP_INFO = 2 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 40

CSP_BUFFER = 3 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 40

CSP_PACKET = 4 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 40

CSP_PROTOCOL = 5 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 40

CSP_LOCK = 6 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 40

csp_debug_level_t = enum_anon_7 # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 40

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 42
try:
    csp_debug_level_enabled = (POINTER(c_ubyte)).in_dll(_libs['csp'], 'csp_debug_level_enabled')
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 48
for _lib in _libs.itervalues():
    if not hasattr(_lib, 'csp_assert_fail_action'):
        continue
    csp_assert_fail_action = _lib.csp_assert_fail_action
    csp_assert_fail_action.argtypes = [String, String, c_int]
    csp_assert_fail_action.restype = None
    break

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 129
if hasattr(_libs['csp'], 'do_csp_debug'):
    _func = _libs['csp'].do_csp_debug
    _restype = None
    _argtypes = [csp_debug_level_t, String]
    do_csp_debug = _variadic_function(_func,_restype,_argtypes)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 135
if hasattr(_libs['csp'], 'csp_debug_toggle_level'):
    csp_debug_toggle_level = _libs['csp'].csp_debug_toggle_level
    csp_debug_toggle_level.argtypes = [csp_debug_level_t]
    csp_debug_toggle_level.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 142
if hasattr(_libs['csp'], 'csp_debug_set_level'):
    csp_debug_set_level = _libs['csp'].csp_debug_set_level
    csp_debug_set_level.argtypes = [csp_debug_level_t, c_bool]
    csp_debug_set_level.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 149
if hasattr(_libs['csp'], 'csp_debug_get_level'):
    csp_debug_get_level = _libs['csp'].csp_debug_get_level
    csp_debug_get_level.argtypes = [csp_debug_level_t]
    csp_debug_get_level.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_buffer.h: 38
if hasattr(_libs['csp'], 'csp_buffer_init'):
    csp_buffer_init = _libs['csp'].csp_buffer_init
    csp_buffer_init.argtypes = [c_int, c_int]
    csp_buffer_init.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_buffer.h: 47
if hasattr(_libs['csp'], 'csp_buffer_get'):
    csp_buffer_get = _libs['csp'].csp_buffer_get
    csp_buffer_get.argtypes = [c_size_t]
    csp_buffer_get.restype = POINTER(None)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_buffer.h: 56
if hasattr(_libs['csp'], 'csp_buffer_get_isr'):
    csp_buffer_get_isr = _libs['csp'].csp_buffer_get_isr
    csp_buffer_get_isr.argtypes = [c_size_t]
    csp_buffer_get_isr.restype = POINTER(None)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_buffer.h: 62
if hasattr(_libs['csp'], 'csp_buffer_free'):
    csp_buffer_free = _libs['csp'].csp_buffer_free
    csp_buffer_free.argtypes = [POINTER(None)]
    csp_buffer_free.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_buffer.h: 68
if hasattr(_libs['csp'], 'csp_buffer_free_isr'):
    csp_buffer_free_isr = _libs['csp'].csp_buffer_free_isr
    csp_buffer_free_isr.argtypes = [POINTER(None)]
    csp_buffer_free_isr.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_buffer.h: 74
if hasattr(_libs['csp'], 'csp_buffer_clone'):
    csp_buffer_clone = _libs['csp'].csp_buffer_clone
    csp_buffer_clone.argtypes = [POINTER(None)]
    csp_buffer_clone.restype = POINTER(None)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_buffer.h: 80
if hasattr(_libs['csp'], 'csp_buffer_remaining'):
    csp_buffer_remaining = _libs['csp'].csp_buffer_remaining
    csp_buffer_remaining.argtypes = []
    csp_buffer_remaining.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_buffer.h: 86
if hasattr(_libs['csp'], 'csp_buffer_size'):
    csp_buffer_size = _libs['csp'].csp_buffer_size
    csp_buffer_size.argtypes = []
    csp_buffer_size.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 35
if hasattr(_libs['csp'], 'csp_rtable_find_iface'):
    csp_rtable_find_iface = _libs['csp'].csp_rtable_find_iface
    csp_rtable_find_iface.argtypes = [c_uint8]
    csp_rtable_find_iface.restype = POINTER(csp_iface_t)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 42
if hasattr(_libs['csp'], 'csp_rtable_find_mac'):
    csp_rtable_find_mac = _libs['csp'].csp_rtable_find_mac
    csp_rtable_find_mac.argtypes = [c_uint8]
    csp_rtable_find_mac.restype = c_uint8

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 52
if hasattr(_libs['csp'], 'csp_rtable_set'):
    csp_rtable_set = _libs['csp'].csp_rtable_set
    csp_rtable_set.argtypes = [c_uint8, c_uint8, POINTER(csp_iface_t), c_uint8]
    csp_rtable_set.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 57
if hasattr(_libs['csp'], 'csp_rtable_print'):
    csp_rtable_print = _libs['csp'].csp_rtable_print
    csp_rtable_print.argtypes = []
    csp_rtable_print.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 70
for _lib in _libs.itervalues():
    if not hasattr(_lib, 'csp_route_table_load'):
        continue
    csp_route_table_load = _lib.csp_route_table_load
    csp_route_table_load.argtypes = [c_uint8 * (5 * (((1 << 5) - 1) + 2))]
    csp_route_table_load.restype = None
    break

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 82
for _lib in _libs.itervalues():
    if not hasattr(_lib, 'csp_route_table_save'):
        continue
    csp_route_table_save = _lib.csp_route_table_save
    csp_route_table_save.argtypes = [c_uint8 * (5 * (((1 << 5) - 1) + 2))]
    csp_route_table_save.restype = None
    break

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 91
if hasattr(_libs['csp'], 'csp_rtable_save'):
    csp_rtable_save = _libs['csp'].csp_rtable_save
    csp_rtable_save.argtypes = [String, c_int]
    csp_rtable_save.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 105
if hasattr(_libs['csp'], 'csp_rtable_load'):
    csp_rtable_load = _libs['csp'].csp_rtable_load
    csp_rtable_load.argtypes = [String]
    csp_rtable_load.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 112
if hasattr(_libs['csp'], 'csp_rtable_check'):
    csp_rtable_check = _libs['csp'].csp_rtable_check
    csp_rtable_check.argtypes = [String]
    csp_rtable_check.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 118
if hasattr(_libs['csp'], 'csp_rtable_clear'):
    csp_rtable_clear = _libs['csp'].csp_rtable_clear
    csp_rtable_clear.argtypes = []
    csp_rtable_clear.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_iflist.h: 28
if hasattr(_libs['csp'], 'csp_iflist_add'):
    csp_iflist_add = _libs['csp'].csp_iflist_add
    csp_iflist_add.argtypes = [POINTER(csp_iface_t)]
    csp_iflist_add.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_iflist.h: 35
if hasattr(_libs['csp'], 'csp_iflist_get_by_name'):
    csp_iflist_get_by_name = _libs['csp'].csp_iflist_get_by_name
    csp_iflist_get_by_name.argtypes = [String]
    csp_iflist_get_by_name.restype = POINTER(csp_iface_t)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_iflist.h: 40
if hasattr(_libs['csp'], 'csp_iflist_print'):
    csp_iflist_print = _libs['csp'].csp_iflist_print
    csp_iflist_print.argtypes = []
    csp_iflist_print.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 43
try:
    my_address = (c_uint8).in_dll(_libs['csp'], 'my_address')
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 49
if hasattr(_libs['csp'], 'csp_init'):
    csp_init = _libs['csp'].csp_init
    csp_init.argtypes = [c_uint8]
    csp_init.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 56
if hasattr(_libs['csp'], 'csp_set_hostname'):
    csp_set_hostname = _libs['csp'].csp_set_hostname
    csp_set_hostname.argtypes = [String]
    csp_set_hostname.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 62
if hasattr(_libs['csp'], 'csp_get_hostname'):
    csp_get_hostname = _libs['csp'].csp_get_hostname
    csp_get_hostname.argtypes = []
    if sizeof(c_int) == sizeof(c_void_p):
        csp_get_hostname.restype = ReturnString
    else:
        csp_get_hostname.restype = String
        csp_get_hostname.errcheck = ReturnString

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 69
if hasattr(_libs['csp'], 'csp_set_model'):
    csp_set_model = _libs['csp'].csp_set_model
    csp_set_model.argtypes = [String]
    csp_set_model.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 75
if hasattr(_libs['csp'], 'csp_get_model'):
    csp_get_model = _libs['csp'].csp_get_model
    csp_get_model.argtypes = []
    if sizeof(c_int) == sizeof(c_void_p):
        csp_get_model.restype = ReturnString
    else:
        csp_get_model.restype = String
        csp_get_model.errcheck = ReturnString

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 82
if hasattr(_libs['csp'], 'csp_socket'):
    csp_socket = _libs['csp'].csp_socket
    csp_socket.argtypes = [c_uint32]
    csp_socket.restype = POINTER(csp_socket_t)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 90
if hasattr(_libs['csp'], 'csp_accept'):
    csp_accept = _libs['csp'].csp_accept
    csp_accept.argtypes = [POINTER(csp_socket_t), c_uint32]
    csp_accept.restype = POINTER(csp_conn_t)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 102
if hasattr(_libs['csp'], 'csp_read'):
    csp_read = _libs['csp'].csp_read
    csp_read.argtypes = [POINTER(csp_conn_t), c_uint32]
    csp_read.restype = POINTER(csp_packet_t)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 111
if hasattr(_libs['csp'], 'csp_send'):
    csp_send = _libs['csp'].csp_send
    csp_send.argtypes = [POINTER(csp_conn_t), POINTER(csp_packet_t), c_uint32]
    csp_send.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 125
if hasattr(_libs['csp'], 'csp_send_prio'):
    csp_send_prio = _libs['csp'].csp_send_prio
    csp_send_prio.argtypes = [c_uint8, POINTER(csp_conn_t), POINTER(csp_packet_t), c_uint32]
    csp_send_prio.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 141
if hasattr(_libs['csp'], 'csp_transaction'):
    csp_transaction = _libs['csp'].csp_transaction
    csp_transaction.argtypes = [c_uint8, c_uint8, c_uint8, c_uint32, POINTER(None), c_int, POINTER(None), c_int]
    csp_transaction.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 154
if hasattr(_libs['csp'], 'csp_transaction_persistent'):
    csp_transaction_persistent = _libs['csp'].csp_transaction_persistent
    csp_transaction_persistent.argtypes = [POINTER(csp_conn_t), c_uint32, POINTER(None), c_int, POINTER(None), c_int]
    csp_transaction_persistent.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 163
if hasattr(_libs['csp'], 'csp_recvfrom'):
    csp_recvfrom = _libs['csp'].csp_recvfrom
    csp_recvfrom.argtypes = [POINTER(csp_socket_t), c_uint32]
    csp_recvfrom.restype = POINTER(csp_packet_t)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 176
if hasattr(_libs['csp'], 'csp_sendto'):
    csp_sendto = _libs['csp'].csp_sendto
    csp_sendto.argtypes = [c_uint8, c_uint8, c_uint8, c_uint8, c_uint32, POINTER(csp_packet_t), c_uint32]
    csp_sendto.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 187
if hasattr(_libs['csp'], 'csp_sendto_reply'):
    csp_sendto_reply = _libs['csp'].csp_sendto_reply
    csp_sendto_reply.argtypes = [POINTER(csp_packet_t), POINTER(csp_packet_t), c_uint32, c_uint32]
    csp_sendto_reply.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 201
if hasattr(_libs['csp'], 'csp_connect'):
    csp_connect = _libs['csp'].csp_connect
    csp_connect.argtypes = [c_uint8, c_uint8, c_uint8, c_uint32, c_uint32]
    csp_connect.restype = POINTER(csp_conn_t)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 208
if hasattr(_libs['csp'], 'csp_close'):
    csp_close = _libs['csp'].csp_close
    csp_close.argtypes = [POINTER(csp_conn_t)]
    csp_close.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 214
if hasattr(_libs['csp'], 'csp_conn_dport'):
    csp_conn_dport = _libs['csp'].csp_conn_dport
    csp_conn_dport.argtypes = [POINTER(csp_conn_t)]
    csp_conn_dport.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 220
if hasattr(_libs['csp'], 'csp_conn_sport'):
    csp_conn_sport = _libs['csp'].csp_conn_sport
    csp_conn_sport.argtypes = [POINTER(csp_conn_t)]
    csp_conn_sport.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 226
if hasattr(_libs['csp'], 'csp_conn_dst'):
    csp_conn_dst = _libs['csp'].csp_conn_dst
    csp_conn_dst.argtypes = [POINTER(csp_conn_t)]
    csp_conn_dst.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 232
if hasattr(_libs['csp'], 'csp_conn_src'):
    csp_conn_src = _libs['csp'].csp_conn_src
    csp_conn_src.argtypes = [POINTER(csp_conn_t)]
    csp_conn_src.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 238
if hasattr(_libs['csp'], 'csp_conn_flags'):
    csp_conn_flags = _libs['csp'].csp_conn_flags
    csp_conn_flags.argtypes = [POINTER(csp_conn_t)]
    csp_conn_flags.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 246
if hasattr(_libs['csp'], 'csp_listen'):
    csp_listen = _libs['csp'].csp_listen
    csp_listen.argtypes = [POINTER(csp_socket_t), c_size_t]
    csp_listen.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 254
if hasattr(_libs['csp'], 'csp_bind'):
    csp_bind = _libs['csp'].csp_bind
    csp_bind.argtypes = [POINTER(csp_socket_t), c_uint8]
    csp_bind.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 261
if hasattr(_libs['csp'], 'csp_route_start_task'):
    csp_route_start_task = _libs['csp'].csp_route_start_task
    csp_route_start_task.argtypes = [c_uint, c_uint]
    csp_route_start_task.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 271
if hasattr(_libs['csp'], 'csp_bridge_start'):
    csp_bridge_start = _libs['csp'].csp_bridge_start
    csp_bridge_start.argtypes = [c_uint, c_uint, POINTER(csp_iface_t), POINTER(csp_iface_t)]
    csp_bridge_start.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 282
if hasattr(_libs['csp'], 'csp_promisc_enable'):
    csp_promisc_enable = _libs['csp'].csp_promisc_enable
    csp_promisc_enable.argtypes = [c_uint]
    csp_promisc_enable.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 289
if hasattr(_libs['csp'], 'csp_promisc_disable'):
    csp_promisc_disable = _libs['csp'].csp_promisc_disable
    csp_promisc_disable.argtypes = []
    csp_promisc_disable.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 299
if hasattr(_libs['csp'], 'csp_promisc_read'):
    csp_promisc_read = _libs['csp'].csp_promisc_read
    csp_promisc_read.argtypes = [c_uint32]
    csp_promisc_read.restype = POINTER(csp_packet_t)

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 312
if hasattr(_libs['csp'], 'csp_sfp_send'):
    csp_sfp_send = _libs['csp'].csp_sfp_send
    csp_sfp_send.argtypes = [POINTER(csp_conn_t), POINTER(None), c_int, c_int, c_uint32]
    csp_sfp_send.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 322
if hasattr(_libs['csp'], 'csp_sfp_recv'):
    csp_sfp_recv = _libs['csp'].csp_sfp_recv
    csp_sfp_recv.argtypes = [POINTER(csp_conn_t), POINTER(POINTER(None)), POINTER(c_int), c_uint32]
    csp_sfp_recv.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 335
if hasattr(_libs['csp'], 'csp_service_handler'):
    csp_service_handler = _libs['csp'].csp_service_handler
    csp_service_handler.argtypes = [POINTER(csp_conn_t), POINTER(csp_packet_t)]
    csp_service_handler.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 345
if hasattr(_libs['csp'], 'csp_ping'):
    csp_ping = _libs['csp'].csp_ping
    csp_ping.argtypes = [c_uint8, c_uint32, c_uint, c_uint8]
    csp_ping.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 351
if hasattr(_libs['csp'], 'csp_ping_noreply'):
    csp_ping_noreply = _libs['csp'].csp_ping_noreply
    csp_ping_noreply.argtypes = [c_uint8]
    csp_ping_noreply.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 359
if hasattr(_libs['csp'], 'csp_ps'):
    csp_ps = _libs['csp'].csp_ps
    csp_ps.argtypes = [c_uint8, c_uint32]
    csp_ps.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 366
if hasattr(_libs['csp'], 'csp_memfree'):
    csp_memfree = _libs['csp'].csp_memfree
    csp_memfree.argtypes = [c_uint8, c_uint32]
    csp_memfree.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 373
if hasattr(_libs['csp'], 'csp_buf_free'):
    csp_buf_free = _libs['csp'].csp_buf_free
    csp_buf_free.argtypes = [c_uint8, c_uint32]
    csp_buf_free.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 379
if hasattr(_libs['csp'], 'csp_reboot'):
    csp_reboot = _libs['csp'].csp_reboot
    csp_reboot.argtypes = [c_uint8]
    csp_reboot.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 386
if hasattr(_libs['csp'], 'csp_uptime'):
    csp_uptime = _libs['csp'].csp_uptime
    csp_uptime.argtypes = [c_uint8, c_uint32]
    csp_uptime.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 397
if hasattr(_libs['csp'], 'csp_rdp_set_opt'):
    csp_rdp_set_opt = _libs['csp'].csp_rdp_set_opt
    csp_rdp_set_opt.argtypes = [c_uint, c_uint, c_uint, c_uint, c_uint, c_uint]
    csp_rdp_set_opt.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 410
if hasattr(_libs['csp'], 'csp_rdp_get_opt'):
    csp_rdp_get_opt = _libs['csp'].csp_rdp_get_opt
    csp_rdp_get_opt.argtypes = [POINTER(c_uint), POINTER(c_uint), POINTER(c_uint), POINTER(c_uint), POINTER(c_uint), POINTER(c_uint)]
    csp_rdp_get_opt.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 420
if hasattr(_libs['csp'], 'csp_xtea_set_key'):
    csp_xtea_set_key = _libs['csp'].csp_xtea_set_key
    csp_xtea_set_key.argtypes = [String, c_uint32]
    csp_xtea_set_key.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 428
if hasattr(_libs['csp'], 'csp_hmac_set_key'):
    csp_hmac_set_key = _libs['csp'].csp_hmac_set_key
    csp_hmac_set_key.argtypes = [String, c_uint32]
    csp_hmac_set_key.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 433
if hasattr(_libs['csp'], 'csp_conn_print_table'):
    csp_conn_print_table = _libs['csp'].csp_conn_print_table
    csp_conn_print_table.argtypes = []
    csp_conn_print_table.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 438
for _lib in _libs.itervalues():
    if not hasattr(_lib, 'csp_buffer_print_table'):
        continue
    csp_buffer_print_table = _lib.csp_buffer_print_table
    csp_buffer_print_table.argtypes = []
    csp_buffer_print_table.restype = None
    break

csp_debug_hook_func_t = CFUNCTYPE(UNCHECKED(None), csp_debug_level_t, String) # /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 444

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp.h: 445
if hasattr(_libs['csp'], 'csp_debug_hook_set'):
    csp_debug_hook_set = _libs['csp'].csp_debug_hook_set
    csp_debug_hook_set.argtypes = [csp_debug_hook_func_t]
    csp_debug_hook_set.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 59
class struct_i2c_frame_s(Structure):
    pass

struct_i2c_frame_s.__slots__ = [
    'padding',
    'retries',
    'reserved',
    'dest',
    'len_rx',
    'len',
    'data',
]
struct_i2c_frame_s._fields_ = [
    ('padding', c_uint8),
    ('retries', c_uint8),
    ('reserved', c_uint32),
    ('dest', c_uint8),
    ('len_rx', c_uint8),
    ('len', c_uint16),
    ('data', c_uint8 * 256),
]

i2c_frame_t = struct_i2c_frame_s # /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 59

i2c_callback_t = CFUNCTYPE(UNCHECKED(None), POINTER(i2c_frame_t), POINTER(None)) # /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 73

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 74
for _lib in _libs.itervalues():
    if not hasattr(_lib, 'i2c_init'):
        continue
    i2c_init = _lib.i2c_init
    i2c_init.argtypes = [c_int, c_int, c_uint8, c_uint16, c_int, c_int, i2c_callback_t]
    i2c_init.restype = c_int
    break

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 84
for _lib in _libs.itervalues():
    if not hasattr(_lib, 'i2c_send'):
        continue
    i2c_send = _lib.i2c_send
    i2c_send.argtypes = [c_int, POINTER(i2c_frame_t), c_uint16]
    i2c_send.restype = c_int
    break

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 36
class struct_usart_conf(Structure):
    pass

struct_usart_conf.__slots__ = [
    'device',
    'baudrate',
    'databits',
    'stopbits',
    'paritysetting',
    'checkparity',
]
struct_usart_conf._fields_ = [
    ('device', String),
    ('baudrate', c_uint32),
    ('databits', c_uint8),
    ('stopbits', c_uint8),
    ('paritysetting', c_uint8),
    ('checkparity', c_uint8),
]

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 49
if hasattr(_libs['csp'], 'usart_init'):
    usart_init = _libs['csp'].usart_init
    usart_init.argtypes = [POINTER(struct_usart_conf)]
    usart_init.restype = None

usart_callback_t = CFUNCTYPE(UNCHECKED(None), POINTER(c_uint8), c_int, POINTER(None)) # /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 57

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 58
if hasattr(_libs['csp'], 'usart_set_callback'):
    usart_set_callback = _libs['csp'].usart_set_callback
    usart_set_callback.argtypes = [usart_callback_t]
    usart_set_callback.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 65
if hasattr(_libs['csp'], 'usart_insert'):
    usart_insert = _libs['csp'].usart_insert
    usart_insert.argtypes = [c_char, POINTER(None)]
    usart_insert.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 73
if hasattr(_libs['csp'], 'usart_putc'):
    usart_putc = _libs['csp'].usart_putc
    usart_putc.argtypes = [c_char]
    usart_putc.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 82
if hasattr(_libs['csp'], 'usart_putstr'):
    usart_putstr = _libs['csp'].usart_putstr
    usart_putstr.argtypes = [String, c_int]
    usart_putstr.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 90
if hasattr(_libs['csp'], 'usart_getc'):
    usart_getc = _libs['csp'].usart_getc
    usart_getc.argtypes = []
    usart_getc.restype = c_char

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 92
if hasattr(_libs['csp'], 'usart_messages_waiting'):
    usart_messages_waiting = _libs['csp'].usart_messages_waiting
    usart_messages_waiting.argtypes = [c_int]
    usart_messages_waiting.restype = c_int

# ../../include/csp/csp_interface.h: 48
if hasattr(_libs['csp'], 'csp_qfifo_write'):
    csp_qfifo_write = _libs['csp'].csp_qfifo_write
    csp_qfifo_write.argtypes = [POINTER(csp_packet_t), POINTER(csp_iface_t), POINTER(c_int)]
    csp_qfifo_write.restype = None

# ../../include/csp/csp_interface.h: 60
for _lib in _libs.itervalues():
    if not hasattr(_lib, 'csp_route_get_mac'):
        continue
    csp_route_get_mac = _lib.csp_route_get_mac
    csp_route_get_mac.argtypes = [c_uint8]
    csp_route_get_mac.restype = c_uint8
    break

# ../../include/csp/csp_interface.h: 66
if hasattr(_libs['csp'], 'csp_iflist_add'):
    csp_iflist_add = _libs['csp'].csp_iflist_add
    csp_iflist_add.argtypes = [POINTER(csp_iface_t)]
    csp_iflist_add.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_can.h: 37
try:
    csp_if_can = (csp_iface_t).in_dll(_libs['csp'], 'csp_if_can')
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_can.h: 40
class struct_csp_can_config(Structure):
    pass

struct_csp_can_config.__slots__ = [
    'bitrate',
    'clock_speed',
    'ifc',
]
struct_csp_can_config._fields_ = [
    ('bitrate', c_uint32),
    ('clock_speed', c_uint32),
    ('ifc', String),
]

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_can.h: 52
if hasattr(_libs['csp'], 'csp_can_init'):
    csp_can_init = _libs['csp'].csp_can_init
    csp_can_init.argtypes = [c_uint8, POINTER(struct_csp_can_config)]
    csp_can_init.restype = c_int

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_i2c.h: 33
for _lib in _libs.values():
    try:
        csp_if_i2c = (csp_iface_t).in_dll(_lib, 'csp_if_i2c')
        break
    except:
        pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_i2c.h: 42
for _lib in _libs.itervalues():
    if not hasattr(_lib, 'csp_i2c_init'):
        continue
    csp_i2c_init = _lib.csp_i2c_init
    csp_i2c_init.argtypes = [c_uint8, c_int, c_int]
    csp_i2c_init.restype = c_int
    break

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 50
if hasattr(_libs['csp'], 'csp_kiss_rx'):
    csp_kiss_rx = _libs['csp'].csp_kiss_rx
    csp_kiss_rx.argtypes = [POINTER(csp_iface_t), POINTER(c_uint8), c_int, POINTER(None)]
    csp_kiss_rx.restype = None

csp_kiss_putc_f = CFUNCTYPE(UNCHECKED(None), c_char) # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 59

csp_kiss_discard_f = CFUNCTYPE(UNCHECKED(None), c_char, POINTER(None)) # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 73

enum_anon_8 = c_int # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 80

KISS_MODE_NOT_STARTED = 0 # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 80

KISS_MODE_STARTED = (KISS_MODE_NOT_STARTED + 1) # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 80

KISS_MODE_ESCAPED = (KISS_MODE_STARTED + 1) # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 80

KISS_MODE_SKIP_FRAME = (KISS_MODE_ESCAPED + 1) # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 80

kiss_mode_e = enum_anon_8 # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 80

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 95
class struct_csp_kiss_handle_s(Structure):
    pass

struct_csp_kiss_handle_s.__slots__ = [
    'kiss_putc',
    'kiss_discard',
    'rx_length',
    'rx_mode',
    'rx_first',
    'rx_cbuf',
    'rx_packet',
]
struct_csp_kiss_handle_s._fields_ = [
    ('kiss_putc', csp_kiss_putc_f),
    ('kiss_discard', csp_kiss_discard_f),
    ('rx_length', c_uint),
    ('rx_mode', kiss_mode_e),
    ('rx_first', c_uint),
    ('rx_cbuf', POINTER(c_ubyte)),
    ('rx_packet', POINTER(csp_packet_t)),
]

csp_kiss_handle_t = struct_csp_kiss_handle_s # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 95

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 97
if hasattr(_libs['csp'], 'csp_kiss_init'):
    csp_kiss_init = _libs['csp'].csp_kiss_init
    csp_kiss_init.argtypes = [POINTER(csp_iface_t), POINTER(csp_kiss_handle_t), csp_kiss_putc_f, csp_kiss_discard_f, String]
    csp_kiss_init.restype = None

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_lo.h: 32
try:
    csp_if_lo = (csp_iface_t).in_dll(_libs['csp'], 'csp_if_lo')
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_zmqhub.h: 6
try:
    csp_if_zmqhub = (csp_iface_t).in_dll(_libs['csp'], 'csp_if_zmqhub')
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_zmqhub.h: 14
if hasattr(_libs['csp'], 'csp_zmqhub_init'):
    csp_zmqhub_init = _libs['csp'].csp_zmqhub_init
    csp_zmqhub_init.argtypes = [c_char, String]
    csp_zmqhub_init.restype = c_int

# /tmp/tmpwAji6q.h: 1
try:
    __STDC__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __STDC_HOSTED__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GNUC__ = 4
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GNUC_MINOR__ = 8
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GNUC_PATCHLEVEL__ = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __VERSION__ = '4.8.2'
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ATOMIC_RELAXED = 0
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ATOMIC_SEQ_CST = 5
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ATOMIC_ACQUIRE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ATOMIC_RELEASE = 3
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ATOMIC_ACQ_REL = 4
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ATOMIC_CONSUME = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FINITE_MATH_ONLY__ = 0
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    _LP64 = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LP64__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_INT__ = 4
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_LONG__ = 8
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_LONG_LONG__ = 8
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_SHORT__ = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_FLOAT__ = 4
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_DOUBLE__ = 8
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_LONG_DOUBLE__ = 16
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_SIZE_T__ = 8
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __CHAR_BIT__ = 8
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __BIGGEST_ALIGNMENT__ = 16
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ORDER_LITTLE_ENDIAN__ = 1234
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ORDER_BIG_ENDIAN__ = 4321
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ORDER_PDP_ENDIAN__ = 3412
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __BYTE_ORDER__ = __ORDER_LITTLE_ENDIAN__
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLOAT_WORD_ORDER__ = __ORDER_LITTLE_ENDIAN__
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_POINTER__ = 8
except:
    pass

__SIZE_TYPE__ = c_ulong # /tmp/tmpwAji6q.h: 1

__PTRDIFF_TYPE__ = c_long # /tmp/tmpwAji6q.h: 1

__WCHAR_TYPE__ = c_int # /tmp/tmpwAji6q.h: 1

__WINT_TYPE__ = c_uint # /tmp/tmpwAji6q.h: 1

__INTMAX_TYPE__ = c_long # /tmp/tmpwAji6q.h: 1

__UINTMAX_TYPE__ = c_ulong # /tmp/tmpwAji6q.h: 1

__CHAR16_TYPE__ = c_uint # /tmp/tmpwAji6q.h: 1

__CHAR32_TYPE__ = c_uint # /tmp/tmpwAji6q.h: 1

__SIG_ATOMIC_TYPE__ = c_int # /tmp/tmpwAji6q.h: 1

__INT8_TYPE__ = c_char # /tmp/tmpwAji6q.h: 1

__INT16_TYPE__ = c_int # /tmp/tmpwAji6q.h: 1

__INT32_TYPE__ = c_int # /tmp/tmpwAji6q.h: 1

__INT64_TYPE__ = c_long # /tmp/tmpwAji6q.h: 1

__UINT8_TYPE__ = c_ubyte # /tmp/tmpwAji6q.h: 1

__UINT16_TYPE__ = c_uint # /tmp/tmpwAji6q.h: 1

__UINT32_TYPE__ = c_uint # /tmp/tmpwAji6q.h: 1

__UINT64_TYPE__ = c_ulong # /tmp/tmpwAji6q.h: 1

__INT_LEAST8_TYPE__ = c_char # /tmp/tmpwAji6q.h: 1

__INT_LEAST16_TYPE__ = c_int # /tmp/tmpwAji6q.h: 1

__INT_LEAST32_TYPE__ = c_int # /tmp/tmpwAji6q.h: 1

__INT_LEAST64_TYPE__ = c_long # /tmp/tmpwAji6q.h: 1

__UINT_LEAST8_TYPE__ = c_ubyte # /tmp/tmpwAji6q.h: 1

__UINT_LEAST16_TYPE__ = c_uint # /tmp/tmpwAji6q.h: 1

__UINT_LEAST32_TYPE__ = c_uint # /tmp/tmpwAji6q.h: 1

__UINT_LEAST64_TYPE__ = c_ulong # /tmp/tmpwAji6q.h: 1

__INT_FAST8_TYPE__ = c_char # /tmp/tmpwAji6q.h: 1

__INT_FAST16_TYPE__ = c_long # /tmp/tmpwAji6q.h: 1

__INT_FAST32_TYPE__ = c_long # /tmp/tmpwAji6q.h: 1

__INT_FAST64_TYPE__ = c_long # /tmp/tmpwAji6q.h: 1

__UINT_FAST8_TYPE__ = c_ubyte # /tmp/tmpwAji6q.h: 1

__UINT_FAST16_TYPE__ = c_ulong # /tmp/tmpwAji6q.h: 1

__UINT_FAST32_TYPE__ = c_ulong # /tmp/tmpwAji6q.h: 1

__UINT_FAST64_TYPE__ = c_ulong # /tmp/tmpwAji6q.h: 1

__INTPTR_TYPE__ = c_long # /tmp/tmpwAji6q.h: 1

__UINTPTR_TYPE__ = c_ulong # /tmp/tmpwAji6q.h: 1

# /tmp/tmpwAji6q.h: 1
try:
    __GXX_ABI_VERSION = 1002
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SCHAR_MAX__ = 127
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SHRT_MAX__ = 32767
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT_MAX__ = 2147483647
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LONG_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LONG_LONG_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __WCHAR_MAX__ = 2147483647
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __WCHAR_MIN__ = ((-__WCHAR_MAX__) - 1)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __WINT_MAX__ = 4294967295
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __WINT_MIN__ = 0
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __PTRDIFF_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZE_MAX__ = 18446744073709551615L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INTMAX_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINTMAX_MAX__ = 18446744073709551615L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIG_ATOMIC_MAX__ = 2147483647
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIG_ATOMIC_MIN__ = ((-__SIG_ATOMIC_MAX__) - 1)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT8_MAX__ = 127
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT16_MAX__ = 32767
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT32_MAX__ = 2147483647
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT64_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT8_MAX__ = 255
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT16_MAX__ = 65535
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT32_MAX__ = 4294967295
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT64_MAX__ = 18446744073709551615L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT_LEAST8_MAX__ = 127
except:
    pass

# /tmp/tmpwAji6q.h: 1
def __INT8_C(c):
    return c

# /tmp/tmpwAji6q.h: 1
try:
    __INT_LEAST16_MAX__ = 32767
except:
    pass

# /tmp/tmpwAji6q.h: 1
def __INT16_C(c):
    return c

# /tmp/tmpwAji6q.h: 1
try:
    __INT_LEAST32_MAX__ = 2147483647
except:
    pass

# /tmp/tmpwAji6q.h: 1
def __INT32_C(c):
    return c

# /tmp/tmpwAji6q.h: 1
try:
    __INT_LEAST64_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT_LEAST8_MAX__ = 255
except:
    pass

# /tmp/tmpwAji6q.h: 1
def __UINT8_C(c):
    return c

# /tmp/tmpwAji6q.h: 1
try:
    __UINT_LEAST16_MAX__ = 65535
except:
    pass

# /tmp/tmpwAji6q.h: 1
def __UINT16_C(c):
    return c

# /tmp/tmpwAji6q.h: 1
try:
    __UINT_LEAST32_MAX__ = 4294967295
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT_LEAST64_MAX__ = 18446744073709551615L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT_FAST8_MAX__ = 127
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT_FAST16_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT_FAST32_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INT_FAST64_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT_FAST8_MAX__ = 255
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT_FAST16_MAX__ = 18446744073709551615L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT_FAST32_MAX__ = 18446744073709551615L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINT_FAST64_MAX__ = 18446744073709551615L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __INTPTR_MAX__ = 9223372036854775807L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __UINTPTR_MAX__ = 18446744073709551615L
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_EVAL_METHOD__ = 0
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC_EVAL_METHOD__ = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_RADIX__ = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_MANT_DIG__ = 24
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_DIG__ = 6
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_MIN_EXP__ = (-125)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_MIN_10_EXP__ = (-37)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_MAX_EXP__ = 128
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_MAX_10_EXP__ = 38
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_DECIMAL_DIG__ = 9
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_MAX__ = 3.4028234663852886e+38
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_MIN__ = 1.1754943508222875e-38
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_EPSILON__ = 1.1920928955078125e-07
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_DENORM_MIN__ = 1.401298464324817e-45
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_HAS_DENORM__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_HAS_INFINITY__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FLT_HAS_QUIET_NAN__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_MANT_DIG__ = 53
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_DIG__ = 15
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_MIN_EXP__ = (-1021)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_MIN_10_EXP__ = (-307)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_MAX_EXP__ = 1024
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_MAX_10_EXP__ = 308
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_DECIMAL_DIG__ = 17
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_MAX__ = 1.7976931348623157e+308
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_MIN__ = 2.2250738585072014e-308
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_EPSILON__ = 2.220446049250313e-16
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_DENORM_MIN__ = 5e-324
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_HAS_DENORM__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_HAS_INFINITY__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DBL_HAS_QUIET_NAN__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_MANT_DIG__ = 64
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_DIG__ = 18
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_MIN_EXP__ = (-16381)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_MIN_10_EXP__ = (-4931)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_MAX_EXP__ = 16384
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_MAX_10_EXP__ = 4932
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DECIMAL_DIG__ = 21
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_MAX__ = float('inf')
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_MIN__ = 0.0
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_EPSILON__ = 1.0842021724855044e-19
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_DENORM_MIN__ = 0.0
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_HAS_DENORM__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_HAS_INFINITY__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __LDBL_HAS_QUIET_NAN__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC32_MANT_DIG__ = 7
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC32_MIN_EXP__ = (-94)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC32_MAX_EXP__ = 97
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC64_MANT_DIG__ = 16
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC64_MIN_EXP__ = (-382)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC64_MAX_EXP__ = 385
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC128_MANT_DIG__ = 34
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC128_MIN_EXP__ = (-6142)
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DEC128_MAX_EXP__ = 6145
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GNUC_GNU_INLINE__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __NO_INLINE__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8 = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_BOOL_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_CHAR_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_CHAR16_T_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_CHAR32_T_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_WCHAR_T_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_SHORT_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_INT_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_LONG_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_LLONG_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_TEST_AND_SET_TRUEVAL = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_ATOMIC_POINTER_LOCK_FREE = 2
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __GCC_HAVE_DWARF2_CFI_ASM = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __PRAGMA_REDEFINE_EXTNAME = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SSP__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_INT128__ = 16
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_WCHAR_T__ = 4
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_WINT_T__ = 4
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SIZEOF_PTRDIFF_T__ = 8
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __amd64 = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __amd64__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __x86_64 = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __x86_64__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ATOMIC_HLE_ACQUIRE = 65536
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ATOMIC_HLE_RELEASE = 131072
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __k8 = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __k8__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __code_model_small__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __MMX__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SSE__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SSE2__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __FXSR__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SSE_MATH__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __SSE2_MATH__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __gnu_linux__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __linux = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __linux__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    linux = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __unix = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __unix__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    unix = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __ELF__ = 1
except:
    pass

# /tmp/tmpwAji6q.h: 1
try:
    __DECIMAL_BID_FORMAT__ = 1
except:
    pass

__const = c_int # <command-line>: 4

# <command-line>: 7
try:
    CTYPESGEN = 1
except:
    pass

# /usr/include/stdc-predef.h: 19
try:
    _STDC_PREDEF_H = 1
except:
    pass

# /usr/include/stdc-predef.h: 41
try:
    __STDC_IEC_559__ = 1
except:
    pass

# /usr/include/stdc-predef.h: 49
try:
    __STDC_IEC_559_COMPLEX__ = 1
except:
    pass

# /usr/include/stdc-predef.h: 54
try:
    __STDC_ISO_10646__ = 201103L
except:
    pass

# /usr/include/stdc-predef.h: 57
try:
    __STDC_NO_THREADS__ = 1
except:
    pass

# /usr/include/stdint.h: 23
try:
    _STDINT_H = 1
except:
    pass

# /usr/include/features.h: 19
try:
    _FEATURES_H = 1
except:
    pass

# /usr/include/features.h: 145
def __GNUC_PREREQ(maj, min):
    return 0

# /usr/include/features.h: 186
try:
    _DEFAULT_SOURCE = 1
except:
    pass

# /usr/include/features.h: 188
try:
    _BSD_SOURCE = 1
except:
    pass

# /usr/include/features.h: 190
try:
    _SVID_SOURCE = 1
except:
    pass

# /usr/include/features.h: 225
try:
    __USE_POSIX_IMPLICITLY = 1
except:
    pass

# /usr/include/features.h: 228
try:
    _POSIX_SOURCE = 1
except:
    pass

# /usr/include/features.h: 230
try:
    _POSIX_C_SOURCE = 200809L
except:
    pass

# /usr/include/features.h: 248
try:
    __USE_POSIX = 1
except:
    pass

# /usr/include/features.h: 252
try:
    __USE_POSIX2 = 1
except:
    pass

# /usr/include/features.h: 256
try:
    __USE_POSIX199309 = 1
except:
    pass

# /usr/include/features.h: 260
try:
    __USE_POSIX199506 = 1
except:
    pass

# /usr/include/features.h: 264
try:
    __USE_XOPEN2K = 1
except:
    pass

# /usr/include/features.h: 266
try:
    __USE_ISOC95 = 1
except:
    pass

# /usr/include/features.h: 268
try:
    __USE_ISOC99 = 1
except:
    pass

# /usr/include/features.h: 272
try:
    __USE_XOPEN2K8 = 1
except:
    pass

# /usr/include/features.h: 274
try:
    _ATFILE_SOURCE = 1
except:
    pass

# /usr/include/features.h: 316
try:
    __USE_MISC = 1
except:
    pass

# /usr/include/features.h: 320
try:
    __USE_BSD = 1
except:
    pass

# /usr/include/features.h: 324
try:
    __USE_SVID = 1
except:
    pass

# /usr/include/features.h: 328
try:
    __USE_ATFILE = 1
except:
    pass

# /usr/include/features.h: 347
try:
    __USE_FORTIFY_LEVEL = 0
except:
    pass

# /usr/include/features.h: 361
try:
    __GNU_LIBRARY__ = 6
except:
    pass

# /usr/include/features.h: 365
try:
    __GLIBC__ = 2
except:
    pass

# /usr/include/features.h: 366
try:
    __GLIBC_MINOR__ = 19
except:
    pass

# /usr/include/features.h: 368
def __GLIBC_PREREQ(maj, min):
    return (((__GLIBC__ << 16) + __GLIBC_MINOR__) >= ((maj << 16) + min))

# /usr/include/x86_64-linux-gnu/sys/cdefs.h: 19
try:
    _SYS_CDEFS_H = 1
except:
    pass

# /usr/include/x86_64-linux-gnu/sys/cdefs.h: 76
def __NTH(fct):
    return fct

# /usr/include/x86_64-linux-gnu/sys/cdefs.h: 82
def __P(args):
    return args

# /usr/include/x86_64-linux-gnu/sys/cdefs.h: 83
def __PMT(args):
    return args

# /usr/include/x86_64-linux-gnu/sys/cdefs.h: 89
def __STRING(x):
    return x

__ptr_t = POINTER(None) # /usr/include/x86_64-linux-gnu/sys/cdefs.h: 92

# /usr/include/x86_64-linux-gnu/sys/cdefs.h: 381
def __glibc_unlikely(cond):
    return cond

# /usr/include/x86_64-linux-gnu/sys/cdefs.h: 382
def __glibc_likely(cond):
    return cond

# /usr/include/x86_64-linux-gnu/bits/wordsize.h: 4
try:
    __WORDSIZE = 64
except:
    pass

# /usr/include/x86_64-linux-gnu/bits/wordsize.h: 10
try:
    __WORDSIZE_TIME64_COMPAT32 = 1
except:
    pass

# /usr/include/x86_64-linux-gnu/bits/wordsize.h: 12
try:
    __SYSCALL_WORDSIZE = 64
except:
    pass

# /usr/include/x86_64-linux-gnu/sys/cdefs.h: 407
def __LDBL_REDIR1(name, proto, alias):
    return (name + proto)

# /usr/include/x86_64-linux-gnu/sys/cdefs.h: 408
def __LDBL_REDIR(name, proto):
    return (name + proto)

# /usr/include/x86_64-linux-gnu/bits/wchar.h: 20
try:
    _BITS_WCHAR_H = 1
except:
    pass

# /usr/include/x86_64-linux-gnu/bits/wchar.h: 34
try:
    __WCHAR_MAX = __WCHAR_MAX__
except:
    pass

# /usr/include/x86_64-linux-gnu/bits/wchar.h: 42
try:
    __WCHAR_MIN = __WCHAR_MIN__
except:
    pass

# /usr/include/x86_64-linux-gnu/bits/wordsize.h: 4
try:
    __WORDSIZE = 64
except:
    pass

# /usr/include/x86_64-linux-gnu/bits/wordsize.h: 10
try:
    __WORDSIZE_TIME64_COMPAT32 = 1
except:
    pass

# /usr/include/x86_64-linux-gnu/bits/wordsize.h: 12
try:
    __SYSCALL_WORDSIZE = 64
except:
    pass

# /usr/include/stdint.h: 155
try:
    INT8_MIN = (-128)
except:
    pass

# /usr/include/stdint.h: 156
try:
    INT16_MIN = ((-32767) - 1)
except:
    pass

# /usr/include/stdint.h: 157
try:
    INT32_MIN = ((-2147483647) - 1)
except:
    pass

# /usr/include/stdint.h: 160
try:
    INT8_MAX = 127
except:
    pass

# /usr/include/stdint.h: 161
try:
    INT16_MAX = 32767
except:
    pass

# /usr/include/stdint.h: 162
try:
    INT32_MAX = 2147483647
except:
    pass

# /usr/include/stdint.h: 166
try:
    UINT8_MAX = 255
except:
    pass

# /usr/include/stdint.h: 167
try:
    UINT16_MAX = 65535
except:
    pass

# /usr/include/stdint.h: 168
try:
    UINT32_MAX = 4294967295
except:
    pass

# /usr/include/stdint.h: 173
try:
    INT_LEAST8_MIN = (-128)
except:
    pass

# /usr/include/stdint.h: 174
try:
    INT_LEAST16_MIN = ((-32767) - 1)
except:
    pass

# /usr/include/stdint.h: 175
try:
    INT_LEAST32_MIN = ((-2147483647) - 1)
except:
    pass

# /usr/include/stdint.h: 178
try:
    INT_LEAST8_MAX = 127
except:
    pass

# /usr/include/stdint.h: 179
try:
    INT_LEAST16_MAX = 32767
except:
    pass

# /usr/include/stdint.h: 180
try:
    INT_LEAST32_MAX = 2147483647
except:
    pass

# /usr/include/stdint.h: 184
try:
    UINT_LEAST8_MAX = 255
except:
    pass

# /usr/include/stdint.h: 185
try:
    UINT_LEAST16_MAX = 65535
except:
    pass

# /usr/include/stdint.h: 186
try:
    UINT_LEAST32_MAX = 4294967295
except:
    pass

# /usr/include/stdint.h: 191
try:
    INT_FAST8_MIN = (-128)
except:
    pass

# /usr/include/stdint.h: 193
try:
    INT_FAST16_MIN = ((-9223372036854775807L) - 1)
except:
    pass

# /usr/include/stdint.h: 194
try:
    INT_FAST32_MIN = ((-9223372036854775807L) - 1)
except:
    pass

# /usr/include/stdint.h: 201
try:
    INT_FAST8_MAX = 127
except:
    pass

# /usr/include/stdint.h: 203
try:
    INT_FAST16_MAX = 9223372036854775807L
except:
    pass

# /usr/include/stdint.h: 204
try:
    INT_FAST32_MAX = 9223372036854775807L
except:
    pass

# /usr/include/stdint.h: 212
try:
    UINT_FAST8_MAX = 255
except:
    pass

# /usr/include/stdint.h: 214
try:
    UINT_FAST16_MAX = 18446744073709551615L
except:
    pass

# /usr/include/stdint.h: 215
try:
    UINT_FAST32_MAX = 18446744073709551615L
except:
    pass

# /usr/include/stdint.h: 225
try:
    INTPTR_MIN = ((-9223372036854775807L) - 1)
except:
    pass

# /usr/include/stdint.h: 226
try:
    INTPTR_MAX = 9223372036854775807L
except:
    pass

# /usr/include/stdint.h: 227
try:
    UINTPTR_MAX = 18446744073709551615L
except:
    pass

# /usr/include/stdint.h: 248
try:
    PTRDIFF_MIN = ((-9223372036854775807L) - 1)
except:
    pass

# /usr/include/stdint.h: 249
try:
    PTRDIFF_MAX = 9223372036854775807L
except:
    pass

# /usr/include/stdint.h: 256
try:
    SIG_ATOMIC_MIN = ((-2147483647) - 1)
except:
    pass

# /usr/include/stdint.h: 257
try:
    SIG_ATOMIC_MAX = 2147483647
except:
    pass

# /usr/include/stdint.h: 261
try:
    SIZE_MAX = 18446744073709551615L
except:
    pass

# /usr/include/stdint.h: 269
try:
    WCHAR_MIN = __WCHAR_MIN
except:
    pass

# /usr/include/stdint.h: 270
try:
    WCHAR_MAX = __WCHAR_MAX
except:
    pass

# /usr/include/stdint.h: 274
try:
    WINT_MIN = 0
except:
    pass

# /usr/include/stdint.h: 275
try:
    WINT_MAX = 4294967295
except:
    pass

# /usr/include/stdint.h: 278
def INT8_C(c):
    return c

# /usr/include/stdint.h: 279
def INT16_C(c):
    return c

# /usr/include/stdint.h: 280
def INT32_C(c):
    return c

# /usr/include/stdint.h: 288
def UINT8_C(c):
    return c

# /usr/include/stdint.h: 289
def UINT16_C(c):
    return c

# ../../build/include/csp/csp_autoconfig.h: 6
try:
    GIT_REV = 'v1.2-57-geb1aee6'
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 8
try:
    CSP_POSIX = 1
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 11
try:
    CSP_DEBUG = 1
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 12
try:
    CSP_VERBOSE = 1
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 20
try:
    CSP_CONN_MAX = 10
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 21
try:
    CSP_CONN_QUEUE_LENGTH = 100
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 22
try:
    CSP_FIFO_INPUT = 10
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 23
try:
    CSP_MAX_BIND_PORT = 31
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 24
try:
    CSP_RDP_MAX_WINDOW = 20
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 25
try:
    CSP_PADDING_BYTES = 8
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 26
try:
    CSP_CONNECTION_SO = 0
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 27
try:
    CSP_LOG_LEVEL_DEBUG = 1
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 28
try:
    CSP_LOG_LEVEL_INFO = 1
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 29
try:
    CSP_LOG_LEVEL_WARN = 1
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 30
try:
    CSP_LOG_LEVEL_ERROR = 1
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 31
try:
    CSP_LITTLE_ENDIAN = 1
except:
    pass

# ../../build/include/csp/csp_autoconfig.h: 33
try:
    CSP_HAVE_STDBOOL_H = 1
except:
    pass

bool = c_bool # /usr/lib/gcc/x86_64-linux-gnu/4.8/include/stdbool.h: 33

# /usr/lib/gcc/x86_64-linux-gnu/4.8/include/stdbool.h: 34
try:
    true = 1
except:
    pass

# /usr/lib/gcc/x86_64-linux-gnu/4.8/include/stdbool.h: 35
try:
    false = 0
except:
    pass

# /usr/lib/gcc/x86_64-linux-gnu/4.8/include/stdbool.h: 48
try:
    __bool_true_false_are_defined = 1
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 58
try:
    CSP_PRIORITIES = (1 << CSP_ID_PRIO_SIZE)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 65
try:
    CSP_RX_QUEUE_LENGTH = CSP_CONN_QUEUE_LENGTH
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 66
try:
    CSP_ROUTE_FIFOS = 1
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 67
try:
    CSP_RX_QUEUES = 1
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 71
try:
    CSP_ID_PRIO_SIZE = 2
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 72
try:
    CSP_ID_HOST_SIZE = 5
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 73
try:
    CSP_ID_PORT_SIZE = 6
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 74
try:
    CSP_ID_FLAGS_SIZE = 8
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 76
try:
    CSP_HEADER_BITS = (((CSP_ID_PRIO_SIZE + (2 * CSP_ID_HOST_SIZE)) + (2 * CSP_ID_PORT_SIZE)) + CSP_ID_FLAGS_SIZE)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 77
try:
    CSP_HEADER_LENGTH = (CSP_HEADER_BITS / 8)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 84
try:
    CSP_ID_PRIO_MAX = ((1 << CSP_ID_PRIO_SIZE) - 1)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 85
try:
    CSP_ID_HOST_MAX = ((1 << CSP_ID_HOST_SIZE) - 1)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 86
try:
    CSP_ID_PORT_MAX = ((1 << CSP_ID_PORT_SIZE) - 1)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 87
try:
    CSP_ID_FLAGS_MAX = ((1 << CSP_ID_FLAGS_SIZE) - 1)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 90
try:
    CSP_ID_PRIO_MASK = (CSP_ID_PRIO_MAX << ((CSP_ID_FLAGS_SIZE + (2 * CSP_ID_PORT_SIZE)) + (2 * CSP_ID_HOST_SIZE)))
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 91
try:
    CSP_ID_SRC_MASK = (CSP_ID_HOST_MAX << ((CSP_ID_FLAGS_SIZE + (2 * CSP_ID_PORT_SIZE)) + (1 * CSP_ID_HOST_SIZE)))
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 92
try:
    CSP_ID_DST_MASK = (CSP_ID_HOST_MAX << (CSP_ID_FLAGS_SIZE + (2 * CSP_ID_PORT_SIZE)))
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 93
try:
    CSP_ID_DPORT_MASK = (CSP_ID_PORT_MAX << (CSP_ID_FLAGS_SIZE + (1 * CSP_ID_PORT_SIZE)))
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 94
try:
    CSP_ID_SPORT_MASK = (CSP_ID_PORT_MAX << CSP_ID_FLAGS_SIZE)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 95
try:
    CSP_ID_FLAGS_MASK = (CSP_ID_FLAGS_MAX << 0)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 97
try:
    CSP_ID_CONN_MASK = (((CSP_ID_SRC_MASK | CSP_ID_DST_MASK) | CSP_ID_DPORT_MASK) | CSP_ID_SPORT_MASK)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 124
try:
    CSP_BROADCAST_ADDR = CSP_ID_HOST_MAX
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 127
try:
    CSP_DEFAULT_ROUTE = (CSP_ID_HOST_MAX + 1)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 130
try:
    CSP_FRES1 = 128
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 131
try:
    CSP_FRES2 = 64
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 132
try:
    CSP_FRES3 = 32
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 133
try:
    CSP_FFRAG = 16
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 134
try:
    CSP_FHMAC = 8
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 135
try:
    CSP_FXTEA = 4
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 136
try:
    CSP_FRDP = 2
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 137
try:
    CSP_FCRC32 = 1
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 140
try:
    CSP_SO_NONE = 0
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 141
try:
    CSP_SO_RDPREQ = 1
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 142
try:
    CSP_SO_RDPPROHIB = 2
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 143
try:
    CSP_SO_HMACREQ = 4
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 144
try:
    CSP_SO_HMACPROHIB = 8
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 145
try:
    CSP_SO_XTEAREQ = 16
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 146
try:
    CSP_SO_XTEAPROHIB = 32
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 147
try:
    CSP_SO_CRC32REQ = 64
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 148
try:
    CSP_SO_CRC32PROHIB = 128
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 149
try:
    CSP_SO_CONN_LESS = 256
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 152
try:
    CSP_O_NONE = CSP_SO_NONE
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 153
try:
    CSP_O_RDP = CSP_SO_RDPREQ
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 154
try:
    CSP_O_NORDP = CSP_SO_RDPPROHIB
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 155
try:
    CSP_O_HMAC = CSP_SO_HMACREQ
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 156
try:
    CSP_O_NOHMAC = CSP_SO_HMACPROHIB
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 157
try:
    CSP_O_XTEA = CSP_SO_XTEAREQ
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 158
try:
    CSP_O_NOXTEA = CSP_SO_XTEAPROHIB
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 159
try:
    CSP_O_CRC32 = CSP_SO_CRC32REQ
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 160
try:
    CSP_O_NOCRC32 = CSP_SO_CRC32PROHIB
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 209
try:
    CSP_BUFFER_PACKET_OVERHEAD = (sizeof(csp_packet_t) - sizeof((None.contents.data)))
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 215
try:
    CSP_HOSTNAME_LEN = 20
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 216
try:
    CSP_MODEL_LEN = 30
except:
    pass

CSP_BASE_TYPE = c_int # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_platform.h: 32

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_platform.h: 33
try:
    CSP_MAX_DELAY = UINT32_MAX
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_platform.h: 34
try:
    CSP_INFINITY = UINT32_MAX
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 28
try:
    CSP_ERR_NONE = 0
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 29
try:
    CSP_ERR_NOMEM = (-1)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 30
try:
    CSP_ERR_INVAL = (-2)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 31
try:
    CSP_ERR_TIMEDOUT = (-3)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 32
try:
    CSP_ERR_USED = (-4)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 33
try:
    CSP_ERR_NOTSUP = (-5)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 34
try:
    CSP_ERR_BUSY = (-6)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 35
try:
    CSP_ERR_ALREADY = (-7)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 36
try:
    CSP_ERR_RESET = (-8)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 37
try:
    CSP_ERR_NOBUFS = (-9)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 38
try:
    CSP_ERR_TX = (-10)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 39
try:
    CSP_ERR_DRIVER = (-11)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 40
try:
    CSP_ERR_AGAIN = (-12)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 42
try:
    CSP_ERR_HMAC = (-100)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 43
try:
    CSP_ERR_XTEA = (-101)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_error.h: 44
try:
    CSP_ERR_CRC32 = (-102)
except:
    pass

# /usr/include/inttypes.h: 23
try:
    _INTTYPES_H = 1
except:
    pass

# /usr/include/inttypes.h: 40
try:
    ____gwchar_t_defined = 1
except:
    pass

# /usr/include/inttypes.h: 44
try:
    __PRI64_PREFIX = 'l'
except:
    pass

# /usr/include/inttypes.h: 45
try:
    __PRIPTR_PREFIX = 'l'
except:
    pass

# /usr/include/inttypes.h: 54
try:
    PRId8 = 'd'
except:
    pass

# /usr/include/inttypes.h: 55
try:
    PRId16 = 'd'
except:
    pass

# /usr/include/inttypes.h: 56
try:
    PRId32 = 'd'
except:
    pass

# /usr/include/inttypes.h: 59
try:
    PRIdLEAST8 = 'd'
except:
    pass

# /usr/include/inttypes.h: 60
try:
    PRIdLEAST16 = 'd'
except:
    pass

# /usr/include/inttypes.h: 61
try:
    PRIdLEAST32 = 'd'
except:
    pass

# /usr/include/inttypes.h: 64
try:
    PRIdFAST8 = 'd'
except:
    pass

# /usr/include/inttypes.h: 70
try:
    PRIi8 = 'i'
except:
    pass

# /usr/include/inttypes.h: 71
try:
    PRIi16 = 'i'
except:
    pass

# /usr/include/inttypes.h: 72
try:
    PRIi32 = 'i'
except:
    pass

# /usr/include/inttypes.h: 75
try:
    PRIiLEAST8 = 'i'
except:
    pass

# /usr/include/inttypes.h: 76
try:
    PRIiLEAST16 = 'i'
except:
    pass

# /usr/include/inttypes.h: 77
try:
    PRIiLEAST32 = 'i'
except:
    pass

# /usr/include/inttypes.h: 80
try:
    PRIiFAST8 = 'i'
except:
    pass

# /usr/include/inttypes.h: 86
try:
    PRIo8 = 'o'
except:
    pass

# /usr/include/inttypes.h: 87
try:
    PRIo16 = 'o'
except:
    pass

# /usr/include/inttypes.h: 88
try:
    PRIo32 = 'o'
except:
    pass

# /usr/include/inttypes.h: 91
try:
    PRIoLEAST8 = 'o'
except:
    pass

# /usr/include/inttypes.h: 92
try:
    PRIoLEAST16 = 'o'
except:
    pass

# /usr/include/inttypes.h: 93
try:
    PRIoLEAST32 = 'o'
except:
    pass

# /usr/include/inttypes.h: 96
try:
    PRIoFAST8 = 'o'
except:
    pass

# /usr/include/inttypes.h: 102
try:
    PRIu8 = 'u'
except:
    pass

# /usr/include/inttypes.h: 103
try:
    PRIu16 = 'u'
except:
    pass

# /usr/include/inttypes.h: 104
try:
    PRIu32 = 'u'
except:
    pass

# /usr/include/inttypes.h: 107
try:
    PRIuLEAST8 = 'u'
except:
    pass

# /usr/include/inttypes.h: 108
try:
    PRIuLEAST16 = 'u'
except:
    pass

# /usr/include/inttypes.h: 109
try:
    PRIuLEAST32 = 'u'
except:
    pass

# /usr/include/inttypes.h: 112
try:
    PRIuFAST8 = 'u'
except:
    pass

# /usr/include/inttypes.h: 118
try:
    PRIx8 = 'x'
except:
    pass

# /usr/include/inttypes.h: 119
try:
    PRIx16 = 'x'
except:
    pass

# /usr/include/inttypes.h: 120
try:
    PRIx32 = 'x'
except:
    pass

# /usr/include/inttypes.h: 123
try:
    PRIxLEAST8 = 'x'
except:
    pass

# /usr/include/inttypes.h: 124
try:
    PRIxLEAST16 = 'x'
except:
    pass

# /usr/include/inttypes.h: 125
try:
    PRIxLEAST32 = 'x'
except:
    pass

# /usr/include/inttypes.h: 128
try:
    PRIxFAST8 = 'x'
except:
    pass

# /usr/include/inttypes.h: 134
try:
    PRIX8 = 'X'
except:
    pass

# /usr/include/inttypes.h: 135
try:
    PRIX16 = 'X'
except:
    pass

# /usr/include/inttypes.h: 136
try:
    PRIX32 = 'X'
except:
    pass

# /usr/include/inttypes.h: 139
try:
    PRIXLEAST8 = 'X'
except:
    pass

# /usr/include/inttypes.h: 140
try:
    PRIXLEAST16 = 'X'
except:
    pass

# /usr/include/inttypes.h: 141
try:
    PRIXLEAST32 = 'X'
except:
    pass

# /usr/include/inttypes.h: 144
try:
    PRIXFAST8 = 'X'
except:
    pass

# /usr/include/inttypes.h: 171
try:
    SCNd8 = 'hhd'
except:
    pass

# /usr/include/inttypes.h: 172
try:
    SCNd16 = 'hd'
except:
    pass

# /usr/include/inttypes.h: 173
try:
    SCNd32 = 'd'
except:
    pass

# /usr/include/inttypes.h: 176
try:
    SCNdLEAST8 = 'hhd'
except:
    pass

# /usr/include/inttypes.h: 177
try:
    SCNdLEAST16 = 'hd'
except:
    pass

# /usr/include/inttypes.h: 178
try:
    SCNdLEAST32 = 'd'
except:
    pass

# /usr/include/inttypes.h: 181
try:
    SCNdFAST8 = 'hhd'
except:
    pass

# /usr/include/inttypes.h: 187
try:
    SCNi8 = 'hhi'
except:
    pass

# /usr/include/inttypes.h: 188
try:
    SCNi16 = 'hi'
except:
    pass

# /usr/include/inttypes.h: 189
try:
    SCNi32 = 'i'
except:
    pass

# /usr/include/inttypes.h: 192
try:
    SCNiLEAST8 = 'hhi'
except:
    pass

# /usr/include/inttypes.h: 193
try:
    SCNiLEAST16 = 'hi'
except:
    pass

# /usr/include/inttypes.h: 194
try:
    SCNiLEAST32 = 'i'
except:
    pass

# /usr/include/inttypes.h: 197
try:
    SCNiFAST8 = 'hhi'
except:
    pass

# /usr/include/inttypes.h: 203
try:
    SCNu8 = 'hhu'
except:
    pass

# /usr/include/inttypes.h: 204
try:
    SCNu16 = 'hu'
except:
    pass

# /usr/include/inttypes.h: 205
try:
    SCNu32 = 'u'
except:
    pass

# /usr/include/inttypes.h: 208
try:
    SCNuLEAST8 = 'hhu'
except:
    pass

# /usr/include/inttypes.h: 209
try:
    SCNuLEAST16 = 'hu'
except:
    pass

# /usr/include/inttypes.h: 210
try:
    SCNuLEAST32 = 'u'
except:
    pass

# /usr/include/inttypes.h: 213
try:
    SCNuFAST8 = 'hhu'
except:
    pass

# /usr/include/inttypes.h: 219
try:
    SCNo8 = 'hho'
except:
    pass

# /usr/include/inttypes.h: 220
try:
    SCNo16 = 'ho'
except:
    pass

# /usr/include/inttypes.h: 221
try:
    SCNo32 = 'o'
except:
    pass

# /usr/include/inttypes.h: 224
try:
    SCNoLEAST8 = 'hho'
except:
    pass

# /usr/include/inttypes.h: 225
try:
    SCNoLEAST16 = 'ho'
except:
    pass

# /usr/include/inttypes.h: 226
try:
    SCNoLEAST32 = 'o'
except:
    pass

# /usr/include/inttypes.h: 229
try:
    SCNoFAST8 = 'hho'
except:
    pass

# /usr/include/inttypes.h: 235
try:
    SCNx8 = 'hhx'
except:
    pass

# /usr/include/inttypes.h: 236
try:
    SCNx16 = 'hx'
except:
    pass

# /usr/include/inttypes.h: 237
try:
    SCNx32 = 'x'
except:
    pass

# /usr/include/inttypes.h: 240
try:
    SCNxLEAST8 = 'hhx'
except:
    pass

# /usr/include/inttypes.h: 241
try:
    SCNxLEAST16 = 'hx'
except:
    pass

# /usr/include/inttypes.h: 242
try:
    SCNxLEAST32 = 'x'
except:
    pass

# /usr/include/inttypes.h: 245
try:
    SCNxFAST8 = 'hhx'
except:
    pass

# /usr/include/string.h: 23
try:
    _STRING_H = 1
except:
    pass

# /usr/include/xlocale.h: 21
try:
    _XLOCALE_H = 1
except:
    pass

# /usr/include/string.h: 433
try:
    strerror_r = __xpg_strerror_r
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_debug.h: 81
def CONSTSTR(data):
    return data

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 26
try:
    CSP_NODE_MAC = 255
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 27
try:
    CSP_ROUTE_COUNT = (CSP_ID_HOST_MAX + 2)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 28
try:
    CSP_ROUTE_TABLE_SIZE = (5 * CSP_ROUTE_COUNT)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/csp_rtable.h: 129
def csp_route_set(node, ifc, mac):
    return (csp_rtable_set (node, CSP_ID_HOST_SIZE, ifc, mac))

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 35
try:
    E_NO_ERR = (-1)
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 40
try:
    I2C_MTU = 256
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 45
try:
    I2C_MASTER = 0
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 46
try:
    I2C_SLAVE = 1
except:
    pass

# ../../include/csp/csp_interface.h: 53
try:
    csp_new_packet = csp_qfifo_write
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_can.h: 34
try:
    CSP_CAN_MASKED = 0
except:
    pass

# /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_can.h: 35
try:
    CSP_CAN_PROMISC = 1
except:
    pass

csp_iface_s = struct_csp_iface_s # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 184

csp_conn_s = struct_csp_conn_s # /home/johan/git/pygnd/lib/libcsp/include/csp/csp_types.h: 212

__locale_data = struct___locale_data # /usr/include/xlocale.h: 30

__locale_struct = struct___locale_struct # /usr/include/xlocale.h: 27

i2c_frame_s = struct_i2c_frame_s # /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/i2c.h: 59

usart_conf = struct_usart_conf # /home/johan/git/pygnd/lib/libcsp/include/csp/drivers/usart.h: 36

csp_can_config = struct_csp_can_config # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_can.h: 40

csp_kiss_handle_s = struct_csp_kiss_handle_s # /home/johan/git/pygnd/lib/libcsp/include/csp/interfaces/csp_if_kiss.h: 95

# No inserted files

