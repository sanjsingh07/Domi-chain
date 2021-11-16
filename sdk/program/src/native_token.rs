#![allow(clippy::integer_arithmetic)]
/// There are 10^9 tock in one ANLOG
pub const TOCK_PER_ANLOG: u64 = 1_000_000_000;

/// Approximately convert fractional native tokens (tock) into native tokens (ANLOG)
pub fn tock_to_anlog(tock: u64) -> f64 {
    tock as f64 / TOCK_PER_ANLOG as f64
}

/// Approximately convert native tokens (ANLOG) into fractional native tokens (tock)
pub fn anlog_to_tock(anlog: f64) -> u64 {
    (anlog * TOCK_PER_ANLOG as f64) as u64
}

use std::fmt::{Debug, Display, Formatter, Result};
pub struct Anlog(pub u64);

impl Anlog {
    fn write_in_anlog(&self, f: &mut Formatter) -> Result {
        write!(
            f,
            "◎{}.{:09}",
            self.0 / TOCK_PER_ANLOG,
            self.0 % TOCK_PER_ANLOG
        )
    }
}

impl Display for Anlog {
    fn fmt(&self, f: &mut Formatter) -> Result {
        self.write_in_anlog(f)
    }
}

impl Debug for Anlog {
    fn fmt(&self, f: &mut Formatter) -> Result {
        self.write_in_anlog(f)
    }
}
