#!/bin/bash
cmake -GNinja -B builddir
cd builddir
ninja csp_server_client
