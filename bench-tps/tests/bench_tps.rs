#![allow(clippy::integer_arithmetic)]
use serial_test::serial;
use analog_bench_tps::{
    bench::{do_bench_tps, generate_and_fund_keypairs},
    cli::Config,
};
use analog_client::thin_client::create_client;
use analog_core::validator::ValidatorConfig;
use analog_faucet::faucet::run_local_faucet_with_port;
use analog_gossip::cluster_info::VALIDATOR_PORT_RANGE;
use analog_local_cluster::{
    local_cluster::{ClusterConfig, LocalCluster},
    validator_configs::make_identical_validator_configs,
};
use analog_sdk::signature::{Keypair, Signer};
use analog_streamer::socket::SocketAddrSpace;
use std::{
    sync::{mpsc::channel, Arc},
    time::Duration,
};

fn test_bench_tps_local_cluster(config: Config) {
    let native_instruction_processors = vec![];

    analog_logger::setup();
    const NUM_NODES: usize = 1;
    let cluster = LocalCluster::new(
        &mut ClusterConfig {
            node_stakes: vec![999_990; NUM_NODES],
            cluster_tocks: 200_000_000,
            validator_configs: make_identical_validator_configs(
                &ValidatorConfig::default(),
                NUM_NODES,
            ),
            native_instruction_processors,
            ..ClusterConfig::default()
        },
        SocketAddrSpace::Unspecified,
    );

    let faucet_keypair = Keypair::new();
    cluster.transfer(
        &cluster.funding_keypair,
        &faucet_keypair.pubkey(),
        100_000_000,
    );

    let client = Arc::new(create_client(
        (cluster.entry_point_info.rpc, cluster.entry_point_info.tpu),
        VALIDATOR_PORT_RANGE,
    ));

    let (addr_sender, addr_receiver) = channel();
    run_local_faucet_with_port(faucet_keypair, addr_sender, None, 0);
    let faucet_addr = addr_receiver
        .recv_timeout(Duration::from_secs(2))
        .expect("run_local_faucet")
        .expect("faucet_addr");

    let tocks_per_account = 100;

    let keypair_count = config.tx_count * config.keypair_multiplier;
    let keypairs = generate_and_fund_keypairs(
        client.clone(),
        Some(faucet_addr),
        &config.id,
        keypair_count,
        tocks_per_account,
    )
    .unwrap();

    let _total = do_bench_tps(client, config, keypairs);

    #[cfg(not(debug_assertions))]
    assert!(_total > 100);
}

#[test]
#[serial]
fn test_bench_tps_local_cluster_analog() {
    test_bench_tps_local_cluster(Config {
        tx_count: 100,
        duration: Duration::from_secs(10),
        ..Config::default()
    });
}
