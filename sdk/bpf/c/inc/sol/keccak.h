#pragma once
/**
 * @brief Analog keccak system call
**/

#include <anlog/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Length of a Keccak hash result
 */
#define KECCAK_RESULT_LENGTH 32

/**
 * Keccak
 *
 * @param bytes Array of byte arrays
 * @param bytes_len Number of byte arrays
 * @param result 32 byte array to hold the result
 */
uint64_t anlog_keccak256(
    const SolBytes *bytes,
    int bytes_len,
    uint8_t *result
);

#ifdef __cplusplus
}
#endif

/**@}*/
