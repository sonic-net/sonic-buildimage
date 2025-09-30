package controller

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/gnoi"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/apis/meta/v1/unstructured"
	"k8s.io/apimachinery/pkg/fields"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/runtime/schema"
	"k8s.io/apimachinery/pkg/watch"
	"k8s.io/client-go/dynamic"
	"k8s.io/client-go/rest"
	"k8s.io/client-go/tools/cache"
	"k8s.io/klog/v2"
)

const (
	defaultDownloadPath = "/tmp/sonic-image.bin"
)

// Controller manages NetworkDevice CRDs for this node
type Controller struct {
	deviceName    string
	gnoiClient    gnoi.Client
	dynamicClient dynamic.Interface
	informer      cache.SharedIndexInformer

	// Prevents concurrent reconciliations
	reconcileMutex sync.Mutex
}

// NewController creates a new controller
func NewController(deviceName string, gnoiClient gnoi.Client, kubeConfig *rest.Config) (*Controller, error) {
	dynamicClient, err := dynamic.NewForConfig(kubeConfig)
	if err != nil {
		return nil, fmt.Errorf("failed to create dynamic client: %w", err)
	}

	controller := &Controller{
		deviceName:    deviceName,
		gnoiClient:    gnoiClient,
		dynamicClient: dynamicClient,
	}

	if err := controller.setupInformer(); err != nil {
		return nil, fmt.Errorf("failed to setup informer: %w", err)
	}

	return controller, nil
}

// setupInformer creates the informer for NetworkDevice CRDs
func (c *Controller) setupInformer() error {
	gvr := schema.GroupVersionResource{
		Group:    "sonic.io",
		Version:  "v1",
		Resource: "networkdevices",
	}

	// Watch only our device
	fieldSelector := fields.OneTermEqualSelector("metadata.name", c.deviceName)

	listWatch := &cache.ListWatch{
		ListFunc: func(options metav1.ListOptions) (runtime.Object, error) {
			options.FieldSelector = fieldSelector.String()
			return c.dynamicClient.Resource(gvr).Namespace("default").List(context.TODO(), options)
		},
		WatchFunc: func(options metav1.ListOptions) (watch.Interface, error) {
			options.FieldSelector = fieldSelector.String()
			return c.dynamicClient.Resource(gvr).Namespace("default").Watch(context.TODO(), options)
		},
	}

	c.informer = cache.NewSharedIndexInformer(
		listWatch,
		&unstructured.Unstructured{},
		5*time.Minute, // Resync period
		cache.Indexers{},
	)

	c.informer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc:    c.onAdd,
		UpdateFunc: c.onUpdate,
		DeleteFunc: c.onDelete,
	})

	return nil
}

// Run starts the controller
func (c *Controller) Run(ctx context.Context) error {
	klog.InfoS("Starting controller", "device", c.deviceName)

	go c.informer.Run(ctx.Done())

	klog.InfoS("Waiting for cache sync")
	if !cache.WaitForCacheSync(ctx.Done(), c.informer.HasSynced) {
		return fmt.Errorf("failed to sync cache")
	}
	klog.InfoS("Cache synced successfully")

	<-ctx.Done()
	klog.InfoS("Controller stopped")
	return nil
}

// Event handlers
func (c *Controller) onAdd(obj interface{}) {
	klog.InfoS("NetworkDevice ADDED", "device", c.deviceName)
	c.reconcile(obj)
}

func (c *Controller) onUpdate(oldObj, newObj interface{}) {
	klog.InfoS("NetworkDevice UPDATED", "device", c.deviceName)
	c.reconcile(newObj)
}

func (c *Controller) onDelete(obj interface{}) {
	klog.InfoS("NetworkDevice DELETED", "device", c.deviceName)
}

