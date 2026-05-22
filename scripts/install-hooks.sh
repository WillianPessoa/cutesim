#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOOKS_DIR="$REPO_ROOT/.git/hooks"

if [ ! -d "$HOOKS_DIR" ]; then
    echo "error: .git/hooks not found — run this from inside the repo"
    exit 1
fi

ln -sf "$SCRIPT_DIR/pre-commit" "$HOOKS_DIR/pre-commit"
chmod +x "$SCRIPT_DIR/pre-commit"
echo "installed: pre-commit -> $HOOKS_DIR/pre-commit"
