package config

import (
	"fmt"

	"github.com/go-redis/redis/v8"
	"golang.org/x/net/context"
)

const (
	// Redis database and key for Kubernetes master config
	redisDB             = 4
	kubernetesServerKey = "KUBERNETES_MASTER|SERVER"
)

// KubernetesServerConfig holds the API server configuration from Redis
type KubernetesServerConfig struct {
	IP       string
	Port     string
	Insecure bool
	Disable  bool
}

// GetKubernetesServerFromRedis reads the Kubernetes API server config from SONiC Redis
func GetKubernetesServerFromRedis() (*KubernetesServerConfig, error) {
	ctx := context.Background()

	// Connect to Redis
	client := redis.NewClient(&redis.Options{
		Addr: "localhost:6379",
		DB:   redisDB,
	})
	defer client.Close()

	// Ping to verify connection
	if err := client.Ping(ctx).Err(); err != nil {
		return nil, fmt.Errorf("failed to connect to Redis: %w", err)
	}

	// Get the hash
	result, err := client.HGetAll(ctx, kubernetesServerKey).Result()
	if err != nil {
		return nil, fmt.Errorf("failed to read %s from Redis: %w", kubernetesServerKey, err)
	}

	if len(result) == 0 {
		return nil, fmt.Errorf("no Kubernetes server config found in Redis at key %s", kubernetesServerKey)
	}

	config := &KubernetesServerConfig{
		IP:       result["ip"],
		Port:     result["port"],
		Insecure: result["insecure"] == "True",
		Disable:  result["disable"] == "True",
	}

	if config.IP == "" || config.Port == "" {
		return nil, fmt.Errorf("invalid Kubernetes server config: ip=%s port=%s", config.IP, config.Port)
	}

	return config, nil
}

// GetAPIServerURL returns the full API server URL
func (c *KubernetesServerConfig) GetAPIServerURL() string {
	return fmt.Sprintf("https://%s:%s", c.IP, c.Port)
}
