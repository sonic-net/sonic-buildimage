#!/bin/bash
#
# Script to generate and install HTTPS certificates for SONiC BMC
#
# Modes:
#   generate (default) - Generate certificates locally
#   install            - Copy generated certificates to the BMC and, with
#                        --push-config, point the redfish container at them
#
# Generates:
#   CA-cert.pem, CA-key.pem          - CA certificate and key
#   server-cert.pem, server-key.pem  - Server certificate and key
#   client-cert.pem, client-key.pem  - Client certificate and key (stays on client)
#
#   The OpenSSL request files it works with are removed afterwards; an
#   existing CA-cert.pem/CA-key.pem pair in the working directory is reused,
#   which is how server and client certificates are rotated under one CA.
#
# How the redfish container consumes them:
#   The certificates are delivered to /etc/sonic/credentials/redfish on the
#   BMC host, which the container mounts, and REDFISH|certs in CONFIG_DB names
#   their paths.
#   stage-credentials.sh stages them into what bmcweb reads and restarts it.
#   bmcweb keeps its default auth configuration (mTLS on, TLSStrict off), so
#   the service root stays reachable without a certificate.
#
#   REDFISH|certs   server_crt        /etc/sonic/credentials/redfish/server-cert.pem
#   REDFISH|certs   server_key        /etc/sonic/credentials/redfish/server-key.pem
#   REDFISH|certs   ca_crt            /etc/sonic/credentials/redfish/CA-cert.pem
#   REDFISH|certs   client_crt_cname  client certificate CN (sonic-dbus-bridge
#                                     reads it on every request)
#
# Usage:
#   Generate:
#     ./generate_certificates.sh -c <CN> -i <IP> [options]
#
#   Install:
#     ./generate_certificates.sh --install --bmc-ip <IP> --bmc-pass <PASS> [--push-config] [options]
#
# Required for generate:
#   -i, --ip IP              Server IP address for SAN
#
# Optional for generate:
#   -c, --common-name CN     Server Common Name (default: bmc.example.com)
#
# Required for install:
#       --bmc-ip IP          BMC IP address
#
# Optional for install:
#       --bmc-pass PASS      BMC SSH password (default: YourPaSsWoRd)
#       --push-config        Write REDFISH|certs to CONFIG_DB and restart the
#                            redfish container so the certificates take effect
#                            at once rather than on the watcher's next pass.
#                            The config is not saved, so the entry is lost on a
#                            reboot or config reload unless saved by hand.
#                            Without it, the commands are printed instead.
#
# Optional:
#   -d, --dir DIR            Working directory (default: current directory)
#       --country CODE       2-letter country code (default: IN)
#       --state STATE        State or province (default: Karnataka)
#       --city CITY          City (default: Bengaluru)
#       --org NAME           Organization name (default: Nexthop AI)
#       --ou OU              Organizational unit (default: BMC)
#       --client-cn CN       Client Common Name for mTLS (default: bmcweb; on
#                            install, read from client-cert.pem when present)
#       --bmc-user USER      BMC SSH user (default: admin)
#       --container NAME     Docker container name (default: redfish)
#   -h, --help               Show this help message
#

set -e

# Default values
WORK_DIR="$(pwd)"
SERVER_CN="bmc.example.com"
SERVER_IP=""
COUNTRY="IN"
STATE="Karnataka"
CITY="Bengaluru"
ORG="Nexthop AI"
OU="BMC"
CLIENT_CN="bmcweb"
CLIENT_CN_SET=false
INSTALL=false
PUSH_CONFIG=false
BMC_IP=""
BMC_USER="admin"
BMC_PASS="YourPaSsWoRd"
CONTAINER="redfish"
REMOTE_DIR="/etc/sonic/credentials/redfish"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--dir)
            WORK_DIR="$2"
            shift 2
            ;;
        -c|--common-name)
            SERVER_CN="$2"
            shift 2
            ;;
        -i|--ip)
            SERVER_IP="$2"
            shift 2
            ;;
        --country)
            COUNTRY="$2"
            shift 2
            ;;
        --state)
            STATE="$2"
            shift 2
            ;;
        --city)
            CITY="$2"
            shift 2
            ;;
        --org)
            ORG="$2"
            shift 2
            ;;
        --ou)
            OU="$2"
            shift 2
            ;;
        --client-cn)
            CLIENT_CN="$2"
            CLIENT_CN_SET=true
            shift 2
            ;;
        --install)
            INSTALL=true
            shift
            ;;
        --push-config)
            PUSH_CONFIG=true
            shift
            ;;
        --bmc-ip)
            BMC_IP="$2"
            shift 2
            ;;
        --bmc-user)
            BMC_USER="$2"
            shift 2
            ;;
        --bmc-pass)
            BMC_PASS="$2"
            shift 2
            ;;
        --container)
            CONTAINER="$2"
            shift 2
            ;;
        -h|--help)
            grep '^#' "$0" | grep -v '#!/bin/bash' | sed 's/^# //'
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

