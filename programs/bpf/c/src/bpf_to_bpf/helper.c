/**
 * @brief Example C-based BPF program that prints out the parameters
 * passed to it
 */
#include <analog_sdk.h>
#include "helper.h"

void helper_function(void) {
  anlog_log(__FILE__);
}
