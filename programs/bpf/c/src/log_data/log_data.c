/**
 * @brief Example C-based BPF program uses anlog_log_data
 */
#include <analog_sdk.h>

static const uint8_t return_data[] = { 0x08, 0x01, 0x44 };

extern uint64_t entrypoint(const uint8_t *input) {
  SolAccountInfo ka[1];
  SolParameters params = (SolParameters) { .ka = ka };
  SolBytes fields[2];

  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(ka))) {
    return ERROR_INVALID_ARGUMENT;
  }

  // Generate two fields, split at the first 0 in the input
  fields[0].addr = params.data;
  fields[0].len = anlog_strlen((char*)fields[0].addr);
  fields[1].addr = fields[0].addr + fields[0].len + 1;
  fields[1].len = params.data_len - fields[0].len - 1;

  anlog_set_return_data(return_data, sizeof(return_data));

  anlog_log_data(fields, 2);

  return SUCCESS;
}
