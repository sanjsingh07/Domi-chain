#!/usr/bin/env bash
set -ex

[[ $(uname) = Linux ]] || exit 1
[[ $USER = root ]] || exit 1

if grep -q analog /etc/passwd ; then
  echo "User analog already exists"
else
  adduser analog --gecos "" --disabled-password --quiet
  adduser analog sudo
  adduser analog adm
  echo "analog ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
  id analog

  [[ -r /analog-scratch/id_ecdsa ]] || exit 1
  [[ -r /analog-scratch/id_ecdsa.pub ]] || exit 1

  sudo -u analog bash -c "
    echo 'PATH=\"/home/analog/.cargo/bin:$PATH\"' > /home/analog/.profile
    mkdir -p /home/analog/.ssh/
    cd /home/analog/.ssh/
    cp /analog-scratch/id_ecdsa.pub authorized_keys
    umask 377
    cp /analog-scratch/id_ecdsa id_ecdsa
    echo \"
      Host *
      BatchMode yes
      IdentityFile ~/.ssh/id_ecdsa
      StrictHostKeyChecking no
    \" > config
  "
fi
