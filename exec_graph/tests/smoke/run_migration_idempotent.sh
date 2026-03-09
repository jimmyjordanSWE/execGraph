#!/usr/bin/env bash
set -euo pipefail

eg_migrate_path="$1"

repo_root="$(cd "$(dirname "$eg_migrate_path")/../.." && pwd)"
cd "$repo_root"

tmp_db="$(mktemp /tmp/exec_graph_migrate.XXXXXX.db)"
trap 'rm -f "$tmp_db"' EXIT

first_output="$($eg_migrate_path --db "$tmp_db")"
printf '%s\n' "$first_output"
grep -q 'discovered=2, applied=2, skipped=0' <<<"$first_output"

second_output="$($eg_migrate_path --db "$tmp_db")"
printf '%s\n' "$second_output"
grep -q 'discovered=2, applied=0, skipped=2' <<<"$second_output"