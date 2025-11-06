package workflow

import (
	"fmt"

	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/gnoi"
)

// NewWorkflow creates a workflow instance based on type
func NewWorkflow(workflowType string, gnoiClient gnoi.Client) (Workflow, error) {
	switch workflowType {
	case "preload":
		return NewPreloadWorkflow(gnoiClient), nil
	default:
		return nil, fmt.Errorf("unknown workflow type: %s", workflowType)
	}
}