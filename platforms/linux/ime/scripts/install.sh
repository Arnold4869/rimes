#!/usr/bin/env bash
# Build and install the Fcitx5 RIMES addon plus reviewed rime-data.
set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
IME_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd -P)
REPO_ROOT=$(CDPATH= cd -- "$IME_ROOT/../../.." && pwd -P)
if [[ ! -f "$REPO_ROOT/scripts/platform-preview/preview.py" ]]; then
    echo "error: could not locate the repository root from $IME_ROOT" >&2
    exit 1
fi

PREFIX="${RIMES_IME_PREFIX:-$HOME/.local}"
BUILD_DIR="${RIMES_IME_BUILD_DIR:-$IME_ROOT/build}"
STAGE_DIR="${RIMES_IME_STAGE_DIR:-}"

usage() {
    cat <<'EOF'
Install the RIMES Fcitx5 addon and its reviewed rime-data tree.

Usage:
  install.sh [--prefix DIR] [--build-dir DIR] [--skip-build]

Default prefix is $HOME/.local. System-wide install:
  sudo RIMES_IME_PREFIX=/usr install.sh
EOF
}

SKIP_BUILD=0
while (($# > 0)); do
    case "$1" in
        --prefix)
            PREFIX=$2
            shift 2
            ;;
        --build-dir)
            BUILD_DIR=$2
            shift 2
            ;;
        --skip-build)
            SKIP_BUILD=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

if (( SKIP_BUILD == 0 )); then
    RIMES_IME_PREFIX="$PREFIX" RIMES_IME_BUILD_DIR="$BUILD_DIR" \
        "$SCRIPT_DIR/build.sh"
fi

DESTDIR="${DESTDIR:-}"
cmake --install "$BUILD_DIR" --prefix "$PREFIX"

if [[ -z "$STAGE_DIR" ]]; then
    STAGE_DIR=$(mktemp -d /tmp/rimes-ime-stage.XXXXXX)
    trap 'rm -rf "$STAGE_DIR"' EXIT
    python3 "$REPO_ROOT/scripts/platform-preview/preview.py" stage \
        --repo-root "$REPO_ROOT" \
        --output-dir "$STAGE_DIR"
fi

DATA_DEST="${DESTDIR}${PREFIX}/share/rimes/data"
mkdir -p "$DATA_DEST"
cp -a "$STAGE_DIR"/. "$DATA_DEST/"

cat <<EOF
Installed the RIMES Fcitx5 addon.
  prefix: $PREFIX
  addon:  $PREFIX/lib/*/fcitx5/rimes.so (or lib64)
  data:   $PREFIX/share/rimes/data
Restart Fcitx5 (fcitx5 -r) and add "RIMES" in the input method list.
User schema/state lives in \$XDG_DATA_HOME/rimes, isolated from fcitx5-rime.
EOF
