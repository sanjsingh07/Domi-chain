analog_sdk::declare_builtin!(
    analog_sdk::bpf_loader::ID,
    analog_bpf_loader_program_with_jit,
    analog_bpf_loader_program::process_instruction_jit
);
