/**
 * @brief Example C++ based BPF program that prints out the parameters
 * passed to it
 */
#include <analog_sdk.h>

extern uint64_t entrypoint(const uint8_t *input) {
  AnlogAccountInfo ka[1];
  AnlogParameters params = (AnlogParameters) { .ka = ka };

  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(ka))) {
    return ERROR_INVALID_ARGUMENT;
  }

  return SUCCESS;
}
