/**
 * @brief Example C-based BPF program that moves funds from one account to
 * another
 */
#include <analog_sdk.h>

/**
 * Number of SolKeyedAccount expected. The program should bail if an
 * unexpected number of accounts are passed to the program's entrypoint
 */
#define NUM_KA 3

extern uint64_t entrypoint(const uint8_t *input) {
  SolAccountInfo ka[NUM_KA];
  SolParameters params = (SolParameters) { .ka = ka };

  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(ka))) {
    return ERROR_INVALID_ARGUMENT;
  }

  if (!params.ka[0].is_signer) {
    anlog_log("Transaction not signed by key 0");
    return ERROR_MISSING_REQUIRED_SIGNATURES;
  }

  int64_t tock = *(int64_t *)params.data;
  if (*params.ka[0].tock >= tock) {
    *params.ka[0].tock -= tock;
    *params.ka[2].tock += tock;
    // anlog_log_64(0, 0, *ka[0].tock, *ka[2].tock, tock);
  } else {
    // anlog_log_64(0, 0, 0xFF, *ka[0].tock, tock);
  }
  return SUCCESS;
}
