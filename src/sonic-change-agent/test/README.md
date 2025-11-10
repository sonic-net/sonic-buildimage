# sonic-change-agent Integration Tests

pytest-based integration tests for sonic-change-agent Kubernetes controller.

## Overview

These tests validate the complete sonic-change-agent workflow:
1. **Cluster Setup** - Creates minikube cluster
2. **Image Build** - Builds sonic-change-agent Docker image  
3. **Redis Deployment** - Deploys Redis with SONiC CONFIG_DB simulation
4. **Agent Deployment** - Deploys sonic-change-agent with CRD, RBAC, DaemonSet
5. **Workflow Testing** - Creates NetworkDevice resources and validates execution
6. **Cleanup** - Removes all test resources

## Quick Start

```bash
# Install dependencies (from project root)
pip install -r test/requirements.txt

# Run all integration tests
make test-integration

# Or run pytest directly  
cd test && python3 -m pytest -v
```

## Test Structure

### Fixtures (conftest.py)
- **cluster** - Creates/destroys minikube cluster
- **docker_image** - Builds/removes test Docker image
- **redis_deployment** - Deploys Redis with CONFIG_DB
- **sonic_deployment** - Deploys sonic-change-agent components
- **network_device** - Factory for creating test NetworkDevice resources

### Test Cases (test_integration.py)
- **test_preload_workflow** - Validates OSUpgrade PreloadImage workflow
- **test_unsupported_workflow_handling** - Tests handling of unsupported operations
- **test_preload_workflow_variations** - Tests PreloadImage with different configurations
- **test_crd_compliance** - Validates CRD deployment and structure
- **test_system_health** - Checks overall system health
- **test_error_handling** - Tests graceful handling of invalid configurations

## Test Markers

Run specific test categories:
```bash
# Only workflow tests
cd test && python3 -m pytest -v -m workflow

# Exclude slow tests
cd test && python3 -m pytest -v -m "not slow"

# Specific test
cd test && python3 -m pytest -v test_integration.py::test_preload_workflow
```

## Prerequisites

- **Docker** - For building images
- **minikube** - For Kubernetes cluster
- **Python 3.7+** - For running tests

## Expected Output

Successful test run shows:
```
🏗️  Setting up test cluster: sonic-test
✅ Cluster is ready
🐳 Building Docker image: sonic-change-agent:test  
✅ Docker image built
📦 Deploying Redis...
✅ Redis deployed and configured
🚀 Deploying sonic-change-agent...
✅ sonic-change-agent deployed and ready

test_integration.py::test_preload_workflow PASSED
test_integration.py::test_install_workflow PASSED  
test_integration.py::test_multiple_devices_concurrent PASSED
test_integration.py::test_crd_compliance PASSED
test_integration.py::test_system_health PASSED
test_integration.py::test_error_handling PASSED

📋 Collecting logs to: test_logs/test_preload_workflow_20251110_173121
✅ Logs collected in: test_logs/test_preload_workflow_20251110_173121

🧹 Cleaning up...
```

## Log Collection

**Automatic log collection** happens after each test:

### Log Files Created:
```
test_logs/
└── test_name_timestamp/
    ├── README.txt                          # Test summary
    ├── sonic-change-agent-<pod>.log        # Controller logs
    ├── sonic-change-agent-<pod>_describe.txt # Pod details
    ├── redis-<pod>.log                     # Redis logs  
    ├── redis-<pod>_describe.txt           # Redis pod details
    ├── cluster_pods.txt                    # All pods status
    ├── cluster_nodes.txt                   # Cluster nodes
    ├── cluster_networkdevices.txt          # NetworkDevice resources
    ├── cluster_crd.txt                     # CRD status
    ├── cluster_services.txt                # Services
    └── cluster_events.txt                  # Cluster events
```

### Key Log Files:
- **`sonic-change-agent-*.log`** - Controller workflow execution logs
- **`cluster_networkdevices.txt`** - NetworkDevice resource states  
- **`cluster_events.txt`** - Kubernetes events for debugging
- **`README.txt`** - Test summary and file list

## Debugging

If tests fail:
```bash
# Run with more verbose output
cd test && python3 -m pytest -v -s

# Run single test for debugging
cd test && python3 -m pytest -v -s test_integration.py::test_preload_workflow

# Check cluster state manually
minikube status --profile sonic-test
minikube kubectl --profile sonic-test -- get pods
minikube kubectl --profile sonic-test -- logs daemonset/sonic-change-agent
```

## CI/CD Integration

```yaml
# GitHub Actions example
- name: Run integration tests
  run: make test-integration
```

The tests automatically handle cleanup even if they fail, ensuring no leftover resources.