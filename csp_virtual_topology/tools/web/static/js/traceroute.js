/**
 * Traceroute functionality for CSP Virtual Topology Dashboard
 */

// Global state
let currentTrace = null;
let batchResults = null;
let topologyNodes = null; // Store topology data for interface lookups
let topologyProxies = null; // Store ZMQ proxy data for address calculation

/**
 * Calculate fully-qualified 14-bit CSP address
 * @param {number} subnetPrefix - Subnet prefix from ZMQ proxy
 * @param {number} netmask - Number of network bits (CIDR notation)
 * @param {number} hostAddress - Host address from interface
 * @returns {number} Fully-qualified 14-bit CSP address
 */
function calculateFullAddress(subnetPrefix, netmask, hostAddress) {
    const networkBits = 14 - netmask;  // netmask = network bits, host bits = 14 - netmask
    const hostMask = (1 << (14 - netmask)) - 1;
    const fullAddress = ((subnetPrefix << networkBits) | (hostAddress & hostMask)) & 0x3FFF;
    return fullAddress;
}

/**
 * Get fully-qualified address for a node's default interface
 * @param {object} node - Node object from topology
 * @returns {number|null} Fully-qualified address or null if not found
 */
function getNodeFullAddress(node) {
    if (!node || !node.interfaces || node.interfaces.length === 0) {
        return null;
    }

    // Find default interface or use first interface
    const defaultIface = node.interfaces.find(i => i.is_default) || node.interfaces[0];
    const proxyName = defaultIface.zmq_proxy;
    const hostAddr = defaultIface.address;

    // Find the proxy configuration
    if (!topologyProxies) {
        return null;
    }

    const proxy = topologyProxies.find(p => p.name === proxyName);
    if (!proxy) {
        return null;
    }

    return calculateFullAddress(proxy.subnet_prefix || 0, proxy.netmask || 8, hostAddr);
}

/**
 * Initialize traceroute UI when topology data is loaded
 */
function initTracerouteUI(topologyData) {
    console.log('[Traceroute] Initializing UI with topology data:', topologyData);

    if (!topologyData) {
        console.error('[Traceroute] No topology data provided');
        return;
    }

    const nodes = topologyData.nodes || [];
    const proxies = topologyData.topology?.zmq_proxies || [];
    console.log('[Traceroute] Found', nodes.length, 'nodes and', proxies.length, 'proxies');

    // Store topology data globally for interface lookups
    topologyNodes = nodes;
    topologyProxies = proxies;

    // Populate source and destination node dropdowns
    const srcSelect = document.getElementById('trace-src-node');
    const dstSelect = document.getElementById('trace-dst-node');
    const batchSrcSelect = document.getElementById('batch-src-node');

    if (!srcSelect || !dstSelect) {
        console.error('[Traceroute] Dropdown elements not found in DOM');
        return;
    }

    srcSelect.innerHTML = '<option value="">Select node...</option>';
    dstSelect.innerHTML = '<option value="">Select node...</option>';

    if (batchSrcSelect) {
        batchSrcSelect.innerHTML = '<option value="">Select source node...</option>';
    }

    nodes.forEach(node => {
        // Calculate fully-qualified address for display
        const fullAddr = getNodeFullAddress(node);
        const displayText = fullAddr !== null ? `${node.name} [${fullAddr}]` : node.name;

        const srcOption = document.createElement('option');
        srcOption.value = node.name;
        srcOption.textContent = displayText;
        srcSelect.appendChild(srcOption);

        const dstOption = document.createElement('option');
        dstOption.value = node.name;
        dstOption.textContent = displayText;
        dstSelect.appendChild(dstOption);

        // Populate batch source dropdown
        if (batchSrcSelect) {
            const batchOption = document.createElement('option');
            batchOption.value = node.name;
            batchOption.textContent = displayText;
            batchSrcSelect.appendChild(batchOption);
        }
    });

    // Add event listeners to populate interface dropdowns when node is selected
    srcSelect.addEventListener('change', () => populateInterfaceDropdown('src'));
    dstSelect.addEventListener('change', () => populateInterfaceDropdown('dst'));

    console.log('[Traceroute] UI initialized with', nodes.length, 'nodes');
}

/**
 * Populate interface dropdown when a node is selected
 */