TOTAL=6

# =========================================================================
# INSTALL-ONLY MODE: skip generation, deliver existing certs to the BMC
# =========================================================================

if $INSTALL; then
    cd "$WORK_DIR"

    # Validate install requirements
    if [ -z "$BMC_IP" ]; then
        echo "ERROR: Missing required option: --bmc-ip"
        echo "Usage: $0 --install --bmc-ip <IP> [--bmc-pass <PASS>] [--bmc-user <USER>] [--push-config] [--container <NAME>] [-d <DIR>]"
        exit 1
    fi

    for f in server-cert.pem server-key.pem CA-cert.pem; do
        if [ ! -f "$f" ]; then
            echo "ERROR: $f not found in $WORK_DIR"
            echo "Run without --install first to generate certificates."
            exit 1
        fi
    done

    # The trusted client CN is the one the client certificate was issued with.
    if ! $CLIENT_CN_SET && [ -f client-cert.pem ]; then
        cn="$(openssl x509 -in client-cert.pem -noout -subject -nameopt RFC2253 | sed -n 's/^subject=.*CN=\([^,]*\).*/\1/p')"
        [ -n "$cn" ] && CLIENT_CN="$cn"
    fi

    # Check if SSH key auth works, otherwise require sshpass
    if ssh -o StrictHostKeyChecking=no -o BatchMode=yes -o ConnectTimeout=5 "${BMC_USER}@${BMC_IP}" true 2>/dev/null; then
        echo "Using SSH key authentication"
        SCP="scp -o StrictHostKeyChecking=no"
        SSH="ssh -o StrictHostKeyChecking=no"
    elif command -v sshpass &>/dev/null; then
        echo "Using sshpass for authentication"
        export SSHPASS="$BMC_PASS"
        SCP="sshpass -e scp -o StrictHostKeyChecking=no"
        SSH="sshpass -e ssh -o StrictHostKeyChecking=no"
    else
        echo "ERROR: Cannot authenticate to BMC."
        echo ""
        echo "Either:"
        echo "  1. Set up SSH key auth:  ssh-copy-id ${BMC_USER}@${BMC_IP}"
        echo "  2. Install sshpass:      sudo apt install sshpass"
        exit 1
    fi

    SERVER_CRT_PATH="${REMOTE_DIR}/server-cert.pem"
    SERVER_KEY_PATH="${REMOTE_DIR}/server-key.pem"
    CA_CRT_PATH="${REMOTE_DIR}/CA-cert.pem"
    DB_SET="sudo sonic-db-cli CONFIG_DB HSET 'REDFISH|certs' server_crt '${SERVER_CRT_PATH}' server_key '${SERVER_KEY_PATH}' ca_crt '${CA_CRT_PATH}' client_crt_cname '${CLIENT_CN}'"

    echo "==========================================================="
    echo " INSTALLING CERTIFICATES"
    echo "==========================================================="
    echo ""
    echo "  Working dir: $WORK_DIR"
    echo "  BMC IP:      $BMC_IP"
    echo "  BMC User:    $BMC_USER"
    echo "  Remote dir:  $REMOTE_DIR"
    echo "  Client CN:   $CLIENT_CN"
    echo "  Container:   $CONTAINER"
    echo ""

    # --- Step 1: Deliver the certificates to the BMC host ---
    echo "--- Step 1: Copying certificates to ${REMOTE_DIR} ---"
    echo ""

    $SCP server-cert.pem server-key.pem CA-cert.pem "${BMC_USER}@${BMC_IP}:/tmp/"
    $SSH "${BMC_USER}@${BMC_IP}" "
        set -e
        sudo mkdir -p '${REMOTE_DIR}'
        sudo install -m 0644 /tmp/server-cert.pem '${SERVER_CRT_PATH}'
        sudo install -m 0600 /tmp/server-key.pem '${SERVER_KEY_PATH}'
        sudo install -m 0644 /tmp/CA-cert.pem '${CA_CRT_PATH}'
        rm -f /tmp/server-cert.pem /tmp/server-key.pem /tmp/CA-cert.pem
    "

    echo "    Certificates installed"
    echo ""

    if ! $PUSH_CONFIG; then
        echo "--- Not applied: --push-config not given ---"
        echo ""
        echo "  To make the $CONTAINER container use them, run on the BMC:"
        echo ""
        echo "    $DB_SET"
        echo "    sudo systemctl restart ${CONTAINER}"
        echo ""
        echo "=== Installation Complete (config not pushed) ==="
        exit 0
    fi

    # --- Step 2: Point the redfish container at them ---
    echo "--- Step 2: Writing REDFISH|certs to CONFIG_DB ---"
    echo ""

    # Not saved: config save would persist every other unsaved change too.
    $SSH "${BMC_USER}@${BMC_IP}" "${DB_SET} >/dev/null"

    echo "    REDFISH|certs set (not saved)"
    echo ""

    # --- Step 3: Restart the container ---
    # The watcher would stage the certificates on its next pass (up to a
    # minute); the restart stages them now, through the same guard.
    echo "--- Step 3: Restarting ${CONTAINER} ---"
    echo ""

    state="$($SSH "${BMC_USER}@${BMC_IP}" "sonic-db-cli CONFIG_DB HGET 'FEATURE|${CONTAINER}' state" 2>/dev/null || true)"
    if [ "$state" != "enabled" ]; then
        echo "    WARNING: feature ${CONTAINER} is '${state:-unknown}', not enabled; skipping the restart."
        echo "    The certificates take effect when the feature is enabled."
        echo ""
        echo "=== Installation Complete (container not restarted) ==="
        exit 0
    fi

    $SSH "${BMC_USER}@${BMC_IP}" "sudo systemctl restart ${CONTAINER}"

    echo "  Waiting for bmcweb..."
    up=false
    for _ in $(seq 1 45); do
        if $SSH "${BMC_USER}@${BMC_IP}" "docker exec ${CONTAINER} supervisorctl status bmcweb 2>/dev/null | grep -q RUNNING"; then
            up=true
            break
        fi
        sleep 2
    done
    if $up; then
        echo "    bmcweb running"
    else
        echo "    WARNING: bmcweb not RUNNING after 90s; check: docker exec ${CONTAINER} supervisorctl status"
    fi
    echo ""

    # --- Verify ---
    echo "--- Verifying installation ---"
    echo ""

    echo "  Checking server certificate..."
    openssl s_client -connect "${BMC_IP}:443" -showcerts < /dev/null 2>/dev/null | \
        openssl x509 -text -noout | grep -E "Subject:|Issuer:|DNS:|IP:" || true

    # TLSStrict stays off: the service root must answer without a certificate.
    # Access is decided per request from the client CN, so a privileged path
    # is what shows the client certificate being accepted.
    echo ""
    echo -n "  Service root without a client certificate (expect 200): "
    curl -s -o /dev/null -w '%{http_code}\n' --noproxy "${BMC_IP}" \
        --cacert CA-cert.pem "https://${BMC_IP}/redfish/v1/" || echo "request failed"

    if [ -f client-cert.pem ] && [ -f client-key.pem ]; then
        echo -n "  /redfish/v1/Managers with the client certificate (expect 200): "
        curl -s -o /dev/null -w '%{http_code}\n' --noproxy "${BMC_IP}" \
            --cert client-cert.pem --key client-key.pem --cacert CA-cert.pem \
            "https://${BMC_IP}/redfish/v1/Managers" || echo "request failed"
    fi

    echo ""
    echo "  Test mTLS with:"
    echo ""
    echo "    curl --noproxy ${BMC_IP} --cert $WORK_DIR/client-cert.pem --key $WORK_DIR/client-key.pem \\"
    echo "         --cacert $WORK_DIR/CA-cert.pem \\"
    echo "         https://${BMC_IP}/redfish/v1/Managers"
    echo ""

    echo "=== Installation Complete! ==="
    exit 0
