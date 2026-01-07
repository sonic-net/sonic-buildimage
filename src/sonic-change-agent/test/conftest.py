"""
pytest configuration and fixtures for sonic-change-agent integration tests.

Provides setup/teardown for cluster, image, and deployment.
"""

import pytest
import subprocess
import time
import os
import tempfile
import yaml
from datetime import datetime


def run_cmd(cmd, capture=True, cwd=None):
    """Run command and return result."""
    if isinstance(cmd, str):
        cmd = cmd.split()
    
    if capture:
        return subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)
    else:
        return subprocess.run(cmd, cwd=cwd)


def kubectl(*args):
    """Helper to run kubectl commands in test cluster."""
    cmd = ["minikube", "kubectl", "--profile", "sonic-test", "--"] + list(args)
    return run_cmd(cmd)


def collect_logs(test_name):
    """Collect container logs for the test."""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_dir = os.path.join("test_logs", f"{test_name}_{timestamp}")
    os.makedirs(log_dir, exist_ok=True)
    
    print(f"\n📋 Collecting logs to: {log_dir}")
    
    # Get all pods
    result = kubectl("get", "pods", "-o", "json")
    if result.returncode != 0:
        print(f"Failed to get pods: {result.stderr}")
        return log_dir
    
    import json
    try:
        pods_data = json.loads(result.stdout)
        pods = pods_data.get("items", [])
    except json.JSONDecodeError:
        print("Failed to parse pods JSON")
        return log_dir
    
    # Collect logs from each pod
    for pod in pods:
        pod_name = pod["metadata"]["name"]
        pod_namespace = pod["metadata"].get("namespace", "default")
        
        print(f"  📜 Collecting logs from {pod_name}...")
        
        # Get pod logs
        result = kubectl("logs", pod_name, "-n", pod_namespace, "--all-containers=true")
        if result.returncode == 0:
            log_file = os.path.join(log_dir, f"{pod_name}.log")
            with open(log_file, "w") as f:
                f.write(f"Pod: {pod_name}\n")
                f.write(f"Namespace: {pod_namespace}\n") 
                f.write(f"Collected: {datetime.now().isoformat()}\n")
                f.write("=" * 60 + "\n")
                f.write(result.stdout)
        else:
            print(f"    ⚠️  Failed to get logs from {pod_name}: {result.stderr}")
        
        # Get pod description
        result = kubectl("describe", "pod", pod_name, "-n", pod_namespace)
        if result.returncode == 0:
            desc_file = os.path.join(log_dir, f"{pod_name}_describe.txt")
            with open(desc_file, "w") as f:
                f.write(result.stdout)
    
    # Collect cluster-wide information
    cluster_info = {
        "nodes": kubectl("get", "nodes", "-o", "wide"),
        "pods": kubectl("get", "pods", "-o", "wide", "--all-namespaces"),
        "services": kubectl("get", "services", "-o", "wide", "--all-namespaces"),
        "networkdevices": kubectl("get", "networkdevices", "-o", "yaml"),
        "crd": kubectl("get", "crd", "-o", "yaml"),
        "events": kubectl("get", "events", "--sort-by='.lastTimestamp'")
    }
    
    for info_name, result in cluster_info.items():
        if result.returncode == 0:
            info_file = os.path.join(log_dir, f"cluster_{info_name}.txt")
            with open(info_file, "w") as f:
                f.write(f"Command: kubectl get {info_name}\n")
                f.write(f"Collected: {datetime.now().isoformat()}\n")
                f.write("=" * 60 + "\n")
                f.write(result.stdout)
    
    # Create summary file
    summary_file = os.path.join(log_dir, "README.txt")
    with open(summary_file, "w") as f:
        f.write(f"Test Logs Collection\n")
        f.write(f"==================\n\n")
        f.write(f"Test: {test_name}\n")
        f.write(f"Timestamp: {timestamp}\n")
        f.write(f"Collected: {datetime.now().isoformat()}\n\n")
        f.write("Files:\n")
        for file in sorted(os.listdir(log_dir)):
            if file != "README.txt":
                f.write(f"  - {file}\n")
    
    print(f"✅ Logs collected in: {log_dir}")
    return log_dir


@pytest.fixture(scope="session")
def cluster():
    """Create and manage test cluster lifecycle."""
    cluster_name = "sonic-test"
    
    print(f"\n🏗️  Setting up test cluster: {cluster_name}")
    
    # Clean up existing cluster
    print("Cleaning up any existing cluster...")
    run_cmd(["minikube", "delete", "--profile", cluster_name])
    
    # Create cluster
    print("Creating minikube cluster...")
    result = run_cmd([
        "minikube", "start", 
        "--profile", cluster_name,
        "--driver=docker",
        "--kubernetes-version=v1.29.0"
    ])
    
    if result.returncode != 0:
        pytest.fail(f"Failed to create cluster: {result.stderr}")
    
    # Wait for cluster ready
    print("Waiting for cluster to be ready...")
    for i in range(30):
        result = kubectl("get", "nodes")
        if result.returncode == 0 and "Ready" in result.stdout:
            print("✅ Cluster is ready")
            break
        time.sleep(5)
    else:
        pytest.fail("Cluster not ready after 2.5 minutes")
    
    yield cluster_name
    
    # Cleanup
    print(f"\n🧹 Cleaning up cluster: {cluster_name}")
    run_cmd(["minikube", "delete", "--profile", cluster_name])


