// CSP Topology Editor JavaScript
// Provides interactive topology creation and editing capabilities

// =============================================================================
// TopologyEditor Class
// =============================================================================

class TopologyEditor {
    constructor() {
        // Vis.js network instance
        this.network = null;
        this.networkNodes = null;
        this.networkEdges = null;

        // Topology data structure (matches schema)
        this.topology = this.createEmptyTopology();

        // Editor state
        this.selectedNodeId = null;
        this.selectedSubnetId = null;
        this.isDirty = false;
        this.isRestarting = false;

        // Mode management (Read-only vs Edit)
        this.isReadOnlyMode = true;  // Default to read-only when topology loaded
        this.isEmptyTopology = false; // Track if started with empty topology

        // Undo/Redo stacks
        this.undoStack = [];
        this.redoStack = [];
        this.maxUndoLevels = 50;

        // Auto-save
        this.autoSaveInterval = null;
        this.lastSaveTime = null;

        // Live topology sync
        this.isLiveTopology = false;  // True when editing a running topology
        this.serverTopologyName = null;  // Name of the topology loaded from server

        // Filtering state
        this.originalEdges = [];  // Store original edges for filtering
        this.showSubnets = true;
        this.showNodes = true;

        // Subnet colors for visualization
        this.subnetColors = [
            '#ff6b6b', '#4ecdc4', '#95e1d3', '#a8dadc', '#f9c74f',
            '#90be6d', '#43aa8b', '#577590', '#f8961e', '#f3722c'
        ];
        this.subnetColorIndex = 0;

        // Routing intelligence
        this.routingAnalyzer = null;
        this.routingSuggestions = null;

        // Event handlers for mode switching (stored as properties for proper cleanup)
        this.readOnlyClickHandler = null;
        this.editModeClickHandler = null;
    }

    // Create an empty topology structure
    createEmptyTopology() {
        return {
            topology: {
                name: 'New Topology',
                description: 'Created with CSP Topology Editor',
                csp_version: 2,
                deduplication: 'all',
                zmq_proxies: []
            },
            nodes: [],
            control_hub: {
                enabled: true,
                port: 5555
            }
        };
    }

    // Initialize the editor
    init() {
        console.log('Initializing Topology Editor...');

        // Initialize routing intelligence
        this.routingAnalyzer = new RoutingAnalyzer(this.topology);
        this.routingSuggestions = new RoutingSuggestions(this.topology, this.routingAnalyzer);

        this.initCanvas();
        this.setupEventHandlers();
        this.setupSocketIOListeners();

        // Try to load from server (running topology)
        this.loadFromServer().then(loaded => {
            // Determine initial mode based on topology state
            if (this.topology.nodes.length === 0) {
                // Empty topology - start in edit mode
                this.isEmptyTopology = true;
                console.log('Setting initial mode: EDIT (empty topology)');
                this.setEditMode();
            } else if (loaded) {
                // Topology loaded from server - start in read-only mode
                console.log('Setting initial mode: READ-ONLY (loaded from server)');
                this.setReadOnlyMode();
            } else {
                // No topology on server - start in edit mode
                console.log('Setting initial mode: EDIT (no server topology)');
                this.setEditMode();
            }

            this.updateUI();
            this.updateLiveTopologyIndicator();
            console.log('Topology Editor initialized. Mode:', this.isReadOnlyMode ? 'READ-ONLY' : 'EDIT');
        });
    }

    // Load topology from server API (running topology)
    async loadFromServer() {
        try {
            const response = await fetch('/api/topology');
            if (!response.ok) {
                console.log('No topology available from server');
                return false;
            }

            const data = await response.json();

            // Check if we got a valid topology
            if (data && (data.nodes || data.topology)) {
                // Server returns flat structure, convert to editor format
                this.topology = this.normalizeTopology(data);
                this.isLiveTopology = true;
                this.serverTopologyName = this.topology.topology?.name || 'Server Topology';

                // Update routing analyzer with new topology
                this.updateRoutingAnalyzer();

                this.renderCanvas();
                this.updateUI();
                console.log('Topology loaded from server:', this.serverTopologyName);
                return true;
            }
            return false;
        } catch (e) {
            console.log('Could not load from server:', e.message);
            return false;
        }
    }

    // Normalize server topology format to editor format
    normalizeTopology(data) {
        // The server may return either:
        // 1. Full structure: { topology: {...}, nodes: [...], control_hub: {...} }
        // 2. Flat structure: { name, zmq_proxies, nodes, ... }

        if (data.topology && data.nodes) {
            // Already in correct format
            return data;
        }

        // Convert flat structure to editor format
        return {
            topology: {
                name: data.name || 'Imported Topology',
                description: data.description || '',
                csp_version: data.csp_version || 2,
                deduplication: data.deduplication || 'all',
                zmq_proxies: data.zmq_proxies || []
            },
            nodes: data.nodes || [],
            control_hub: data.control_hub || { enabled: true, port: 5555 }
        };
    }

    // ==========================================================================
    // Mode Switching (Read-only vs Edit)
    // ==========================================================================

    // Switch to read-only mode
    setReadOnlyMode() {
        // Check for unsaved changes
        if (!this.isReadOnlyMode && this.isDirty) {
            if (!confirm('You have unsaved changes. Switch to read-only mode anyway?')) {
                return;
            }
        }

        this.isReadOnlyMode = true;

        // Update UI classes
        const editorTab = document.getElementById('editor');
        if (editorTab) {
            editorTab.classList.remove('edit-mode');
            editorTab.classList.add('readonly-mode');
        }

        // Update mode toggle button
        const modeBtn = document.getElementById('editor-mode-toggle');
        if (modeBtn) {
            modeBtn.innerHTML = '<i class="bi bi-pencil"></i> Edit Mode';
            modeBtn.title = 'Switch to edit mode';
        }

        // Enable overlays
        this.enableOverlays();

        // Deselect any selected nodes
        this.deselectAll();

        // Resize canvas to full width
        this.resizeCanvas();

        this.updateStatus('Read-only mode');
        console.log('Switched to read-only mode');
    }

    // Switch to edit mode
    setEditMode() {
        this.isReadOnlyMode = false;

        // Update UI classes
        const editorTab = document.getElementById('editor');
        if (editorTab) {
            editorTab.classList.remove('readonly-mode');
            editorTab.classList.add('edit-mode');
        }

        // Update mode toggle button
        const modeBtn = document.getElementById('editor-mode-toggle');
        if (modeBtn) {
            modeBtn.innerHTML = '<i class="bi bi-eye"></i> Read-only Mode';
            modeBtn.title = 'Switch to read-only mode';
        }

        // Disable overlays
        this.disableOverlays();

        // Resize canvas to three-column layout
        this.resizeCanvas();

        this.updateStatus('Edit mode');
        console.log('Switched to edit mode');
    }

    // Toggle between modes
    toggleMode() {
        if (this.isReadOnlyMode) {
            this.setEditMode();
        } else {
            this.setReadOnlyMode();
        }
    }

    // Enable hover overlays for read-only mode
    enableOverlays() {
        if (!this.network) return;

        // Remove all existing listeners first
        this.network.off('hoverNode');
        this.network.off('blurNode');
        this.network.off('click');

        // Create and store the read-only click handler
        this.readOnlyClickHandler = (params) => {
            if (params.nodes.length > 0) {
                const nodeId = params.nodes[0];
                window.lastHoverPosition = params.pointer.DOM;

                // Check if this is a subnet/proxy node
                if (nodeId.startsWith('subnet_') || nodeId.startsWith('proxy_')) {
                    // Show subnet members
                    this.displaySubnetMembers(nodeId, params.pointer.DOM);
                } else {
                    // In read-only mode, clicking shows stats overlay
                    this.requestNodeStats(nodeId);
                }
            }
        };

        // Add hover listeners for stats overlay
        this.network.on('hoverNode', (params) => {
            const nodeId = params.node;
            window.lastHoverPosition = params.pointer.DOM;
            console.log('[enableOverlays] Hovering over node:', nodeId, 'Is subnet:', nodeId.startsWith('subnet_'));

            // Check if this is a subnet/proxy node
            if (nodeId.startsWith('proxy_') || nodeId.startsWith('subnet_')) {
                // Show subnet members
                this.displaySubnetMembers(nodeId, params.pointer.DOM);
            } else {
                // Show regular node stats
                console.log('[enableOverlays] Requesting stats for regular node:', nodeId);
                this.requestNodeStats(nodeId);
            }
        });

        this.network.on('blurNode', () => {
            setTimeout(() => this.hideNodeStats(), 300);
        });

        // Add the stored click handler for read-only mode
        this.network.on('click', this.readOnlyClickHandler);
    }

    // Disable overlays for edit mode
    disableOverlays() {
        if (!this.network) return;

        // Remove all existing listeners
        this.network.off('hoverNode');
        this.network.off('blurNode');
        this.network.off('click');

        // Hide any visible overlays
        this.hideNodeStats();

        // Create and store the edit mode click handler
        this.editModeClickHandler = (params) => {
            if (params.nodes.length > 0) {
                const nodeId = params.nodes[0];
                if (nodeId.startsWith('subnet_')) {
                    this.selectSubnet(nodeId.replace('subnet_', ''));
                } else {
                    // Select regular nodes
                    this.selectNode(nodeId);
                }
            } else {
                this.deselectAll();
            }
        };

        // Add the stored click handler for edit mode
        this.network.on('click', this.editModeClickHandler);
    }

    // Resize canvas when switching modes
    resizeCanvas() {
        if (this.network) {
            setTimeout(() => {
                this.network.redraw();
                this.network.fit();
            }, 100);
        }
    }

    // ==========================================================================
    // Node Stats Overlay (Read-only mode)
    // ==========================================================================

    // Request node stats from server
    requestNodeStats(nodeName) {
        if (window.socket) {
            window.socket.emit('get_node_stats', { node: nodeName });
        }
    }

    // Display node stats in overlay
    displayNodeStats(data) {
        // Only show overlay in read-only mode
        if (!this.isReadOnlyMode) return;

        const overlay = document.getElementById('node-stats-overlay');
        const statsNodeName = document.getElementById('stats-node-name');
        const statsContent = document.getElementById('stats-content');

        if (!overlay || !statsNodeName || !statsContent) return;

        statsNodeName.textContent = data.node;

        // Build content with interfaces and routing info
        let content = '';

        // Get node configuration from topology
        const nodeConfig = this.topology.nodes.find(n => n.name === data.node);
        console.log('[displayNodeStats] Node:', data.node, 'Config found:', !!nodeConfig, 'Topology nodes:', this.topology.nodes.length);

        if (nodeConfig) {
            // Show interfaces with addresses and netmasks
            const interfaces = nodeConfig.interfaces || [];
            console.log('[displayNodeStats] Interfaces:', interfaces.length);

            if (interfaces.length > 0) {
                content += 'Interfaces:\n';
                interfaces.forEach(iface => {
                    const subnet = this.getSubnet(iface.zmq_proxy);
                    const netmask = subnet ? subnet.netmask : '?';
                    const defaultMark = iface.is_default ? ' [DEFAULT]' : '';
                    content += `  ${iface.name}: ${iface.address}/${netmask}`;
                    if (iface.zmq_proxy) {
                        content += ` (${iface.zmq_proxy})`;
                    }
                    content += defaultMark + '\n';
                });
            } else {
                content += 'No interfaces configured\n';
            }

            // Show routing table
            const routingTable = nodeConfig.routing_table || [];
            if (routingTable.length > 0) {
                content += '\nRouting Table:\n';
                routingTable.forEach(route => {
                    content += `  ${route.address}/${route.netmask} → ${route.interface}\n`;
                });
            } else {
                content += '\n⚠ No routing table defined\n';
            }
        } else {
            content = 'Node configuration not found';
            console.log('[displayNodeStats] Node not found. Available nodes:', this.topology.nodes.map(n => n.name));
        }

        statsContent.textContent = content;

        // Position overlay near the hovered node if position is available
        if (window.lastHoverPosition) {
            const domPos = window.lastHoverPosition;
            const canvasContainer = document.getElementById('editor-canvas');
            if (canvasContainer) {
                const containerRect = canvasContainer.getBoundingClientRect();

                // Calculate position relative to canvas container
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
            }
        }

        overlay.style.display = 'block';
    }

    // Hide node stats overlay
    hideNodeStats() {
        const overlay = document.getElementById('node-stats-overlay');
        if (overlay) {
            overlay.style.display = 'none';
        }
    }

    // Display subnet members in overlay
    displaySubnetMembers(subnetId, domPos) {
        // Only show overlay in read-only mode
        if (!this.isReadOnlyMode) return;

        const overlay = document.getElementById('node-stats-overlay');
        const statsNodeName = document.getElementById('stats-node-name');
        const statsContent = document.getElementById('stats-content');

        if (!overlay || !statsNodeName || !statsContent) return;

        // Extract subnet name from ID (remove 'subnet_' or 'proxy_' prefix)
        const subnetName = subnetId.replace(/^(subnet_|proxy_)/, '');

        // Find the subnet configuration
        const subnet = this.getSubnet(subnetName);
        if (!subnet) {
            statsNodeName.textContent = subnetName;
            statsContent.textContent = 'Subnet not found';
            return;
        }

        // Set header
        statsNodeName.textContent = `Subnet: ${subnetName}`;

        // Find all nodes connected to this subnet
        const members = [];
        this.topology.nodes.forEach(node => {
            node.interfaces.forEach(iface => {
                if (iface.zmq_proxy === subnetName) {
                    members.push({
                        name: node.name,
                        address: iface.address,
                        interfaceName: iface.name
                    });
                }
            });
        });

        // Build content
        let content = `Subnet Prefix: ${subnet.subnet_prefix}/${subnet.netmask}\n`;
        content += `Members: ${members.length}\n\n`;

        if (members.length > 0) {
            members.forEach(member => {
                content += `${member.name}: ${member.address} (${member.interfaceName})\n`;
            });
        } else {
            content += 'No nodes connected to this subnet';
        }

        statsContent.textContent = content;

        // Position overlay near the hovered node if position is available
        if (domPos) {
            const canvasContainer = document.getElementById('editor-canvas');
            if (canvasContainer) {
                const containerRect = canvasContainer.getBoundingClientRect();

                // Calculate position relative to canvas container
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
            }
        }

        overlay.style.display = 'block';
    }

