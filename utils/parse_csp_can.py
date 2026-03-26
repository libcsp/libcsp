#!/usr/bin/env python3
"""
parse_csp_can.py — CSP CAN frame parser for candump log files.

Parses CSP (CubeSat Space Protocol) v1 or v2 frames from candump-format logs,
reassembles fragmented packets, and prints decoded messages.

Usage:
    parse_csp_can.py [OPTIONS] LOGFILE [LOGFILE ...]

See --help for full options.
"""

import argparse
import re
import struct
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Line parsing
# ---------------------------------------------------------------------------

# Accepts:
#   (timestamp)  iface  CANID  [DLC]  byte byte ...
#   CANID  [DLC]  byte byte ...
_LINE_RE = re.compile(
    r"^\s*"
    r"(?:\((?P<ts>[0-9]+\.[0-9]+)\)\s+)?"                    # optional ts
    r"(?:(?P<iface>[a-zA-Z]\S*)\s+)?"                        # optional iface
    r"(?P<can_id>[0-9A-Fa-f]{3,8})"                          # CAN ID (hex)
    r"\s+\[(?P<dlc>[0-8])\]"                                  # DLC
    r"(?P<data>(?:\s+[0-9A-Fa-f]{2})*)"                      # data bytes
    r"\s*$"
)


def parse_line(line: str) -> Optional[Tuple[Optional[str], Optional[str], int, bytes]]:
    """Parse a single candump line.

    Returns (timestamp, iface, can_id_int, data_bytes) or None if not a data frame.
    """
    m = _LINE_RE.match(line)
    if not m:
        return None
    ts = m.group("ts")
    iface = m.group("iface")
    can_id = int(m.group("can_id"), 16)
    data_str = m.group("data").strip()
    data = bytes(int(b, 16) for b in data_str.split()) if data_str else b""
    return ts, iface, can_id, data


# ---------------------------------------------------------------------------
# CFP v1 field extraction  (from csp_if_can.h / csp_id.c)
# ---------------------------------------------------------------------------

@dataclass
class CFP1Fields:
    src: int
    dst: int
    frame_type: int   # 0=BEGIN, 1=MORE
    remain: int
    ident: int


def decode_cfp_v1(can_id: int) -> CFP1Fields:
    # Bit layout (28..0):
    #   28-24  src (5)
    #   23-19  dst (5)
    #   18     type (1)
    #   17-10  remain (8)
    #   9-0    ident (10)
    src   = (can_id >> 24) & 0x1F
    dst   = (can_id >> 19) & 0x1F
    ftype = (can_id >> 18) & 0x01
    remain = (can_id >> 10) & 0xFF
    ident = (can_id >>  0) & 0x3FF
    return CFP1Fields(src=src, dst=dst, frame_type=ftype, remain=remain, ident=ident)


# ---------------------------------------------------------------------------
# CFP v2 field extraction  (from csp_if_can.h)
# ---------------------------------------------------------------------------

@dataclass
class CFP2Fields:
    pri: int
    dst: int
    sender: int
    pkt_cnt: int
    frm_cnt: int
    begin: int
    end: int


def decode_cfp_v2(can_id: int) -> CFP2Fields:
    # Bit layout (28..0):
    #   28-27  pri (2)
    #   26-13  dst (14)
    #   12-7   sender (6)
    #   6-5    pkt_cnt (2)
    #   4-2    frm_cnt (3)
    #   1      begin (1)
    #   0      end (1)
    pri     = (can_id >> 27) & 0x03
    dst     = (can_id >> 13) & 0x3FFF
    sender  = (can_id >>  7) & 0x3F
    pkt_cnt = (can_id >>  5) & 0x03
    frm_cnt = (can_id >>  2) & 0x07
    begin   = (can_id >>  1) & 0x01
    end     = (can_id >>  0) & 0x01
    return CFP2Fields(pri=pri, dst=dst, sender=sender,
                      pkt_cnt=pkt_cnt, frm_cnt=frm_cnt, begin=begin, end=end)


# ---------------------------------------------------------------------------
# CSP header parsing (payload bytes)
# ---------------------------------------------------------------------------

@dataclass
class CSPHeader:
    pri: int
    src: int
    dst: int
    sport: int
    dport: int
    flags: int


