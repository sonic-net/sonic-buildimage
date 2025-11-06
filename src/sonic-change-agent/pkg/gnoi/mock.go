package gnoi

import (
	"context"
	"fmt"
	"sync"
)

// MockClient is a mock implementation of Client for testing
type MockClient struct {
	mu sync.Mutex

	// Mock behaviors
	DownloadImageFunc      func(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error
	TransferToRemoteFunc   func(ctx context.Context, sourceURL, remotePath string) error
	VerifyLocalImageFunc   func(downloadPath, expectedMD5 string) (bool, error)
	CloseFunc              func() error

	// Call tracking
	DownloadImageCalls      []DownloadImageCall
	TransferToRemoteCalls   []TransferToRemoteCall
	VerifyLocalImageCalls   []VerifyLocalImageCall
	CloseCalls              int
}

type DownloadImageCall struct {
	ImageURL     string
	DownloadPath string
	ExpectedMD5  string
}

type TransferToRemoteCall struct {
	SourceURL  string
	RemotePath string
}

type VerifyLocalImageCall struct {
	DownloadPath string
	ExpectedMD5  string
}

// NewMockClient creates a new mock client with default behaviors
func NewMockClient() *MockClient {
	return &MockClient{
		DownloadImageFunc: func(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error {
			return nil
		},
		TransferToRemoteFunc: func(ctx context.Context, sourceURL, remotePath string) error {
			return nil
		},
		VerifyLocalImageFunc: func(downloadPath, expectedMD5 string) (bool, error) {
			return false, nil
		},
		CloseFunc: func() error {
			return nil
		},
	}
}

func (m *MockClient) DownloadImage(ctx context.Context, imageURL, downloadPath, expectedMD5 string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	m.DownloadImageCalls = append(m.DownloadImageCalls, DownloadImageCall{
		ImageURL:     imageURL,
		DownloadPath: downloadPath,
		ExpectedMD5:  expectedMD5,
	})

	return m.DownloadImageFunc(ctx, imageURL, downloadPath, expectedMD5)
}

func (m *MockClient) TransferToRemote(ctx context.Context, sourceURL, remotePath string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	m.TransferToRemoteCalls = append(m.TransferToRemoteCalls, TransferToRemoteCall{
		SourceURL:  sourceURL,
		RemotePath: remotePath,
	})

	return m.TransferToRemoteFunc(ctx, sourceURL, remotePath)
}

func (m *MockClient) VerifyLocalImage(downloadPath, expectedMD5 string) (bool, error) {
	m.mu.Lock()
	defer m.mu.Unlock()

	m.VerifyLocalImageCalls = append(m.VerifyLocalImageCalls, VerifyLocalImageCall{
		DownloadPath: downloadPath,
		ExpectedMD5:  expectedMD5,
	})

	return m.VerifyLocalImageFunc(downloadPath, expectedMD5)
}

func (m *MockClient) Close() error {
	m.mu.Lock()
	defer m.mu.Unlock()

	m.CloseCalls++
	return m.CloseFunc()
}

// Helper methods for test assertions
func (m *MockClient) GetDownloadImageCallCount() int {
	m.mu.Lock()
	defer m.mu.Unlock()
	return len(m.DownloadImageCalls)
}

func (m *MockClient) GetLastDownloadImageCall() (DownloadImageCall, error) {
	m.mu.Lock()
	defer m.mu.Unlock()

	if len(m.DownloadImageCalls) == 0 {
		return DownloadImageCall{}, fmt.Errorf("no DownloadImage calls recorded")
	}
	return m.DownloadImageCalls[len(m.DownloadImageCalls)-1], nil
}

func (m *MockClient) ResetCalls() {
	m.mu.Lock()
	defer m.mu.Unlock()

	m.DownloadImageCalls = nil
	m.TransferToRemoteCalls = nil
	m.VerifyLocalImageCalls = nil
	m.CloseCalls = 0
}

// Ensure MockClient implements Client interface
var _ Client = (*MockClient)(nil)
