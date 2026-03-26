// CSP Virtual Topology Dashboard JavaScript

// Global variables
let socket;
let network;
let networkNodes;  // vis.js DataSet for nodes
let networkEdges;  // vis.js DataSet for edges
let originalEdges = [];  // Store original edges for filtering
let topologyData = null;
let nodesData = null;
let activeLogNodes = new Set();  // Changed from single node to Set of active nodes
let commandHistory = [];
let historyIndex = -1;

// Activity tracking for log nodes
let nodeActivityTimestamps = new Map();  // Track last activity time for each node
let nodeActivityTimeouts = new Map();    // Track timeout IDs for each node
const ACTIVITY_TIMEOUT_MS = 10000;       // 10 seconds

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    initializeSocketIO();
    setupCLIInput();
});

// Initialize Socket.IO connection
function initializeSocketIO() {
    socket = io();
    window.socket = socket;  // Make socket globally accessible

    socket.on('connect', function() {
        console.log('Connected to server');
        updateConnectionStatus(true);
        requestTopologyData();

        // Setup editor Socket.IO listeners if editor is initialized
        if (window.topologyEditor) {
            window.topologyEditor.setupSocketIOListeners();
        }
    });

    socket.on('disconnect', function() {
        console.log('Disconnected from server');
        updateConnectionStatus(false);
    });

    socket.on('topology_data', function(data) {
        console.log('Received topology data:', data);
        topologyData = data;
        updateTopologyInfo(data);
        updateNodesTable(data.nodes);
        updateLogNodesList(data.nodes);
        // Graph tab has been removed - topology is now in the Editor tab
        // The editor will handle its own rendering

        // Initialize traceroute UI
        if (typeof initTracerouteUI === 'function') {
            initTracerouteUI(data);
        }
    });

    socket.on('status_update', function(data) {
        console.log('Status update:', data);
        updateTopologyStatus(data.status);
        if (data.nodes) {
            updateNodesStatus(data.nodes);
        }
    });

    socket.on('command_response', function(data) {
        appendCLIOutput(data.output);
    });

    socket.on('log_update', function(data) {
        if (activeLogNodes.has(data.node)) {
            appendLogOutput(data.node, data.lines);
        }
        // Update activity indicator for this node
        updateNodeActivity(data.node);
    });

    socket.on('node_stats', function(data) {
        console.log('[node_stats] Received node_stats event:', data);
        console.log('[node_stats] topologyEditor exists:', !!window.topologyEditor);
        console.log('[node_stats] isReadOnlyMode:', window.topologyEditor ? window.topologyEditor.isReadOnlyMode : 'N/A');

        // Only show overlays in read-only mode
        if (window.topologyEditor && window.topologyEditor.isReadOnlyMode) {
            console.log('[node_stats] Calling displayNodeStats on editor');
            window.topologyEditor.displayNodeStats(data);
        }
        // Legacy: also call displayNodeStats for backward compatibility
        // (though this function now also checks for editor-canvas)
        else if (!window.topologyEditor) {
            console.log('[node_stats] Calling legacy displayNodeStats');
            displayNodeStats(data);
        } else {
            console.log('[node_stats] Not in read-only mode, skipping display');
        }
    });
}

// Request topology data from server
function requestTopologyData() {
    fetch('/api/topology')
        .then(response => response.json())
        .then(data => {
            topologyData = data;
            updateTopologyInfo(data);
            updateNodesTable(data.nodes);
            updateLogNodesList(data.nodes);
        })
        .catch(error => console.error('Error fetching topology:', error));
}

// Update connection status indicator
function updateConnectionStatus(connected) {
    const statusEl = document.getElementById('connection-status');
    if (connected) {
        statusEl.textContent = 'Connected';
        statusEl.className = 'badge bg-success';
    } else {
        statusEl.textContent = 'Disconnected';
        statusEl.className = 'badge bg-danger';
    }
}

// Update topology information
function updateTopologyInfo(data) {
    document.getElementById('topology-name').textContent = data.name || '-';
    document.getElementById('topology-description').textContent = data.description || '-';
    document.getElementById('topology-version').textContent = data.csp_version || '-';
}

