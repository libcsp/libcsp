#!/usr/bin/env python3
"""
CSP Virtual Topology Launcher

This script orchestrates a complete CSP virtual topology by:
1. Starting ZMQ proxy processes
2. Starting virtual node processes
3. Aggregating logs
4. Handling graceful shutdown
5. Optionally generating topology visualizations
"""

import json
import subprocess
import signal
import sys
import os
import time
import argparse
import threading
import zmq
from pathlib import Path
from typing import List, Dict, Optional

class TopologyLauncher:
    """Manages the lifecycle of a CSP virtual topology"""

    def __init__(self, config_file: str, binaries_dir: str = None):
        """
        Initialize the topology launcher

        Args:
            config_file: Path to topology JSON configuration
            binaries_dir: Path to directory containing executables (default: same directory as launcher script)
        """
        self.config_file = Path(config_file)
        self.config = self._load_config()
        self.processes: List[subprocess.Popen] = []
        self.process_names = {}  # Map PID to node name
        self.running = True

        # Control hub
        self.control_hub_enabled = self.config.get('control_hub', {}).get('enabled', True)
        self.control_hub_port = self.config.get('control_hub', {}).get('port', 5555)
        self.control_hub_thread = None
        self.zmq_context = None
        self.control_router = None
        self.control_dealer = None  # For sending commands from orchestrator

        # Set binaries directory
        if binaries_dir:
            self.binaries_dir = Path(binaries_dir)
        else:
            # Default to same directory as launcher script
            self.binaries_dir = Path(__file__).parent

        # Find executables
        self.zmqproxy_exe = self._find_executable("csp_virtual_zmqproxy")
        self.virtual_node_exe = self._find_executable("csp_virtual_node")

        # Setup signal handlers
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)

    def _load_config(self) -> dict:
        """Load and parse topology configuration"""
        with open(self.config_file, 'r') as f:
            config = json.load(f)
            # Handle nested topology structure
            if 'topology' in config:
                # Merge topology section with root
                topo = config['topology']
                config.update(topo)
            return config

    def _find_executable(self, name: str) -> Path:
        """Find an executable in binaries directory"""
        # Try binaries directory directly
        exe_path = self.binaries_dir / name
        if exe_path.exists():
            return exe_path

        raise FileNotFoundError(f"Could not find executable '{name}' in {self.binaries_dir}")

    def _signal_handler(self, signum, frame):
        """Handle shutdown signals"""
        print(f"\nReceived signal {signum}, shutting down...")
        self.running = False
        self.shutdown()

    def _sanitize_dirname(self, name: str) -> str:
        """Sanitize directory name for filesystem use"""
        import re
        sanitized = name.replace(' ', '_')
        sanitized = re.sub(r'[^a-zA-Z0-9_-]', '', sanitized)
        return sanitized

    def _get_log_dir(self) -> Path:
        """Get the log directory for this topology"""
        topology_name = self.config.get('name', 'topology')
        log_dir = Path.cwd() / self._sanitize_dirname(topology_name)
        log_dir.mkdir(parents=True, exist_ok=True)
        return log_dir

    def _monitor_proxy_output(self, proc, name):
        """Monitor and log proxy output in a separate thread"""
        try:
            for line in iter(proc.stdout.readline, ''):
                if line:
                    # Print to console with proxy name prefix
                    print(f"[ZMQ-{name}] {line.rstrip()}")
                    sys.stdout.flush()
        except Exception as e:
            print(f"[ZMQ-{name}] Error reading output: {e}")

    def start_zmq_proxies(self):
        """Start all ZMQ proxy processes"""
        print(f"\n{'='*60}")
        print(f"Starting {len(self.config['zmq_proxies'])} ZMQ proxy process(es)...")
        print(f"{'='*60}")

        # Get log directory for subnet logs
        log_dir = self._get_log_dir()

        for proxy in self.config['zmq_proxies']:
            name = proxy['name']
            host = proxy.get('host', 'localhost')
            sub_port = proxy.get('subscribe_port', 6000)
            pub_port = proxy.get('publish_port', 7000)

            # Build TCP URLs for zmqproxy
            sub_url = f"tcp://0.0.0.0:{sub_port}"
            pub_url = f"tcp://0.0.0.0:{pub_port}"

            # Create subnet log file path
            subnet_log_file = log_dir / f"subnet_{name}.log"

            cmd = [
                str(self.zmqproxy_exe),
                '-s', sub_url,
                '-p', pub_url,
                '-f', str(subnet_log_file)
            ]

            print(f"\n[{name}] Starting ZMQ proxy: sub={sub_url}, pub={pub_url}")
            print(f"  Log file: {subnet_log_file}")
            print(f"  Command: {' '.join(cmd)}")

            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                bufsize=1
            )

            self.processes.append(proc)
            print(f"  PID: {proc.pid}")

            # Start a thread to monitor proxy output
            monitor_thread = threading.Thread(
                target=self._monitor_proxy_output,
                args=(proc, name),
                daemon=True
            )
            monitor_thread.start()

        # Give proxies time to start
        time.sleep(0.5)

    def start_virtual_nodes(self):
        """Start all virtual node processes"""
        print(f"\n{'='*60}")
        print(f"Starting {len(self.config['nodes'])} virtual node process(es)...")
        print(f"{'='*60}")

        for node in self.config['nodes']:
            name = node['name']

            cmd = [
                str(self.virtual_node_exe),
                "--config", str(self.config_file.absolute()),
                "--node", name
            ]

            print(f"\n[{name}] Starting virtual node")
            print(f"  Command: {' '.join(cmd)}")

            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                bufsize=1
            )

            self.processes.append(proc)
            self.process_names[proc.pid] = name
            print(f"  PID: {proc.pid}")

        # Give nodes time to initialize
        time.sleep(1.0)

    def start_control_hub(self):
        """Start the ZMQ control hub for orchestration"""
        if not self.control_hub_enabled:
            return

        print(f"\n{'='*60}")
        print(f"Starting Control Hub on port {self.control_hub_port}...")
        print(f"{'='*60}\n")

        self.zmq_context = zmq.Context()
        self.control_router = self.zmq_context.socket(zmq.ROUTER)
        self.control_router.bind(f"tcp://*:{self.control_hub_port}")

        # Create DEALER socket for sending commands
        self.control_dealer = self.zmq_context.socket(zmq.DEALER)
        self.control_dealer.setsockopt(zmq.IDENTITY, b"orchestrator")
        self.control_dealer.connect(f"tcp://localhost:{self.control_hub_port}")

        # Start control hub thread
        self.control_hub_thread = threading.Thread(
            target=self._control_hub_worker,
            daemon=True
        )
        self.control_hub_thread.start()

        print(f"Control Hub listening on tcp://*:{self.control_hub_port}")

    def _control_hub_worker(self):
        """Control hub worker thread - routes messages between orchestrator and nodes"""
        poller = zmq.Poller()
        poller.register(self.control_router, zmq.POLLIN)

        while self.running:
            try:
                socks = dict(poller.poll(timeout=1000))

                if self.control_router in socks:
                    # Receive message: [identity, empty, message]
                    identity = self.control_router.recv()
                    empty = self.control_router.recv()
                    message = self.control_router.recv()

                    # Parse message
                    try:
                        msg_data = json.loads(message.decode('utf-8'))

                        # Check if this is a command to a node or a response from a node
                        if 'target_node' in msg_data:
                            # Command from orchestrator to node
                            target = msg_data['target_node'].encode('utf-8')
                            # Forward to target node: [target_identity, empty, message]
                            self.control_router.send_multipart([target, b'', message])
                        else:
                            # Response from node to orchestrator
                            # Forward back to orchestrator: [orchestrator_identity, empty, message]
                            self.control_router.send_multipart([b'orchestrator', b'', message])

                    except json.JSONDecodeError as e:
                        print(f"[CONTROL] Invalid JSON: {e}")

            except zmq.ZMQError as e:
                if self.running:
                    print(f"[CONTROL] ZMQ Error: {e}")
                break

    def send_command(self, target_node: str, command: str, params: dict, timeout_ms: int = 5000):
        """Send a command to a specific node and wait for response"""
        if not self.control_hub_enabled or not self.control_dealer:
            print("[ERROR] Control hub not enabled")
            return None

        # Build command message
        cmd_msg = {
            "target_node": target_node,
            "command": command,
            "params": params
        }

        cmd_json = json.dumps(cmd_msg)

        # Send command
        self.control_dealer.send_multipart([b'', cmd_json.encode('utf-8')])

        # Wait for response with timeout
        poller = zmq.Poller()
        poller.register(self.control_dealer, zmq.POLLIN)

        socks = dict(poller.poll(timeout=timeout_ms))

        if self.control_dealer in socks:
            # Receive response
            empty = self.control_dealer.recv()
            response = self.control_dealer.recv()

            try:
                response_data = json.loads(response.decode('utf-8'))
                return response_data
            except json.JSONDecodeError as e:
                print(f"[ERROR] Invalid JSON response: {e}")
                return None
        else:
            print(f"[ERROR] Timeout waiting for response from {target_node}")
            return None

    def interactive_cli(self):
        """Interactive command-line interface for sending commands"""
        print(f"\n{'='*60}")
        print("Interactive Control Interface")
        print(f"{'='*60}")
        print("\nAvailable commands:")
        print("  ping <node> <dest> [count] [timeout] [size]")
        print("    - Tell <node> to ping <dest> address")
        print("    - count: number of pings (default: 5)")
        print("    - timeout: timeout in ms (default: 1000)")
        print("    - size: ping size in bytes (default: 100)")
        print("  nodes")
        print("    - List all nodes in topology")
        print("  quit")
        print("    - Exit interactive mode")
        print(f"{'='*60}\n")

        while self.running:
            try:
                cmd_line = input("control> ").strip()

                if not cmd_line:
                    continue

                parts = cmd_line.split()
                cmd = parts[0].lower()

                if cmd == "quit" or cmd == "exit":
                    print("Exiting interactive mode...")
                    self.running = False
                    break

                elif cmd == "nodes":
                    print("\nNodes in topology:")
                    for node in self.config['nodes']:
                        print(f"  - {node['name']}: {node.get('description', 'N/A')}")
                    print()

                elif cmd == "ping":
                    if len(parts) < 3:
                        print("Usage: ping <node> <dest> [count] [timeout] [size]")
                        continue

                    node_name = parts[1]
                    dest_addr = int(parts[2])
                    count = int(parts[3]) if len(parts) > 3 else 5
                    timeout = int(parts[4]) if len(parts) > 4 else 1000
                    size = int(parts[5]) if len(parts) > 5 else 100

                    print(f"\nSending ping command to {node_name}...")
                    print(f"  Destination: {dest_addr}")
                    print(f"  Count: {count}")
                    print(f"  Timeout: {timeout} ms")
                    print(f"  Size: {size} bytes\n")

                    response = self.send_command(
                        node_name,
                        "csp_ping",
                        {
                            "destination": dest_addr,
                            "count": count,
                            "timeout_ms": timeout,
                            "size": size
                        },
                        timeout_ms=timeout * count + 2000
                    )

                    if response:
                        print(f"\n[RESPONSE from {response.get('source_node', 'unknown')}]")
                        print(f"  Status: {response.get('status', 'unknown')}")
                        if 'result' in response:
                            result = response['result']
                            print(f"  Sent: {result.get('sent', 0)}")
                            print(f"  Received: {result.get('received', 0)}")
                            print(f"  Avg time: {result.get('avg_time_ms', 0)} ms")
                            print(f"  Min time: {result.get('min_time_ms', 0)} ms")
                            print(f"  Max time: {result.get('max_time_ms', 0)} ms")
                        print()
                    else:
                        print("[ERROR] No response received\n")

                else:
                    print(f"Unknown command: {cmd}")
                    print("Type 'quit' to exit or use available commands\n")

            except KeyboardInterrupt:
                print("\nExiting interactive mode...")
                self.running = False
                break
            except Exception as e:
                print(f"[ERROR] {e}\n")

    def run(self, interactive: bool = False):
        """Main run loop - start processes and monitor"""
        print(f"\n{'='*60}")
        print(f"Topology: {self.config['name']}")
        print(f"Description: {self.config.get('description', 'N/A')}")
        print(f"CSP Version: {self.config.get('csp_version', 2)}")
        print(f"{'='*60}")

        try:
            # Start control hub first
            self.start_control_hub()

            # Start ZMQ proxies
            self.start_zmq_proxies()

            # Start virtual nodes
            self.start_virtual_nodes()

            print(f"\n{'='*60}")
            print("All processes started successfully!")
            if interactive:
                print("Starting interactive control interface...")
            else:
                print("Press Ctrl+C to stop the topology")
            print(f"{'='*60}\n")

            # Give nodes time to connect to control hub
            time.sleep(1.0)

            # Run interactive CLI or monitor processes
            if interactive:
                self.interactive_cli()
            else:
                self._monitor_processes()

        except KeyboardInterrupt:
            print("\nKeyboard interrupt received")
        except Exception as e:
            print(f"\nError: {e}")
            import traceback
            traceback.print_exc()
        finally:
            self.shutdown()

    def _check_process_health(self):
        """Check health of all processes and report issues"""
        num_proxies = len(self.config['zmq_proxies'])
        num_nodes = len(self.config['nodes'])

        dead_processes = []
        zombie_processes = []

        for i, proc in enumerate(self.processes):
            returncode = proc.poll()
            if returncode is not None:
                # Process has exited
                proc_type = "ZMQ Proxy" if i < num_proxies else "Virtual Node"
                proc_name = self.process_names.get(proc.pid, f"PID {proc.pid}")

                if returncode == 0:
                    dead_processes.append((proc_type, proc_name, proc.pid, returncode))
                else:
                    dead_processes.append((proc_type, proc_name, proc.pid, returncode))
            else:
                # Check if process is a zombie
                try:
                    import psutil
                    p = psutil.Process(proc.pid)
                    if p.status() == psutil.STATUS_ZOMBIE:
                        proc_type = "ZMQ Proxy" if i < num_proxies else "Virtual Node"
                        proc_name = self.process_names.get(proc.pid, f"PID {proc.pid}")
                        zombie_processes.append((proc_type, proc_name, proc.pid))
                except:
                    pass

        if dead_processes or zombie_processes:
            print(f"\n{'='*60}")
            print("⚠️  PROCESS HEALTH ALERT")
            print(f"{'='*60}")

            if dead_processes:
                print("\n❌ Dead Processes:")
                for proc_type, name, pid, code in dead_processes:
                    print(f"  - {proc_type} '{name}' (PID {pid}) exited with code {code}")

            if zombie_processes:
                print("\n🧟 Zombie Processes:")
                for proc_type, name, pid in zombie_processes:
                    print(f"  - {proc_type} '{name}' (PID {pid}) is in zombie state")

            print(f"\n{'='*60}\n")
            return False

        return True

    def _monitor_processes(self):
        """Monitor all processes and aggregate their output"""
        import select

        # Create mapping of file descriptors to process info
        fd_to_proc = {}
        for proc in self.processes:
            if proc.stdout:
                fd_to_proc[proc.stdout.fileno()] = proc

        health_check_interval = 5.0  # Check health every 5 seconds
        last_health_check = time.time()

        while self.running:
            # Periodic health check
            current_time = time.time()
            if current_time - last_health_check >= health_check_interval:
                if not self._check_process_health():
                    print("[ERROR] Critical process failure detected. Shutting down...")
                    self.running = False
                    break
                last_health_check = current_time

            # Check if any processes have died
            for proc in self.processes:
                if proc.poll() is not None:
                    proc_name = self.process_names.get(proc.pid, f"PID {proc.pid}")
                    print(f"\n[WARNING] Process '{proc_name}' ({proc.pid}) exited with code {proc.returncode}")
                    self.running = False
                    break

            if not self.running:
                break

            # Read output from processes (non-blocking)
            try:
                # Use select with timeout for non-blocking read
                readable, _, _ = select.select(list(fd_to_proc.keys()), [], [], 0.1)

                for fd in readable:
                    proc = fd_to_proc[fd]
                    line = proc.stdout.readline()
                    if line:
                        # Print with process PID prefix
                        print(f"[{proc.pid}] {line.rstrip()}")
            except Exception as e:
                # Handle select errors gracefully
                if self.running:
                    print(f"Error reading process output: {e}")
                break

    def shutdown(self):
        """Gracefully shutdown all processes"""
        self.running = False

        # Shutdown control hub
        if self.control_hub_thread:
            print("Stopping control hub...")
            if self.control_dealer:
                self.control_dealer.close()
            if self.control_router:
                self.control_router.close()
            if self.zmq_context:
                self.zmq_context.term()

        if not self.processes:
            return

        print(f"\n{'='*60}")
        print("Shutting down topology...")
        print(f"{'='*60}")

        # Send SIGTERM to all processes
        for proc in self.processes:
            if proc.poll() is None:  # Still running
                print(f"Terminating process {proc.pid}...")
                try:
                    proc.terminate()
                except Exception as e:
                    print(f"  Error terminating {proc.pid}: {e}")

        # Wait for processes to exit (with timeout)
        timeout = 5.0
        start_time = time.time()

        while time.time() - start_time < timeout:
            all_done = True
            for proc in self.processes:
                if proc.poll() is None:
                    all_done = False
                    break

            if all_done:
                break

            time.sleep(0.1)

        # Force kill any remaining processes
        for proc in self.processes:
            if proc.poll() is None:
                print(f"Force killing process {proc.pid}...")
                try:
                    proc.kill()
                    proc.wait()
                except Exception as e:
                    print(f"  Error killing {proc.pid}: {e}")

        print("All processes stopped.")
        self.processes.clear()

    def generate_visualization(self, output_file: str = None, format: str = 'png', show: bool = False):
        """Generate Graphviz visualization of topology"""
        try:
            import graphviz
        except ImportError:
            print("Error: graphviz Python package not installed")
            print("Install with: pip install graphviz")
            return

        print(f"\nGenerating topology visualization...")

        # Create directed graph
        dot = graphviz.Digraph(comment=self.config['name'])
        dot.attr(rankdir='LR', splines='ortho')

        # Define node shapes by type
        node_shapes = {
            'ground': 'house',
            'radio': 'diamond',
            'router': 'hexagon',
            'sensor': 'box',
            'i2c': 'box',
        }

        # Define interface colors
        interface_colors = {
            'RF': 'red',
            'CAN': 'blue',
            'I2C': 'green',
        }

        # Add ZMQ proxies as clusters
        for proxy in self.config['zmq_proxies']:
            with dot.subgraph(name=f'cluster_{proxy["name"]}') as c:
                c.attr(label=f'{proxy["name"]} Proxy', style='dashed', color='gray')
                c.node(f'proxy_{proxy["name"]}',
                      label=f'{proxy["name"]}\\n{proxy.get("host", "localhost")}',
                      shape='cylinder', style='filled', fillcolor='lightgray')

        # Add nodes
        for node in self.config['nodes']:
            node_type = node.get('type', 'node')
            shape = node_shapes.get(node_type, 'ellipse')

            # Get node address from first interface
            addr = node['interfaces'][0]['address'] if node.get('interfaces') else '?'

            dot.node(node['name'],
                    label=f'{node["name"]}\\nAddr: {addr}',
                    shape=shape,
                    style='filled',
                    fillcolor='lightblue')

        # Add edges for interfaces
        for node in self.config['nodes']:
            for iface in node.get('interfaces', []):
                proxy_name = iface['zmq_proxy']
                color = interface_colors.get(proxy_name, 'black')

                # Connect node to proxy
                dot.edge(node['name'], f'proxy_{proxy_name}',
                        label=iface['name'],
                        color=color,
                        style='dashed')

        # Render visualization
        if output_file is None:
            output_file = f"{self.config['name']}_topology"

        try:
            dot.render(output_file, format=format, cleanup=True)
            print(f"Visualization saved to: {output_file}.{format}")

            if show:
                dot.view()
        except Exception as e:
            print(f"Error rendering visualization: {e}")


