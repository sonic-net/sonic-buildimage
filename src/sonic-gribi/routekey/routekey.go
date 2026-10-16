// Package routekey maps a gRIBI (network instance, prefix) pair onto the key
// routeorch expects in ROUTE_TABLE. It is shared by the adapter that programs
// routes and the gRIBI wrapper that correlates operations with FIB responses,
// so the two can never disagree on a key.
package routekey

import (
	"strings"

	"github.com/openconfig/gribigo/server"
)

// vrfPrefix is VRF_PREFIX in routeorch.cpp: a named VRF's route keys are
// `Vrf<name>:<prefix>` and the VRF must already exist in CONFIG_DB, or
// routeorch defers the route until it does.
const vrfPrefix = "Vrf"

// For returns the ROUTE_TABLE key for prefix in network instance ni. Only
// gribigo's default instance maps to the bare prefix, since that is the only
// instance SONiC's default VRF has a name for.
func For(ni, prefix string) string {
	if vrf := VRF(ni); vrf != "" {
		return vrf + ":" + prefix
	}
	return prefix
}

// VRF returns the SONiC VRF name for network instance ni, or "" for the
// default VRF.
func VRF(ni string) string {
	if ni == "" || ni == server.DefaultNetworkInstanceName {
		return ""
	}
	if strings.HasPrefix(ni, vrfPrefix) {
		return ni
	}
	return vrfPrefix + ni
}