// Update topology status
function updateTopologyStatus(status) {
    const statusEl = document.getElementById('topology-status');
    statusEl.textContent = status;

    if (status === 'Running') {
        statusEl.className = 'badge status-running';
        document.getElementById('btn-start').disabled = true;
        document.getElementById('btn-stop').disabled = false;
    } else if (status === 'Stopped') {
        statusEl.className = 'badge status-stopped';
        document.getElementById('btn-start').disabled = false;
        document.getElementById('btn-stop').disabled = true;
    } else {
        statusEl.className = 'badge status-error';
    }
}

// Update nodes table
function updateNodesTable(nodes) {
    const tbody = document.getElementById('nodes-table-body');
    tbody.innerHTML = '';

    if (!nodes || nodes.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5" class="text-center">No nodes available</td></tr>';
        return;
    }

    nodes.forEach(node => {
        const row = document.createElement('tr');

        // Build interfaces information
        let interfacesHtml = '';
        if (node.interfaces && node.interfaces.length > 0) {
            interfacesHtml = '<div class="interfaces-list">';
            node.interfaces.forEach(iface => {
                const proxyBadge = iface.zmq_proxy || 'default';
                const isDefault = iface.is_default ? ' <span class="badge bg-success" style="font-size: 0.7em;">default</span>' : '';
                interfacesHtml += `
                    <div class="interface-item">
                        <span class="interface-name">${iface.name}:</span>
                        <span class="interface-addr">${iface.address}</span>
                        <span class="badge bg-secondary interface-proxy">${proxyBadge}</span>${isDefault}
                    </div>
                `;
            });
            interfacesHtml += '</div>';
        } else {
            interfacesHtml = '-';
        }

        row.innerHTML = `
            <td><strong>${node.name}</strong></td>
            <td>${interfacesHtml}</td>
            <td>${node.description || '-'}</td>
            <td><span class="badge ${getStatusClass(node.status)}">${node.status || 'Unknown'}</span></td>
            <td>
                <button class="btn btn-sm btn-primary" onclick="viewNodeLog('${node.name}')">
                    <i class="bi bi-file-text"></i> View Log
                </button>
            </td>
        `;

        tbody.appendChild(row);
    });
}

// Update nodes status
function updateNodesStatus(nodes) {
    nodes.forEach(node => {
        const rows = document.querySelectorAll('#nodes-table-body tr');
        rows.forEach(row => {
            if (row.cells[0] && row.cells[0].textContent === node.name) {
                const statusCell = row.cells[3];
                statusCell.innerHTML = `<span class="badge ${getStatusClass(node.status)}">${node.status}</span>`;
            }
        });
    });
}

// Get status badge class
function getStatusClass(status) {
    if (status === 'Running') return 'status-running';
    if (status === 'Stopped') return 'status-stopped';
    return 'status-error';
}

// Start topology
function startTopology() {
    fetch('/api/start', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                console.log('Topology started');
            } else {
                alert('Failed to start topology: ' + data.error);
            }
        })
        .catch(error => console.error('Error starting topology:', error));
}

// Stop topology
function stopTopology() {
    if (confirm('Are you sure you want to stop the topology?')) {
        fetch('/api/stop', { method: 'POST' })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    console.log('Topology stopped');
                } else {
                    alert('Failed to stop topology: ' + data.error);
                }
            })
            .catch(error => console.error('Error stopping topology:', error));
    }
}

// Setup CLI input handling
function setupCLIInput() {
    const input = document.getElementById('cli-input');

    input.addEventListener('keydown', function(e) {
        if (e.key === 'Enter') {
            sendCommand();
        } else if (e.key === 'ArrowUp') {
            e.preventDefault();
            navigateHistory(-1);
        } else if (e.key === 'ArrowDown') {
            e.preventDefault();
            navigateHistory(1);
        }
    });
}

// Navigate command history
function navigateHistory(direction) {
    if (commandHistory.length === 0) return;

    historyIndex += direction;

    if (historyIndex < 0) {
        historyIndex = 0;
    } else if (historyIndex >= commandHistory.length) {
        historyIndex = commandHistory.length;
        document.getElementById('cli-input').value = '';
        return;
    }

    document.getElementById('cli-input').value = commandHistory[historyIndex];
}

// Send CLI command
function sendCommand() {
    const input = document.getElementById('cli-input');
    const command = input.value.trim();

    if (!command) return;

    // Add to history
    commandHistory.unshift(command);
    if (commandHistory.length > 50) {
        commandHistory.pop();
    }
    historyIndex = -1;

    // Save to localStorage
    localStorage.setItem('cliHistory', JSON.stringify(commandHistory));

    // Display command in output
    appendCLIOutput('control> ' + command + '\n');

    // Send to server
    socket.emit('command', { command: command });

    // Clear input
    input.value = '';
}

