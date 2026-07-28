#!/bin/sh
# /usr/libexec/qemu-vms/run-with-log.sh
#
# Usage: run-with-log.sh <logfile> <command> [args...]
#
# Appends the wrapped command's stdout/stderr to <logfile>, then execs it.
# Deliberately does NOT build a shell string out of <command>/[args...] -
# they are received and forwarded as genuine argv elements ("$@"), never
# re-parsed as shell syntax. This matters because some of the arguments
# QEMU is started with (e.g. custom_arg from the LuCI "Custom QEMU
# arguments" field) are admin-supplied free text: if this script instead
# did something like `sh -c "exec $* >>logfile"`, shell metacharacters in
# that text (;, `, $(...), |, &&) would be interpreted as shell syntax
# and executed - not just passed to qemu as inert argv text. Using "$@"
# throughout preserves the same "argv, not a string" trust model as a
# direct (unwrapped) exec.

logfile="$1"
shift

exec "$@" >> "$logfile" 2>&1
