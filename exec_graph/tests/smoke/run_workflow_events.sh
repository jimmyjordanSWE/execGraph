#!/usr/bin/env bash
set -euo pipefail

bin_path="$1"
workflow_path="$2"
failing_workflow_path="$3"

repo_root="$(cd "$(dirname "$workflow_path")/../.." && pwd)"
cd "$repo_root"

success_output="$($bin_path --workflow "$workflow_path" --emit-events-jsonl)"
printf '%s\n' "$success_output"
grep -q '"name":"workflow.started"' <<<"$success_output"
grep -q '"name":"workflow.step.started"' <<<"$success_output"
grep -q '"name":"process.started"' <<<"$success_output"
grep -q '"name":"process.output"' <<<"$success_output"
grep -q '"name":"process.completed"' <<<"$success_output"
grep -q '"name":"workflow.step.completed"' <<<"$success_output"
grep -q '"name":"workflow.completed"' <<<"$success_output"
grep -q '"subject":"workflow.step.1"' <<<"$success_output"
grep -q '"related_subject":"workflow"' <<<"$success_output"
grep -q '"step_count":4' <<<"$success_output"
grep -q '"completed_step_count":4' <<<"$success_output"
grep -q '2 APPLE' <<<"$success_output"

set +e
failure_output="$($bin_path --workflow "$failing_workflow_path" --emit-events-jsonl 2>&1)"
failure_status=$?
set -e
printf '%s\n' "$failure_output"
test "$failure_status" -ne 0
grep -q '"name":"workflow.started"' <<<"$failure_output"
grep -q '"name":"workflow.step.started"' <<<"$failure_output"
grep -q '"name":"process.started"' <<<"$failure_output"
grep -q '"name":"process.output"' <<<"$failure_output"
grep -q '"name":"process.failed"' <<<"$failure_output"
grep -q '"name":"workflow.step.failed"' <<<"$failure_output"
grep -q '"name":"workflow.failed"' <<<"$failure_output"
grep -q '"subject":"workflow.step.1"' <<<"$failure_output"
grep -q '"related_subject":"workflow.step.1"' <<<"$failure_output"
grep -q '"step_count":1' <<<"$failure_output"
grep -q '"completed_step_count":0' <<<"$failure_output"
grep -q '"stream_name":"stderr"' <<<"$failure_output"
grep -q 'cat: definitely_missing_input.txt: No such file or directory' <<<"$failure_output"