# Sanitizer Testing Guide

## Overview

This project supports multiple GCC/Clang sanitizers to detect memory errors, undefined behavior, and other runtime issues.

## Available Sanitizers

### 1. AddressSanitizer (ASan)
Detects:
- Memory leaks
- Use-after-free
- Buffer overflows/underflows
- Stack-use-after-return
- Use-after-scope
- Double-free
- Memory allocation/deallocation mismatch

### 2. UndefinedBehaviorSanitizer (UBSan)
Detects:
- Undefined behavior according to C/C++ standards
- Integer overflow
- Division by zero
- Invalid shifts
- Invalid pointer arithmetic
- Null pointer dereference

### 3. LeakSanitizer (LSan)
Detects:
- Memory leaks
- (Automatically enabled with AddressSanitizer)

### 4. ThreadSanitizer (TSan)
Detects:
- Data races
- Deadlocks
- (Not applicable for single-threaded code)

---

## Building with Sanitizers

### AddressSanitizer + LeakSanitizer

```bash
cd /mnt/c/Users/ngoth/Downloads/Resample
rm -rf build
cmake -S . -B build \
    -DUSE_IPP=ON \
    -DENABLE_ASAN=ON \
    -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target resampler_cpp_gtest resampler_ipp_gtest -j$(nproc)
```

### UndefinedBehaviorSanitizer

```bash
cd /mnt/c/Users/ngoth/Downloads/Resample
rm -rf build
cmake -S . -B build \
    -DUSE_IPP=ON \
    -DENABLE_UBSAN=ON \
    -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target resampler_cpp_gtest resampler_ipp_gtest -j$(nproc)
```

### Both ASan and UBSan

```bash
cd /mnt/c/Users/ngoth/Downloads/Resample
rm -rf build
cmake -S . -B build \
    -DUSE_IPP=ON \
    -DENABLE_ASAN=ON \
    -DENABLE_UBSAN=ON \
    -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target resampler_cpp_gtest resampler_ipp_gtest -j$(nproc)
```

---

## Running Tests with Sanitizers

### Pure C++ Tests

```bash
cd /mnt/c/Users/ngoth/Downloads/Resample/build
./resampler_cpp_gtest
```

### Intel IPP Tests

```bash
cd /mnt/c/Users/ngoth/Downloads/Resample/build
export LD_LIBRARY_PATH=/opt/intel/oneapi/ipp/latest/lib/intel64:$LD_LIBRARY_PATH
./resampler_ipp_gtest
```

---

## Sanitizer Options (Environment Variables)

### AddressSanitizer Options

```bash
# Detect memory leaks
export ASAN_OPTIONS=detect_leaks=1

# Halt on first error
export ASAN_OPTIONS=halt_on_error=1

# Print more detailed error reports
export ASAN_OPTIONS=verbosity=1

# Combined options
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:verbosity=1:print_stats=1
```

### UndefinedBehaviorSanitizer Options

```bash
# Print stack traces
export UBSAN_OPTIONS=print_stacktrace=1

# Halt on first error
export UBSAN_OPTIONS=halt_on_error=1

# Combined options
export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1
```

### LeakSanitizer Options

```bash
# Report all leaks
export LSAN_OPTIONS=report_objects=1

# Suppress known leaks (if any)
export LSAN_OPTIONS=suppressions=lsan_suppressions.txt
```

---

## Expected Results

### Clean Run (No Errors)

If the code is memory-safe and correct, you should see:

```
[==========] Running 13 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 13 tests from IQResamplerCPPTest
[ RUN      ] IQResamplerCPPTest.Initialization
[       OK ] IQResamplerCPPTest.Initialization (0 ms)
...
[  PASSED  ] 13 tests.

=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks

SUMMARY: AddressSanitizer: 0 byte(s) leaked in 0 allocation(s).
```

### Example Error Output

If there's a memory leak:
```
=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 1024 bytes in 1 object(s) allocated from:
    #0 0x7f8b9c3d4d38 in malloc
    #1 0x55c3a2b1e234 in IQResamplerCPP::process
    #2 0x55c3a2b1f567 in TEST_F
    ...

SUMMARY: AddressSanitizer: 1024 byte(s) leaked in 1 allocation(s).
```

If there's undefined behavior:
```
runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
    #0 0x55c3a2b1e234 in IQResamplerCPP::process
```

---

## Automated Test Script

Create a script to run all sanitizer tests:

