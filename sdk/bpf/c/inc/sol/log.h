#pragma once
/**
 * @brief Analog logging utilities
 */

#include <anlog/types.h>
#include <anlog/string.h>
#include <anlog/entrypoint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prints a string to stdout
 */
void anlog_log_(const char *, uint64_t);
#define anlog_log(message) anlog_log_(message, anlog_strlen(message))

/**
 * Prints a 64 bit values represented in hexadecimal to stdout
 */
void anlog_log_64_(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
#define anlog_log_64 anlog_log_64_

/**
 * Prints the current compute unit consumption to stdout
 */
void anlog_log_compute_units_();
#define anlog_log_compute_units() anlog_log_compute_units_()

/**
 * Prints the hexadecimal representation of an array
 *
 * @param array The array to print
 */
static void anlog_log_array(const uint8_t *array, int len) {
  for (int j = 0; j < len; j++) {
    anlog_log_64(0, 0, 0, j, array[j]);
  }
}

/**
 * Print the base64 representation of some arrays.
 */
void anlog_log_data(SolBytes *fields, uint64_t fields_len);

/**
 * Prints the program's input parameters
 *
 * @param params Pointer to a SolParameters structure
 */
static void anlog_log_params(const SolParameters *params) {
  anlog_log("- Program identifier:");
  anlog_log_pubkey(params->program_id);

  anlog_log("- Number of KeyedAccounts");
  anlog_log_64(0, 0, 0, 0, params->ka_num);
  for (int i = 0; i < params->ka_num; i++) {
    anlog_log("  - Is signer");
    anlog_log_64(0, 0, 0, 0, params->ka[i].is_signer);
    anlog_log("  - Is writable");
    anlog_log_64(0, 0, 0, 0, params->ka[i].is_writable);
    anlog_log("  - Key");
    anlog_log_pubkey(params->ka[i].key);
    anlog_log("  - Lamports");
    anlog_log_64(0, 0, 0, 0, *params->ka[i].tock);
    anlog_log("  - data");
    anlog_log_array(params->ka[i].data, params->ka[i].data_len);
    anlog_log("  - Owner");
    anlog_log_pubkey(params->ka[i].owner);
    anlog_log("  - Executable");
    anlog_log_64(0, 0, 0, 0, params->ka[i].executable);
    anlog_log("  - Rent Epoch");
    anlog_log_64(0, 0, 0, 0, params->ka[i].rent_epoch);
  }
  anlog_log("- Instruction data\0");
  anlog_log_array(params->data, params->data_len);
}

#ifdef ANLOG_TEST
/**
 * Stub functions when building tests
 */
#include <stdio.h>

void anlog_log_(const char *s, uint64_t len) {
  printf("Program log: %s\n", s);
}
void anlog_log_64(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
  printf("Program log: %llu, %llu, %llu, %llu, %llu\n", arg1, arg2, arg3, arg4, arg5);
}

void anlog_log_compute_units_() {
  printf("Program consumption: __ units remaining\n");
}
#endif

#ifdef __cplusplus
}
#endif

/**@}*/