// reconcile is the main reconciliation logic
func (c *Controller) reconcile(obj interface{}) {
	// Prevent concurrent reconciliations
	c.reconcileMutex.Lock()
	defer c.reconcileMutex.Unlock()

	u, ok := obj.(*unstructured.Unstructured)
	if !ok {
		klog.ErrorS(nil, "Failed to convert object to unstructured")
		return
	}

	// Extract spec fields
	spec, found, err := unstructured.NestedMap(u.Object, "spec", "os")
	if !found || err != nil {
		klog.ErrorS(err, "Failed to get spec.os", "found", found)
		return
	}

	desiredVersion, _, _ := unstructured.NestedString(spec, "desiredVersion")
	imageURL, _, _ := unstructured.NestedString(spec, "imageURL")
	downloadPath, _, _ := unstructured.NestedString(spec, "downloadPath")
	if downloadPath == "" {
		downloadPath = defaultDownloadPath
	}

	// Extract checksum if provided
	checksum, _, _ := unstructured.NestedMap(spec, "checksum")
	expectedMD5, _, _ := unstructured.NestedString(checksum, "md5")

	// Extract status fields
	status, found, err := unstructured.NestedMap(u.Object, "status", "downloadStatus")
	if err != nil {
		klog.ErrorS(err, "Failed to get status.downloadStatus")
		return
	}

	var downloadedVersion string
	if found && status != nil {
		downloadedVersion, _, _ = unstructured.NestedString(status, "downloadedVersion")
	}

	klog.InfoS("Reconciliation state",
		"desiredVersion", desiredVersion,
		"downloadedVersion", downloadedVersion,
		"imageURL", imageURL,
		"downloadPath", downloadPath,
		"expectedMD5", expectedMD5)

	// Check if reconciliation needed
	if desiredVersion == downloadedVersion && downloadedVersion != "" {
		klog.InfoS("No reconciliation needed - versions match",
			"version", desiredVersion)
		return
	}

	// Check if file already exists with correct MD5
	if expectedMD5 != "" {
		matches, err := c.gnoiClient.VerifyLocalImage(downloadPath, expectedMD5)
		if err != nil {
			klog.ErrorS(err, "Failed to verify local image")
		} else if matches {
			klog.InfoS("Local image already exists and matches expected MD5",
				"path", downloadPath,
				"md5", expectedMD5)
			c.updateStatus(u, "Succeeded", "Image already present with correct checksum", desiredVersion, expectedMD5)
			return
		}
	}

	// Perform download
	klog.InfoS("Starting image download reconciliation",
		"desiredVersion", desiredVersion,
		"imageURL", imageURL,
		"downloadPath", downloadPath)

	c.updateStatus(u, "Downloading", "Downloading image via gNOI", "", "")

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Minute)
	defer cancel()

	err = c.gnoiClient.DownloadImage(ctx, imageURL, downloadPath, expectedMD5)
	if err != nil {
		klog.ErrorS(err, "Failed to download image")
		c.updateStatus(u, "Failed", fmt.Sprintf("Download failed: %v", err), "", "")
		return
	}

	klog.InfoS("Image download completed successfully",
		"desiredVersion", desiredVersion,
		"downloadPath", downloadPath)

	c.updateStatus(u, "Succeeded", "Image downloaded successfully", desiredVersion, expectedMD5)
}

// updateStatus updates the NetworkDevice status
func (c *Controller) updateStatus(u *unstructured.Unstructured, phase, message, downloadedVersion, downloadedChecksum string) {
	// Build status object
	statusUpdate := map[string]interface{}{
		"downloadStatus": map[string]interface{}{
			"phase":           phase,
			"message":         message,
			"lastAttemptTime": time.Now().Format(time.RFC3339),
		},
	}

	if downloadedVersion != "" {
		statusUpdate["downloadStatus"].(map[string]interface{})["downloadedVersion"] = downloadedVersion
	}
	if downloadedChecksum != "" {
		statusUpdate["downloadStatus"].(map[string]interface{})["downloadedChecksum"] = downloadedChecksum
	}

	// Update status subresource
	if err := unstructured.SetNestedMap(u.Object, statusUpdate, "status"); err != nil {
		klog.ErrorS(err, "Failed to set status")
		return
	}

	// Skip actual API call if dynamicClient is nil (for unit tests)
	if c.dynamicClient == nil {
		klog.InfoS("Skipping status update (no dynamic client - unit test mode)",
			"phase", phase,
			"message", message,
			"downloadedVersion", downloadedVersion)
		return
	}

	gvr := schema.GroupVersionResource{
		Group:    "sonic.io",
		Version:  "v1",
		Resource: "networkdevices",
	}

	_, err := c.dynamicClient.Resource(gvr).Namespace(u.GetNamespace()).UpdateStatus(context.TODO(), u, metav1.UpdateOptions{})
	if err != nil {
		klog.ErrorS(err, "Failed to update status", "phase", phase, "message", message)
		return
	}

	klog.InfoS("Status updated successfully",
		"phase", phase,
		"message", message,
		"downloadedVersion", downloadedVersion)
}
