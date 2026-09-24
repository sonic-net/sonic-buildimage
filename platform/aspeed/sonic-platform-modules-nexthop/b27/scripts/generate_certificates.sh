#!/bin/bash
#
# Script to generate and install HTTPS certificates for SONiC BMC
#
# Modes:
#   generate (default) — Generate certificates locally
#   install            — Install generated certificates into the redfish container
#
# Generates:
#   CA-cert.pem, CA-key.pem          — CA certificate and key
#   server-cert.pem, server-key.pem  — Server certificate and key
#   server-combined.pem              — Combined cert+key (for /etc/ssl/certs/https/server.pem)
#   client-cert.pem, client-key.pem  — Client certificate and key (stays on client)
#   client-combined.pem              — Combined client cert+key
#   bmcweb_tls_config.json           — bmcweb persistent data with TLSStrict
#
# BMC container paths:
#   /etc/ssl/certs/https/server.pem    — HTTPS server certificate (cert+key)
#   /etc/ssl/certs/authority/<id>.pem  — CA certificates for client verification
#   /bmcweb_persistent_data.json       — bmcweb auth configuration
#
# Usage:
#   Generate:
#     ./generate_https_certificate.sh -c <CN> -i <IP> [options]
#
#   Install:
#     ./generate_https_certificate.sh --install --bmc-ip <IP> --bmc-pass <PASS> [options]
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
#
# Optional:
#   -d, --dir DIR            Working directory (default: current directory)
#       --country CODE       2-letter country code (default: IN)
#       --state STATE        State or province (default: Karnataka)
#       --city CITY          City (default: Bengaluru)
#       --org NAME           Organization name (default: Nexthop AI)
#       --ou OU              Organizational unit (default: BMC)
#       --client-cn CN       Client Common Name for mTLS (default: bmcweb)
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
INSTALL=false
BMC_IP=""
BMC_USER="admin"
BMC_PASS="YourPaSsWoRd"
CONTAINER="redfish"

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
            shift 2
            ;;
        --install)
            INSTALL=true
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

TOTAL=8

# =========================================================================
# INSTALL-ONLY MODE — skip generation, just install existing certs
# =========================================================================

if $INSTALL; then
    cd "$WORK_DIR"

    # Validate install requirements
    if [ -z "$BMC_IP" ]; then
        echo "ERROR: Missing required option: --bmc-ip"
        echo "Usage: $0 --install --bmc-ip <IP> [--bmc-pass <PASS>] [--bmc-user <USER>] [--container <NAME>] [-d <DIR>]"
        exit 1
    fi

    for f in server-combined.pem CA-cert.pem bmcweb_tls_config.json; do
        if [ ! -f "$f" ]; then
            echo "ERROR: $f not found in $WORK_DIR"
            echo "Run without --install first to generate certificates."
            exit 1
        fi
    done

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

    echo "==========================================================="
    echo " INSTALLING CERTIFICATES"
    echo "==========================================================="
    echo ""
    echo "  Working dir: $WORK_DIR"
    echo "  BMC IP:      $BMC_IP"
    echo "  BMC User:    $BMC_USER"
    echo "  Container:   $CONTAINER"
    echo ""

    # --- Step 1: Install server certificate ---
    echo "--- Step 1: Installing HTTPS server certificate ---"
    echo ""

    echo "  Copying server-combined.pem to BMC..."
    $SCP server-combined.pem "${BMC_USER}@${BMC_IP}:/tmp/server-combined.pem"

    echo "  Installing into container $CONTAINER:/etc/ssl/certs/https/server.pem..."
    $SSH "${BMC_USER}@${BMC_IP}" "docker cp /tmp/server-combined.pem ${CONTAINER}:/etc/ssl/certs/https/server.pem"

    echo "    Server certificate installed"
    echo ""

    # --- Step 2: Install CA cert to truststore ---
    echo "--- Step 2: Installing CA certificate to truststore ---"
    echo ""

    echo "  Copying CA-cert.pem to BMC..."
    $SCP CA-cert.pem "${BMC_USER}@${BMC_IP}:/tmp/CA-cert.pem"

    echo "  Creating truststore directory and installing CA cert..."
    $SSH "${BMC_USER}@${BMC_IP}" "
        docker exec ${CONTAINER} mkdir -p /etc/ssl/certs/authority
        docker cp /tmp/CA-cert.pem ${CONTAINER}:/etc/ssl/certs/authority/CA-cert.pem
        docker exec ${CONTAINER} bash -c 'cd /etc/ssl/certs/authority && \
            ln -sf CA-cert.pem \$(openssl x509 -hash -noout -in CA-cert.pem).0'
    "

    echo "    CA certificate installed to truststore"
    echo ""

    # --- Step 3: Enable TLSStrict ---
    echo "--- Step 3: Enabling TLSStrict on bmcweb ---"
    echo ""

    echo "  Stopping bmcweb..."
    $SSH "${BMC_USER}@${BMC_IP}" "docker exec ${CONTAINER} supervisorctl stop bmcweb"

    echo "  Copying TLS config to BMC..."
    $SCP bmcweb_tls_config.json "${BMC_USER}@${BMC_IP}:/tmp/bmcweb_tls_config.json"

    echo "  Installing bmcweb_persistent_data.json..."
    $SSH "${BMC_USER}@${BMC_IP}" "
        docker cp /tmp/bmcweb_tls_config.json ${CONTAINER}:/bmcweb_persistent_data.json
    "

    echo "    TLSStrict configuration installed"
    echo ""

    # --- Restart bmcweb ---
    echo "--- Restarting bmcweb ---"
    echo ""

    $SSH "${BMC_USER}@${BMC_IP}" "docker exec ${CONTAINER} supervisorctl start bmcweb"

    echo "    bmcweb restarted"
    echo ""

    # --- Verify ---
    echo "--- Verifying installation ---"
    echo ""

    echo "  Checking server certificate..."
    openssl s_client -connect "${BMC_IP}:443" -showcerts < /dev/null 2>/dev/null | \
        openssl x509 -text -noout | grep -E "Subject:|Issuer:|DNS:|IP:" || true

    echo ""
    echo "  Test mTLS with:"
    echo ""
    echo "    curl --noproxy ${BMC_IP} --cert $WORK_DIR/client-cert.pem --key $WORK_DIR/client-key.pem \\"
    echo "         --cacert $WORK_DIR/CA-cert.pem \\"
    echo "         https://${BMC_IP}/redfish/v1/"
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
    echo "  $0 -i <BMC_IP> -c <BMC_IP> --client-cn bmcweb"
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

