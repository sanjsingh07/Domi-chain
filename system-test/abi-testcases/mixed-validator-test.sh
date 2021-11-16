#!/usr/bin/env bash
#
# Basic empirical ABI system test - can validators on all supported versions of
# Analog talk to each other?
#

set -e
cd "$(dirname "$0")"
SOLANA_ROOT="$(cd ../..; pwd)"

logDir="$PWD"/logs
ledgerDir="$PWD"/config
rm -rf "$ledgerDir" "$logDir"
mkdir -p "$logDir"

baselineVersion=1.1.18  # <-- oldest version we remain compatible with
otherVersions=(
  beta
  edge
)

analogInstallDataDir=$PWD/releases
analogInstallGlobalOpts=(
  --data-dir "$analogInstallDataDir"
  --config "$analogInstallDataDir"/config.yml
  --no-modify-path
)

# Install all the analog versions
bootstrapInstall() {
  declare v=$1
  if [[ ! -h $analogInstallDataDir/active_release ]]; then
    sh "$SOLANA_ROOT"/install/analog-install-init.sh "$v" "${analogInstallGlobalOpts[@]}"
  fi
  export PATH="$analogInstallDataDir/active_release/bin/:$PATH"
}

bootstrapInstall "$baselineVersion"
for v in "${otherVersions[@]}"; do
  analog-install-init "${analogInstallGlobalOpts[@]}" "$v"
  analog -V
done


ORIGINAL_PATH=$PATH
analogInstallUse() {
  declare version=$1
  echo "--- Now using analog $version"
  SOLANA_BIN="$analogInstallDataDir/releases/$version/analog-release/bin"
  export PATH="$SOLANA_BIN:$ORIGINAL_PATH"
}

killSession() {
  tmux kill-session -t abi || true
}

export RUST_BACKTRACE=1

# Start up the bootstrap validator using the baseline version
analogInstallUse "$baselineVersion"
echo "--- Starting $baselineVersion bootstrap validator"
trap 'killSession' INT TERM ERR EXIT
killSession
(
  set -x
  if [[ ! -x baseline-run.sh ]]; then
    curl https://raw.githubusercontent.com/analog-labs/analog/v"$baselineVersion"/run.sh -o baseline-run.sh
    chmod +x baseline-run.sh
  fi
  tmux new -s abi -d " \
    ./baseline-run.sh 2>&1 | tee $logDir/$baselineVersion.log \
  "

  SECONDS=
  while [[ ! -f config/baseline-run/init-completed ]]; do
    sleep 5
    if [[ $SECONDS -gt 60 ]]; then
      echo "Error: validator failed to start"
      exit 1
    fi
  done

  analog --url http://127.0.0.1:8899 show-validators
)

# Ensure all versions can see the bootstrap validator
for v in "${otherVersions[@]}"; do
  analogInstallUse "$v"
  echo "--- Looking for bootstrap validator on gossip"
  (
    set -x
    "$SOLANA_BIN"/analog-gossip spy \
      --entrypoint 127.0.0.1:8001 \
      --num-nodes-exactly 1 \
      --timeout 30
  )
  echo Ok
done

# Start a validator for each version and look for it
#
# Once https://github.com/analog-labs/analog/issues/7738 is resolved, remove
# `--no-snapshot-fetch` when starting the validators
#
nodeCount=1
for v in "${otherVersions[@]}"; do
  nodeCount=$((nodeCount + 1))
  analogInstallUse "$v"
  # start another validator
  ledger="$ledgerDir"/ledger-"$v"
  rm -rf "$ledger"
  echo "--- Looking for $nodeCount validators on gossip"
  (
    set -x
    tmux new-window -t abi -n "$v" " \
      $SOLANA_BIN/analog-validator \
      --ledger $ledger \
      --no-snapshot-fetch \
      --entrypoint 127.0.0.1:8001 \
      -o - 2>&1 | tee $logDir/$v.log \
    "
    "$SOLANA_BIN"/analog-gossip spy \
      --entrypoint 127.0.0.1:8001 \
      --num-nodes-exactly $nodeCount \
      --timeout 30

    # Wait for it to make a snapshot root
    SECONDS=
    while [[ ! -d $ledger/snapshot ]]; do
      sleep 5
      if [[ $SECONDS -gt 60 ]]; then
        echo "Error: validator failed to create a snapshot"
        exit 1
      fi
    done
  )
  echo Ok
done

# Terminate all the validators
killSession

echo
echo Pass
exit 0
