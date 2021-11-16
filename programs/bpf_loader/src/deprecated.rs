analog_sdk::declare_builtin!(
    analog_sdk::bpf_loader_deprecated::ID,
    analog_bpf_loader_deprecated_program,
    analog_bpf_loader_program::process_instruction,
    deprecated::id
);
