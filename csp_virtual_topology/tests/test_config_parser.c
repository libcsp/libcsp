/**
 * @file test_config_parser.c
 * @brief Unit tests for config_parser module
 */

#include "../src/config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    printf("Running test: %s\n", #name); \
    tests_run++; \
    if (test_##name()) { \
        printf("  ✓ PASSED\n"); \
        tests_passed++; \
    } else { \
        printf("  ✗ FAILED\n"); \
    }

/* Test: Parse valid configuration */
static int test_parse_valid_config(void) {
    topology_config_t *topology = parse_topology_config("topologies/examples/linear_3node.json");

    if (!topology) {
        printf("    Failed to parse configuration\n");
        return 0;
    }

    /* Verify topology metadata */
    assert(topology->name != NULL);
    assert(strcmp(topology->name, "Linear 3-Node Topology") == 0);
    assert(topology->csp_version == 2);
    assert(topology->num_nodes == 3);
    assert(topology->num_proxies == 1);

    /* Verify first node */
    assert(strcmp(topology->nodes[0].name, "node1") == 0);
    assert(topology->nodes[0].num_interfaces == 1);
    assert(topology->nodes[0].interfaces[0].address == 1);
    assert(topology->nodes[0].enable_ping == true);

    /* Verify monitoring config */
    assert(topology->monitoring.enabled == true);
    assert(strcmp(topology->monitoring.log_format, "text") == 0);

    free_topology_config(topology);
    return 1;
}

/* Test: Parse non-existent file */
static int test_parse_nonexistent_file(void) {
    topology_config_t *topology = parse_topology_config("nonexistent.json");

    if (topology != NULL) {
        printf("    Should have failed on non-existent file\n");
        free_topology_config(topology);
        return 0;
    }

    return 1;
}

/* Test: Get node by name */
static int test_get_node_config(void) {
    topology_config_t *topology = parse_topology_config("topologies/examples/linear_3node.json");

    if (!topology) {
        printf("    Failed to parse configuration\n");
        return 0;
    }

    /* Find existing node */
    node_config_t *node = get_node_config(topology, "node2");
    if (!node) {
        printf("    Failed to find node2\n");
        free_topology_config(topology);
        return 0;
    }

    assert(strcmp(node->name, "node2") == 0);
    assert(node->interfaces[0].address == 2);

    /* Try to find non-existent node */
    node_config_t *missing = get_node_config(topology, "nonexistent");
    if (missing != NULL) {
        printf("    Should not have found non-existent node\n");
        free_topology_config(topology);
        return 0;
    }

    free_topology_config(topology);
    return 1;
}

/* Test: Generate routing table string */
static int test_generate_rtable_string(void) {
    topology_config_t *topology = parse_topology_config("topologies/examples/linear_3node.json");

    if (!topology) {
        printf("    Failed to parse configuration\n");
        return 0;
    }

    node_config_t *node = get_node_config(topology, "node1");
    if (!node) {
        printf("    Failed to find node1\n");
        free_topology_config(topology);
        return 0;
    }

    char *rtable_str = generate_rtable_string(node);
    if (!rtable_str) {
        printf("    Failed to generate routing table string\n");
        free_topology_config(topology);
        return 0;
    }

    /* Verify format: "0/0 ZMQ0" */
    assert(strstr(rtable_str, "0/0") != NULL);
    assert(strstr(rtable_str, "ZMQ0") != NULL);

    printf("    Generated rtable: %s\n", rtable_str);

    free(rtable_str);
    free_topology_config(topology);
    return 1;
}

/* Test: Validate topology */
static int test_validate_topology(void) {
    topology_config_t *topology = parse_topology_config("topologies/examples/linear_3node.json");

    if (!topology) {
        printf("    Failed to parse configuration\n");
        return 0;
    }

    int result = validate_topology_config(topology);
    if (result != 0) {
        printf("    Validation failed for valid topology\n");
        free_topology_config(topology);
        return 0;
    }

    free_topology_config(topology);
    return 1;
}

/* Main test runner */
int main(int argc, char **argv) {
    printf("\n=== Config Parser Unit Tests ===\n\n");

    /* Change to project root if test is run from build directory */
    if (argc > 1) {
        if (chdir(argv[1]) != 0) {
            printf("Warning: Could not change to directory %s\n", argv[1]);
        }
    }

    TEST(parse_valid_config);
    TEST(parse_nonexistent_file);
    TEST(get_node_config);
    TEST(generate_rtable_string);
    TEST(validate_topology);

    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);

    return (tests_run == tests_passed) ? 0 : 1;
}

