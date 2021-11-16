//! config for staking
//!  carries variables that the stake program cares about
use bincode::deserialize;
use analog_config_program::{create_config_account, get_config_data};
use analog_sdk::{
    account::{AccountSharedData, ReadableAccount, WritableAccount},
    genesis_config::GenesisConfig,
    instruction::InstructionError,
    keyed_account::KeyedAccount,
    stake::config::{self, Config},
};

#[deprecated(
    since = "1.8.0",
    note = "Please use `analog_sdk::stake::config` or `solana_program::stake::config` instead"
)]
pub use analog_sdk::stake::config::*;

pub fn from<T: ReadableAccount>(account: &T) -> Option<Config> {
    get_config_data(account.data())
        .ok()
        .and_then(|data| deserialize(data).ok())
}

pub fn from_keyed_account(account: &KeyedAccount) -> Result<Config, InstructionError> {
    if !config::check_id(account.unsigned_key()) {
        return Err(InstructionError::InvalidArgument);
    }
    from(&*account.try_account_ref()?).ok_or(InstructionError::InvalidArgument)
}

pub fn create_account(tock: u64, config: &Config) -> AccountSharedData {
    create_config_account(vec![], config, tock)
}

pub fn add_genesis_account(genesis_config: &mut GenesisConfig) -> u64 {
    let mut account = create_config_account(vec![], &Config::default(), 0);
    let tock = genesis_config.rent.minimum_balance(account.data().len());

    account.set_lamports(tock.max(1));

    genesis_config.add_account(config::id(), account);

    tock
}

#[cfg(test)]
mod tests {
    use super::*;
    use analog_sdk::pubkey::Pubkey;
    use std::cell::RefCell;

    #[test]
    fn test() {
        let account = RefCell::new(create_account(0, &Config::default()));
        assert_eq!(from(&account.borrow()), Some(Config::default()));
        assert_eq!(
            from_keyed_account(&KeyedAccount::new(&Pubkey::default(), false, &account)),
            Err(InstructionError::InvalidArgument)
        );
    }
}
