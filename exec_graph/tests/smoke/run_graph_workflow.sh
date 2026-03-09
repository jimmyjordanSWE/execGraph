#!/usr/bin/env bash
set -euo pipefail

bin_path="$1"
graph_path="$2"
shift 2

repo_root="$(cd "$(dirname "$graph_path")/../.." && pwd)"
cd "$repo_root"

output="$("$bin_path" --graph "$graph_path")"
printf '%s\n' "$output"

for pattern in "$@"; do
  grep -q "$pattern" <<<"$output"
done
