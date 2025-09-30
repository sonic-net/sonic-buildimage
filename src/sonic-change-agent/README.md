# sonic-change-agent

Kubernetes-native controller for managing SONiC binary image downloads on network devices.

## Overview

`sonic-change-agent` is a DaemonSet controller that watches `NetworkDevice` Custom Resources and downloads SONiC binary images via gNOI (gRPC Network Operations Interface). It provides declarative, GitOps-style image preloading for SONiC devices.

### Key Features

- **Declarative Configuration**: Specify desired SONiC version in Kubernetes CRD
- **gNOI Integration**: Downloads images using gRPC Network Operations Interface
- **MD5 Verification**: Ensures image integrity with checksum validation
- **Idempotent**: Skips download if correct image already exists locally
- **Per-Device Control**: Each agent instance manages only its assigned device
- **DRY_RUN Support**: Test reconciliation logic without actual downloads

## Architecture

```
┌─────────────────────┐
│ NetworkDevice CRD   │  User updates desiredVersion
│ (Kubernetes etcd)   │
└──────────┬──────────┘
           │ Watch
           ▼
┌─────────────────────┐
│ sonic-change-agent  │  Reconcile: compare spec vs status
│ (DaemonSet Pod)     │
└──────────┬──────────┘
           │ gNOI System.SetPackage (activate=false)
           ▼
┌─────────────────────┐
│ gnmi container      │  Downloads image to /tmp/sonic-image.bin
│ (gNOI Server)       │
└─────────────────────┘
```

## Quick Start

### Prerequisites

- Kubernetes cluster with SONiC worker nodes
- SONiC devices running gnmi container with gNOI support
- kubectl access to the cluster

### Installation

1. **Apply CRD**:
```bash
kubectl apply -f manifests/crd.yaml
```

2. **Create RBAC**:
```bash
kubectl apply -f manifests/rbac.yaml
```

3. **Deploy DaemonSet**:
```bash
kubectl apply -f manifests/daemonset.yaml
```

4. **Create NetworkDevice CR**:
```yaml
apiVersion: sonic.io/v1
kind: NetworkDevice
metadata:
  name: sonic-device-01
  namespace: default
spec:
  os:
    desiredVersion: "202511.01"
    imageURL: "http://image-server.example.com/sonic-202511.01.bin"
    downloadPath: "/tmp/sonic-image.bin"  # Optional, defaults to /tmp/sonic-image.bin
    checksum:
      md5: "a1b2c3d4e5f6789012345678901234ab"  # Optional but recommended
```

```bash
kubectl apply -f networkdevice.yaml
```

### Verify Operation

```bash
# Check agent logs
kubectl logs -l app=sonic-change-agent -f

# Check download status
kubectl get networkdevice sonic-device-01 -o yaml

# Expected status:
# status:
#   downloadStatus:
#     phase: Succeeded
#     downloadedVersion: "202511.01"
#     downloadedChecksum: "a1b2c3d4e5f6789012345678901234ab"
```

## Configuration

### Environment Variables

- `NODE_NAME`: Device name to manage (auto-populated by DaemonSet)
- `DRY_RUN`: Set to `"true"` to simulate downloads without executing gRPC calls

### Command-Line Flags

- `--device-name`: Override device name (defaults to NODE_NAME env var)
- `--gnoi-endpoint`: gNOI server address (default: `localhost:8080`)
- `-v`: Log verbosity level (klog standard)

## Reconciliation Logic

The controller reconciles when `spec.os.desiredVersion != status.downloadStatus.downloadedVersion`:

1. **Check local image**: If file exists at `downloadPath` with matching MD5 → update status, skip download
2. **Download image**: Call gNOI `System.SetPackage` with `activate=false`
3. **Verify checksum**: If `spec.os.checksum.md5` provided, verify downloaded file
4. **Update status**: Set `downloadedVersion` and `downloadedChecksum` in status

Once status matches spec, reconciliation stops until spec changes again.

## Development

### Build

```bash
make build
```

### Test

```bash
make test
```

### Run Locally

```bash
export NODE_NAME=test-device
export DRY_RUN=true
./sonic-change-agent --device-name=$NODE_NAME --gnoi-endpoint=localhost:8080 -v=2
```

### Docker Build

```bash
make docker-build VERSION=v0.1.0
```

## Testing with DRY_RUN

To test reconciliation logic without actual downloads:

1. Deploy with `DRY_RUN=true` (already set in manifests/daemonset.yaml)
2. Create/update NetworkDevice CR
3. Observe logs showing reconciliation attempt
4. Verify status updated with `downloadedVersion`
5. Change `desiredVersion` to trigger new reconciliation

## Project Structure

```
.
├── cmd/
│   └── sonic-change-agent/
│       └── main.go              # Entry point
├── pkg/
│   ├── controller/
│   │   ├── controller.go        # Reconciliation logic
│   │   └── controller_test.go
│   ├── gnoi/
│   │   ├── client.go            # gNOI client wrapper
│   │   ├── client_test.go
│   │   └── mock.go              # Mock client for tests
│   └── version/
│       └── version.go
├── manifests/
│   ├── crd.yaml
│   ├── rbac.yaml
│   └── daemonset.yaml
├── Makefile
├── Dockerfile
└── README.md
```

## NetworkDevice CRD Schema

### Spec

- `spec.os.desiredVersion` (string, required): Target SONiC version
- `spec.os.imageURL` (string, required): HTTP/HTTPS URL to download from
- `spec.os.downloadPath` (string, optional): Local file path (default: `/tmp/sonic-image.bin`)
- `spec.os.checksum.md5` (string, optional): Expected MD5 checksum for verification

### Status

- `status.downloadStatus.phase` (string): `Pending`, `Downloading`, `Succeeded`, `Failed`
- `status.downloadStatus.message` (string): Human-readable status message
- `status.downloadStatus.downloadedVersion` (string): Version of currently downloaded image
- `status.downloadStatus.downloadedChecksum` (string): MD5 of downloaded file
- `status.downloadStatus.lastAttemptTime` (string): RFC3339 timestamp of last reconciliation

## Integration with Legacy Workflow

This agent handles **preload only**. Legacy upgrade workflow:

1. `sonic-change-agent` downloads image to `/tmp/sonic-image.bin`
2. Legacy script detects file presence
3. Legacy script proceeds with `sonic-installer install` and reboot

## License

Apache 2.0
