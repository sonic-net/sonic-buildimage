package workflow

import (
	"testing"

	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/gnoi"
)

func TestNewWorkflow_Preload(t *testing.T) {
	mockClient := gnoi.NewMockClient()
	
	workflow, err := NewWorkflow("preload", mockClient)
	if err != nil {
		t.Fatalf("Expected no error for 'preload' workflow, got: %v", err)
	}
	
	if workflow == nil {
		t.Fatal("Expected workflow to be created, got nil")
	}
	
	if workflow.GetName() != "preload" {
		t.Errorf("Expected workflow name 'preload', got '%s'", workflow.GetName())
	}
}

func TestNewWorkflow_OSUpgradePreloadImage(t *testing.T) {
	mockClient := gnoi.NewMockClient()
	
	workflow, err := NewWorkflow("OSUpgrade-PreloadImage", mockClient)
	if err != nil {
		t.Fatalf("Expected no error for 'OSUpgrade-PreloadImage' workflow, got: %v", err)
	}
	
	if workflow == nil {
		t.Fatal("Expected workflow to be created, got nil")
	}
	
	if workflow.GetName() != "preload" {
		t.Errorf("Expected workflow name 'preload' (implementation), got '%s'", workflow.GetName())
	}
}

func TestNewWorkflow_UnknownType(t *testing.T) {
	mockClient := gnoi.NewMockClient()
	
	workflow, err := NewWorkflow("unknown", mockClient)
	if err == nil {
		t.Fatal("Expected error for unknown workflow type, got nil")
	}
	
	if workflow != nil {
		t.Error("Expected workflow to be nil for unknown type")
	}
	
	expectedMsg := "unknown workflow type: unknown"
	if err.Error() != expectedMsg {
		t.Errorf("Expected error message '%s', got '%s'", expectedMsg, err.Error())
	}
}