fi

# =========================================================================
# GENERATE MODE
# =========================================================================

# Validate required options for generate
MISSING=""
[ -z "$SERVER_IP" ] && MISSING="$MISSING -i"
if [ -n "$MISSING" ]; then
    echo "ERROR: Missing required options:$MISSING"
    echo ""
    echo "Usage: $0 -i <IP> [options]"
    echo ""
    echo "Example:"
    echo "  $0 -i <BMC_IP> -c <BMC_HOSTNAME> --client-cn bmcweb"
    echo ""
    echo "Run with -h for full usage."
    exit 1
fi

# Derive CA Common Name from organization
CA_CN="${ORG} BMC CA"

echo "=== SONiC BMC HTTPS Certificate Generator ==="
echo ""
echo "Working directory: $WORK_DIR"
echo "Server Common Name: $SERVER_CN"
if [ -n "$SERVER_IP" ]; then
    echo "Server IP (SAN): $SERVER_IP"
fi
echo "Client CN (mTLS): $CLIENT_CN"
echo ""

# Create working directory
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

# Step 1: Create OpenSSL configuration for server certificate
echo "[1/$TOTAL] Creating OpenSSL configuration..."

cat > openssl-server.cnf << 'EOF'
[ req ]
default_bits = 2048
prompt = no
default_md = sha256
distinguished_name = dn
req_extensions = v3_req

