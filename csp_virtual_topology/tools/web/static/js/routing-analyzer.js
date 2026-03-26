/**
 * Routing Analyzer - Core analysis engine for CSP routing tables
 * Analyzes topology structure and provides routing intelligence
 */

class RoutingAnalyzer {
    constructor(topology) {
        this.topology = topology;
    }

    /**
     * Analyze node role based on interfaces
     * @param {string} nodeName - Name of the node to analyze
     * @returns {string} - 'router', 'end-node', 'isolated', or 'bridge'
     */
    analyzeNodeRole(nodeName) {
        const node = this.getNode(nodeName);
        if (!node) return 'unknown';

        const interfaces = node.interfaces || [];

        // No interfaces = isolated
        if (interfaces.length === 0) {
            return 'isolated';
        }

        // Single interface = end node
        if (interfaces.length === 1) {
            return 'end-node';
        }

        // Multiple interfaces - check if on different subnets
        const subnets = new Set();
        interfaces.forEach(iface => {
            if (iface.zmq_proxy) {
                subnets.add(iface.zmq_proxy);
            }
        });

        // Multiple interfaces on different subnets = router
        if (subnets.size > 1) {
            return 'router';
        }

        // Multiple interfaces on same subnet = bridge
        return 'bridge';
    }

    /**
     * Find all routers on a given subnet
     * @param {string} subnetName - Name of the subnet
     * @returns {Array} - Array of router info objects
     */
    findRoutersOnSubnet(subnetName) {
        const routers = [];
        const nodes = this.topology.nodes || [];

        nodes.forEach(node => {
            // Check if node has interface on this subnet
            const ifaceOnSubnet = node.interfaces.find(i => i.zmq_proxy === subnetName);
            if (!ifaceOnSubnet) return;

            // Check if node has interfaces on other subnets (making it a router)
            const otherSubnets = node.interfaces
                .filter(i => i.zmq_proxy && i.zmq_proxy !== subnetName)
                .map(i => i.zmq_proxy);

            if (otherSubnets.length > 0) {
                // Get unique subnets
                const uniqueSubnets = [subnetName, ...new Set(otherSubnets)];

                routers.push({
                    name: node.name,
                    address: ifaceOnSubnet.address,
                    interface: ifaceOnSubnet.name,
                    connectedSubnets: uniqueSubnets
                });
            }
        });

        return routers;
    }

    /**
     * Check if a node can reach a destination address
     * @param {string} nodeName - Name of the source node
     * @param {number} destAddress - Destination address
     * @returns {Object} - Reachability info
     */
    canReachDestination(nodeName, destAddress) {
        const node = this.getNode(nodeName);
        if (!node) {
            return { reachable: false, reason: 'Node not found' };
        }

        // Check routing table for matching route
        let bestMatch = null;
        let bestMatchBits = -1;

        const routingTable = node.routing_table || [];

        for (const route of routingTable) {
            // Check if destination matches this route
            const routeBase = route.address;
            const routeMask = route.netmask === 0 ? 0 : (0xFFFF << (16 - route.netmask)) & 0xFFFF;
            const destMasked = destAddress & routeMask;
            const routeMasked = routeBase & routeMask;

            if (destMasked === routeMasked) {
                // This route matches
                if (route.netmask > bestMatchBits) {
                    bestMatch = route;
                    bestMatchBits = route.netmask;
                }
            }
        }

        if (bestMatch) {
            return {
                reachable: true,
                via: bestMatch.interface,
                gateway: bestMatch.via || null,
                route: bestMatch
            };
        }

        // No route found, check default interface
        const defaultIface = node.interfaces.find(i => i.is_default);
        if (defaultIface) {
            return {
                reachable: true,
                via: defaultIface.name,
                gateway: null,
                route: null,
                warning: 'Using default interface (no specific route)'
            };
        }

        return {
            reachable: false,
            reason: 'No matching route and no default interface'
        };
    }

