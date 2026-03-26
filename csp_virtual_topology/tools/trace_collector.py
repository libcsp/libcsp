#!/usr/bin/env python3
"""
Trace Collector Service for CSP Virtual Topology Traceroute

This service collects and aggregates traceroute data from virtual nodes.
Each virtual node publishes trace events via ZMQ when packets with the
CSP_FTRACE flag are processed.

The collector groups trace events by packet 5-tuple (src, dst, dport, sport)
and provides APIs to retrieve complete packet paths.

Design Philosophy:
- Single-packet tracing: Only one traced packet at a time
- Sequential testing: No concurrent traceroutes
- Simple correlation: Group by 5-tuple, sort by timestamp
"""

import zmq
import json
import threading
import os
import time
from collections import defaultdict
from datetime import datetime
from pathlib import Path


class TraceCollector:
    """Collects and aggregates traceroute data from virtual nodes"""

    def __init__(self, bind_port=5570, log_dir=None):
        """
        Initialize the trace collector.

        Args:
            bind_port: Port to bind ZMQ PULL socket (default: 5570)
            log_dir: Directory to save trace logs (default: None, no logging)
        """
        self.bind_port = bind_port
        self.traces = defaultdict(list)  # Key: (src, dst, dport, sport), Value: list of hops
        self.lock = threading.Lock()
        self.running = False
        self.context = None
        self.socket = None
        self.thread = None

        # File logging setup
        self.log_dir = Path(log_dir) if log_dir else None
        self.session_log_file = None
        self.session_start_time = None
        self.trace_event_count = 0

        if self.log_dir:
            self.log_dir.mkdir(parents=True, exist_ok=True)
            self._start_new_session()

    def _start_new_session(self):
        """Start a new trace logging session"""
        if not self.log_dir:
            return

        self.session_start_time = datetime.now()
        timestamp = self.session_start_time.strftime("%Y%m%d_%H%M%S")
        self.session_log_file = self.log_dir / f"trace_session_{timestamp}.jsonl"
        self.trace_event_count = 0

        # Write session header
        with open(self.session_log_file, 'w') as f:
            header = {
                'type': 'session_start',
                'timestamp': self.session_start_time.isoformat(),
                'bind_port': self.bind_port
            }
            f.write(json.dumps(header) + '\n')

        print(f"[TraceCollector] Started new session log: {self.session_log_file}")

    def start(self):
        """Start the trace collector service"""
        if self.running:
            print("Trace collector already running")
            return

        print(f"Starting trace collector on port {self.bind_port}...")
        self.running = True
        self.thread = threading.Thread(target=self._collector_loop, daemon=True)
        self.thread.start()
        print("Trace collector started")

    def stop(self):
        """Stop the trace collector service"""
        if not self.running:
            return

        print("Stopping trace collector...")
        self.running = False

        if self.thread:
            self.thread.join(timeout=2)

        if self.socket:
            self.socket.close()

        if self.context:
            self.context.term()

        # Write session end marker
        if self.log_dir and self.session_log_file:
            try:
                with open(self.session_log_file, 'a') as f:
                    footer = {
                        'type': 'session_end',
                        'timestamp': datetime.now().isoformat(),
                        'total_events': self.trace_event_count,
                        'unique_traces': len(self.traces)
                    }
                    f.write(json.dumps(footer) + '\n')
                print(f"[TraceCollector] Session log closed: {self.trace_event_count} events")
            except Exception as e:
                print(f"[TraceCollector] Error writing session end: {e}")

        print("Trace collector stopped")

    def _collector_loop(self):
        """Main loop to receive trace events from virtual nodes"""
        try:
            self.context = zmq.Context()
            self.socket = self.context.socket(zmq.PULL)
            self.socket.bind(f"tcp://*:{self.bind_port}")
            self.socket.setsockopt(zmq.RCVTIMEO, 1000)  # 1 second timeout

            print(f"Trace collector listening on tcp://*:{self.bind_port}")

            while self.running:
                try:
                    msg = self.socket.recv_string()
                    import sys
                    print(f"[TraceCollector] Received message: {msg[:200]}", flush=True)  # Debug: print first 200 chars
                    sys.stdout.flush()
                    event = json.loads(msg)

                    if event.get('type') == 'trace':
                        self._process_trace_event(event)

                except zmq.Again:
                    # Timeout - continue loop
                    continue
                except json.JSONDecodeError as e:
                    print(f"Error decoding JSON trace event: {e}")
                except Exception as e:
                    print(f"Error processing trace event: {e}")

        except Exception as e:
            print(f"Fatal error in trace collector loop: {e}")
        finally:
            if self.socket:
                self.socket.close()
            if self.context:
                self.context.term()

    def _process_trace_event(self, event):
        """
        Process a single trace event.

        Expected event format:
        {
            "type": "trace",
            "src": <source address>,
            "dst": <destination address>,
            "dport": <destination port>,
            "sport": <source port>,
            "node_addr": <address of this node>,
            "iface": <interface name>,
            "timestamp": <timestamp in ms>,
            "action": <0=received, 1=forwarded, 2=dropped>,
            "route_code": <routing decision or drop reason code>,
            "via": <via address for routing table entries>
        }
        """
        try:
            key = (event['src'], event['dst'], event['dport'], event['sport'])

            hop = {
                'node_addr': event['node_addr'],
                'iface': event['iface'],
                'timestamp': event['timestamp'],
                'action': event['action'],  # 0=received, 1=forwarded, 2=dropped
                'route_code': event.get('route_code', 0),
                'via': event.get('via', 0xFFFF),  # CSP_NO_VIA_ADDRESS
                'sport': event['sport'],  # Source port for trace correlation
            }

            with self.lock:
                self.traces[key].append(hop)

            # Debug logging
            action_str = ['RECEIVED', 'FORWARDED', 'DROPPED'][hop['action']]
            print(f"Trace event: Node {hop['node_addr']}, {action_str}, Code {hop['route_code']}")

            # Log to file if enabled
            if self.log_dir and self.session_log_file:
                try:
                    with open(self.session_log_file, 'a') as f:
                        log_entry = {
                            'type': 'trace_event',
                            'received_at': datetime.now().isoformat(),
                            'event': event
                        }
                        f.write(json.dumps(log_entry) + '\n')
                    self.trace_event_count += 1
                except Exception as e:
                    print(f"[TraceCollector] Error writing trace event to file: {e}")

        except KeyError as e:
            print(f"Missing required field in trace event: {e}")
        except Exception as e:
            print(f"Error processing trace event: {e}")

    def get_all_trace_keys(self):
        """
        Get all trace keys currently stored.

        Returns:
            List of (src, dst, dport, sport) tuples
        """
        with self.lock:
            return list(self.traces.keys())

    def get_trace(self, src, dst, dport=None, sport=None):
        """
        Get the complete trace for a packet.

        Args:
            src: Source address
            dst: Destination address
            dport: Destination port (optional, if None returns most recent packet)
            sport: Source port (optional, if None returns most recent packet)

        Returns:
            List of hops sorted by timestamp
        """
        with self.lock:
            # If specific ports given, use exact key lookup
            if dport is not None and sport is not None:
                key = (src, dst, dport, sport)
                hops = self.traces.get(key, [])
            else:
                # Match any trace with given src/dst, but return only the most recent packet
                # (the one with the highest timestamp)
                matching_traces = {}
                for key, trace_hops in self.traces.items():
                    if key[0] == src and key[1] == dst:
                        matching_traces[key] = trace_hops

                if not matching_traces:
                    return []

                # Find the trace with the most recent (highest) timestamp
                most_recent_key = max(matching_traces.keys(),
                                     key=lambda k: max((h['timestamp'] for h in matching_traces[k]), default=0))
                hops = matching_traces[most_recent_key]

            # Sort by timestamp to get chronological order
            hops_sorted = sorted(hops, key=lambda h: h['timestamp'])
            return hops_sorted

    def get_trace_from_file(self, src, dst, dport=None, sport=None):
        """
        Get trace directly from the session file (most reliable method).

        Args:
            src: Source address
            dst: Destination address
            dport: Destination port (optional, if None returns most recent packet)
            sport: Source port (optional, if None returns most recent packet)

        Returns:
            List of hops sorted by timestamp
        """
        if not self.session_log_file or not os.path.exists(self.session_log_file):
            return []

        import json

        # Collect all matching trace events from file
        matching_traces = {}

        try:
            with open(self.session_log_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    if not line or not line.startswith('{'):
                        continue

                    try:
                        data = json.loads(line)
                        if data.get('type') != 'trace_event':
                            continue

                        event = data.get('event', {})
                        if event.get('src') != src or event.get('dst') != dst:
                            continue

                        # If specific ports requested, filter by them
                        if dport is not None and event.get('dport') != dport:
                            continue
                        if sport is not None and event.get('sport') != sport:
                            continue

                        # Group by (dport, sport) to separate different packets
                        key = (event.get('dport'), event.get('sport'))
                        if key not in matching_traces:
                            matching_traces[key] = []

                        matching_traces[key].append(event)

                    except json.JSONDecodeError:
                        continue

        except Exception as e:
            print(f"Error reading trace file: {e}")
            return []

        if not matching_traces:
            return []

        # If specific ports were given, return that trace
        if dport is not None and sport is not None:
            hops = matching_traces.get((dport, sport), [])
        else:
            # Return the most recent packet (highest timestamp)
            most_recent_key = max(matching_traces.keys(),
                                 key=lambda k: max((h['timestamp'] for h in matching_traces[k]), default=0))
            hops = matching_traces[most_recent_key]

        # Sort by timestamp
        return sorted(hops, key=lambda h: h['timestamp'])

    def clear_traces(self):
        """Clear all collected traces and optionally save summary"""
        with self.lock:
            count = len(self.traces)

            # Log summary before clearing if file logging is enabled
            if self.log_dir and self.session_log_file and count > 0:
                try:
                    with open(self.session_log_file, 'a') as f:
                        summary = {
                            'type': 'clear_traces',
                            'timestamp': datetime.now().isoformat(),
                            'traces_cleared': count,
                            'total_events_so_far': self.trace_event_count
                        }
                        f.write(json.dumps(summary) + '\n')
                except Exception as e:
                    print(f"[TraceCollector] Error writing clear summary: {e}")

            self.traces.clear()
            print(f"Cleared {count} trace(s)")

    def get_all_traces(self):
        """
        Get all traces (for debugging).

        Returns:
            Dictionary of all traces keyed by (src, dst, dport, sport)
        """
        with self.lock:
            return dict(self.traces)

    def get_trace_count(self):
        """Get the number of unique traces collected"""
        with self.lock:
            return len(self.traces)


# Standalone test mode
if __name__ == '__main__':
    import signal
    import sys

    collector = TraceCollector(bind_port=5570)

    def signal_handler(sig, frame):
        print("\nShutting down trace collector...")
        collector.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    collector.start()

    print("Trace collector running. Press Ctrl+C to exit.")
    print("Listening for trace events from virtual nodes...")

    try:
        while True:
            time.sleep(1)
            # Periodically print stats
            count = collector.get_trace_count()
            if count > 0:
                print(f"Collected {count} unique trace(s)")
    except KeyboardInterrupt:
        pass

    collector.stop()

