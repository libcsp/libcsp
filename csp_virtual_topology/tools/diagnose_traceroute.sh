#!/bin/bash
# Traceroute Diagnostic Script
# Runs batch tests via curl and collects diagnostic data to investigate
# missing Delivered events in the trace route web view.
#
# Usage: ./diagnose_traceroute.sh [WEB_PORT] [SRC_NODE]
#   WEB_PORT: Web server port (default: 9999)
#   SRC_NODE: Source node name (default: GroundStation)

set -e

WEB_PORT="${1:-9999}"
SRC_NODE="${2:-GroundStation}"
BASE_URL="http://localhost:${WEB_PORT}"
OUTPUT_DIR="traceroute_diagnosis_$(date +%Y%m%d_%H%M%S)"

echo "=============================================="
echo "Traceroute Diagnostic Script"
echo "=============================================="
echo "Web Server: ${BASE_URL}"
echo "Source Node: ${SRC_NODE}"
echo "Output Dir: ${OUTPUT_DIR}"
echo "=============================================="

mkdir -p "${OUTPUT_DIR}"

# Step 1: Check web server is running
echo -e "\n[1/6] Checking web server connectivity..."
if ! curl -s "${BASE_URL}/api/topology" > "${OUTPUT_DIR}/topology.json" 2>&1; then
    echo "ERROR: Cannot connect to web server at ${BASE_URL}"
    echo "Make sure topology_launcher.py is running (web server starts automatically)"
    exit 1
fi
echo "OK - Topology has $(jq '.nodes | length' "${OUTPUT_DIR}/topology.json") nodes"

# Step 2: Get list of all node addresses
echo -e "\n[2/6] Extracting node addresses from topology..."
jq -r '.nodes[] | "\(.name): \(.interfaces[] | "\(.name)=\(.address)")"' "${OUTPUT_DIR}/topology.json" > "${OUTPUT_DIR}/node_addresses.txt"
echo "Node addresses saved to ${OUTPUT_DIR}/node_addresses.txt"
cat "${OUTPUT_DIR}/node_addresses.txt"

# Step 3: Clear any existing traces
echo -e "\n[3/6] Clearing existing traces..."
curl -s -X POST "${BASE_URL}/api/traceroute/clear" | jq .

# Step 4: Start batch traceroute
echo -e "\n[4/6] Starting batch traceroute from ${SRC_NODE}..."
BATCH_RESPONSE=$(curl -s -X POST "${BASE_URL}/api/traceroute/batch" \
    -H "Content-Type: application/json" \
    -d "{\"src_node\": \"${SRC_NODE}\"}")
echo "${BATCH_RESPONSE}" | jq .

if [ "$(echo "${BATCH_RESPONSE}" | jq -r '.success')" != "true" ]; then
    echo "ERROR: Failed to start batch traceroute"
    echo "${BATCH_RESPONSE}" | jq .
    exit 1
fi

TOTAL_TESTS=$(echo "${BATCH_RESPONSE}" | jq -r '.total_tests')
echo "Total tests to run: ${TOTAL_TESTS}"

# Step 5: Poll for progress until complete
echo -e "\n[5/6] Waiting for batch test to complete..."
while true; do
    PROGRESS=$(curl -s "${BASE_URL}/api/traceroute/batch/progress")
    RUNNING=$(echo "${PROGRESS}" | jq -r '.running')
    CURRENT=$(echo "${PROGRESS}" | jq -r '.current')
    CURRENT_TEST=$(echo "${PROGRESS}" | jq -r '.current_test')

    if [ "${RUNNING}" == "false" ]; then
        echo -e "\nBatch test complete!"
        break
    fi

    echo -ne "\rProgress: ${CURRENT}/${TOTAL_TESTS} - ${CURRENT_TEST}          "
    sleep 0.5
done

# Save final progress/results
curl -s "${BASE_URL}/api/traceroute/batch/progress" > "${OUTPUT_DIR}/batch_results.json"
echo "Results saved to ${OUTPUT_DIR}/batch_results.json"

# Step 6: Analyze results
echo -e "\n[6/6] Analyzing results..."
echo ""
echo "================================================================================"
echo "REACHABILITY MATRIX SUMMARY"
echo "================================================================================"

