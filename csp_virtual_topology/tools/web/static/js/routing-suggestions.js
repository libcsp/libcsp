/**
 * Routing Suggestions - Smart suggestion system for routing configurations
 * Provides context-aware suggestions based on topology analysis
 */

class RoutingSuggestions {
    constructor(topology, analyzer) {
        this.topology = topology;
        this.analyzer = analyzer;
        this.dismissedSuggestions = new Map(); // nodeName -> Set of dismissed suggestion IDs
    }

    /**
     * Get all suggestions for a node
     * @param {string} nodeName - Name of the node
     * @returns {Array} - Array of suggestion objects
     */
    getSuggestionsForNode(nodeName) {
        const suggestions = [];
        const node = this.analyzer.getNode(nodeName);
        if (!node) return suggestions;

        const role = this.analyzer.analyzeNodeRole(nodeName);

        // Router configuration suggestion
        if (role === 'router') {
            const routerSuggestion = this.suggestRouterConfig(nodeName);
            if (routerSuggestion && !this.isDismissed(nodeName, routerSuggestion.id)) {
                suggestions.push(routerSuggestion);
            }
        }

        // Default route suggestion for end nodes
        if (role === 'end-node') {
            const defaultRouteSuggestion = this.suggestDefaultRoute(nodeName);
            if (defaultRouteSuggestion && !this.isDismissed(nodeName, defaultRouteSuggestion.id)) {
                suggestions.push(defaultRouteSuggestion);
            }

            // Gateway routes suggestion
            const gatewaySuggestion = this.suggestGatewayRoutes(nodeName);
            if (gatewaySuggestion && !this.isDismissed(nodeName, gatewaySuggestion.id)) {
                suggestions.push(gatewaySuggestion);
            }
        }

        // Empty routing table suggestion
        if ((node.routing_table || []).length === 0) {
            const emptyTableSuggestion = this.suggestAutoConfig(nodeName);
            if (emptyTableSuggestion && !this.isDismissed(nodeName, emptyTableSuggestion.id)) {
                suggestions.push(emptyTableSuggestion);
            }
        }

        return suggestions;
    }

    /**
     * Suggest router configuration
     * @param {string} nodeName - Name of the node
     * @returns {Object|null} - Suggestion object or null
     */
    suggestRouterConfig(nodeName) {
        const node = this.analyzer.getNode(nodeName);
        if (!node) return null;

        const routingTable = node.routing_table || [];

        // Check if already configured
        const interfaces = node.interfaces || [];
        const subnets = new Set(interfaces.filter(i => i.zmq_proxy).map(i => i.zmq_proxy));

        // Count how many subnets have direct routes
        let configuredSubnets = 0;
        subnets.forEach(subnetName => {
            const subnet = this.analyzer.getSubnet(subnetName);
            if (!subnet) return;

            const hasRoute = routingTable.some(r =>
                r.address === (subnet.subnet_prefix || 0) &&
                r.netmask === (subnet.netmask || 8) &&
                r.via === 0
            );

            if (hasRoute) configuredSubnets++;
        });

        // If all subnets are configured, no suggestion needed
        if (configuredSubnets === subnets.size && subnets.size > 0) {
            return null;
        }

        return {
            id: 'router-config',
            type: 'router',
            priority: 'high',
            icon: 'bi-router',
            title: 'Router Detected',
            description: `Node "${nodeName}" connects to ${subnets.size} subnets and can act as a router. Configure direct routes to all connected subnets?`,
            action: 'auto_configure_router',
            data: {
                nodeName: nodeName,
                subnets: Array.from(subnets)
            }
        };
    }

    /**
     * Suggest default route
     * @param {string} nodeName - Name of the node
     * @returns {Object|null} - Suggestion object or null
     */
    suggestDefaultRoute(nodeName) {
        const node = this.analyzer.getNode(nodeName);
        if (!node) return null;

        const routingTable = node.routing_table || [];

        // Check if default route already exists
        const hasDefaultRoute = routingTable.some(r => r.address === 0 && r.netmask === 0);
        if (hasDefaultRoute) return null;

        const interfaces = node.interfaces || [];
        if (interfaces.length !== 1) return null;

        return {
            id: 'default-route',
            type: 'default-route',
            priority: 'medium',
            icon: 'bi-signpost',
            title: 'Default Route Suggestion',
            description: `Node "${nodeName}" has only one interface. Add a default route (0/0) via this interface?`,
            action: 'add_default_route',
            data: {
                nodeName: nodeName,
                interface: interfaces[0].name
            }
        };
    }

    /**
     * Suggest gateway routes
     * @param {string} nodeName - Name of the node
     * @returns {Object|null} - Suggestion object or null
     */
    suggestGatewayRoutes(nodeName) {
        const node = this.analyzer.getNode(nodeName);
        if (!node) return null;

        const interfaces = node.interfaces || [];
        if (interfaces.length !== 1 || !interfaces[0].zmq_proxy) return null;

        // Find routers on same subnet
        const routers = this.analyzer.findRoutersOnSubnet(interfaces[0].zmq_proxy);
        if (routers.length === 0) return null;

        const gateway = routers[0];
        const routingTable = node.routing_table || [];

        // Find subnets reachable via gateway that aren't in routing table
        const unreachableSubnets = [];
        gateway.connectedSubnets.forEach(subnetName => {
            if (subnetName === interfaces[0].zmq_proxy) return; // Skip local subnet

            const subnet = this.analyzer.getSubnet(subnetName);
            if (!subnet) return;

            const hasRoute = routingTable.some(r =>
                r.address === (subnet.subnet_prefix || 0) &&
                r.netmask === (subnet.netmask || 8)
            );

            if (!hasRoute) {
                unreachableSubnets.push(subnet);
            }
        });

        if (unreachableSubnets.length === 0) return null;

        return {
            id: 'gateway-routes',
            type: 'gateway',
            priority: 'high',
            icon: 'bi-globe',
            title: 'Gateway Available',
            description: `Node "${nodeName}" can reach ${unreachableSubnets.length} subnet(s) via "${gateway.name}". Add routes via this gateway?`,
            action: 'add_gateway_routes',
            data: {
                nodeName: nodeName,
                gateway: gateway,
                subnets: unreachableSubnets
            }
        };
    }

