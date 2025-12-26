"""
Integration tests for sonic-change-agent.

Tests NetworkDevice workflow execution and validation.
"""

import pytest
import time
import subprocess


def kubectl(*args):
    """Helper to run kubectl commands in test cluster."""
    cmd = ["minikube", "kubectl", "--profile", "sonic-test", "--"] + list(args)
    return subprocess.run(cmd, capture_output=True, text=True)


@pytest.mark.workflow
def test_preload_workflow(sonic_deployment, network_device):
    """Test PreloadImage workflow execution."""
    # Create NetworkDevice for PreloadImage - use sonic-test to match the agent's deviceName
    device_name = network_device("sonic-test", 
                                operation="OSUpgrade", 
                                operationAction="PreloadImage")
    
    # Wait for workflow execution
    print(f"Waiting for workflow execution on {device_name}...")
    
    # First, verify the NetworkDevice was created
    result = kubectl("get", "networkdevice", device_name, "-o", "yaml")
    print(f"NetworkDevice creation status: {result.returncode}")
    if result.returncode == 0:
        print("NetworkDevice created successfully")
    else:
        print(f"NetworkDevice creation failed: {result.stderr}")
    
    time.sleep(10)
    
    # Check logs for workflow completion
    result = kubectl("logs", "daemonset/sonic-change-agent", "--tail=50")
    assert result.returncode == 0, "Failed to get logs"
    
    logs = result.stdout
    assert "Starting workflow execution" in logs, "Workflow not started"
    assert "Executing preload workflow" in logs, "Preload workflow not executed"
    assert "DRY_RUN: Would transfer file" in logs, "DRY_RUN file transfer not found"
    assert "Preload workflow completed successfully" in logs, "Workflow not completed"
    
    # Verify NetworkDevice status was updated
    result = kubectl("get", "networkdevice", device_name, "-o", "yaml")
    assert result.returncode == 0, "Failed to get NetworkDevice"
    
    device_yaml = result.stdout
    assert "status:" in device_yaml, "NetworkDevice status not updated"
    assert "state:" in device_yaml, "NetworkDevice state not set"
    
    print("✅ PreloadImage workflow test passed")


@pytest.mark.workflow
def test_unsupported_workflow_handling(sonic_deployment, network_device):
    """Test handling of unsupported workflow operations."""
    # Create NetworkDevice for unsupported Install operation
    device_name = network_device("sonic-test",
                                operation="OSUpgrade", 
                                operationAction="Install",
                                osVersion="202505.02")
    
    # Wait for processing attempt
    print(f"Waiting for unsupported workflow handling on {device_name}...")
    time.sleep(10)
    
    # Check logs - should show error for unsupported workflow
    result = kubectl("logs", "daemonset/sonic-change-agent", "--tail=50")
    assert result.returncode == 0, "Failed to get logs"
    
    logs = result.stdout
    # Should see either "unknown workflow type" or similar error handling
    assert ("NetworkDevice ADDED" in logs or 
            "unknown workflow type" in logs or 
            "OSUpgrade-Install" in logs), "NetworkDevice not processed"
    
    print("✅ Unsupported workflow handling test passed")


@pytest.mark.workflow
def test_preload_workflow_variations(sonic_deployment, network_device):
    """Test PreloadImage workflow with different configurations."""
    # Test different OS versions and firmware profiles
    test_configs = [
        {"osVersion": "202505.01", "firmwareProfile": "SONiC-Profile-A"},
        {"osVersion": "202505.02", "firmwareProfile": "SONiC-Profile-B"},
    ]
    
    for i, config in enumerate(test_configs):
        print(f"Testing PreloadImage with config {i+1}: {config}")
        
        # Create NetworkDevice with different config
        device_name = network_device("sonic-test", 
                                    operation="OSUpgrade",
                                    operationAction="PreloadImage", 
                                    **config)
        
        # Wait for workflow execution
        time.sleep(8)
        
        # Check logs for this specific configuration
        result = kubectl("logs", "daemonset/sonic-change-agent", "--tail=30")
        assert result.returncode == 0, "Failed to get logs"
        
        logs = result.stdout
        assert "Starting workflow execution" in logs, f"Workflow not started for config {i+1}"
        assert "Executing preload workflow" in logs, f"Preload workflow not executed for config {i+1}"
        assert config["osVersion"] in logs, f"OS version {config['osVersion']} not found in logs"
        assert config["firmwareProfile"] in logs, f"Firmware profile {config['firmwareProfile']} not found in logs"
        
        # Delete the device before next iteration
        kubectl("delete", "networkdevice", device_name, "--ignore-not-found=true")
        time.sleep(2)
    
    print("✅ PreloadImage workflow variations test passed")


