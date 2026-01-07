.PHONY: help setup install-deps check dev all build unit test test-integration test-quick clean docker-build fmt vet lint tidy ci

# Variables
BINARY_NAME=sonic-change-agent
DOCKER_IMAGE=sonic-change-agent
VERSION?=dev
GIT_COMMIT=$(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_TIME=$(shell date -u '+%Y-%m-%d_%H:%M:%S')
LDFLAGS=-X github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version.Version=$(VERSION) \
        -X github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version.GitCommit=$(GIT_COMMIT) \
        -X github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version.BuildTime=$(BUILD_TIME)

# Test parameters
PYTEST_ARGS?=-v
MINIKUBE_PROFILE=sonic-test

# Go parameters
GOCMD=go
GOBUILD=$(GOCMD) build
GOTEST=$(GOCMD) test
GOCLEAN=$(GOCMD) clean
GOMOD=$(GOCMD) mod
GOFMT=$(GOCMD) fmt
GOVET=$(GOCMD) vet

# Default target
.DEFAULT_GOAL := help

# Help target - shows available commands
help:
	@echo "sonic-change-agent Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  help              Show this help message"
	@echo ""
	@echo "Setup:"
	@echo "  setup             Initial environment setup (install prerequisites)"
	@echo "  install-deps      Install Python test dependencies"
	@echo ""
	@echo "Development:"
	@echo "  dev               Quick development workflow (fmt, vet, test, build)"
	@echo "  check             Fast validation without building"
	@echo "  all               Full workflow (fmt, vet, test, build)"
	@echo "  build             Build the binary"
	@echo ""
	@echo "Testing:"
	@echo "  test              Run all tests (unit + integration)"
	@echo "  unit              Run Go unit tests only"
	@echo "  test-integration  Run integration tests with Docker rebuild"
	@echo "  test-quick        Run integration tests without Docker rebuild"
	@echo ""
	@echo "Code Quality:"
	@echo "  fmt               Format Go code"
	@echo "  vet               Run Go vet"
	@echo "  lint              Run golangci-lint"
	@echo "  tidy              Run go mod tidy"
	@echo "  coverage          Generate test coverage report"
	@echo ""
	@echo "CI/CD:"
	@echo "  ci                Full CI workflow (includes all tests)"
	@echo "  docker-build      Build Docker image"
	@echo ""
	@echo "Cleanup:"
	@echo "  clean             Clean all artifacts (build, test, logs, clusters)"
	@echo ""
	@echo "Options:"
	@echo "  PYTEST_ARGS      Pass arguments to pytest (default: -v)"
	@echo "                   Example: make test-integration PYTEST_ARGS='-v -k test_preload'"

# Setup and Dependencies
setup: install-deps
	@echo "Checking prerequisites..."
	@which docker > /dev/null || (echo "❌ Docker not found. Please install Docker." && exit 1)
	@which minikube > /dev/null || (echo "❌ minikube not found. Please install minikube." && exit 1)
	@which python3 > /dev/null || (echo "❌ python3 not found. Please install Python 3." && exit 1)
	@which go > /dev/null || (echo "❌ Go not found. Please install Go." && exit 1)
	@echo "✅ All prerequisites found"

install-deps:
	@echo "Installing Python test dependencies..."
	pip install -r test/requirements.txt

# Development Workflows
dev: fmt vet unit build
	@echo "✅ Development workflow completed"

check: fmt vet
	@echo "✅ Quick validation passed"

all: fmt vet unit build
	@echo "✅ Full workflow completed"

# Build
build:
	@echo "Building $(BINARY_NAME)..."
	$(GOBUILD) -ldflags "$(LDFLAGS)" -o $(BINARY_NAME) ./cmd/sonic-change-agent

# Testing
unit:
	@echo "Running Go unit tests..."
	$(GOTEST) -v -race -coverprofile=coverage.out ./...

test: unit test-integration
	@echo "✅ All tests completed"

coverage: unit
	@echo "Generating coverage report..."
	$(GOCMD) tool cover -html=coverage.out -o coverage.html
	@echo "Coverage report generated: coverage.html"

# Code Quality
fmt:
	@echo "Formatting Go code..."
	$(GOFMT) ./...

vet:
	@echo "Running Go vet..."
	$(GOVET) ./...

lint:
	@echo "Running golangci-lint..."
	@which golangci-lint > /dev/null || (echo "❌ golangci-lint not found, install from https://golangci-lint.run/usage/install/" && exit 1)
	golangci-lint run

tidy:
	@echo "Running go mod tidy..."
	$(GOMOD) tidy

# Docker
docker-build:
	@echo "Building Docker image..."
	docker build -t $(DOCKER_IMAGE):$(VERSION) .
	docker tag $(DOCKER_IMAGE):$(VERSION) $(DOCKER_IMAGE):latest
	@echo "✅ Docker image built: $(DOCKER_IMAGE):$(VERSION)"

# Integration Testing
test-integration: _check-test-prereqs
	@echo "Running integration tests with Docker rebuild..."
	cd test && python3 -m pytest $(PYTEST_ARGS)

test-quick: _check-test-prereqs
	@echo "Running integration tests (skipping Docker rebuild)..."
	cd test && SKIP_DOCKER_BUILD=1 python3 -m pytest $(PYTEST_ARGS)


# Internal target to check test prerequisites
_check-test-prereqs:
	@which docker > /dev/null || (echo "❌ Docker required for integration tests. Run 'make setup'" && exit 1)
	@which minikube > /dev/null || (echo "❌ minikube required for integration tests. Run 'make setup'" && exit 1)
	@which python3 > /dev/null || (echo "❌ python3 required for integration tests. Run 'make setup'" && exit 1)
	@test -f test/requirements.txt || (echo "❌ test/requirements.txt not found" && exit 1)
	@python3 -c "import pytest" 2>/dev/null || (echo "❌ pytest not installed. Run 'make install-deps'" && exit 1)

# Cleanup
clean:
	@echo "Cleaning all artifacts..."
	$(GOCLEAN)
	rm -f $(BINARY_NAME)
	rm -f coverage.out coverage.html
	-minikube delete --profile $(MINIKUBE_PROFILE) 2>/dev/null || true
	-docker rmi $(DOCKER_IMAGE):test 2>/dev/null || true
	rm -rf test/test_logs test/.pytest_cache
	@echo "✅ All artifacts cleaned"

# CI/CD
ci: fmt vet test docker-build
	@echo "✅ CI pipeline completed successfully"