// Append to CLI output
function appendCLIOutput(text) {
    const output = document.getElementById('cli-output');
    output.textContent += text;
    output.scrollTop = output.scrollHeight;
}

// Load command history from localStorage
function loadCommandHistory() {
    const saved = localStorage.getItem('cliHistory');
    if (saved) {
        try {
            commandHistory = JSON.parse(saved);
        } catch (e) {
            commandHistory = [];
        }
    }
}

// Initialize command history on load
loadCommandHistory();

// Update log nodes list
function updateLogNodesList(nodes) {
    const list = document.getElementById('log-nodes-list');
    list.innerHTML = '';

    if (!nodes || nodes.length === 0) {
        list.innerHTML = '<button class="btn btn-sm btn-outline-secondary disabled">No nodes available</button>';
        return;
    }

    nodes.forEach(node => {
        const btn = document.createElement('button');
        btn.className = 'btn btn-sm btn-outline-primary';
        btn.id = `log-btn-${node.name}`;

        // Add activity indicator
        const indicator = document.createElement('span');
        indicator.className = 'activity-indicator inactive';
        indicator.id = `activity-${node.name}`;

        btn.appendChild(indicator);
        btn.appendChild(document.createTextNode(node.name));
        btn.onclick = () => viewNodeLog(node.name);
        list.appendChild(btn);
    });
}

// Update node activity indicator
function updateNodeActivity(nodeName) {
    const indicator = document.getElementById(`activity-${nodeName}`);
    if (!indicator) return;

    // Update timestamp
    nodeActivityTimestamps.set(nodeName, Date.now());

    // Clear existing timeout if any
    if (nodeActivityTimeouts.has(nodeName)) {
        clearTimeout(nodeActivityTimeouts.get(nodeName));
    }

    // Set indicator to active (this will trigger the animation)
    indicator.className = 'activity-indicator inactive';
    // Force reflow to restart animation
    void indicator.offsetWidth;
    indicator.className = 'activity-indicator active';

    // Set timeout to mark as inactive after 10 seconds
    const timeoutId = setTimeout(() => {
        const lastActivity = nodeActivityTimestamps.get(nodeName);
        const timeSinceActivity = Date.now() - lastActivity;

        // Only mark inactive if no new activity in the last 10 seconds
        if (timeSinceActivity >= ACTIVITY_TIMEOUT_MS) {
            indicator.className = 'activity-indicator inactive';
            nodeActivityTimeouts.delete(nodeName);
        }
    }, ACTIVITY_TIMEOUT_MS);

    nodeActivityTimeouts.set(nodeName, timeoutId);
}

// View node log (toggle on/off)
function viewNodeLog(nodeName) {
    if (activeLogNodes.has(nodeName)) {
        // Remove log
        removeNodeLog(nodeName);
    } else {
        // Add log
        addNodeLog(nodeName);
    }
}

// Add a node log container
function addNodeLog(nodeName) {
    // Add to active set
    activeLogNodes.add(nodeName);

    // Update button state
    updateLogButtonStates();

    // Create log container
    const container = document.getElementById('log-containers');

    // Remove placeholder if it exists
    const placeholder = container.querySelector('.text-center.text-muted');
    if (placeholder) {
        placeholder.remove();
    }

    // Create new log card
    const logCard = document.createElement('div');
    logCard.className = 'card mb-3 log-card';
    logCard.id = `log-card-${nodeName}`;
    logCard.innerHTML = `
        <div class="card-header d-flex justify-content-between align-items-center bg-dark text-white">
            <h6 class="mb-0">
                <i class="bi bi-file-text"></i> ${nodeName}
            </h6>
            <div>
                <button class="btn btn-sm btn-outline-warning me-2" onclick="clearNodeLog('${nodeName}')" title="Clear log display">
                    <i class="bi bi-eraser"></i> Clear
                </button>
                <a href="/api/logs/${nodeName}" class="btn btn-sm btn-outline-light me-2" download>
                    <i class="bi bi-download"></i> Download
                </a>
                <button class="btn btn-sm btn-outline-light" onclick="removeNodeLog('${nodeName}')">
                    <i class="bi bi-x-lg"></i> Close
                </button>
            </div>
        </div>
        <div class="card-body p-0">
            <div id="log-output-${nodeName}" class="log-output"></div>
        </div>
    `;

    container.appendChild(logCard);

    // Switch to console & logs tab
    const cliTab = new bootstrap.Tab(document.getElementById('cli-tab'));
    cliTab.show();

    // Request log tail
    socket.emit('tail_log', { node: nodeName });
}

