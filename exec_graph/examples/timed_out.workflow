# Minimal timeout workflow that proves graceful-stop escalation to force-kill.
timeout_ms=200 graceful_shutdown_ms=100 /usr/bin/env bash ignore_term.sh