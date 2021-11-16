#pragma once
/**
 * @brief Analog assert and panic utilities
 */

#include <anlog/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Panics
 *
 * Prints the line number where the panic occurred and then causes
 * the BPF VM to immediately halt execution. No accounts' data are updated
 */
void anlog_panic_(const char *, uint64_t, uint64_t, uint64_t);
#define anlog_panic() anlog_panic_(__FILE__, sizeof(__FILE__), __LINE__, 0)

/**
 * Asserts
 */
#define anlog_assert(expr)  \
if (!(expr)) {          \
  anlog_panic(); \
}

#ifdef ANLOG_TEST
/**
 * Stub functions when building tests
 */
#include <stdio.h>

void anlog_panic_(const char *file, uint64_t len, uint64_t line, uint64_t column) {
  printf("Panic in %s at %d:%d\n", file, line, column);
  abort();
}
#endif

#ifdef __cplusplus
}
#endif

/**@}*/
