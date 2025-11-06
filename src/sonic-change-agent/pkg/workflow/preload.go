package workflow

import (
	"context"
	"fmt"

	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/gnoi"
	"k8s.io/apimachinery/pkg/apis/meta/v1/unstructured"
	"k8s.io/klog/v2"
)

const (
	defaultDownloadPath = "/tmp/sonic-image.bin"
)

// PreloadWorkflow implements the preload workflow using gnoi.file.TransferToRemote
type PreloadWorkflow struct {
	gnoi gnoi.Client
}

// NewPreloadWorkflow creates a new preload workflow
func NewPreloadWorkflow(gnoiClient gnoi.Client) *PreloadWorkflow {
	return &PreloadWorkflow{
		gnoi: gnoiClient,
	}
}

// GetName returns the workflow name
func (w *PreloadWorkflow) GetName() string {
	return "preload"
}

// Execute runs the preload workflow
func (w *PreloadWorkflow) Execute(ctx context.Context, device *unstructured.Unstructured) error {
	// Extract required fields from device spec
	spec, found, err := unstructured.NestedMap(device.Object, "spec", "os")
	if !found || err != nil {
		return fmt.Errorf("failed to get spec.os: %w", err)
	}

	imageURL, found, err := unstructured.NestedString(spec, "imageURL")
	if !found || err != nil {
		return fmt.Errorf("failed to get imageURL: %w", err)
	}

	downloadPath, found, _ := unstructured.NestedString(spec, "downloadPath")
	if !found || downloadPath == "" {
		downloadPath = defaultDownloadPath
	}

	klog.InfoS("Executing preload workflow",
		"imageURL", imageURL,
		"downloadPath", downloadPath)

	// Execute single step: Transfer file using gNOI file service
	if err := w.gnoi.TransferToRemote(ctx, imageURL, downloadPath); err != nil {
		return fmt.Errorf("failed to transfer file: %w", err)
	}

	klog.InfoS("Preload workflow completed successfully",
		"downloadPath", downloadPath)

	return nil
}