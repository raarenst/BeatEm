#!/usr/bin/env bash
# End-to-end smoke test: server + two clients exchange reciprocal messages.
# Generates fresh keypairs each run, derives paths from the script's own
# location, and exits non-zero if either direction fails.
set -uo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BIN_DIR="$SCRIPT_DIR/../build"
SERVER="$BIN_DIR/beatem_server"
CLIENT="$BIN_DIR/beatem_client"
KEYGEN="$BIN_DIR/beatem_keygen"

for f in "$SERVER" "$CLIENT" "$KEYGEN"; do
    if [ ! -x "$f" ]; then
        echo "Missing binary: $f"
        echo "Run 'make all && make beatem_keygen' from $BIN_DIR first."
        exit 2
    fi
done

WORK=$(mktemp -d -t beatem_smoke.XXXXXX)

cleanup() {
    [ -n "${SERVER_PID:-}" ] && kill "$SERVER_PID" 2>/dev/null
    [ -n "${CA_PID:-}"     ] && kill "$CA_PID"     2>/dev/null
    [ -n "${CB_PID:-}"     ] && kill "$CB_PID"     2>/dev/null
    exec 3>&- 4>&- 2>/dev/null || true
    rm -rf "$WORK"
}
trap cleanup EXIT

# Fresh keypairs every run — never reuse hardcoded keys.
mapfile -t LINES < <("$KEYGEN")
SK_A=${LINES[0]#KEYPAIR_0_SK=}
PK_A=${LINES[1]#KEYPAIR_0_PK=}
SK_B=${LINES[2]#KEYPAIR_1_SK=}
PK_B=${LINES[3]#KEYPAIR_1_PK=}

mkfifo "$WORK/a_in" "$WORK/b_in"
# Hold the write ends open so the clients see EAGAIN, not EOF.
exec 3<>"$WORK/a_in"
exec 4<>"$WORK/b_in"

"$SERVER" > "$WORK/server.log" 2>&1 &
SERVER_PID=$!
sleep 0.5

"$CLIENT" "$SK_A" "$PK_A" "$PK_B" 127.0.0.1 < "$WORK/a_in" > "$WORK/a.log" 2>&1 &
CA_PID=$!
"$CLIENT" "$SK_B" "$PK_B" "$PK_A" 127.0.0.1 < "$WORK/b_in" > "$WORK/b.log" 2>&1 &
CB_PID=$!

# Connect + handshake + first heartbeat tick.
sleep 4

echo "hello from A" >&3
sleep 4

echo "greetings from B" >&4
sleep 4

echo "=== checks ==="
status=0
if grep -q "greetings from B" "$WORK/a.log"; then
    echo "PASS: client A received message from B"
else
    echo "FAIL: client A did not receive B's message"
    echo "--- a.log ---"; cat "$WORK/a.log"
    status=1
fi
if grep -q "hello from A" "$WORK/b.log"; then
    echo "PASS: client B received message from A"
else
    echo "FAIL: client B did not receive A's message"
    echo "--- b.log ---"; cat "$WORK/b.log"
    status=1
fi
exit $status
