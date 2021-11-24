/**
 * @brief Compute budget tuner program.  Spins in a loop consuming the entire
 * budget, used by the tuner bench test to tune the compute budget costs.
 *
 * Care should be taken because the compiler might optimize out the mechanism
 * you are trying to tune.
 */

#include <analog_sdk.h>

#define NUM_KA 1

extern uint64_t entrypoint(const uint8_t *input) {
  AnlogAccountInfo ka[NUM_KA];
  AnlogParameters params = (AnlogParameters){.ka = ka};
  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(ka))) {
    return ERROR_INVALID_ARGUMENT;
  }
  uint8_t *val = (uint8_t *)ka[0].data;
  size_t current = 1;
  for (uint64_t i = 0; i < UINT64_MAX; i++) {

    // Uncomment for raw compute
    {
      *val ^= val[current % 10000001] + 13181312;
      current *= 12345678;
    }

    // // Uncomment for SHA256 syscall
    // {
    //   uint8_t result[SHA256_RESULT_LENGTH];
    //   uint8_t bytes1[1024];
    //   const AnlogBytes bytes[] = {{bytes1, ANLOG_ARRAY_SIZE(bytes1)}};

    //   anlog_sha256(bytes, ANLOG_ARRAY_SIZE(bytes), result);
    //   *val = result[0];
    // }

    // // Uncomment for Pubkey logging syscall
    // {
    //   AnlogPubkey pubkey;
    //   anlog_log_pubkey(&pubkey);
    // }
  }
  return *val;
}
