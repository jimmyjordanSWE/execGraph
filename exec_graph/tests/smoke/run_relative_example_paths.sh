#!/usr/bin/env bash
set -euo pipefail

bin_path="$1"

repo_root="$(cd "$(dirname "$bin_path")/../.." && pwd)"
cd "$repo_root/exec_graph"

workflow_output="$($bin_path --workflow examples/toy_linux.workflow)"
printf '%s\n' "$workflow_output"
grep -q '2 APPLE' <<<"$workflow_output"
grep -q '1 BANANA' <<<"$workflow_output"
grep -q '2 PEAR' <<<"$workflow_output"

graph_output="$($bin_path --graph examples/toy_process.graph)"
printf '%s\n' "$graph_output"
grep -q 'sink count:' <<<"$graph_output"
grep -q '2 APPLE' <<<"$graph_output"
grep -q '1 BANANA' <<<"$graph_output"
grep -q '2 PEAR' <<<"$graph_output"