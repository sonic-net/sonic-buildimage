use super::{build_client_identity, build_restapi_https_client, parse_ca_certificates};
use openssl::asn1::Asn1Time;
use openssl::bn::{BigNum, MsbOption};
use openssl::ec::{EcGroup, EcKey};
use openssl::hash::MessageDigest;
use openssl::nid::Nid;
use openssl::pkey::{PKey, Private};
use openssl::rsa::Rsa;
use openssl::ssl::{SslAcceptor, SslMethod, SslVerifyMode, SslVersion};
use openssl::x509::extension::{
    BasicConstraints, ExtendedKeyUsage, KeyUsage, SubjectAlternativeName,
};
use openssl::x509::{X509NameBuilder, X509};
use std::io::{BufRead, BufReader, Write};
use std::net::TcpListener;
use std::thread;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

enum Profile {
    Ca,
    DnsOnly,
    NoSan,
    Expired,
    NotYetValid,
    ClientOnly,
    BadSignature,
}

struct Credentials {
    cert: X509,
    key: PKey<Private>,
}

fn rsa_key() -> PKey<Private> {
    PKey::from_rsa(Rsa::generate(2048).unwrap()).unwrap()
}

fn issue(key: PKey<Private>, issuer: Option<&Credentials>, profile: Profile) -> Credentials {
    let mut cert = X509::builder().unwrap();
    cert.set_version(2).unwrap();
    let mut serial = BigNum::new().unwrap();
    serial.rand(128, MsbOption::MAYBE_ZERO, false).unwrap();
    cert.set_serial_number(&serial.to_asn1_integer().unwrap())
        .unwrap();
    let mut name = X509NameBuilder::new().unwrap();
    let common_name = if matches!(profile, Profile::Ca) {
        format!("Watchdog Test CA {}", serial.to_hex_str().unwrap())
    } else {
        "test.server.restapi.sonic".to_string()
    };
    name.append_entry_by_text("CN", &common_name).unwrap();
    let name = name.build();
    cert.set_subject_name(&name).unwrap();
    cert.set_issuer_name(issuer.map_or(name.as_ref(), |ca| ca.cert.subject_name()))
        .unwrap();
    cert.set_pubkey(&key).unwrap();
    let now = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_secs() as i64;
    let (start, end) = match profile {
        Profile::Expired => (now - 7200, now - 3600),
        Profile::NotYetValid => (now + 3600, now + 7200),
        _ => (now - 60, now + 3600),
    };
    cert.set_not_before(&Asn1Time::from_unix(start).unwrap())
        .unwrap();
    cert.set_not_after(&Asn1Time::from_unix(end).unwrap())
        .unwrap();
    if matches!(profile, Profile::Ca) {
        cert.append_extension(BasicConstraints::new().critical().ca().build().unwrap())
            .unwrap();
        cert.append_extension(KeyUsage::new().key_cert_sign().crl_sign().build().unwrap())
            .unwrap();
    } else {
        cert.append_extension(BasicConstraints::new().critical().build().unwrap())
            .unwrap();
        cert.append_extension(
            KeyUsage::new()
                .digital_signature()
                .key_encipherment()
                .build()
                .unwrap(),
        )
        .unwrap();
        let mut usage = ExtendedKeyUsage::new();
        usage.client_auth();
        if !matches!(profile, Profile::ClientOnly) {
            usage.server_auth();
        }
        cert.append_extension(usage.build().unwrap()).unwrap();
        if !matches!(profile, Profile::NoSan) {
            let san = SubjectAlternativeName::new()
                .dns("test.server.restapi.sonic")
                .build(&cert.x509v3_context(issuer.map(|ca| ca.cert.as_ref()), None))
                .unwrap();
            cert.append_extension(san).unwrap();
        }
    }
    let wrong_key = matches!(profile, Profile::BadSignature).then(rsa_key);
    let signer = wrong_key
        .as_ref()
        .unwrap_or_else(|| issuer.map_or(&key, |ca| &ca.key));
    cert.sign(signer, MessageDigest::sha256()).unwrap();
    Credentials {
        cert: cert.build(),
        key,
    }
}

fn client(ca: &Credentials, identity: &Credentials) -> reqwest::blocking::Client {
    client_from_ca_pem(&ca.cert.to_pem().unwrap(), identity)
}

