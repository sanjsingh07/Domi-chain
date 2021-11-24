#![cfg(feature = "full")]

use crate::{
    account::{ReadableAccount, WritableAccount},
    account_utils::State as AccountUtilsState,
    feature_set, ic_msg,
    keyed_account::KeyedAccount,
    nonce_account::create_account,
    process_instruction::InvokeContext,
};
use analog_program::{
    instruction::{checked_add, InstructionError},
    nonce::{self, state::Versions, State},
    pubkey::Pubkey,
    system_instruction::{nonce_to_instruction_error, NonceError},
    sysvar::rent::Rent,
};
use std::collections::HashSet;

pub trait NonceKeyedAccount {
    fn advance_nonce_account(
        &self,
        signers: &HashSet<Pubkey>,
        invoke_context: &dyn InvokeContext,
    ) -> Result<(), InstructionError>;
    fn withdraw_nonce_account(
        &self,
        tocks: u64,
        to: &KeyedAccount,
        rent: &Rent,
        signers: &HashSet<Pubkey>,
        invoke_context: &dyn InvokeContext,
    ) -> Result<(), InstructionError>;
    fn initialize_nonce_account(
        &self,
        nonce_authority: &Pubkey,
        rent: &Rent,
        invoke_context: &dyn InvokeContext,
    ) -> Result<(), InstructionError>;
    fn authorize_nonce_account(
        &self,
        nonce_authority: &Pubkey,
        signers: &HashSet<Pubkey>,
        invoke_context: &dyn InvokeContext,
    ) -> Result<(), InstructionError>;
}

impl<'a> NonceKeyedAccount for KeyedAccount<'a> {
    fn advance_nonce_account(
        &self,
        signers: &HashSet<Pubkey>,
        invoke_context: &dyn InvokeContext,
    ) -> Result<(), InstructionError> {
        let merge_nonce_error_into_system_error = invoke_context
            .is_feature_active(&feature_set::merge_nonce_error_into_system_error::id());
        let state = AccountUtilsState::<Versions>::state(self)?.convert_to_current();
        match state {
            State::Initialized(data) => {
                if !signers.contains(&data.authority) {
                    ic_msg!(
                        invoke_context,
                        "Advance nonce account: Account {} must be a signer",
                        data.authority
                    );
                    return Err(InstructionError::MissingRequiredSignature);
                }
                let recent_blockhash = *invoke_context.get_blockhash();
                if data.blockhash == recent_blockhash {
                    ic_msg!(
                        invoke_context,
                        "Advance nonce account: nonce can only advance once per slot"
                    );
                    return Err(nonce_to_instruction_error(
                        NonceError::NotExpired,
                        merge_nonce_error_into_system_error,
                    ));
                }

                let new_data = nonce::state::Data {
                    blockhash: recent_blockhash,
                    fee_calculator: invoke_context.get_fee_calculator().clone(),
                    ..data
                };
                self.set_state(&Versions::new_current(State::Initialized(new_data)))
            }
            _ => {
                ic_msg!(
                    invoke_context,
                    "Advance nonce account: Account {} state is invalid",
                    self.unsigned_key()
                );
                Err(nonce_to_instruction_error(
                    NonceError::BadAccountState,
                    merge_nonce_error_into_system_error,
                ))
            }
        }
    }