// Remove a node log container
function removeNodeLog(nodeName) {
    // Remove from active set
    activeLogNodes.delete(nodeName);

    // Update button state
    updateLogButtonStates();

    // Remove log card
    const logCard = document.getElementById(`log-card-${nodeName}`);
    if (logCard) {
        logCard.remove();
    }

    // Stop tailing
    socket.emit('stop_tail', { node: nodeName });

    // Add placeholder if no logs are active
    const container = document.getElementById('log-containers');
    if (activeLogNodes.size === 0) {
        container.innerHTML = `
            <div class="text-center text-muted p-4">
                <i class="bi bi-info-circle"></i> Select one or more nodes above to view their logs
            </div>
        `;
    }
}

// Clear a single node log display
function clearNodeLog(nodeName) {
    const output = document.getElementById(`log-output-${nodeName}`);
    if (output) {
        output.textContent = '';
    }
}

// Clear all log displays
function clearAllLogs() {
    activeLogNodes.forEach(nodeName => {
        clearNodeLog(nodeName);
    });
}

// Update log button states
function updateLogButtonStates() {
    const buttons = document.querySelectorAll('#log-nodes-list button');
    buttons.forEach(btn => {
        const nodeName = btn.textContent;
        if (activeLogNodes.has(nodeName)) {
            btn.classList.remove('btn-outline-primary');
            btn.classList.add('btn-primary');
        } else {
            btn.classList.remove('btn-primary');
            btn.classList.add('btn-outline-primary');
        }
    });

    // Show/hide "Clear All" button based on active logs
    const clearAllBtn = document.getElementById('clear-all-logs-btn');
    if (clearAllBtn) {
        clearAllBtn.style.display = activeLogNodes.size > 0 ? 'block' : 'none';
    }
}

// Append to log output for a specific node
function appendLogOutput(nodeName, lines) {
    const output = document.getElementById(`log-output-${nodeName}`);
    if (output) {
        output.textContent += lines.join('\n') + '\n';
        output.scrollTop = output.scrollHeight;
    }
}

