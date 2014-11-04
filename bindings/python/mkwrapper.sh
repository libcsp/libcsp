#!/bin/sh
ctypesgen.py \
-I../../build/include \
-I../../include \
../../include/csp/*.h \
../../include/csp/drivers/*.h \
../../include/csp/interfaces/*.h \
-lcsp \
-o pycspauto.py
