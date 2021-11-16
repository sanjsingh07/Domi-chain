#include <analog_sdk.h>

static const char msg[] = "This is a message";
static const char msg2[] = "This is a different message";

extern uint64_t entrypoint(const uint8_t *input) {
  anlog_log((char*)msg);
  anlog_log((char*)msg2);
  return SUCCESS;
}
