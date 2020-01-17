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

# Split CSP header in protocol fields

import sys


def usage():
    print("usage: cspsplit.py HEADER")


def main():
    if len(sys.argv) != 2:
        usage()
        sys.exit(-1)

    try:
        hdrhex = int(sys.argv[1], 16)
    except Exception:
        print("HEADER must be in hexadecimal format")
        sys.exit(-1)

    print("Priotity:         {0}".format((hdrhex >> 30) & 0x03))
    print("Source:           {0}".format((hdrhex >> 25) & 0x1f))
    print("Destination:      {0}".format((hdrhex >> 20) & 0x1f))
    print("Destination port: {0}".format((hdrhex >> 14) & 0x3f))
    print("Source port:      {0}".format((hdrhex >> 8) & 0x3f))
    print("HMAC:             {0}".format("Yes" if ((hdrhex >> 3) & 0x01) else "No"))
    print("XTEA:             {0}".format("Yes" if ((hdrhex >> 2) & 0x01) else "No"))
    print("RDP:              {0}".format("Yes" if ((hdrhex >> 1) & 0x01) else "No"))
    print("CRC32:            {0}".format("Yes" if ((hdrhex >> 0) & 0x01) else "No"))


if __name__ == "__main__":
    main()
