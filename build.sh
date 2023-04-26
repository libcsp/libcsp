#!/bin/bash
cmake -DCMAKE_BUILD_TYPE=Debug -GNinja -B builddir
cd builddir
ninja csp_server_client