def decode_csp_v1_header(data: bytes) -> Optional[Tuple[CSPHeader, int]]:
    """Parse 4-byte CSP v1 header + 2-byte length from BEGIN frame payload.

    Returns (CSPHeader, declared_length) or None if data too short.
    """
    if len(data) < 6:
        return None
    raw = struct.unpack_from(">I", data, 0)[0]  # big-endian uint32
    length = struct.unpack_from(">H", data, 4)[0]
    hdr = CSPHeader(
        pri   = (raw >> 30) & 0x03,
        src   = (raw >> 25) & 0x1F,
        dst   = (raw >> 20) & 0x1F,
        dport = (raw >> 14) & 0x3F,
        sport = (raw >>  8) & 0x3F,
        flags = (raw >>  0) & 0xFF,
    )
    return hdr, length


def decode_csp_v2_ext(data: bytes) -> Optional[CSPHeader]:
    """Parse 4-byte extended header from v2 BEGIN frame payload.

    These 4 bytes carry: src(14), dport(6), sport(6), flags(6) — 32 bits.
    The dst and pri come from the CAN ID CFP fields.
    Returns a partial CSPHeader (dst/pri set to 0; caller fills them in).
    """
    if len(data) < 4:
        return None
    raw = struct.unpack_from(">I", data, 0)[0]
    return CSPHeader(
        pri   = 0,           # filled in by caller from CFP fields
        src   = (raw >> 18) & 0x3FFF,
        dst   = 0,           # filled in by caller
        dport = (raw >> 12) & 0x3F,
        sport = (raw >>  6) & 0x3F,
        flags = (raw >>  0) & 0x3F,
    )


# ---------------------------------------------------------------------------
# Flags helpers
# ---------------------------------------------------------------------------

FLAG_CRC32 = 0x01
FLAG_RDP   = 0x02
FLAG_HMAC  = 0x08
FLAG_FRAG  = 0x10


def flags_str(flags: int) -> str:
    parts = [
        f"HMAC={'Y' if flags & FLAG_HMAC else 'N'}",
        f"RDP={'Y' if flags & FLAG_RDP else 'N'}",
        f"CRC={'Y' if flags & FLAG_CRC32 else 'N'}",
        f"FRAG={'Y' if flags & FLAG_FRAG else 'N'}",
    ]
    return " ".join(parts)


# ---------------------------------------------------------------------------
# Reassembled CSP message
# ---------------------------------------------------------------------------

@dataclass
class CSPMessage:
    line_no: int               # line number of BEGIN frame in source file
    filename: str
    timestamp: Optional[str]
    iface: Optional[str]
    hdr: CSPHeader
    payload: bytes


# ---------------------------------------------------------------------------
# Fragment assembler
# ---------------------------------------------------------------------------

@dataclass
class _FragBuf:
    line_no: int
    filename: str
    timestamp: Optional[str]
    iface: Optional[str]
    hdr: CSPHeader
    data: bytearray = field(default_factory=bytearray)
    # v1 only: expected remaining frame count
    remain: int = 0