function populateInterfaceDropdown(type) {
    const nodeSelect = document.getElementById(`trace-${type}-node`);
    const ifaceSelect = document.getElementById(`trace-${type}-interface`);

    const nodeName = nodeSelect.value;

    if (!nodeName) {
        ifaceSelect.innerHTML = '<option value="">Interface...</option>';
        return;
    }

    // Find the selected node
    const node = topologyNodes.find(n => n.name === nodeName);
    if (!node || !node.interfaces || node.interfaces.length === 0) {
        ifaceSelect.innerHTML = '<option value="">No interfaces</option>';
        return;
    }

    // Populate interface dropdown
    ifaceSelect.innerHTML = '';

    // Determine which interface to select by default
    let defaultIndex = 0;

    // Find default interface
    const defaultIfaceIndex = node.interfaces.findIndex(iface => iface.is_default);
    if (defaultIfaceIndex >= 0) {
        defaultIndex = defaultIfaceIndex;
    } else {
        // Prefer CAN interfaces
        const canIfaceIndex = node.interfaces.findIndex(iface => {
            const name = iface.name || '';
            const proxy = iface.zmq_proxy || '';
            return name.includes('CAN') || proxy.includes('CAN');
        });
        if (canIfaceIndex >= 0) {
            defaultIndex = canIfaceIndex;
        }
    }

    node.interfaces.forEach((iface, index) => {
        const option = document.createElement('option');
        option.value = iface.address;
        option.textContent = `${iface.name} (${iface.address})`;
        if (index === defaultIndex) {
            option.selected = true;
        }
        ifaceSelect.appendChild(option);
    });
}

/**
 * Run a single traceroute between two nodes
 */
async function runSingleTraceroute() {
    const srcNode = document.getElementById('trace-src-node').value;
    const dstNode = document.getElementById('trace-dst-node').value;
    const srcAddr = document.getElementById('trace-src-interface').value;
    const dstAddr = document.getElementById('trace-dst-interface').value;

    console.log('[Traceroute] Running traceroute from', srcNode, '@', srcAddr, 'to', dstNode, '@', dstAddr);

    if (!srcNode || !dstNode) {
        showTraceError('Please select both source and destination nodes');
        return;
    }

    if (!srcAddr || !dstAddr) {
        showTraceError('Please select interfaces for both source and destination');
        return;
    }

    if (srcNode === dstNode && srcAddr === dstAddr) {
        showTraceError('Source and destination must be different');
        return;
    }

    // Show loading indicator
    document.getElementById('trace-loading').style.display = 'block';
    document.getElementById('trace-results').innerHTML = '';
    document.getElementById('btn-run-trace').disabled = true;

    try {
        console.log('[Traceroute] Sending request to /api/traceroute/start');
        const response = await fetch('/api/traceroute/start', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({
                src_node: srcNode,
                dst_node: dstNode,
                src_addr: parseInt(srcAddr),
                dst_addr: parseInt(dstAddr)
            })
        });

        console.log('[Traceroute] Response status:', response.status);
        const result = await response.json();
        console.log('[Traceroute] Response data:', result);

        if (result.success) {
            currentTrace = result;
            displaySingleTrace(result);
        } else {
            showTraceError(result.error || 'Traceroute failed');
        }

    } catch (error) {
        console.error('[Traceroute] Error:', error);
        showTraceError(`Error: ${error.message}`);
    } finally {
        document.getElementById('trace-loading').style.display = 'none';
        document.getElementById('btn-run-trace').disabled = false;
    }
}

/**
 * Display single traceroute results
 */