    // ==========================================================================
    // Graph Filtering (Read-only mode)
    // ==========================================================================

    // Apply graph filters based on checkbox states
    applyGraphFilters() {
        if (!this.networkNodes || !this.networkEdges || !this.originalEdges) {
            return;
        }

        const showSubnetsCheckbox = document.getElementById('editor-filter-show-subnets');
        const showNodesCheckbox = document.getElementById('editor-filter-show-nodes');

        this.showSubnets = showSubnetsCheckbox ? showSubnetsCheckbox.checked : true;
        this.showNodes = showNodesCheckbox ? showNodesCheckbox.checked : true;

        // Get all nodes from the DataSet
        const allNodes = this.networkNodes.get();

        // Restore original edges first
        this.networkEdges.clear();
        this.networkEdges.add(this.originalEdges);

        // Determine which nodes to hide
        const nodesToHide = new Set();
        const nodesToShow = new Set();
        const subnetNodes = new Set();
        const regularNodes = new Set();

        allNodes.forEach(node => {
            const isSubnet = node.id.startsWith('subnet_');

            if (isSubnet) {
                subnetNodes.add(node.id);
                if (!this.showSubnets) {
                    nodesToHide.add(node.id);
                } else {
                    nodesToShow.add(node.id);
                }
            } else {
                regularNodes.add(node.id);
                if (!this.showNodes) {
                    nodesToHide.add(node.id);
                } else {
                    nodesToShow.add(node.id);
                }
            }
        });

        // Create bridge edges when nodes are hidden
        const bridgeEdges = [];
        let bridgeEdgeId = 10000;

        if (!this.showSubnets && this.showNodes) {
            // Subnets are hidden, nodes are visible
            // Create edges between nodes that share a subnet
            subnetNodes.forEach(subnetId => {
                const connectedNodes = [];
                this.originalEdges.forEach(edge => {
                    if (edge.from === subnetId && regularNodes.has(edge.to)) {
                        connectedNodes.push(edge.to);
                    } else if (edge.to === subnetId && regularNodes.has(edge.from)) {
                        connectedNodes.push(edge.from);
                    }
                });

                // Create edges between all pairs of connected nodes
                for (let i = 0; i < connectedNodes.length; i++) {
                    for (let j = i + 1; j < connectedNodes.length; j++) {
                        bridgeEdges.push({
                            id: `bridge_${bridgeEdgeId++}`,
                            from: connectedNodes[i],
                            to: connectedNodes[j],
                            color: { color: '#cccccc', opacity: 0.5 },
                            dashes: true,
                            width: 1
                        });
                    }
                }
            });

            // Add bridge edges
            if (bridgeEdges.length > 0) {
                this.networkEdges.add(bridgeEdges);
            }
        }

        // Hide edges connected to hidden nodes
        const allEdges = this.networkEdges.get();
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
            this.networkNodes.update(nodeUpdates);
        }
        if (edgeUpdates.length > 0) {
            this.networkEdges.update(edgeUpdates);
        }

