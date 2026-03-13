// build.rs — tonic-build compiles watchdog.proto into Rust gRPC stubs.
//
// The generated code is placed in OUT_DIR and included via the
// `tonic::include_proto!` macro in main.rs.
fn main() -> Result<(), Box<dyn std::error::Error>> {
    tonic_build::compile_protos("proto/watchdog.proto")?;
    Ok(())
}
