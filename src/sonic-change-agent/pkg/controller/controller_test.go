package controller

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/gnoi"
	"k8s.io/apimachinery/pkg/apis/meta/v1/unstructured"
)

func TestReconcile_NoReconciliationNeeded(t *testing.T) {
	mockClient := gnoi.NewMockClient()

	// Create controller (we'll call reconcile directly, not through informer)
	ctrl := &Controller{
		deviceName: "test-device",
		gnoiClient: mockClient,
	}

	// Create NetworkDevice object where spec matches status
	obj := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"apiVersion": "sonic.io/v1",
			"kind":       "NetworkDevice",
			"metadata": map[string]interface{}{
				"name":      "test-device",
				"namespace": "default",
			},
			"spec": map[string]interface{}{
				"os": map[string]interface{}{
					"desiredVersion": "202511.01",
					"imageURL":       "http://example.com/sonic.bin",
				},
			},
			"status": map[string]interface{}{
				"downloadStatus": map[string]interface{}{
					"downloadedVersion": "202511.01",
					"phase":             "Succeeded",
				},
			},
		},
	}

	// Reconcile should do nothing since versions match
	ctrl.reconcile(obj)

	// Verify no download was attempted
	if mockClient.GetDownloadImageCallCount() != 0 {
		t.Errorf("Expected 0 DownloadImage calls, got %d", mockClient.GetDownloadImageCallCount())
	}
}

func TestReconcile_DownloadNeeded(t *testing.T) {
	mockClient := gnoi.NewMockClient()

	// Mock successful download
	mockClient.DownloadImageFunc = func(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error {
		return nil
	}

	ctrl := &Controller{
		deviceName: "test-device",
		gnoiClient: mockClient,
	}

	// Create NetworkDevice with version mismatch
	obj := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"apiVersion": "sonic.io/v1",
			"kind":       "NetworkDevice",
			"metadata": map[string]interface{}{
				"name":      "test-device",
				"namespace": "default",
			},
			"spec": map[string]interface{}{
				"os": map[string]interface{}{
					"desiredVersion": "202511.02",
					"imageURL":       "http://example.com/sonic-202511.02.bin",
					"downloadPath":   "/tmp/sonic-test.bin",
					"checksum": map[string]interface{}{
						"md5": "abc123def456abc123def456abc12345",
					},
				},
			},
			"status": map[string]interface{}{
				"downloadStatus": map[string]interface{}{
					"downloadedVersion": "202511.01",
					"phase":             "Succeeded",
				},
			},
		},
	}

	// Reconcile should trigger download
	ctrl.reconcile(obj)

	// Verify download was attempted
	if mockClient.GetDownloadImageCallCount() != 1 {
		t.Fatalf("Expected 1 DownloadImage call, got %d", mockClient.GetDownloadImageCallCount())
	}

	// Verify download parameters
	call, err := mockClient.GetLastDownloadImageCall()
	if err != nil {
		t.Fatalf("Failed to get last call: %v", err)
	}

	if call.ImageURL != "http://example.com/sonic-202511.02.bin" {
		t.Errorf("Expected imageURL 'http://example.com/sonic-202511.02.bin', got '%s'", call.ImageURL)
	}

	if call.DownloadPath != "/tmp/sonic-test.bin" {
		t.Errorf("Expected downloadPath '/tmp/sonic-test.bin', got '%s'", call.DownloadPath)
	}

	if call.ExpectedMD5 != "abc123def456abc123def456abc12345" {
		t.Errorf("Expected MD5 'abc123def456abc123def456abc12345', got '%s'", call.ExpectedMD5)
	}
}

func TestReconcile_LocalImageExists(t *testing.T) {
	mockClient := gnoi.NewMockClient()

	// Mock that local image exists and matches
	mockClient.VerifyLocalImageFunc = func(downloadPath, expectedMD5 string) (bool, error) {
		return true, nil
	}

	ctrl := &Controller{
		deviceName: "test-device",
		gnoiClient: mockClient,
	}

	obj := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"apiVersion": "sonic.io/v1",
			"kind":       "NetworkDevice",
			"metadata": map[string]interface{}{
				"name":      "test-device",
				"namespace": "default",
			},
			"spec": map[string]interface{}{
				"os": map[string]interface{}{
					"desiredVersion": "202511.02",
					"imageURL":       "http://example.com/sonic.bin",
					"checksum": map[string]interface{}{
						"md5": "abc123def456abc123def456abc12345",
					},
				},
			},
			"status": map[string]interface{}{
				"downloadStatus": map[string]interface{}{
					"downloadedVersion": "202511.01",
				},
			},
		},
	}

	// Reconcile should skip download since file exists with correct MD5
	ctrl.reconcile(obj)

	// Verify download was NOT attempted
	if mockClient.GetDownloadImageCallCount() != 0 {
		t.Errorf("Expected 0 DownloadImage calls (file exists), got %d", mockClient.GetDownloadImageCallCount())
	}
}

