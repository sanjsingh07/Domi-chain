/**
 * @brief Example C-based BPF program that exercises duplicate keyed accounts
 * passed to it
 */
#include <analog_sdk.h>

/**
 * Custom error for when input serialization fails
 */

extern uint64_t entrypoint(const uint8_t *input) {
  AnlogAccountInfo accounts[5];
  AnlogParameters params = (AnlogParameters){.ka = accounts};

  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(accounts))) {
    return ERROR_INVALID_ARGUMENT;
  }

  switch (params.data[0]) {
  case (1):
    anlog_log("modify first account data");
    accounts[2].data[0] = 1;
    break;
  case (2):
    anlog_log("modify first account data");
    accounts[3].data[0] = 2;
    break;
  case (3):
    anlog_log("modify both account data");
    accounts[2].data[0] += 1;
    accounts[3].data[0] += 2;
    break;
  case (4):
    anlog_log("modify first account tocks");
    *accounts[1].tocks -= 1;
    *accounts[2].tocks += 1;
    break;
  case (5):
    anlog_log("modify first account tocks");
    *accounts[1].tocks -= 2;
    *accounts[3].tocks += 2;
    break;
  case (6):
    anlog_log("modify both account tocks");
    *accounts[1].tocks -= 3;
    *accounts[2].tocks += 1;
    *accounts[3].tocks += 2;
    break;
  case (7):
    anlog_log("check account (0,1,2,3) privs");
    anlog_assert(accounts[0].is_signer);
    anlog_assert(!accounts[1].is_signer);
    anlog_assert(accounts[2].is_signer);
    anlog_assert(accounts[3].is_signer);

    anlog_assert(accounts[0].is_writable);
    anlog_assert(accounts[1].is_writable);
    anlog_assert(accounts[2].is_writable);
    anlog_assert(accounts[3].is_writable);

    if (params.ka_num > 4) {
      {
        AnlogAccountMeta arguments[] = {{accounts[0].key, true, true},
                                      {accounts[1].key, true, false},
                                      {accounts[2].key, true, false},
                                      {accounts[3].key, false, true}};
        uint8_t data[] = {7};
        const AnlogInstruction instruction = {
            (AnlogPubkey *)params.program_id, arguments,
            ANLOG_ARRAY_SIZE(arguments), data, ANLOG_ARRAY_SIZE(data)};
        anlog_assert(SUCCESS ==
                   anlog_invoke(&instruction, accounts, params.ka_num));
      }
      {
        AnlogAccountMeta arguments[] = {{accounts[0].key, true, true},
                                      {accounts[1].key, true, false},
                                      {accounts[2].key, true, false},
                                      {accounts[3].key, true, false}};
        uint8_t data[] = {3};
        const AnlogInstruction instruction = {
            (AnlogPubkey *)params.program_id, arguments,
            ANLOG_ARRAY_SIZE(arguments), data, ANLOG_ARRAY_SIZE(data)};
        anlog_assert(SUCCESS ==
                   anlog_invoke(&instruction, accounts, params.ka_num));
      }
      anlog_assert(accounts[2].data[0] == 3);
      anlog_assert(accounts[3].data[0] == 3);
    }
    break;
  default:
    anlog_log("Unrecognized command");
    return ERROR_INVALID_INSTRUCTION_DATA;
  }
  return SUCCESS;
}
