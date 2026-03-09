#!/usr/bin/env bash
set -euo pipefail

bin_path="$1"
graph_path="$2"

repo_root="$(cd "$(dirname "$graph_path")/../.." && pwd)"
cd "$repo_root"

success_output="$("$bin_path" --graph "$graph_path" --emit-events-jsonl)"
printf '%s\n' "$success_output"
grep -q '"name":"process.started"' <<<"$success_output"
grep -q '"name":"process.output"' <<<"$success_output"
grep -q '"name":"process.completed"' <<<"$success_output"
grep -q '"subject":"count"' <<<"$success_output"
grep -q '"stream_name":"stdout"' <<<"$success_output"
grep -q 'sink count:' <<<"$success_output"

set +e
failure_output="$("$bin_path" --graph exec_graph/examples/failing_stderr.graph --emit-events-jsonl 2>&1)"
failure_status=$?
set -e
printf '%s\n' "$failure_output"
test "$failure_status" -ne 0
grep -q '"name":"process.started"' <<<"$failure_output"
grep -q '"name":"process.output"' <<<"$failure_output"
grep -q '"name":"process.failed"' <<<"$failure_output"
grep -q '"terminal_cause":"exit_non_zero"' <<<"$failure_output"
grep -q '"stream_name":"stderr"' <<<"$failure_output"
grep -q '"stderr_excerpt":"cat: exec_graph/examples/definitely_missing_input.txt: No such file or directory"' <<<"$failure_output"
