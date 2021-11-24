/**
 * @brief Example C-based BPF program that tests cross-program invocations
 */
#include "../invoked/instruction.h"
#include <anlog/entrypoint.h>
#include <anlog/cpi.h>
#include <anlog/pubkey.h>
#include <anlog/log.h>
#include <anlog/assert.h>
#include <anlog/deserialize.h>
#include <anlog/return_data.h>

static const uint8_t TEST_SUCCESS = 1;
static const uint8_t TEST_PRIVILEGE_ESCALATION_SIGNER = 2;
static const uint8_t TEST_PRIVILEGE_ESCALATION_WRITABLE = 3;
static const uint8_t TEST_PPROGRAM_NOT_EXECUTABLE = 4;
static const uint8_t TEST_EMPTY_ACCOUNTS_SLICE = 5;
static const uint8_t TEST_CAP_SEEDS = 6;
static const uint8_t TEST_CAP_SIGNERS = 7;
static const uint8_t TEST_ALLOC_ACCESS_VIOLATION = 8;
static const uint8_t TEST_INSTRUCTION_DATA_TOO_LARGE = 9;
static const uint8_t TEST_INSTRUCTION_META_TOO_LARGE = 10;
static const uint8_t TEST_RETURN_ERROR = 11;
static const uint8_t TEST_PRIVILEGE_DEESCALATION_ESCALATION_SIGNER = 12;
static const uint8_t TEST_PRIVILEGE_DEESCALATION_ESCALATION_WRITABLE = 13;
static const uint8_t TEST_WRITABLE_DEESCALATION_WRITABLE = 14;
static const uint8_t TEST_NESTED_INVOKE_TOO_DEEP = 15;
static const uint8_t TEST_EXECUTABLE_TOCKS = 16;
static const uint8_t ADD_TOCKS = 17;
static const uint8_t TEST_RETURN_DATA_TOO_LARGE = 18;

static const int MINT_INDEX = 0;
static const int ARGUMENT_INDEX = 1;
static const int INVOKED_PROGRAM_INDEX = 2;
static const int INVOKED_ARGUMENT_INDEX = 3;
static const int INVOKED_PROGRAM_DUP_INDEX = 4;
static const int ARGUMENT_DUP_INDEX = 5;
static const int DERIVED_KEY1_INDEX = 6;
static const int DERIVED_KEY2_INDEX = 7;
static const int DERIVED_KEY3_INDEX = 8;
static const int SYSTEM_PROGRAM_INDEX = 9;
static const int FROM_INDEX = 10;

uint64_t do_nested_invokes(uint64_t num_nested_invokes,
                           AnlogAccountInfo *accounts, uint64_t num_accounts) {
  anlog_assert(accounts[ARGUMENT_INDEX].is_signer);

  *accounts[ARGUMENT_INDEX].tocks -= 5;
  *accounts[INVOKED_ARGUMENT_INDEX].tocks += 5;

  AnlogAccountMeta arguments[] = {
      {accounts[INVOKED_ARGUMENT_INDEX].key, true, true},
      {accounts[ARGUMENT_INDEX].key, true, true},
      {accounts[INVOKED_PROGRAM_INDEX].key, false, false}};
  uint8_t data[] = {NESTED_INVOKE, num_nested_invokes};
  const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                      arguments, ANLOG_ARRAY_SIZE(arguments),
                                      data, ANLOG_ARRAY_SIZE(data)};

  anlog_log("First invoke");
  anlog_assert(SUCCESS == anlog_invoke(&instruction, accounts, num_accounts));
  anlog_log("2nd invoke from first program");
  anlog_assert(SUCCESS == anlog_invoke(&instruction, accounts, num_accounts));

  anlog_assert(*accounts[ARGUMENT_INDEX].tocks ==
             42 - 5 + (2 * num_nested_invokes));
  anlog_assert(*accounts[INVOKED_ARGUMENT_INDEX].tocks ==
             10 + 5 - (2 * num_nested_invokes));

  return SUCCESS;
}

