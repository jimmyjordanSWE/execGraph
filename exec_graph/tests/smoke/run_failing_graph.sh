#!/usr/bin/env bash
set -euo pipefail

bin_path="$1"
graph_path="$2"

repo_root="$(cd "$(dirname "$graph_path")/../.." && pwd)"
cd "$repo_root"

set +e
output="$("$bin_path" --graph "$graph_path" 2>&1)"
status=$?
set -e

printf '%s\n' "$output"
test "$status" -ne 0
grep -q "node fail failed with exit code" <<<"$output"
grep -q "stderr:" <<<"$output"
grep -q "definitely_missing_input.txt" <<<"$output"
