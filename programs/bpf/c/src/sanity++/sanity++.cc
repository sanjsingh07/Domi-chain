/**
 * @brief Example C++-based BPF program that prints out the parameters
 * passed to it
 */
#include <analog_sdk.h>

/**
 * Custom error for when input serialization fails
 */
#define INVALID_INPUT 1

extern uint64_t entrypoint(const uint8_t *input) {
  AnlogAccountInfo ka[1];
  AnlogParameters params = (AnlogParameters) { .ka = ka };

  anlog_log(__FILE__);

  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(ka))) {
    return ERROR_INVALID_ARGUMENT;
  }

  // Log the provided input parameters.  In the case of  the no-op
  // program, no account keys or input data are expected but real
  // programs will have specific requirements so they can do their work.
  anlog_log_params(&params);
  return SUCCESS;
}
