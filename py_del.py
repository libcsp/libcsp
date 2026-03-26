#!/usr/bin/env python3
import os
import sys

# List of files to delete
files_to_delete = [
    ".codespell-ignore",
    "contrib/scripts/set-version.sh",
    "contrib/windows/windows_queue.c",
    "contrib/windows/windows_queue.h",
    "doc/api/drivers/can_zephyr_h.rst",
    "doc/Dockerfile",
    "doc/samples/posix/simple-send-canbus.md",
    "doc/samples/posix/simple-send-udp.md",
    "doc/samples/posix/simple-send-usart.md",
    "doc/samples/posix/simple-send-zmq.md",
    "doc/samples/posix/simple-sfp-send-recv.md",
    "doc/scsp_sfp_server_client.c",
    ".github/workflows/check-merge-commits.yml",
    ".github/workflows/codespell.yml",
    "samples/posix/simple-send-udp/CMakeLists.txt",
    "samples/posix/simple-send-udp/README.md",
    "samples/posix/simple-send-udp/src/main.c",
    "samples/posix/simple-send-zmq/CMakeLists.txt",
    "samples/posix/simple-send-zmq/README.md",
    "samples/posix/simple-send-zmq/src/main.c",
    "samples/posix/simple-sfp-send-recv/CMakeLists.txt",
    "samples/posix/simple-sfp-send-recv/README.md",
    "samples/posix/simple-sfp-send-recv/src/main.c",
    "src/csp_buffer_private.h",
    "src/drivers/can/can_socketcan.c",
    "src/drivers/can/can_zephyr.c",
    "zephyr/CMakeLists.txt",
    "zephyr/Kconfig",
    "zephyr/module.yml",
]

def main():
    print("WARNING: This script will delete the following files:")
    for file in files_to_delete:
        print(f"  - {file}")
    
    response = input("\nAre you sure you want to continue? (yes/no): ")
    if response.lower() != "yes":
        print("Operation cancelled.")
        sys.exit(0)
    
    deleted_count = 0
    not_found_count = 0
    error_count = 0
    
    for file_path in files_to_delete:
        try:
            if os.path.exists(file_path):
                os.remove(file_path)
                print(f"✓ Deleted: {file_path}")
                deleted_count += 1
            else:
                print(f"⊘ Not found: {file_path}")
                not_found_count += 1
        except Exception as e:
            print(f"✗ Error deleting {file_path}: {e}")
            error_count += 1
    
    print(f"\n--- Summary ---")
    print(f"Deleted: {deleted_count}")
    print(f"Not found: {not_found_count}")
    print(f"Errors: {error_count}")

if __name__ == "__main__":
    main()
