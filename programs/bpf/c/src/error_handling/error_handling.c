/**
 * @brief Example C-based BPF program that exercises duplicate keyed ka
 * passed to it
 */
#include <analog_sdk.h>

/**
 * Custom error for when input serialization fails
 */

extern uint64_t entrypoint(const uint8_t *input) {
  SolAccountInfo ka[1];
  SolParameters params = (SolParameters) { .ka = ka };

  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(ka))) {
    return ERROR_INVALID_ARGUMENT;
  }

  switch (params.data[0]) {
    case(1):
      anlog_log("return success");
      return SUCCESS;
    case(2):
      anlog_log("return a builtin");
      return ERROR_INVALID_ACCOUNT_DATA;
    case(3):
      anlog_log("return custom error 0");
      return ERROR_CUSTOM_ZERO;
    case(4):
      anlog_log("return custom error 42");
      return 42;
    case(5):
      anlog_log("return an invalid error");
      return ERROR_INVALID_ACCOUNT_DATA + 1;
    case(6):
      anlog_log("return unknown builtin");
      return TO_BUILTIN(50);
    case(9):
      anlog_log("return pubkey error");
      return MAX_SEED_LENGTH_EXCEEDED;
    default:
      anlog_log("Unrecognized command");
      return ERROR_INVALID_INSTRUCTION_DATA;
  }
  return ERROR_INVALID_INSTRUCTION_DATA;
}
