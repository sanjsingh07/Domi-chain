use crate::instruction::InstructionError;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum TocksError {
    /// arithmetic underflowed
    #[error("Arithmetic underflowed")]
    ArithmeticUnderflow,

    /// arithmetic overflowed
    #[error("Arithmetic overflowed")]
    ArithmeticOverflow,
}

impl From<TocksError> for InstructionError {
    fn from(error: TocksError) -> Self {
        match error {
            TocksError::ArithmeticOverflow => InstructionError::ArithmeticOverflow,
            TocksError::ArithmeticUnderflow => InstructionError::ArithmeticOverflow,
        }
    }
}