[ dn ]
C = COUNTRY_PLACEHOLDER
ST = STATE_PLACEHOLDER
L = CITY_PLACEHOLDER
O = ORG_PLACEHOLDER
OU = OU_PLACEHOLDER
CN = CN_PLACEHOLDER

[ v3_req ]
keyUsage = digitalSignature, keyAgreement
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[ alt_names ]
DNS.1 = CN_PLACEHOLDER
EOF

# Replace placeholders
sed -i "s/COUNTRY_PLACEHOLDER/$COUNTRY/" openssl-server.cnf
sed -i "s/STATE_PLACEHOLDER/$STATE/" openssl-server.cnf
sed -i "s/CITY_PLACEHOLDER/$CITY/" openssl-server.cnf
sed -i "s/ORG_PLACEHOLDER/$ORG/" openssl-server.cnf
sed -i "s/OU_PLACEHOLDER/$OU/" openssl-server.cnf
sed -i "s/CN_PLACEHOLDER/$SERVER_CN/g" openssl-server.cnf

# Add IP to SAN if provided
if [ -n "$SERVER_IP" ]; then
    echo "IP.1 = $SERVER_IP" >> openssl-server.cnf
fi

# Step 2: Create extension file for CA signing
# NOTE: openssl x509 -req does NOT copy extensions from the CSR, so we must
#       include subjectAltName here to preserve SAN in the signed certificate.
echo "[2/$TOTAL] Creating certificate extension file..."

cat > myext-server.cnf << EOFEXT
[ my_ext_section ]
keyUsage = digitalSignature, keyAgreement
extendedKeyUsage = serverAuth
authorityKeyIdentifier = keyid
subjectKeyIdentifier = hash
subjectAltName = @alt_names

[ alt_names ]
DNS.1 = $SERVER_CN
EOFEXT

if [ -n "$SERVER_IP" ]; then
    echo "IP.1 = $SERVER_IP" >> myext-server.cnf
fi

# Step 3: Generate CA certificate (if not exists)
if [ ! -f "CA-cert.pem" ] || [ ! -f "CA-key.pem" ]; then
    echo "[3/$TOTAL] Generating CA certificate..."

    openssl genrsa -out CA-key.pem 2048

    openssl req -new -x509 -days 3650 -key CA-key.pem -out CA-cert.pem \
        -subj "/C=$COUNTRY/ST=$STATE/L=$CITY/O=$ORG/OU=$OU/CN=$CA_CN"

    echo "    CA certificate created"
else
    echo "[3/$TOTAL] Using existing CA certificate"
fi

