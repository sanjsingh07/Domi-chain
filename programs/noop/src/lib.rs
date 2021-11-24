use log::*;
use analog_sdk::{
    instruction::InstructionError, process_instruction::InvokeContext, pubkey::Pubkey,
};

analog_sdk::declare_program!(
    "Noop111111111111111111111111111111111111111",
    analog_noop_program,
    process_instruction
);

pub fn process_instruction(
    program_id: &Pubkey,
    data: &[u8],
    _invoke_context: &mut dyn InvokeContext,
) -> Result<(), InstructionError> {
    analog_logger::setup();
    trace!("noop: program_id: {:?}", program_id);
    trace!("noop: data: {:?}", data);
    Ok(())
}