function displaySingleTrace(trace) {
    const container = document.getElementById('trace-results');

    if (!trace.hops || trace.hops.length === 0) {
        container.innerHTML = '<div class="alert alert-warning mb-0"><small>No trace data received</small></div>';
        return;
    }

    // Separate actual route hops from promiscuous captures
    const routeHops = [];
    const promiscuousHops = [];

    trace.hops.forEach(hop => {
        // Determine if this is part of the actual route or just a promiscuous capture
        // - route_code == 5 (CSP_TRACE_DELIVERED) → Delivered (part of route)
        // - action == 1 (Forwarded) → Part of route
        // - action == 2 (Dropped) → Part of route (shows where it failed)
        // - action == 0 and route_code != 5 → Promiscuous capture (received but not delivered)
        const isDelivered = (hop.route_code === 5);  // CSP_TRACE_DELIVERED
        const isForwarded = (hop.action === 1);
        const isDropped = (hop.action === 2);
        const isPromiscuous = (hop.action === 0 && !isDelivered);

        if (isPromiscuous) {
            promiscuousHops.push(hop);
        } else if (isDelivered || isForwarded || isDropped) {
            routeHops.push(hop);
        }
    });

    // Check if destination received the packet
    // Only route_code === 5 (CSP_TRACE_DELIVERED) means successful delivery
    // action === 0 (RECEIVED) is not sufficient - packet could be dropped after reception
    const destinationReceived = routeHops.some(hop =>
        hop.route_code === 5  // CSP_TRACE_DELIVERED
    );
    const summaryClass = destinationReceived ? 'alert-success' : 'alert-warning';
    const summaryIcon = destinationReceived ? 'check-circle-fill' : 'exclamation-triangle-fill';
    const summaryText = destinationReceived ?
        `Packet successfully reached destination (${trace.dst})` :
        `Packet did not reach destination (${trace.dst})`;

    // Calculate actual hop count (unique nodes in the route)
    // Count: 1 for source forwarding + 1 for destination delivery
    const uniqueNodes = new Set(routeHops.map(hop => hop.node_addr));
    const hopCount = uniqueNodes.size;

    // Build HTML - status box first
    let html = `<div class="alert ${summaryClass} mb-2 p-2">`;
    html += `<small><i class="bi bi-${summaryIcon}"></i> ${summaryText}</small>`;
    html += `<br><small class="text-muted">Actual route: ${hopCount} hop${hopCount !== 1 ? 's' : ''}`;
    if (promiscuousHops.length > 0) {
        html += ` | Promiscuous captures: ${promiscuousHops.length}`;
    }
    html += '</small></div>';

    // Then the trace table
    html += '<div class="table-responsive">';
    html += '<table class="table table-sm table-hover mb-0">';
    html += '<thead class="table-light"><tr>';
    html += '<th style="width: 50px;">Hop</th>';
    html += '<th>Node</th>';
    html += '<th>Interface</th>';
    html += '<th>Action</th>';
    html += '<th>Code</th>';
    html += '<th>Via</th>';
    html += '</tr></thead><tbody>';

    // Group route hops by node to show only one row per node
    const nodeHops = new Map();
    routeHops.forEach(hop => {
        const nodeAddr = hop.node_addr;
        if (!nodeHops.has(nodeAddr)) {
            nodeHops.set(nodeAddr, []);
        }
        nodeHops.get(nodeAddr).push(hop);
    });

    // Display actual route hops first (one row per unique node)
    let hopNumber = 1;
    nodeHops.forEach((hops) => {
        // Use the first hop for this node as the representative
        const hop = hops[0];
        const action = getActionInfo(hop.action, hop.route_code);
        const isDelivered = (hop.route_code === 5);  // CSP_TRACE_DELIVERED

        // Override action label for delivered packets
        let displayAction = action;
        if (isDelivered) {
            displayAction = {
                icon: '✅',
                label: 'Delivered',
                code: 'R05: Delivered',
                color: 'success'
            };
        }

        // Build row class
        let rowClass = '';
        if (hop.action === 2) {
            // Dropped packets - red
            rowClass = 'table-danger';
        }

        const via = hop.via === 0xFFFF ? '-' : hop.via;

        // Show all unique interfaces if multiple (deduplicate)
        let ifaceDisplay = hop.iface;
        if (hops.length > 1) {
            const uniqueIfaces = [...new Set(hops.map(h => h.iface))];
            ifaceDisplay = uniqueIfaces.join(', ');
        }

        html += `<tr class="${rowClass}">`;
        html += `<td><strong>${hopNumber}</strong></td>`;
        html += `<td>${hop.node_addr}${isDelivered ? ' 🎯' : ''}</td>`;
        html += `<td><code class="small">${ifaceDisplay}</code></td>`;
        html += `<td>${displayAction.icon} <small>${displayAction.label}</small></td>`;
        html += `<td><small>${displayAction.code}</small></td>`;
        html += `<td>${via}</td>`;
        html += '</tr>';

        hopNumber++;
    });

    // Then display promiscuous captures (grayed out)
    promiscuousHops.forEach((hop) => {
        const action = getActionInfo(hop.action, hop.route_code);
        const via = hop.via === 0xFFFF ? '-' : hop.via;

        html += `<tr style="opacity: 0.4; font-style: italic;">`;
        html += `<td><small class="text-muted">—</small></td>`;
        html += `<td>${hop.node_addr}</td>`;
        html += `<td><code class="small">${hop.iface}</code></td>`;
        html += `<td>${action.icon} <small>${action.label}</small></td>`;
        html += `<td><small>${action.code}</small></td>`;
        html += `<td>${via}</td>`;
        html += '</tr>';
    });

    html += '</tbody></table></div>';

    container.innerHTML = html;
}

