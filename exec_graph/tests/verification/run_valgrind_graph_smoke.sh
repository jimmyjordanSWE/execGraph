#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$repo_root"

build/exec_graph/eg_demo_pipeline --graph exec_graph/examples/toy_process.graph >/dev/null
valgrind --error-exitcode=99 --leak-check=full --track-origins=yes \
  build/exec_graph/eg_demo_pipeline --graph exec_graph/examples/toy_process.graph