// Render topology graph with vis.js
// NOTE: This function is deprecated - topology is now rendered in editor.js
function renderTopologyGraph(data) {
    const container = document.getElementById('editor-canvas');

    if (!container) {
        console.warn('Editor canvas not found');
        return;
    }

    if (!data || !data.nodes) {
        console.warn('No topology data available');
        return;
    }

    // Store topology data globally for click handlers
    window.topologyGraphData = data;

    // Build nodes for vis.js
    const visNodes = [];
    const visEdges = [];
    const nodeMap = new Map();

    // Create proxy nodes (subnets)
    const proxyColors = {
        'RF': '#ff6b6b',
        'CAN': '#4ecdc4',
        'I2C': '#95e1d3',
        'default': '#a8dadc'
    };

    if (data.zmq_proxies) {
        data.zmq_proxies.forEach(proxy => {
            const proxyId = 'proxy_' + proxy.name;
            const bgColor = proxyColors[proxy.name] || proxyColors['default'];
            visNodes.push({
                id: proxyId,
                label: proxy.name + '\n(Subnet)',
                shape: 'box',
                color: {
                    background: bgColor,
                    border: '#2c3e50',
                    hover: {
                        background: bgColor,
                        border: '#1a252f'
                    },
                    highlight: {
                        background: bgColor,
                        border: '#1a252f'
                    }
                },
                font: { color: '#2c3e50', size: 14, bold: true },
                margin: 10
            });
        });
    }

    // Create node nodes
    data.nodes.forEach(node => {
        const interfaces = node.interfaces || [];
        const routingTable = node.routing_table || [];
        const hasRoutes = routingTable.length > 0;

        // Build label showing all interface addresses grouped by proxy
        let label = `${node.name}\n`;
        const ifacesByProxy = {};
        interfaces.forEach(iface => {
            const proxy = iface.zmq_proxy || 'default';
            if (!ifacesByProxy[proxy]) {
                ifacesByProxy[proxy] = [];
            }
            ifacesByProxy[proxy].push(iface.address);
        });

        // Add interface info to label
        for (const [proxy, addrs] of Object.entries(ifacesByProxy)) {
            label += `${proxy}: ${addrs.join(',')}\n`;
        }

        // Add routing warning if no routes defined
        if (!hasRoutes) {
            label += '⚠ No routes';
        }

        // Determine node color based on routing configuration
        // Nodes with empty routing tables get orange/warning color
        const nodeColor = hasRoutes ? {
            background: '#457b9d',
            border: '#1d3557',
            hover: { background: '#2c5f7a', border: '#1d3557' },
            highlight: { background: '#2c5f7a', border: '#1d3557' }
        } : {
            background: '#f39c12',  // Orange/warning color
            border: '#d35400',
            hover: { background: '#e67e22', border: '#d35400' },
            highlight: { background: '#e67e22', border: '#d35400' }
        };

        visNodes.push({
            id: node.name,
            label: label.trim(),
            shape: 'ellipse',
            color: nodeColor,
            font: { color: '#ffffff', size: 11 },
            margin: 12,
            borderWidth: hasRoutes ? 2 : 4,  // Thicker border for warning
            nodeData: node  // Store full node data for stats
        });

        nodeMap.set(node.name, node);

        // Create edges from node to proxies based on interfaces
        // Track edges per proxy to add curvature for multiple edges
        const edgesPerProxy = {};

        interfaces.forEach((iface, index) => {
            const proxyName = iface.zmq_proxy || 'default';
            const proxyId = 'proxy_' + proxyName;
            const isDefault = iface.is_default === true;

            // Count edges to this proxy
            if (!edgesPerProxy[proxyId]) {
                edgesPerProxy[proxyId] = 0;
            }
            const edgeIndex = edgesPerProxy[proxyId];
            edgesPerProxy[proxyId]++;

            // Calculate smooth curve for multiple edges to same proxy
            const smooth = edgesPerProxy[proxyId] > 1 ? {
                enabled: true,
                type: 'curvedCW',
                roundness: 0.2 + (edgeIndex * 0.15)
            } : { enabled: false };

            // Build edge label with default indicator
            const defaultIndicator = isDefault ? ' ★' : '';
            const edgeLabel = `${iface.name}${defaultIndicator}\nAddr: ${iface.address}`;

            // Style edge based on whether it's the default interface
            // Default interfaces: solid, thicker line
            // Non-default interfaces: dashed, thinner line
            const edgeStyle = isDefault ? {
                color: { color: '#27ae60' },  // Green for default
                width: 4,
                dashes: false
            } : {
                color: { color: '#95a5a6' },  // Gray for non-default
                width: 2,
                dashes: [5, 5]  // Dashed line
            };

            visEdges.push({
                from: node.name,
                to: proxyId,
                label: edgeLabel,
                font: { size: 9, align: 'middle' },
                arrows: { to: false, from: false },
                color: edgeStyle.color,
                width: edgeStyle.width,
                dashes: edgeStyle.dashes,
                smooth: smooth
            });
        });
    });

    // Create vis.js network with DataSets stored globally for filtering
    networkNodes = new vis.DataSet(visNodes);
    networkEdges = new vis.DataSet(visEdges);

    // Store original edges for filtering
    originalEdges = visEdges.map(e => ({...e}));

    const graphData = {
        nodes: networkNodes,
        edges: networkEdges
    };

    const options = {
        layout: {
            hierarchical: {
                enabled: false
            }
        },
        physics: {
            enabled: true,
            barnesHut: {
                gravitationalConstant: -12000,  // Moderate repulsion
                centralGravity: 0.2,            // Reduced central pull
                springLength: 220,              // Compact by default (was 350)
                springConstant: 0.02,           // Reduced spring stiffness
                avoidOverlap: 0.5               // Add overlap avoidance
            },
            stabilization: {
                iterations: 300                 // More iterations for better layout
            }
        },
        interaction: {
            hover: true,
            tooltipDelay: 100
        },
        nodes: {
            borderWidth: 2,
            borderWidthSelected: 4
        }
    };

    network = new vis.Network(container, graphData, options);

    // Handle node hover for stats
    let statsHideTimeout = null;

    network.on('hoverNode', function(params) {
        const nodeId = params.node;
        if (!nodeId.startsWith('proxy_')) {
            // Clear any pending hide timeout
            if (statsHideTimeout) {
                clearTimeout(statsHideTimeout);
                statsHideTimeout = null;
            }
            // Store hover position for overlay positioning
            window.lastHoverPosition = params.pointer.DOM;
            requestNodeStats(nodeId);
        }
    });

    network.on('blurNode', function(params) {
        // Delay hiding to allow user to move mouse to overlay
        statsHideTimeout = setTimeout(function() {
            hideNodeStats();
        }, 300);
    });

    // Keep overlay visible when hovering over it
    const overlay = document.getElementById('node-stats-overlay');
    if (overlay) {
        overlay.addEventListener('mouseenter', function() {
            if (statsHideTimeout) {
                clearTimeout(statsHideTimeout);
                statsHideTimeout = null;
            }
        });

        overlay.addEventListener('mouseleave', function() {
            hideNodeStats();
        });
    }

    // Handle click on proxy nodes to show subnet members
    network.on('click', function(params) {
        if (params.nodes.length > 0) {
            const nodeId = params.nodes[0];
            if (nodeId.startsWith('proxy_')) {
                // Get click position
                const canvasPos = params.pointer.canvas;
                const domPos = params.pointer.DOM;

                // Show subnet members
                showSubnetMembers(nodeId, domPos);
            }
        }
    });
}

