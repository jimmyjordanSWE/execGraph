# Security Status

ExecGraph is a research prototype and **is not a security boundary**.

The current runtime executes commands as the invoking operating-system user. Graph and workflow files must therefore be treated like shell scripts: do not run files from untrusted sources, and do not use the prototype to execute untrusted agent-generated code on a valuable host.

Implemented safety-related behavior is limited to owned process groups, timeouts, graceful termination followed by forced termination, captured output, and structured lifecycle events. The repository does not yet provide container or microVM isolation, filesystem restrictions, network policy, secret mediation, capability enforcement, or a tamper-resistant audit log.

The intended security model is documented in [`docs/agent-execution-ledger.md`](docs/agent-execution-ledger.md).

If this experiment is resumed, security-sensitive failures should be reported privately to the repository owner rather than demonstrated against systems you do not own or control.
