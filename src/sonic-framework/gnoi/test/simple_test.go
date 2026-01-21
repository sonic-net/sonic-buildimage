// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package simple

import (
	"testing"

	//lint:ignore SA1019 Test depends on deprecated proto marshal API.
	"github.com/golang/protobuf/proto"
	bgppb "github.com/openconfig/gnoi/bgp"
	cpb "github.com/openconfig/gnoi/common"
	spb "github.com/openconfig/gnoi/system"
	tpb "github.com/openconfig/gnoi/types"
)

func TestGNOI(t *testing.T) {
	tests := []struct {
		desc string
		in   proto.Message
		want string
	}{{
		desc: "tpb.Path",
		in: &tpb.Path{
			Origin: "oc",
			Elem:   []*tpb.PathElem{{Name: "interfaces", Key: map[string]string{"name": "Ethernet1/1/0"}}},
		},
		want: "origin: \"oc\"\nelem: <\n  name: \"interfaces\"\n  key: <\n    key: \"name\"\n    value: \"Ethernet1/1/0\"\n  >\n>\n",
	}, {
		desc: "tpb.HashType",
		in: &tpb.HashType{
			Method: tpb.HashType_MD5,
			Hash:   []byte("foo"),
		},
		want: "method: MD5\nhash: \"foo\"\n",
	}, {
		desc: "bgp.ClearBGPNeighborRequest",
		in: &bgppb.ClearBGPNeighborRequest{
			Address:         "foo",
			RoutingInstance: "bar",
			Mode:            bgppb.ClearBGPNeighborRequest_HARD,
		},
		want: "address: \"foo\"\nrouting_instance: \"bar\"\nmode: HARD\n",
	}, {
		desc: "system.SetPackage",
		in: &spb.Package{
			Filename: "filename",
			RemoteDownload: &cpb.RemoteDownload{
				Path:     "foo",
				Protocol: cpb.RemoteDownload_SCP,
				Credentials: &tpb.Credentials{
					Username: "bar",
				},
			},
		},
		want: "filename: \"filename\"\nremote_download: <\n  path: \"foo\"\n  protocol: SCP\n  credentials: <\n    username: \"bar\"\n  >\n>\n",
	}}
	for _, tt := range tests {
		t.Run(tt.desc, func(t *testing.T) {
			if got := proto.MarshalTextString(tt.in); got != tt.want {
				t.Fatalf("Text Proto output failed: got %q, want %q", got, tt.want)
			}
		})
	}
}
