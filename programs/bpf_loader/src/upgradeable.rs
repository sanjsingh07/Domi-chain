analog_sdk::declare_builtin!(
    analog_sdk::bpf_loader_upgradeable::ID,
    analog_bpf_loader_upgradeable_program,
    analog_bpf_loader_program::process_instruction,
    upgradeable::id
);