@pytest.fixture(scope="session")
def docker_image():
    """Build test Docker image."""
    image_name = "sonic-change-agent:test"
    
    # Check if we should skip Docker build for faster iteration
    if os.getenv("SKIP_DOCKER_BUILD"):
        print(f"\n🐳 Skipping Docker build (SKIP_DOCKER_BUILD=1)")
        # Check if image already exists
        result = run_cmd(["docker", "inspect", image_name])
        if result.returncode != 0:
            pytest.fail(f"Image {image_name} not found and SKIP_DOCKER_BUILD=1. Run 'make test-integration' first.")
        print(f"✅ Using existing Docker image: {image_name}")
        yield image_name
        return
    
    print(f"\n🐳 Building Docker image: {image_name}")
    
    project_root = os.path.dirname(os.path.abspath(__file__))
    dockerfile_path = os.path.join(project_root, "..", "Dockerfile")
    
    if not os.path.exists(dockerfile_path):
        pytest.fail(f"Dockerfile not found at {dockerfile_path}")
    
    result = run_cmd(["docker", "build", "-t", image_name, ".."], cwd=project_root)
    if result.returncode != 0:
        pytest.fail(f"Failed to build image: {result.stderr}")
    
    print("✅ Docker image built")
    
    yield image_name
    
    # Only cleanup if we built the image (don't remove existing images for test-quick)
    if not os.getenv("SKIP_DOCKER_BUILD"):
        print(f"\n🧹 Removing test image: {image_name}")
        run_cmd(["docker", "rmi", image_name])


@pytest.fixture(scope="session") 
def redis_deployment(cluster):
    """Deploy Redis with CONFIG_DB."""
    print("\n📦 Deploying Redis...")
    
    # Redis manifest
    redis_manifest = {
        "apiVersion": "apps/v1",
        "kind": "Deployment",
        "metadata": {
            "name": "redis",
            "labels": {"app": "redis"}
        },
        "spec": {
            "replicas": 1,
            "selector": {"matchLabels": {"app": "redis"}},
            "template": {
                "metadata": {"labels": {"app": "redis"}},
                "spec": {
                    "hostNetwork": True,
                    "containers": [{
                        "name": "redis",
                        "image": "redis:7-alpine",
                        "ports": [{"containerPort": 6379}],
                        "command": ["redis-server", "--save", "", "--appendonly", "no"]
                    }]
                }
            }
        }
    }
    
    # Deploy Redis
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        yaml.dump(redis_manifest, f)
        manifest_path = f.name
    
    try:
        result = kubectl("apply", "-f", manifest_path)
        if result.returncode != 0:
            pytest.fail(f"Failed to deploy Redis: {result.stderr}")
        
        # Wait for Redis
        print("Waiting for Redis to be ready...")
        for i in range(12):
            result = kubectl("get", "pods", "-l", "app=redis")
            if result.returncode == 0 and "Running" in result.stdout:
                break
            time.sleep(5)
        else:
            pytest.fail("Redis not ready")
        
        # Configure Redis - get node IP
        result = kubectl("get", "nodes", "-o", "jsonpath={.items[0].status.addresses[0].address}")
        if result.returncode != 0:
            pytest.fail("Failed to get node IP")
        
        node_ip = result.stdout.strip()
        print(f"Using node IP: {node_ip}")
        
        # Set CONFIG_DB data in database 4
        config_commands = [
            f"redis-cli -n 4 HSET 'KUBERNETES_MASTER|SERVER' ip '{node_ip}' port '8443' insecure 'False' disable 'False'",
            f"redis-cli -n 4 HSET 'GNMI|gnmi' port '8080' client_auth 'false'"
        ]
        
        for cmd in config_commands:
            result = kubectl("exec", "deployment/redis", "--", "sh", "-c", cmd)
            if result.returncode != 0:
                pytest.fail(f"Failed to set Redis config: {cmd}")
        
        print("✅ Redis deployed and configured")
        
        yield "redis"
        
    finally:
        # Cleanup
        os.unlink(manifest_path)
        print("\n🧹 Cleaning up Redis...")
        kubectl("delete", "deployment", "redis", "--ignore-not-found=true")


