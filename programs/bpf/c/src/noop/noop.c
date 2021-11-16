/**
 * @brief Example C based BPF program that prints out the parameters
 * passed to it
 */
#include <anlog/deserialize.h>


extern uint64_t entrypoint(const uint8_t *input) {
  SolAccountInfo ka[1];
  SolParameters params = (SolParameters) { .ka = ka };

  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(ka))) {
    return ERROR_INVALID_ARGUMENT;
  }

  return SUCCESS;
}
