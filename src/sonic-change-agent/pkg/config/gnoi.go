package config

import (
	"context"
	"fmt"
	"strconv"

	"github.com/go-redis/redis/v8"
)

const (
	gnmiTableKey  = "GNMI|gnmi"
	certsTableKey = "GNMI|certs"
	defaultPort   = 8080
)

// GNOIConfig holds gNOI server configuration
type GNOIConfig struct {
	Port       int
	ServerCert string
	ServerKey  string
	CACert     string
	ClientAuth bool
	UseTLS     bool
	Insecure   bool
}

// GetGNOIEndpoint returns the gNOI server endpoint address
func (c *GNOIConfig) GetGNOIEndpoint() string {
	return fmt.Sprintf("localhost:%d", c.Port)
}

// GetGNOIConfigFromRedis reads gNOI configuration from CONFIG_DB
func GetGNOIConfigFromRedis() (*GNOIConfig, error) {
	client := redis.NewClient(&redis.Options{
		Addr: "localhost:6379",
		DB:   redisDB, // CONFIG_DB = 4
	})
	defer client.Close()

	ctx := context.Background()

	config := &GNOIConfig{
		Port: defaultPort,
	}

	// Read GNMI|gnmi table
	gnmiData, err := client.HGetAll(ctx, gnmiTableKey).Result()
	if err != nil {
		return nil, fmt.Errorf("failed to read GNMI table from Redis: %w", err)
	}

	// Parse port
	if portStr, ok := gnmiData["port"]; ok && portStr != "" {
		port, err := strconv.Atoi(portStr)
		if err != nil {
			return nil, fmt.Errorf("invalid port value '%s': %w", portStr, err)
		}
		config.Port = port
	}

	// Parse client_auth
	if clientAuth, ok := gnmiData["client_auth"]; ok {
		config.ClientAuth = clientAuth == "true"
	}

	// Read GNMI|certs table for TLS configuration
	certsData, err := client.HGetAll(ctx, certsTableKey).Result()
	if err != nil {
		// No error if table doesn't exist, just means no TLS
		certsData = make(map[string]string)
	}

	// TLS support: Check if server is configured with TLS
	if len(certsData) > 0 {
		config.ServerCert = certsData["server_crt"]
		config.ServerKey = certsData["server_key"]
		config.CACert = certsData["ca_crt"]

		if config.ServerCert != "" && config.ServerKey != "" {
			// Server has TLS configured - not yet supported
			return nil, fmt.Errorf("TLS is configured but not yet implemented (server_crt: %s, server_key: %s)",
				config.ServerCert, config.ServerKey)
		}
	}

	// No TLS configured, use insecure connection
	config.UseTLS = false
	config.Insecure = false

	return config, nil
}