        // Adaptive spring length: use larger spacing when filtered
        if (this.network) {
            const isFiltered = !this.showSubnets || !this.showNodes;
            const springLength = isFiltered ? 320 : 220;

            this.network.setOptions({
                physics: {
                    barnesHut: {
                        springLength: springLength
                    }
                }
            });

            // Fit the network to show visible nodes
            this.network.fit({
                animation: {
                    duration: 500,
                    easingFunction: 'easeInOutQuad'
                }
            });
        }
    }

    // Reset graph filters to show everything
    resetGraphFilters() {
        const showSubnetsCheckbox = document.getElementById('editor-filter-show-subnets');
        const showNodesCheckbox = document.getElementById('editor-filter-show-nodes');

        if (showSubnetsCheckbox) showSubnetsCheckbox.checked = true;
        if (showNodesCheckbox) showNodesCheckbox.checked = true;

        this.applyGraphFilters();
    }

    // Fit graph to screen
    fitGraphToScreen() {
        if (this.network) {
            this.network.fit({
                animation: {
                    duration: 500,
                    easingFunction: 'easeInOutQuad'
                }
            });
        }
    }

    // Update the live topology indicator in the UI
    updateLiveTopologyIndicator() {
        const indicator = document.getElementById('editor-live-indicator');
        if (indicator) {
            if (this.isLiveTopology) {
                indicator.innerHTML = `
                    <span class="badge bg-success me-2">
                        <i class="bi bi-broadcast"></i> Live: ${this.serverTopologyName}
                    </span>
                    <small class="text-muted">Changes require restart to take effect</small>
                `;
                indicator.style.display = 'block';
            } else {
                indicator.innerHTML = `
                    <span class="badge bg-secondary me-2">
                        <i class="bi bi-pencil"></i> Draft Mode
                    </span>
                `;
                indicator.style.display = 'block';
            }
        }
    }

    // Reload topology from server (sync with running simulation)
    async syncWithServer() {
        const loaded = await this.loadFromServer();
        if (loaded) {
            this.isDirty = false;
            this.updateLiveTopologyIndicator();
            this.updateStatus('Synced with running topology');
        } else {
            this.updateStatus('No running topology to sync with', true);
        }
    }

    // Apply changes to running topology (requires restart)
    async applyChanges() {
        // Validate topology first
        const validation = this.validate();
        if (!validation.isValid) {
            // validate() already shows an alert with errors
            return;
        }

        // Confirm with user
        const confirmed = confirm(
            'This will:\n' +
            '1. Stop the current topology\n' +
            '2. Save your changes\n' +
            '3. Restart with the new configuration\n\n' +
            'All running nodes will be restarted. Continue?'
        );

        if (!confirmed) return;

        try {
            // Disable the apply button and show loading state
            const applyBtn = document.getElementById('editor-apply-btn');
            if (applyBtn) {
                applyBtn.disabled = true;
                applyBtn.innerHTML = '<span class="spinner-border spinner-border-sm me-1"></span> Applying...';
            }

            this.updateStatus('Applying changes and restarting topology...');

            // Send topology to server
            const response = await fetch('/api/topology/update', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(this.topology)
            });

            const result = await response.json();

            if (result.success) {
                this.updateStatus('Topology update initiated, restarting...');
                this.isDirty = false;
                this.isRestarting = true;

                // The persistent Socket.IO listener will handle button re-enable
            } else {
                this.updateStatus('Error applying changes: ' + result.error, true);
                alert('Error applying changes:\n' + result.error);

                // Re-enable button
                if (applyBtn) {
                    applyBtn.disabled = false;
                    applyBtn.innerHTML = '<i class="bi bi-lightning-charge"></i> Apply & Restart';
                }
            }

        } catch (error) {
            console.error('Error applying changes:', error);
            this.updateStatus('Error applying changes: ' + error.message, true);
            alert('Error applying changes:\n' + error.message);

            // Re-enable button
            const applyBtn = document.getElementById('editor-apply-btn');
            if (applyBtn) {
                applyBtn.disabled = false;
                applyBtn.innerHTML = '<i class="bi bi-lightning-charge"></i> Apply & Restart';
            }
        }
    }

    // Setup Socket.IO listeners for real-time sync
    setupSocketIOListeners() {
        if (typeof io === 'undefined') {
            console.warn('Socket.IO not available');
            return;
        }

        if (!window.socket) {
            console.warn('window.socket not available yet, will retry when connected');
            return;
        }

        // Prevent duplicate listener registration
        if (this.socketListenersSetup) {
            console.log('Socket.IO listeners already set up for editor');
            return;
        }

        console.log('Setting up Socket.IO listeners for editor...');
        this.socketListenersSetup = true;

        // Listen for topology updates from server
        if (window.socket) {
            window.socket.on('topology_data', (data) => {
                // Only auto-sync if we're in live mode and the editor tab is visible
                const editorTab = document.getElementById('editor-tab');
                if (editorTab && editorTab.classList.contains('active') && this.isLiveTopology) {
                    console.log('Topology updated on server, syncing editor...');
                    this.loadFromServer();
                }
            });

            // Listen for topology restart events
            window.socket.on('topology_restart', (data) => {
                console.log('Topology restart event:', data);

                // Update status based on restart phase
                if (data.status === 'stopping') {
                    this.updateStatus('⏸️ ' + data.message);
                } else if (data.status === 'reloading') {
                    this.updateStatus('🔄 ' + data.message);
                } else if (data.status === 'starting') {
                    this.updateStatus('▶️ ' + data.message);
                } else if (data.status === 'complete') {
                    this.updateStatus('✅ ' + data.message);

                    // Re-enable apply button
                    if (this.isRestarting) {
                        const applyBtn = document.getElementById('editor-apply-btn');
                        if (applyBtn) {
                            applyBtn.disabled = false;
                            applyBtn.innerHTML = '<i class="bi bi-lightning-charge"></i> Apply & Restart';
                        }
                        this.isRestarting = false;

                        // Reload from server to get fresh data
                        setTimeout(() => this.syncWithServer(), 1000);
                    }
                } else if (data.status === 'error') {
                    this.updateStatus('❌ ' + data.message, true);

                    // Re-enable apply button on error
                    if (this.isRestarting) {
                        const applyBtn = document.getElementById('editor-apply-btn');
                        if (applyBtn) {
                            applyBtn.disabled = false;
                            applyBtn.innerHTML = '<i class="bi bi-lightning-charge"></i> Apply & Restart';
                        }
                        this.isRestarting = false;
                    }
                }
            });
        }
    }

    // Initialize Vis.js canvas
    initCanvas() {
        const container = document.getElementById('editor-canvas');
        if (!container) {
            console.error('Editor canvas container not found');
            return;
        }

        this.networkNodes = new vis.DataSet([]);
        this.networkEdges = new vis.DataSet([]);

        const data = {
            nodes: this.networkNodes,
            edges: this.networkEdges
        };

        const options = {
            manipulation: {
                enabled: false  // We handle add/delete ourselves
            },
            interaction: {
                hover: true,
                selectConnectedEdges: false,
                multiselect: false
            },
            physics: {
                enabled: true,
                barnesHut: {
                    gravitationalConstant: -8000,
                    centralGravity: 0.3,
                    springLength: 150,
                    springConstant: 0.04
                },
                stabilization: {
                    iterations: 100
                }
            },
            nodes: {
                borderWidth: 2,
                borderWidthSelected: 4,
                font: { size: 12 }
            },
            edges: {
                width: 2,
                smooth: { type: 'continuous' }
            }
        };

        this.network = new vis.Network(container, data, options);

        // Disable physics after stabilization to prevent constant re-rendering
        this.network.on('stabilizationIterationsDone', () => {
            this.network.setOptions({ physics: false });
        });

        // Note: Node selection is handled by the click event in edit mode
        // (see disableOverlays() method which sets up editModeClickHandler)
        // We don't need selectNode/deselectNode events here as they cause
        // duplicate event handling and the "double-click to show properties" bug

        // Handle double-click to edit
        this.network.on('doubleClick', (params) => {
            if (params.nodes.length > 0) {
                const nodeId = params.nodes[0];
                if (!nodeId.startsWith('subnet_')) {
                    this.editNodeDialog(nodeId);
                }
            }
        });

        // Handle right-click context menu
        this.network.on('oncontext', (params) => {
            params.event.preventDefault();
            const nodeId = this.network.getNodeAt(params.pointer.DOM);
            this.showContextMenu(params.pointer.DOM, params.pointer.canvas, nodeId);
        });
    }

    // Setup keyboard and other event handlers
    setupEventHandlers() {
        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => {
            // Only handle if editor tab is active
            const editorTab = document.getElementById('editor');
            if (!editorTab || !editorTab.classList.contains('active')) return;

            if (e.ctrlKey || e.metaKey) {
                if (e.key === 'z') {
                    e.preventDefault();
                    this.undo();
                } else if (e.key === 'y') {
                    e.preventDefault();
                    this.redo();
                } else if (e.key === 's') {
                    e.preventDefault();
                    this.saveDraft();
                }
            } else if (e.key === 'Delete') {
                if (this.selectedNodeId) {
                    this.deleteNode(this.selectedNodeId);
                }
            }
        });
    }

    // ==========================================================================
    // Node Management
    // ==========================================================================

    // Add a new node
    addNode(name = null) {
        const nodeCount = this.topology.nodes.length;
        const nodeName = name || `Node${nodeCount + 1}`;

        // Check for duplicate name
        if (this.topology.nodes.find(n => n.name === nodeName)) {
            alert(`Node "${nodeName}" already exists`);
            return null;
        }

        const node = {
            name: nodeName,
            description: '',
            interfaces: [],
            routing_table: []
        };

        this.pushUndo();
        this.topology.nodes.push(node);
        this.markDirty();
        this.renderCanvas();
        this.updateNodesList();
        this.selectNode(nodeName);

        return node;
    }

    // Delete a node
    deleteNode(nodeName) {
        const index = this.topology.nodes.findIndex(n => n.name === nodeName);
        if (index < 0) return;

        if (!confirm(`Delete node "${nodeName}"?`)) return;

        this.pushUndo();
        this.topology.nodes.splice(index, 1);
        this.deselectAll();
        this.markDirty();
        this.renderCanvas();
        this.updateNodesList();
    }

    // Select a node
    selectNode(nodeName) {
        this.selectedNodeId = nodeName;
        this.selectedSubnetId = null;
        this.updatePropertiesPanel();

        // Highlight in vis.js
        const visNodeId = nodeName;
        if (this.network && this.networkNodes.get(visNodeId)) {
            this.network.selectNodes([visNodeId]);
        }
    }

    // Get node by name
    getNode(nodeName) {
        return this.topology.nodes.find(n => n.name === nodeName);
    }

    // Update node properties
    updateNode(nodeName, updates) {
        const node = this.getNode(nodeName);
        if (!node) return;

        this.pushUndo();

        // Handle name change
        if (updates.name && updates.name !== nodeName) {
            if (this.topology.nodes.find(n => n.name === updates.name)) {
                alert(`Node "${updates.name}" already exists`);
                return;
            }
            node.name = updates.name;
            this.selectedNodeId = updates.name;
        }

        if (updates.description !== undefined) node.description = updates.description;

        this.markDirty();
        // Only update labels, no physics needed
        this.renderCanvas({ enablePhysics: false, structureChanged: false });
        this.updateNodesList();
        this.updatePropertiesPanel();
    }

    // Edit node dialog (placeholder - will use properties panel)
    editNodeDialog(nodeName) {
        this.selectNode(nodeName);
        // Focus on the properties panel name input
        const nameInput = document.getElementById('prop-node-name');
        if (nameInput) nameInput.focus();
    }

    // Show context menu
    showContextMenu(domPosition, canvasPosition, nodeId) {
        // Only show context menu in edit mode
        if (this.isReadOnlyMode) {
            return;
        }

        // Remove any existing context menu
        this.hideContextMenu();

        // Build menu items based on what was clicked
        const menuItems = [];

        if (nodeId) {
            if (nodeId.startsWith('subnet_')) {
                const subnetName = nodeId.replace('subnet_', '');
                this.selectSubnet(subnetName);
                menuItems.push({
                    label: `<i class="bi bi-plus-circle"></i> Add Node to "${subnetName}"`,
                    action: () => this.addNodeToSubnet(subnetName)
                });
                menuItems.push({ divider: true });
                menuItems.push({
                    label: `<i class="bi bi-trash text-danger"></i> Delete Subnet`,
                    action: () => this.deleteSubnet(subnetName),
                    className: 'text-danger'
                });
            } else {
                this.selectNode(nodeId);
                menuItems.push({
                    label: `<i class="bi bi-trash text-danger"></i> Delete Node`,
                    action: () => this.deleteNode(nodeId),
                    className: 'text-danger'
                });
            }
        } else {
            // Clicked on empty canvas
            menuItems.push({
                label: `<i class="bi bi-plus-circle"></i> Add Isolated Node`,
                action: () => this.addNode()
            });
            menuItems.push({
                label: `<i class="bi bi-diagram-3"></i> Add Subnet`,
                action: () => this.addSubnet()
            });
        }

        // Create and show the menu
        const menu = document.createElement('div');
        menu.id = 'editor-context-menu';
        menu.className = 'context-menu';
        menu.style.left = `${domPosition.x}px`;
        menu.style.top = `${domPosition.y}px`;

        menuItems.forEach(item => {
            if (item.divider) {
                const divider = document.createElement('div');
                divider.className = 'context-menu-divider';
                menu.appendChild(divider);
            } else {
                const menuItem = document.createElement('div');
                menuItem.className = 'context-menu-item' + (item.className ? ` ${item.className}` : '');
                menuItem.innerHTML = item.label;
                menuItem.onclick = (e) => {
                    e.stopPropagation();
                    this.hideContextMenu();
                    item.action();
                };
                menu.appendChild(menuItem);
            }
        });

        // Add to canvas container
        const canvas = document.getElementById('editor-canvas');
        if (canvas) {
            canvas.appendChild(menu);

            // Close menu when clicking elsewhere
            const closeHandler = (e) => {
                if (!menu.contains(e.target)) {
                    this.hideContextMenu();
                    document.removeEventListener('click', closeHandler);
                }
            };
            setTimeout(() => document.addEventListener('click', closeHandler), 0);
        }
    }

    // Hide context menu
    hideContextMenu() {
        const menu = document.getElementById('editor-context-menu');
        if (menu) menu.remove();
    }

    // Add a node connected to a specific subnet
    addNodeToSubnet(subnetName) {
        const node = this.addNode();
        if (node) {
            this.addInterface(node.name, subnetName);
            this.selectNode(node.name);
        }
    }



    // ==========================================================================
    // Interface Management
    // ==========================================================================

    // Add interface to a node
    addInterface(nodeName, subnetName = null) {
        const node = this.getNode(nodeName);
        if (!node) return null;

        const ifaceCount = node.interfaces.length;
        const ifaceName = `ZMQ${ifaceCount}`;

        // Auto-assign address if subnet specified
        let address = 0;
        let netmask = 8;

        if (subnetName) {
            const subnet = this.getSubnet(subnetName);
            if (subnet) {
                address = this.getNextAvailableAddress(subnetName);
                netmask = subnet.netmask || 8;
            }
        }

        const iface = {
            name: ifaceName,
            driver: 'zmq',
            zmq_proxy: subnetName || '',
            address: address,
            netmask: netmask,
            is_default: node.interfaces.length === 0  // First interface is default
        };

        this.pushUndo();
        node.interfaces.push(iface);
        this.markDirty();
        this.renderCanvas();
        this.updatePropertiesPanel();

        return iface;
    }

    // Remove interface from a node
    removeInterface(nodeName, ifaceIndex) {
        const node = this.getNode(nodeName);
        if (!node || ifaceIndex < 0 || ifaceIndex >= node.interfaces.length) return;

        this.pushUndo();

        const removedIface = node.interfaces.splice(ifaceIndex, 1)[0];

        // If removed interface was default, make first one default
        if (removedIface.is_default && node.interfaces.length > 0) {
            node.interfaces[0].is_default = true;
        }

        // Remove routing entries that reference this interface
        node.routing_table = node.routing_table.filter(
            r => r.interface !== removedIface.name
        );

        this.markDirty();
        this.renderCanvas();
        this.updatePropertiesPanel();
    }

    // Check if address (host portion) is valid for a subnet
    // Note: addresses in JSON are host offsets (0, 1, 2...), not full 14-bit addresses
    // netmask represents NETWORK bits (standard CIDR notation)
    // Host bits = 14 - netmask
    isAddressInSubnetRange(address, subnetName) {
        const subnet = this.getSubnet(subnetName);
        if (!subnet) return false;

        const netmask = subnet.netmask || 8;  // Number of network bits
        const hostBits = 14 - netmask;  // Number of host bits
        const maxHostAddress = (1 << hostBits) - 1;

        // Address must be within the host portion range [0, maxHostAddress]
        return address >= 0 && address <= maxHostAddress;
    }

    // Update interface properties
    updateInterface(nodeName, ifaceIndex, updates) {
        const node = this.getNode(nodeName);
        if (!node || ifaceIndex < 0 || ifaceIndex >= node.interfaces.length) return;

        this.pushUndo();
        const iface = node.interfaces[ifaceIndex];

        // Validate unique interface name within the node
        if (updates.name !== undefined && updates.name !== iface.name) {
            const duplicate = node.interfaces.find((i, idx) =>
                idx !== ifaceIndex && i.name === updates.name
            );
            if (duplicate) {
                alert(`Interface name "${updates.name}" already exists on node "${nodeName}"`);
                return;
            }
            iface.name = updates.name;
        }

        if (updates.address !== undefined) {
            const newAddress = parseInt(updates.address) || 0;

            // Validate address is within subnet range
            if (iface.zmq_proxy) {
                if (!this.isAddressInSubnetRange(newAddress, iface.zmq_proxy)) {
                    const subnet = this.getSubnet(iface.zmq_proxy);
                    const range = this.formatAddressRange(subnet.subnet_prefix || 0, subnet.netmask || 8);
                    alert(`Address ${newAddress} is outside the valid range for subnet '${iface.zmq_proxy}'.\nValid range: ${range}`);
                    return;
                }
            }

            iface.address = newAddress;
        }

        // When subnet changes, auto-update netmask and auto-assign address
        if (updates.zmq_proxy !== undefined) {
            iface.zmq_proxy = updates.zmq_proxy;
            if (updates.zmq_proxy) {
                const subnet = this.getSubnet(updates.zmq_proxy);
                if (subnet) {
                    iface.netmask = subnet.netmask || 8;
                    // Auto-assign next available address in the new subnet
                    iface.address = this.getNextAvailableAddress(updates.zmq_proxy);
                }
            }
        }

        if (updates.is_default !== undefined) {
            if (updates.is_default) {
                // Check if another interface is already default
                const otherDefaults = node.interfaces.filter((i, idx) => idx !== ifaceIndex && i.is_default);
                if (otherDefaults.length > 0) {
                    // Warn user about multiple defaults
                    const otherNames = otherDefaults.map(i => i.name).join(', ');
                    const confirmed = confirm(
                        `Warning: Interface(s) "${otherNames}" already marked as default on node "${nodeName}".\n\n` +
                        `Having multiple default interfaces will cause packets to be sent to ALL default interfaces ` +
                        `(broadcast/flooding behavior) when no specific route matches.\n\n` +
                        `Do you want to mark "${iface.name}" as an additional default interface?`
                    );
                    if (!confirmed) {
                        this.updatePropertiesPanel(); // Reset checkbox state
                        return;
                    }
                }
                iface.is_default = true;
            } else {
                // If unchecking default, ensure at least one interface remains default
                iface.is_default = false;
                if (node.interfaces.length > 0 && !node.interfaces.some(i => i.is_default)) {
                    node.interfaces[0].is_default = true;
                }
            }
        }

        this.markDirty();
        // Only update interface labels, no physics needed
        this.renderCanvas({ enablePhysics: false, structureChanged: false });
        this.updatePropertiesPanel();
    }

    // ==========================================================================
    // Subnet (ZMQ Proxy) Management
    // ==========================================================================

    // Add a new subnet
    addSubnet(name = null) {
        const subnets = this.topology.topology.zmq_proxies;
        const subnetCount = subnets.length;
        const subnetName = name || `Subnet${subnetCount + 1}`;

        // Check for duplicate name
        if (subnets.find(s => s.name === subnetName)) {
            alert(`Subnet "${subnetName}" already exists`);
            return null;
        }

        // Auto-assign base address and ports
        const baseAddress = 256 + (subnetCount * 256);  // Each subnet gets 256 addresses
        const basePort = 6000 + subnetCount;

        // Auto-assign unique netmask (start from 8 and increment)
        let netmask = 8;
        while (subnets.find(s => s.netmask === netmask)) {
            netmask++;
            if (netmask > 14) {
                alert('Cannot create more subnets: all netmask values (8-14) are already in use');
                return null;
            }
        }

        const subnet = {
            name: subnetName,
            subnet_prefix: baseAddress,
            netmask: netmask,
            subscribe_port: basePort,
            publish_port: basePort + 1000
        };

        this.pushUndo();
        subnets.push(subnet);
        this.markDirty();
        this.renderCanvas();
        this.updateSubnetsList();
        this.selectSubnet(subnetName);

        return subnet;
    }

    // Delete a subnet
    deleteSubnet(subnetName) {
        const subnets = this.topology.topology.zmq_proxies;
        const index = subnets.findIndex(s => s.name === subnetName);
        if (index < 0) return;

        // Check if any interfaces use this subnet
        const usedBy = [];
        this.topology.nodes.forEach(node => {
            node.interfaces.forEach(iface => {
                if (iface.zmq_proxy === subnetName) {
                    usedBy.push({ node: node.name, iface: iface.name });
                }
            });
        });

        if (usedBy.length > 0) {
            // Ask user if they want to unassign nodes
            const nodeList = usedBy.map(u => `${u.node}.${u.iface}`).join(', ');
            const choice = confirm(
                `Subnet "${subnetName}" is used by ${usedBy.length} interface(s):\n${nodeList}\n\n` +
                `Click OK to remove these interfaces and delete the subnet.\n` +
                `Click Cancel to abort.`
            );
            if (!choice) return;

            // Remove interfaces connected to this subnet
            this.pushUndo();
            usedBy.forEach(({ node, iface }) => {
                const nodeObj = this.getNode(node);
                if (nodeObj) {
                    const ifaceIndex = nodeObj.interfaces.findIndex(i => i.name === iface);
                    if (ifaceIndex >= 0) {
                        nodeObj.interfaces.splice(ifaceIndex, 1);
                    }
                }
            });
        } else {
            if (!confirm(`Delete subnet "${subnetName}"?`)) return;
            this.pushUndo();
        }

        subnets.splice(index, 1);
        this.deselectAll();
        this.markDirty();
        this.renderCanvas();
        this.updateSubnetsList();
        this.updateNodesList();
    }

    // Select a subnet
    selectSubnet(subnetName) {
        this.selectedNodeId = null;
        this.selectedSubnetId = subnetName;
        this.updatePropertiesPanel();

        // Highlight in vis.js
        const visNodeId = 'subnet_' + subnetName;
        if (this.network && this.networkNodes.get(visNodeId)) {
            this.network.selectNodes([visNodeId]);
        }
    }

    // Get subnet by name
    getSubnet(subnetName) {
        return this.topology.topology.zmq_proxies.find(s => s.name === subnetName);
    }

    // Update subnet properties
    updateSubnet(subnetName, updates) {
        const subnet = this.getSubnet(subnetName);
        if (!subnet) return;

        this.pushUndo();

        // Handle name change
        if (updates.name && updates.name !== subnetName) {
            if (this.topology.topology.zmq_proxies.find(s => s.name === updates.name)) {
                alert(`Subnet "${updates.name}" already exists`);
                return;
            }
            // Update all interfaces that reference this subnet
            this.topology.nodes.forEach(node => {
                node.interfaces.forEach(iface => {
                    if (iface.zmq_proxy === subnetName) {
                        iface.zmq_proxy = updates.name;
                    }
                });
            });
            subnet.name = updates.name;
            this.selectedSubnetId = updates.name;
        }

        if (updates.subnet_prefix !== undefined) subnet.subnet_prefix = parseInt(updates.subnet_prefix) || 0;

        // Allow duplicate netmasks but show warning in validation
        if (updates.netmask !== undefined) {
            const newNetmask = parseInt(updates.netmask) || 8;
            subnet.netmask = newNetmask;

            // Update netmask for all interfaces connected to this subnet
            this.topology.nodes.forEach(node => {
                node.interfaces.forEach(iface => {
                    if (iface.zmq_proxy === subnetName) {
                        iface.netmask = newNetmask;
                    }
                });
            });
        }

        if (updates.subscribe_port !== undefined) subnet.subscribe_port = parseInt(updates.subscribe_port) || 6000;
        if (updates.publish_port !== undefined) subnet.publish_port = parseInt(updates.publish_port) || 7000;

        this.markDirty();
        // Only update subnet labels, no physics needed
        this.renderCanvas({ enablePhysics: false, structureChanged: false });
        this.updateSubnetsList();
        this.updatePropertiesPanel();
    }

    // Calculate address range from netmask
    // CSP uses 14-bit address space (0-16383)
    getAddressRange(netmask) {
        const hostBits = 14 - netmask;
        const numAddresses = 1 << hostBits;
        return numAddresses;
    }

    // Format address range for display
    // Shows both host address range and full 14-bit address range
    // Note: netmask represents NETWORK bits (standard CIDR notation)
    // Host bits = 14 - netmask
    formatAddressRange(baseAddress, netmask) {
        const hostBits = 14 - netmask;  // Number of host bits
        const numAddresses = 1 << hostBits;  // 2^hostBits available addresses
        const fullStart = (baseAddress << hostBits) & 0x3FFF;
        const fullEnd = fullStart + numAddresses - 1;
        return `Host: 0-${numAddresses - 1}, Full: ${fullStart}-${fullEnd}`;
    }

    // Get next available address in a subnet
    // Get next available host address in a subnet
    // Returns a host offset (0, 1, 2...), not a full 14-bit address
    getNextAvailableAddress(subnetName) {
        const subnet = this.getSubnet(subnetName);
        if (!subnet) return 0;

        const usedAddresses = new Set();
        this.topology.nodes.forEach(node => {
            node.interfaces.forEach(iface => {
                if (iface.zmq_proxy === subnetName) {
                    usedAddresses.add(iface.address);
                }
            });
        });

        const netmask = subnet.netmask || 8;  // Number of host bits
        const maxHostAddress = (1 << netmask) - 1;

        // Find first available host address starting from 1 (0 is often reserved)
        for (let hostAddr = 1; hostAddr <= maxHostAddress; hostAddr++) {
            if (!usedAddresses.has(hostAddr)) {
                return hostAddr;
            }
        }

        return 1;  // Fallback to 1
    }

    // Get color for a subnet
    getSubnetColor(subnetName) {
        const subnets = this.topology.topology.zmq_proxies;
        const index = subnets.findIndex(s => s.name === subnetName);
        if (index >= 0) {
            return this.subnetColors[index % this.subnetColors.length];
        }
        return '#a8dadc';
    }

    // Deselect all
    deselectAll() {
        this.selectedNodeId = null;
        this.selectedSubnetId = null;
        if (this.network) {
            this.network.unselectAll();
        }
        this.updatePropertiesPanel();
    }

    // ==========================================================================
    // Undo/Redo System
    // ==========================================================================

    pushUndo() {
        const snapshot = JSON.stringify(this.topology);
        this.undoStack.push(snapshot);
        if (this.undoStack.length > this.maxUndoLevels) {
            this.undoStack.shift();
        }
        this.redoStack = [];  // Clear redo stack on new action
    }

    undo() {
        if (this.undoStack.length === 0) return;

        const currentState = JSON.stringify(this.topology);
        this.redoStack.push(currentState);

        const previousState = this.undoStack.pop();
        this.topology = JSON.parse(previousState);

        this.deselectAll();
        this.markDirty();
        this.renderCanvas();
        this.updateUI();
    }

    redo() {
        if (this.redoStack.length === 0) return;

        const currentState = JSON.stringify(this.topology);
        this.undoStack.push(currentState);

        const nextState = this.redoStack.pop();
        this.topology = JSON.parse(nextState);

        this.deselectAll();
        this.markDirty();
        this.renderCanvas();
        this.updateUI();
    }

    // ==========================================================================
    // State Management
    // ==========================================================================

    markDirty() {
        this.isDirty = true;
        this.updateStatusIndicator();
        // Auto-run validation to update warnings card
        this.validate();
    }

    markClean() {
        this.isDirty = false;
        this.updateStatusIndicator();
    }

    updateStatusIndicator() {
        const statusEl = document.getElementById('editor-status');
        if (!statusEl) return;

        const nodeCount = this.topology.nodes.length;
        const subnetCount = this.topology.topology.zmq_proxies.length;

        if (nodeCount === 0 && subnetCount === 0) {
            statusEl.innerHTML = '<i class="bi bi-circle text-secondary"></i> No topology';
        } else if (this.isDirty) {
            statusEl.innerHTML = `<i class="bi bi-circle-fill text-warning"></i> ${nodeCount} nodes, ${subnetCount} subnets (unsaved)`;
        } else {
            statusEl.innerHTML = `<i class="bi bi-circle-fill text-success"></i> ${nodeCount} nodes, ${subnetCount} subnets`;
        }
    }

    updateStatus(message, isError = false) {
        const statusEl = document.getElementById('editor-autosave-status');
        if (!statusEl) return;

        if (isError) {
            statusEl.innerHTML = `<span class="text-danger">${message}</span>`;
        } else {
            statusEl.innerHTML = `<span class="text-info">${message}</span>`;
        }

        // Clear after 5 seconds
        setTimeout(() => {
            statusEl.innerHTML = '';
        }, 5000);
    }

    // ==========================================================================
    // UI Update Methods
    // ==========================================================================

    updateUI() {
        this.updateNodesList();
        this.updateSubnetsList();
        this.updatePropertiesPanel();
        this.updateStatusIndicator();
    }

    updateNodesList() {
        const container = document.getElementById('editor-nodes-list');
        if (!container) return;

        if (this.topology.nodes.length === 0) {
            container.innerHTML = '<div class="text-muted small p-2">No nodes</div>';
            return;
        }

        let html = '';
        this.topology.nodes.forEach(node => {
            const isSelected = this.selectedNodeId === node.name;
            const ifaceCount = node.interfaces.length;
            html += `
                <div class="list-group-item py-1 px-2 ${isSelected ? 'active' : ''}" style="display: flex; justify-content: space-between; align-items: center;">
                    <a href="#" style="flex: 1; text-decoration: none; color: inherit;"
                       onclick="topologyEditor.selectNode('${node.name}'); return false;">
                        <span><i class="bi bi-cpu"></i> ${node.name}</span>
                        <span class="badge bg-secondary ms-2">${ifaceCount}</span>
                    </a>
                    <button class="btn btn-sm btn-outline-danger" style="padding: 0.15rem 0.4rem; font-size: 0.75rem;"
                            onclick="topologyEditor.deleteNode('${node.name}'); return false;"
                            title="Delete node">
                        <i class="bi bi-trash"></i>
                    </button>
                </div>
            `;
        });
        container.innerHTML = html;
    }

    updateSubnetsList() {
        const container = document.getElementById('editor-subnets-list');
        if (!container) return;

        const subnets = this.topology.topology.zmq_proxies;
        if (subnets.length === 0) {
            container.innerHTML = '<div class="text-muted small p-2">No subnets</div>';
            return;
        }

        let html = '';
        subnets.forEach(subnet => {
            const isSelected = this.selectedSubnetId === subnet.name;
            const color = this.getSubnetColor(subnet.name);
            // Count nodes connected to this subnet
            let connectedNodes = 0;
            this.topology.nodes.forEach(node => {
                if (node.interfaces.some(i => i.zmq_proxy === subnet.name)) {
                    connectedNodes++;
                }
            });
            html += `
                <div class="list-group-item py-1 px-2 ${isSelected ? 'active' : ''}" style="display: flex; justify-content: space-between; align-items: center;">
                    <a href="#" style="flex: 1; text-decoration: none; color: inherit;"
                       onclick="topologyEditor.selectSubnet('${subnet.name}'); return false;">
                        <span><i class="bi bi-diagram-3" style="color: ${color}"></i> ${subnet.name}</span>
                        <span class="badge bg-secondary ms-2">${connectedNodes}</span>
                    </a>
                    <button class="btn btn-sm btn-outline-danger" style="padding: 0.15rem 0.4rem; font-size: 0.75rem;"
                            onclick="topologyEditor.deleteSubnet('${subnet.name}'); return false;"
                            title="Delete subnet">
                        <i class="bi bi-trash"></i>
                    </button>
                </div>
            `;
        });
        container.innerHTML = html;
    }

    updatePropertiesPanel() {
        const container = document.getElementById('editor-properties-panel');
        if (!container) return;

        if (this.selectedNodeId) {
            this.renderNodeProperties(container);
        } else if (this.selectedSubnetId) {
            this.renderSubnetProperties(container);
        } else {
            // Show topology settings when nothing is selected
            this.renderTopologySettings(container);
        }
    }

    renderTopologySettings(container) {
        const topo = this.topology.topology;
        const deduplicationOptions = ['all', 'first', 'none'];
        const dedupSelect = deduplicationOptions.map(opt =>
            `<option value="${opt}" ${topo.deduplication === opt ? 'selected' : ''}>${opt}</option>`
        ).join('');

        container.innerHTML = `
            <div class="small">
                <h6 class="mb-2"><i class="bi bi-gear"></i> Topology Settings</h6>
                <hr class="my-2">

                <div class="mb-2">
                    <label class="form-label small mb-1">Name</label>
                    <input type="text" class="form-control form-control-sm" id="topo-name"
                           value="${topo.name || ''}"
                           onchange="topologyEditor.updateTopologySetting('name', this.value)">
                </div>

                <div class="mb-2">
                    <label class="form-label small mb-1">Description</label>
                    <textarea class="form-control form-control-sm" id="topo-description" rows="2"
                              onchange="topologyEditor.updateTopologySetting('description', this.value)">${topo.description || ''}</textarea>
                </div>

                <div class="mb-2">
                    <label class="form-label small mb-1">CSP Version</label>
                    <select class="form-select form-select-sm" id="topo-csp-version"
                            onchange="topologyEditor.updateTopologySetting('csp_version', parseInt(this.value))">
                        <option value="1" ${topo.csp_version === 1 ? 'selected' : ''}>CSP v1</option>
                        <option value="2" ${topo.csp_version === 2 ? 'selected' : ''}>CSP v2</option>
                    </select>
                </div>

                <div class="mb-2">
                    <label class="form-label small mb-1">Deduplication</label>
                    <select class="form-select form-select-sm" id="topo-deduplication"
                            onchange="topologyEditor.updateTopologySetting('deduplication', this.value)">
                        ${dedupSelect}
                    </select>
                    <small class="text-muted d-block mt-1">
                        <strong>all</strong>: Deduplicate on all interfaces<br>
                        <strong>first</strong>: Only on first matching route<br>
                        <strong>none</strong>: No deduplication
                    </small>
                </div>

                <hr class="my-2">
                <div class="text-muted small">
                    <i class="bi bi-info-circle"></i> Click a node or subnet to edit its properties
                </div>
            </div>
        `;
    }

    updateTopologySetting(key, value) {
        this.pushUndo();
        this.topology.topology[key] = value;
        this.markDirty();
        // Update the header display if name changed
        if (key === 'name') {
            const nameEl = document.getElementById('topology-name');
            if (nameEl) nameEl.textContent = value;
        }
        if (key === 'description') {
            const descEl = document.getElementById('topology-description');
            if (descEl) descEl.textContent = value;
        }
        if (key === 'csp_version') {
            const versionEl = document.getElementById('topology-version');
            if (versionEl) versionEl.textContent = value;
        }
    }

    renderNodeProperties(container) {
        const node = this.getNode(this.selectedNodeId);
        if (!node) return;

        const subnets = this.topology.topology.zmq_proxies;
        const subnetOptions = subnets.map(s =>
            `<option value="${s.name}">${s.name}</option>`
        ).join('');

        let interfacesHtml = '';
        node.interfaces.forEach((iface, idx) => {
            const color = iface.zmq_proxy ? this.getSubnetColor(iface.zmq_proxy) : '#888';

            // Get subnet info for inherited values display
            let subnetInfo = '';
            if (iface.zmq_proxy) {
                const subnet = this.getSubnet(iface.zmq_proxy);
                if (subnet) {
                    const subnetPrefix = subnet.subnet_prefix || 0;
                    const netmask = subnet.netmask || 8;
                    const nodeAddr = iface.address || 0;

                    // Calculate full 14-bit address:
                    // netmask = number of host bits, network bits = 14 - netmask
                    // Full address = (subnet_prefix << (14 - netmask)) | host_address
                    const networkBits = 14 - netmask;
                    const fullAddress = ((subnetPrefix << networkBits) | nodeAddr) & 0x3FFF; // 14-bit max
                    const fullAddrHex = '0x' + fullAddress.toString(16).toUpperCase().padStart(4, '0');

                    subnetInfo = `
                        <div class="alert alert-secondary py-1 px-2 mb-1 small">
                            <i class="bi bi-diagram-3"></i> <strong>${subnet.name}:</strong>
                            [${subnetPrefix}]/${nodeAddr} = <strong>${fullAddress}</strong> (${fullAddrHex})
                            <span class="text-muted">Netmask: /${netmask}</span>
                        </div>
                    `;
                }
            }

            interfacesHtml += `
                <div class="card mb-2">
                    <div class="card-body p-2">
                        <div class="d-flex justify-content-between align-items-center mb-2">
                            <div class="flex-grow-1">
                                <label class="form-label small mb-0">Name</label>
                                <input type="text" class="form-control form-control-sm" value="${iface.name}" style="color: ${color}; font-weight: bold;"
                                       onchange="topologyEditor.updateInterface('${node.name}', ${idx}, {name: this.value})">
                            </div>
                            <button class="btn btn-sm btn-outline-danger ms-2" onclick="topologyEditor.removeInterface('${node.name}', ${idx})" title="Remove interface">
                                <i class="bi bi-trash"></i>
                            </button>
                        </div>
                        <div class="mb-1">
                            <label class="form-label small mb-0">Subnet</label>
                            <select class="form-select form-select-sm" onchange="topologyEditor.updateInterface('${node.name}', ${idx}, {zmq_proxy: this.value})">
                                <option value="">-- No Subnet --</option>
                                ${subnets.map(s => `<option value="${s.name}" ${iface.zmq_proxy === s.name ? 'selected' : ''}>${s.name}</option>`).join('')}
                            </select>
                        </div>
                        ${subnetInfo}
                        <div class="row g-1 mb-1">
                            <div class="col-6">
                                <label class="form-label small mb-0">Address</label>
                                <input type="number" class="form-control form-control-sm" value="${iface.address}"
                                       onchange="topologyEditor.updateInterface('${node.name}', ${idx}, {address: this.value})">
                            </div>
                            <div class="col-6">
                                <label class="form-label small mb-0">Netmask</label>
                                <input type="number" class="form-control form-control-sm" value="${iface.netmask}" min="1" max="14" readonly disabled
                                       title="Netmask is inherited from subnet">
                            </div>
                        </div>
                        <div class="form-check">
                            <input class="form-check-input" type="checkbox" ${iface.is_default ? 'checked' : ''}
                                   onchange="topologyEditor.updateInterface('${node.name}', ${idx}, {is_default: this.checked})">
                            <label class="form-check-label small">Default interface</label>
                        </div>
                    </div>
                </div>
            `;
        });

        container.innerHTML = `
            <h6 class="mb-3"><i class="bi bi-cpu"></i> Node: ${node.name}</h6>

            <div class="mb-3">
                <label class="form-label small">Name</label>
                <input type="text" class="form-control form-control-sm" id="prop-node-name" value="${node.name}"
                       onchange="topologyEditor.updateNode('${node.name}', {name: this.value})">
            </div>

            <div class="mb-3">
                <label class="form-label small">Description</label>
                <input type="text" class="form-control form-control-sm" value="${node.description || ''}"
                       onchange="topologyEditor.updateNode('${node.name}', {description: this.value})">
            </div>

            <hr>

            <!-- Interfaces Section (Collapsible) -->
            <div class="card mb-2">
                <div class="card-header p-2 bg-light d-flex justify-content-between align-items-center" style="cursor: pointer;" onclick="topologyEditor.toggleSection('interfaces-section')">
                    <h6 class="mb-0"><i class="bi bi-ethernet"></i> Interfaces</h6>
                    <i id="interfaces-section-chevron" class="bi bi-chevron-down"></i>
                </div>
                <div id="interfaces-section" class="card-body p-2">
                    <div class="d-flex justify-content-end mb-2">
                        <div class="dropdown">
                            <button class="btn btn-sm btn-outline-primary dropdown-toggle" type="button" data-bs-toggle="dropdown">
                                <i class="bi bi-plus"></i> Add
                            </button>
                            <ul class="dropdown-menu">
                                <li><a class="dropdown-item" href="#" onclick="topologyEditor.addInterface('${node.name}'); return false;">Unassigned</a></li>
                                ${subnets.map(s => `<li><a class="dropdown-item" href="#" onclick="topologyEditor.addInterface('${node.name}', '${s.name}'); return false;">${s.name}</a></li>`).join('')}
                            </ul>
                        </div>
                    </div>
                    ${interfacesHtml || '<div class="text-muted small">No interfaces</div>'}
                </div>
            </div>

            <!-- Routing Section (Collapsible) -->
            <div class="card mb-2">
                <div class="card-header p-2 bg-light d-flex justify-content-between align-items-center" style="cursor: pointer;" onclick="topologyEditor.toggleSection('routing-section')">
                    <h6 class="mb-0"><i class="bi bi-signpost"></i> Routing Table</h6>
                    <i id="routing-section-chevron" class="bi bi-chevron-down"></i>
                </div>
                <div id="routing-section" class="card-body p-2">
                    ${this.renderRoutingSection(node)}
                </div>
            </div>

            <hr>

            <button class="btn btn-sm btn-outline-danger w-100" onclick="topologyEditor.deleteNode('${node.name}')">
                <i class="bi bi-trash"></i> Delete Node
            </button>
        `;
    }

    /**
     * Render routing table section for node properties
     * @param {Object} node - Node object
     * @returns {string} - HTML string
     */
    renderRoutingSection(node) {
        if (!this.routingAnalyzer || !this.routingSuggestions) {
            return '';
        }

        // Get routing table
        const routingTable = node.routing_table || [];

        // Get suggestions
        const suggestions = this.routingSuggestions.getSuggestionsForNode(node.name);

        // Get validation results
        const validation = this.routingAnalyzer.validateRoutingTable(node.name);

        // Render suggestions
        let suggestionsHtml = '';
        if (suggestions.length > 0) {
            suggestionsHtml = `
                <div class="mb-3">
                    ${suggestions.map(s => this.renderSuggestionCard(node.name, s)).join('')}
                </div>
            `;
        }

        // Render routing table entries
        let routingEntriesHtml = '';
        if (routingTable.length > 0) {
            routingEntriesHtml = routingTable.map((route, idx) => {
                const destStr = route.netmask === 0 ? 'default' : `${route.address}/${route.netmask}`;
                const viaStr = route.via === 0 ? 'direct' : `via ${route.via}`;
                const comment = route.comment ? `<div class="text-muted small">${route.comment}</div>` : '';

                return `
                    <div class="card mb-2 routing-entry"
                         draggable="true"
                         data-route-index="${idx}"
                         data-node-name="${node.name}"
                         ondragstart="topologyEditor.handleRouteDragStart(event)"
                         ondragover="topologyEditor.handleRouteDragOver(event)"
                         ondrop="topologyEditor.handleRouteDrop(event)"
                         ondragend="topologyEditor.handleRouteDragEnd(event)">
                        <div class="card-body p-2">
                            <div class="d-flex justify-content-between align-items-start">
                                <div class="d-flex align-items-center" style="cursor: grab;">
                                    <i class="bi bi-grip-vertical text-muted me-2" title="Drag to reorder"></i>
                                    <span class="badge bg-secondary me-2">${idx + 1}</span>
                                </div>
                                <div class="flex-grow-1">
                                    <div class="d-flex align-items-center">
                                        <code class="small">${destStr} → ${route.interface} ${viaStr}</code>
                                    </div>
                                    ${comment}
                                </div>
                                <div class="btn-group btn-group-sm ms-2">
                                    <button class="btn btn-outline-danger btn-sm" onclick="topologyEditor.deleteRoutingEntry('${node.name}', ${idx}); event.stopPropagation();" title="Delete route">
                                        <i class="bi bi-trash"></i>
                                    </button>
                                </div>
                            </div>
                        </div>
                    </div>
                `;
            }).join('');
        } else {
            routingEntriesHtml = '<div class="text-muted small">No routing entries</div>';
        }

        // Render validation warnings/errors
        let validationHtml = '';
        if (validation.errors.length > 0 || validation.warnings.length > 0) {
            const allIssues = [...validation.errors, ...validation.warnings];
            validationHtml = `
                <div class="alert alert-warning alert-sm p-2 mb-2">
                    <div class="small">
                        ${allIssues.map(issue => `
                            <div class="mb-1">
                                <i class="bi bi-exclamation-triangle"></i> ${issue.message}
                            </div>
                        `).join('')}
                    </div>
                </div>
            `;
        }

        return `
            <div class="d-flex justify-content-end mb-2">
                <div class="dropdown">
                    <button class="btn btn-sm btn-outline-primary dropdown-toggle" type="button" data-bs-toggle="dropdown">
                        <i class="bi bi-gear"></i> Actions
                    </button>
                    <ul class="dropdown-menu">
                        <li><a class="dropdown-item" href="#" onclick="topologyEditor.showAddRouteDialog('${node.name}'); return false;">
                            <i class="bi bi-plus"></i> Add Route
                        </a></li>
                        <li><a class="dropdown-item" href="#" onclick="topologyEditor.autoConfigureNodeRouting('${node.name}'); return false;">
                            <i class="bi bi-magic"></i> Auto-Configure
                        </a></li>
                        <li><hr class="dropdown-divider"></li>
                        <li><a class="dropdown-item text-danger" href="#" onclick="topologyEditor.clearRoutingTable('${node.name}'); return false;">
                            <i class="bi bi-trash"></i> Clear All
                        </a></li>
                    </ul>
                </div>
            </div>

            ${suggestionsHtml}
            ${validationHtml}
            ${routingEntriesHtml}
        `;
    }

    /**
     * Render a suggestion card
     * @param {string} nodeName - Name of the node
     * @param {Object} suggestion - Suggestion object
     * @returns {string} - HTML string
     */
    renderSuggestionCard(nodeName, suggestion) {
        const iconClass = suggestion.icon || 'bi-lightbulb';
        const priorityClass = suggestion.priority === 'high' ? 'border-warning' : 'border-info';

        return `
            <div class="card ${priorityClass} mb-2">
                <div class="card-body p-2">
                    <div class="d-flex align-items-start">
                        <i class="${iconClass} me-2 mt-1"></i>
                        <div class="flex-grow-1">
                            <div class="fw-bold small">${suggestion.title}</div>
                            <div class="text-muted small">${suggestion.description}</div>
                        </div>
                    </div>
                    <div class="d-flex gap-1 mt-2">
                        <button class="btn btn-sm btn-primary" onclick="topologyEditor.applySuggestion('${nodeName}', ${JSON.stringify(suggestion).replace(/"/g, '&quot;')}); return false;">
                            <i class="bi bi-check"></i> Apply
                        </button>
                        <button class="btn btn-sm btn-outline-secondary" onclick="topologyEditor.dismissSuggestion('${nodeName}', '${suggestion.id}'); return false;">
                            Dismiss
                        </button>
                    </div>
                </div>
            </div>
        `;
    }

    /**
     * Show add route dialog
     * @param {string} nodeName - Name of the node
     */
    showAddRouteDialog(nodeName) {
        const node = this.getNode(nodeName);
        if (!node) return;

        // Build interface options
        const interfaceOptions = node.interfaces.map(iface =>
            `<option value="${iface.name}">${iface.name} (${iface.address})</option>`
        ).join('');

        if (node.interfaces.length === 0) {
            alert('Node has no interfaces. Add an interface first.');
            return;
        }

        // Create modal HTML
        const modalHtml = `
            <div class="modal fade" id="addRouteModal" tabindex="-1">
                <div class="modal-dialog">
                    <div class="modal-content">
                        <div class="modal-header">
                            <h5 class="modal-title">Add Routing Entry</h5>
                            <button type="button" class="btn-close" data-bs-dismiss="modal"></button>
                        </div>
                        <div class="modal-body">
                            <div class="mb-3">
                                <label class="form-label">Destination Address</label>
                                <input type="number" class="form-control" id="route-address" value="0" min="0" max="16383">
                                <div class="form-text">Use 0 for default route</div>
                            </div>
                            <div class="mb-3">
                                <label class="form-label">Netmask</label>
                                <input type="number" class="form-control" id="route-netmask" value="0" min="0" max="14">
                                <div class="form-text">Use 0 for default route</div>
                            </div>
                            <div class="mb-3">
                                <label class="form-label">Interface</label>
                                <select class="form-select" id="route-interface">
                                    ${interfaceOptions}
                                </select>
                            </div>
                            <div class="mb-3">
                                <label class="form-label">Via (Gateway)</label>
                                <input type="number" class="form-control" id="route-via" value="0" min="0" max="16383">
                                <div class="form-text">Use 0 for direct route</div>
                            </div>
                            <div class="mb-3">
                                <label class="form-label">Comment (optional)</label>
                                <input type="text" class="form-control" id="route-comment" placeholder="e.g., Default route">
                            </div>
                        </div>
                        <div class="modal-footer">
                            <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">Cancel</button>
                            <button type="button" class="btn btn-primary" onclick="topologyEditor.addRouteFromDialog('${nodeName}')">Add Route</button>
                        </div>
                    </div>
                </div>
            </div>
        `;

        // Remove existing modal if any
        const existingModal = document.getElementById('addRouteModal');
        if (existingModal) {
            existingModal.remove();
        }

        // Add modal to body
        document.body.insertAdjacentHTML('beforeend', modalHtml);

        // Show modal
        const modal = new bootstrap.Modal(document.getElementById('addRouteModal'));
        modal.show();

        // Clean up on hide
        document.getElementById('addRouteModal').addEventListener('hidden.bs.modal', function() {
            this.remove();
        });
    }

    /**
     * Add route from dialog
     * @param {string} nodeName - Name of the node
     */
    addRouteFromDialog(nodeName) {
        const address = parseInt(document.getElementById('route-address').value) || 0;
        const netmask = parseInt(document.getElementById('route-netmask').value) || 0;
        const iface = document.getElementById('route-interface').value;
        const via = parseInt(document.getElementById('route-via').value) || 0;
        const comment = document.getElementById('route-comment').value.trim();

        const entry = {
            address: address,
            netmask: netmask,
            interface: iface,
            via: via
        };

        if (comment) {
            entry.comment = comment;
        }

        this.addRoutingEntry(nodeName, entry);

        // Close modal
        const modal = bootstrap.Modal.getInstance(document.getElementById('addRouteModal'));
        if (modal) {
            modal.hide();
        }
    }

    /**
     * Toggle collapsible section in properties panel
     * @param {string} sectionId - ID of the section to toggle
     */
    toggleSection(sectionId) {
        const section = document.getElementById(sectionId);
        const chevron = document.getElementById(`${sectionId}-chevron`);

        if (!section || !chevron) return;

        if (section.style.display === 'none') {
            section.style.display = 'block';
            chevron.classList.remove('bi-chevron-right');
            chevron.classList.add('bi-chevron-down');
        } else {
            section.style.display = 'none';
            chevron.classList.remove('bi-chevron-down');
            chevron.classList.add('bi-chevron-right');
        }
    }



    renderSubnetProperties(container) {
        const subnet = this.getSubnet(this.selectedSubnetId);
        if (!subnet) return;

        // Count connected nodes
        let connectedNodes = [];
        this.topology.nodes.forEach(node => {
            const ifaces = node.interfaces.filter(i => i.zmq_proxy === subnet.name);
            if (ifaces.length > 0) {
                connectedNodes.push({ node: node.name, interfaces: ifaces });
            }
        });

        let nodesHtml = '';
        if (connectedNodes.length > 0) {
            nodesHtml = connectedNodes.map(cn => `
                <div class="small mb-1">
                    <i class="bi bi-cpu"></i> ${cn.node}
                    (${cn.interfaces.map(i => `${i.name}:${i.address}`).join(', ')})
                </div>
            `).join('');
        } else {
            nodesHtml = '<div class="text-muted small">No connected nodes</div>';
        }

        const color = this.getSubnetColor(subnet.name);

        container.innerHTML = `
            <h6 class="mb-3"><i class="bi bi-diagram-3" style="color: ${color}"></i> Subnet: ${subnet.name}</h6>

            <div class="mb-3">
                <label class="form-label small">Name</label>
                <input type="text" class="form-control form-control-sm" value="${subnet.name}"
                       onchange="topologyEditor.updateSubnet('${subnet.name}', {name: this.value})">
            </div>

            <div class="row g-2 mb-3">
                <div class="col-6">
                    <label class="form-label small" title="Network prefix combined with node address to form fully-qualified CSP address">Subnet Prefix</label>
                    <input type="number" class="form-control form-control-sm" value="${subnet.subnet_prefix || 0}"
                           onchange="topologyEditor.updateSubnet('${subnet.name}', {subnet_prefix: this.value})">
                </div>
                <div class="col-6">
                    <label class="form-label small">Netmask</label>
                    <input type="number" class="form-control form-control-sm" value="${subnet.netmask || 8}" min="1" max="14"
                           onchange="topologyEditor.updateSubnet('${subnet.name}', {netmask: this.value})">
                </div>
            </div>

            <div class="alert alert-info py-1 px-2 mb-3 small">
                <i class="bi bi-info-circle"></i> <strong>Address Range:</strong> ${this.formatAddressRange(subnet.subnet_prefix || 0, subnet.netmask || 8)}
            </div>

            <div class="row g-2 mb-3">
                <div class="col-6">
                    <label class="form-label small">Subscribe Port</label>
                    <input type="number" class="form-control form-control-sm" value="${subnet.subscribe_port || 6000}"
                           onchange="topologyEditor.updateSubnet('${subnet.name}', {subscribe_port: this.value})">
                </div>
                <div class="col-6">
                    <label class="form-label small">Publish Port</label>
                    <input type="number" class="form-control form-control-sm" value="${subnet.publish_port || 7000}"
                           onchange="topologyEditor.updateSubnet('${subnet.name}', {publish_port: this.value})">
                </div>
            </div>

            <hr>

            <h6 class="mb-2">Connected Nodes</h6>
            ${nodesHtml}

            <hr>

            <button class="btn btn-sm btn-outline-danger w-100" onclick="topologyEditor.deleteSubnet('${subnet.name}')">
                <i class="bi bi-trash"></i> Delete Subnet
            </button>
        `;
    }

    // ==========================================================================
    // Canvas Rendering
    // ==========================================================================

    renderCanvas(options = {}) {
        // Options:
        //   enablePhysics: boolean - whether to run physics simulation (default: true)
        //   structureChanged: boolean - whether topology structure changed (default: true)
        const enablePhysics = options.enablePhysics !== undefined ? options.enablePhysics : true;
        const structureChanged = options.structureChanged !== undefined ? options.structureChanged : true;

        if (!this.networkNodes || !this.networkEdges) {
            console.error('Network nodes or edges not initialized');
            return;
        }

        // Save current positions if structure hasn't changed
        const savedPositions = {};
        if (!structureChanged && this.network) {
            const positions = this.network.getPositions();
            Object.keys(positions).forEach(nodeId => {
                savedPositions[nodeId] = positions[nodeId];
            });
        }

        // Clear existing
        this.networkNodes.clear();
        this.networkEdges.clear();

        const visNodes = [];
        const visEdges = [];

        // Add subnet nodes
        this.topology.topology.zmq_proxies.forEach(subnet => {
            const color = this.getSubnetColor(subnet.name);

            // Ensure subnet has subnet_prefix and netmask (set defaults if missing)
            if (subnet.subnet_prefix === undefined) subnet.subnet_prefix = 0;
            if (subnet.netmask === undefined) subnet.netmask = 8;  // Default to netmask 8 (256 addresses)

            // Build label - always show subnet_prefix/netmask
            let label = subnet.name + '\n(' + subnet.subnet_prefix + '/' + subnet.netmask + ')';

            const nodeData = {
                id: 'subnet_' + subnet.name,
                label: label,
                shape: 'diamond',
                color: {
                    background: color,
                    border: color,
                    highlight: { background: color, border: '#333' }
                },
                font: { size: 10 },
                size: 25
            };

            // Restore saved position if available
            const nodeId = 'subnet_' + subnet.name;
            if (savedPositions[nodeId]) {
                nodeData.x = savedPositions[nodeId].x;
                nodeData.y = savedPositions[nodeId].y;
            }

            visNodes.push(nodeData);
        });

        // Add node nodes and edges to subnets
        this.topology.nodes.forEach(node => {
            // Determine node color based on first interface's subnet
            let nodeColor = '#a8dadc';
            if (node.interfaces.length > 0 && node.interfaces[0].zmq_proxy) {
                nodeColor = this.getSubnetColor(node.interfaces[0].zmq_proxy);
            }

            const nodeData = {
                id: node.name,
                label: node.name + '\n(' + node.interfaces.length + ' ifaces)',
                shape: 'box',
                color: {
                    background: '#f8f9fa',
                    border: nodeColor,
                    highlight: { background: '#fff', border: '#333' }
                },
                font: { size: 11 },
                borderWidth: 3
            };

            // Restore saved position if available
            if (savedPositions[node.name]) {
                nodeData.x = savedPositions[node.name].x;
                nodeData.y = savedPositions[node.name].y;
            }

            visNodes.push(nodeData);

            // Add edges for each interface connection
            node.interfaces.forEach(iface => {
                if (iface.zmq_proxy) {
                    const color = this.getSubnetColor(iface.zmq_proxy);
                    visEdges.push({
                        from: node.name,
                        to: 'subnet_' + iface.zmq_proxy,
                        label: iface.name + ':' + iface.address,
                        color: { color: color, highlight: color },
                        font: { size: 8, align: 'middle' },
                        smooth: { type: 'curvedCW', roundness: 0.2 }
                    });
                }
            });
        });

        this.networkNodes.add(visNodes);
        this.networkEdges.add(visEdges);

        // Store original edges for filtering
        this.originalEdges = visEdges.map(e => ({...e}));

        // Handle physics based on options
        if (this.network) {
            if (enablePhysics && structureChanged) {
                // Re-enable physics for layout, then disable after stabilization
                this.network.setOptions({ physics: { enabled: true } });

                // Use stabilized event instead of stabilizationIterationsDone
                this.network.once('stabilized', () => {
                    this.network.setOptions({ physics: false });
                    this.network.redraw();
                    this.network.fit({
                        animation: {
                            duration: 500,
                            easingFunction: 'easeInOutQuad'
                        }
                    });
                });
            } else {
                // Just redraw without physics (for property-only updates)
                this.network.setOptions({ physics: false });
                this.network.redraw();
            }
        } else {
            console.error('Network not initialized!');
        }
    }

    // ==========================================================================
    // LocalStorage Auto-save
    // ==========================================================================

    // Save draft to server (without restarting)
    async saveDraft() {
        // Validate (warn but don't block)
        const validation = this.validate();
        if (!validation.isValid) {
            if (!confirm('Topology has errors. Save anyway?')) return;
        }

        try {
            const response = await fetch('/api/topology/save-draft', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(this.topology)
            });

            const result = await response.json();

            if (result.success) {
                this.markClean();
                this.updateStatus('Draft saved to server');
                console.log('Draft saved successfully');
            } else {
                throw new Error(result.error || 'Failed to save draft');
            }
        } catch (error) {
            console.error('Error saving draft:', error);
            this.updateStatus('Error saving draft: ' + error.message, true);
            alert('Error saving draft:\n' + error.message);
        }
    }

    // Discard changes and reload from server
    async discardChanges() {
        if (!confirm('Discard all unsaved changes and reload from server?')) return;

        try {
            const loaded = await this.loadFromServer();
            if (loaded) {
                this.markClean();
                this.updateStatus('Changes discarded, reloaded from server');
                console.log('Changes discarded');
            } else {
                this.updateStatus('No topology on server to reload', true);
            }
        } catch (error) {
            console.error('Error discarding changes:', error);
            this.updateStatus('Error reloading from server: ' + error.message, true);
        }
    }

    // Refresh from running server (live mode only)
    async refreshFromServer() {
        try {
            const loaded = await this.loadFromServer();
            if (loaded) {
                this.updateStatus('Refreshed from running topology');
                console.log('Refreshed from server');
            } else {
                this.updateStatus('No running topology to refresh from', true);
            }
        } catch (error) {
            console.error('Error refreshing from server:', error);
            this.updateStatus('Error refreshing: ' + error.message, true);
        }
    }

    updateAutoSaveStatus() {
        const el = document.getElementById('editor-autosave-status');
        if (!el) return;

        if (this.lastSaveTime) {
            const time = this.lastSaveTime.toLocaleTimeString();
            el.innerHTML = `<i class="bi bi-cloud-check text-success"></i> Saved ${time}`;
        } else {
            el.innerHTML = '';
        }
    }


    // ==========================================================================
    // Import/Export
    // ==========================================================================

    exportTopology() {
        const data = JSON.stringify(this.topology, null, 2);
        const blob = new Blob([data], { type: 'application/json' });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = (this.topology.topology.name || 'topology').replace(/\s+/g, '_') + '.json';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    importTopology() {
        const input = document.createElement('input');
        input.type = 'file';
        input.accept = '.json';

        input.onchange = (e) => {
            const file = e.target.files[0];
            if (!file) return;

            const reader = new FileReader();
            reader.onload = (event) => {
                try {
                    const data = JSON.parse(event.target.result);

                    // Validate basic structure
                    if (!data.topology || !data.nodes) {
                        alert('Invalid topology file: missing topology or nodes section');
                        return;
                    }

                    // Store the parsed data temporarily
                    this.pendingImportData = data;

                    // Show modal to ask user how to import
                    this.showImportModal();

                } catch (err) {
                    alert('Failed to parse JSON file: ' + err.message);
                }
            };
            reader.readAsText(file);
        };

        input.click();
    }

    showImportModal() {
        const modalElement = document.getElementById('importTopologyModal');

        if (!modalElement) {
            console.error('[Import] Modal element not found!');
            alert('Import modal not found. Please refresh the page.');
            return;
        }

        // Create Bootstrap modal instance
        let modal;
        try {
            if (typeof bootstrap !== 'undefined') {
                modal = new bootstrap.Modal(modalElement);
            } else {
                console.error('[Import] Bootstrap not loaded!');
                alert('Bootstrap not loaded. Please refresh the page.');
                return;
            }
        } catch (error) {
            console.error('[Import] Error creating modal:', error);
            alert('Error creating modal: ' + error.message);
            return;
        }

        // Set up the confirm button handler
        const confirmBtn = document.getElementById('confirmImportBtn');
        const newHandler = async () => {
            const importMode = document.querySelector('input[name="importMode"]:checked').value;

            // Close modal
            modal.hide();

            // Store a copy of the data before we modify this.topology
            const importedData = JSON.parse(JSON.stringify(this.pendingImportData));

            // Load topology into editor
            this.pushUndo();
            this.topology = this.pendingImportData;

            // Update routing analyzer with new topology
            this.updateRoutingAnalyzer();

            this.markDirty();
            this.renderCanvas();
            this.updateUI();

            console.log('Topology imported from file (mode:', importMode, ')');

            // If user chose to apply to simulation, trigger update
            if (importMode === 'apply') {
                await this.applyToSimulation(importedData);
            }

            // Clean up
            this.pendingImportData = null;
        };

        // Remove old event listener and add new one
        confirmBtn.replaceWith(confirmBtn.cloneNode(true));
        document.getElementById('confirmImportBtn').addEventListener('click', newHandler);

        // Show modal
        modal.show();
    }

    async applyToSimulation(topologyData) {
        try {
            // Show loading indicator
            const statusDiv = document.getElementById('topology-status');
            if (statusDiv) {
                statusDiv.innerHTML = '<span class="badge bg-warning">Applying topology...</span>';
            }

            console.log('Applying topology to simulation...');

            // Send topology to server
            const response = await fetch('/api/topology/update', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(topologyData)
            });

            const result = await response.json();

            if (result.success) {
                console.log('Topology applied to simulation successfully');
                this.isLiveTopology = true;
                this.serverTopologyName = this.topology.topology?.name || 'Server Topology';
                this.markClean();
                this.updateLiveTopologyIndicator();

                // Show success message
                if (statusDiv) {
                    statusDiv.innerHTML = '<span class="badge bg-success">Topology applied and restarting...</span>';
                    setTimeout(() => {
                        statusDiv.innerHTML = '<span class="badge bg-success">Running</span>';
                    }, 3000);
                }
            } else {
                throw new Error(result.error || 'Failed to apply topology');
            }
        } catch (error) {
            console.error('Failed to apply topology to simulation:', error);
            alert('Failed to apply topology to simulation: ' + error.message);
        }
    }

    newTopology() {
        if (this.isDirty || this.topology.nodes.length > 0) {
            if (!confirm('Create a new topology? Unsaved changes will be lost.')) {
                return;
            }
        }

        this.undoStack = [];
        this.redoStack = [];
        this.topology = this.createEmptyTopology();

        // Update routing analyzer with new topology
        this.updateRoutingAnalyzer();

        this.deselectAll();
        this.markDirty();
        this.renderCanvas();
        this.updateUI();
    }

    // ==========================================================================
    // Validation
    // ==========================================================================

    updateValidationCard(errors, warnings) {
        const card = document.getElementById('editor-validation-card');
        const errorBadge = document.getElementById('validation-error-badge');
        const warningBadge = document.getElementById('validation-warning-badge');
        const messagesDiv = document.getElementById('validation-messages');

        // If elements don't exist (e.g., wrong tab), skip update
        if (!card || !errorBadge || !warningBadge || !messagesDiv) {
            return;
        }

        // Hide card if no errors or warnings
        if (errors.length === 0 && warnings.length === 0) {
            card.style.display = 'none';
            return;
        }

        // Show card and update badges
        card.style.display = 'block';

        if (errors.length > 0) {
            errorBadge.style.display = 'inline-block';
            errorBadge.textContent = `${errors.length} Error${errors.length > 1 ? 's' : ''}`;
        } else {
            errorBadge.style.display = 'none';
        }

        if (warnings.length > 0) {
            warningBadge.style.display = 'inline-block';
            warningBadge.textContent = `${warnings.length} Warning${warnings.length > 1 ? 's' : ''}`;
        } else {
            warningBadge.style.display = 'none';
        }

        // Build messages HTML - compact clickable items
        let html = '';

        errors.forEach(err => {
            const clickAttr = this.buildValidationClickAttr(err);
            const targetLabel = this.buildValidationTargetLabel(err);
            html += `
                <div class="validation-item validation-error" ${clickAttr}>
                    <i class="bi bi-x-circle-fill text-danger"></i>
                    ${targetLabel}${err.message}
                </div>
            `;
        });

        warnings.forEach(warn => {
            const clickAttr = this.buildValidationClickAttr(warn);
            const targetLabel = this.buildValidationTargetLabel(warn);
            html += `
                <div class="validation-item validation-warning" ${clickAttr}>
                    <i class="bi bi-exclamation-triangle-fill text-warning"></i>
                    ${targetLabel}${warn.message}
                </div>
            `;
        });

        messagesDiv.innerHTML = html;
    }

    /**
     * Build onclick attribute for validation item
     */
    buildValidationClickAttr(item) {
        if (item.nodeName) {
            return `onclick="topologyEditor.focusOnNode('${item.nodeName}')" style="cursor:pointer"`;
        } else if (item.subnetName) {
            return `onclick="topologyEditor.focusOnSubnet('${item.subnetName}')" style="cursor:pointer"`;
        }
        return '';
    }

    /**
     * Build target label for validation item
     */
    buildValidationTargetLabel(item) {
        if (item.nodeName) {
            return `<span class="validation-target">${item.nodeName}</span>`;
        } else if (item.subnetName) {
            return `<span class="validation-target">${item.subnetName}</span>`;
        }
        return '';
    }

    /**
     * Focus on a node: select it, center view, show properties
     */
    focusOnNode(nodeName) {
        this.selectNode(nodeName);
        // Center the network view on this node
        if (this.network) {
            const nodeId = nodeName;
            this.network.focus(nodeId, { scale: 1.2, animation: true });
        }
    }

    /**
     * Focus on a subnet: select it, center view, show properties
     */
    focusOnSubnet(subnetName) {
        this.selectSubnet(subnetName);
        // Center the network view on this subnet
        if (this.network) {
            const nodeId = 'subnet_' + subnetName;
            this.network.focus(nodeId, { scale: 1.2, animation: true });
        }
    }

    validate(showSuccessAlert = false) {
        console.log('validate() called with showSuccessAlert:', showSuccessAlert);
        const errors = [];
        const warnings = [];

        // Check for duplicate node names
        const nodeNames = new Set();
        this.topology.nodes.forEach(node => {
            if (nodeNames.has(node.name)) {
                errors.push({ message: `Duplicate node name: ${node.name}`, nodeName: node.name });
            }
            nodeNames.add(node.name);
        });

        // Check for duplicate interface names within same node
        this.topology.nodes.forEach(node => {
            const ifaceNames = new Set();
            node.interfaces.forEach(iface => {
                if (ifaceNames.has(iface.name)) {
                    errors.push({ message: `Duplicate interface '${iface.name}'`, nodeName: node.name });
                }
                ifaceNames.add(iface.name);
            });
        });

        // Check for duplicate addresses within same subnet
        const subnetAddresses = {};
        this.topology.nodes.forEach(node => {
            node.interfaces.forEach(iface => {
                if (iface.zmq_proxy) {
                    const key = `${iface.zmq_proxy}:${iface.address}`;
                    if (subnetAddresses[key]) {
                        errors.push({ message: `Duplicate address ${iface.address} in ${iface.zmq_proxy}`, nodeName: node.name, subnetName: iface.zmq_proxy });
                    } else {
                        subnetAddresses[key] = `${node.name}.${iface.name}`;
                    }
                }
            });
        });

        // Check for duplicate subnet netmasks (WARNING, not ERROR)
        const netmasks = new Map();
        this.topology.topology.zmq_proxies.forEach(subnet => {
            if (subnet.netmask !== undefined) {
                if (netmasks.has(subnet.netmask)) {
                    const existingSubnets = netmasks.get(subnet.netmask);
                    netmasks.set(subnet.netmask, existingSubnets + ', ' + subnet.name);
                } else {
                    netmasks.set(subnet.netmask, subnet.name);
                }
            }
        });
        netmasks.forEach((subnets, netmask) => {
            if (subnets.includes(',')) {
                warnings.push({ message: `Duplicate netmask /${netmask}: ${subnets}` });
            }
        });

        // Check for addresses outside subnet range
        this.topology.nodes.forEach(node => {
            node.interfaces.forEach(iface => {
                if (iface.zmq_proxy) {
                    if (!this.isAddressInSubnetRange(iface.address, iface.zmq_proxy)) {
                        const subnet = this.getSubnet(iface.zmq_proxy);
                        const range = this.formatAddressRange(subnet.subnet_prefix || 0, subnet.netmask || 8);
                        errors.push({ message: `${iface.name} addr ${iface.address} outside ${iface.zmq_proxy} (${range})`, nodeName: node.name });
                    }
                }
            });
        });

        // Check for nodes without interfaces
        this.topology.nodes.forEach(node => {
            if (node.interfaces.length === 0) {
                warnings.push({ message: `No interfaces`, nodeName: node.name });
            }
        });

        // Check for interfaces not assigned to subnets
        this.topology.nodes.forEach(node => {
            node.interfaces.forEach(iface => {
                if (!iface.zmq_proxy) {
                    warnings.push({ message: `${iface.name} not assigned to subnet`, nodeName: node.name });
                }
            });
        });

        // Check for empty subnets
        this.topology.topology.zmq_proxies.forEach(subnet => {
            let hasNodes = false;
            this.topology.nodes.forEach(node => {
                if (node.interfaces.some(i => i.zmq_proxy === subnet.name)) {
                    hasNodes = true;
                }
            });
            if (!hasNodes) {
                warnings.push({ message: `No connected nodes`, subnetName: subnet.name });
            }
        });

        // Check for netmask = 0 (ERROR - invalid configuration)
        this.topology.nodes.forEach(node => {
            node.interfaces.forEach(iface => {
                if (iface.netmask === 0) {
                    errors.push({ message: `${iface.name} has netmask=0 (invalid)`, nodeName: node.name });
                }
            });
        });

        // Check for Split Horizon configuration (WARNING)
        this.topology.nodes.forEach(node => {
            const ifaceConfigs = new Map();
            node.interfaces.forEach(iface => {
                if (iface.zmq_proxy && iface.address !== undefined && iface.netmask !== undefined) {
                    const key = `${iface.address}/${iface.netmask}`;
                    if (ifaceConfigs.has(key)) {
                        ifaceConfigs.get(key).push(iface.name);
                    } else {
                        ifaceConfigs.set(key, [iface.name]);
                    }
                }
            });
            ifaceConfigs.forEach((ifaceNames, config) => {
                if (ifaceNames.length > 1) {
                    warnings.push({ message: `Split Horizon (${config}): ${ifaceNames.join(', ')}`, nodeName: node.name });
                }
            });
        });

        // Check for no default interface (WARNING)
        let hasDefaultInterface = false;
        this.topology.nodes.forEach(node => {
            if (node.interfaces.some(i => i.is_default)) {
                hasDefaultInterface = true;
            }
        });
        if (!hasDefaultInterface && this.topology.nodes.length > 0) {
            warnings.push({ message: `No default interface configured` });
        }

        // Check for multiple default interfaces on a node (WARNING)
        this.topology.nodes.forEach(node => {
            const defaultIfaces = node.interfaces.filter(i => i.is_default);
            if (defaultIfaces.length > 1) {
                const names = defaultIfaces.map(i => i.name).join(', ');
                warnings.push({
                    message: `Multiple default interfaces (${names}) - packets will flood to all`,
                    nodeName: node.name
                });
            }
        });

        // Update the validation card UI
        this.updateValidationCard(errors, warnings);

        // Show success message if manually triggered and no issues
        if (showSuccessAlert && errors.length === 0 && warnings.length === 0) {
            console.log('Showing success alert');
            alert('✓ Topology is valid!');
        }

        // Return string arrays for backward compatibility
        console.log('validate() returning:', { errors: errors.length, warnings: warnings.length, isValid: errors.length === 0 });
        return {
            errors: errors.map(e => e.message),
            warnings: warnings.map(w => w.message),
            isValid: errors.length === 0
        };
    }

    // ==========================================================================
    // Routing Table Management
    // ==========================================================================

    /**
     * Add a routing entry to a node
     * @param {string} nodeName - Name of the node
     * @param {Object} entry - Routing entry object
     */
    addRoutingEntry(nodeName, entry) {
        const node = this.getNode(nodeName);
        if (!node) return;

        this.pushUndo();

        if (!node.routing_table) {
            node.routing_table = [];
        }

        node.routing_table.push(entry);
        this.markDirty();
        this.updatePropertiesPanel();
    }

    /**
     * Update a routing entry
     * @param {string} nodeName - Name of the node
     * @param {number} index - Index of the routing entry
     * @param {Object} updates - Updates to apply
     */
    updateRoutingEntry(nodeName, index, updates) {
        const node = this.getNode(nodeName);
        if (!node || !node.routing_table || index < 0 || index >= node.routing_table.length) return;

        this.pushUndo();

        Object.assign(node.routing_table[index], updates);
        this.markDirty();
        this.updatePropertiesPanel();
    }

    /**
     * Delete a routing entry
     * @param {string} nodeName - Name of the node
     * @param {number} index - Index of the routing entry
     */
    deleteRoutingEntry(nodeName, index) {
        const node = this.getNode(nodeName);
        if (!node || !node.routing_table || index < 0 || index >= node.routing_table.length) return;

        this.pushUndo();

        node.routing_table.splice(index, 1);
        this.markDirty();
        this.updatePropertiesPanel();
    }

    /**
     * Clear all routing entries for a node
     * @param {string} nodeName - Name of the node
     */
    clearRoutingTable(nodeName) {
        const node = this.getNode(nodeName);
        if (!node) return;

        if (!confirm(`Clear all routing entries for node "${nodeName}"?`)) {
            return;
        }

        this.pushUndo();

        node.routing_table = [];
        this.markDirty();
        this.updatePropertiesPanel();
    }

    /**
     * Auto-configure routing for a node
     * @param {string} nodeName - Name of the node
     * @param {Object} options - Configuration options
     */
    autoConfigureNodeRouting(nodeName, options = {}) {
        if (!this.routingAnalyzer) return;

        const routes = this.routingAnalyzer.generateRoutingTable(nodeName, options);
        if (routes.length === 0) {
            alert('No routes could be generated for this node.');
            return;
        }

        const node = this.getNode(nodeName);
        if (!node) return;

        this.pushUndo();

        node.routing_table = routes;
        this.markDirty();
        this.updatePropertiesPanel();

        // Clear dismissed suggestions for this node
        if (this.routingSuggestions) {
            this.routingSuggestions.clearDismissed(nodeName);
        }
    }

    /**
     * Auto-configure routing for all nodes in the topology
     * Implements the algorithm from smart-routing-table-automation.md
     */
    autoConfigureAllRouting() {
        if (!this.routingAnalyzer) return;

        // Phase 1: Analyze topology
        const analysis = this.analyzeTopologyForRouting();

        // Show confirmation with analysis results
        const confirmMsg = `Auto-Configure Routing\n\n` +
            `Analysis Results:\n` +
            `  • ${analysis.routers.length} router node(s)\n` +
            `  • ${analysis.endNodes.length} end node(s)\n` +
            `  • ${analysis.bridges.length} bridge node(s)\n` +
            `  • ${analysis.isolated.length} isolated node(s)\n` +
            `  • ${analysis.subnets.length} subnet(s)\n\n` +
            `This will replace existing routing tables.\n\nContinue?`;

        if (!confirm(confirmMsg)) {
            return;
        }

        this.pushUndo();

        // Phase 2: Fix invalid interface addresses
        const addressFixes = [];
        this.topology.nodes.forEach(node => {
            node.interfaces.forEach(iface => {
                if (iface.zmq_proxy) {
                    if (!this.isAddressInSubnetRange(iface.address, iface.zmq_proxy)) {
                        const oldAddress = iface.address;
                        const newAddress = this.getNextAvailableAddress(iface.zmq_proxy);
                        iface.address = newAddress;
                        // Also sync netmask from subnet
                        const subnet = this.getSubnet(iface.zmq_proxy);
                        if (subnet) {
                            iface.netmask = subnet.netmask || 8;
                        }
                        addressFixes.push({
                            node: node.name,
                            interface: iface.name,
                            subnet: iface.zmq_proxy,
                            oldAddress: oldAddress,
                            newAddress: newAddress
                        });
                    }
                }
            });
        });

        // Phase 3: Generate routing tables
        const results = {
            configured: [],
            skipped: [],
            errors: [],
            addressFixes: addressFixes
        };

        this.topology.nodes.forEach(node => {
            try {
                const routes = this.routingAnalyzer.generateRoutingTable(node.name);
                if (routes.length > 0) {
                    node.routing_table = routes;
                    results.configured.push({
                        name: node.name,
                        role: this.routingAnalyzer.analyzeNodeRole(node.name),
                        routeCount: routes.length
                    });
                } else {
                    results.skipped.push({
                        name: node.name,
                        reason: 'No routes generated (isolated or no interfaces)'
                    });
                }
            } catch (err) {
                results.errors.push({
                    name: node.name,
                    error: err.message
                });
            }
        });

        // Phase 3: Validate results
        const allWarnings = [];
        const allErrors = [];

        this.topology.nodes.forEach(node => {
            const validation = this.routingAnalyzer.validateRoutingTable(node.name);
            validation.warnings.forEach(w => {
                allWarnings.push({ node: node.name, ...w });
            });
            validation.errors.forEach(e => {
                allErrors.push({ node: node.name, ...e });
            });
        });

        // Phase 4: Attempt to fix errors deterministically
        const fixedErrors = [];
        allErrors.forEach(error => {
            if (error.type === 'invalid_interface') {
                // Remove routes that reference non-existent interfaces
                const node = this.getNode(error.node);
                if (node && node.routing_table) {
                    const before = node.routing_table.length;
                    node.routing_table = node.routing_table.filter(r =>
                        node.interfaces.some(i => i.name === r.interface)
                    );
                    if (node.routing_table.length < before) {
                        fixedErrors.push({
                            node: error.node,
                            type: error.type,
                            action: 'Removed invalid route'
                        });
                    }
                }
            }
        });

        // Re-validate after fixes
        const remainingWarnings = [];
        const remainingErrors = [];

        this.topology.nodes.forEach(node => {
            const validation = this.routingAnalyzer.validateRoutingTable(node.name);
            validation.warnings.forEach(w => {
                remainingWarnings.push({ node: node.name, ...w });
            });
            validation.errors.forEach(e => {
                remainingErrors.push({ node: node.name, ...e });
            });
        });

        this.markDirty();
        this.updatePropertiesPanel();

        // Clear all dismissed suggestions
        if (this.routingSuggestions) {
            this.routingSuggestions.clearAllDismissed();
        }

        // Phase 5: Show results
        const hasAddressFixes = results.addressFixes && results.addressFixes.length > 0;

        if (remainingErrors.length > 0) {
            // There are still errors - ask user what to do
            this.showAutoConfigResultsDialog(results, remainingWarnings, remainingErrors, fixedErrors);
        } else if (remainingWarnings.length > 0 || hasAddressFixes) {
            // Warnings or address fixes - show info but keep config
            this.showAutoConfigResultsDialog(results, remainingWarnings, remainingErrors, fixedErrors);
        } else {
            // Success - no issues
            alert(`✓ Auto-configured routing successfully!\n\n` +
                `Configured: ${results.configured.length} node(s)\n` +
                `Skipped: ${results.skipped.length} node(s)\n` +
                `No warnings or errors.`);
        }
    }

    /**
     * Analyze topology structure for routing configuration
     */
    analyzeTopologyForRouting() {
        const routers = [];
        const endNodes = [];
        const bridges = [];
        const isolated = [];

        this.topology.nodes.forEach(node => {
            const role = this.routingAnalyzer.analyzeNodeRole(node.name);
            switch (role) {
                case 'router':
                    routers.push(node.name);
                    break;
                case 'end-node':
                    endNodes.push(node.name);
                    break;
                case 'bridge':
                    bridges.push(node.name);
                    break;
                case 'isolated':
                    isolated.push(node.name);
                    break;
            }
        });

        return {
            routers,
            endNodes,
            bridges,
            isolated,
            subnets: this.topology.topology.zmq_proxies.map(s => s.name)
        };
    }

    /**
     * Show dialog with auto-configuration results
     */
    showAutoConfigResultsDialog(results, warnings, errors, fixedErrors) {
        // Build message
        let msg = `Auto-Configuration Results\n\n`;

        msg += `Configured: ${results.configured.length} node(s)\n`;
        results.configured.forEach(n => {
            msg += `  • ${n.name} (${n.role}): ${n.routeCount} routes\n`;
        });

        if (results.skipped.length > 0) {
            msg += `\nSkipped: ${results.skipped.length} node(s)\n`;
            results.skipped.forEach(n => {
                msg += `  • ${n.name}: ${n.reason}\n`;
            });
        }

        // Show address fixes (invalid addresses that were auto-corrected)
        if (results.addressFixes && results.addressFixes.length > 0) {
            msg += `\n🔧 Address fixes: ${results.addressFixes.length}\n`;
            results.addressFixes.forEach(f => {
                msg += `  • ${f.node}.${f.interface}: ${f.oldAddress} → ${f.newAddress} (subnet ${f.subnet})\n`;
            });
        }

        if (fixedErrors.length > 0) {
            msg += `\nAuto-fixed routing: ${fixedErrors.length} issue(s)\n`;
            fixedErrors.forEach(f => {
                msg += `  • ${f.node}: ${f.action}\n`;
            });
        }

        if (errors.length > 0) {
            msg += `\n⛔ Errors: ${errors.length}\n`;
            errors.forEach(e => {
                msg += `  • ${e.node}: ${e.message}\n`;
            });
        }

        if (warnings.length > 0) {
            msg += `\n⚠️ Warnings: ${warnings.length}\n`;
            // Group by type to avoid verbose output
            const byType = {};
            warnings.forEach(w => {
                if (!byType[w.type]) byType[w.type] = [];
                byType[w.type].push(w);
            });
            Object.entries(byType).forEach(([type, items]) => {
                if (type === 'overlapping_routes') {
                    msg += `  • ${items.length} overlapping route(s) detected (may be intentional for deduplication)\n`;
                } else if (type === 'unreachable_subnet') {
                    msg += `  • ${items.length} unreachable subnet warning(s)\n`;
                } else if (type === 'empty_table') {
                    msg += `  • ${items.length} node(s) with empty routing table\n`;
                } else {
                    msg += `  • ${items.length} ${type} warning(s)\n`;
                }
            });
        }

        if (errors.length > 0) {
            msg += `\nConfiguration has errors. Keep partial configuration or discard?`;
            if (confirm(msg + '\n\nClick OK to keep, Cancel to discard.')) {
                // Keep - already applied
            } else {
                // Discard - undo
                this.undo();
            }
        } else {
            // Only warnings - show info
            alert(msg);
        }
    }

    /**
     * Apply a routing suggestion
     * @param {string} nodeName - Name of the node
     * @param {Object} suggestion - Suggestion object
     */
    applySuggestion(nodeName, suggestion) {
        if (!this.routingSuggestions) return;

        this.pushUndo();

        const success = this.routingSuggestions.applySuggestion(nodeName, suggestion);
        if (success) {
            this.markDirty();
            this.updatePropertiesPanel();
        } else {
            alert('Failed to apply suggestion.');
        }
    }

    /**
     * Dismiss a routing suggestion
     * @param {string} nodeName - Name of the node
     * @param {string} suggestionId - ID of the suggestion
     */
    dismissSuggestion(nodeName, suggestionId) {
        if (!this.routingSuggestions) return;

        this.routingSuggestions.dismissSuggestion(nodeName, suggestionId);
        this.updatePropertiesPanel();
    }

    // ==========================================================================
    // Routing Table Reordering (Drag and Drop)
    // ==========================================================================

    /**
     * Handle drag start event for routing entries
     * @param {DragEvent} event - Drag event
     */
    handleRouteDragStart(event) {
        const card = event.target.closest('.routing-entry');
        if (!card) return;

        this.draggedRouteIndex = parseInt(card.dataset.routeIndex);
        this.draggedNodeName = card.dataset.nodeName;

        event.dataTransfer.effectAllowed = 'move';
        event.dataTransfer.setData('text/html', card.innerHTML);

        // Add visual feedback
        card.style.opacity = '0.4';
        card.style.cursor = 'grabbing';
    }

    /**
     * Handle drag over event for routing entries
     * @param {DragEvent} event - Drag event
     */
    handleRouteDragOver(event) {
        if (event.preventDefault) {
            event.preventDefault();
        }

        event.dataTransfer.dropEffect = 'move';

        const card = event.target.closest('.routing-entry');
        if (card && this.draggedRouteIndex !== undefined) {
            // Add visual indicator for drop zone
            card.style.borderTop = '2px solid #0d6efd';
        }

        return false;
    }

    /**
     * Handle drop event for routing entries
     * @param {DragEvent} event - Drag event
     */
    handleRouteDrop(event) {
        if (event.stopPropagation) {
            event.stopPropagation();
        }

        const card = event.target.closest('.routing-entry');
        if (!card) return false;

        const dropIndex = parseInt(card.dataset.routeIndex);
        const nodeName = card.dataset.nodeName;

        // Only reorder if dropping on a different position
        if (this.draggedRouteIndex !== dropIndex && this.draggedNodeName === nodeName) {
            this.reorderRoutingEntry(nodeName, this.draggedRouteIndex, dropIndex);
        }

        // Remove visual indicators
        card.style.borderTop = '';

        return false;
    }

    /**
     * Handle drag end event for routing entries
     * @param {DragEvent} event - Drag event
     */
    handleRouteDragEnd(event) {
        const card = event.target.closest('.routing-entry');
        if (card) {
            card.style.opacity = '1';
            card.style.cursor = 'grab';
        }

        // Remove all visual indicators
        document.querySelectorAll('.routing-entry').forEach(entry => {
            entry.style.borderTop = '';
        });

        this.draggedRouteIndex = undefined;
        this.draggedNodeName = undefined;
    }

    /**
     * Reorder a routing table entry
     * @param {string} nodeName - Name of the node
     * @param {number} fromIndex - Source index
     * @param {number} toIndex - Destination index
     */
    reorderRoutingEntry(nodeName, fromIndex, toIndex) {
        const node = this.getNode(nodeName);
        if (!node || !node.routing_table) return;

        this.pushUndo();

        const routingTable = node.routing_table;
        const [movedRoute] = routingTable.splice(fromIndex, 1);
        routingTable.splice(toIndex, 0, movedRoute);

        this.markDirty();
        this.updatePropertiesPanel();
    }

    /**
     * Update routing analyzer with current topology
     * Call this after loading a new topology
     */
    updateRoutingAnalyzer() {
        if (this.routingAnalyzer) {
            this.routingAnalyzer.topology = this.topology;
        }
        if (this.routingSuggestions) {
            this.routingSuggestions.topology = this.topology;
        }
    }

    /**
     * Validate routing for a node
     * @param {string} nodeName - Name of the node
     * @returns {Object} - Validation results
     */
    validateNodeRouting(nodeName) {
        if (!this.routingAnalyzer) return { valid: true, warnings: [], errors: [] };

        return this.routingAnalyzer.validateRoutingTable(nodeName);
    }
}

