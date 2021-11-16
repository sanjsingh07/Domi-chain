analog_sdk::declare_builtin!(
    analog_sdk::bpf_loader_upgradeable::ID,
    analog_bpf_loader_upgradeable_program_with_jit,
    analog_bpf_loader_program::process_instruction_jit,
    upgradeable_with_jit::id
);
