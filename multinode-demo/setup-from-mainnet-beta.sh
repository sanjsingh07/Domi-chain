#!/usr/bin/env bash

here=$(dirname "$0")
# shellcheck source=multinode-demo/common.sh
source "$here"/common.sh

set -e

rm -rf "$ANALOG_CONFIG_DIR"/latest-mainnet-beta-snapshot
mkdir -p "$ANALOG_CONFIG_DIR"/latest-mainnet-beta-snapshot
(
  cd "$ANALOG_CONFIG_DIR"/latest-mainnet-beta-snapshot || exit 1
  set -x
  wget http://api.mainnet-beta.analog.com/genesis.tar.bz2
  wget --trust-server-names http://api.mainnet-beta.analog.com/snapshot.tar.bz2
)

snapshot=$(ls "$ANALOG_CONFIG_DIR"/latest-mainnet-beta-snapshot/snapshot-[0-9]*-*.tar.zst)
if [[ -z $snapshot ]]; then
  echo Error: Unable to find latest snapshot
  exit 1
fi

if [[ ! $snapshot =~ snapshot-([0-9]*)-.*.tar.zst ]]; then
  echo Error: Unable to determine snapshot slot for "$snapshot"
  exit 1
fi

snapshot_slot="${BASH_REMATCH[1]}"

rm -rf "$ANALOG_CONFIG_DIR"/bootstrap-validator
mkdir -p "$ANALOG_CONFIG_DIR"/bootstrap-validator


# Create genesis ledger
if [[ -r $FAUCET_KEYPAIR ]]; then
  cp -f "$FAUCET_KEYPAIR" "$ANALOG_CONFIG_DIR"/faucet.json
else
  $analog_keygen new --no-passphrase -fso "$ANALOG_CONFIG_DIR"/faucet.json
fi

if [[ -f $BOOTSTRAP_VALIDATOR_IDENTITY_KEYPAIR ]]; then
  cp -f "$BOOTSTRAP_VALIDATOR_IDENTITY_KEYPAIR" "$ANALOG_CONFIG_DIR"/bootstrap-validator/identity.json
else
  $analog_keygen new --no-passphrase -so "$ANALOG_CONFIG_DIR"/bootstrap-validator/identity.json
fi

$analog_keygen new --no-passphrase -so "$ANALOG_CONFIG_DIR"/bootstrap-validator/vote-account.json
$analog_keygen new --no-passphrase -so "$ANALOG_CONFIG_DIR"/bootstrap-validator/stake-account.json

$analog_ledger_tool create-snapshot \
  --ledger "$ANALOG_CONFIG_DIR"/latest-mainnet-beta-snapshot \
  --faucet-pubkey "$ANALOG_CONFIG_DIR"/faucet.json \
  --faucet-lamports 500000000000000000 \
  --bootstrap-validator "$ANALOG_CONFIG_DIR"/bootstrap-validator/identity.json \
                        "$ANALOG_CONFIG_DIR"/bootstrap-validator/vote-account.json \
                        "$ANALOG_CONFIG_DIR"/bootstrap-validator/stake-account.json \
  --hashes-per-tick sleep \
  "$snapshot_slot" "$ANALOG_CONFIG_DIR"/bootstrap-validator

$analog_ledger_tool modify-genesis \
  --ledger "$ANALOG_CONFIG_DIR"/latest-mainnet-beta-snapshot \
  --hashes-per-tick sleep \
  "$ANALOG_CONFIG_DIR"/bootstrap-validator
