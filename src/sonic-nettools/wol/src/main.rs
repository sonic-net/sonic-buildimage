mod wol;

extern crate clap;
extern crate pnet;

fn main() {
    if let Err(e) = wol::build_and_send() {
        eprintln!("Error: {}", e.msg);
        std::process::exit(e.code);
    } else {
        std::process::exit(0);
    }
}
