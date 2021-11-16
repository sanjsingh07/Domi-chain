---
title: Rust API
---

Analog's Rust crates are [published to crates.io][crates.io] and can be found
[on docs.rs with the "analog-" prefix][docs.rs].

[crates.io]: https://crates.io/search?q=analog-
[docs.rs]: https://docs.rs/releases/search?query=analog-

Some important crates:

- [`analog-program`] &mdash; Imported by programs running on Analog, compiled
  to BPF. This crate contains many fundamental data types and is re-exported from
  [`analog-sdk`], which cannot be imported from a Analog program.

- [`analog-sdk`] &mdash; The basic off-chain SDK, it re-exports
  [`analog-program`] and adds more APIs on top of that. Most Analog programs
  that do not run on-chain will import this.

- [`analog-client`] &mdash; For interacting with a Analog node via the
  [JSON RPC API](jsonrpc-api).

- [`analog-clap-utils`] &mdash; Routines for setting up a CLI, using [`clap`],
  as used by the main Analog CLI.

[`analog-program`]: https://docs.rs/analog-program
[`analog-sdk`]: https://docs.rs/analog-sdk
[`analog-client`]: https://docs.rs/analog-client
[`analog-clap-utils`]: https://docs.rs/analog-clap-utils
[`clap`]: https://docs.rs/clap
