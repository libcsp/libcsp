# CSP Network Simulator - Requirements and Design

## Overview

This document defines comprehensive requirements for a CSP (CubeSat Space Protocol) network simulator with interactive topology editor, traceroute capabilities, and intelligent routing automation.

**Core Capabilities:**
- Interactive visual topology design and editing
- Real-time network simulation and testing
- Traceroute and reachability analysis
- Automatic routing configuration
- Comprehensive validation and error detection

---

## Table of Contents

1. [Address Space Model](#address-space-model)
2. [Topology Editor](#topology-editor)
3. [Traceroute](#traceroute)
4. [Routing Configuration](#routing-configuration)
5. [Smart Routing Automation](#smart-routing-automation)
6. [Design Principles](#design-principles)

---

## 1. Address Space Model

### Address Requirements (AR1-AR3)

**AR1: CSP v2 Address Space**
- The system SHALL support CSP v2 14-bit address space (0-16383)
- The system SHALL support CSP v1 5-bit address space (0-31) for backward compatibility
- Address space selection SHALL be configurable at topology level

**AR2: Hierarchical Addressing**
- The system SHALL support hierarchical address calculation
- Formula: `fully_qualified_address = (base_address << (14 - netmask)) | local_address`
- Subnet configuration SHALL define base_address and netmask
- Interface addresses SHALL be local within subnet range

**AR3: Address Consistency**
- All components SHALL use fully-qualified 14-bit addresses for CSP v2
- Trace events SHALL use fully-qualified addresses
- Ping commands SHALL use fully-qualified addresses
- UI displays SHALL show both local and fully-qualified addresses

**Rationale:**
- CSP v2 uses hierarchical addressing for scalability
- Subnet-based allocation simplifies address management
- Consistency prevents address mismatch bugs

**Common Pitfalls:**
- Mixing raw addresses with fully-qualified addresses
- Using local addresses in routing decisions
- Forgetting to apply base_address offset

---

## 2. Topology Editor

### Editor Core (ER1-ER10)

**ER1: Visual Topology Design**
- The system SHALL provide an interactive canvas for visual topology design
- Users SHALL be able to add, remove, and position nodes graphically
- The system SHALL preserve node positions across sessions

**ER2: Node Management**
- The system SHALL support CRUD operations for nodes
- Each node SHALL have configurable name and description
- Nodes SHALL support multiple network interfaces

**ER3: Interface Management**
- Each node SHALL support multiple network interfaces
- Interfaces SHALL be configurable with: name, subnet assignment, address, netmask
- Interface names SHALL be unique within a node
- Interface netmask SHALL inherit from assigned subnet

**ER4: Subnet Management**
- The system SHALL support CRUD operations for subnets
- Each subnet SHALL have: name, base address, netmask, communication ports
- Subnet address ranges MAY overlap, but the system SHALL emit a WARNING when overlap is detected
- The system SHALL calculate and display address range for each subnet

**ER5: Properties Panel**
- The system SHALL provide a properties panel for editing selected elements
- The panel SHALL adapt based on selection (node, subnet, or topology settings)
- Changes SHALL be applied in real-time to the visual representation

**ER6: Topology Settings**
- The system SHALL support global topology configuration:
  - Topology name and description
  - CSP protocol version (v1 or v2)
  - Packet deduplication mode (all, first, none)

**ER7: Import/Export**
- The system SHALL support exporting topology to JSON format
- The system SHALL support importing topology from JSON format
- Export format SHALL be human-readable and version-controlled

**ER8: Save and Apply**
- The system SHALL support saving topology without restarting simulation
- The system SHALL support applying topology changes and restarting simulation
- The system SHALL create backup files before overwriting existing configurations

**ER9: State Management**
- The system SHALL support undo/redo for all editing operations
- The system SHALL track unsaved changes with visual indicator
- The system SHALL prevent data loss on accidental navigation

**ER10: Visualization**
- Subnets SHALL be visually distinguished by color coding
- Interface connections SHALL display labels with interface name and address
- The system SHALL support zoom, pan, and auto-fit operations

### Dual Mode Operation (DMO1-DMO5)

**DMO1: Read-Only Mode**
- The system SHALL provide a read-only viewing mode for browsing topologies
- Read-only mode SHALL prevent accidental modifications
- Read-only mode SHALL use full-width canvas for maximum visibility
- Read-only mode SHALL be default when loading existing topology

**DMO2: Edit Mode**
- The system SHALL provide an edit mode for topology modification
- Edit mode SHALL support full CRUD operations (create, read, update, delete)
- Edit mode SHALL display properties panel for selected elements
- Edit mode SHALL be default when creating new topology

**DMO3: Mode Switching**
- The system SHALL allow switching between read-only and edit modes
- Mode switch SHALL be accessible via toolbar button
- Mode switch SHALL preserve current topology state
- Mode switch SHALL update UI layout appropriately

**DMO4: Mode Indicators**
- The system SHALL clearly indicate current mode (read-only vs. edit)
- Mode indicator SHALL be visible in toolbar or status bar
- The system SHALL use visual cues (colors, icons) to distinguish modes

**DMO5: Mode-Specific Interactions**
- Read-only mode SHALL show information on hover
- Edit mode SHALL show properties panel on click
- Read-only mode SHALL disable modification operations
- Edit mode SHALL enable all editing operations

### Validation Engine (VR1-VR8)

**VR1: Real-Time Validation**
- The system SHALL validate topology configuration in real-time
- Validation SHALL run automatically on every change
- Validation results SHALL be displayed immediately

**VR2: Address Validation**
- The system SHALL detect address conflicts (duplicate addresses)
- The system SHALL validate addresses are within subnet range
- The system SHALL emit a WARNING when subnet address ranges overlap

**VR3: Interface Validation**
- The system SHALL validate each interface is assigned to exactly one subnet
- The system SHALL validate interface addresses are unique within subnet
- The system SHALL validate interface names are unique within node

**VR4: Routing Validation**
- The system SHALL validate routing table entries reference existing interfaces
- The system SHALL validate gateway addresses are reachable
- The system SHALL detect routing loops and circular dependencies

**VR5: Connectivity Validation**
- The system SHALL detect unreachable nodes
- The system SHALL identify missing routes
- The system SHALL suggest fixes for connectivity issues

**VR6: Error Severity Levels**
- The system SHALL classify validation issues as: ERROR, WARNING, INFO
- ERRORs SHALL prevent topology from being applied
- WARNINGs SHALL allow topology to be applied with user confirmation
- INFO messages SHALL provide optimization suggestions

**VR7: Validation Feedback**
- Validation errors SHALL be displayed in dedicated panel
- Invalid elements SHALL be highlighted in visualization
- The system SHALL provide actionable error messages with suggested fixes

**VR8: Batch Validation**
- The system SHALL support validating entire topology on demand
- Validation report SHALL list all issues grouped by severity
- The system SHALL support exporting validation report

### Auto-Assignment (AA1-AA5)

**AA1: Automatic Address Assignment**
- The system SHALL support automatic address assignment for new interfaces
- Auto-assignment SHALL use next available address in subnet
- Auto-assignment SHALL avoid conflicts with existing addresses

**AA2: Automatic Interface Naming**
- The system SHALL suggest interface names based on subnet
- Interface names SHALL follow consistent naming convention
- Users SHALL be able to override suggested names

**AA3: Automatic Netmask Inheritance**
- Interface netmask SHALL automatically inherit from assigned subnet
- Changing subnet assignment SHALL update interface netmask
- The system SHALL warn if manual netmask differs from subnet

**AA4: Automatic Routing Suggestions**
- The system SHALL suggest routing entries for new interfaces
- Suggestions SHALL be based on topology analysis
- Users SHALL be able to accept, modify, or reject suggestions

**AA5: Smart Defaults**
- New nodes SHALL have sensible default configuration
- New subnets SHALL have non-conflicting default addresses
- Default values SHALL be configurable at system level

---

## 3. Traceroute

### Traceroute Functional Requirements (TR1-TR7)

**TR1: Single Traceroute**
- The system SHALL support traceroute from source node to destination address
- Traceroute SHALL display hop-by-hop path through network
- Each hop SHALL show: node address, interface, timestamp, latency

**TR2: Batch Traceroute**
- The system SHALL support batch traceroute for multiple source-destination pairs
- Batch mode SHALL execute traceroutes sequentially
- Batch mode SHALL clear state between tests
- Batch mode SHALL include inter-test delay (300-500ms recommended)

**TR3: Trace Collection**
- The system SHALL collect CSP trace events from all nodes
- Trace events SHALL include: timestamp, source, destination, hop address, interface
- Trace collector SHALL listen on dedicated UDP port (default: 5570)

**TR4: Trace Correlation**
- The system SHALL correlate trace events to reconstruct packet path
- Correlation SHALL match on: source address, destination address, packet ID
- The system SHALL handle out-of-order trace events

**TR5: Path Visualization**
- The system SHALL display traceroute results in tabular format
- Each hop SHALL show: hop number, node name, address, interface, latency
- The system SHALL highlight failed or incomplete paths

**TR6: Failure Detection**
- The system SHALL detect traceroute failures (timeout, unreachable)
- The system SHALL identify where in path the failure occurred
- The system SHALL suggest potential causes for failures

**TR7: Performance Metrics**
- The system SHALL calculate end-to-end latency
- The system SHALL calculate per-hop latency
- The system SHALL support exporting traceroute results

### Traceroute Non-Functional Requirements (NFR1-NFR3)

**NFR1: Performance**
- Single traceroute SHALL complete within 5 seconds
- Batch traceroute SHALL support at least 100 paths
- Trace collection SHALL handle at least 1000 events/second

**NFR2: Reliability**
- Traceroute SHALL have >95% success rate for valid paths
- The system SHALL handle packet loss gracefully
- The system SHALL recover from temporary network issues

**NFR3: Usability**
- Traceroute interface SHALL be intuitive for non-experts
- Results SHALL be clearly formatted and easy to interpret
- The system SHALL provide help text and examples

---

## 4. Routing Configuration

### Configuration Scope (RR1-RR3)

**RR1: Topology-Level Configuration**
- The system SHALL support topology-wide default routing configuration
- Topology-level settings SHALL apply to all nodes unless overridden
- Topology-level settings SHALL include: deduplication mode, CSP version

**RR2: Node-Level Configuration**
- The system SHALL support per-node routing configuration overrides
- Node-level settings SHALL take precedence over topology-level defaults
- Node-level settings SHALL be optional (inherit from topology if not specified)

**RR3: Configuration Inheritance**
- Nodes without explicit configuration SHALL inherit topology-level settings
- The system SHALL clearly indicate when node uses inherited vs. explicit configuration
- Changes to topology-level settings SHALL propagate to nodes using inherited configuration

### Packet Deduplication (RR4-RR7)

**RR4: Deduplication Modes**
- The system SHALL support the following deduplication modes:
  - **OFF**: No deduplication
  - **FORWARD**: Deduplication on forwarded packets only
  - **INCOMING**: Deduplication on incoming packets only
  - **ALL**: Deduplication on both incoming and forwarded packets

**RR5: Deduplication Purpose**
- Deduplication SHALL prevent routing loops in multi-hop networks
- Deduplication SHALL prevent duplicate packet processing
- The system SHALL track recently seen packets to detect duplicates

**RR6: Deduplication Configuration**
- Deduplication mode SHALL be configurable at topology level
- Deduplication mode SHALL be configurable at node level (override)
- The system SHALL validate deduplication mode values

**RR7: Deduplication Use Cases**
- Routers SHOULD typically use deduplication mode ALL
- End nodes MAY use deduplication mode OFF for performance
- Gateway nodes SHOULD use deduplication mode FORWARD

### Routing Table Management (RR8-RR9)

**RR8: Routing Table Structure**
- Each node SHALL have a routing table with zero or more entries
- Each entry SHALL specify: destination address, netmask, outgoing interface, gateway (optional)
- Routing table SHALL support default route (address=0, netmask=0)

**RR9: Routing Table Validation**
- The system SHALL validate routing entries reference existing interfaces
- The system SHALL validate gateway addresses are reachable
- The system SHALL detect routing loops and conflicts

---

## 5. Smart Routing Automation

### Automatic Router Detection (AA1-AA4)

**AA1: Multi-Subnet Node Detection**
- The system SHALL detect nodes connected to 2 or more subnets
- The system SHALL identify such nodes as potential routers
- The system SHALL offer to auto-configure routing for detected routers

**AA2: Direct Route Generation**
- The system SHALL generate direct routes for locally-connected subnets
- Direct routes SHALL use via=0 (no gateway)
- Direct routes SHALL use subnet's base address and netmask

**AA3: Router Configuration Suggestion**
- The system SHALL suggest router configuration when multi-subnet node is created
- The system SHALL display which subnets the node can route between
- The system SHALL allow user to accept, modify, or dismiss suggestion

**AA4: Router Configuration Content**
- Generated routing table SHALL include one entry per connected subnet
- Each entry SHALL specify: destination address, netmask, outgoing interface
- Each entry SHALL include descriptive comment for clarity

### Reachability Analysis (AA5-AA8)

**AA5: Connectivity Check**
- The system SHALL analyze which nodes can reach which destinations
- The system SHALL identify unreachable nodes
- The system SHALL suggest routing configuration to fix unreachability

**AA6: Missing Route Detection**
- The system SHALL detect when node cannot reach specific destination
- The system SHALL identify which routing entry is missing
- The system SHALL suggest adding missing route

**AA7: Redundant Route Detection**
- The system SHALL detect redundant or overlapping routing entries
- The system SHALL suggest removing or consolidating redundant routes
- The system SHALL preserve user-configured routes

**AA8: Reachability Visualization**
- The system SHALL display reachability matrix (which nodes can reach which)
- The system SHALL highlight unreachable node pairs
- The system SHALL allow clicking unreachable pair to suggest fix

### Multi-Hop Routing (AA9-AA11)

**AA9: Path Discovery**
- The system SHALL analyze topology to discover multi-hop paths
- The system SHALL identify intermediate routers between source and destination
- The system SHALL suggest routing configuration for multi-hop paths

**AA10: Gateway Configuration**
- The system SHALL identify appropriate gateway nodes for indirect routes
- Gateway SHALL be a node on same subnet that can reach destination
- The system SHALL generate routes with via=gateway_address

**AA11: Routing Loop Prevention**
- The system SHALL detect potential routing loops
- The system SHALL warn about circular routing dependencies
- The system SHALL prevent auto-generation of looping routes

---

## 6. Design Principles

### DP1: Implementation Agnostic

**Principle:** Requirements define behavior, not implementation.

**Rationale:**
- Allows flexibility in technology choices
- Supports long-term maintainability
- Enables AI-assisted development

**Guidance:**
- Focus on "what" and "why", not "how"
- Use formal requirement language (SHALL/SHOULD/MAY)
- Avoid references to specific files, functions, or frameworks

### DP2: User-Centric Design

**Principle:** Prioritize user experience and workflow efficiency.

**Rationale:**
- Reduces learning curve
- Minimizes configuration errors
- Supports both novice and expert users

**Guidance:**
- Provide sensible defaults
- Offer automation with transparency
- Support progressive disclosure of complexity
- Include validation and helpful error messages

### DP3: Consistency and Predictability

**Principle:** Maintain consistent behavior across all features.

**Rationale:**
- Reduces cognitive load
- Prevents confusion and errors
- Builds user confidence

**Guidance:**
- Use consistent terminology throughout
- Apply same interaction patterns across features
- Maintain visual consistency in UI
- Provide clear feedback for all actions

### DP4: Safety and Recoverability

**Principle:** Prevent data loss and support error recovery.

**Rationale:**
- Protects user work
- Reduces frustration
- Enables experimentation

**Guidance:**
- Support undo/redo for all operations
- Create backups before destructive operations
- Warn before irreversible actions
- Provide clear indicators for unsaved changes

### DP5: Automation with Control

**Principle:** Automate common tasks while keeping user in control.

**Rationale:**
- Reduces manual work
- Prevents errors through validation
- Maintains user understanding

**Guidance:**
- Show what will be automated before applying
- Allow review and modification of suggestions
- Clearly mark auto-generated vs. manual entries
- Support manual override of all automation

