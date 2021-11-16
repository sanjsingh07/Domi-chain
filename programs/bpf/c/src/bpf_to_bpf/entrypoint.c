/**
 * @brief Example C-based BPF program that prints out the parameters
 * passed to it
 */
#include <analog_sdk.h>
#include "helper.h"

extern uint64_t entrypoint(const uint8_t *input) {
  anlog_log(__FILE__);
  helper_function();
  anlog_log(__FILE__);
  return SUCCESS;
}
