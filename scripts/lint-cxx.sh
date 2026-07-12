#!/usr/bin/env bash
# Q2 — run clang-tidy (+ clazy on Qt TUs) over the project's own translation
# units, using the debug preset's compile_commands.json.
#
# Usage:
#   scripts/lint-cxx.sh              # clang-tidy over all project TUs
#   scripts/lint-cxx.sh --with-clazy # also run clazy on the Qt TUs (see below)
#   scripts/lint-cxx.sh --clazy-only
#   scripts/lint-cxx.sh src/queue.c  # restrict to the given files
#
# BUG-28: clazy-standalone never finishes on the tests/viewer TUs (hangs on
# test_sim_controller.cpp; the app TUs are fine). Until that is understood,
# clazy is OFF by default — enable it explicitly with --with-clazy or
# --clazy-only, ideally on specific files.
#
# Exit status is non-zero if any tool emitted a warning, so CI can gate on it.

set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"
QTC_BIN="/Users/williangomespessoa/Qt/Qt Creator.app/Contents/Resources/libexec/clang/bin"

find_tool() { # name -> first hit in PATH, then Qt Creator's bundled LLVM
    command -v "$1" 2>/dev/null || { [ -x "$QTC_BIN/$1" ] && echo "$QTC_BIN/$1"; }
}

CLANG_TIDY="$(find_tool clang-tidy)"
CLAZY="$(find_tool clazy-standalone)"

RUN_TIDY=1
RUN_CLAZY=0 # BUG-28: opt-in until the tests/viewer hang is fixed
FILES=()
for arg in "$@"; do
    case "$arg" in
    --tidy-only) RUN_CLAZY=0 ;;
    --with-clazy) RUN_CLAZY=1 ;;
    --clazy-only)
        RUN_TIDY=0
        RUN_CLAZY=1
        ;;
    *) FILES+=("$arg") ;;
    esac
done

[ -f "$BUILD_DIR/compile_commands.json" ] || {
    echo "error: $BUILD_DIR/compile_commands.json not found — configure the debug preset first" >&2
    exit 2
}

# Project TUs = everything in the compile db outside _deps/ (GTest) and the
# build tree (moc/qrc generated sources), deduplicated.
all_tus() {
    python3 - "$BUILD_DIR/compile_commands.json" <<'EOF'
import json, sys
seen = set()
for e in json.load(open(sys.argv[1])):
    f = e["file"]
    if "_deps" in f or "/build" in f or f in seen:
        continue
    seen.add(f)
    print(f)
EOF
}

if [ "${#FILES[@]}" -gt 0 ]; then
    TUS=()
    for f in "${FILES[@]}"; do TUS+=("$(cd "$ROOT" && realpath "$f")"); done
else
    TUS=()
    while IFS= read -r f; do TUS+=("$f"); done < <(all_tus)
fi

STATUS=0

if [ "$RUN_TIDY" -eq 1 ]; then
    [ -n "$CLANG_TIDY" ] || { echo "error: clang-tidy not found" >&2; exit 2; }
    echo "== clang-tidy (${#TUS[@]} TUs) =="
    for f in "${TUS[@]}"; do
        OUT="$("$CLANG_TIDY" -p "$BUILD_DIR" --quiet "$f" 2>/dev/null)"
        [ -n "$OUT" ] && { echo "$OUT"; STATUS=1; }
    done
fi

if [ "$RUN_CLAZY" -eq 1 ]; then
    [ -n "$CLAZY" ] || { echo "error: clazy-standalone not found" >&2; exit 2; }
    # clazy only makes sense on the Qt-based TUs (viewer app + its tests)
    echo "== clazy level0,level1 (Qt TUs) =="
    for f in "${TUS[@]}"; do
        case "$f" in
        */apps/viewer/* | */tests/viewer/*) ;;
        *) continue ;;
        esac
        OUT="$("$CLAZY" -p "$BUILD_DIR" --checks=level0,level1 "$f" 2>&1 |
            grep -v '^[0-9]* warnings\{0,1\} generated' | grep -v '^$')"
        [ -n "$OUT" ] && { echo "$OUT"; STATUS=1; }
    done
fi

exit $STATUS
