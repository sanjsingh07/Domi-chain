#!/usr/bin/env bash
set -ex

cd "$(dirname "$0")"

# shellcheck source=net/scripts/analog-user-authorized_keys.sh
source analog-user-authorized_keys.sh

# analog-user-authorized_keys.sh defines the public keys for users that should
# automatically be granted access to ALL datacenter nodes.
for i in "${!ANALOG_USERS[@]}"; do
  echo "environment=\"ANALOG_USER=${ANALOG_USERS[i]}\" ${ANALOG_PUBKEYS[i]}"
done