class Assembler:
    def __init__(self, version: int):
        self._version = version
        # key → _FragBuf
        self._bufs: Dict[tuple, _FragBuf] = {}

    def feed(
        self,
        line_no: int,
        filename: str,
        ts: Optional[str],
        iface: Optional[str],
        can_id: int,
        data: bytes,
    ) -> Optional[CSPMessage]:
        """Process one CAN frame. Returns a CSPMessage when a packet is complete."""
        if self._version == 2:
            return self._feed_v2(line_no, filename, ts, iface, can_id, data)
        else:
            return self._feed_v1(line_no, filename, ts, iface, can_id, data)

    def flush_incomplete(self) -> List[Tuple[tuple, _FragBuf]]:
        return list(self._bufs.items())

    # --- v2 ---

    def _feed_v2(self, line_no, filename, ts, iface, can_id, data):
        cfp = decode_cfp_v2(can_id)
        key = (cfp.dst, cfp.sender, cfp.pkt_cnt)

        if cfp.begin:
            hdr = decode_csp_v2_ext(data)
            if hdr is None:
                return None  # malformed
            hdr.dst = cfp.dst
            hdr.pri = cfp.pri
            payload = bytearray(data[4:])  # bytes after ext header
            buf = _FragBuf(
                line_no=line_no, filename=filename,
                timestamp=ts, iface=iface, hdr=hdr, data=payload,
            )
            if cfp.end:
                # single-frame packet
                return CSPMessage(
                    line_no=buf.line_no, filename=buf.filename,
                    timestamp=buf.timestamp, iface=buf.iface,
                    hdr=buf.hdr, payload=bytes(buf.data),
                )
            self._bufs[key] = buf
            return None

        # continuation / end frame
        buf = self._bufs.get(key)
        if buf is None:
            # No BEGIN seen — skip orphan fragment silently
            return None
        buf.data.extend(data)

        if cfp.end:
            del self._bufs[key]
            return CSPMessage(
                line_no=buf.line_no, filename=buf.filename,
                timestamp=buf.timestamp, iface=buf.iface,
                hdr=buf.hdr, payload=bytes(buf.data),
            )
        return None

    # --- v1 ---

    def _feed_v1(self, line_no, filename, ts, iface, can_id, data):
        cfp = decode_cfp_v1(can_id)
        key = (cfp.src, cfp.dst, cfp.ident)

        if cfp.frame_type == 0:  # CFP_BEGIN
            parsed = decode_csp_v1_header(data)
            if parsed is None:
                return None
            hdr, _declared_len = parsed
            payload = bytearray(data[6:])  # bytes after 4B header + 2B length
            buf = _FragBuf(
                line_no=line_no, filename=filename,
                timestamp=ts, iface=iface, hdr=hdr, data=payload,
                remain=cfp.remain,
            )
            if cfp.remain == 0:
                return CSPMessage(
                    line_no=buf.line_no, filename=buf.filename,
                    timestamp=buf.timestamp, iface=buf.iface,
                    hdr=buf.hdr, payload=bytes(buf.data),
                )
            self._bufs[key] = buf
            return None

        # CFP_MORE
        buf = self._bufs.get(key)
        if buf is None:
            return None
        buf.data.extend(data)
        buf.remain -= 1
        if buf.remain == 0:
            del self._bufs[key]
            return CSPMessage(
                line_no=buf.line_no, filename=buf.filename,
                timestamp=buf.timestamp, iface=buf.iface,
                hdr=buf.hdr, payload=bytes(buf.data),
            )
        return None


# ---------------------------------------------------------------------------
# Filters
# ---------------------------------------------------------------------------

def apply_filters(msg: CSPMessage, args: argparse.Namespace) -> bool:
    h = msg.hdr
    if args.src is not None and h.src != args.src:
        return False
    if args.dst is not None and h.dst != args.dst:
        return False
    if args.sport is not None and h.sport != args.sport:
        return False
    if args.dport is not None and h.dport != args.dport:
        return False
    if args.node is not None and h.src != args.node and h.dst != args.node:
        return False
    if args.port is not None and h.sport != args.port and h.dport != args.port:
        return False
    return True


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def fmt_hexdump(data: bytes, indent: str = "  ") -> str:
    """Classic hex dump: offset  hex bytes (padded to 16)  |ascii|"""
    lines = []
    for i in range(0, max(len(data), 1), 16):
        chunk = data[i:i + 16]
        hex_part = " ".join(f"{b:02X}" for b in chunk)
        # two groups of 8
        if len(chunk) > 8:
            hex_part = hex_part[:23] + "  " + hex_part[23:]
        hex_part = f"{hex_part:<50}"
        ascii_part = "".join(chr(b) if 0x20 <= b < 0x7F else "." for b in chunk)
        lines.append(f"{indent}{i:04X}  {hex_part}  {ascii_part}")
    return "\n".join(lines)


def fmt_console(msg: CSPMessage) -> str:
    h = msg.hdr
    ts_part = f"  ({msg.timestamp})" if msg.timestamp else ""
    iface_part = f"  {msg.iface}" if msg.iface else ""
    header = f"#{msg.line_no}{ts_part}{iface_part}"
    fields = (
        f"  src={h.src}  dst={h.dst}  sport={h.sport}  dport={h.dport}"
        f"  pri={h.pri}  flags=0x{h.flags:02X} [{flags_str(h.flags)}]"
    )
    dump = fmt_hexdump(msg.payload)
    return f"{header}\n{fields}\n  Payload ({len(msg.payload)} bytes):\n{dump}"


