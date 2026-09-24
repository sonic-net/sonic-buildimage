# Makefile for syslog-counter

.PHONY: all build test test-coverage clean install lint format check doc

# Default target
all: build test

# Build the project
build:
	cargo build --release

# Run tests
test:
	cargo test

# Run tests with coverage
test-coverage:
	cargo tarpaulin --out Html --output-dir target/coverage --timeout 120 --all-features

# Clean build artifacts
clean:
	cargo clean

# Install the binary
install: build
	cargo install --path .

# Run linting
lint:
	cargo clippy -- -D warnings

# Format code
format:
	cargo fmt

# Check formatting and linting
check: format lint

# Generate documentation
doc:
	cargo doc --no-deps --open

# Run all quality checks
ci: check test test-coverage

# Quick development check
dev: format
	cargo check
	cargo test

# Profile release build
profile:
	cargo build --release
	@echo "Binary size:"
	@ls -lh target/release/syslog-counter

# Run benchmarks (if any)
bench:
	cargo bench

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build and test (default)"
	@echo "  build        - Build the project"
	@echo "  test         - Run tests"
	@echo "  test-coverage- Run tests with coverage report"
	@echo "  clean        - Clean build artifacts"
	@echo "  install      - Install the binary"
	@echo "  lint         - Run linting (clippy)"
	@echo "  format       - Format code"
	@echo "  check        - Check formatting and linting"
	@echo "  doc          - Generate documentation"
	@echo "  ci           - Run all quality checks"
	@echo "  dev          - Quick development check"
	@echo "  profile      - Show release binary size"
	@echo "  bench        - Run benchmarks"
	@echo "  help         - Show this help"