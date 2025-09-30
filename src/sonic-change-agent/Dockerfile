# Build stage
FROM golang:1.21-alpine AS builder

WORKDIR /workspace

# Copy go mod files
COPY go.mod go.sum ./
RUN go mod download

# Copy source code
COPY . .

# Build
ARG VERSION=dev
ARG GIT_COMMIT=unknown
ARG BUILD_TIME=unknown
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build \
    -ldflags "-X github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version.Version=${VERSION} \
              -X github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version.GitCommit=${GIT_COMMIT} \
              -X github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version.BuildTime=${BUILD_TIME}" \
    -o sonic-change-agent ./cmd/sonic-change-agent

# Runtime stage
FROM alpine:3.18

RUN apk --no-cache add ca-certificates

WORKDIR /

COPY --from=builder /workspace/sonic-change-agent /sonic-change-agent

USER 65532:65532

ENTRYPOINT ["/sonic-change-agent"]
