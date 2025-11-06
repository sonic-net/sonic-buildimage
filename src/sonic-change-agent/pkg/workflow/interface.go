package workflow

import (
	"context"

	"k8s.io/apimachinery/pkg/apis/meta/v1/unstructured"
)

// Workflow defines the interface for different types of workflows
type Workflow interface {
	// Execute runs the workflow
	Execute(ctx context.Context, device *unstructured.Unstructured) error

	// GetName returns workflow identifier
	GetName() string
}