_PRIORITY_NAMES = {0: "CRITICAL", 1: "HIGH", 2: "NORM", 3: "LOW"}


def fmt_stats(messages: List["CSPMessage"]) -> str:
    """Build a traffic summary block from a list of decoded messages."""
    if not messages:
        return "## Traffic Summary\n  (no messages)\n"

    src_counts: Counter = Counter()
    dst_counts: Counter = Counter()
    port_pair_counts: Counter = Counter()
    payload_sizes: List[int] = []

    for msg in messages:
        h = msg.hdr
        src_counts[h.src] += 1
        dst_counts[h.dst] += 1
        port_pair_counts[(h.sport, h.dport)] += 1
        payload_sizes.append(len(msg.payload))

    total = len(messages)
    src_nodes = ", ".join(str(n) for n in sorted(src_counts))
    dst_nodes = ", ".join(str(n) for n in sorted(dst_counts))
    min_sz = min(payload_sizes)
    max_sz = max(payload_sizes)
    avg_sz = sum(payload_sizes) / total

    lines = [
        "## Traffic Summary",
        f"  Total messages  : {total}",
        f"  Unique src nodes: {src_nodes}",
        f"  Unique dst nodes: {dst_nodes}",
        "  Active port pairs (sport→dport : count):",
    ]
    for (sp, dp), cnt in sorted(port_pair_counts.items(), key=lambda x: -x[1]):
        lines.append(f"    {sp:>3}→{dp:<3}  : {cnt}")
    lines.append(f"  Payload sizes   : min={min_sz} B  max={max_sz} B  avg={avg_sz:.1f} B")
    lines.append("")
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# Explode-mode helpers
# ---------------------------------------------------------------------------

def group_messages_for_explode(
    messages: List[CSPMessage],
) -> Dict[Tuple[str, int, int], List[CSPMessage]]:
    """Group messages by (source logfile path, node_a, node_b).

    node_a = min(src, dst) and node_b = max(src, dst) so that traffic in both
    directions of the same conversation lands in the same bucket/folder.
    """
    groups: Dict[Tuple[str, int, int], List[CSPMessage]] = defaultdict(list)
    for msg in messages:
        a, b = sorted((msg.hdr.src, msg.hdr.dst))
        groups[(msg.filename, a, b)].append(msg)
    return dict(groups)


def write_explode_group(
    path: Path,
    msgs: List[CSPMessage],
    args: argparse.Namespace,
    source_logfile: str,
) -> None:
    """Write one (node_a, node_b) group's output file under *path*."""
    path.mkdir(parents=True, exist_ok=True)

    if args.ai_export:
        outname = path / "messages.ai.txt"
    elif args.markdown:
        outname = path / "messages.md"
    else:
        outname = path / "messages.txt"

    stats = fmt_stats(msgs)
    with outname.open("w", encoding="utf-8") as f:
        if args.ai_export:
            f.write(fmt_ai_header(msgs, args, [source_logfile]))
            f.write(_AI_PROTOCOL_REF)
            f.write(stats)
            f.write(_AI_MSG_FORMAT_LEGEND)
            for msg in msgs:
                f.write(fmt_ai_msg(msg))
                f.write("\n\n")
        elif args.markdown:
            f.write(stats)
            f.write("\n")
            f.write(_MD_HEADER)
            for msg in msgs:
                f.write(fmt_markdown_row(msg))
        else:
            f.write(stats)
            for msg in msgs:
                f.write(fmt_console(msg))
                f.write("\n\n")


# ---------------------------------------------------------------------------
# AI export — static protocol reference
# ---------------------------------------------------------------------------

