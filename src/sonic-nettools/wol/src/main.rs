mod wol;

extern crate clap;
extern crate pnet;

fn main() {
    if let Err(e) = wol::build_and_send() {
        panic!("Error: {}", e);
    }
}
