#pragma once
/**
 * @brief Analog program entrypoint
 */

#include <anlog/types.h>
#include <anlog/pubkey.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Keyed Account
 */
typedef struct {
  AnlogPubkey *key;      /** Public key of the account */
  uint64_t *tocks;  /** Number of tocks owned by this account */
  uint64_t data_len;   /** Length of data in bytes */
  uint8_t *data;       /** On-chain data within this account */
  AnlogPubkey *owner;    /** Program that owns this account */
  uint64_t rent_epoch; /** The epoch at which this account will next owe rent */
  bool is_signer;      /** Transaction was signed by this account's key? */
  bool is_writable;    /** Is the account writable? */
  bool executable;     /** This account's data contains a loaded program (and is now read-only) */
} AnlogAccountInfo;

/**
 * The Analog runtime provides a memory region that is available to programs at
 * a fixed virtual address and length. The builtin functions `anlog_calloc` and
 * `anlog_free` call into the Analog runtime to allocate from this memory region
 * for heap operations.  Because the memory region is directly available to
 * programs another option is a program can implement their own heap directly on
 * top of that region.  If a program chooses to implement their own heap they
 * should not call the builtin heap functions because they will conflict.
 * `HEAP_START_ADDRESS` and `HEAP_LENGTH` specify the memory region's start
 * virtual address and length.
 */
#define HEAP_START_ADDRESS (uint64_t)0x300000000
#define HEAP_LENGTH (uint64_t)(32 * 1024)

/**
 * Structure that the program's entrypoint input data is deserialized into.
 */
typedef struct {
  AnlogAccountInfo* ka; /** Pointer to an array of AnlogAccountInfo, must already
                          point to an array of AnlogAccountInfos */
  uint64_t ka_num; /** Number of AnlogAccountInfo entries in `ka` */
  const uint8_t *data; /** pointer to the instruction data */
  uint64_t data_len; /** Length in bytes of the instruction data */
  const AnlogPubkey *program_id; /** program_id of the currently executing program */
} AnlogParameters;

/**
 * Program instruction entrypoint
 *
 * @param input Buffer of serialized input parameters.  Use anlog_deserialize() to decode
 * @return 0 if the instruction executed successfully
 */
uint64_t entrypoint(const uint8_t *input);

#ifdef __cplusplus
}
#endif

/**@}*/