    fn withdraw_nonce_account(
        &self,
        tocks: u64,
        to: &KeyedAccount,
        rent: &Rent,
        signers: &HashSet<Pubkey>,
        invoke_context: &dyn InvokeContext,
    ) -> Result<(), InstructionError> {
        let merge_nonce_error_into_system_error = invoke_context
            .is_feature_active(&feature_set::merge_nonce_error_into_system_error::id());
        let signer = match AccountUtilsState::<Versions>::state(self)?.convert_to_current() {
            State::Uninitialized => {
                if tocks > self.tocks()? {
                    ic_msg!(
                        invoke_context,
                        "Withdraw nonce account: insufficient tocks {}, need {}",
                        self.tocks()?,
                        tocks,
                    );
                    return Err(InstructionError::InsufficientFunds);
                }
                *self.unsigned_key()
            }
            State::Initialized(ref data) => {
                if tocks == self.tocks()? {
                    if data.blockhash == *invoke_context.get_blockhash() {
                        ic_msg!(
                            invoke_context,
                            "Withdraw nonce account: nonce can only advance once per slot"
                        );
                        return Err(nonce_to_instruction_error(
                            NonceError::NotExpired,
                            merge_nonce_error_into_system_error,
                        ));
                    }
                    self.set_state(&Versions::new_current(State::Uninitialized))?;
                } else {
                    let min_balance = rent.minimum_balance(self.data_len()?);
                    let amount = checked_add(tocks, min_balance)?;
                    if amount > self.tocks()? {
                        ic_msg!(
                            invoke_context,
                            "Withdraw nonce account: insufficient tocks {}, need {}",
                            self.tocks()?,
                            amount,
                        );
                        return Err(InstructionError::InsufficientFunds);
                    }
                }
                data.authority
            }
        };

        if !signers.contains(&signer) {
            ic_msg!(
                invoke_context,
                "Withdraw nonce account: Account {} must sign",
                signer
            );
            return Err(InstructionError::MissingRequiredSignature);
        }

        let nonce_balance = self.try_account_ref_mut()?.tocks();
        self.try_account_ref_mut()?.set_tocks(
            nonce_balance
                .checked_sub(tocks)
                .ok_or(InstructionError::ArithmeticOverflow)?,
        );
        let to_balance = to.try_account_ref_mut()?.tocks();
        to.try_account_ref_mut()?.set_tocks(
            to_balance
                .checked_add(tocks)
                .ok_or(InstructionError::ArithmeticOverflow)?,
        );

        Ok(())
    }

    fn initialize_nonce_account(
        &self,
        nonce_authority: &Pubkey,
        rent: &Rent,
        invoke_context: &dyn InvokeContext,
    ) -> Result<(), InstructionError> {
        let merge_nonce_error_into_system_error = invoke_context
            .is_feature_active(&feature_set::merge_nonce_error_into_system_error::id());
        match AccountUtilsState::<Versions>::state(self)?.convert_to_current() {
            State::Uninitialized => {
                let min_balance = rent.minimum_balance(self.data_len()?);
                if self.tocks()? < min_balance {
                    ic_msg!(
                        invoke_context,
                        "Initialize nonce account: insufficient tocks {}, need {}",
                        self.tocks()?,
                        min_balance
                    );
                    return Err(InstructionError::InsufficientFunds);
                }
                let data = nonce::state::Data {
                    authority: *nonce_authority,
                    blockhash: *invoke_context.get_blockhash(),
                    fee_calculator: invoke_context.get_fee_calculator().clone(),
                };
                self.set_state(&Versions::new_current(State::Initialized(data)))
            }
            _ => {
                ic_msg!(
                    invoke_context,
                    "Initialize nonce account: Account {} state is invalid",
                    self.unsigned_key()
                );
                Err(nonce_to_instruction_error(
                    NonceError::BadAccountState,
                    merge_nonce_error_into_system_error,
                ))
            }
        }
    }

    fn authorize_nonce_account(
        &self,
        nonce_authority: &Pubkey,
        signers: &HashSet<Pubkey>,
        invoke_context: &dyn InvokeContext,
    ) -> Result<(), InstructionError> {
        let merge_nonce_error_into_system_error = invoke_context
            .is_feature_active(&feature_set::merge_nonce_error_into_system_error::id());
        match AccountUtilsState::<Versions>::state(self)?.convert_to_current() {
            State::Initialized(data) => {
                if !signers.contains(&data.authority) {
                    ic_msg!(
                        invoke_context,
                        "Authorize nonce account: Account {} must sign",
                        data.authority
                    );
                    return Err(InstructionError::MissingRequiredSignature);
                }
                let new_data = nonce::state::Data {
                    authority: *nonce_authority,
                    ..data
                };
                self.set_state(&Versions::new_current(State::Initialized(new_data)))
            }
            _ => {
                ic_msg!(
                    invoke_context,
                    "Authorize nonce account: Account {} state is invalid",
                    self.unsigned_key()
                );
                Err(nonce_to_instruction_error(
                    NonceError::BadAccountState,
                    merge_nonce_error_into_system_error,
                ))
            }
        }
    }
}

/// Convenience function for working with keyed accounts in tests
pub fn with_test_keyed_account<F>(tocks: u64, signer: bool, f: F)
where
    F: Fn(&KeyedAccount),
{
    let pubkey = Pubkey::new_unique();
    let account = create_account(tocks);
    let keyed_account = KeyedAccount::new(&pubkey, signer, &account);
    f(&keyed_account)
}

