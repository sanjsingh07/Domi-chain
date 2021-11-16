#!/usr/bin/env bash
set -ex

cd "$(dirname "$0")"

docker build -t analoglabs/rust .

read -r rustc version _ < <(docker run analoglabs/rust rustc --version)
[[ $rustc = rustc ]]
docker tag analoglabs/rust:latest analoglabs/rust:"$version"
docker push analoglabs/rust:"$version"
docker push analoglabs/rust:latest
