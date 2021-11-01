#!/usr/bin/env python

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