# Step 4: Generate server private key and CSR
echo "[4/$TOTAL] Generating server certificate..."

openssl genrsa -out server-key.pem 2048

openssl req -new -config openssl-server.cnf -key server-key.pem -out server.csr

# Step 5: Sign server certificate with CA
openssl x509 -req -extensions my_ext_section -extfile myext-server.cnf -days 730 \
    -in server.csr -CA CA-cert.pem -CAkey CA-key.pem -CAcreateserial \
    -out server-cert.pem

echo "    Server certificate created"

# Step 5: Create combined PEM (cert + key for bmcweb)
echo "[5/$TOTAL] Creating combined PEM file..."

cat server-cert.pem server-key.pem > server-combined.pem

echo "    Combined PEM: server-combined.pem"

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
# mTLS — client certificate + CA truststore + TLSStrict config
# ---------------------------------------------------------------------------
echo ""
echo "=== mTLS ==="
echo ""

# Step 6: OpenSSL config for client certificate
echo "[6/$TOTAL] Creating client certificate OpenSSL configuration..."

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

# Step 7: Generate client private key, CSR, and sign with CA
echo "[7/$TOTAL] Generating client certificate..."

openssl genrsa -out client-key.pem 2048

openssl req -new -config openssl-client.cnf -key client-key.pem -out client.csr

openssl x509 -req -extensions my_ext_section -extfile myext-client.cnf -days 730 \
    -in client.csr -CA CA-cert.pem -CAkey CA-key.pem -CAcreateserial \
    -out client-cert.pem

cat client-cert.pem client-key.pem > client-combined.pem

echo "    Client certificate created"

# Verify client cert
echo -n "Client certificate: "
openssl verify -CAfile CA-cert.pem client-cert.pem

echo ""
echo "Client certificate details:"
openssl x509 -in client-cert.pem -noout -subject -issuer -dates

# Step 8: Create bmcweb TLSStrict config
echo ""
echo "[8/$TOTAL] Creating mTLS configuration files..."

# bmcweb persistent data — enable mTLS with TLSStrict
cat > bmcweb_tls_config.json << 'EOFJ'
{
  "auth_config": {
    "BasicAuth": true,
    "Cookie": true,
    "SessionToken": true,
    "XToken": true,
    "TLS": true,
    "TLSStrict": true,
    "MTLSCommonNameParseMode": 2
  },
  "sessions": [],
  "revision": 1
}
EOFJ

echo "    TLSStrict config: bmcweb_tls_config.json"

echo ""
echo "=== Certificate Generation Complete! ==="
echo ""
echo "Generated files:"
echo "  CA-cert.pem              — CA certificate"
echo "  CA-key.pem               — CA private key (keep secure)"
echo "  server-cert.pem          — Server certificate"
echo "  server-key.pem           — Server private key"
echo "  server-combined.pem      — Server cert + key combined"
echo "  client-cert.pem          — Client certificate (stays on client)"
echo "  client-key.pem           — Client private key (stays on client)"
echo "  client-combined.pem      — Client cert + key combined"
echo "  bmcweb_tls_config.json   — bmcweb TLSStrict config"
echo ""

echo ""
echo "To install certificates into the BMC, run:"
echo ""
echo "  $0 --install --bmc-ip <BMC_IP> [--bmc-pass <PASS>] [--bmc-user <USER>] [-d $WORK_DIR]"
echo ""
