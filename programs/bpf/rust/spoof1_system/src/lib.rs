use solana_program::{
    account_info::AccountInfo, entrypoint, entrypoint::ProgramResult, pubkey::Pubkey,
};

entrypoint!(process_instruction);
#[allow(clippy::unnecessary_wraps)]
fn process_instruction(
    _program_id: &Pubkey,
    accounts: &[AccountInfo],
    _instruction_data: &[u8],
) -> ProgramResult {
    let from = &accounts[0];
    let to = &accounts[1];

    let to_balance = to.tock();
    **to.tock.borrow_mut() = to_balance + from.tock();
    **from.tock.borrow_mut() = 0u64;

    Ok(())
}