_AI_PROTOCOL_REF = """\
## CSP / CAN Protocol Reference

### Overview
CubeSat Space Protocol (CSP) is a small-network protocol designed for CubeSat systems.
On CAN bus, CSP packets are carried by the CAN Fragmentation Protocol (CFP), which
uses the 29-bit extended CAN identifier to carry fragmentation metadata.  Every logical
CSP message may span one or more CAN frames; this file contains only fully-reassembled
messages.

### CSP v2 — CAN ID field layout (29-bit extended identifier)
Bits are numbered from the MSB of the 29-bit field (bit 28) downward.

  Bits 28-27  Priority    (2 bits)  0=CRITICAL  1=HIGH  2=NORM  3=LOW
  Bits 26-13  Destination (14 bits) CSP node address of the receiver
  Bits 12-7   Sender ID   (6 bits)  LS bits of the transmitting node address
  Bits  6-5   Pkt counter (2 bits)  Increments per CSP packet; wraps at 4
  Bits  4-2   Frm counter (3 bits)  Fragment index within the packet
  Bit   1     BEGIN flag  (1 bit)   Set on the first CAN frame of a CSP packet
  Bit   0     END flag    (1 bit)   Set on the last CAN frame of a CSP packet

### CSP v2 — First 4 payload bytes of BEGIN frame (extended header)
These bytes carry the remaining CSP header fields not present in the CAN ID.

  Bits 31-18  Source      (14 bits) CSP node address of the sender
  Bits 17-12  Dest port   (6 bits)  Destination service port (0–63)
  Bits 11-6   Src port    (6 bits)  Source (ephemeral) port (0–63)
  Bits  5-0   Flags       (6 bits)  See flag table below

### CSP v1 — CAN ID field layout (29-bit extended identifier)
  Bits 28-24  Source      (5 bits)  CSP node address of the sender
  Bits 23-19  Destination (5 bits)  CSP node address of the receiver
  Bit  18     Type        (1 bit)   0=BEGIN  1=MORE (continuation frame)
  Bits 17-10  Remain      (8 bits)  Remaining fragment count (decrements)
  Bits  9-0   Identifier  (10 bits) Session identifier (auto-incrementing)

### CSP v1 — BEGIN frame payload layout
  Bytes 0-3   CSP header  (32 bits big-endian)
                Bits 31-30  Priority   (2 bits)
                Bits 29-25  Source     (5 bits)
                Bits 24-20  Destination(5 bits)
                Bits 19-14  Dest port  (6 bits)
                Bits 13-8   Src port   (6 bits)
                Bits  7-0   Flags      (8 bits)
  Bytes 4-5   Data length (16 bits big-endian) — payload byte count
  Bytes 6-7   First up to 2 bytes of payload data

### CSP header flags
  Bit 0  CRC32  CRC-32 checksum appended to payload (last 4 bytes); NOT stripped here
  Bit 1  RDP    Reliable Datagram Protocol connection
  Bit 3  HMAC   HMAC authentication tag present
  Bit 4  FRAG   Packet was fragmented at the CSP layer (above CAN fragmentation)

### Well-known destination ports
  Port  1   ping          CSP built-in echo/ping service
  Port  2   info          Memory and system information
  Port  4   reboot        Reboot command
  Port  5   routing       Route table management
  Port  8   param         Parameter / management service
  Port 12   FP/UDP        EnduroSat File Protocol over CSP UDP (MacProtoId_FP)
  Port 13   FP/RDP        EnduroSat File Protocol over CSP RDP (MacProtoId_Service)
  17–63     ephemeral     Dynamically assigned outgoing ports (base = CSP_PORT_MAX_BIND+1 = 17)

### Payload representation
Payload bytes are the reassembled CSP data (excluding the CSP header itself).
They are shown as space-separated hex pairs.  When the CRC flag is set, the last
4 bytes of the payload are the CRC-32 checksum and have NOT been stripped.

"""

_AI_MSG_FORMAT_LEGEND = """\
## Message Records
Each entry is one fully-reassembled CSP packet (aggregated from 1 or more CAN frames).

Record format:
  MSG #<line> | t=<unix-timestamp> | if=<can-interface>
    src=<node> dst=<node> sport=<port> dport=<port> pri=<n>(<name>) flags=0x<HH>[<active-flag-names>]
    payload(<N> bytes): <HH HH HH ...>

Field meanings:
  line    Line number of the first (BEGIN) CAN frame in the source log file
  t       Unix timestamp with microsecond precision from candump capture
  if      CAN interface name (e.g. can0)
  src     CSP source node address
  dst     CSP destination node address
  sport   Source (ephemeral) port; base 17, wraps at 63 due to 6-bit field
  dport   Destination service port (see port table above)
  pri     Priority level: numeric value followed by name in parentheses
  flags   Hex byte of active flags, followed by active flag names in brackets
  payload Reassembled payload in hex; byte count in parentheses

"""


