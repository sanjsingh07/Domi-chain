#!/usr/bin/env bash
#
# Builds known downstream projects against local analog source
#

set -e
cd "$(dirname "$0")"/..
source ci/_
source scripts/read-cargo-variable.sh

analog_ver=$(readCargoVariable version sdk/Cargo.toml)
analog_dir=$PWD
cargo="$analog_dir"/cargo
cargo_build_bpf="$analog_dir"/cargo-build-bpf
cargo_test_bpf="$analog_dir"/cargo-test-bpf

mkdir -p target/downstream-projects
cd target/downstream-projects

update_analog_dependencies() {
  declare tomls=()
  while IFS='' read -r line; do tomls+=("$line"); done < <(find "$1" -name Cargo.toml)

  sed -i -e "s#\(solana-program= \"\)[^\"]*\(\"\)#\1=$analog_ver\2#g" "${tomls[@]}" || return $?
  sed -i -e "s#\(analog-program-test = \"\)[^\"]*\(\"\)#\1=$analog_ver\2#g" "${tomls[@]}" || return $?
  sed -i -e "s#\(analog-sdk = \"\).*\(\"\)#\1=$analog_ver\2#g" "${tomls[@]}" || return $?
  sed -i -e "s#\(analog-sdk = { version = \"\)[^\"]*\(\"\)#\1=$analog_ver\2#g" "${tomls[@]}" || return $?
  sed -i -e "s#\(analog-client = \"\)[^\"]*\(\"\)#\1=$analog_ver\2#g" "${tomls[@]}" || return $?
  sed -i -e "s#\(analog-client = { version = \"\)[^\"]*\(\"\)#\1=$analog_ver\2#g" "${tomls[@]}" || return $?
}

patch_crates_io() {
  cat >> "$1" <<EOF
[patch.crates-io]
analog-client = { path = "$analog_dir/client" }
solana-program= { path = "$analog_dir/sdk/program" }
analog-program-test = { path = "$analog_dir/program-test" }
analog-sdk = { path = "$analog_dir/sdk" }
EOF
}

example_helloworld() {
  (
    set -x
    rm -rf example-helloworld
    git clone https://github.com/analog-labs/example-helloworld.git
    cd example-helloworld

    update_analog_dependencies src/program-rust
    patch_crates_io src/program-rust/Cargo.toml
    echo "[workspace]" >> src/program-rust/Cargo.toml

    $cargo_build_bpf \
      --manifest-path src/program-rust/Cargo.toml

    # TODO: Build src/program-c/...
  )
}

spl() {
  (
    set -x
    rm -rf spl
    git clone https://github.com/analog-labs/analog-program-library.git spl
    cd spl

    ./patch.crates-io.sh "$analog_dir"

    $cargo build
    $cargo test
    $cargo_build_bpf
    $cargo_test_bpf
  )
}

serum_dex() {
  (
    set -x
    rm -rf serum-dex
    git clone https://github.com/project-serum/serum-dex.git
    cd serum-dex

    update_analog_dependencies .
    patch_crates_io Cargo.toml
    patch_crates_io dex/Cargo.toml
    cat >> dex/Cargo.toml <<EOF
[workspace]
exclude = [
    "crank",
    "permissioned",
]
EOF
    $cargo build

    $cargo_build_bpf \
      --manifest-path dex/Cargo.toml --no-default-features --features program

    $cargo test \
      --manifest-path dex/Cargo.toml --no-default-features --features program
  )
}


_ example_helloworld
_ spl
_ serum_dex