// =============================================================================
// Global Instance and Helper Functions
// =============================================================================

let topologyEditor = null;

// Initialize editor when document is ready
document.addEventListener('DOMContentLoaded', () => {
    // Initialize editor only when the Editor tab is shown for the first time
    const editorTab = document.getElementById('editor-tab');
    if (editorTab) {
        editorTab.addEventListener('shown.bs.tab', () => {
            if (!topologyEditor) {
                topologyEditor = new TopologyEditor();
                // Make editor globally accessible for socket.io listeners
                window.topologyEditor = topologyEditor;
                topologyEditor.init();
            } else {
                // Re-render canvas when tab is shown (fixes rendering issues)
                topologyEditor.renderCanvas();
                // Also redraw the network to ensure proper sizing
                if (topologyEditor.network) {
                    topologyEditor.network.redraw();
                    setTimeout(() => {
                        topologyEditor.network.fit();
                    }, 100);
                }
                // Sync with server if we're in live mode and topology might have changed
                if (topologyEditor.isLiveTopology) {
                    topologyEditor.updateLiveTopologyIndicator();
                }
            }
        });
    }
});

// Global functions called from HTML buttons
function editorNewTopology() {
    if (topologyEditor) topologyEditor.newTopology();
}

function editorImportTopology() {
    if (topologyEditor) topologyEditor.importTopology();
}

