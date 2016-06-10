#!/usr/bin/pytho
from csp import lib as csp
from csp import ffi

csp.csp_set_hostname("hest")
print ffi.string(csp.csp_get_hostname())