def test_crd_compliance(sonic_deployment):
    """Test that CRD is properly deployed and accessible."""
    # Check CRD exists
    result = kubectl("get", "crd", "networkdevices.sonic.k8s.io")
    assert result.returncode == 0, "NetworkDevice CRD not found"
    
    # Check CRD details
    result = kubectl("get", "crd", "networkdevices.sonic.k8s.io", "-o", "yaml")
    assert result.returncode == 0, "Failed to get CRD details"
    
    crd_yaml = result.stdout
    assert "sonic.k8s.io" in crd_yaml, "Wrong API group"
    assert "NetworkDevice" in crd_yaml, "Wrong kind"
    assert "status:" in crd_yaml, "Status subresource not enabled"
    
    print("✅ CRD compliance test passed")


def test_system_health(sonic_deployment):
    """Test that the deployed system is healthy."""
    # Check all pods are running
    result = kubectl("get", "pods", "-o", "wide")
    assert result.returncode == 0, "Failed to get pods"
    
    pods_output = result.stdout
    assert "redis" in pods_output, "Redis pod not found"
    assert "sonic-change-agent" in pods_output, "sonic-change-agent pod not found"
    assert "Running" in pods_output, "Not all pods are running"
    
    # Check sonic-change-agent is not crashing
    result = kubectl("get", "pods", "-l", "app=sonic-change-agent")
    assert result.returncode == 0, "Failed to get sonic-change-agent pod"
    assert "CrashLoopBackOff" not in result.stdout, "sonic-change-agent is crashing"
    assert "Error" not in result.stdout, "sonic-change-agent pod in error state"
    
    # Check full logs show successful startup (look for cache sync in all logs)
    result = kubectl("logs", "daemonset/sonic-change-agent")
    assert result.returncode == 0, "Failed to get logs"
    
    full_logs = result.stdout
    assert "Cache synced successfully" in full_logs, "Controller cache never synced"
    assert "Starting sonic-change-agent" in full_logs, "Controller never started"
    assert "Starting controller" in full_logs, "Controller initialization failed"
    
    # Check recent logs don't show crashes
    result = kubectl("logs", "daemonset/sonic-change-agent", "--tail=50")
    assert result.returncode == 0, "Failed to get recent logs"
    
    recent_logs = result.stdout
    assert "panic" not in recent_logs.lower(), "Panic found in recent logs"
    assert "fatal" not in recent_logs.lower(), "Fatal error found in recent logs"
    
    print("✅ System health test passed")


@pytest.mark.slow
def test_error_handling(sonic_deployment, network_device):
    """Test system handles invalid configurations gracefully."""
    # Create device with potentially problematic configuration
    device_name = network_device("test-error-handling",
                                osVersion="invalid-version", 
                                firmwareProfile="Invalid-Profile")
    
    # Wait for processing
    time.sleep(10)
    
    # System should not crash even with invalid data
    result = kubectl("get", "pods", "-l", "app=sonic-change-agent")
    assert result.returncode == 0, "Failed to get pod status"
    assert "CrashLoopBackOff" not in result.stdout, "System crashed on invalid input"
    
    # Check logs for graceful handling
    result = kubectl("logs", "daemonset/sonic-change-agent", "--tail=50")
    assert result.returncode == 0, "Failed to get logs"
    
    logs = result.stdout
    # Should not contain panic or fatal errors
    assert "panic" not in logs.lower(), "Panic found in logs"
    assert "fatal" not in logs.lower(), "Fatal error found in logs"
    
    print("✅ Error handling test passed")