# Step 4: Generate server private key and CSR, sign with the CA
echo "[4/$TOTAL] Generating server certificate..."

openssl genrsa -out server-key.pem 2048

openssl req -new -config openssl-server.cnf -key server-key.pem -out server.csr

openssl x509 -req -extensions my_ext_section -extfile myext-server.cnf -days 730 \
    -in server.csr -CA CA-cert.pem -CAkey CA-key.pem -CAcreateserial \
    -out server-cert.pem

echo "    Server certificate created"

# Verify server certificates
echo ""
echo "=== Verification ==="

echo -n "CA certificate: "
openssl x509 -in CA-cert.pem -noout -subject -dates | head -1

echo -n "Server certificate: "
openssl verify -CAfile CA-cert.pem server-cert.pem

echo ""
echo "Server certificate details:"
openssl x509 -in server-cert.pem -noout -subject -issuer -dates -ext subjectAltName

# ---------------------------------------------------------------------------
# mTLS: client certificate
# ---------------------------------------------------------------------------
echo ""
echo "=== mTLS ==="
echo ""

# Step 5: OpenSSL config for client certificate
echo "[5/$TOTAL] Creating client certificate OpenSSL configuration..."

cat > openssl-client.cnf << 'EOF'
[ req ]
default_bits = 2048
prompt = no
default_md = sha256
distinguished_name = dn
req_extensions = v3_req

[ dn ]
C = COUNTRY_PLACEHOLDER
ST = STATE_PLACEHOLDER
L = CITY_PLACEHOLDER
O = ORG_PLACEHOLDER
OU = OU_PLACEHOLDER
CN = CN_PLACEHOLDER

[ v3_req ]
keyUsage = digitalSignature
extendedKeyUsage = clientAuth
EOF

sed -i "s/COUNTRY_PLACEHOLDER/$COUNTRY/" openssl-client.cnf
sed -i "s/STATE_PLACEHOLDER/$STATE/" openssl-client.cnf
sed -i "s/CITY_PLACEHOLDER/$CITY/" openssl-client.cnf
sed -i "s/ORG_PLACEHOLDER/$ORG/" openssl-client.cnf
sed -i "s/OU_PLACEHOLDER/$OU/" openssl-client.cnf
sed -i "s/CN_PLACEHOLDER/$CLIENT_CN/g" openssl-client.cnf

cat > myext-client.cnf << 'EOF'
[ my_ext_section ]
keyUsage = digitalSignature
extendedKeyUsage = clientAuth
authorityKeyIdentifier = keyid
subjectKeyIdentifier = hash
EOF

# Step 6: Generate client private key, CSR, and sign with CA
echo "[6/$TOTAL] Generating client certificate..."

openssl genrsa -out client-key.pem 2048

openssl req -new -config openssl-client.cnf -key client-key.pem -out client.csr

openssl x509 -req -extensions my_ext_section -extfile myext-client.cnf -days 730 \
    -in client.csr -CA CA-cert.pem -CAkey CA-key.pem -CAcreateserial \
    -out client-cert.pem

echo "    Client certificate created"

# Verify client cert
echo -n "Client certificate: "
openssl verify -CAfile CA-cert.pem client-cert.pem

echo ""
echo "Client certificate details:"
openssl x509 -in client-cert.pem -noout -subject -issuer -dates

# Only the certificates and keys are kept; the request configs, CSRs and the
# serial file are OpenSSL working files.
rm -f openssl-server.cnf myext-server.cnf server.csr \
      openssl-client.cnf myext-client.cnf client.csr CA-cert.srl

echo ""
echo "=== Certificate Generation Complete! ==="
echo ""
echo "Generated files:"
echo "  CA-cert.pem              - CA certificate"
echo "  CA-key.pem               - CA private key (keep secure)"
echo "  server-cert.pem          - Server certificate"
echo "  server-key.pem           - Server private key"
echo "  client-cert.pem          - Client certificate (stays on client)"
echo "  client-key.pem           - Client private key (stays on client)"
echo ""

echo ""
echo "To install certificates into the BMC, run:"
echo ""
echo "  $0 --install --bmc-ip <BMC_IP> [--bmc-pass <PASS>] [--bmc-user <USER>] [-d $WORK_DIR] [--push-config]"
echo ""