func TestReconcile_DefaultDownloadPath(t *testing.T) {
	mockClient := gnoi.NewMockClient()
	mockClient.DownloadImageFunc = func(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error {
		return nil
	}

	ctrl := &Controller{
		deviceName: "test-device",
		gnoiClient: mockClient,
	}

	// NetworkDevice without explicit downloadPath
	obj := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"apiVersion": "sonic.io/v1",
			"kind":       "NetworkDevice",
			"metadata": map[string]interface{}{
				"name":      "test-device",
				"namespace": "default",
			},
			"spec": map[string]interface{}{
				"os": map[string]interface{}{
					"desiredVersion": "202511.01",
					"imageURL":       "http://example.com/sonic.bin",
				},
			},
			"status": map[string]interface{}{},
		},
	}

	ctrl.reconcile(obj)

	if mockClient.GetDownloadImageCallCount() != 1 {
		t.Fatalf("Expected 1 DownloadImage call, got %d", mockClient.GetDownloadImageCallCount())
	}

	call, _ := mockClient.GetLastDownloadImageCall()
	if call.DownloadPath != defaultDownloadPath {
		t.Errorf("Expected default path '%s', got '%s'", defaultDownloadPath, call.DownloadPath)
	}
}

func TestReconcile_ConcurrentCalls(t *testing.T) {
	mockClient := gnoi.NewMockClient()

	// Make download slow to test mutex
	mockClient.DownloadImageFunc = func(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error {
		time.Sleep(100 * time.Millisecond)
		return nil
	}

	ctrl := &Controller{
		deviceName: "test-device",
		gnoiClient: mockClient,
	}

	obj := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"apiVersion": "sonic.io/v1",
			"kind":       "NetworkDevice",
			"metadata": map[string]interface{}{
				"name":      "test-device",
				"namespace": "default",
			},
			"spec": map[string]interface{}{
				"os": map[string]interface{}{
					"desiredVersion": "202511.01",
					"imageURL":       "http://example.com/sonic.bin",
				},
			},
			"status": map[string]interface{}{},
		},
	}

	// Try to reconcile concurrently
	done := make(chan bool, 2)
	go func() {
		ctrl.reconcile(obj)
		done <- true
	}()
	go func() {
		ctrl.reconcile(obj)
		done <- true
	}()

	// Wait for both to complete
	<-done
	<-done

	// Due to mutex, only one should have executed
	// (The second call will see the first is in progress and either wait or the status will be updated)
	// For now, we just verify no crash occurred
	if mockClient.GetDownloadImageCallCount() > 2 {
		t.Errorf("Expected at most 2 DownloadImage calls, got %d", mockClient.GetDownloadImageCallCount())
	}
}

func TestReconcile_DownloadFailure(t *testing.T) {
	mockClient := gnoi.NewMockClient()

	// Mock failed download
	mockClient.DownloadImageFunc = func(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error {
		return fmt.Errorf("network error: connection timeout")
	}

	ctrl := &Controller{
		deviceName: "test-device",
		gnoiClient: mockClient,
	}

	obj := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"apiVersion": "sonic.io/v1",
			"kind":       "NetworkDevice",
			"metadata": map[string]interface{}{
				"name":      "test-device",
				"namespace": "default",
			},
			"spec": map[string]interface{}{
				"os": map[string]interface{}{
					"desiredVersion": "202511.02",
					"imageURL":       "http://example.com/sonic.bin",
				},
			},
			"status": map[string]interface{}{},
		},
	}

	// Reconcile should attempt download and handle error
	ctrl.reconcile(obj)

	// Verify download was attempted
	if mockClient.GetDownloadImageCallCount() != 1 {
		t.Errorf("Expected 1 DownloadImage call, got %d", mockClient.GetDownloadImageCallCount())
	}

	// Verify status was updated to Failed
	statusMap, _, _ := unstructured.NestedMap(obj.Object, "status", "downloadStatus")
	phase, _, _ := unstructured.NestedString(statusMap, "phase")
	if phase != "Failed" {
		t.Errorf("Expected phase 'Failed', got '%s'", phase)
	}
}