def fmt_ai_header(messages: List["CSPMessage"], args: argparse.Namespace, sources: List[str]) -> str:
    ifaces = sorted({msg.iface for msg in messages if msg.iface})
    iface_str = ", ".join(ifaces) if ifaces else "unknown"

    timestamps = [float(msg.timestamp) for msg in messages if msg.timestamp]
    if timestamps:
        t_min, t_max = min(timestamps), max(timestamps)
        span = f"{t_min:.6f} – {t_max:.6f}  ({t_max - t_min:.1f} s)"
    else:
        span = "unknown"

    filter_parts = []
    for attr, label in [("src", "src"), ("dst", "dst"), ("sport", "sport"),
                        ("dport", "dport"), ("node", "node"), ("port", "port")]:
        val = getattr(args, attr, None)
        if val is not None:
            filter_parts.append(f"{label}={val}")
    filters_str = ", ".join(filter_parts) if filter_parts else "none"

    src_files = "\n#   ".join(sources)
    return (
        f"# CSP CAN Traffic Analysis\n"
        f"# Generated by  : parse_csp_can.py\n"
        f"# Source file(s) :\n#   {src_files}\n"
        f"# Interface(s)  : {iface_str}\n"
        f"# CSP version   : {args.csp_version}\n"
        f"# Filters       : {filters_str}\n"
        f"# Messages      : {len(messages)}  |  Time span: {span}\n"
        f"\n"
    )


def _active_flags(flags: int) -> str:
    names = []
    if flags & FLAG_CRC32:
        names.append("CRC")
    if flags & FLAG_RDP:
        names.append("RDP")
    if flags & FLAG_HMAC:
        names.append("HMAC")
    if flags & FLAG_FRAG:
        names.append("FRAG")
    label = " ".join(names) if names else "none"
    return f"0x{flags:02X}[{label}]"


def fmt_ai_msg(msg: "CSPMessage") -> str:
    h = msg.hdr
    ts_part = f"t={msg.timestamp}" if msg.timestamp else "t=?"
    iface_part = f"if={msg.iface}" if msg.iface else "if=?"
    pri_name = _PRIORITY_NAMES.get(h.pri, str(h.pri))
    hex_payload = " ".join(f"{b:02X}" for b in msg.payload)
    return (
        f"MSG #{msg.line_no} | {ts_part} | {iface_part}\n"
        f"  src={h.src} dst={h.dst} sport={h.sport} dport={h.dport}"
        f" pri={h.pri}({pri_name}) flags={_active_flags(h.flags)}\n"
        f"  payload({len(msg.payload)} bytes): {hex_payload}"
    )


_MD_HEADER = (
    "| Line | File | Timestamp | Iface | Src | Dst | SPrt | DPrt | Pri | Flags"
    " | Len | Payload (hex) | ASCII |\n"
    "|------|------|-----------|-------|-----|-----|------|------|-----|-------"
    "|-----|---------------|-------|\n"
)

_PAYLOAD_LIMIT = 32  # bytes shown in markdown table


def _escape_md(s: str) -> str:
    return s.replace("|", "\\|")


def fmt_markdown_row(msg: CSPMessage) -> str:
    h = msg.hdr
    ts = msg.timestamp or ""
    iface = msg.iface or ""
    fname = msg.filename

    trunc = msg.payload[:_PAYLOAD_LIMIT]
    hex_str = " ".join(f"{b:02X}" for b in trunc)
    if len(msg.payload) > _PAYLOAD_LIMIT:
        hex_str += " …"
    ascii_str = "".join(chr(b) if 0x20 <= b < 0x7F else "." for b in trunc)
    if len(msg.payload) > _PAYLOAD_LIMIT:
        ascii_str += "…"

    return (
        f"| {msg.line_no} | {_escape_md(fname)} | {ts} | {iface}"
        f" | {h.src} | {h.dst} | {h.sport} | {h.dport}"
        f" | {h.pri} | 0x{h.flags:02X} | {len(msg.payload)}"
        f" | {_escape_md(hex_str)} | {_escape_md(ascii_str)} |\n"
    )


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Parse CSP CAN frames from candump log files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s run1_image_bin.log
  %(prog)s --csp-version 1 run1_image_bin.log
  %(prog)s --src 3 --dport 32 run1_image_bin.log
  %(prog)s --node 5 run1_image_bin.log
  %(prog)s --markdown -o report.md run1_image_bin.log run2_image_bin.log
