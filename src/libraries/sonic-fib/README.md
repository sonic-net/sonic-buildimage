# sonic-fib

## Cheating Sheet
### 1. Clean stale build artifacts
make distclean 2>/dev/null || true
rm -rf autom4te.cache config.log config.status

### 2. Regenerate build system (critical after Makefile.am changes)
autoreconf -fiv

### 3. Reconfigure
./configure

### 4. Build (includes tests)
make -j$(nproc)

### 5. Run tests (uses pre-built binary)
```
make check
make check V=1    # Verbose output
```

Note:
1. Step 1 and Step 2 are used for installing test_drivers. Use the following command to check ls -la config/test-driver
2. test log is at tests/tests.log
