#!/usr/bin/env bash
set -euo pipefail

bin_path="$1"
workflow_path="$2"

repo_root="$(cd "$(dirname "$workflow_path")/../.." && pwd)"
cd "$repo_root"

set +e
output="$($bin_path --workflow "$workflow_path" --emit-events-jsonl 2>&1)"
status=$?
set -e

printf '%s\n' "$output"
test "$status" -ne 0
grep -q 'service-started' <<<"$output"
grep -q '"name":"workflow.started"' <<<"$output"
grep -q '"name":"process.started"' <<<"$output"
grep -q '"name":"process.stop.requested"' <<<"$output"
grep -q '"signal_number":15' <<<"$output"
grep -q '"name":"process.kill.sent"' <<<"$output"
grep -q '"signal_number":9' <<<"$output"
grep -q '"name":"process.killed"' <<<"$output"
grep -q '"name":"workflow.step.failed"' <<<"$output"
grep -q '"name":"workflow.failed"' <<<"$output"
grep -q '"terminal_cause":"timeout"' <<<"$output"