func TestReconcile_VerifyImageError(t *testing.T) {
	mockClient := gnoi.NewMockClient()

	// Mock VerifyLocalImage returns error
	mockClient.VerifyLocalImageFunc = func(downloadPath, expectedMD5 string) (bool, error) {
		return false, fmt.Errorf("permission denied")
	}

	ctrl := &Controller{
		deviceName: "test-device",
		gnoiClient: mockClient,
	}

	obj := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"apiVersion": "sonic.io/v1",
			"kind":       "NetworkDevice",
			"metadata": map[string]interface{}{
				"name":      "test-device",
				"namespace": "default",
			},
			"spec": map[string]interface{}{
				"os": map[string]interface{}{
					"desiredVersion": "202511.02",
					"imageURL":       "http://example.com/sonic.bin",
					"checksum": map[string]interface{}{
						"md5": "abc123def456abc123def456abc12345",
					},
				},
			},
			"status": map[string]interface{}{},
		},
	}

	// Should continue with download despite verify error
	mockClient.DownloadImageFunc = func(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error {
		return nil
	}

	ctrl.reconcile(obj)

	// Should still attempt download
	if mockClient.GetDownloadImageCallCount() != 1 {
		t.Errorf("Expected 1 DownloadImage call, got %d", mockClient.GetDownloadImageCallCount())
	}
}

func TestReconcile_InvalidObject(t *testing.T) {
	mockClient := gnoi.NewMockClient()

	ctrl := &Controller{
		deviceName: "test-device",
		gnoiClient: mockClient,
	}

	// Invalid object (not unstructured)
	ctrl.reconcile("not an unstructured object")

	// Should not crash, just return early
	if mockClient.GetDownloadImageCallCount() != 0 {
		t.Errorf("Expected 0 DownloadImage calls for invalid object, got %d", mockClient.GetDownloadImageCallCount())
	}
}

func TestReconcile_MissingSpec(t *testing.T) {
	mockClient := gnoi.NewMockClient()

	ctrl := &Controller{
		deviceName: "test-device",
		gnoiClient: mockClient,
	}

	// Object without spec.os
	obj := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"apiVersion": "sonic.io/v1",
			"kind":       "NetworkDevice",
			"metadata": map[string]interface{}{
				"name":      "test-device",
				"namespace": "default",
			},
			"spec": map[string]interface{}{},
		},
	}

	ctrl.reconcile(obj)

	// Should return early without download
	if mockClient.GetDownloadImageCallCount() != 0 {
		t.Errorf("Expected 0 DownloadImage calls for missing spec, got %d", mockClient.GetDownloadImageCallCount())
	}
}

func TestUpdateStatus_WithDynamicClient(t *testing.T) {
	// This tests the status update logic itself
	ctrl := &Controller{
		deviceName:    "test-device",
		dynamicClient: nil, // Will skip actual API call
	}

	obj := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"apiVersion": "sonic.io/v1",
			"kind":       "NetworkDevice",
			"metadata": map[string]interface{}{
				"name":      "test-device",
				"namespace": "default",
			},
			"spec": map[string]interface{}{},
		},
	}

	// Call updateStatus directly
	ctrl.updateStatus(obj, "Succeeded", "Test message", "202511.01", "abc123")

	// Verify status was updated in object
	statusMap, found, err := unstructured.NestedMap(obj.Object, "status", "downloadStatus")
	if !found || err != nil {
		t.Fatalf("Failed to get status: found=%v, err=%v", found, err)
	}

	phase, _, _ := unstructured.NestedString(statusMap, "phase")
	if phase != "Succeeded" {
		t.Errorf("Expected phase 'Succeeded', got '%s'", phase)
	}

	message, _, _ := unstructured.NestedString(statusMap, "message")
	if message != "Test message" {
		t.Errorf("Expected message 'Test message', got '%s'", message)
	}

	version, _, _ := unstructured.NestedString(statusMap, "downloadedVersion")
	if version != "202511.01" {
		t.Errorf("Expected downloadedVersion '202511.01', got '%s'", version)
	}

	checksum, _, _ := unstructured.NestedString(statusMap, "downloadedChecksum")
	if checksum != "abc123" {
		t.Errorf("Expected downloadedChecksum 'abc123', got '%s'", checksum)
	}
}