extern uint64_t entrypoint(const uint8_t *input) {
  anlog_log("Invoke C program");

  AnlogAccountInfo accounts[12];
  AnlogParameters params = (AnlogParameters){.ka = accounts};

  if (!anlog_deserialize(input, &params, ANLOG_ARRAY_SIZE(accounts))) {
    return ERROR_INVALID_ARGUMENT;
  }

  uint8_t bump_seed1 = params.data[1];
  uint8_t bump_seed2 = params.data[2];
  uint8_t bump_seed3 = params.data[3];

  switch (params.data[0]) {
  case TEST_SUCCESS: {
    anlog_log("Call system program create account");
    {
      uint64_t from_tocks = *accounts[FROM_INDEX].tocks;
      uint64_t to_tocks = *accounts[DERIVED_KEY1_INDEX].tocks;
      AnlogAccountMeta arguments[] = {
          {accounts[FROM_INDEX].key, true, true},
          {accounts[DERIVED_KEY1_INDEX].key, true, true}};
      uint8_t data[4 + 8 + 8 + 32];
      *(uint64_t *)(data + 4) = 42;
      *(uint64_t *)(data + 4 + 8) = MAX_PERMITTED_DATA_INCREASE;
      anlog_memcpy(data + 4 + 8 + 8, params.program_id, SIZE_PUBKEY);
      const AnlogInstruction instruction = {accounts[SYSTEM_PROGRAM_INDEX].key,
                                          arguments, ANLOG_ARRAY_SIZE(arguments),
                                          data, ANLOG_ARRAY_SIZE(data)};
      uint8_t seed1[] = {'Y', 'o', 'u', ' ', 'p', 'a', 's', 's',
                         ' ', 'b', 'u', 't', 't', 'e', 'r'};
      const AnlogSignerSeed seeds1[] = {{seed1, ANLOG_ARRAY_SIZE(seed1)},
                                      {&bump_seed1, 1}};
      const AnlogSignerSeeds signers_seeds[] = {{seeds1, ANLOG_ARRAY_SIZE(seeds1)}};
      anlog_assert(SUCCESS == anlog_invoke_signed(&instruction, accounts,
                                              ANLOG_ARRAY_SIZE(accounts),
                                              signers_seeds,
                                              ANLOG_ARRAY_SIZE(signers_seeds)));
      anlog_assert(*accounts[FROM_INDEX].tocks == from_tocks - 42);
      anlog_assert(*accounts[DERIVED_KEY1_INDEX].tocks == to_tocks + 42);
      anlog_assert(AnlogPubkey_same(accounts[DERIVED_KEY1_INDEX].owner,
                                params.program_id));
      anlog_assert(accounts[DERIVED_KEY1_INDEX].data_len ==
                 MAX_PERMITTED_DATA_INCREASE);
      anlog_assert(
          accounts[DERIVED_KEY1_INDEX].data[MAX_PERMITTED_DATA_INCREASE - 1] ==
          0);
      accounts[DERIVED_KEY1_INDEX].data[MAX_PERMITTED_DATA_INCREASE - 1] = 0x0f;
      anlog_assert(
          accounts[DERIVED_KEY1_INDEX].data[MAX_PERMITTED_DATA_INCREASE - 1] ==
          0x0f);
      for (uint8_t i = 0; i < 20; i++) {
        accounts[DERIVED_KEY1_INDEX].data[i] = i;
      }
    }

    anlog_log("Call system program transfer");
    {
      uint64_t from_tocks = *accounts[FROM_INDEX].tocks;
      uint64_t to_tocks = *accounts[DERIVED_KEY1_INDEX].tocks;
      AnlogAccountMeta arguments[] = {
          {accounts[FROM_INDEX].key, true, true},
          {accounts[DERIVED_KEY1_INDEX].key, true, false}};
      uint8_t data[] = {2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0};
      const AnlogInstruction instruction = {accounts[SYSTEM_PROGRAM_INDEX].key,
                                          arguments, ANLOG_ARRAY_SIZE(arguments),
                                          data, ANLOG_ARRAY_SIZE(data)};
      anlog_assert(SUCCESS ==
                 anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));
      anlog_assert(*accounts[FROM_INDEX].tocks == from_tocks - 1);
      anlog_assert(*accounts[DERIVED_KEY1_INDEX].tocks == to_tocks + 1);
    }

    anlog_log("Test data translation");
    {
      for (int i = 0; i < accounts[ARGUMENT_INDEX].data_len; i++) {
        accounts[ARGUMENT_INDEX].data[i] = i;
      }

      AnlogAccountMeta arguments[] = {
          {accounts[ARGUMENT_INDEX].key, true, true},
          {accounts[INVOKED_ARGUMENT_INDEX].key, true, true},
          {accounts[INVOKED_PROGRAM_INDEX].key, false, false},
          {accounts[INVOKED_PROGRAM_DUP_INDEX].key, false, false}};
      uint8_t data[] = {VERIFY_TRANSLATIONS, 1, 2, 3, 4, 5};
      const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                          arguments, ANLOG_ARRAY_SIZE(arguments),
                                          data, ANLOG_ARRAY_SIZE(data)};

      anlog_assert(SUCCESS ==
                 anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));
    }

    anlog_log("Test no instruction data");
    {
      AnlogAccountMeta arguments[] = {{accounts[ARGUMENT_INDEX].key, true, true}};
      uint8_t data[] = {};
      const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                          arguments, ANLOG_ARRAY_SIZE(arguments),
                                          data, ANLOG_ARRAY_SIZE(data)};

      anlog_assert(SUCCESS ==
                 anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));
    }

    anlog_log("Test return data");
    {
      AnlogAccountMeta arguments[] = {{accounts[ARGUMENT_INDEX].key, true, true}};
      uint8_t data[] = { SET_RETURN_DATA };
      uint8_t buf[100];

      const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                          arguments, ANLOG_ARRAY_SIZE(arguments),
                                          data, ANLOG_ARRAY_SIZE(data)};

      // set some return data, so that the callee can check it is cleared
      anlog_set_return_data((uint8_t[]){1, 2, 3, 4}, 4);

      anlog_assert(SUCCESS ==
                 anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));

      AnlogPubkey setter;

      uint64_t ret = anlog_get_return_data(data, sizeof(data), &setter);

      anlog_assert(ret == sizeof(RETURN_DATA_VAL));

      anlog_assert(anlog_memcmp(data, RETURN_DATA_VAL, sizeof(RETURN_DATA_VAL)));
      anlog_assert(AnlogPubkey_same(&setter, accounts[INVOKED_PROGRAM_INDEX].key));
    }

    anlog_log("Test create_program_address");
    {
      uint8_t seed1[] = {'Y', 'o', 'u', ' ', 'p', 'a', 's', 's',
                         ' ', 'b', 'u', 't', 't', 'e', 'r'};
      const AnlogSignerSeed seeds1[] = {{seed1, ANLOG_ARRAY_SIZE(seed1)},
                                      {&bump_seed1, 1}};
      AnlogPubkey address;
      anlog_assert(SUCCESS ==
                 anlog_create_program_address(seeds1, ANLOG_ARRAY_SIZE(seeds1),
                                            params.program_id, &address));
      anlog_assert(AnlogPubkey_same(&address, accounts[DERIVED_KEY1_INDEX].key));
    }

    anlog_log("Test try_find_program_address");
    {
      uint8_t seed[] = {'Y', 'o', 'u', ' ', 'p', 'a', 's', 's',
                        ' ', 'b', 'u', 't', 't', 'e', 'r'};
      const AnlogSignerSeed seeds[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
      AnlogPubkey address;
      uint8_t bump_seed;
      anlog_assert(SUCCESS == anlog_try_find_program_address(
                                seeds, ANLOG_ARRAY_SIZE(seeds), params.program_id,
                                &address, &bump_seed));
      anlog_assert(AnlogPubkey_same(&address, accounts[DERIVED_KEY1_INDEX].key));
      anlog_assert(bump_seed == bump_seed1);
    }

    anlog_log("Test derived signers");
    {
      anlog_assert(!accounts[DERIVED_KEY1_INDEX].is_signer);
      anlog_assert(!accounts[DERIVED_KEY2_INDEX].is_signer);
      anlog_assert(!accounts[DERIVED_KEY3_INDEX].is_signer);

      AnlogAccountMeta arguments[] = {
          {accounts[INVOKED_PROGRAM_INDEX].key, false, false},
          {accounts[DERIVED_KEY1_INDEX].key, true, true},
          {accounts[DERIVED_KEY2_INDEX].key, true, false},
          {accounts[DERIVED_KEY3_INDEX].key, false, false}};
      uint8_t data[] = {DERIVED_SIGNERS, bump_seed2, bump_seed3};
      const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                          arguments, ANLOG_ARRAY_SIZE(arguments),
                                          data, ANLOG_ARRAY_SIZE(data)};
      uint8_t seed1[] = {'Y', 'o', 'u', ' ', 'p', 'a', 's', 's',
                         ' ', 'b', 'u', 't', 't', 'e', 'r'};
      const AnlogSignerSeed seeds1[] = {{seed1, ANLOG_ARRAY_SIZE(seed1)},
                                      {&bump_seed1, 1}};
      const AnlogSignerSeeds signers_seeds[] = {{seeds1, ANLOG_ARRAY_SIZE(seeds1)}};
      anlog_assert(SUCCESS == anlog_invoke_signed(&instruction, accounts,
                                              ANLOG_ARRAY_SIZE(accounts),
                                              signers_seeds,
                                              ANLOG_ARRAY_SIZE(signers_seeds)));
    }

    anlog_log("Test readonly with writable account");
    {
      AnlogAccountMeta arguments[] = {
          {accounts[INVOKED_ARGUMENT_INDEX].key, true, false}};
      uint8_t data[] = {VERIFY_WRITER};
      const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                          arguments, ANLOG_ARRAY_SIZE(arguments),
                                          data, ANLOG_ARRAY_SIZE(data)};

      anlog_assert(SUCCESS ==
                 anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));
    }

    anlog_log("Test nested invoke");
    {
      anlog_assert(SUCCESS == do_nested_invokes(4, accounts, params.ka_num));
    }

    anlog_log("Test privilege deescalation");
    {
      anlog_assert(true == accounts[INVOKED_ARGUMENT_INDEX].is_signer);
      anlog_assert(true == accounts[INVOKED_ARGUMENT_INDEX].is_writable);
      AnlogAccountMeta arguments[] = {
          {accounts[INVOKED_ARGUMENT_INDEX].key, false, false}};
      uint8_t data[] = {VERIFY_PRIVILEGE_DEESCALATION};
      const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                          arguments, ANLOG_ARRAY_SIZE(arguments),
                                          data, ANLOG_ARRAY_SIZE(data)};
      anlog_assert(SUCCESS ==
                 anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));
    }

    anlog_log("Verify data values are retained and updated");
    for (int i = 0; i < accounts[ARGUMENT_INDEX].data_len; i++) {
      anlog_assert(accounts[ARGUMENT_INDEX].data[i] == i);
    }
    for (int i = 0; i < accounts[INVOKED_ARGUMENT_INDEX].data_len; i++) {
      anlog_assert(accounts[INVOKED_ARGUMENT_INDEX].data[i] == i);
    }

    anlog_log("Verify data write before ro cpi call");
    {
      for (int i = 0; i < accounts[ARGUMENT_INDEX].data_len; i++) {
        accounts[ARGUMENT_INDEX].data[i] = 0;
      }

      AnlogAccountMeta arguments[] = {
          {accounts[ARGUMENT_INDEX].key, false, false}};
      uint8_t data[] = {VERIFY_PRIVILEGE_DEESCALATION};
      const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                          arguments, ANLOG_ARRAY_SIZE(arguments),
                                          data, ANLOG_ARRAY_SIZE(data)};
      anlog_assert(SUCCESS ==
                 anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));

      for (int i = 0; i < accounts[ARGUMENT_INDEX].data_len; i++) {
        anlog_assert(accounts[ARGUMENT_INDEX].data[i] == 0);
      }
    }
    break;
  }
  case TEST_PRIVILEGE_ESCALATION_SIGNER: {
    anlog_log("Test privilege escalation signer");
    AnlogAccountMeta arguments[] = {
        {accounts[DERIVED_KEY3_INDEX].key, false, false}};
    uint8_t data[] = {VERIFY_PRIVILEGE_ESCALATION};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};
    anlog_assert(SUCCESS ==
               anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));

    // Signer privilege escalation will always fail the whole transaction
    instruction.accounts[0].is_signer = true;
    anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts));
    break;
  }
  case TEST_PRIVILEGE_ESCALATION_WRITABLE: {
    anlog_log("Test privilege escalation writable");
    AnlogAccountMeta arguments[] = {
        {accounts[DERIVED_KEY3_INDEX].key, false, false}};
    uint8_t data[] = {VERIFY_PRIVILEGE_ESCALATION};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};
    anlog_assert(SUCCESS ==
               anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));

    // Writable privilege escalation will always fail the whole transaction
    instruction.accounts[0].is_writable = true;
    anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts));
    break;
  }
  case TEST_PPROGRAM_NOT_EXECUTABLE: {
    anlog_log("Test program not executable");
    AnlogAccountMeta arguments[] = {
        {accounts[DERIVED_KEY3_INDEX].key, false, false}};
    uint8_t data[] = {VERIFY_PRIVILEGE_ESCALATION};
    const AnlogInstruction instruction = {accounts[ARGUMENT_INDEX].key, arguments,
                                        ANLOG_ARRAY_SIZE(arguments), data,
                                        ANLOG_ARRAY_SIZE(data)};
    return anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts));
  }
  case TEST_EMPTY_ACCOUNTS_SLICE: {
    anlog_log("Empty accounts slice");

    AnlogAccountMeta arguments[] = {
        {accounts[INVOKED_ARGUMENT_INDEX].key, false, false}};
    uint8_t data[] = {};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};

    anlog_assert(SUCCESS == anlog_invoke(&instruction, 0, 0));
    break;
  }
  case TEST_CAP_SEEDS: {
    anlog_log("Test cap seeds");
    AnlogAccountMeta arguments[] = {};
    uint8_t data[] = {};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};
    uint8_t seed[] = {"seed"};
    const AnlogSignerSeed seeds[] = {
        {seed, ANLOG_ARRAY_SIZE(seed)}, {seed, ANLOG_ARRAY_SIZE(seed)},
        {seed, ANLOG_ARRAY_SIZE(seed)}, {seed, ANLOG_ARRAY_SIZE(seed)},
        {seed, ANLOG_ARRAY_SIZE(seed)}, {seed, ANLOG_ARRAY_SIZE(seed)},
        {seed, ANLOG_ARRAY_SIZE(seed)}, {seed, ANLOG_ARRAY_SIZE(seed)},
        {seed, ANLOG_ARRAY_SIZE(seed)}, {seed, ANLOG_ARRAY_SIZE(seed)},
        {seed, ANLOG_ARRAY_SIZE(seed)}, {seed, ANLOG_ARRAY_SIZE(seed)},
        {seed, ANLOG_ARRAY_SIZE(seed)}, {seed, ANLOG_ARRAY_SIZE(seed)},
        {seed, ANLOG_ARRAY_SIZE(seed)}, {seed, ANLOG_ARRAY_SIZE(seed)},
        {seed, ANLOG_ARRAY_SIZE(seed)},
    };
    const AnlogSignerSeeds signers_seeds[] = {{seeds, ANLOG_ARRAY_SIZE(seeds)}};
    anlog_assert(SUCCESS == anlog_invoke_signed(
                              &instruction, accounts, ANLOG_ARRAY_SIZE(accounts),
                              signers_seeds, ANLOG_ARRAY_SIZE(signers_seeds)));
    break;
  }
  case TEST_CAP_SIGNERS: {
    anlog_log("Test cap signers");
    AnlogAccountMeta arguments[] = {};
    uint8_t data[] = {};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};
    uint8_t seed[] = {"seed"};
    const AnlogSignerSeed seed1[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed2[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed3[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed4[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed5[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed6[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed7[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed8[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed9[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed10[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed11[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed12[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed13[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed14[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed15[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed16[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeed seed17[] = {{seed, ANLOG_ARRAY_SIZE(seed)}};
    const AnlogSignerSeeds signers_seeds[] = {
        {seed1, ANLOG_ARRAY_SIZE(seed1)},   {seed2, ANLOG_ARRAY_SIZE(seed2)},
        {seed3, ANLOG_ARRAY_SIZE(seed3)},   {seed4, ANLOG_ARRAY_SIZE(seed4)},
        {seed5, ANLOG_ARRAY_SIZE(seed5)},   {seed6, ANLOG_ARRAY_SIZE(seed6)},
        {seed7, ANLOG_ARRAY_SIZE(seed7)},   {seed8, ANLOG_ARRAY_SIZE(seed8)},
        {seed9, ANLOG_ARRAY_SIZE(seed9)},   {seed10, ANLOG_ARRAY_SIZE(seed10)},
        {seed11, ANLOG_ARRAY_SIZE(seed11)}, {seed12, ANLOG_ARRAY_SIZE(seed12)},
        {seed13, ANLOG_ARRAY_SIZE(seed13)}, {seed14, ANLOG_ARRAY_SIZE(seed14)},
        {seed15, ANLOG_ARRAY_SIZE(seed15)}, {seed16, ANLOG_ARRAY_SIZE(seed16)},
        {seed17, ANLOG_ARRAY_SIZE(seed17)}};
    anlog_assert(SUCCESS == anlog_invoke_signed(
                              &instruction, accounts, ANLOG_ARRAY_SIZE(accounts),
                              signers_seeds, ANLOG_ARRAY_SIZE(signers_seeds)));
    break;
  }
  case TEST_ALLOC_ACCESS_VIOLATION: {
    anlog_log("Test resize violation");
    AnlogAccountMeta arguments[] = {
        {accounts[FROM_INDEX].key, true, true},
        {accounts[DERIVED_KEY1_INDEX].key, true, true}};
    uint8_t data[4 + 8 + 8 + 32];
    *(uint64_t *)(data + 4) = 42;
    *(uint64_t *)(data + 4 + 8) = MAX_PERMITTED_DATA_INCREASE;
    anlog_memcpy(data + 4 + 8 + 8, params.program_id, SIZE_PUBKEY);
    const AnlogInstruction instruction = {accounts[SYSTEM_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};
    uint8_t seed1[] = {'Y', 'o', 'u', ' ', 'p', 'a', 's', 's',
                       ' ', 'b', 'u', 't', 't', 'e', 'r'};
    const AnlogSignerSeed seeds1[] = {{seed1, ANLOG_ARRAY_SIZE(seed1)},
                                    {&bump_seed1, 1}};
    const AnlogSignerSeeds signers_seeds[] = {{seeds1, ANLOG_ARRAY_SIZE(seeds1)}};

    AnlogAccountInfo derived_account = {
        .key = accounts[DERIVED_KEY1_INDEX].key,
        .tocks = accounts[DERIVED_KEY1_INDEX].tocks,
        .data_len = accounts[DERIVED_KEY1_INDEX].data_len,
        // Point to top edge of heap, attempt to allocate into unprivileged
        // memory
        .data = (uint8_t *)0x300007ff8,
        .owner = accounts[DERIVED_KEY1_INDEX].owner,
        .rent_epoch = accounts[DERIVED_KEY1_INDEX].rent_epoch,
        .is_signer = accounts[DERIVED_KEY1_INDEX].is_signer,
        .is_writable = accounts[DERIVED_KEY1_INDEX].is_writable,
        .executable = accounts[DERIVED_KEY1_INDEX].executable,
    };
    const AnlogAccountInfo invoke_accounts[] = {
        accounts[FROM_INDEX], accounts[SYSTEM_PROGRAM_INDEX], derived_account};
    anlog_assert(SUCCESS ==
               anlog_invoke_signed(&instruction,
                                 (const AnlogAccountInfo *)invoke_accounts, 3,
                                 signers_seeds, ANLOG_ARRAY_SIZE(signers_seeds)));
    break;
  }
  case TEST_INSTRUCTION_DATA_TOO_LARGE: {
    anlog_log("Test instruction data too large");
    AnlogAccountMeta arguments[] = {};
    uint8_t *data = anlog_calloc(1500, 1);
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, 1500};
    const AnlogSignerSeeds signers_seeds[] = {};
    anlog_assert(SUCCESS == anlog_invoke_signed(
                              &instruction, accounts, ANLOG_ARRAY_SIZE(accounts),
                              signers_seeds, ANLOG_ARRAY_SIZE(signers_seeds)));

    break;
  }
  case TEST_INSTRUCTION_META_TOO_LARGE: {
    anlog_log("Test instruction meta too large");
    AnlogAccountMeta *arguments = anlog_calloc(40, sizeof(AnlogAccountMeta));
    anlog_log_64(0, 0, 0, 0, (uint64_t)arguments);
    anlog_assert(0 != arguments);
    uint8_t data[] = {};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, 40, data,
                                        ANLOG_ARRAY_SIZE(data)};
    const AnlogSignerSeeds signers_seeds[] = {};
    anlog_assert(SUCCESS == anlog_invoke_signed(
                              &instruction, accounts, ANLOG_ARRAY_SIZE(accounts),
                              signers_seeds, ANLOG_ARRAY_SIZE(signers_seeds)));

    break;
  }
  case TEST_RETURN_ERROR: {
    anlog_log("Test return error");
    AnlogAccountMeta arguments[] = {{accounts[ARGUMENT_INDEX].key, false, true}};
    uint8_t data[] = {RETURN_ERROR};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};

    anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts));
    break;
  }
  case TEST_PRIVILEGE_DEESCALATION_ESCALATION_SIGNER: {
    anlog_log("Test privilege deescalation escalation signer");
    anlog_assert(true == accounts[INVOKED_ARGUMENT_INDEX].is_signer);
    anlog_assert(true == accounts[INVOKED_ARGUMENT_INDEX].is_writable);
    AnlogAccountMeta arguments[] = {
        {accounts[INVOKED_PROGRAM_INDEX].key, false, false},
        {accounts[INVOKED_ARGUMENT_INDEX].key, false, false}};
    uint8_t data[] = {VERIFY_PRIVILEGE_DEESCALATION_ESCALATION_SIGNER};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};
    anlog_assert(SUCCESS ==
               anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));
    break;
  }
  case TEST_PRIVILEGE_DEESCALATION_ESCALATION_WRITABLE: {
    anlog_log("Test privilege deescalation escalation writable");
    anlog_assert(true == accounts[INVOKED_ARGUMENT_INDEX].is_signer);
    anlog_assert(true == accounts[INVOKED_ARGUMENT_INDEX].is_writable);
    AnlogAccountMeta arguments[] = {
        {accounts[INVOKED_PROGRAM_INDEX].key, false, false},
        {accounts[INVOKED_ARGUMENT_INDEX].key, false, false}};
    uint8_t data[] = {VERIFY_PRIVILEGE_DEESCALATION_ESCALATION_WRITABLE};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};
    anlog_assert(SUCCESS ==
               anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts)));
    break;
  }
  case TEST_WRITABLE_DEESCALATION_WRITABLE: {
    anlog_log("Test writable deescalation");
    uint8_t buffer[10];
    for (int i = 0; i < 10; i++) {
      buffer[i] = accounts[INVOKED_ARGUMENT_INDEX].data[i];
    }
    AnlogAccountMeta arguments[] = {
        {accounts[INVOKED_ARGUMENT_INDEX].key, false, false}};
    uint8_t data[] = {WRITE_ACCOUNT, 10};
    const AnlogInstruction instruction = {accounts[INVOKED_PROGRAM_INDEX].key,
                                        arguments, ANLOG_ARRAY_SIZE(arguments),
                                        data, ANLOG_ARRAY_SIZE(data)};
    anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts));

    for (int i = 0; i < 10; i++) {
      anlog_assert(buffer[i] == accounts[INVOKED_ARGUMENT_INDEX].data[i]);
    }
    break;
  }
  case TEST_NESTED_INVOKE_TOO_DEEP: {
    do_nested_invokes(5, accounts, params.ka_num);
    break;
  }
  case TEST_EXECUTABLE_TOCKS: {
    anlog_log("Test executable tocks");
    accounts[ARGUMENT_INDEX].executable = true;
    *accounts[ARGUMENT_INDEX].tocks -= 1;
    *accounts[DERIVED_KEY1_INDEX].tocks +=1;
    AnlogAccountMeta arguments[] = {
      {accounts[ARGUMENT_INDEX].key, true, false},
      {accounts[DERIVED_KEY1_INDEX].key, true, false},
    };
    uint8_t data[] = {ADD_TOCKS, 0, 0, 0};
    AnlogPubkey program_id;
    anlog_memcpy(&program_id, params.program_id, sizeof(AnlogPubkey));
    const AnlogInstruction instruction = {&program_id,
					arguments, ANLOG_ARRAY_SIZE(arguments),
					data, ANLOG_ARRAY_SIZE(data)};
    anlog_invoke(&instruction, accounts, ANLOG_ARRAY_SIZE(accounts));
    *accounts[ARGUMENT_INDEX].tocks += 1;
    break;
  }
  case ADD_TOCKS: {
    *accounts[0].tocks += 1;
     break;
  }
  case TEST_RETURN_DATA_TOO_LARGE: {
    anlog_log("Test setting return data too long");
    // The actual buffer doesn't matter, just pass null
    anlog_set_return_data(NULL, 1027);
    break;
  }

  default:
    anlog_panic();
  }

  return SUCCESS;
}