/**
 * Get action information (icon, label, code description)
 */
function getActionInfo(action, routeCode) {
    const actions = {
        0: { icon: '📥', label: 'Received', color: 'success' },
        1: { icon: '➡️', label: 'Forwarded', color: 'primary' },
        2: { icon: '❌', label: 'Dropped', color: 'danger' }
    };

    // Drop reason codes - mapped to actual CSP_TRACE_DROP_* values from csp_traceroute.h
    const dropReasons = {
        10: 'D01: Duplicate packet',           // CSP_TRACE_DROP_DUPLICATE
        11: 'D02: Unsupported HMAC',           // CSP_TRACE_DROP_UNSUP_HMAC
        12: 'D03: Unsupported RDP',            // CSP_TRACE_DROP_UNSUP_RDP
        13: 'D04: CRC32 verification failed',  // CSP_TRACE_DROP_CRC32_FAIL
        14: 'D05: CRC32 required',             // CSP_TRACE_DROP_CRC32_REQ
        15: 'D06: HMAC verification failed',   // CSP_TRACE_DROP_HMAC_FAIL
        16: 'D07: HMAC required',              // CSP_TRACE_DROP_HMAC_REQ
        17: 'D08: RDP required',               // CSP_TRACE_DROP_RDP_REQ
        18: 'D09: No socket listening',        // CSP_TRACE_DROP_NO_SOCKET
        19: 'D10: Socket queue full',          // CSP_TRACE_DROP_QUEUE_SOCKET
        20: 'D11: Connection queue full',      // CSP_TRACE_DROP_QUEUE_CONN
        21: 'D12: No connection available',    // CSP_TRACE_DROP_NO_CONN
        22: 'D13: Socket queue full (conn)',   // CSP_TRACE_DROP_SOCK_QUEUE
        23: 'D14: No route found'              // CSP_TRACE_DROP_NO_ROUTE
    };

    // Routing decision codes - mapped to actual CSP_TRACE_* values from csp_traceroute.h
    const routeCodes = {
        0: 'Received',                    // CSP_TRACE_RECEIVED
        1: 'R01: Loopback',               // CSP_TRACE_ROUTE_LOOPBACK
        2: 'R02: Subnet match',           // CSP_TRACE_ROUTE_SUBNET
        3: 'R03: Routing table',          // CSP_TRACE_ROUTE_TABLE
        4: 'R04: Default interface',      // CSP_TRACE_ROUTE_DEFAULT
        5: 'Delivered'                    // CSP_TRACE_DELIVERED
    };

    const actionInfo = actions[action] || { icon: '?', label: 'Unknown', color: 'secondary' };

    let code = '';
    if (action === 2) {
        // Dropped - show drop reason
        code = dropReasons[routeCode] || `D${String(routeCode).padStart(2, '0')}`;
    } else if (action === 1) {
        // Forwarded - show routing decision
        code = routeCodes[routeCode] || `R${String(routeCode).padStart(2, '0')}`;
    }

    return {
        icon: actionInfo.icon,
        label: actionInfo.label,
        code: code,
        color: actionInfo.color
    };
}

/**
 * Run batch traceroute from one source node to all destinations using all interfaces
 */
async function runBatchTraceroute() {
    const srcNode = document.getElementById('batch-src-node').value;

    if (!srcNode) {
        showBatchError('Please select a source node');
        return;
    }

    console.log('[Batch] Running batch traceroute from', srcNode);

    // Show loading indicator
    document.getElementById('batch-loading').style.display = 'block';
    document.getElementById('batch-results').innerHTML = '';
    document.getElementById('btn-run-batch').disabled = true;

    // Reset progress bar
    updateBatchProgress(0, 0, 'Starting...');

    try {
        const response = await fetch('/api/traceroute/batch', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({
                src_node: srcNode
            })
        });

        const result = await response.json();
        console.log('[Batch] Response:', result);

        if (result.success) {
            // Start polling for progress
            pollBatchProgress();
        } else {
            showBatchError(result.error || 'Batch traceroute failed');
            document.getElementById('batch-loading').style.display = 'none';
            document.getElementById('btn-run-batch').disabled = false;
        }

    } catch (error) {
        console.error('[Batch] Error:', error);
        showBatchError(`Error: ${error.message}`);
        document.getElementById('batch-loading').style.display = 'none';
        document.getElementById('btn-run-batch').disabled = false;
    }
}