    /**
     * Find unreachable subnets for a node
     * @param {string} nodeName - Name of the node
     * @returns {Array} - Array of unreachable subnet info
     */
    findUnreachableSubnets(nodeName) {
        const unreachable = [];
        const node = this.getNode(nodeName);
        if (!node) return unreachable;

        const subnets = this.topology.topology?.zmq_proxies || [];

        subnets.forEach(subnet => {
            // Check if node is directly connected to this subnet
            const directConnection = node.interfaces.find(i => i.zmq_proxy === subnet.name);
            if (directConnection) return; // Can reach directly

            // Check if routing table has route to this subnet
            const canReach = this.canReachSubnet(node, subnet);
            if (!canReach) {
                // Find potential gateways
                const nodeSubnets = node.interfaces
                    .filter(i => i.zmq_proxy)
                    .map(i => i.zmq_proxy);

                let suggestedGateway = null;
                for (const nodeSubnet of nodeSubnets) {
                    const routers = this.findRoutersOnSubnet(nodeSubnet);
                    const routerToSubnet = routers.find(r =>
                        r.connectedSubnets.includes(subnet.name)
                    );
                    if (routerToSubnet) {
                        suggestedGateway = routerToSubnet;
                        break;
                    }
                }

                unreachable.push({
                    subnet: subnet,
                    suggestedGateway: suggestedGateway
                });
            }
        });

        return unreachable;
    }

    /**
     * Check if node can reach a subnet
     * @param {Object} node - Node object
     * @param {Object} subnet - Subnet object
     * @returns {boolean} - True if reachable
     */
    canReachSubnet(node, subnet) {
        const routingTable = node.routing_table || [];

        for (const route of routingTable) {
            // Check if route covers this subnet
            const subnetBase = subnet.subnet_prefix || 0;
            const subnetMask = subnet.netmask || 8;

            // Check if route matches subnet
            if (route.address === subnetBase && route.netmask === subnetMask) {
                return true;
            }

            // Check if default route exists
            if (route.address === 0 && route.netmask === 0) {
                return true;
            }
        }

        return false;
    }

    /**
     * Validate routing table for a node
     * @param {string} nodeName - Name of the node
     * @returns {Object} - Validation results with warnings and errors
     */
    validateRoutingTable(nodeName) {
        const node = this.getNode(nodeName);
        const warnings = [];
        const errors = [];

        if (!node) {
            errors.push({ type: 'node_not_found', message: 'Node not found' });
            return { valid: false, warnings, errors };
        }

        const routingTable = node.routing_table || [];

        // Rule 1: Empty routing table
        if (routingTable.length === 0) {
            warnings.push({
                type: 'empty_table',
                severity: 'warning',
                message: 'Routing table is empty. Node will use default interface for all traffic.',
                suggestion: 'auto_configure'
            });
        }

        // Rule 2: Interface references
        routingTable.forEach((route, idx) => {
            const ifaceExists = node.interfaces.find(i => i.name === route.interface);
            if (!ifaceExists) {
                errors.push({
                    type: 'invalid_interface',
                    severity: 'error',
                    message: `Route ${idx + 1} references non-existent interface "${route.interface}"`,
                    route: route,
                    routeIndex: idx
                });
            }
        });

        // Rule 3: Unreachable subnets
        const unreachableSubnets = this.findUnreachableSubnets(nodeName);
        unreachableSubnets.forEach(({ subnet, suggestedGateway }) => {
            warnings.push({
                type: 'unreachable_subnet',
                severity: 'warning',
                message: `Cannot reach subnet "${subnet.name}" (${subnet.subnet_prefix}/${subnet.netmask})`,
                subnet: subnet,
                suggestedGateway: suggestedGateway,
                suggestion: 'add_gateway_route'
            });
        });

        // Rule 4: Duplicate/overlapping routes
        for (let i = 0; i < routingTable.length; i++) {
            for (let j = i + 1; j < routingTable.length; j++) {
                if (this.routesOverlap(routingTable[i], routingTable[j])) {
                    warnings.push({
                        type: 'overlapping_routes',
                        severity: 'info',
                        message: 'Multiple routes match the same destination range',
                        routes: [routingTable[i], routingTable[j]],
                        routeIndices: [i, j]
                    });
                }
            }
        }

        return {
            valid: errors.length === 0,
            warnings,
            errors
        };
    }

    /**
     * Check if two routes overlap
     * @param {Object} route1 - First route
     * @param {Object} route2 - Second route
     * @returns {boolean} - True if routes overlap
     */
    routesOverlap(route1, route2) {
        // Same address and netmask = exact duplicate
        if (route1.address === route2.address && route1.netmask === route2.netmask) {
            return true;
        }

        // Check if one route is more specific than the other
        const mask1 = route1.netmask === 0 ? 0 : (0xFFFF << (16 - route1.netmask)) & 0xFFFF;
        const mask2 = route2.netmask === 0 ? 0 : (0xFFFF << (16 - route2.netmask)) & 0xFFFF;

        const base1 = route1.address & mask1;
        const base2 = route2.address & mask2;

        // Check if route2 is within route1's range
        if ((base2 & mask1) === base1) return true;

        // Check if route1 is within route2's range
        if ((base1 & mask2) === base2) return true;

        return false;
    }

