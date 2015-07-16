import os
import fnmatch
from cffi import FFI


MODULE_DIR = os.path.dirname(__file__)
CSP_ROOT_DIR = os.path.abspath(os.path.join(MODULE_DIR, os.path.pardir,
                                            os.path.pardir))
CSP_INCLUDE_DIR = os.path.join(CSP_ROOT_DIR, 'include')
INCLUDE_DIRS = [CSP_INCLUDE_DIR]
INCLUDE_DIRS += [os.path.join(CSP_ROOT_DIR, 'build', 'include')]
EXPORTED_SYMBOLS_PATH = os.path.join(MODULE_DIR, 'cffi_csp_exported_symbols.h')
ffi = FFI()


def get_includes(all_source_files):
    possible_headers = []
    for source in all_source_files:
        possible_headers.append(os.path.basename(source).replace('.c', '.h'))
    headers = []
    for root, _, files in os.walk(CSP_ROOT_DIR, topdown=True):
        for filename in files:
            if fnmatch.filter(possible_headers, filename):
                filename = os.path.join(root, filename)
                if CSP_INCLUDE_DIR in os.path.join(root, filename):
                    header = os.path.relpath(filename, CSP_INCLUDE_DIR)
                    headers.append(header)
    return headers


def ffi_compile(files, include_dirs=None, compile_args=None, output_dir=None):
    include_dirs = include_dirs or []
    extra_compile_args = compile_args or []
    output_dir = output_dir or os.path.join(CSP_ROOT_DIR, 'build', 'python')
    content = '#include "csp/csp_autoconfig.h"\n'
    all_headers = ['csp/csp.h', 'csp/csp_types.h'] + get_includes(files.split())
    for include in all_headers:
        content += '#include "{}"\n'.format(include)
    include_dirs += INCLUDE_DIRS
    library_dirs = [os.path.join(output_dir, os.path.pardir)]
    ffi.set_source('csp', content, include_dirs=include_dirs,
                   extra_compile_args=extra_compile_args, libraries=['csp'],
                   library_dirs=library_dirs)
    exported_symbols = open(EXPORTED_SYMBOLS_PATH).read()
    ffi.cdef(exported_symbols, packed=True)
    ffi.compile(output_dir)