// Request node stats from server
function requestNodeStats(nodeName) {
    socket.emit('get_node_stats', { node: nodeName });
}

// Display node stats in overlay
function displayNodeStats(data) {
    const overlay = document.getElementById('node-stats-overlay');
    const statsNodeName = document.getElementById('stats-node-name');
    const statsContent = document.getElementById('stats-content');

    statsNodeName.textContent = data.node;

    // Build content with stats and routing info
    let content = data.stats || 'No statistics available';

    // Append routing configuration from topology data
    const topoData = window.topologyGraphData;
    if (topoData && topoData.nodes) {
        const nodeConfig = topoData.nodes.find(n => n.name === data.node);
        if (nodeConfig) {
            content += '\n\n--- Routing Configuration ---\n';

            // Show interfaces with default indicator
            const interfaces = nodeConfig.interfaces || [];
            if (interfaces.length > 0) {
                content += 'Interfaces:\n';
                interfaces.forEach(iface => {
                    const defaultMark = iface.is_default ? ' [DEFAULT]' : '';
                    content += `  ${iface.name}: addr=${iface.address}, proxy=${iface.zmq_proxy || 'default'}${defaultMark}\n`;
                });
            }

            // Show routing table
            const routes = nodeConfig.routing_table || [];
            content += '\nRouting Table:\n';
            if (routes.length === 0) {
                content += '  ⚠ EMPTY - uses default interface only\n';
            } else {
                routes.forEach(route => {
                    content += `  ${route.address}/${route.netmask} → ${route.interface}\n`;
                });
            }
        }
    }

    statsContent.textContent = content;

    // Position overlay near the hovered node if position is available
    if (window.lastHoverPosition) {
        const domPos = window.lastHoverPosition;
        const graphContainer = document.getElementById('editor-canvas');
        if (!graphContainer) {
            console.warn('Editor canvas not found, cannot position overlay');
            return;
        }
        const containerRect = graphContainer.getBoundingClientRect();

        // Calculate position relative to graph container
        let left = domPos.x + 20;
        let top = domPos.y + 20;

        // Ensure overlay stays within bounds
        const overlayWidth = 350;
        const overlayHeight = 500;

        if (left + overlayWidth > containerRect.width) {
            left = domPos.x - overlayWidth - 20;
        }
        if (top + overlayHeight > containerRect.height) {
            top = containerRect.height - overlayHeight - 20;
        }
        if (left < 0) left = 20;
        if (top < 0) top = 20;

        overlay.style.left = left + 'px';
        overlay.style.top = top + 'px';
        overlay.style.right = 'auto';
        overlay.style.bottom = 'auto';
    } else {
        // Fallback to default position (top-right)
        overlay.style.top = '20px';
        overlay.style.right = '20px';
        overlay.style.left = 'auto';
        overlay.style.bottom = 'auto';
    }

    overlay.style.display = 'block';
}