    /**
     * Suggest auto-configuration for empty routing table
     * @param {string} nodeName - Name of the node
     * @returns {Object|null} - Suggestion object or null
     */
    suggestAutoConfig(nodeName) {
        const node = this.analyzer.getNode(nodeName);
        if (!node) return null;

        const routingTable = node.routing_table || [];
        if (routingTable.length > 0) return null;

        const interfaces = node.interfaces || [];
        if (interfaces.length === 0) return null;

        return {
            id: 'auto-config',
            type: 'auto-config',
            priority: 'high',
            icon: 'bi-magic',
            title: 'Empty Routing Table',
            description: `Node "${nodeName}" has no routing configuration. Auto-configure routing based on topology?`,
            action: 'auto_configure',
            data: {
                nodeName: nodeName
            }
        };
    }

    /**
     * Apply a suggestion
     * @param {string} nodeName - Name of the node
     * @param {Object} suggestion - Suggestion object
     * @returns {boolean} - True if applied successfully
     */
    applySuggestion(nodeName, suggestion) {
        if (!suggestion || !suggestion.action) return false;

        switch (suggestion.action) {
            case 'auto_configure':
            case 'auto_configure_router':
                return this.applyAutoConfig(nodeName);

            case 'add_default_route':
                return this.applyDefaultRoute(nodeName, suggestion.data);

            case 'add_gateway_routes':
                return this.applyGatewayRoutes(nodeName, suggestion.data);

            default:
                console.warn('Unknown suggestion action:', suggestion.action);
                return false;
        }
    }

    /**
     * Apply auto-configuration
     * @param {string} nodeName - Name of the node
     * @returns {boolean} - True if applied successfully
     */
    applyAutoConfig(nodeName) {
        const routes = this.analyzer.generateRoutingTable(nodeName);
        if (routes.length === 0) return false;

        const node = this.analyzer.getNode(nodeName);
        if (!node) return false;

        node.routing_table = routes;
        return true;
    }

    /**
     * Apply default route
     * @param {string} nodeName - Name of the node
     * @param {Object} data - Suggestion data
     * @returns {boolean} - True if applied successfully
     */
    applyDefaultRoute(nodeName, data) {
        const node = this.analyzer.getNode(nodeName);
        if (!node) return false;

        const routingTable = node.routing_table || [];

        // Check if default route already exists
        const hasDefaultRoute = routingTable.some(r => r.address === 0 && r.netmask === 0);
        if (hasDefaultRoute) return false;

        routingTable.push({
            address: 0,
            netmask: 0,
            interface: data.interface,
            via: 0,
            comment: 'Default route'
        });

        node.routing_table = routingTable;
        return true;
    }

    /**
     * Apply gateway routes
     * @param {string} nodeName - Name of the node
     * @param {Object} data - Suggestion data
     * @returns {boolean} - True if applied successfully
     */
    applyGatewayRoutes(nodeName, data) {
        const node = this.analyzer.getNode(nodeName);
        if (!node) return false;

        const routingTable = node.routing_table || [];
        const interfaces = node.interfaces || [];
        if (interfaces.length === 0) return false;

        const iface = interfaces[0];

        // Add routes to each subnet via gateway
        data.subnets.forEach(subnet => {
            routingTable.push({
                address: subnet.subnet_prefix || 0,
                netmask: subnet.netmask || 8,
                interface: iface.name,
                via: data.gateway.address,
                comment: `Via ${data.gateway.name} to ${subnet.name}`
            });
        });

        node.routing_table = routingTable;
        return true;
    }

    /**
     * Dismiss a suggestion
     * @param {string} nodeName - Name of the node
     * @param {string} suggestionId - ID of the suggestion to dismiss
     */
    dismissSuggestion(nodeName, suggestionId) {
        if (!this.dismissedSuggestions.has(nodeName)) {
            this.dismissedSuggestions.set(nodeName, new Set());
        }
        this.dismissedSuggestions.get(nodeName).add(suggestionId);
    }

    /**
     * Check if a suggestion is dismissed
     * @param {string} nodeName - Name of the node
     * @param {string} suggestionId - ID of the suggestion
     * @returns {boolean} - True if dismissed
     */
    isDismissed(nodeName, suggestionId) {
        if (!this.dismissedSuggestions.has(nodeName)) return false;
        return this.dismissedSuggestions.get(nodeName).has(suggestionId);
    }

    /**
     * Clear dismissed suggestions for a node
     * @param {string} nodeName - Name of the node
     */
    clearDismissed(nodeName) {
        this.dismissedSuggestions.delete(nodeName);
    }

    /**
     * Clear all dismissed suggestions
     */
    clearAllDismissed() {
        this.dismissedSuggestions.clear();
    }
}

