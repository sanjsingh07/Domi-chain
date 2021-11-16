/**
 * @brief Example C-based BPF program that prints out the parameters
 * passed to it
 */
#include <analog_sdk.h>

extern uint64_t entrypoint(const uint8_t *input) {
  anlog_panic();
  return SUCCESS;
}