#[cfg(test)]
mod test {
    use super::*;
    use crate::{
        account::ReadableAccount,
        account_utils::State as AccountUtilsState,
        fee_calculator::FeeCalculator,
        keyed_account::KeyedAccount,
        nonce::{self, State},
        nonce_account::verify_nonce_account,
        process_instruction::MockInvokeContext,
        system_instruction::SystemError,
    };
    use analog_program::hash::{hash, Hash};

    fn create_test_blockhash(seed: usize) -> (Hash, FeeCalculator) {
        (
            hash(&bincode::serialize(&seed).unwrap()),
            FeeCalculator::new((seed as u64).saturating_mul(100)),
        )
    }

    fn create_invoke_context_with_blockhash<'a>(seed: usize) -> MockInvokeContext<'a> {
        let mut invoke_context = MockInvokeContext::new(vec![]);
        let (blockhash, fee_calculator) = create_test_blockhash(seed);
        invoke_context.blockhash = blockhash;
        invoke_context.fee_calculator = fee_calculator;
        invoke_context
    }

    #[test]
    fn default_is_uninitialized() {
        assert_eq!(State::default(), State::Uninitialized)
    }

    #[test]
    fn keyed_account_expected_behavior() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |keyed_account| {
            let data = nonce::state::Data {
                authority: *keyed_account.unsigned_key(),
                ..nonce::state::Data::default()
            };
            let mut signers = HashSet::new();
            signers.insert(*keyed_account.signer_key().unwrap());
            let state = AccountUtilsState::<Versions>::state(keyed_account)
                .unwrap()
                .convert_to_current();
            // New is in Uninitialzed state
            assert_eq!(state, State::Uninitialized);
            let invoke_context = create_invoke_context_with_blockhash(95);
            let authorized = keyed_account.unsigned_key();
            keyed_account
                .initialize_nonce_account(authorized, &rent, &invoke_context)
                .unwrap();
            let state = AccountUtilsState::<Versions>::state(keyed_account)
                .unwrap()
                .convert_to_current();
            let data = nonce::state::Data {
                blockhash: *invoke_context.get_blockhash(),
                fee_calculator: invoke_context.get_fee_calculator().clone(),
                ..data
            };
            // First nonce instruction drives state from Uninitialized to Initialized
            assert_eq!(state, State::Initialized(data.clone()));
            let invoke_context = create_invoke_context_with_blockhash(63);
            keyed_account
                .advance_nonce_account(&signers, &invoke_context)
                .unwrap();
            let state = AccountUtilsState::<Versions>::state(keyed_account)
                .unwrap()
                .convert_to_current();
            let data = nonce::state::Data {
                blockhash: *invoke_context.get_blockhash(),
                fee_calculator: invoke_context.get_fee_calculator().clone(),
                ..data
            };
            // Second nonce instruction consumes and replaces stored nonce
            assert_eq!(state, State::Initialized(data.clone()));
            let invoke_context = create_invoke_context_with_blockhash(31);
            keyed_account
                .advance_nonce_account(&signers, &invoke_context)
                .unwrap();
            let state = AccountUtilsState::<Versions>::state(keyed_account)
                .unwrap()
                .convert_to_current();
            let data = nonce::state::Data {
                blockhash: *invoke_context.get_blockhash(),
                fee_calculator: invoke_context.get_fee_calculator().clone(),
                ..data
            };
            // Third nonce instruction for fun and profit
            assert_eq!(state, State::Initialized(data));
            with_test_keyed_account(42, false, |to_keyed| {
                let invoke_context = create_invoke_context_with_blockhash(0);
                let withdraw_tocks = keyed_account.account.borrow().tocks();
                let expect_nonce_tocks =
                    keyed_account.account.borrow().tocks() - withdraw_tocks;
                let expect_to_tocks = to_keyed.account.borrow().tocks() + withdraw_tocks;
                keyed_account
                    .withdraw_nonce_account(
                        withdraw_tocks,
                        to_keyed,
                        &rent,
                        &signers,
                        &invoke_context,
                    )
                    .unwrap();
                // Empties Account balance
                assert_eq!(
                    keyed_account.account.borrow().tocks(),
                    expect_nonce_tocks
                );
                // Account balance goes to `to`
                assert_eq!(to_keyed.account.borrow().tocks(), expect_to_tocks);
                let state = AccountUtilsState::<Versions>::state(keyed_account)
                    .unwrap()
                    .convert_to_current();
                // Empty balance deinitializes data
                assert_eq!(state, State::Uninitialized);
            })
        })
    }

    #[test]
    fn nonce_inx_initialized_account_not_signer_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_account| {
            let invoke_context = create_invoke_context_with_blockhash(31);
            let authority = *nonce_account.unsigned_key();
            nonce_account
                .initialize_nonce_account(&authority, &rent, &invoke_context)
                .unwrap();
            let pubkey = *nonce_account.account.borrow().owner();
            let nonce_account = KeyedAccount::new(&pubkey, false, nonce_account.account);
            let state = AccountUtilsState::<Versions>::state(&nonce_account)
                .unwrap()
                .convert_to_current();
            let data = nonce::state::Data {
                authority,
                blockhash: *invoke_context.get_blockhash(),
                fee_calculator: invoke_context.get_fee_calculator().clone(),
            };
            assert_eq!(state, State::Initialized(data));
            let signers = HashSet::new();
            let invoke_context = create_invoke_context_with_blockhash(0);

            let result = nonce_account.advance_nonce_account(&signers, &invoke_context);
            assert_eq!(result, Err(InstructionError::MissingRequiredSignature),);
        })
    }

    #[test]
    fn nonce_inx_too_early_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |keyed_account| {
            let mut signers = HashSet::new();
            signers.insert(*keyed_account.signer_key().unwrap());
            let invoke_context = create_invoke_context_with_blockhash(63);
            let authorized = *keyed_account.unsigned_key();
            keyed_account
                .initialize_nonce_account(&authorized, &rent, &invoke_context)
                .unwrap();
            let result = keyed_account.advance_nonce_account(&signers, &invoke_context);
            assert_eq!(result, Err(SystemError::NonceBlockhashNotExpired.into()));
        })
    }

    #[test]
    fn nonce_inx_uninitialized_account_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |keyed_account| {
            let mut signers = HashSet::new();
            signers.insert(*keyed_account.signer_key().unwrap());
            let invoke_context = create_invoke_context_with_blockhash(63);
            let result = keyed_account.advance_nonce_account(&signers, &invoke_context);
            assert_eq!(result, Err(InstructionError::InvalidAccountData));
        })
    }

    #[test]
    fn nonce_inx_independent_nonce_authority_ok() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_account| {
            with_test_keyed_account(42, true, |nonce_authority| {
                let mut signers = HashSet::new();
                signers.insert(*nonce_account.signer_key().unwrap());
                let invoke_context = create_invoke_context_with_blockhash(63);
                let authorized = *nonce_authority.unsigned_key();
                nonce_account
                    .initialize_nonce_account(&authorized, &rent, &invoke_context)
                    .unwrap();
                let mut signers = HashSet::new();
                signers.insert(*nonce_authority.signer_key().unwrap());
                let invoke_context = create_invoke_context_with_blockhash(31);
                let result = nonce_account.advance_nonce_account(&signers, &invoke_context);
                assert_eq!(result, Ok(()));
            });
        });
    }

    #[test]
    fn nonce_inx_no_nonce_authority_sig_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_account| {
            with_test_keyed_account(42, false, |nonce_authority| {
                let mut signers = HashSet::new();
                signers.insert(*nonce_account.signer_key().unwrap());
                let invoke_context = create_invoke_context_with_blockhash(63);
                let authorized = *nonce_authority.unsigned_key();
                nonce_account
                    .initialize_nonce_account(&authorized, &rent, &invoke_context)
                    .unwrap();
                let result = nonce_account.advance_nonce_account(&signers, &invoke_context);
                assert_eq!(result, Err(InstructionError::MissingRequiredSignature),);
            });
        });
    }

    #[test]
    fn withdraw_inx_unintialized_acc_ok() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_keyed| {
            let state = AccountUtilsState::<Versions>::state(nonce_keyed)
                .unwrap()
                .convert_to_current();
            assert_eq!(state, State::Uninitialized);
            with_test_keyed_account(42, false, |to_keyed| {
                let mut signers = HashSet::new();
                signers.insert(*nonce_keyed.signer_key().unwrap());
                let invoke_context = create_invoke_context_with_blockhash(0);
                let withdraw_tocks = nonce_keyed.account.borrow().tocks();
                let expect_nonce_tocks =
                    nonce_keyed.account.borrow().tocks() - withdraw_tocks;
                let expect_to_tocks = to_keyed.account.borrow().tocks() + withdraw_tocks;
                nonce_keyed
                    .withdraw_nonce_account(
                        withdraw_tocks,
                        to_keyed,
                        &rent,
                        &signers,
                        &invoke_context,
                    )
                    .unwrap();
                let state = AccountUtilsState::<Versions>::state(nonce_keyed)
                    .unwrap()
                    .convert_to_current();
                // Withdraw instruction...
                // Deinitializes Account state
                assert_eq!(state, State::Uninitialized);
                // Empties Account balance
                assert_eq!(
                    nonce_keyed.account.borrow().tocks(),
                    expect_nonce_tocks
                );
                // Account balance goes to `to`
                assert_eq!(to_keyed.account.borrow().tocks(), expect_to_tocks);
            })
        })
    }

    #[test]
    fn withdraw_inx_unintialized_acc_unsigned_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, false, |nonce_keyed| {
            let state = AccountUtilsState::<Versions>::state(nonce_keyed)
                .unwrap()
                .convert_to_current();
            assert_eq!(state, State::Uninitialized);
            with_test_keyed_account(42, false, |to_keyed| {
                let signers = HashSet::new();
                let invoke_context = create_invoke_context_with_blockhash(0);
                let tocks = nonce_keyed.account.borrow().tocks();
                let result = nonce_keyed.withdraw_nonce_account(
                    tocks,
                    to_keyed,
                    &rent,
                    &signers,
                    &invoke_context,
                );
                assert_eq!(result, Err(InstructionError::MissingRequiredSignature),);
            })
        })
    }

    #[test]
    fn withdraw_inx_unintialized_acc_insuff_funds_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_keyed| {
            let state = AccountUtilsState::<Versions>::state(nonce_keyed)
                .unwrap()
                .convert_to_current();
            assert_eq!(state, State::Uninitialized);
            with_test_keyed_account(42, false, |to_keyed| {
                let mut signers = HashSet::new();
                signers.insert(*nonce_keyed.signer_key().unwrap());
                let invoke_context = create_invoke_context_with_blockhash(0);
                let tocks = nonce_keyed.account.borrow().tocks() + 1;
                let result = nonce_keyed.withdraw_nonce_account(
                    tocks,
                    to_keyed,
                    &rent,
                    &signers,
                    &invoke_context,
                );
                assert_eq!(result, Err(InstructionError::InsufficientFunds));
            })
        })
    }

    #[test]
    fn withdraw_inx_uninitialized_acc_two_withdraws_ok() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_keyed| {
            with_test_keyed_account(42, false, |to_keyed| {
                let mut signers = HashSet::new();
                signers.insert(*nonce_keyed.signer_key().unwrap());
                let invoke_context = create_invoke_context_with_blockhash(0);
                let withdraw_tocks = nonce_keyed.account.borrow().tocks() / 2;
                let nonce_expect_tocks =
                    nonce_keyed.account.borrow().tocks() - withdraw_tocks;
                let to_expect_tocks = to_keyed.account.borrow().tocks() + withdraw_tocks;
                nonce_keyed
                    .withdraw_nonce_account(
                        withdraw_tocks,
                        to_keyed,
                        &rent,
                        &signers,
                        &invoke_context,
                    )
                    .unwrap();
                let state = AccountUtilsState::<Versions>::state(nonce_keyed)
                    .unwrap()
                    .convert_to_current();
                assert_eq!(state, State::Uninitialized);
                assert_eq!(
                    nonce_keyed.account.borrow().tocks(),
                    nonce_expect_tocks
                );
                assert_eq!(to_keyed.account.borrow().tocks(), to_expect_tocks);
                let withdraw_tocks = nonce_keyed.account.borrow().tocks();
                let nonce_expect_tocks =
                    nonce_keyed.account.borrow().tocks() - withdraw_tocks;
                let to_expect_tocks = to_keyed.account.borrow().tocks() + withdraw_tocks;
                nonce_keyed
                    .withdraw_nonce_account(
                        withdraw_tocks,
                        to_keyed,
                        &rent,
                        &signers,
                        &invoke_context,
                    )
                    .unwrap();
                let state = AccountUtilsState::<Versions>::state(nonce_keyed)
                    .unwrap()
                    .convert_to_current();
                assert_eq!(state, State::Uninitialized);
                assert_eq!(
                    nonce_keyed.account.borrow().tocks(),
                    nonce_expect_tocks
                );
                assert_eq!(to_keyed.account.borrow().tocks(), to_expect_tocks);
            })
        })
    }

    #[test]
    fn withdraw_inx_initialized_acc_two_withdraws_ok() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_keyed| {
            let mut signers = HashSet::new();
            signers.insert(*nonce_keyed.signer_key().unwrap());
            let invoke_context = create_invoke_context_with_blockhash(31);
            let authority = *nonce_keyed.unsigned_key();
            nonce_keyed
                .initialize_nonce_account(&authority, &rent, &invoke_context)
                .unwrap();
            let state = AccountUtilsState::<Versions>::state(nonce_keyed)
                .unwrap()
                .convert_to_current();
            let data = nonce::state::Data {
                authority,
                blockhash: *invoke_context.get_blockhash(),
                fee_calculator: invoke_context.get_fee_calculator().clone(),
            };
            assert_eq!(state, State::Initialized(data.clone()));
            with_test_keyed_account(42, false, |to_keyed| {
                let withdraw_tocks = nonce_keyed.account.borrow().tocks() - min_tocks;
                let nonce_expect_tocks =
                    nonce_keyed.account.borrow().tocks() - withdraw_tocks;
                let to_expect_tocks = to_keyed.account.borrow().tocks() + withdraw_tocks;
                nonce_keyed
                    .withdraw_nonce_account(
                        withdraw_tocks,
                        to_keyed,
                        &rent,
                        &signers,
                        &invoke_context,
                    )
                    .unwrap();
                let state = AccountUtilsState::<Versions>::state(nonce_keyed)
                    .unwrap()
                    .convert_to_current();
                let data = nonce::state::Data {
                    blockhash: *invoke_context.get_blockhash(),
                    fee_calculator: invoke_context.get_fee_calculator().clone(),
                    ..data.clone()
                };
                assert_eq!(state, State::Initialized(data));
                assert_eq!(
                    nonce_keyed.account.borrow().tocks(),
                    nonce_expect_tocks
                );
                assert_eq!(to_keyed.account.borrow().tocks(), to_expect_tocks);
                let invoke_context = create_invoke_context_with_blockhash(0);
                let withdraw_tocks = nonce_keyed.account.borrow().tocks();
                let nonce_expect_tocks =
                    nonce_keyed.account.borrow().tocks() - withdraw_tocks;
                let to_expect_tocks = to_keyed.account.borrow().tocks() + withdraw_tocks;
                nonce_keyed
                    .withdraw_nonce_account(
                        withdraw_tocks,
                        to_keyed,
                        &rent,
                        &signers,
                        &invoke_context,
                    )
                    .unwrap();
                let state = AccountUtilsState::<Versions>::state(nonce_keyed)
                    .unwrap()
                    .convert_to_current();
                assert_eq!(state, State::Uninitialized);
                assert_eq!(
                    nonce_keyed.account.borrow().tocks(),
                    nonce_expect_tocks
                );
                assert_eq!(to_keyed.account.borrow().tocks(), to_expect_tocks);
            })
        })
    }

    #[test]
    fn withdraw_inx_initialized_acc_nonce_too_early_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_keyed| {
            let invoke_context = create_invoke_context_with_blockhash(0);
            let authorized = *nonce_keyed.unsigned_key();
            nonce_keyed
                .initialize_nonce_account(&authorized, &rent, &invoke_context)
                .unwrap();
            with_test_keyed_account(42, false, |to_keyed| {
                let mut signers = HashSet::new();
                signers.insert(*nonce_keyed.signer_key().unwrap());
                let withdraw_tocks = nonce_keyed.account.borrow().tocks();
                let result = nonce_keyed.withdraw_nonce_account(
                    withdraw_tocks,
                    to_keyed,
                    &rent,
                    &signers,
                    &invoke_context,
                );
                assert_eq!(result, Err(SystemError::NonceBlockhashNotExpired.into()));
            })
        })
    }

    #[test]
    fn withdraw_inx_initialized_acc_insuff_funds_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_keyed| {
            let invoke_context = create_invoke_context_with_blockhash(95);
            let authorized = *nonce_keyed.unsigned_key();
            nonce_keyed
                .initialize_nonce_account(&authorized, &rent, &invoke_context)
                .unwrap();
            with_test_keyed_account(42, false, |to_keyed| {
                let invoke_context = create_invoke_context_with_blockhash(63);
                let mut signers = HashSet::new();
                signers.insert(*nonce_keyed.signer_key().unwrap());
                let withdraw_tocks = nonce_keyed.account.borrow().tocks() + 1;
                let result = nonce_keyed.withdraw_nonce_account(
                    withdraw_tocks,
                    to_keyed,
                    &rent,
                    &signers,
                    &invoke_context,
                );
                assert_eq!(result, Err(InstructionError::InsufficientFunds));
            })
        })
    }

    #[test]
    fn withdraw_inx_initialized_acc_insuff_rent_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_keyed| {
            let invoke_context = create_invoke_context_with_blockhash(95);
            let authorized = *nonce_keyed.unsigned_key();
            nonce_keyed
                .initialize_nonce_account(&authorized, &rent, &invoke_context)
                .unwrap();
            with_test_keyed_account(42, false, |to_keyed| {
                let invoke_context = create_invoke_context_with_blockhash(63);
                let mut signers = HashSet::new();
                signers.insert(*nonce_keyed.signer_key().unwrap());
                let withdraw_tocks = nonce_keyed.account.borrow().tocks() - min_tocks + 1;
                let result = nonce_keyed.withdraw_nonce_account(
                    withdraw_tocks,
                    to_keyed,
                    &rent,
                    &signers,
                    &invoke_context,
                );
                assert_eq!(result, Err(InstructionError::InsufficientFunds));
            })
        })
    }

    #[test]
    fn withdraw_inx_overflow() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_keyed| {
            let invoke_context = create_invoke_context_with_blockhash(95);
            let authorized = *nonce_keyed.unsigned_key();
            nonce_keyed
                .initialize_nonce_account(&authorized, &rent, &invoke_context)
                .unwrap();
            with_test_keyed_account(55, false, |to_keyed| {
                let invoke_context = create_invoke_context_with_blockhash(63);
                let mut signers = HashSet::new();
                signers.insert(*nonce_keyed.signer_key().unwrap());
                let withdraw_tocks = u64::MAX - 54;
                let result = nonce_keyed.withdraw_nonce_account(
                    withdraw_tocks,
                    to_keyed,
                    &rent,
                    &signers,
                    &invoke_context,
                );
                assert_eq!(result, Err(InstructionError::InsufficientFunds));
            })
        })
    }

    #[test]
    fn initialize_inx_ok() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |keyed_account| {
            let state = AccountUtilsState::<Versions>::state(keyed_account)
                .unwrap()
                .convert_to_current();
            assert_eq!(state, State::Uninitialized);
            let mut signers = HashSet::new();
            signers.insert(*keyed_account.signer_key().unwrap());
            let invoke_context = create_invoke_context_with_blockhash(0);
            let authority = *keyed_account.unsigned_key();
            let result = keyed_account.initialize_nonce_account(&authority, &rent, &invoke_context);
            let data = nonce::state::Data {
                authority,
                blockhash: *invoke_context.get_blockhash(),
                fee_calculator: invoke_context.get_fee_calculator().clone(),
            };
            assert_eq!(result, Ok(()));
            let state = AccountUtilsState::<Versions>::state(keyed_account)
                .unwrap()
                .convert_to_current();
            assert_eq!(state, State::Initialized(data));
        })
    }

    #[test]
    fn initialize_inx_initialized_account_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |keyed_account| {
            let invoke_context = create_invoke_context_with_blockhash(31);
            let authorized = *keyed_account.unsigned_key();
            keyed_account
                .initialize_nonce_account(&authorized, &rent, &invoke_context)
                .unwrap();
            let invoke_context = create_invoke_context_with_blockhash(0);
            let result =
                keyed_account.initialize_nonce_account(&authorized, &rent, &invoke_context);
            assert_eq!(result, Err(InstructionError::InvalidAccountData));
        })
    }

    #[test]
    fn initialize_inx_uninitialized_acc_insuff_funds_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks - 42, true, |keyed_account| {
            let invoke_context = create_invoke_context_with_blockhash(63);
            let authorized = *keyed_account.unsigned_key();
            let result =
                keyed_account.initialize_nonce_account(&authorized, &rent, &invoke_context);
            assert_eq!(result, Err(InstructionError::InsufficientFunds));
        })
    }

    #[test]
    fn authorize_inx_ok() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_account| {
            let mut signers = HashSet::new();
            signers.insert(*nonce_account.signer_key().unwrap());
            let invoke_context = create_invoke_context_with_blockhash(31);
            let authorized = *nonce_account.unsigned_key();
            nonce_account
                .initialize_nonce_account(&authorized, &rent, &invoke_context)
                .unwrap();
            let authority = Pubkey::default();
            let data = nonce::state::Data {
                authority,
                blockhash: *invoke_context.get_blockhash(),
                fee_calculator: invoke_context.get_fee_calculator().clone(),
            };
            let result = nonce_account.authorize_nonce_account(
                &Pubkey::default(),
                &signers,
                &invoke_context,
            );
            assert_eq!(result, Ok(()));
            let state = AccountUtilsState::<Versions>::state(nonce_account)
                .unwrap()
                .convert_to_current();
            assert_eq!(state, State::Initialized(data));
        })
    }

    #[test]
    fn authorize_inx_uninitialized_state_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_account| {
            let mut signers = HashSet::new();
            let invoke_context = MockInvokeContext::new(vec![]);
            signers.insert(*nonce_account.signer_key().unwrap());
            let result = nonce_account.authorize_nonce_account(
                &Pubkey::default(),
                &signers,
                &invoke_context,
            );
            assert_eq!(result, Err(InstructionError::InvalidAccountData));
        })
    }

    #[test]
    fn authorize_inx_bad_authority_fail() {
        let rent = Rent {
            tocks_per_byte_year: 42,
            ..Rent::default()
        };
        let min_tocks = rent.minimum_balance(State::size());
        with_test_keyed_account(min_tocks + 42, true, |nonce_account| {
            let mut signers = HashSet::new();
            signers.insert(*nonce_account.signer_key().unwrap());
            let invoke_context = create_invoke_context_with_blockhash(31);
            let authorized = &Pubkey::default().clone();
            nonce_account
                .initialize_nonce_account(authorized, &rent, &invoke_context)
                .unwrap();
            let result = nonce_account.authorize_nonce_account(
                &Pubkey::default(),
                &signers,
                &invoke_context,
            );
            assert_eq!(result, Err(InstructionError::MissingRequiredSignature));
        })
    }

    #[test]
    fn verify_nonce_ok() {
        with_test_keyed_account(42, true, |nonce_account| {
            let mut signers = HashSet::new();
            signers.insert(nonce_account.signer_key().unwrap());
            let state: State = nonce_account.state().unwrap();
            // New is in Uninitialzed state
            assert_eq!(state, State::Uninitialized);
            let invoke_context = create_invoke_context_with_blockhash(0);
            let authorized = nonce_account.unsigned_key();
            nonce_account
                .initialize_nonce_account(authorized, &Rent::free(), &invoke_context)
                .unwrap();
            assert!(verify_nonce_account(
                &nonce_account.account.borrow(),
                invoke_context.get_blockhash(),
            ));
        });
    }

    #[test]
    fn verify_nonce_bad_acc_state_fail() {
        with_test_keyed_account(42, true, |nonce_account| {
            assert!(!verify_nonce_account(
                &nonce_account.account.borrow(),
                &Hash::default()
            ));
        });
    }

    #[test]
    fn verify_nonce_bad_query_hash_fail() {
        with_test_keyed_account(42, true, |nonce_account| {
            let mut signers = HashSet::new();
            signers.insert(nonce_account.signer_key().unwrap());
            let state: State = nonce_account.state().unwrap();
            // New is in Uninitialzed state
            assert_eq!(state, State::Uninitialized);
            let invoke_context = create_invoke_context_with_blockhash(0);
            let authorized = nonce_account.unsigned_key();
            nonce_account
                .initialize_nonce_account(authorized, &Rent::free(), &invoke_context)
                .unwrap();
            let invoke_context = create_invoke_context_with_blockhash(1);
            assert!(!verify_nonce_account(
                &nonce_account.account.borrow(),
                invoke_context.get_blockhash(),
            ));
        });
    }
}
