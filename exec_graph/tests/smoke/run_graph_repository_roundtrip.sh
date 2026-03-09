#!/usr/bin/env bash
set -euo pipefail

bin_path="$1"
eg_migrate_path="$2"
graph_path="$3"

repo_root="$(cd "$(dirname "$graph_path")/../.." && pwd)"
cd "$repo_root"

tmp_db="$(mktemp /tmp/exec_graph_roundtrip.XXXXXX.db)"
trap 'rm -f "$tmp_db"' EXIT

migrate_output="$("$eg_migrate_path" --db "$tmp_db")"
printf '%s\n' "$migrate_output"
grep -q "applied migrations" <<<"$migrate_output"

save_output="$("$bin_path" --graph "$graph_path" --save-graph --graph-id demo_graph --db "$tmp_db")"
printf '%s\n' "$save_output"
grep -q "stored graph demo_graph at revision 1" <<<"$save_output"

run_output="$("$bin_path" --stored-graph demo_graph --db "$tmp_db")"
printf '%s\n' "$run_output"
grep -q "sink count:" <<<"$run_output"
grep -q "2 APPLE" <<<"$run_output"
grep -q "1 BANANA" <<<"$run_output"
grep -q "2 PEAR" <<<"$run_output"

set +e
conflict_output="$("$bin_path" --graph "$graph_path" --save-graph --graph-id demo_graph --db "$tmp_db" --expected-revision 7 2>&1)"
conflict_status=$?
set -e
printf '%s\n' "$conflict_output"
test "$conflict_status" -ne 0
grep -q "revision_conflict" <<<"$conflict_output"

update_output="$("$bin_path" --graph "$graph_path" --save-graph --graph-id demo_graph --db "$tmp_db" --expected-revision 1)"
printf '%s\n' "$update_output"
grep -q "stored graph demo_graph at revision 2" <<<"$update_output"
