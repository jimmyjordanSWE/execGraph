#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$repo_root"

cd exec_graph
cmake --preset tsan
cd "$repo_root"
cmake --build build/exec_graph_tsan
ctest --test-dir build/exec_graph_tsan --output-on-failure