@pytest.fixture(scope="session")
def sonic_deployment(cluster, docker_image, redis_deployment):
    """Deploy sonic-change-agent."""
    image_name = docker_image
    
    print(f"\n🚀 Deploying sonic-change-agent with image: {image_name}")
    
    # Load image into cluster
    print("Loading Docker image into cluster...")
    result = run_cmd(["minikube", "image", "load", image_name, "--profile", cluster])
    if result.returncode != 0:
        pytest.fail(f"Failed to load image: {result.stderr}")
    
    project_root = os.path.dirname(os.path.abspath(__file__))
    
    # Deploy CRD
    print("Deploying CRD...")
    crd_path = os.path.join(project_root, "..", "manifests", "crd.yaml")
    result = kubectl("apply", "-f", crd_path)
    if result.returncode != 0:
        pytest.fail(f"Failed to deploy CRD: {result.stderr}")
    
    # Wait for CRD
    for i in range(12):
        result = kubectl("get", "crd", "networkdevices.sonic.k8s.io")
        if result.returncode == 0:
            break
        time.sleep(5)
    else:
        pytest.fail("CRD not established")
    
    # Deploy RBAC
    print("Deploying RBAC...")
    rbac_path = os.path.join(project_root, "..", "manifests", "rbac.yaml")
    result = kubectl("apply", "-f", rbac_path)
    if result.returncode != 0:
        pytest.fail(f"Failed to deploy RBAC: {result.stderr}")
    
    # Deploy DaemonSet with correct image
    print("Deploying DaemonSet...")
    daemonset_path = os.path.join(project_root, "..", "manifests", "daemonset.yaml")
    with open(daemonset_path, "r") as f:
        daemonset_content = f.read()
    
    # Update image name
    updated_content = daemonset_content.replace("sonic-change-agent:latest", image_name)
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
        f.write(updated_content)
        daemonset_temp_path = f.name
    
    try:
        result = kubectl("apply", "-f", daemonset_temp_path)
        if result.returncode != 0:
            pytest.fail(f"Failed to deploy DaemonSet: {result.stderr}")
        
        # Wait for pod
        print("Waiting for sonic-change-agent to be ready...")
        for i in range(24):
            result = kubectl("get", "pods", "-l", "app=sonic-change-agent")
            if result.returncode == 0 and "Running" in result.stdout:
                # Additional check: ensure controller is synced
                result = kubectl("logs", "daemonset/sonic-change-agent", "--tail=20")
                if result.returncode == 0 and "Cache synced successfully" in result.stdout:
                    print("✅ sonic-change-agent deployed and ready")
                    break
            time.sleep(5)
        else:
            pytest.fail("sonic-change-agent not ready")
        
        yield "sonic-change-agent"
        
    finally:
        # Cleanup
        os.unlink(daemonset_temp_path)
        print("\n🧹 Cleaning up sonic-change-agent...")
        kubectl("delete", "daemonset", "sonic-change-agent", "--ignore-not-found=true")
        kubectl("delete", "-f", rbac_path, "--ignore-not-found=true") 
        kubectl("delete", "-f", crd_path, "--ignore-not-found=true")


@pytest.fixture
def network_device():
    """Factory to create and cleanup NetworkDevice resources."""
    created_devices = []
    
    def _create_device(name, **spec_kwargs):
        device_spec = {
            "apiVersion": "sonic.k8s.io/v1",
            "kind": "NetworkDevice", 
            "metadata": {"name": name},
            "spec": {
                "type": "leafRouter",
                "osVersion": "202505.01", 
                "firmwareProfile": "SONiC-Test-Profile",
                "operation": "OSUpgrade",
                "operationAction": "PreloadImage",
                **spec_kwargs
            }
        }
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
            yaml.dump(device_spec, f)
            device_path = f.name
        
        try:
            result = kubectl("apply", "-f", device_path)
            if result.returncode != 0:
                pytest.fail(f"Failed to create NetworkDevice {name}: {result.stderr}")
            
            created_devices.append(name)
            print(f"✅ Created NetworkDevice: {name}")
            return name
            
        finally:
            os.unlink(device_path)
    
    yield _create_device
    
    # Cleanup
    for device_name in created_devices:
        kubectl("delete", "networkdevice", device_name, "--ignore-not-found=true")
        print(f"🧹 Deleted NetworkDevice: {device_name}")


@pytest.fixture(autouse=True)
def auto_collect_logs(request, sonic_deployment):
    """Automatically collect logs after each test."""
    yield  # Run the test
    
    # Collect logs after test completion
    test_name = request.node.name
    log_dir = collect_logs(test_name)
    
    # Add log directory to test report
    if hasattr(request.node, 'user_properties'):
        request.node.user_properties.append(("log_directory", log_dir))


def pytest_configure(config):
    """Configure pytest with custom markers."""
    config.addinivalue_line("markers", "workflow: marks tests that validate workflow execution")
    config.addinivalue_line("markers", "slow: marks tests as slow")


def pytest_runtest_makereport(item, call):
    """Add log collection info to test reports."""
    if call.when == "call":
        log_dir = None
        for name, value in getattr(item, 'user_properties', []):
            if name == "log_directory":
                log_dir = value
                break
        
        if log_dir:
            print(f"\n📋 Test logs saved to: {log_dir}")
            
            # If test failed, show quick summary
            if call.excinfo is not None:
                summary_file = os.path.join(log_dir, "README.txt")
                if os.path.exists(summary_file):
                    print(f"💡 For debugging, check logs in: {log_dir}")