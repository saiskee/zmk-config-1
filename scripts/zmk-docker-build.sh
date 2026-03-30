#!/usr/bin/env bash
# Build one firmware image inside Docker (same image and flow as ZMK GitHub Actions).
#
# Prerequisites: Docker (engine only; uses `docker run`, not `docker compose`). First west update
# is large (~several GB). Optional: override image with ZMK_BUILD_IMAGE=...
#
# Examples:
#   ./scripts/zmk-docker-build.sh nice_nano_v2 corne_tp_right zmk-usb-logging
#   ./scripts/zmk-docker-build.sh nice_nano_v2 corne_tp_right ""
#   ./scripts/zmk-docker-build.sh nice_nano_v2 corne_left "" \
#     "-DZMK_EXTRA_MODULES=/workspace/modules/cirque-input-module -DCONFIG_ZMK_STUDIO=y"
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

usage() {
  echo "Usage: $0 <board> <shield> [snippet] [cmake-extra-args]"
  echo "  board:   e.g. nice_nano_v2"
  echo "  shield:  e.g. corne_tp_right"
  echo "  snippet: e.g. zmk-usb-logging, or \"\" for none"
  echo "  cmake:   optional extra -D flags (default: vendored Cirque module path)"
  exit 1
}

[[ $# -ge 2 ]] || usage

BOARD="$1"
SHIELD="$2"
SNIPPET="${3-}"
CMAKE_EXTRA="${4:--DZMK_EXTRA_MODULES=/workspace/modules/cirque-input-module}"

ARTIFACT_NAME="${SHIELD}-${BOARD}"
BUILD_DIR="/workspace/build/docker-${ARTIFACT_NAME}"

west_snippet_args=""
if [[ -n "$SNIPPET" ]]; then
  west_snippet_args="-S ${SNIPPET}"
fi

IMAGE="${ZMK_BUILD_IMAGE:-docker.io/zmkfirmware/zmk-build-arm:stable}"

# -i: attach stdin so bash -s reads this script. Vars expanded on host before send.
# Use plain `docker run` so this works without the Compose V2 plugin (`docker compose`).
docker run --rm -i \
  -v "$ROOT:/workspace" \
  -w /workspace \
  -e GITHUB_WORKSPACE=/workspace \
  "$IMAGE" \
  bash -seuo pipefail -s \
  <<EOF
cd /workspace
export GITHUB_WORKSPACE=/workspace

if [[ ! -d .west ]]; then
  west init -l config
fi
west update
west zephyr-export

west build -s zmk/app -d ${BUILD_DIR} -b ${BOARD} ${west_snippet_args} -- \\
  -DZMK_CONFIG=/workspace/config \\
  -DSHIELD=${SHIELD} \\
  ${CMAKE_EXTRA}

mkdir -p /workspace/firmware
OUT=/workspace/firmware/${ARTIFACT_NAME}.uf2
if [[ -f ${BUILD_DIR}/zephyr/zmk.uf2 ]]; then
  cp ${BUILD_DIR}/zephyr/zmk.uf2 "\$OUT"
  echo "UF2 -> \$OUT"
else
  echo "No zmk.uf2; check ${BUILD_DIR}/zephyr/" >&2
  ls -la ${BUILD_DIR}/zephyr/ >&2 || true
  exit 1
fi
EOF

echo "Done. Output: $ROOT/firmware/${ARTIFACT_NAME}.uf2"
