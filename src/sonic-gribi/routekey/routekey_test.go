package routekey

import "testing"

func TestFor(t *testing.T) {
	cases := []struct{ ni, prefix, want string }{
		{"DEFAULT", "10.20.0.0/16", "10.20.0.0/16"},
		{"", "10.20.0.0/16", "10.20.0.0/16"},
		{"DEFAULT", "2001:db8:20::/64", "2001:db8:20::/64"},
		{"blue", "10.20.0.0/16", "Vrfblue:10.20.0.0/16"},
		{"Vrfblue", "10.20.0.0/16", "Vrfblue:10.20.0.0/16"},
		{"default", "10.20.0.0/16", "Vrfdefault:10.20.0.0/16"},
	}
	for _, c := range cases {
		if got := For(c.ni, c.prefix); got != c.want {
			t.Errorf("For(%q, %q) = %q, want %q", c.ni, c.prefix, got, c.want)
		}
	}
}

func TestVRF(t *testing.T) {
	cases := []struct{ ni, want string }{
		{"DEFAULT", ""},
		{"", ""},
		{"blue", "Vrfblue"},
		{"Vrfblue", "Vrfblue"},
	}
	for _, c := range cases {
		if got := VRF(c.ni); got != c.want {
			t.Errorf("VRF(%q) = %q, want %q", c.ni, got, c.want)
		}
	}
}