/**
 * Poll batch traceroute progress
 */
let batchProgressInterval = null;

async function pollBatchProgress() {
    // Clear any existing interval
    if (batchProgressInterval) {
        clearInterval(batchProgressInterval);
    }

    batchProgressInterval = setInterval(async () => {
        try {
            const response = await fetch('/api/traceroute/batch/progress');
            const progress = await response.json();

            console.log('[Batch] Progress:', progress);

            // Update progress bar
            const percent = progress.total > 0 ? Math.round((progress.current / progress.total) * 100) : 0;
            updateBatchProgress(progress.current, progress.total, progress.current_test);

            // Check if complete
            if (!progress.running) {
                clearInterval(batchProgressInterval);
                batchProgressInterval = null;

                // Display results
                if (progress.results && progress.results.length > 0) {
                    batchResults = {
                        success: true,
                        results: progress.results,
                        total_tests: progress.total,
                        src_node: progress.results[0].src_node
                    };
                    displayBatchResults(batchResults);
                } else {
                    showBatchError('No results received');
                }

                // Hide loading, enable button
                document.getElementById('batch-loading').style.display = 'none';
                document.getElementById('btn-run-batch').disabled = false;
            }

        } catch (error) {
            console.error('[Batch] Progress poll error:', error);
            clearInterval(batchProgressInterval);
            batchProgressInterval = null;
            showBatchError(`Error polling progress: ${error.message}`);
            document.getElementById('batch-loading').style.display = 'none';
            document.getElementById('btn-run-batch').disabled = false;
        }
    }, 500); // Poll every 500ms
}

/**
 * Update batch progress bar
 */
function updateBatchProgress(current, total, message) {
    const percent = total > 0 ? Math.round((current / total) * 100) : 0;
    const progressBar = document.getElementById('batch-progress-bar');
    const progressText = document.getElementById('batch-progress-text');

    if (progressBar) {
        progressBar.style.width = `${percent}%`;
        progressBar.setAttribute('aria-valuenow', percent);
        progressBar.innerHTML = `<small>${percent}%</small>`;
    }

    if (progressText) {
        progressText.textContent = `${message} (${current}/${total})`;
    }
}

/**
 * Display batch traceroute results as an interface-level reachability matrix
 */
function displayBatchResults(result) {
    const container = document.getElementById('batch-results');

    if (!result.results || result.results.length === 0) {
        container.innerHTML = '<div class="alert alert-warning mb-0"><small>No results</small></div>';
        return;
    }

    // Group results by source interface and destination node
    const srcInterfaces = new Set();
    const dstNodes = new Set();
    const matrix = {}; // matrix[src_iface][dst_node] = array of results

    result.results.forEach(r => {
        const srcKey = `${r.src_iface} (${r.src_addr})`;
        srcInterfaces.add(srcKey);
        dstNodes.add(r.dst_node);

        if (!matrix[srcKey]) {
            matrix[srcKey] = {};
        }
        if (!matrix[srcKey][r.dst_node]) {
            matrix[srcKey][r.dst_node] = [];
        }
        matrix[srcKey][r.dst_node].push(r);
    });

    const srcIfaceList = Array.from(srcInterfaces).sort();
    const dstNodeList = Array.from(dstNodes).sort();

    // Build summary header
    let html = `<div class="alert alert-info mb-2 p-2">`;
    html += `<small><strong>Source:</strong> ${result.src_node} | `;
    html += `<strong>Tested:</strong> ${result.total_tests} interface combinations</small>`;
    html += `</div>`;

    // Build HTML table
    html += '<div class="table-responsive">';
    html += '<table class="table table-sm table-bordered mb-0 reachability-matrix">';
    html += '<thead class="table-light"><tr>';
    html += '<th class="small">Source Interface</th>';

    dstNodeList.forEach(node => {
        html += `<th class="text-center small" style="min-width: 80px;"><small>${node}</small></th>`;
    });
    html += '</tr></thead><tbody>';

    // Each row is a source interface
    srcIfaceList.forEach(srcIface => {
        html += `<tr><th class="small">${srcIface}</th>`;

        dstNodeList.forEach(dstNode => {
            const results = matrix[srcIface] && matrix[srcIface][dstNode];

            if (!results || results.length === 0) {
                html += '<td class="text-center"><small>-</small></td>';
            } else {
                // Check if ANY interface combination is reachable
                const anyReachable = results.some(r => r.reachable);
                const allReachable = results.every(r => r.reachable);

                let cellClass, icon, title;
                if (allReachable) {
                    cellClass = 'bg-success-subtle';
                    icon = '✓';
                    title = `All ${results.length} interface(s) reachable`;
                } else if (anyReachable) {
                    cellClass = 'bg-warning-subtle';
                    icon = '◐';
                    const reachableCount = results.filter(r => r.reachable).length;
                    title = `${reachableCount}/${results.length} interface(s) reachable`;
                } else {
                    cellClass = 'bg-danger-subtle';
                    icon = '✗';
                    title = `All ${results.length} interface(s) unreachable`;
                }

                // Make cell clickable to show details
                const srcAddr = results[0].src_addr;
                const dstAddr = results[0].dst_addr;
                html += `<td class="text-center ${cellClass}" title="${title}" style="cursor: pointer;" onclick="showBatchTraceDetail(${srcAddr}, ${dstAddr}, '${dstNode}')">`;
                html += `<small>${icon}</small>`;

                // Show interface count if multiple
                if (results.length > 1) {
                    html += `<br><small style="font-size: 0.7em;">${results.length} ifaces</small>`;
                }
                html += '</td>';
            }
        });

        html += '</tr>';
    });

    html += '</tbody></table></div>';

    // Add legend
    html += '<div class="mt-2 p-2 bg-light rounded">';
    html += '<small class="text-muted">';
    html += '<strong>Legend:</strong> ';
    html += '<span class="badge bg-success-subtle text-dark">✓</span> All reachable &nbsp;';
    html += '<span class="badge bg-warning-subtle text-dark">◐</span> Partial &nbsp;';
    html += '<span class="badge bg-danger-subtle text-dark">✗</span> Unreachable &nbsp;';
    html += '<span class="text-muted">Click a cell to see trace details</span>';
    html += '</small></div>';

    container.innerHTML = html;
}

