---
title: Connecting to a Cluster
---

See [Analog Clusters](../clusters.md) for general information about the
available clusters.

## Configure the command-line tool

You can check what cluster the Analog command-line tool (CLI) is currently targeting by
running the following command:

```bash
analog config get
```

Use `analog config set` command to target a particular cluster. After setting
a cluster target, any future subcommands will send/receive information from that
cluster.

For example to target the Devnet cluster, run:

```bash
analog config set --url https://api.devnet.analog.com
```

## Ensure Versions Match

Though not strictly necessary, the CLI will generally work best when its version
matches the software version running on the cluster. To get the locally-installed
CLI version, run:

```bash
analog --version
```

To get the cluster version, run:

```bash
analog cluster-version
```

Ensure the local CLI version is greater than or equal to the cluster version.
