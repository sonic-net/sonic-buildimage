package gnoi

import (
	"context"
	"crypto/md5"
	"fmt"
	"io"
	"os"

	"github.com/openconfig/gnoi/common"
	"github.com/openconfig/gnoi/file"
	"github.com/openconfig/gnoi/system"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"k8s.io/klog/v2"
)

// Client is an interface for gNOI operations
type Client interface {
	// DownloadImage downloads a SONiC image to the specified path
	DownloadImage(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error
	// TransferToRemote transfers a file from URL to remote path using gnoi.file service
	TransferToRemote(ctx context.Context, sourceURL, remotePath string) error
	// VerifyLocalImage checks if a local file exists and matches the expected MD5
	VerifyLocalImage(downloadPath, expectedMD5 string) (bool, error)
	// Close closes the gRPC connection
	Close() error
}

// grpcClient implements Client using real gRPC calls
type grpcClient struct {
	conn       *grpc.ClientConn
	systemConn system.SystemClient
	fileConn   file.FileClient
}

// NewClient creates a new gNOI client
func NewClient(endpoint string) (Client, error) {
	conn, err := grpc.Dial(endpoint, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		return nil, fmt.Errorf("failed to dial gNOI server: %w", err)
	}

	return &grpcClient{
		conn:       conn,
		systemConn: system.NewSystemClient(conn),
		fileConn:   file.NewFileClient(conn),
	}, nil
}

// DownloadImage downloads a SONiC image using gNOI System.SetPackage
func (c *grpcClient) DownloadImage(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error {
	klog.InfoS("Starting image download via gNOI",
		"url", imageURL,
		"path", downloadPath,
		"expectedMD5", expectedMD5)

	// Check if DRY_RUN mode
	if os.Getenv("DRY_RUN") == "true" {
		klog.InfoS("DRY_RUN: Would download image via gNOI System.SetPackage",
			"url", imageURL,
			"path", downloadPath,
			"expectedMD5", expectedMD5)
		return nil
	}

	// Create SetPackage request with activate=false (preload only)
	stream, err := c.systemConn.SetPackage(ctx)
	if err != nil {
		return fmt.Errorf("failed to create SetPackage stream: %w", err)
	}

	// Send the Package request with RemoteDownload
	req := &system.SetPackageRequest{
		Request: &system.SetPackageRequest_Package{
			Package: &system.Package{
				Filename: downloadPath,
				Activate: false, // Preload only, don't activate
				RemoteDownload: &common.RemoteDownload{
					Path:     imageURL,
					Protocol: common.RemoteDownload_HTTP,
				},
			},
		},
	}

	if err := stream.Send(req); err != nil {
		return fmt.Errorf("failed to send SetPackage request: %w", err)
	}

	// Wait for completion
	resp, err := stream.CloseAndRecv()
	if err != nil {
		return fmt.Errorf("SetPackage failed: %w", err)
	}

	klog.InfoS("Image download completed successfully",
		"response", resp.String(),
		"path", downloadPath)

	// Verify downloaded file MD5 if expected MD5 provided
	if expectedMD5 != "" {
		matches, err := c.VerifyLocalImage(downloadPath, expectedMD5)
		if err != nil {
			return fmt.Errorf("failed to verify downloaded image: %w", err)
		}
		if !matches {
			return fmt.Errorf("downloaded image MD5 mismatch")
		}
		klog.InfoS("Downloaded image MD5 verified", "md5", expectedMD5)
	}

	return nil
}

// TransferToRemote transfers a file from URL to remote path using gnoi.file service
func (c *grpcClient) TransferToRemote(ctx context.Context, sourceURL, remotePath string) error {
	klog.InfoS("Starting file transfer via gNOI file service",
		"sourceURL", sourceURL,
		"remotePath", remotePath)

	// Check if DRY_RUN mode
	if os.Getenv("DRY_RUN") == "true" {
		klog.InfoS("DRY_RUN: Would transfer file via gNOI file.TransferToRemote",
			"sourceURL", sourceURL,
			"remotePath", remotePath)
		return nil
	}

	// Create TransferToRemote request
	req := &file.TransferToRemoteRequest{
		LocalPath: remotePath,
		RemoteDownload: &common.RemoteDownload{
			Path:     sourceURL,
			Protocol: common.RemoteDownload_HTTP,
		},
	}

	// Execute the transfer
	resp, err := c.fileConn.TransferToRemote(ctx, req)
	if err != nil {
		return fmt.Errorf("file transfer failed: %w", err)
	}

	klog.InfoS("File transfer completed successfully",
		"response", resp.String(),
		"remotePath", remotePath)

	return nil
}

// VerifyLocalImage checks if a file exists and matches the expected MD5
func (c *grpcClient) VerifyLocalImage(downloadPath, expectedMD5 string) (bool, error) {
	// Check if file exists
	fileInfo, err := os.Stat(downloadPath)
	if err != nil {
		if os.IsNotExist(err) {
			return false, nil
		}
		return false, fmt.Errorf("failed to stat file: %w", err)
	}

	// If no expected MD5, just check existence
	if expectedMD5 == "" {
		return true, nil
	}

	klog.InfoS("Verifying local image MD5",
		"path", downloadPath,
		"size", fileInfo.Size(),
		"expectedMD5", expectedMD5)

	// Calculate MD5
	file, err := os.Open(downloadPath)
	if err != nil {
		return false, fmt.Errorf("failed to open file: %w", err)
	}
	defer file.Close()

	// Justification: MD5 used for data integrity verification (checksum), not cryptographic security
	hash := md5.New() // nosemgrep: go.lang.security.audit.crypto.use_of_weak_crypto.use-of-md5
	if _, err := io.Copy(hash, file); err != nil {
		return false, fmt.Errorf("failed to calculate MD5: %w", err)
	}

	actualMD5 := fmt.Sprintf("%x", hash.Sum(nil))
	matches := actualMD5 == expectedMD5

	klog.InfoS("MD5 verification result",
		"expected", expectedMD5,
		"actual", actualMD5,
		"matches", matches)

	return matches, nil
}

// Close closes the gRPC connection
func (c *grpcClient) Close() error {
	if c.conn != nil {
		return c.conn.Close()
	}
	return nil
}

// Ensure grpcClient implements Client interface
var _ Client = (*grpcClient)(nil)
