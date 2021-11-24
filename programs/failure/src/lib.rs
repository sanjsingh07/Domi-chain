use analog_sdk::{
    instruction::InstructionError, process_instruction::InvokeContext, pubkey::Pubkey,
};

analog_sdk::declare_program!(
    "FaiLure111111111111111111111111111111111111",
    analog_failure_program,
    process_instruction
);

fn process_instruction(
    _program_id: &Pubkey,
    _data: &[u8],
    _invoke_context: &mut dyn InvokeContext,
) -> Result<(), InstructionError> {
    Err(InstructionError::Custom(0))
}
