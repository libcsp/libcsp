#!/usr/bin/env python

# Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
# Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
# Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# Split CFP header in protocol fields

import sys


def usage():
    print("usage: cfpsplit.py HEADER")


def main():
    if len(sys.argv) != 2:
        usage()
        sys.exit(-1)

    try:
        hdrhex = int(sys.argv[1], 16)
    except Exception:
        print("HEADER must be in hexadecimal format")
        sys.exit(-1)

    if hdrhex > 0x1fffffff:
        print("HEADER is not a valid CFP header")
        sys.exit(-1)

    print("Source:           {0}".format((hdrhex >> 24) & 0x1f))
    print("Destination:      {0}".format((hdrhex >> 19) & 0x1f))
    print("Type:             {0}".format("MORE" if ((hdrhex >> 18) & 0x01) else "BEGIN"))
    print("Remain:           {0}".format((hdrhex >> 10) & 0xff))
    print("Identifier:       {0}".format((hdrhex >> 0) & 0x3ff))


if __name__ == "__main__":
    main()
