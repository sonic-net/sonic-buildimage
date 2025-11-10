.PHONY: all build test test-integration test-all clean docker-build fmt vet lint

# Variables
BINARY_NAME=sonic-change-agent
DOCKER_IMAGE=sonic-change-agent
VERSION?=dev
GIT_COMMIT=$(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_TIME=$(shell date -u '+%Y-%m-%d_%H:%M:%S')
LDFLAGS=-X github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version.Version=$(VERSION) \
        -X github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version.GitCommit=$(GIT_COMMIT) \
        -X github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version.BuildTime=$(BUILD_TIME)

# Go parameters
GOCMD=go
GOBUILD=$(GOCMD) build
GOTEST=$(GOCMD) test
GOCLEAN=$(GOCMD) clean
GOMOD=$(GOCMD) mod
GOFMT=$(GOCMD) fmt
GOVET=$(GOCMD) vet

all: fmt vet test build

build:
	$(GOBUILD) -ldflags "$(LDFLAGS)" -o $(BINARY_NAME) ./cmd/sonic-change-agent

test:
	$(GOTEST) -v -race -coverprofile=coverage.out ./...

coverage: test
	$(GOCMD) tool cover -html=coverage.out -o coverage.html

clean:
	$(GOCLEAN)
	rm -f $(BINARY_NAME)
	rm -f coverage.out coverage.html

fmt:
	$(GOFMT) ./...

vet:
	$(GOVET) ./...

lint:
	@which golangci-lint > /dev/null || (echo "golangci-lint not found, install from https://golangci-lint.run/usage/install/" && exit 1)
	golangci-lint run

tidy:
	$(GOMOD) tidy

docker-build:
	docker build -t $(DOCKER_IMAGE):$(VERSION) .
	docker tag $(DOCKER_IMAGE):$(VERSION) $(DOCKER_IMAGE):latest

ci: fmt vet test
	@echo "CI checks passed"

# Integration testing
test-integration:
	@echo "Running sonic-change-agent integration tests..."
	cd test && python3 -m pytest -v

test-all: test test-integration

.DEFAULT_GOAL := all
