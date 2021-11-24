#![cfg_attr(RUSTC_WITH_SPECIALIZATION, feature(min_specialization))]
#![allow(clippy::integer_arithmetic)]
use analog_sdk::genesis_config::GenesisConfig;

#[deprecated(
    since = "1.8.0",
    note = "Please use `analog_sdk::stake::program::id` or `analog_program::stake::program::id` instead"
)]
pub use analog_sdk::stake::program::{check_id, id};

pub mod config;
pub mod stake_instruction;
pub mod stake_state;

pub fn add_genesis_accounts(genesis_config: &mut GenesisConfig) -> u64 {
    config::add_genesis_account(genesis_config)
}