""",
    )
    p.add_argument("logfiles", metavar="LOGFILE", nargs="+", help="candump log file(s)")
    p.add_argument(
        "--csp-version", dest="csp_version", type=int, choices=[1, 2], default=2,
        help="CSP version for decoding (default: 2)",
    )
    p.add_argument("--src", type=int, metavar="NODE", help="Filter: source node address")
    p.add_argument("--dst", type=int, metavar="NODE", help="Filter: destination node address")
    p.add_argument("--sport", type=int, metavar="PORT", help="Filter: source port")
    p.add_argument("--dport", type=int, metavar="PORT", help="Filter: destination port")
    p.add_argument(
        "--node", type=int, metavar="NODE",
        help="Filter: source OR destination node matches NODE",
    )
    p.add_argument(
        "--port", type=int, metavar="PORT",
        help="Filter: source OR destination port matches PORT",
    )
    mode = p.add_mutually_exclusive_group()
    mode.add_argument(
        "--markdown", action="store_true",
        help="Output a Markdown table instead of console view",
    )
    mode.add_argument(
        "--ai-export", dest="ai_export", action="store_true", default=False,
        help="Write self-contained AI analysis file (use -o for filename)",
    )
    p.add_argument(
        "-o", metavar="FILE", dest="outfile", default=None,
        help="Write output to FILE (default: stdout)",
    )
    p.add_argument(
        "--explode", action="store_true", default=False,
        help=(
            "Split output into per-node-pair files under <logfile_stem>/<a>-<b>/ "
            "where a=min(src,dst) and b=max(src,dst). Both directions of a "
            "conversation land in the same folder. Incompatible with -o (ignored)."
        ),
    )
    return p.parse_args()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    args = parse_args()
    if args.explode and args.outfile:
        print("WARNING: --explode active; -o FILE ignored.", file=sys.stderr)
        args.outfile = None
    out = open(args.outfile, "w", encoding="utf-8") if args.outfile else sys.stdout

    try:
        assembler = Assembler(version=args.csp_version)
        messages: List[CSPMessage] = []

        for logfile in args.logfiles:
            try:
                fh = open(logfile, encoding="utf-8", errors="replace")
            except OSError as e:
                print(f"ERROR: cannot open {logfile}: {e}", file=sys.stderr)
                continue

            with fh:
                for line_no, raw_line in enumerate(fh, start=1):
                    parsed = parse_line(raw_line)
                    if parsed is None:
                        continue
                    ts, iface, can_id, data = parsed
                    msg = assembler.feed(line_no, logfile, ts, iface, can_id, data)
                    if msg is not None and apply_filters(msg, args):
                        messages.append(msg)

        # Warn about incomplete packets
        incomplete = assembler.flush_incomplete()
        if incomplete:
            for key, buf in incomplete:
                print(
                    f"WARNING: incomplete CSP packet (key={key})"
                    f" started at line {buf.line_no} of {buf.filename}",
                    file=sys.stderr,
                )

        # Output
        if args.explode:
            groups = group_messages_for_explode(messages)
            print(fmt_stats(messages), end="")  # full-corpus summary to stdout
            for (logfile_str, a, b), group_msgs in groups.items():
                stem = Path(logfile_str).stem
                dir_path = Path(stem) / f"{a}-{b}"
                write_explode_group(dir_path, group_msgs, args, logfile_str)
                print(f"  wrote {len(group_msgs):>5} messages → {dir_path}/", file=sys.stderr)
        else:
            stats = fmt_stats(messages)
            if args.ai_export:
                out.write(fmt_ai_header(messages, args, args.logfiles))
                out.write(_AI_PROTOCOL_REF)
                out.write(stats)
                out.write(_AI_MSG_FORMAT_LEGEND)
                for msg in messages:
                    out.write(fmt_ai_msg(msg))
                    out.write("\n\n")
            elif args.markdown:
                out.write(stats)
                out.write("\n")
                out.write(_MD_HEADER)
                for msg in messages:
                    out.write(fmt_markdown_row(msg))
            else:
                out.write(stats)
                for msg in messages:
                    out.write(fmt_console(msg))
                    out.write("\n\n")

        if not messages:
            print("No CSP messages found (after filters).", file=sys.stderr)

    finally:
        if args.outfile:
            out.close()


if __name__ == "__main__":
    main()
