#!/usr/bin/env bash
#
# Starts an instance of analog-faucet
#
here=$(dirname "$0")

# shellcheck source=multinode-demo/common.sh
source "$here"/common.sh

[[ -f "$ANALOG_CONFIG_DIR"/faucet.json ]] || {
  echo "$ANALOG_CONFIG_DIR/faucet.json not found, create it by running:"
  echo
  echo "  ${here}/setup.sh"
  exit 1
}

set -x
# shellcheck disable=SC2086 # Don't want to double quote $analog_faucet
exec $analog_faucet --keypair "$ANALOG_CONFIG_DIR"/faucet.json "$@"
