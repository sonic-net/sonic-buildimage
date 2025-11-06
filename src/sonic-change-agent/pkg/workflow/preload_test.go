package workflow

import (
	"context"
	"testing"

	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/gnoi"
	"k8s.io/apimachinery/pkg/apis/meta/v1/unstructured"
)

func TestPreloadWorkflow_GetName(t *testing.T) {
	mockClient := gnoi.NewMockClient()
	workflow := NewPreloadWorkflow(mockClient)

	if workflow.GetName() != "preload" {
		t.Errorf("Expected workflow name 'preload', got '%s'", workflow.GetName())
	}
}

func TestPreloadWorkflow_Execute(t *testing.T) {
	mockClient := gnoi.NewMockClient()
	workflow := NewPreloadWorkflow(mockClient)

	// Create a mock device with required fields
	device := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"spec": map[string]interface{}{
				"os": map[string]interface{}{
					"imageURL":     "http://example.com/image.bin",
					"downloadPath": "/tmp/test-image.bin",
				},
			},
		},
	}

	// Execute workflow
	err := workflow.Execute(context.Background(), device)
	if err != nil {
		t.Fatalf("Expected no error, got: %v", err)
	}

	// Verify TransferToRemote was called
	if len(mockClient.TransferToRemoteCalls) != 1 {
		t.Fatalf("Expected 1 TransferToRemote call, got %d", len(mockClient.TransferToRemoteCalls))
	}

	call := mockClient.TransferToRemoteCalls[0]
	if call.SourceURL != "http://example.com/image.bin" {
		t.Errorf("Expected sourceURL 'http://example.com/image.bin', got '%s'", call.SourceURL)
	}
	if call.RemotePath != "/tmp/test-image.bin" {
		t.Errorf("Expected remotePath '/tmp/test-image.bin', got '%s'", call.RemotePath)
	}
}

func TestPreloadWorkflow_Execute_DefaultPath(t *testing.T) {
	mockClient := gnoi.NewMockClient()
	workflow := NewPreloadWorkflow(mockClient)

	// Create a mock device without downloadPath (should use default)
	device := &unstructured.Unstructured{
		Object: map[string]interface{}{
			"spec": map[string]interface{}{
				"os": map[string]interface{}{
					"imageURL": "http://example.com/image.bin",
				},
			},
		},
	}

	// Execute workflow
	err := workflow.Execute(context.Background(), device)
	if err != nil {
		t.Fatalf("Expected no error, got: %v", err)
	}

	// Verify default path was used
	if len(mockClient.TransferToRemoteCalls) != 1 {
		t.Fatalf("Expected 1 TransferToRemote call, got %d", len(mockClient.TransferToRemoteCalls))
	}

	call := mockClient.TransferToRemoteCalls[0]
	if call.RemotePath != "/tmp/sonic-image.bin" {
		t.Errorf("Expected default remotePath '/tmp/sonic-image.bin', got '%s'", call.RemotePath)
	}
}