class WebServer:
    """Flask web server for topology dashboard"""

    def __init__(self, launcher: TopologyLauncher, port: int = 9999):
        """Initialize web server

        Args:
            launcher: TopologyLauncher instance
            port: Port to run web server on
        """
        from flask import Flask, render_template, jsonify, request, send_file
        from flask_socketio import SocketIO, emit
        from trace_collector import TraceCollector

        self.launcher = launcher
        self.port = port
        # Web assets are in ../web/ relative to the script location (in bin/)
        script_dir = Path(__file__).parent
        web_dir = script_dir.parent / 'web'
        self.app = Flask(__name__,
                        template_folder=str(web_dir / 'templates'),
                        static_folder=str(web_dir / 'static'))
        self.app.config['SECRET_KEY'] = 'csp-virtual-topology-secret'
        self.socketio = SocketIO(self.app, cors_allowed_origins="*")

        self.log_tail_threads = {}
        self.running = True

        # Initialize trace collector with file logging
        # Save trace logs in the same directory as node logs
        topology_name = self.launcher.config.get('name', 'topology')
        log_dir = Path.cwd() / self._sanitize_dirname(topology_name) / 'traces'

        self.trace_collector = TraceCollector(bind_port=5570, log_dir=str(log_dir))
        self.trace_collector.start()
        print(f"[WebServer] Trace collector started on port 5570")
        print(f"[WebServer] Trace logs will be saved to: {log_dir}")

        # Batch traceroute progress tracking
        self.batch_progress = {
            'running': False,
            'current': 0,
            'total': 0,
            'current_test': '',
            'results': []
        }
        self.batch_progress_lock = threading.Lock()

        # Build proxy lookup for address calculation
        self._proxy_lookup = {}
        for proxy in self.launcher.config.get('zmq_proxies', []):
            self._proxy_lookup[proxy['name']] = proxy

        print(f"[DEBUG] Built proxy lookup with {len(self._proxy_lookup)} proxies:")
        for name, proxy in self._proxy_lookup.items():
            print(f"  {name}: subnet_prefix={proxy.get('subnet_prefix')}, netmask={proxy.get('netmask')}")

        # Setup routes
        self._setup_routes()
        self._setup_socketio()

        # Start background tasks
        self._start_background_tasks()

    def _calculate_full_csp_address(self, subnet_prefix: int, netmask: int, host_address: int) -> int:
        """
        Calculate fully-qualified 14-bit CSP address.

        CSP v2 uses 14-bit addresses where:
        - netmask = number of host bits
        - network bits = 14 - netmask
        - full_address = (subnet_prefix << network_bits) | (host_address & host_mask)

        Args:
            subnet_prefix: Subnet prefix from ZMQ proxy config
            netmask: Network mask (number of host bits)
            host_address: Host address from interface config

        Returns:
            Fully-qualified 14-bit CSP address
        """
        network_bits = 14 - netmask
        host_mask = (1 << netmask) - 1
        full_address = ((subnet_prefix << network_bits) | (host_address & host_mask)) & 0x3FFF
        return full_address

    def _get_full_address(self, iface: dict) -> int:
        """
        Get fully-qualified CSP address for an interface.

        Looks up the interface's ZMQ proxy and calculates the full address
        using the proxy's subnet_prefix and netmask.

        Args:
            iface: Interface configuration dictionary

        Returns:
            Fully-qualified CSP address, or raw address if proxy not found
        """
        host_addr = iface.get('address', 0)
        proxy_name = iface.get('zmq_proxy')

        if proxy_name and proxy_name in self._proxy_lookup:
            proxy = self._proxy_lookup[proxy_name]
            subnet_prefix = proxy.get('subnet_prefix', 0)
            netmask = proxy.get('netmask', 8)
            full_addr = self._calculate_full_csp_address(subnet_prefix, netmask, host_addr)
            print(f"[DEBUG] _get_full_address: proxy={proxy_name}, subnet_prefix={subnet_prefix}, netmask={netmask}, host={host_addr} -> full={full_addr}")
            return full_addr

        # No proxy found, return raw address
        print(f"[DEBUG] _get_full_address: NO PROXY for {iface.get('name')}, returning raw address {host_addr}")
        return host_addr

    def _setup_routes(self):
        """Setup Flask routes"""
        from flask import render_template, jsonify, send_file, request
        import time

        @self.app.route('/')
        def index():
            # Generate cache buster based on current timestamp
            cache_bust = str(int(time.time()))
            return render_template('dashboard.html', cache_bust=cache_bust)

        @self.app.route('/api/topology')
        def get_topology():
            return jsonify(self.launcher.config)

        @self.app.route('/api/nodes')
        def get_nodes():
            nodes = []
            for node in self.launcher.config.get('nodes', []):
                node_status = self._get_node_status(node['name'])
                nodes.append({
                    'name': node['name'],
                    'description': node.get('description', ''),
                    'interfaces': node.get('interfaces', []),
                    'status': node_status
                })
            return jsonify({'nodes': nodes})

        @self.app.route('/api/start', methods=['POST'])
        def start_topology():
            try:
                if not self.launcher.processes:
                    # Start in a background thread
                    threading.Thread(target=self._start_topology_async, daemon=True).start()
                    return jsonify({'success': True})
                else:
                    return jsonify({'success': False, 'error': 'Topology already running'})
            except Exception as e:
                return jsonify({'success': False, 'error': str(e)})

        @self.app.route('/api/stop', methods=['POST'])
        def stop_topology():
            try:
                self.launcher.shutdown()
                return jsonify({'success': True})
            except Exception as e:
                return jsonify({'success': False, 'error': str(e)})

        @self.app.route('/api/topology/save-draft', methods=['POST'])
        def save_draft():
            """Save topology to server without restarting"""
            try:
                # Get topology data from request
                topology_data = request.get_json()
                if not topology_data:
                    return jsonify({'success': False, 'error': 'No topology data provided'}), 400

                # Validate topology structure
                if 'topology' not in topology_data or 'nodes' not in topology_data:
                    return jsonify({'success': False, 'error': 'Invalid topology structure'}), 400

                # Save to the original config file
                config_file = self.launcher.config_file

                # Create backup of original file
                backup_file = config_file.with_suffix('.json.backup')
                if config_file.exists():
                    import shutil
                    shutil.copy2(config_file, backup_file)
                    print(f"[WebServer] Created backup: {backup_file}")

                # Write new topology
                with open(config_file, 'w') as f:
                    json.dump(topology_data, f, indent=2)
                print(f"[WebServer] Saved draft topology: {config_file}")

                # Reload launcher config
                self.launcher.config = self.launcher._load_config()

                return jsonify({'success': True, 'message': 'Draft saved successfully'})

            except Exception as e:
                print(f"[WebServer] Error saving draft: {e}")
                import traceback
                traceback.print_exc()
                return jsonify({'success': False, 'error': str(e)}), 500

        @self.app.route('/api/topology/update', methods=['POST'])
        def update_topology():
            """Update topology configuration and restart"""
            try:
                # Get topology data from request
                topology_data = request.get_json()
                if not topology_data:
                    return jsonify({'success': False, 'error': 'No topology data provided'}), 400

                # Validate topology structure
                if 'topology' not in topology_data or 'nodes' not in topology_data:
                    return jsonify({'success': False, 'error': 'Invalid topology structure'}), 400

                # Save to the original config file
                config_file = self.launcher.config_file

                # Create backup of original file
                backup_file = config_file.with_suffix('.json.backup')
                if config_file.exists():
                    import shutil
                    shutil.copy2(config_file, backup_file)
                    print(f"[WebServer] Created backup: {backup_file}")

                # Write new topology
                with open(config_file, 'w') as f:
                    json.dump(topology_data, f, indent=2)
                print(f"[WebServer] Updated topology file: {config_file}")

                # Trigger restart in background
                threading.Thread(target=self._restart_topology_async, daemon=True).start()

                return jsonify({'success': True, 'message': 'Topology update initiated, restarting...'})

            except Exception as e:
                print(f"[WebServer] Error updating topology: {e}")
                import traceback
                traceback.print_exc()
                return jsonify({'success': False, 'error': str(e)}), 500

        @self.app.route('/api/logs/<node_name>')
        def get_log(node_name):
            try:
                # Get topology name and sanitize it
                topology_name = self.launcher.config.get('name', 'topology')
                log_dir = self._sanitize_dirname(topology_name)

                # Try subdirectory first (relative to cwd)
                log_file = Path.cwd() / log_dir / f"{node_name}.log"

                # Fallback to current directory if not found in subdirectory
                if not log_file.exists():
                    log_file = Path.cwd() / f"{node_name}.log"

                if log_file.exists():
                    return send_file(str(log_file), as_attachment=True, download_name=f"{node_name}.log")
                else:
                    return jsonify({'error': 'Log file not found'}), 404
            except Exception as e:
                return jsonify({'error': str(e)}), 500

        @self.app.route('/api/traceroute/start', methods=['POST'])
        def start_traceroute():
            """Initiate a traceroute between two nodes"""
            try:
                data = request.get_json()
                src_node = data.get('src_node')
                dst_node = data.get('dst_node')

                if not src_node or not dst_node:
                    return jsonify({'success': False, 'error': 'Missing src_node or dst_node'}), 400

                # Get fully-qualified addresses for both nodes
                # The C code now uses fully-qualified addresses everywhere
                src_addr_full = self._get_node_address(src_node)
                if src_addr_full is None:
                    return jsonify({'success': False, 'error': f'Source node {src_node} not found'}), 404

                dst_addr_full = self._get_node_address(dst_node)
                if dst_addr_full is None:
                    return jsonify({'success': False, 'error': f'Destination node {dst_node} not found'}), 404

                # Clear previous traces to avoid correlation issues
                # We only trace ONE packet at a time
                self.trace_collector.clear_traces()

                # Send ONE ping with trace flag enabled
                # The trace flag sets CSP_FTRACE in the packet header
                # Use fully-qualified address for ping - the C code uses fully-qualified addresses
                command = f"ping {src_node} {dst_addr_full} 1 1000 64 --trace"
                print(f"[WebServer] Sending traced ping: {command} (src_full={src_addr_full}, dst_full={dst_addr_full})")

                response = self._execute_command(command)
                print(f"[WebServer] Ping response: {response}")

                # Wait for trace events to arrive from all hops
                # Increased to 200ms to ensure all trace events are written to file
                # Even though we read from file, events arrive asynchronously from virtual nodes
                # and need time to be collected and written to disk
                time.sleep(0.2)

                # Retrieve trace from session file for reliability
                # This ensures we get all events including DELIVERED events
                # that may arrive after the initial wait period
                # Use fully-qualified addresses for trace lookup - trace events contain fully-qualified addresses
                trace = self.trace_collector.get_trace_from_file(src_addr_full, dst_addr_full)

                print(f"[TRACE DEBUG] Single traceroute:")
                print(f"  Request: src_node={src_node}, dst_node={dst_node}")
                print(f"  Full addresses: src_full={src_addr_full}, dst_full={dst_addr_full}")
                print(f"  Trace lookup result: {len(trace)} hops")
                if trace:
                    print(f"  First hop: {trace[0]}")
                    print(f"  Last hop: {trace[-1]}")
                else:
                    print(f"  NO TRACE DATA FOUND!")
                    print(f"  Available traces in collector:")
                    all_traces = self.trace_collector.get_all_trace_keys()
                    for key in list(all_traces)[:10]:
                        print(f"    {key}")

                return jsonify({
                    'success': True,
                    'src': src_addr_full,
                    'dst': dst_addr_full,
                    'src_node': src_node,
                    'dst_node': dst_node,
                    'hops': trace,
                    'hop_count': len(trace)
                })

            except Exception as e:
                print(f"[WebServer] Error in traceroute: {e}")
                import traceback
                traceback.print_exc()
                return jsonify({'success': False, 'error': str(e)}), 500

        @self.app.route('/api/traceroute/batch', methods=['POST'])
        def batch_traceroute():
            """
            Start batch traceroute in background thread.

            Request JSON:
                {
                    "src_node": "Ground Station"  // Required: source node name
                }

            Response:
                {
                    "success": true,
                    "message": "Batch traceroute started",
                    "total_tests": 12
                }
            """
            try:
                data = request.json or {}
                src_node_name = data.get('src_node')

                if not src_node_name:
                    return jsonify({
                        'success': False,
                        'error': 'src_node parameter is required'
                    }), 400

                # Check if batch is already running
                with self.batch_progress_lock:
                    if self.batch_progress['running']:
                        return jsonify({
                            'success': False,
                            'error': 'Batch traceroute already running'
                        }), 409

                nodes = self.launcher.config.get('nodes', [])

                # Find source node configuration
                src_node_config = None
                for node in nodes:
                    if node['name'] == src_node_name:
                        src_node_config = node
                        break

                if not src_node_config:
                    return jsonify({
                        'success': False,
                        'error': f'Source node "{src_node_name}" not found'
                    }), 404

                # Get all interfaces for source node
                src_interfaces = src_node_config.get('interfaces', [])
                if not src_interfaces:
                    return jsonify({
                        'success': False,
                        'error': f'Source node "{src_node_name}" has no interfaces'
                    }), 400

                # Calculate total tests
                total_tests = 0
                for dst_node_config in nodes:
                    if dst_node_config['name'] != src_node_name:
                        dst_interfaces = dst_node_config.get('interfaces', [])
                        total_tests += len(src_interfaces) * len(dst_interfaces)

                # Initialize progress
                with self.batch_progress_lock:
                    self.batch_progress = {
                        'running': True,
                        'current': 0,
                        'total': total_tests,
                        'current_test': '',
                        'results': []
                    }

                # Start batch traceroute in background thread
                thread = threading.Thread(
                    target=self._run_batch_traceroute,
                    args=(src_node_name, src_node_config, nodes),
                    daemon=True
                )
                thread.start()

                return jsonify({
                    'success': True,
                    'message': 'Batch traceroute started',
                    'total_tests': total_tests
                })

            except Exception as e:
                print(f"[WebServer] Error starting batch traceroute: {e}")
                import traceback
                traceback.print_exc()
                return jsonify({'success': False, 'error': str(e)}), 500

        @self.app.route('/api/traceroute/batch/progress', methods=['GET'])
        def batch_progress():
            """Get current batch traceroute progress"""
            with self.batch_progress_lock:
                return jsonify(self.batch_progress)

        @self.app.route('/api/traceroute/clear', methods=['POST'])
        def clear_traces():
            """Clear all collected traces"""
            try:
                self.trace_collector.clear_traces()
                return jsonify({'success': True})
            except Exception as e:
                return jsonify({'success': False, 'error': str(e)}), 500

        @self.app.route('/api/trace/detail', methods=['GET'])
        def get_trace_detail():
            """
            Get detailed trace from session file.

            Query parameters:
                src: Source address (required)
                dst: Destination address (required)
                sport: Source port (optional, if not provided returns most recent packet)

            Returns:
                {
                    'success': true,
                    'hops': [...],
                    'hop_count': N
                }
            """
            try:
                src = request.args.get('src', type=int)
                dst = request.args.get('dst', type=int)
                sport = request.args.get('sport', type=int)

                print(f"[TRACE DEBUG] Detail request: src={src}, dst={dst}, sport={sport}")

                if src is None or dst is None:
                    return jsonify({
                        'success': False,
                        'error': 'src and dst parameters are required'
                    }), 400

                # Retrieve trace from session file
                hops = self.trace_collector.get_trace_from_file(src, dst, sport=sport)

                print(f"[TRACE DEBUG] Detail result: {len(hops)} hops")
                if not hops:
                    print(f"  NO HOPS FOUND for src={src}, dst={dst}, sport={sport}")
                    all_traces = self.trace_collector.get_all_trace_keys()
                    print(f"  Available traces ({len(all_traces)} total):")
                    for key in list(all_traces)[:10]:
                        print(f"    {key}")

                return jsonify({
                    'success': True,
                    'hops': hops,
                    'hop_count': len(hops)
                })

            except Exception as e:
                print(f"[WebServer] Error retrieving trace detail: {e}")
                import traceback
                traceback.print_exc()
                return jsonify({'success': False, 'error': str(e)}), 500

    def _run_batch_traceroute(self, src_node_name, src_node_config, nodes):
        """
        Run batch traceroute in background thread.

        Args:
            src_node_name: Name of source node
            src_node_config: Source node configuration dict
            nodes: List of all node configurations
        """
        try:
            # Clear all traces at the start of batch test
            # This ensures we start fresh, but we won't clear between individual tests
            # so that trace data remains available for the web UI
            self.trace_collector.clear_traces()

            src_interfaces = src_node_config.get('interfaces', [])
            results = []
            test_count = 0

            # Test from each source interface
            for src_iface in src_interfaces:
                src_addr_raw = src_iface.get('address')
                src_iface_name = src_iface.get('name', 'Unknown')

                if src_addr_raw is None:
                    continue

                # Calculate fully-qualified CSP address for trace matching
                src_addr_full = self._get_full_address(src_iface)

                # Test to each destination node
                for dst_node_config in nodes:
                    dst_node_name = dst_node_config['name']

                    # Skip self
                    if dst_node_name == src_node_name:
                        continue

                    dst_interfaces = dst_node_config.get('interfaces', [])

                    # Test to each destination interface
                    for dst_iface in dst_interfaces:
                        dst_addr_raw = dst_iface.get('address')
                        dst_iface_name = dst_iface.get('name', 'Unknown')

                        if dst_addr_raw is None:
                            continue

                        # Calculate fully-qualified CSP address for trace matching
                        dst_addr_full = self._get_full_address(dst_iface)

                        test_count += 1

                        # Update progress
                        with self.batch_progress_lock:
                            self.batch_progress['current'] = test_count
                            self.batch_progress['current_test'] = f"{src_node_name}:{src_iface_name} → {dst_node_name}:{dst_iface_name}"

                        # Note: We don't clear traces during batch tests anymore.
                        # Each packet has a unique sport, so traces are stored separately
                        # by (src, dst, dport, sport) key and won't interfere with each other.
                        # This allows the web UI to display complete trace data when clicking
                        # on batch result cells.

                        # Send traced ping from src node to dst address
                        # The ping command uses raw dst_addr since the node's CSP stack
                        # knows how to route to the destination
                        command = f"ping {src_node_name} {dst_addr_raw} 1 1000 64 --trace"
                        print(f"[Batch {test_count}] Testing {src_node_name}:{src_iface_name}({src_addr_full}) → {dst_node_name}:{dst_iface_name}({dst_addr_full})")
                        self._execute_command(command)

                        # Wait for trace events and allow destination to process
                        # Increased to 200ms to ensure all trace events are written to file
                        # Even though we read from file, events arrive asynchronously from virtual nodes
                        # and need time to be collected and written to disk
                        time.sleep(0.2)

                        # Get trace using fully-qualified addresses
                        # The trace events use full CSP addresses computed by the C code
                        trace = self.trace_collector.get_trace(src_addr_full, dst_addr_full)

                        # Determine reachability
                        # A packet is reachable if it was DELIVERED (route_code == 5)
                        # Simply receiving (action == 0) is not enough - it could be dropped after reception
                        reachable = False
                        sport = None
                        if trace:
                            reachable = any(
                                hop['node_addr'] == dst_addr_full and hop['route_code'] == 5  # CSP_TRACE_DELIVERED
                                for hop in trace
                            )
                            # Extract sport from the first hop for later file-based retrieval
                            if len(trace) > 0:
                                sport = trace[0].get('sport')

                        result = {
                            'src_node': src_node_name,
                            'src_iface': src_iface_name,
                            'src_addr': src_addr_full,
                            'dst_node': dst_node_name,
                            'dst_iface': dst_iface_name,
                            'dst_addr': dst_addr_full,
                            'reachable': reachable,
                            'hop_count': len(trace),
                            'sport': sport  # Store sport instead of full hops array
                        }
                        results.append(result)

                        # Update results in progress
                        with self.batch_progress_lock:
                            self.batch_progress['results'] = results

            # Mark as complete
            with self.batch_progress_lock:
                self.batch_progress['running'] = False
                self.batch_progress['current_test'] = 'Complete'

            print(f"[Batch] Completed {test_count} tests")

        except Exception as e:
            print(f"[Batch] Error in background thread: {e}")
            import traceback
            traceback.print_exc()

            with self.batch_progress_lock:
                self.batch_progress['running'] = False
                self.batch_progress['current_test'] = f'Error: {str(e)}'

    def _setup_socketio(self):
        """Setup SocketIO event handlers"""
        from flask_socketio import emit

        @self.socketio.on('connect')
        def handle_connect():
            print('[WebServer] Client connected')
            # Send initial topology data
            emit('topology_data', self.launcher.config)
            # Send initial status
            emit('status_update', {
                'status': 'Running' if self.launcher.processes else 'Stopped',
                'nodes': self._get_all_nodes_status()
            })

        @self.socketio.on('disconnect')
        def handle_disconnect():
            print('[WebServer] Client disconnected')

        @self.socketio.on('command')
        def handle_command(data):
            command = data.get('command', '')
            print(f'[WebServer] Received command: {command}')

            # Execute command using launcher's control interface
            response = self._execute_command(command)
            emit('command_response', {'output': response})

        @self.socketio.on('tail_log')
        def handle_tail_log(data):
            node_name = data.get('node')
            print(f'[WebServer] Tailing log for node: {node_name}')
            self._start_log_tail(node_name)

        @self.socketio.on('stop_tail')
        def handle_stop_tail(data):
            node_name = data.get('node')
            self._stop_log_tail(node_name)

        @self.socketio.on('get_node_stats')
        def handle_get_node_stats(data):
            node_name = data.get('node')
            stats = self._get_node_stats(node_name)
            emit('node_stats', {'node': node_name, 'stats': stats})

    def _sanitize_dirname(self, name):
        """Sanitize directory name"""
        import re
        sanitized = name.replace(' ', '_')
        sanitized = re.sub(r'[^a-zA-Z0-9_-]', '', sanitized)
        return sanitized

    def _get_node_status(self, node_name):
        """Get status of a specific node"""
        for proc in self.launcher.processes:
            if self.launcher.process_names.get(proc.pid) == node_name:
                if proc.poll() is None:
                    return 'Running'
                else:
                    return 'Stopped'
        return 'Stopped'

    def _get_all_nodes_status(self):
        """Get status of all nodes"""
        nodes = []
        for node in self.launcher.config.get('nodes', []):
            nodes.append({
                'name': node['name'],
                'status': self._get_node_status(node['name'])
            })
        return nodes

    def _start_topology_async(self):
        """Start topology in background"""
        try:
            # Reset running flag (critical for restart after shutdown)
            self.launcher.running = True

            self.launcher.start_control_hub()
            self.launcher.start_zmq_proxies()
            self.launcher.start_virtual_nodes()

            # Broadcast status update
            self.socketio.emit('status_update', {
                'status': 'Running',
                'nodes': self._get_all_nodes_status()
            })
        except Exception as e:
            print(f"[WebServer] Error starting topology: {e}")
            self.socketio.emit('status_update', {
                'status': 'Error',
                'error': str(e)
            })

    def _restart_topology_async(self):
        """Restart topology in background"""
        try:
            print("[WebServer] Restarting topology...")

            # Broadcast restart status
            self.socketio.emit('topology_restart', {
                'status': 'stopping',
                'message': 'Stopping current topology...'
            })

            # Shutdown current topology
            self.launcher.shutdown()
            time.sleep(5)  # Give processes more time to clean up and release ports

            # Reload configuration
            self.socketio.emit('topology_restart', {
                'status': 'reloading',
                'message': 'Reloading configuration...'
            })

            self.launcher.config = self.launcher._load_config()

            # Start topology
            self.socketio.emit('topology_restart', {
                'status': 'starting',
                'message': 'Starting topology...'
            })

            self._start_topology_async()

            # Broadcast completion
            self.socketio.emit('topology_restart', {
                'status': 'complete',
                'message': 'Topology restarted successfully'
            })

            # Send updated topology data
            self.socketio.emit('topology_data', self.launcher.config)

            print("[WebServer] Topology restart complete")

        except Exception as e:
            print(f"[WebServer] Error restarting topology: {e}")
            import traceback
            traceback.print_exc()
            self.socketio.emit('topology_restart', {
                'status': 'error',
                'message': f'Error restarting topology: {str(e)}'
            })

    def _execute_command(self, command):
        """Execute CLI command"""
        parts = command.split()
        if not parts:
            return ''

        cmd = parts[0].lower()

        if cmd == 'nodes':
            output = '\nNodes in topology:\n'
            for node in self.launcher.config.get('nodes', []):
                output += f"  - {node['name']}: {node.get('description', 'N/A')}\n"
            return output

        elif cmd == 'ping':
            if len(parts) < 3:
                return 'Usage: ping <node> <dest> [count] [timeout] [size] [--trace]\n'

            node_name = parts[1]
            dest_addr = parts[2]
            count = int(parts[3]) if len(parts) > 3 else 5
            timeout = int(parts[4]) if len(parts) > 4 else 1000
            size = int(parts[5]) if len(parts) > 5 else 100
            trace = '--trace' in parts

            # Send ping command via control interface
            result = self.launcher.send_command(node_name, 'csp_ping', {
                'destination': int(dest_addr),
                'count': count,
                'timeout_ms': timeout,
                'size': size,
                'trace': trace
            }, timeout_ms=timeout * count + 2000)

            if result:
                status = result.get('status', 'unknown')
                output = f"Ping command sent to {node_name}\nStatus: {status}\n"
                if 'result' in result:
                    res = result['result']
                    output += f"Sent: {res.get('sent', 0)}, Received: {res.get('received', 0)}\n"
                    output += f"Avg: {res.get('avg_time_ms', 0)} ms, Min: {res.get('min_time_ms', 0)} ms, Max: {res.get('max_time_ms', 0)} ms\n"
                return output
            else:
                return f"Failed to send ping command to {node_name}\n"

        else:
            return f"Unknown command: {cmd}\n"

    def _start_log_tail(self, node_name):
        """Start tailing log file for a node"""
        if node_name in self.log_tail_threads:
            return

        def tail_log():
            topology_name = self.launcher.config.get('name', 'topology')
            log_dir = self._sanitize_dirname(topology_name)

            # Try subdirectory first (relative to cwd)
            log_file = Path.cwd() / log_dir / f"{node_name}.log"

            # Fallback to current directory if not found in subdirectory
            if not log_file.exists():
                log_file = Path.cwd() / f"{node_name}.log"

            if not log_file.exists():
                self.socketio.emit('log_update', {
                    'node': node_name,
                    'lines': ['Log file not found']
                })
                return

            # Read existing content
            try:
                with open(log_file, 'r') as f:
                    lines = f.readlines()
                    # Send last 50 lines
                    self.socketio.emit('log_update', {
                        'node': node_name,
                        'lines': [line.rstrip() for line in lines[-50:]]
                    })

                    # Continue tailing
                    while self.running and node_name in self.log_tail_threads:
                        line = f.readline()
                        if line:
                            self.socketio.emit('log_update', {
                                'node': node_name,
                                'lines': [line.rstrip()]
                            })
                        else:
                            time.sleep(0.1)
            except Exception as e:
                print(f"[WebServer] Error tailing log for {node_name}: {e}")

        thread = threading.Thread(target=tail_log, daemon=True)
        self.log_tail_threads[node_name] = thread
        thread.start()

    def _stop_log_tail(self, node_name):
        """Stop tailing log file"""
        if node_name in self.log_tail_threads:
            del self.log_tail_threads[node_name]

    def _get_node_stats(self, node_name):
        """Get real-time stats for a node"""
        # Return basic status info
        # Interface and routing information is displayed from the frontend topology data
        status = self._get_node_status(node_name)
        return f"Node: {node_name}\nStatus: {status}"

    def _get_node_address(self, node_name):
        """
        Get fully-qualified CSP address for a node by name.

        Args:
            node_name: Name of the node

        Returns:
            Fully-qualified CSP address (int) or None if not found
        """
        for node in self.launcher.config.get('nodes', []):
            if node['name'] == node_name:
                # Get address from the first (default) interface
                interfaces = node.get('interfaces', [])
                if interfaces:
                    # Find the default interface
                    for iface in interfaces:
                        if iface.get('is_default', False):
                            raw_addr = iface.get('address')
                            full_addr = self._get_full_address(iface)
                            print(f"[DEBUG] _get_node_address({node_name}): using default iface {iface.get('name')}, raw={raw_addr}, full={full_addr}")
                            return full_addr

                    # If no default, prefer CAN interfaces over RF
                    # (most inter-node communication happens on CAN buses)
                    can_iface = None
                    for iface in interfaces:
                        iface_name = iface.get('name', '')
                        zmq_proxy = iface.get('zmq_proxy', '')
                        if 'CAN' in iface_name or 'CAN' in zmq_proxy:
                            can_iface = iface
                            break

                    if can_iface:
                        raw_addr = can_iface.get('address')
                        full_addr = self._get_full_address(can_iface)
                        print(f"[DEBUG] _get_node_address({node_name}): using CAN iface {can_iface.get('name')}, raw={raw_addr}, full={full_addr}")
                        return full_addr

                    # Fall back to first interface
                    raw_addr = interfaces[0].get('address')
                    full_addr = self._get_full_address(interfaces[0])
                    print(f"[DEBUG] _get_node_address({node_name}): using first iface {interfaces[0].get('name')}, raw={raw_addr}, full={full_addr}")
                    return full_addr
        print(f"[DEBUG] _get_node_address({node_name}): NODE NOT FOUND!")
        return None

    def _start_background_tasks(self):
        """Start background monitoring tasks"""
        def monitor_status():
            while self.running:
                time.sleep(2)
                # Broadcast status updates
                self.socketio.emit('status_update', {
                    'status': 'Running' if self.launcher.processes else 'Stopped',
                    'nodes': self._get_all_nodes_status()
                })

        thread = threading.Thread(target=monitor_status, daemon=True)
        thread.start()

    def run(self):
        """Run the web server"""
        print(f"\n{'='*60}")
        print(f"Starting Web Dashboard on http://0.0.0.0:{self.port}")
        print(f"{'='*60}\n")

        try:
            self.socketio.run(self.app, host='0.0.0.0', port=self.port, debug=False, allow_unsafe_werkzeug=True)
        except KeyboardInterrupt:
            print("\nShutting down web server...")
        finally:
            self.running = False
            # Stop trace collector
            if hasattr(self, 'trace_collector'):
                self.trace_collector.stop()
            self.launcher.shutdown()


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='Launch and manage CSP virtual topology',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Run a topology with web dashboard (default on http://localhost:9999)
  %(prog)s topologies/examples/linear_3node.json

  # Run with custom web port
  %(prog)s topologies/examples/linear_3node.json --web-port 8080

  # Generate visualization only (don't run topology)
  %(prog)s topologies/examples/linear_3node.json --viz-only --viz-show

  # Specify custom binaries directory
  %(prog)s topologies/examples/linear_3node.json --binaries-dir ./bin
        '''
    )

    parser.add_argument('config', help='Path to topology JSON configuration file')
    parser.add_argument('--binaries-dir', help='Path to directory containing executables (default: same directory as launcher script)')
    parser.add_argument('--visualize', action='store_true', help='Generate topology visualization')
    parser.add_argument('--viz-format', default='png', choices=['png', 'pdf', 'svg', 'dot'],
                       help='Visualization output format (default: png)')
    parser.add_argument('--viz-output', help='Visualization output file (default: <topology_name>_topology)')
    parser.add_argument('--viz-show', action='store_true', help='Show visualization after generation')
    parser.add_argument('--viz-only', action='store_true', help='Only generate visualization, don\'t run topology')
    parser.add_argument('--web-port', type=int, default=9999, help='Web dashboard port (default: 9999)')

    args = parser.parse_args()

    try:
        launcher = TopologyLauncher(args.config, args.binaries_dir)

        # Generate visualization if requested
        if args.visualize or args.viz_only:
            launcher.generate_visualization(
                output_file=args.viz_output,
                format=args.viz_format,
                show=args.viz_show
            )

        # Run topology unless viz-only mode
        if not args.viz_only:
            # Always start web dashboard
            web_server = WebServer(launcher, port=args.web_port)
            # Start topology in background
            launcher.start_control_hub()
            launcher.start_zmq_proxies()
            launcher.start_virtual_nodes()
            # Run web server (blocking)
            web_server.run()

    except FileNotFoundError as e:
        print(f"Error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
