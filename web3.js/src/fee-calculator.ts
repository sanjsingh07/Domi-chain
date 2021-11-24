import * as BufferLayout from '@solana/buffer-layout';

/**
 * https://github.com/solana-labs/solana/blob/90bedd7e067b5b8f3ddbb45da00a4e9cabb22c62/sdk/src/fee_calculator.rs#L7-L11
 *
 * @internal
 */
export const FeeCalculatorLayout = BufferLayout.nu64('tocksPerSignature');

/**
 * Calculator for transaction fees.
 */
export interface FeeCalculator {
  /** Cost in tocks to validate a signature. */
  tocksPerSignature: number;
}
