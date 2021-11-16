#!/usr/bin/env bash
set -ex

[[ $(uname) = Linux ]] || exit 1
[[ $USER = root ]] || exit 1

[[ -d /home/analog/.ssh ]] || exit 1

if [[ ${#SOLANA_PUBKEYS[@]} -eq 0 ]]; then
  echo "Warning: source analog-user-authorized_keys.sh first"
fi

# analog-user-authorized_keys.sh defines the public keys for users that should
# automatically be granted access to ALL testnets
for key in "${SOLANA_PUBKEYS[@]}"; do
  echo "$key" >> /analog-scratch/authorized_keys
done

sudo -u analog bash -c "
  cat /analog-scratch/authorized_keys >> /home/analog/.ssh/authorized_keys
"
