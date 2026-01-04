package controller

import (
	"context"
	"fmt"
	"sync"
	"time"

	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/gnoi"
	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/workflow"
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
		Group:    "sonic.k8s.io",
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
	deviceType, _, _ := unstructured.NestedString(u.Object, "spec", "type")
	osVersion, _, _ := unstructured.NestedString(u.Object, "spec", "osVersion")
	firmwareProfile, _, _ := unstructured.NestedString(u.Object, "spec", "firmwareProfile")
	operation, _, _ := unstructured.NestedString(u.Object, "spec", "operation")
	operationAction, _, _ := unstructured.NestedString(u.Object, "spec", "operationAction")

	// Extract status fields
	currentOSVersion, _, _ := unstructured.NestedString(u.Object, "status", "osVersion")
	operationState, _, _ := unstructured.NestedString(u.Object, "status", "operationState")
	operationActionState, _, _ := unstructured.NestedString(u.Object, "status", "operationActionState")

	klog.InfoS("Reconciliation state",
		"deviceType", deviceType,
		"osVersion", osVersion,
		"firmwareProfile", firmwareProfile,
		"currentOSVersion", currentOSVersion,
		"operation", operation,
		"operationAction", operationAction,
		"operationState", operationState,
		"operationActionState", operationActionState)

	// Check if reconciliation needed
	// Skip if no operation or if operation is already in completed state
	if operation == "" {
		klog.InfoS("No operation specified, skipping reconciliation")
		return
	}

	// For OSUpgrade operations, check if we need to take action
	if operation == "OSUpgrade" && operationAction == "PreloadImage" {
		if operationActionState == "completed" {
			klog.InfoS("PreloadImage already completed, skipping reconciliation")
			return
		}
		if operationState != "proceed" && operationState != "" {
			klog.InfoS("Operation state not ready for action", "operationState", operationState)
			return
		}
	}

	// Create workflow based on operation and operationAction
	workflowType := fmt.Sprintf("%s-%s", operation, operationAction)
	wf, err := workflow.NewWorkflow(workflowType, c.gnoiClient)
	if err != nil {
		klog.ErrorS(err, "Failed to create workflow", "type", workflowType)
		c.updateOperationStatus(u, "failed", "failed", fmt.Sprintf("Failed to create workflow: %v", err))
		return
	}

	// Execute workflow
	klog.InfoS("Starting workflow execution",
		"operation", operation,
		"operationAction", operationAction,
		"osVersion", osVersion)

	c.updateOperationStatus(u, "in_progress", "in_progress", fmt.Sprintf("Executing %s %s", operation, operationAction))

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Minute)
	defer cancel()

	err = wf.Execute(ctx, u)
	if err != nil {
		klog.ErrorS(err, "Workflow execution failed", "operation", operation, "operationAction", operationAction)
		c.updateOperationStatus(u, "failed", "failed", fmt.Sprintf("Workflow failed: %v", err))
		return
	}

	klog.InfoS("Workflow execution completed successfully",
		"operation", operation,
		"operationAction", operationAction,
		"osVersion", osVersion)

	c.updateOperationStatus(u, "completed", "completed", "Workflow completed successfully")
}

// updateOperationStatus updates the NetworkDevice operation status
func (c *Controller) updateOperationStatus(u *unstructured.Unstructured, operationState, operationActionState, message string) {
	// Get current status or create new one
	currentStatus, _, _ := unstructured.NestedMap(u.Object, "status")
	if currentStatus == nil {
		currentStatus = make(map[string]interface{})
	}

	// Update operation states
	if operationState != "" {
		currentStatus["operationState"] = operationState
	}
	if operationActionState != "" {
		currentStatus["operationActionState"] = operationActionState
	}
	currentStatus["lastTransitionTime"] = time.Now().Format(time.RFC3339)

	// Set overall device state based on operation state
	switch operationState {
	case "completed":
		currentStatus["state"] = "Healthy"
	case "failed":
		currentStatus["state"] = "Failed"
	case "in_progress":
		currentStatus["state"] = "Updating"
	default:
		currentStatus["state"] = "Unknown"
	}

	// Update status subresource
	if err := unstructured.SetNestedMap(u.Object, currentStatus, "status"); err != nil {
		klog.ErrorS(err, "Failed to set status")
		return
	}

	// Skip actual API call if dynamicClient is nil (for unit tests)
	if c.dynamicClient == nil {
		klog.InfoS("Skipping status update (no dynamic client - unit test mode)",
			"operationState", operationState,
			"operationActionState", operationActionState,
			"message", message)
		return
	}

	gvr := schema.GroupVersionResource{
		Group:    "sonic.k8s.io",
		Version:  "v1",
		Resource: "networkdevices",
	}

	_, err := c.dynamicClient.Resource(gvr).Namespace(u.GetNamespace()).UpdateStatus(context.TODO(), u, metav1.UpdateOptions{})
	if err != nil {
		klog.ErrorS(err, "Failed to update status", "operationState", operationState, "operationActionState", operationActionState, "message", message)
		return
	}

	klog.InfoS("Operation status updated successfully",
		"operationState", operationState,
		"operationActionState", operationActionState,
		"message", message)
}