# Extract and display reachability matrix
jq -r '.results[] | "\(.src_node):\(.src_iface)(\(.src_addr)) -> \(.dst_node):\(.dst_iface)(\(.dst_addr)) | Reachable: \(.reachable) | Hops: \(.hop_count)"' \
    "${OUTPUT_DIR}/batch_results.json" | tee "${OUTPUT_DIR}/reachability_matrix.txt"

echo ""
echo "================================================================================"
echo "FAILED TESTS (No Delivered Event)"
echo "================================================================================"

# Find tests where reachable=false
FAILED_TESTS=$(jq -c '.results[] | select(.reachable == false)' "${OUTPUT_DIR}/batch_results.json")

if [ -z "${FAILED_TESTS}" ]; then
    echo "All tests passed! All packets were delivered."
else
    echo "${FAILED_TESTS}" | while read -r test; do
        SRC_ADDR=$(echo "${test}" | jq -r '.src_addr')
        DST_ADDR=$(echo "${test}" | jq -r '.dst_addr')
        SPORT=$(echo "${test}" | jq -r '.sport // empty')
        SRC_NODE_NAME=$(echo "${test}" | jq -r '.src_node')
        DST_NODE_NAME=$(echo "${test}" | jq -r '.dst_node')

        echo ""
        echo "--- ${SRC_NODE_NAME}(${SRC_ADDR}) -> ${DST_NODE_NAME}(${DST_ADDR}) ---"

        # Get detailed trace
        TRACE_URL="${BASE_URL}/api/trace/detail?src=${SRC_ADDR}&dst=${DST_ADDR}"
        [ -n "${SPORT}" ] && TRACE_URL="${TRACE_URL}&sport=${SPORT}"

        TRACE=$(curl -s "${TRACE_URL}")
        echo "${TRACE}" | jq -r '.hops[] | "  Node \(.node_addr) [\(.iface)]: action=\(.action), route_code=\(.route_code)"' 2>/dev/null || echo "  No trace data available"

        # Save individual trace
        echo "${TRACE}" > "${OUTPUT_DIR}/trace_${SRC_ADDR}_to_${DST_ADDR}.json"
    done
fi

echo ""
echo "================================================================================"
echo "ROUTE CODE REFERENCE"
echo "================================================================================"
echo "Action codes: 0=RECEIVED, 1=FORWARDED, 2=DROPPED"
echo ""
echo "Success route_codes (expect action=1 FORWARDED):"
echo "  0 = Received at node"
echo "  1 = Loopback routing"
echo "  2 = Subnet match routing"
echo "  3 = Routing table match"
echo "  4 = Default interface routing"
echo "  5 = DELIVERED to application <-- This is what we want at destination!"
echo ""
echo "Drop route_codes (action=2 DROPPED):"
echo "  10 = Duplicate packet"
echo "  18 = No socket listening (D09)"
echo "  19 = Socket queue full (D10)"
echo "  23 = No route found (D14)"
echo "  24-29 = Split-horizon drops (D15-D20)"

echo ""
echo "================================================================================"
echo "TRACE COLLECTOR LOG LOCATION"
echo "================================================================================"
TOPOLOGY_NAME=$(jq -r '.name' "${OUTPUT_DIR}/topology.json" | tr ' ' '_' | tr -cd '[:alnum:]_-')
echo "Check trace collector logs in: ./${TOPOLOGY_NAME}/traces/"
echo ""
echo "To analyze raw trace events:"
echo "  ./tools/analyze_trace_log.py ${TOPOLOGY_NAME}/traces/trace_session_*.jsonl"
echo ""
echo "To see what the destination node logged:"
echo "  grep TRACE ${TOPOLOGY_NAME}/<destination_node>.log"
echo ""
echo "All diagnostic files saved to: ${OUTPUT_DIR}/"
ls -la "${OUTPUT_DIR}/"

echo ""
echo "================================================================================"
echo "INVESTIGATION CHECKLIST"
echo "================================================================================"
echo "If Delivered events are missing, check:"
echo "  1. Does the destination node receive the packet? (action=0, route_code=0)"
echo "  2. Is the packet being forwarded instead? (action=1, not route_code=5)"
echo "  3. Is there a routing config issue? (packet never reaches destination)"
echo "  4. Check node logs for TRACE_DEBUG messages"
echo "  5. Verify FTRACE flag is set in packet (flags should have 0x40 bit)"
echo "================================================================================"