function editorExportTopology() {
    if (topologyEditor) topologyEditor.exportTopology();
}

function editorSaveDraft() {
    if (topologyEditor) topologyEditor.saveDraft();
}

function editorDiscardChanges() {
    if (topologyEditor) topologyEditor.discardChanges();
}

function editorRefreshFromServer() {
    if (topologyEditor) topologyEditor.refreshFromServer();
}

function editorValidate() {
    console.log('editorValidate called, topologyEditor:', topologyEditor);
    if (topologyEditor) {
        console.log('Calling validate(true)...');
        topologyEditor.validate(true); // Show success alert when manually triggered
    } else {
        console.error('topologyEditor is not initialized!');
    }
}

function editorUndo() {
    if (topologyEditor) topologyEditor.undo();
}

function editorRedo() {
    if (topologyEditor) topologyEditor.redo();
}

function editorAddNode() {
    if (topologyEditor) topologyEditor.addNode();
}

function editorAddSubnet() {
    if (topologyEditor) topologyEditor.addSubnet();
}

function editorSyncWithServer() {
    if (topologyEditor) topologyEditor.syncWithServer();
}

function editorApplyChanges() {
    if (topologyEditor) topologyEditor.applyChanges();
}

// Toggle validation card collapse
function toggleValidationCard() {
    const cardBody = document.getElementById('validation-card-body');
    const chevron = document.getElementById('validation-chevron');

    if (cardBody.classList.contains('collapsed')) {
        cardBody.classList.remove('collapsed');
        chevron.classList.remove('rotated');
    } else {
        cardBody.classList.add('collapsed');
        chevron.classList.add('rotated');
    }
}

// Mode toggle (Read-only vs Edit)
function editorToggleMode() {
    if (topologyEditor) topologyEditor.toggleMode();
}

// Graph filtering functions
function applyGraphFilters() {
    if (topologyEditor) topologyEditor.applyGraphFilters();
}

function resetGraphFilters() {
    if (topologyEditor) topologyEditor.resetGraphFilters();
}

function fitGraphToScreen() {
    if (topologyEditor) topologyEditor.fitGraphToScreen();
}

// Aliases with editor prefix for HTML onclick handlers
function editorApplyFilters() {
    applyGraphFilters();
}

function editorResetFilters() {
    resetGraphFilters();
}

function editorFitToScreen() {
    fitGraphToScreen();
}

// Node stats overlay (called from close button)
function hideNodeStatsOverlay() {
    if (topologyEditor) topologyEditor.hideNodeStats();
}
