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
	// Extract required fields from device spec - new CRD structure
	osVersion, found, _ := unstructured.NestedString(device.Object, "spec", "osVersion")
	if !found || osVersion == "" {
		return fmt.Errorf("osVersion not specified in device spec")
	}

	firmwareProfile, _, _ := unstructured.NestedString(device.Object, "spec", "firmwareProfile")
	
	// For now, we construct the image URL based on osVersion and firmwareProfile
	// This logic may need to be updated based on actual image repository structure
	imageURL := w.constructImageURL(osVersion, firmwareProfile)
	downloadPath := defaultDownloadPath

	klog.InfoS("Executing preload workflow",
		"osVersion", osVersion,
		"firmwareProfile", firmwareProfile,
		"imageURL", imageURL,
		"downloadPath", downloadPath)

	// Execute single step: Transfer file using gNOI file service
	if err := w.gnoi.TransferToRemote(ctx, imageURL, downloadPath); err != nil {
		return fmt.Errorf("failed to transfer file: %w", err)
	}

	klog.InfoS("Preload workflow completed successfully",
		"osVersion", osVersion,
		"downloadPath", downloadPath)

	return nil
}

// constructImageURL builds the image URL based on osVersion and firmwareProfile
// This is a placeholder implementation - actual logic depends on image repository structure
func (w *PreloadWorkflow) constructImageURL(osVersion, firmwareProfile string) string {
	// TODO: Implement actual image URL construction logic
	// For now, return a placeholder URL
	return fmt.Sprintf("http://image-repo.example.com/sonic-%s-%s.bin", osVersion, firmwareProfile)
}