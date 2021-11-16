/**
 * @brief test program that generates BPF PC relative call instructions
 */
#include <analog_sdk.h>

void __attribute__ ((noinline)) helper() {
  anlog_log(__func__);
}

extern uint64_t entrypoint(const uint8_t *input) {
  anlog_log(__func__);
  helper();
  return SUCCESS;
}
