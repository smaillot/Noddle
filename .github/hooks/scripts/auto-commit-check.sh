#!/usr/bin/env bash
# Auto-commit check: reminds the orchestrator to delegate to Git Ops
# after code-modifying tool calls (edit, create_file).
# Runs as PostToolUse hook — reads JSON from stdin, outputs guidance.

set -euo pipefail

INPUT=$(cat)
TOOL_NAME=$(echo "$INPUT" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('toolName',''))" 2>/dev/null || echo "")

# Only trigger after file-modifying tools
case "$TOOL_NAME" in
  replace_string_in_file|create_file|multi_replace_string_in_file)
    # Count uncommitted changes
    cd "$(git rev-parse --show-toplevel 2>/dev/null || echo .)"
    CHANGED=$(git diff --name-only 2>/dev/null | wc -l)
    STAGED=$(git diff --cached --name-only 2>/dev/null | wc -l)
    UNTRACKED=$(git ls-files --others --exclude-standard 2>/dev/null | wc -l)
    TOTAL=$((CHANGED + STAGED + UNTRACKED))

    if [ "$TOTAL" -gt 10 ]; then
      echo "{\"systemMessage\": \"[Git Ops reminder] $TOTAL uncommitted changes detected. Consider delegating to Git Ops for an atomic commit before continuing.\"}"
    fi
    ;;
esac

exit 0