```bash
#!/bin/bash
# File: run_sanitizer_tests.sh

set -e

REPO_DIR="/mnt/c/Users/ngoth/Downloads/Resample"
BUILD_DIR="$REPO_DIR/build_sanitizers"

echo "========================================="
echo "Testing with AddressSanitizer + UBSan"
echo "========================================="

# Clean and build
rm -rf "$BUILD_DIR"
cmake -S "$REPO_DIR" -B "$BUILD_DIR" \
    -DUSE_IPP=ON \
    -DENABLE_ASAN=ON \
    -DENABLE_UBSAN=ON \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build "$BUILD_DIR" --target resampler_cpp_gtest -j$(nproc)

# Run C++ tests
echo ""
echo "Running Pure C++ tests..."
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=0:print_stats=1
export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=0
"$BUILD_DIR/resampler_cpp_gtest"

# Check for IPP build
if [ -f "$BUILD_DIR/resampler_ipp_gtest" ]; then
    echo ""
    echo "Running Intel IPP tests..."
    export LD_LIBRARY_PATH=/opt/intel/oneapi/ipp/latest/lib/intel64:$LD_LIBRARY_PATH
    "$BUILD_DIR/resampler_ipp_gtest"
fi

echo ""
echo "========================================="
echo "Sanitizer tests completed!"
echo "========================================="
```

Save this as `run_sanitizer_tests.sh` and run:

```bash
chmod +x run_sanitizer_tests.sh
./run_sanitizer_tests.sh
```

---

## Interpreting Results

### No Issues Found ✅

```
SUMMARY: AddressSanitizer: 0 byte(s) leaked in 0 allocation(s).
```

This means:
- ✅ No memory leaks
- ✅ No use-after-free
- ✅ No buffer overflows
- ✅ No undefined behavior

### Memory Leak Detected ⚠️

```
Direct leak of 1024 bytes in 1 object(s) allocated from:
```

**Action Required:**
- Check the stack trace to identify where memory was allocated
- Ensure proper `delete` or `free` calls
- Check for missing destructors
- Verify RAII patterns are used correctly

### Buffer Overflow Detected 🔴

```
ERROR: AddressSanitizer: heap-buffer-overflow
```

**Action Required:**
- Check array bounds
- Verify vector sizes before access
- Check loop conditions
- Use `.at()` instead of `[]` for bounds checking

### Undefined Behavior Detected 🔴

```
runtime error: signed integer overflow
```

**Action Required:**
- Fix integer overflow issues
- Check for division by zero
- Verify pointer arithmetic
- Fix null pointer dereferences

---

## Performance Impact

Sanitizers add significant runtime overhead:

| Sanitizer | Slowdown | Memory Overhead |
|-----------|----------|-----------------|
| AddressSanitizer | 2-3x | 2-3x |
| LeakSanitizer | ~1x | minimal |
| UndefinedBehaviorSanitizer | ~1.2x | minimal |
| ThreadSanitizer | 5-15x | 5-10x |

**Note:** Only use sanitizers during development and testing, never in production!

---

## Continuous Integration

Add to your CI pipeline:

```yaml
# .github/workflows/sanitizers.yml
name: Sanitizer Tests

on: [push, pull_request]

jobs:
  asan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build with ASan
        run: |
          cmake -B build -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug
          cmake --build build
      - name: Run tests
        run: |
          cd build
          ./resampler_cpp_gtest

  ubsan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build with UBSan
        run: |
          cmake -B build -DENABLE_UBSAN=ON -DCMAKE_BUILD_TYPE=Debug
          cmake --build build
      - name: Run tests
        run: |
          cd build
          ./resampler_cpp_gtest
```

---

## Common Issues and Solutions

### Issue: "ASan doesn't work in WSL"

**Solution:** Ensure you're using WSL2 (not WSL1) and have a recent kernel:
```bash
wsl --set-version Ubuntu 2
uname -r  # Should show WSL2 kernel
```

### Issue: "Sanitizer runtime not found"

**Solution:** Install sanitizer libraries:
```bash
sudo apt-get update
sudo apt-get install libasan6 libubsan1 liblsan0
```

### Issue: "False positives from system libraries"

**Solution:** Create a suppression file:
```bash
# asan_suppressions.txt
leak:libc.so
leak:libstdc++.so
```

Run with:
```bash
export LSAN_OPTIONS=suppressions=asan_suppressions.txt
./resampler_cpp_gtest
```

---

## Best Practices

1. **Run sanitizers regularly** during development
2. **Fix all issues** before committing code
3. **Test with multiple sanitizers** (ASan, UBSan, etc.)
4. **Use Debug builds** for sanitizer testing
5. **Don't mix sanitizers** (e.g., don't use ASan + TSan together)
6. **Interpret results carefully** (understand false positives)
7. **Document any suppressions** if needed

---

## Summary

Sanitizers are powerful tools for detecting:
- ✅ Memory leaks
- ✅ Buffer overflows
- ✅ Use-after-free
- ✅ Undefined behavior
- ✅ Memory corruption

**Recommendation:** Run sanitizer tests before every release to ensure code quality and memory safety.

---

**Generated:** December 16, 2025
**Updated:** December 16, 2025