fn client_from_ca_pem(ca_pem: &[u8], identity: &Credentials) -> reqwest::blocking::Client {
    let ca_certs = parse_ca_certificates(ca_pem).unwrap();
    let identity = build_client_identity(
        &identity.cert.to_pem().unwrap(),
        &identity.key.private_key_to_pem_pkcs8().unwrap(),
    )
    .unwrap();
    build_restapi_https_client(ca_certs, identity).unwrap()
}

fn probe(
    client: reqwest::blocking::Client,
    server: &Credentials,
    client_ca: &Credentials,
    response: &'static str,
) -> (
    reqwest::Result<reqwest::blocking::Response>,
    Result<(), String>,
) {
    let mut acceptor = SslAcceptor::mozilla_intermediate(SslMethod::tls()).unwrap();
    acceptor
        .set_min_proto_version(Some(SslVersion::TLS1_2))
        .unwrap();
    acceptor.set_certificate(&server.cert).unwrap();
    acceptor.set_private_key(&server.key).unwrap();
    acceptor
        .cert_store_mut()
        .add_cert(client_ca.cert.clone())
        .unwrap();
    acceptor.set_verify(SslVerifyMode::PEER | SslVerifyMode::FAIL_IF_NO_PEER_CERT);
    let acceptor = acceptor.build();
    let listener = TcpListener::bind("127.0.0.1:0").unwrap();
    listener.set_nonblocking(true).unwrap();
    let url = format!(
        "https://{}/v1/state/heartbeat",
        listener.local_addr().unwrap()
    );
    let server_thread = thread::spawn(move || -> Result<(), String> {
        let deadline = Instant::now() + Duration::from_secs(10);
        let stream = loop {
            match listener.accept() {
                Ok((stream, _)) => break stream,
                Err(e)
                    if e.kind() == std::io::ErrorKind::WouldBlock && Instant::now() < deadline =>
                {
                    thread::sleep(Duration::from_millis(10));
                }
                Err(e) => return Err(format!("TLS test listener: {e}")),
            }
        };
        stream
            .set_read_timeout(Some(Duration::from_secs(5)))
            .unwrap();
        stream
            .set_write_timeout(Some(Duration::from_secs(5)))
            .unwrap();
        let mut tls = acceptor.accept(stream).map_err(|e| e.to_string())?;
        assert!(
            tls.ssl().peer_certificate().is_some(),
            "mTLS identity missing"
        );
        let mut reader = BufReader::new(&mut tls);
        let mut line = String::new();
        reader.read_line(&mut line).map_err(|e| e.to_string())?;
        assert_eq!(line, "GET /v1/state/heartbeat HTTP/1.1\r\n");
        loop {
            line.clear();
            if reader.read_line(&mut line).map_err(|e| e.to_string())? == 0 {
                return Err("EOF before HTTP headers completed".to_string());
            }
            if line == "\r\n" {
                break;
            }
        }
        tls.write_all(response.as_bytes())
            .map_err(|e| e.to_string())
    });
    let result = client.get(url).send();
    (result, server_thread.join().unwrap())
}

const OK: &str = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";

#[test]
fn accepts_issuer_second_in_two_certificate_bundle() {
    let unrelated_ca = issue(rsa_key(), None, Profile::Ca);
    let mut bundle = unrelated_ca.cert.to_pem().unwrap();
    let ca = issue(rsa_key(), None, Profile::Ca);
    bundle.extend_from_slice(&ca.cert.to_pem().unwrap());
    assert_eq!(parse_ca_certificates(&bundle).unwrap().len(), 2);
    let server = issue(rsa_key(), Some(&ca), Profile::DnsOnly);
    let (response, served) = probe(client_from_ca_pem(&bundle, &server), &server, &ca, OK);
    assert_eq!(response.unwrap().status(), 200);
    served.unwrap();
}

#[test]
fn rejects_empty_and_malformed_ca_bundles() {
    for pem in [b"".as_slice(), b"not a certificate"] {
        let error = parse_ca_certificates(pem).unwrap_err();
        assert_eq!(error.to_string(), "CA certificate file contains no certificates");
    }
    let ca = issue(rsa_key(), None, Profile::Ca);
    let mut bundle = ca.cert.to_pem().unwrap();
    bundle.extend_from_slice(b"-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----\n");
    assert!(parse_ca_certificates(&bundle).is_err());
}