// Show subnet members when clicking on a proxy node
function showSubnetMembers(proxyId, domPos) {
    const overlay = document.getElementById('node-stats-overlay');
    const statsNodeName = document.getElementById('stats-node-name');
    const statsContent = document.getElementById('stats-content');

    // Extract proxy name from ID (remove 'proxy_' prefix)
    const proxyName = proxyId.replace('proxy_', '');

    // Get topology data
    const data = window.topologyGraphData;
    if (!data || !data.nodes) {
        return;
    }

    // Find all nodes connected to this proxy
    const members = [];
    data.nodes.forEach(node => {
        if (node.interfaces) {
            node.interfaces.forEach(iface => {
                if (iface.zmq_proxy === proxyName) {
                    members.push({
                        name: node.name,
                        address: iface.address,
                        interfaceName: iface.name
                    });
                }
            });
        }
    });

    // Build content
    let content = `Subnet: ${proxyName}\n`;
    content += `Members: ${members.length}\n\n`;

    if (members.length > 0) {
        members.forEach(member => {
            content += `${member.name}: ${member.address} (${member.interfaceName})\n`;
        });
    } else {
        content += 'No nodes connected';
    }

    statsNodeName.textContent = `Subnet: ${proxyName}`;
    statsContent.textContent = content;

    // Position overlay near the clicked node
    const graphContainer = document.getElementById('editor-canvas');
    if (!graphContainer) {
        console.warn('Editor canvas not found, cannot position overlay');
        return;
    }
    const containerRect = graphContainer.getBoundingClientRect();

    // Calculate position relative to graph container
    let left = domPos.x + 20;
    let top = domPos.y + 20;

    // Ensure overlay stays within bounds
    const overlayWidth = 350;
    const overlayHeight = 400;

    if (left + overlayWidth > containerRect.width) {
        left = domPos.x - overlayWidth - 20;
    }
    if (top + overlayHeight > containerRect.height) {
        top = containerRect.height - overlayHeight - 20;
    }
    if (left < 0) left = 20;
    if (top < 0) top = 20;

    overlay.style.left = left + 'px';
    overlay.style.top = top + 'px';
    overlay.style.right = 'auto';
    overlay.style.bottom = 'auto';

    overlay.style.display = 'block';
}

// Hide node stats overlay
function hideNodeStats() {
    const overlay = document.getElementById('node-stats-overlay');
    overlay.style.display = 'none';
}

// Hide node stats overlay (called from close button)
function hideNodeStatsOverlay() {
    hideNodeStats();
}

// Graph tab has been removed - the topology is now in the Editor tab
// The editor handles its own rendering, so this listener is no longer needed

