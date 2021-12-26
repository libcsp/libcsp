#!/usr/bin/env python

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
    print("RDP:              {0}".format("Yes" if ((hdrhex >> 1) & 0x01) else "No"))
    print("CRC32:            {0}".format("Yes" if ((hdrhex >> 0) & 0x01) else "No"))


if __name__ == "__main__":
    main()
