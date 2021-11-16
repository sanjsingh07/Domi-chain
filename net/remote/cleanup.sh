#!/usr/bin/env bash

set -x
! tmux list-sessions || tmux kill-session
declare sudo=
if sudo true; then
  sudo="sudo -n"
fi

echo "pwd: $(pwd)"
for pid in analog/*.pid; do
  pgid=$(ps opgid= "$(cat "$pid")" | tr -d '[:space:]')
  if [[ -n $pgid ]]; then
    $sudo kill -- -"$pgid"
  fi
done
if [[ -f analog/netem.cfg ]]; then
  analog/scripts/netem.sh delete < analog/netem.cfg
  rm -f analog/netem.cfg
fi
analog/scripts/net-shaper.sh cleanup
for pattern in validator.sh boostrap-leader.sh analog- remote- iftop validator client node; do
  echo "killing $pattern"
  pkill -f $pattern
done