// Apply graph filters based on checkbox states
function applyGraphFilters() {
    if (!networkNodes || !networkEdges || !originalEdges) {
        return;
    }

    const showSubnets = document.getElementById('filter-show-subnets').checked;
    const showNodes = document.getElementById('filter-show-nodes').checked;

    // Get all nodes from the DataSet
    const allNodes = networkNodes.get();

    // Restore original edges first
    networkEdges.clear();
    networkEdges.add(originalEdges);

    // Determine which nodes to hide
    const nodesToHide = new Set();
    const nodesToShow = new Set();
    const subnetNodes = new Set();
    const regularNodes = new Set();

    allNodes.forEach(node => {
        const isSubnet = node.id.startsWith('proxy_');

        if (isSubnet) {
            subnetNodes.add(node.id);
            if (!showSubnets) {
                nodesToHide.add(node.id);
            } else {
                nodesToShow.add(node.id);
            }
        } else {
            regularNodes.add(node.id);
            if (!showNodes) {
                nodesToHide.add(node.id);
            } else {
                nodesToShow.add(node.id);
            }
        }
    });

    // Create bridge edges when nodes are hidden
    const bridgeEdges = [];
    let bridgeEdgeId = 10000; // Start with high ID to avoid conflicts

    if (!showSubnets && showNodes) {
        // Subnets are hidden, nodes are visible
        // Create edges between nodes that share a subnet
        subnetNodes.forEach(subnetId => {
            // Find all nodes connected to this subnet
            const connectedNodes = [];
            originalEdges.forEach(edge => {
                if (edge.from === subnetId && regularNodes.has(edge.to)) {
                    connectedNodes.push(edge.to);
                } else if (edge.to === subnetId && regularNodes.has(edge.from)) {
                    connectedNodes.push(edge.from);
                }
            });

            // Get subnet name for label (remove 'proxy_' prefix)
            const subnetName = subnetId.replace('proxy_', '');

            // Create edges between all pairs of connected nodes
            for (let i = 0; i < connectedNodes.length; i++) {
                for (let j = i + 1; j < connectedNodes.length; j++) {
                    bridgeEdges.push({
                        id: `bridge_${bridgeEdgeId++}`,
                        from: connectedNodes[i],
                        to: connectedNodes[j],
                        color: { color: '#4a90e2' },  // Blue color, fully opaque
                        dashes: [10, 5],              // Longer dashes for visibility
                        width: 2,                     // Thicker line
                        label: `via ${subnetName}`,   // Show which subnet connects them
                        font: {
                            size: 12,
                            color: '#4a90e2',
                            background: 'rgba(255, 255, 255, 0.8)',
                            strokeWidth: 0
                        }
                    });
                }
            }
        });
    } else if (showSubnets && !showNodes) {
        // Nodes are hidden, subnets are visible
        // Create edges between subnets that are connected via router nodes
        regularNodes.forEach(nodeId => {
            // Find all subnets connected to this node
            const connectedSubnets = [];
            originalEdges.forEach(edge => {
                if (edge.from === nodeId && subnetNodes.has(edge.to)) {
                    connectedSubnets.push(edge.to);
                } else if (edge.to === nodeId && subnetNodes.has(edge.from)) {
                    connectedSubnets.push(edge.from);
                }
            });

            // Create edges between all pairs of connected subnets
            for (let i = 0; i < connectedSubnets.length; i++) {
                for (let j = i + 1; j < connectedSubnets.length; j++) {
                    bridgeEdges.push({
                        id: `bridge_${bridgeEdgeId++}`,
                        from: connectedSubnets[i],
                        to: connectedSubnets[j],
                        color: { color: '#e67e22' },  // Orange color, fully opaque
                        dashes: [10, 5],              // Longer dashes for visibility
                        width: 2,                     // Thicker line
                        label: `via ${nodeId}`,       // Show which node connects them
                        font: {
                            size: 12,
                            color: '#e67e22',
                            background: 'rgba(255, 255, 255, 0.8)',
                            strokeWidth: 0
                        }
                    });
                }
            }
        });
    }

    // Add bridge edges to the network
    if (bridgeEdges.length > 0) {
        networkEdges.add(bridgeEdges);
    }

    // Hide edges connected to hidden nodes (but not bridge edges)
    const allEdges = networkEdges.get();
    const edgesToHide = [];

    allEdges.forEach(edge => {
        if (!edge.id.toString().startsWith('bridge_')) {
            if (nodesToHide.has(edge.from) || nodesToHide.has(edge.to)) {
                edgesToHide.push(edge.id);
            }
        }
    });

    // Update nodes visibility
    const nodeUpdates = [];
    nodesToHide.forEach(id => {
        nodeUpdates.push({ id: id, hidden: true });
    });
    nodesToShow.forEach(id => {
        nodeUpdates.push({ id: id, hidden: false });
    });

    // Update edges visibility
    const edgeUpdates = edgesToHide.map(id => ({ id: id, hidden: true }));

    // Apply updates
    if (nodeUpdates.length > 0) {
        networkNodes.update(nodeUpdates);
    }
    if (edgeUpdates.length > 0) {
        networkEdges.update(edgeUpdates);
    }

    // Adaptive spring length: use larger spacing when filtered
    if (network) {
        const isFiltered = !showSubnets || !showNodes;
        const springLength = isFiltered ? 320 : 220;  // Larger when filtered, compact when all shown

        network.setOptions({
            physics: {
                barnesHut: {
                    springLength: springLength
                }
            }
        });

        // Fit the network to show visible nodes
        network.fit({
            animation: {
                duration: 500,
                easingFunction: 'easeInOutQuad'
            }
        });
    }
}

// Reset graph filters to show everything
function resetGraphFilters() {
    document.getElementById('filter-show-subnets').checked = true;
    document.getElementById('filter-show-nodes').checked = true;
    applyGraphFilters();
}

// Fit graph to screen
function fitGraphToScreen() {
    if (network) {
        network.fit({
            animation: {
                duration: 500,
                easingFunction: 'easeInOutQuad'
            }
        });
    }
}

