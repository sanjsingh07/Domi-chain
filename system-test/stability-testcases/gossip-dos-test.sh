#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"
SOLANA_ROOT="$(cd ../..; pwd)"

logDir="$PWD"/logs
rm -rf "$logDir"
mkdir "$logDir"

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

bootstrapInstall "edge"
analog-install-init --version
analog-install-init edge
analog-gossip --version
analog-dos --version

killall analog-gossip || true
analog-gossip spy --gossip-port 8001 > "$logDir"/gossip.log 2>&1 &
analogGossipPid=$!
echo "analog-gossip pid: $analogGossipPid"
sleep 5
analog-dos --mode gossip --data-type random --data-size 1232 &
dosPid=$!
echo "analog-dos pid: $dosPid"

pass=true

SECONDS=
while ((SECONDS < 600)); do
  if ! kill -0 $analogGossipPid; then
    echo "analog-gossip is no longer running after $SECONDS seconds"
    pass=false
    break
  fi
  if ! kill -0 $dosPid; then
    echo "analog-dos is no longer running after $SECONDS seconds"
    pass=false
    break
  fi
  sleep 1
done

kill $analogGossipPid || true
kill $dosPid || true
wait || true

$pass && echo Pass
