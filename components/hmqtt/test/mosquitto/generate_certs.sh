#!/bin/bash

# --- Configuration ---
# Set this to your broker's IP or Domain Name
IP_OR_HOSTNAME="localhost"

# Certificate Subjects (Adjust as needed)
CA_SUBJ="/C=US/ST=State/L=City/O=MyCA/CN=MyRootCA"
SERVER_SUBJ="/C=US/ST=State/L=City/O=MyBroker/CN=$IP_OR_HOSTNAME"
CLIENT_SUBJ="/C=US/ST=State/L=City/O=MyClient/CN=client1"

# Create directory for output
mkdir -p certs && cd certs
echo "Generating certificates in $(pwd)..."

# 1. Create Certificate Authority (CA)
# Generates ca.key and ca.crt
openssl genrsa -out ca.key 2048
openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -subj "$CA_SUBJ" -out ca.crt

# 2. Create Server Certificate
# Generates server.key and server.crt (signed by CA)
openssl genrsa -out server.key 2048
openssl req -new -key server.key -subj "$SERVER_SUBJ" -out server.csr
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 3650 -sha256

# 3. Create Client Certificate
# Generates client.key and client.crt (signed by CA)
openssl genrsa -out client.key 2048
openssl req -new -key client.key -subj "$CLIENT_SUBJ" -out client.csr
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 3650 -sha256

# Cleanup temporary signing request files
rm *.csr *.srl
chmod 644 *
echo "-------------------------------------------------"
echo "Generation Complete. Files created:"
ls -1

