package main

import (
	"context"
	"flag"
	"os"
	"os/signal"
	"syscall"

	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/config"
	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/controller"
	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/gnoi"
	"github.com/sonic-net/sonic-buildimage/src/sonic-change-agent/pkg/version"
	"k8s.io/client-go/rest"
	"k8s.io/klog/v2"
)

var (
	deviceName  = flag.String("device-name", "", "Name of the SONiC device (defaults to NODE_NAME env var)")
	showVersion = flag.Bool("version", false, "Show version information")
)

func main() {
	klog.InitFlags(nil)
	flag.Parse()

	if *showVersion {
		klog.InfoS("sonic-change-agent",
			"version", version.Version,
			"gitCommit", version.GitCommit,
			"buildTime", version.BuildTime)
		return
	}

	// Get device name from flag or environment
	if *deviceName == "" {
		*deviceName = os.Getenv("NODE_NAME")
		if *deviceName == "" {
			klog.ErrorS(nil, "Device name must be provided via --device-name or NODE_NAME env var")
			os.Exit(1)
		}
	}

	// Read gNOI server config from SONiC Redis
	gnoiConfig, err := config.GetGNOIConfigFromRedis()
	if err != nil {
		klog.ErrorS(err, "Failed to read gNOI config from Redis")
		os.Exit(1)
	}

	klog.InfoS("Starting sonic-change-agent",
		"version", version.Version,
		"deviceName", *deviceName,
		"gnoiEndpoint", gnoiConfig.GetGNOIEndpoint(),
		"dryRun", os.Getenv("DRY_RUN") == "true")

	// Create gNOI client
	gnoiClient, err := gnoi.NewClient(gnoiConfig.GetGNOIEndpoint())
	if err != nil {
		klog.ErrorS(err, "Failed to create gNOI client")
		os.Exit(1)
	}
	defer gnoiClient.Close()

	// Create Kubernetes client config
	kubeConfig, err := rest.InClusterConfig()
	if err != nil {
		klog.ErrorS(err, "Failed to create in-cluster config")
		os.Exit(1)
	}

	// Read Kubernetes API server config from SONiC Redis
	// SONiC devices can't reach cluster service IP from hostNetwork, so we need the real API server IP
	serverConfig, err := config.GetKubernetesServerFromRedis()
	if err != nil {
		klog.ErrorS(err, "Failed to read Kubernetes server config from Redis")
		os.Exit(1)
	}
	kubeConfig.Host = serverConfig.GetAPIServerURL()
	klog.InfoS("Using Kubernetes API server from Redis", "host", kubeConfig.Host)

	// Create controller
	ctrl, err := controller.NewController(*deviceName, gnoiClient, kubeConfig)
	if err != nil {
		klog.ErrorS(err, "Failed to create controller")
		os.Exit(1)
	}

	// Setup signal handling
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		sig := <-sigCh
		klog.InfoS("Received signal, shutting down", "signal", sig)
		cancel()
	}()

	// Run controller
	if err := ctrl.Run(ctx); err != nil {
		klog.ErrorS(err, "Controller failed")
		os.Exit(1)
	}

	klog.InfoS("Shutdown complete")
}
