package gnoi

import (
	"crypto/md5"
	"fmt"
	"os"
	"path/filepath"
	"testing"
)

func TestVerifyLocalImage_FileNotExists(t *testing.T) {
	client := &grpcClient{}

	matches, err := client.VerifyLocalImage("/nonexistent/file.bin", "abc123")
	if err != nil {
		t.Fatalf("Expected no error for nonexistent file, got: %v", err)
	}
	if matches {
		t.Error("Expected matches=false for nonexistent file")
	}
}

func TestVerifyLocalImage_NoMD5Check(t *testing.T) {
	client := &grpcClient{}

	// Create temporary file
	tmpDir := t.TempDir()
	tmpFile := filepath.Join(tmpDir, "test.bin")
	if err := os.WriteFile(tmpFile, []byte("test content"), 0644); err != nil {
		t.Fatalf("Failed to create test file: %v", err)
	}

	// Verify without MD5 should just check existence
	matches, err := client.VerifyLocalImage(tmpFile, "")
	if err != nil {
		t.Fatalf("Expected no error, got: %v", err)
	}
	if !matches {
		t.Error("Expected matches=true when file exists and no MD5 provided")
	}
}

func TestVerifyLocalImage_MD5Matches(t *testing.T) {
	client := &grpcClient{}

	// Create temporary file with known content
	tmpDir := t.TempDir()
	tmpFile := filepath.Join(tmpDir, "test.bin")
	content := []byte("test content for md5")
	if err := os.WriteFile(tmpFile, content, 0644); err != nil {
		t.Fatalf("Failed to create test file: %v", err)
	}

	// Calculate expected MD5
	hash := md5.Sum(content)
	expectedMD5 := fmt.Sprintf("%x", hash)

	// Verify
	matches, err := client.VerifyLocalImage(tmpFile, expectedMD5)
	if err != nil {
		t.Fatalf("Expected no error, got: %v", err)
	}
	if !matches {
		t.Errorf("Expected matches=true for correct MD5")
	}
}

func TestVerifyLocalImage_MD5Mismatch(t *testing.T) {
	client := &grpcClient{}

	// Create temporary file
	tmpDir := t.TempDir()
	tmpFile := filepath.Join(tmpDir, "test.bin")
	if err := os.WriteFile(tmpFile, []byte("test content"), 0644); err != nil {
		t.Fatalf("Failed to create test file: %v", err)
	}

	// Use wrong MD5
	wrongMD5 := "ffffffffffffffffffffffffffffffff"

	// Verify
	matches, err := client.VerifyLocalImage(tmpFile, wrongMD5)
	if err != nil {
		t.Fatalf("Expected no error, got: %v", err)
	}
	if matches {
		t.Error("Expected matches=false for incorrect MD5")
	}
}

func TestDownloadImage_DryRun(t *testing.T) {
	// Set DRY_RUN mode
	os.Setenv("DRY_RUN", "true")
	defer os.Unsetenv("DRY_RUN")

	// No need for real gRPC connection in DRY_RUN
	client := &grpcClient{}

	err := client.DownloadImage(nil, "http://example.com/sonic.bin", "/tmp/test.bin", "abc123")
	if err != nil {
		t.Errorf("DRY_RUN should not return error, got: %v", err)
	}
}

func TestVerifyLocalImage_StatError(t *testing.T) {
	client := &grpcClient{}

	// Try to stat a directory (permission error scenario)
	tmpDir := t.TempDir()
	restrictedFile := filepath.Join(tmpDir, "restricted")

	// Create file and make it inaccessible
	if err := os.WriteFile(restrictedFile, []byte("test"), 0000); err != nil {
		t.Fatalf("Failed to create test file: %v", err)
	}
	defer os.Chmod(restrictedFile, 0644) // Clean up

	// Trying to read should fail
	matches, err := client.VerifyLocalImage(restrictedFile, "abc123")
	if err == nil {
		t.Error("Expected error when file is not readable")
	}
	if matches {
		t.Error("Expected matches=false on error")
	}
}

func TestVerifyLocalImage_EmptyFile(t *testing.T) {
	client := &grpcClient{}

	tmpDir := t.TempDir()
	tmpFile := filepath.Join(tmpDir, "empty.bin")
	if err := os.WriteFile(tmpFile, []byte{}, 0644); err != nil {
		t.Fatalf("Failed to create test file: %v", err)
	}

	// MD5 of empty file is d41d8cd98f00b204e9800998ecf8427e
	expectedMD5 := "d41d8cd98f00b204e9800998ecf8427e"

	matches, err := client.VerifyLocalImage(tmpFile, expectedMD5)
	if err != nil {
		t.Fatalf("Unexpected error: %v", err)
	}
	if !matches {
		t.Error("Expected matches=true for correct empty file MD5")
	}
}

func TestClose_NilConnection(t *testing.T) {
	client := &grpcClient{conn: nil}

	err := client.Close()
	if err != nil {
		t.Errorf("Close with nil connection should not error, got: %v", err)
	}
}

func TestDownloadImage_WithoutMD5(t *testing.T) {
	os.Setenv("DRY_RUN", "true")
	defer os.Unsetenv("DRY_RUN")

	client := &grpcClient{}

	// Download without MD5 should still work
	err := client.DownloadImage(nil, "http://example.com/sonic.bin", "/tmp/test.bin", "")
	if err != nil {
		t.Errorf("DownloadImage without MD5 should not error, got: %v", err)
	}
}
