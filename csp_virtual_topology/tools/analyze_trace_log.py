#!/usr/bin/env python3
"""
Trace Log Analyzer

Utility to analyze trace session logs created by TraceCollector.
Helps with post-analysis of batch traceroute tests.

Usage:
    ./analyze_trace_log.py <trace_log_file.jsonl>
    ./analyze_trace_log.py <trace_log_file.jsonl> --filter-src 64 --filter-dst 3
    ./analyze_trace_log.py <trace_log_file.jsonl> --show-delivered-only
"""

import json
import sys
from pathlib import Path
from collections import defaultdict
from datetime import datetime


# CSP Trace Code Mappings
ROUTE_CODES = {
    0: "Received",
    1: "Forwarded",
    2: "Dropped",
    3: "Reserved",
    4: "Reserved",
    5: "Delivered"
}

DROP_CODES = {
    10: "D01: Duplicate packet",
    11: "D02: TTL expired",
    12: "D03: Unsupported protocol",
    13: "D04: Checksum failed",
    14: "D05: No route to host",
    15: "D06: No route to network",
    16: "D07: Frame discarded",
    17: "D08: RDP no connection",
    18: "D09: RDP closed connection",
    19: "D10: Socket queue full",
    20: "D11: Socket port unavailable",
    21: "D12: Socket unreachable",
    22: "D13: Destination unreachable",
    23: "D14: Packet discarded"
}

ACTION_NAMES = {
    0: "RECEIVED",
    1: "FORWARDED",
    2: "DROPPED"
}


def format_code(action, route_code):
    """Format action and route_code into human-readable string"""
    if action == 2:  # DROPPED
        return DROP_CODES.get(route_code, f"Unknown drop code {route_code}")
    else:
        return ROUTE_CODES.get(route_code, f"Unknown route code {route_code}")


def analyze_trace_log(log_file, filter_src=None, filter_dst=None, show_delivered_only=False):
    """Analyze a trace log file"""
    
    if not Path(log_file).exists():
        print(f"Error: File not found: {log_file}")
        return
    
    print(f"\n{'='*80}")
    print(f"Trace Log Analysis: {log_file}")
    print(f"{'='*80}\n")
    
    session_start = None
    session_end = None
    total_events = 0
    traces_by_packet = defaultdict(list)
    
    # Read and parse log file
    with open(log_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            try:
                entry = json.loads(line.strip())
                entry_type = entry.get('type')
                
                if entry_type == 'session_start':
                    session_start = entry.get('timestamp')
                    print(f"Session Start: {session_start}")
                    print(f"Bind Port: {entry.get('bind_port')}\n")
                
                elif entry_type == 'session_end':
                    session_end = entry.get('timestamp')
                    print(f"\nSession End: {session_end}")
                    print(f"Total Events: {entry.get('total_events')}")
                    print(f"Unique Traces: {entry.get('unique_traces')}\n")
                
                elif entry_type == 'clear_traces':
                    print(f"\n[{entry.get('timestamp')}] Traces cleared: {entry.get('traces_cleared')} traces")
                
                elif entry_type == 'trace_event':
                    event = entry.get('event', {})
                    src = event.get('src')
                    dst = event.get('dst')
                    
                    # Apply filters
                    if filter_src is not None and src != filter_src:
                        continue
                    if filter_dst is not None and dst != filter_dst:
                        continue
                    
                    action = event.get('action')
                    route_code = event.get('route_code', 0)
                    
                    # Filter for delivered only
                    if show_delivered_only and route_code != 5:
                        continue
                    
                    key = (src, dst, event.get('dport'), event.get('sport'))
                    traces_by_packet[key].append(event)
                    total_events += 1
                
            except json.JSONDecodeError as e:
                print(f"Warning: Invalid JSON on line {line_num}: {e}")
            except Exception as e:
                print(f"Warning: Error processing line {line_num}: {e}")
    
    # Display trace summaries
    print(f"\n{'='*80}")
    print(f"Trace Summary ({total_events} events, {len(traces_by_packet)} unique packets)")
    print(f"{'='*80}\n")
    
    for key, events in sorted(traces_by_packet.items()):
        src, dst, dport, sport = key
        print(f"\nPacket: src={src} → dst={dst}, dport={dport}, sport={sport}")
        print(f"  Events: {len(events)}")
        
        # Sort by timestamp
        events_sorted = sorted(events, key=lambda e: e.get('timestamp', 0))
        
        # Show each hop
        for i, event in enumerate(events_sorted, 1):
            node_addr = event.get('node_addr')
            iface = event.get('iface')
            action = event.get('action')
            route_code = event.get('route_code', 0)
            timestamp = event.get('timestamp')
            
            action_name = ACTION_NAMES.get(action, f"Unknown({action})")
            code_desc = format_code(action, route_code)
            
            print(f"    {i:2d}. [{timestamp:10d} ms] Node {node_addr:3d} on {iface:15s}: {action_name:10s} - {code_desc}")
        
        # Check if delivered
        delivered = any(e.get('route_code') == 5 for e in events_sorted)
        if delivered:
            print(f"  ✓ DELIVERED")
        else:
            print(f"  ✗ NOT DELIVERED")


if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='Analyze CSP trace log files')
    parser.add_argument('log_file', help='Path to trace log file (.jsonl)')
    parser.add_argument('--filter-src', type=int, help='Filter by source address')
    parser.add_argument('--filter-dst', type=int, help='Filter by destination address')
    parser.add_argument('--show-delivered-only', action='store_true', help='Show only delivered events')
    
    args = parser.parse_args()
    
    analyze_trace_log(
        args.log_file,
        filter_src=args.filter_src,
        filter_dst=args.filter_dst,
        show_delivered_only=args.show_delivered_only
    )

