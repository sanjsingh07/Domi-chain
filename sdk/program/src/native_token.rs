#![allow(clippy::integer_arithmetic)]
/// There are 10^9 tocks in one ANLOG
pub const TOCKS_PER_ANLOG: u64 = 1_000_000_000;

/// Approximately convert fractional native tokens (tocks) into native tokens (ANLOG)
pub fn tocks_to_anlog(tocks: u64) -> f64 {
    tocks as f64 / TOCKS_PER_ANLOG as f64
}

/// Approximately convert native tokens (ANLOG) into fractional native tokens (tocks)
pub fn anlog_to_tocks(nub: f64) -> u64 {
    (nub * TOCKS_PER_ANLOG as f64) as u64
}

use std::fmt::{Debug, Display, Formatter, Result};
pub struct Anlog(pub u64);

impl Anlog {
    fn write_in_anlog(&self, f: &mut Formatter) -> Result {
        write!(
            f,
            "GM {}.{:09}",
            self.0 / TOCKS_PER_ANLOG,
            self.0 % TOCKS_PER_ANLOG
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
