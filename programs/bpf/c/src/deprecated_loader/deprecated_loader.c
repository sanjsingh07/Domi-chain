/**
 * @brief Example C-based BPF program that prints out the parameters
 * passed to it
 */
#include <anlog/types.h>
#include <anlog/log.h>
#include <anlog/deserialize_deprecated.h>

extern uint64_t entrypoint(const uint8_t *input) {
  SolAccountInfo ka[1];
  SolParameters params = (SolParameters) { .ka = ka };

  anlog_log(__FILE__);

  if (!anlog_deserialize_deprecated(input, &params, ANLOG_ARRAY_SIZE(ka))) {
    return ERROR_INVALID_ARGUMENT;
  }

  // Log the provided input parameters.  In the case of  the no-op
  // program, no account keys or input data are expected but real
  // programs will have specific requirements so they can do their work.
  anlog_log_params(&params);
  return SUCCESS;
}
