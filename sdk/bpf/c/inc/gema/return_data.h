#pragma once
/**
 * @brief Analog return data system calls
**/

#include <anlog/types.h>
#include <anlog/pubkey.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Maximum size of return data
 */
#define MAX_RETURN_DATA 1024

/**
 * Set the return data
 *
 * @param bytes byte array to set
 * @param bytes_len length of byte array. This may not exceed MAX_RETURN_DATA.
 */
void anlog_set_return_data(const uint8_t *bytes, uint64_t bytes_len);

/**
 * Get the return data
 *
 * @param bytes byte buffer
 * @param bytes_len maximum length of buffer
 * @param program_id the program_id which set the return data. Only set if there was some return data (the function returns non-zero).
 * @param result length of return data (may exceed bytes_len if the return data is longer)
 */
uint64_t anlog_get_return_data(const uint8_t *bytes, uint64_t bytes_len, AnlogPubkey *program_id);

#ifdef __cplusplus
}
#endif

/**@}*/
