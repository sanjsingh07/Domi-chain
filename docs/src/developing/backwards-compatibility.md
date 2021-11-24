---
title: Backward Compatibility Policy
---

As the Analog developer ecosystem grows, so does the need for clear expectations around
breaking API and behavior changes affecting applications and tooling built for Analog.
In a perfect world, Analog development could continue at a very fast pace without ever
causing issues for existing developers. However, some compromises will need to be made
and so this document attempts to clarify and codify the process for new releases.

### Expectations

- Analog software releases include APIs, SDKs, and CLI tooling (with a few [exceptions](#exceptions)).
- Analog software releases follow semantic versioning, more details below.
- Software for a `MINOR` version release will be compatible across all software on the
  same `MAJOR` version.

### Deprecation Process

1. In any `PATCH` or `MINOR` release, a feature, API, endpoint, etc. could be marked as deprecated.
2. According to code upgrade difficulty, some features will be remain deprecated for a few release
   cycles.
3. In a future `MAJOR` release, deprecated features will be removed in an incompatible way.

### Release Cadence

The Analog RPC API, Rust SDK, CLI tooling, and BPF Program SDK are all updated and shipped
along with each Analog software release and should always be compatible between `PATCH`
updates of a particular `MINOR` version release.

#### Release Channels

- `edge` software that contains cutting-edge features with no backward compatibility policy
- `beta` software that runs on the Analog Testnet cluster
- `stable` software that run on the Analog Mainnet Beta and Devnet clusters

#### Major Releases (x.0.0)

`MAJOR` version releases (e.g. 2.0.0) may contain breaking changes and removal of previously
deprecated features. Client SDKs and tooling will begin using new features and endpoints
that were enabled in the previous `MAJOR` version.

#### Minor Releases (1.x.0)

New features and proposal implementations are added to _new_ `MINOR` version
releases (e.g. 1.4.0) and are first run on Analog's Testnet cluster. While running
on the testnet, `MINOR` versions are considered to be in the `beta` release channel. After
those changes have been patched as needed and proven to be reliable, the `MINOR` version will
be upgraded to the `stable` release channel and deployed to the Mainnet Beta cluster.

#### Patch Releases (1.0.x)

Low risk features, non-breaking changes, and security and bug fixes are shipped as part
of `PATCH` version releases (e.g. 1.0.11). Patches may be applied to both `beta` and `stable`
release channels.

### RPC API

Patch releases:

- Bug fixes
- Security fixes
- Endpoint / feature deprecation

Minor releases:

- New RPC endpoints and features

Major releases:

- Removal of deprecated features

### Rust Crates

- [`analog-sdk`](https://docs.rs/analog-sdk/) - Rust SDK for creating transactions and parsing account state
- [`analog-program`](https://docs.rs/analog-program/) - Rust SDK for writing programs
- [`analog-client`](https://docs.rs/analog-client/) - Rust client for connecting to RPC API
- [`analog-cli-config`](https://docs.rs/analog-cli-config/) - Rust client for managing Analog CLI config files

Patch releases:

- Bug fixes
- Security fixes
- Performance improvements

Minor releases:

- New APIs

Major releases

- Removal of deprecated APIs
- Backwards incompatible behavior changes

### CLI Tools

Patch releases:

- Bug and security fixes
- Performance improvements
- Subcommand / argument deprecation

Minor releases:

- New subcommands

Major releases:

- Switch to new RPC API endpoints / configuration introduced in the previous major version.
- Removal of deprecated features

### Runtime Features

New Analog runtime features are feature-switched and manually activated. Runtime features
include: the introduction of new native programs, sysvars, and syscalls; and changes to
their behavior. Feature activation is cluster agnostic, allowing confidence to be built on
Testnet before activation on Mainnet-beta.

The release process is as follows:

1. New runtime feature is included in a new release, deactivated by default
2. Once sufficient staked validators upgrade to the new release, the runtime feature switch
   is activated manually with an instruction
3. The feature takes effect at the beginning of the next epoch

### Infrastructure Changes

#### Public API Nodes

Analog provides publicly available RPC API nodes for all developers to use. The Analog team
will make their best effort to communicate any changes to the host, port, rate-limiting behavior,
availability, etc. However, we recommend that developers rely on their own validator nodes to
discourage dependence upon Analog operated nodes.

#### Local cluster scripts and Docker images

Breaking changes will be limited to `MAJOR` version updates. `MINOR` and `PATCH` updates should always
be backwards compatible.

### Exceptions

#### Web3 JavaScript SDK

The Web3.JS SDK also follows semantic versioning specifications but is shipped separately from Analog
software releases.

#### Attack Vectors

If a new attack vector is discovered in existing code, the above processes may be
circumvented in order to rapidly deploy a fix, depending on the severity of the issue.
