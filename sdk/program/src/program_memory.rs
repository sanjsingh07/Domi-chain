//! Analog Rust-based BPF memory operations

/// Memcpy
///
/// @param dst - Destination
/// @param src - Source
/// @param n - Number of bytes to copy
#[inline]
pub fn anlog_memcpy(dst: &mut [u8], src: &[u8], n: usize) {
    #[cfg(target_arch = "bpf")]
    {
        extern "C" {
            fn anlog_memcpy_(dst: *mut u8, src: *const u8, n: u64);
        }
        unsafe {
            anlog_memcpy_(dst.as_mut_ptr(), src.as_ptr(), n as u64);
        }
    }

    #[cfg(not(target_arch = "bpf"))]
    crate::program_stubs::anlog_memcpy(dst.as_mut_ptr(), src.as_ptr(), n);
}

/// Memmove
///
/// @param dst - Destination
/// @param src - Source
/// @param n - Number of bytes to copy
///
/// # Safety
#[inline]
pub unsafe fn anlog_memmove(dst: *mut u8, src: *mut u8, n: usize) {
    #[cfg(target_arch = "bpf")]
    {
        extern "C" {
            fn anlog_memmove_(dst: *mut u8, src: *const u8, n: u64);
        }
        anlog_memmove_(dst, src, n as u64);
    }

    #[cfg(not(target_arch = "bpf"))]
    crate::program_stubs::anlog_memmove(dst, src, n);
}

/// Memcmp
///
/// @param s1 - Slice to be compared
/// @param s2 - Slice to be compared
/// @param n - Number of bytes to compare
#[inline]
pub fn anlog_memcmp(s1: &[u8], s2: &[u8], n: usize) -> i32 {
    let mut result = 0;

    #[cfg(target_arch = "bpf")]
    {
        extern "C" {
            fn anlog_memcmp_(s1: *const u8, s2: *const u8, n: u64, result: *mut i32);
        }
        unsafe {
            anlog_memcmp_(s1.as_ptr(), s2.as_ptr(), n as u64, &mut result as *mut i32);
        }
    }

    #[cfg(not(target_arch = "bpf"))]
    crate::program_stubs::anlog_memcmp(s1.as_ptr(), s2.as_ptr(), n, &mut result as *mut i32);

    result
}

/// Memset
///
/// @param s1 - Slice to be compared
/// @param s2 - Slice to be compared
/// @param n - Number of bytes to compare
#[inline]
pub fn anlog_memset(s: &mut [u8], c: u8, n: usize) {
    #[cfg(target_arch = "bpf")]
    {
        extern "C" {
            fn anlog_memset_(s: *mut u8, c: u8, n: u64);
        }
        unsafe {
            anlog_memset_(s.as_mut_ptr(), c, n as u64);
        }
    }

    #[cfg(not(target_arch = "bpf"))]
    crate::program_stubs::anlog_memset(s.as_mut_ptr(), c, n);
}