    /**
     * Generate optimal routing table for a node
     * @param {string} nodeName - Name of the node
     * @param {Object} options - Generation options
     * @returns {Array} - Generated routing table entries
     */
    generateRoutingTable(nodeName, options = {}) {
        const node = this.getNode(nodeName);
        if (!node) return [];

        const role = this.analyzeNodeRole(nodeName);

        if (role === 'router') {
            return this.generateRouterRoutingTable(node, options);
        } else if (role === 'end-node') {
            return this.generateEndNodeRoutingTable(node, options);
        } else if (role === 'isolated') {
            return [];
        }

        return [];
    }

    /**
     * Generate routing table for a router node
     * @param {Object} node - Node object
     * @param {Object} options - Generation options
     * @returns {Array} - Generated routing table entries
     */
    generateRouterRoutingTable(node, options = {}) {
        const routes = [];
        const seenSubnets = new Set();

        // For each interface on the node
        node.interfaces.forEach(iface => {
            if (!iface.zmq_proxy) return;
            if (seenSubnets.has(iface.zmq_proxy)) return;

            const subnet = this.getSubnet(iface.zmq_proxy);
            if (!subnet) return;

            seenSubnets.add(iface.zmq_proxy);

            // Add direct route to this subnet
            routes.push({
                address: subnet.subnet_prefix || 0,
                netmask: subnet.netmask || 8,
                interface: iface.name,
                via: 0,  // Direct route
                comment: `Direct route to ${subnet.name}`
            });
        });

        return routes;
    }

    /**
     * Generate routing table for an end node
     * @param {Object} node - Node object
     * @param {Object} options - Generation options
     * @returns {Array} - Generated routing table entries
     */
    generateEndNodeRoutingTable(node, options = {}) {
        const routes = [];

        // Get the node's interface (should be only one)
        const iface = node.interfaces[0];
        if (!iface || !iface.zmq_proxy) {
            return routes;  // Cannot configure without interface
        }

        // Find routers on the same subnet
        const routers = this.findRoutersOnSubnet(iface.zmq_proxy);

        if (routers.length === 0) {
            // No router available, add default route via default interface
            routes.push({
                address: 0,
                netmask: 0,
                interface: iface.name,
                via: 0,
                comment: "Default route (no router available)"
            });
        } else {
            // Use first router as gateway
            const gateway = routers[0];

            // Add routes to all subnets reachable via this gateway
            gateway.connectedSubnets.forEach(subnetName => {
                if (subnetName === iface.zmq_proxy) return;  // Skip local subnet

                const subnet = this.getSubnet(subnetName);
                if (!subnet) return;

                routes.push({
                    address: subnet.subnet_prefix || 0,
                    netmask: subnet.netmask || 8,
                    interface: iface.name,
                    via: gateway.address,
                    comment: `Via ${gateway.name} to ${subnet.name}`
                });
            });

            // Add default route via gateway
            routes.push({
                address: 0,
                netmask: 0,
                interface: iface.name,
                via: gateway.address,
                comment: `Default route via ${gateway.name}`
            });
        }

        return routes;
    }

    /**
     * Get node by name
     * @param {string} nodeName - Name of the node
     * @returns {Object|null} - Node object or null
     */
    getNode(nodeName) {
        const nodes = this.topology.nodes || [];
        return nodes.find(n => n.name === nodeName) || null;
    }

    /**
     * Get subnet by name
     * @param {string} subnetName - Name of the subnet
     * @returns {Object|null} - Subnet object or null
     */
    getSubnet(subnetName) {
        const subnets = this.topology.topology?.zmq_proxies || [];
        return subnets.find(s => s.name === subnetName) || null;
    }

    /**
     * Format route for display
     * @param {Object} route - Route object
     * @returns {string} - Formatted route string
     */
    formatRoute(route) {
        const dest = `${route.address}/${route.netmask}`;
        const via = route.via ? ` via ${route.via}` : ' (direct)';
        return `${dest} → ${route.interface}${via}`;
    }
}