/**
 * Show detailed trace for a specific interface pair from batch results
 */
async function showBatchTraceDetail(srcAddr, dstAddr, dstNode) {
    if (!batchResults || !batchResults.results) {
        return;
    }

    // Find the result matching this src/dst address pair
    const result = batchResults.results.find(r => r.src_addr === srcAddr && r.dst_addr === dstAddr);
    if (!result) {
        console.error('[Batch] No result found for', srcAddr, '->', dstAddr);
        return;
    }

    console.log('[Batch] Showing detail for', result.src_node, result.src_iface, '->', result.dst_node, result.dst_iface);

    // Set the node dropdowns
    document.getElementById('trace-src-node').value = result.src_node;
    document.getElementById('trace-dst-node').value = result.dst_node;

    // Populate interface dropdowns
    populateInterfaceDropdown('src');
    populateInterfaceDropdown('dst');

    // Set the interface dropdowns to match the trace result
    document.getElementById('trace-src-interface').value = result.src_addr;
    document.getElementById('trace-dst-interface').value = result.dst_addr;

    // Fetch the complete trace from the session file
    // This ensures we get all hops including DELIVERED events that may have
    // arrived after the batch test snapshot was taken
    try {
        let url = `/api/trace/detail?src=${srcAddr}&dst=${dstAddr}`;
        if (result.sport) {
            url += `&sport=${result.sport}`;
        }

        const response = await fetch(url);
        const data = await response.json();

        if (data.success && data.hops) {
            // Update result with complete trace from file
            result.hops = data.hops;
            result.hop_count = data.hop_count;
            console.log('[Batch] Retrieved', data.hop_count, 'hops from session file');
        } else {
            console.warn('[Batch] Failed to retrieve trace from file:', data.error);
            // Fall back to empty hops if no data available
            result.hops = [];
        }
    } catch (error) {
        console.error('[Batch] Error fetching trace detail:', error);
        result.hops = [];
    }

    // Display the trace
    currentTrace = result;
    displaySingleTrace(result);

    // Scroll to results
    document.getElementById('trace-results').scrollIntoView({ behavior: 'smooth', block: 'nearest' });
}

/**
 * Show error message in trace results
 */
function showTraceError(message) {
    const container = document.getElementById('trace-results');
    container.innerHTML = `<div class="alert alert-danger mb-0"><small>${message}</small></div>`;
}

/**
 * Show error message in batch results
 */
function showBatchError(message) {
    const container = document.getElementById('batch-results');
    container.innerHTML = `<div class="alert alert-danger mb-0"><small>${message}</small></div>`;
}

