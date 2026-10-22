# platform-api-rs-crates

The crates a Rust consumer of the platform API is built from on Mellanox, as
siblings -- today the Rust pmon daemons, through
`../mlnx-platform-api-rs/platform-native.cargo.toml`.

`platform-provider` takes the native platform API as `../platform-native`, a
path dependency, and cargo cannot `[patch]` a path.  So the provider is built
from here, where its `../platform-native` is this platform's crate
(`../mlnx-platform-api-rs`) instead of the stub beside it in
sonic-platform-common.  `hw-mgmt` (`../hw-mgmt-rs`) is that crate's own
dependency, hw-management's Python helpers in Rust.  Every other entry is a
link to the crate the image would have used anyway, so that the provider's
other path dependencies, and the git dependencies `platform-native.cargo.toml`
patches, resolve to one copy.

Cargo resolves a path dependency from where the manifest was found, not from
where a link points, which is what makes the substitution work.

The native API is linked into each consumer's process. The rest of the
platform's users (healthd, the CLI tools, xcvrd, ledd) keep the Python
`sonic_platform`, on the same switch and the same hardware state, so the two
have to behave alike; see `../mlnx-platform-api-rs/README.md`.
