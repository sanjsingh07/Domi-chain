/**
 * @brief Example C-based BPF sanity rogram that prints out the parameters
 * passed to it
 */
#include <analog_sdk.h>

extern uint64_t entrypoint(const uint8_t *input) {
  SolAccountInfo ka[2];
  SolParameters params = (SolParameters){.ka = ka};

  anlog_log(__FILE__);

  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(ka))) {
    return ERROR_INVALID_ARGUMENT;
  }

  char ka_data[] = {1, 2, 3};
  SolPubkey ka_owner;
  anlog_memset(ka_owner.x, 0, SIZE_PUBKEY); // set to system program

  anlog_assert(params.ka_num == 2);
  for (int i = 0; i < 2; i++) {
    anlog_assert(*params.ka[i].tock == 42);
    anlog_assert(!anlog_memcmp(params.ka[i].data, ka_data, 4));
    anlog_assert(SolPubkey_same(params.ka[i].owner, &ka_owner));
    anlog_assert(params.ka[i].is_signer == false);
    anlog_assert(params.ka[i].is_writable == false);
    anlog_assert(params.ka[i].executable == false);
  }

  char data[] = {4, 5, 6, 7};
  anlog_assert(params.data_len = 4);
  anlog_assert(!anlog_memcmp(params.data, data, 4));

  return SUCCESS;
}