#[test]
fn accepts_dns_only_and_missing_san_with_mutual_tls() {
    let ca = issue(rsa_key(), None, Profile::Ca);
    for profile in [Profile::DnsOnly, Profile::NoSan] {
        let server = issue(rsa_key(), Some(&ca), profile);
        let (response, served) = probe(client(&ca, &server), &server, &ca, OK);
        assert_eq!(response.unwrap().status(), 200);
        served.unwrap();
    }
}

#[test]
fn rejects_invalid_certificates() {
    let ca = issue(rsa_key(), None, Profile::Ca);
    let identity = issue(rsa_key(), Some(&ca), Profile::DnsOnly);
    for (profile, expected) in [
        (Profile::Expired, "certificate has expired"),
        (Profile::NotYetValid, "certificate is not yet valid"),
        (Profile::ClientOnly, "certificate purpose"),
        (Profile::BadSignature, "certificate signature failure"),
    ] {
        let server = issue(rsa_key(), Some(&ca), profile);
        let (response, served) = probe(client(&ca, &identity), &server, &ca, OK);
        let error = format!("{:?}", response.unwrap_err());
        assert!(error.contains(expected), "Expected {expected}, got {error}");
        assert!(served.is_err());
    }
}

#[test]
fn rejects_untrusted_issuer() {
    let ca = issue(rsa_key(), None, Profile::Ca);
    let other_ca = issue(rsa_key(), None, Profile::Ca);
    let identity = issue(rsa_key(), Some(&ca), Profile::DnsOnly);
    let server = issue(rsa_key(), Some(&other_ca), Profile::DnsOnly);
    let (response, served) = probe(client(&ca, &identity), &server, &ca, OK);
    let error = format!("{:?}", response.unwrap_err());
    assert!(
        error.contains("unable to get local issuer certificate"),
        "{error}"
    );
    assert!(served.is_err());
}

#[test]
fn accepts_existing_private_key_formats() {
    let ca = issue(rsa_key(), None, Profile::Ca);
    let rsa = rsa_key();
    let ec = PKey::from_ec_key(
        EcKey::generate(&EcGroup::from_curve_name(Nid::X9_62_PRIME256V1).unwrap()).unwrap(),
    )
    .unwrap();
    let formats = [
        (rsa.clone(), rsa.private_key_to_pem_pkcs8().unwrap()),
        (
            rsa.clone(),
            rsa.rsa().unwrap().private_key_to_pem().unwrap(),
        ),
        (
            ec.clone(),
            ec.ec_key().unwrap().private_key_to_pem().unwrap(),
        ),
    ];
    for (key, pem) in formats {
        let identity = issue(key, Some(&ca), Profile::DnsOnly);
        let client_identity =
            build_client_identity(&identity.cert.to_pem().unwrap(), &pem).unwrap();
        let client = build_restapi_https_client(
            parse_ca_certificates(&ca.cert.to_pem().unwrap()).unwrap(),
            client_identity,
        )
        .unwrap();
        let (response, served) = probe(client, &identity, &ca, OK);
        assert_eq!(response.unwrap().status(), 200);
        served.unwrap();
    }
}

#[test]
fn rejects_malformed_identity() {
    let ca = issue(rsa_key(), None, Profile::Ca);
    assert!(build_client_identity(&ca.cert.to_pem().unwrap(), b"not a private key").is_err());
    assert!(build_client_identity(
        b"not a certificate",
        &ca.key.private_key_to_pem_pkcs8().unwrap()
    )
    .is_err());
}

#[test]
fn rejects_encrypted_keys_without_prompting() {
    let ca = issue(rsa_key(), None, Profile::Ca);
    let encrypted = ca
        .key
        .private_key_to_pem_pkcs8_passphrase(openssl::symm::Cipher::aes_256_cbc(), b"test-only")
        .unwrap();
    let error = build_client_identity(&ca.cert.to_pem().unwrap(), &encrypted).unwrap_err();
    assert_eq!(
        error.to_string(),
        "encrypted private keys are not supported"
    );
}
