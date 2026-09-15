#!/bin/sh

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BINARY="$ROOT_DIR/minigit"
TEST_DIR=$(mktemp -d "${TMPDIR:-/tmp}/minigit-tests.XXXXXX")

cleanup()
{
    rm -rf "$TEST_DIR"
}
trap cleanup EXIT HUP INT TERM

pass_count=0
fail_count=0

pass()
{
    pass_count=$((pass_count + 1))
    printf 'ok - %s\n' "$1"
}

fail()
{
    fail_count=$((fail_count + 1))
    printf 'not ok - %s\n' "$1" >&2
    exit 1
}

assert_contains()
{
    text=$1
    expected=$2
    description=$3

    case "$text" in
        *"$expected"*) pass "$description" ;;
        *) fail "$description" ;;
    esac
}

if [ ! -x "$BINARY" ]; then
    printf 'The minigit executable was not found. Run "make build" first.\n' >&2
    exit 1
fi

cd "$TEST_DIR"

if "$BINARY" > usage.txt 2>&1; then
    fail 'running without arguments returns an error'
else
    pass 'running without arguments returns an error'
fi

assert_contains "$(cat usage.txt)" 'Usage:' 'running without arguments prints usage'

if "$BINARY" unknown > invalid.txt 2>&1; then
    fail 'unknown commands return an error'
else
    pass 'unknown commands return an error'
fi

assert_contains "$(cat invalid.txt)" 'Unknown or invalid command: unknown' 'unknown commands print a useful error'

if [ "$("$BINARY" init)" != 'Initialized repository' ]; then
    fail 'init reports success'
else
    pass 'init reports success'
fi

if [ -d .minigit ] && [ -d .minigit/objects ]; then
    pass 'init creates the repository directories'
else
    fail 'init creates the repository directories'
fi

printf 'hello from minigit\n' > sample.txt
"$BINARY" add sample.txt

if [ -f .minigit/index ]; then
    pass 'add creates the index'
else
    fail 'add creates the index'
fi

hash=$(awk 'NF >= 2 { print $2; exit }' .minigit/index)
case "$hash" in
    ????????-????-????-????-????????????) fail 'add writes a SHA-1 object hash' ;;
    '') fail 'add writes a SHA-1 object hash' ;;
    *)
        case "$hash" in
            *[!0123456789abcdef]*) fail 'add writes a hexadecimal object hash' ;;
            *)
                if [ "${#hash}" -eq 40 ]; then
                    pass 'add writes a SHA-1 object hash'
                else
                    fail 'add writes a SHA-1 object hash'
                fi
                ;;
        esac
        ;;
esac

if [ -f ".minigit/objects/$hash" ]; then
    pass 'add stores the compressed object'
else
    fail 'add stores the compressed object'
fi

"$BINARY" cat "$hash" > restored.txt
if cmp -s sample.txt restored.txt || [ "$(cat restored.txt)" = "$(cat sample.txt)" ]; then
    pass 'cat restores the added file content'
else
    fail 'cat restores the added file content'
fi

if grep -q '^sample.txt ' .minigit/index; then
    pass 'add records the filename in the index'
else
    fail 'add records the filename in the index'
fi

printf '%s test(s) passed\n' "$pass_count"
printf '%s test(s) failed\n' "$fail_count"
