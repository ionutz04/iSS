#!/bin/bash

# Don't use set -e as it causes issues with health check loops

CONFIG_TEMPLATE="/telegraf.conf.template"
CONFIG_FILE="/etc/telegraf/telegraf.conf"
CERT_DIR="/etc/telegraf/certs"
CERT_FILE="${CERT_DIR}/cert.pem"
KEY_FILE="${CERT_DIR}/key.pem"

# Derive bucket name from CUSTOMER_ID (same as InfluxDB)
INFLUXDB_BUCKET="customer_${CUSTOMER_ID}"

echo "========================================"
echo "  Telegraf HTTPS API Server"
echo "  Customer ID: ${CUSTOMER_ID}"
echo "  Bucket: ${INFLUXDB_BUCKET}"
echo "  Organization: ${INFLUXDB_ORG}"
echo "========================================"

# Generate self-signed certificate if it doesn't exist
if [ ! -f "$CERT_FILE" ] || [ ! -f "$KEY_FILE" ]; then
    echo "=> Generating self-signed TLS certificate..."
    openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
        -keyout "$KEY_FILE" \
        -out "$CERT_FILE" \
        -subj "/CN=telegraf-iot-server/O=IoT-Stack/C=RO" \
        2>/dev/null
    
    echo "========================================"
    echo "  TLS CERTIFICATE FINGERPRINT"
    echo "  (Use this in ESP8266 firmware)"
    echo "========================================"
    openssl x509 -in "$CERT_FILE" -noout -fingerprint -sha256
    echo "========================================"
fi

# Resolve InfluxDB hostname to IP (workaround for curl DNS issues)
INFLUXDB_IP=$(getent hosts ${INFLUXDB_HOST} | awk '{print $1}')
if [ -z "$INFLUXDB_IP" ]; then
    echo "=> WARNING: Could not resolve ${INFLUXDB_HOST}, using hostname directly"
    INFLUXDB_IP="${INFLUXDB_HOST}"
fi
echo "=> InfluxDB IP: ${INFLUXDB_IP}"

# Wait for InfluxDB to be ready
echo "=> Waiting for InfluxDB at http://${INFLUXDB_IP}:${INFLUXDB_PORT}/health ..."
MAX_RETRIES=60
RETRY_COUNT=0

while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    HEALTH_RESPONSE=$(curl -s "http://${INFLUXDB_IP}:${INFLUXDB_PORT}/health" 2>/dev/null || echo "connection failed")
    
    if echo "$HEALTH_RESPONSE" | grep -q '"status":"pass"' 2>/dev/null; then
        echo "=> InfluxDB is ready!"
        break
    fi
    
    RETRY_COUNT=$((RETRY_COUNT + 1))
    echo "   Attempt $RETRY_COUNT/$MAX_RETRIES - Waiting..."
    sleep 2
done

if [ $RETRY_COUNT -eq $MAX_RETRIES ]; then
    echo "=> WARNING: InfluxDB not ready after $MAX_RETRIES attempts. Starting anyway..."
fi

# Generate telegraf config from template
# Use resolved IP for InfluxDB to avoid DNS issues
echo "=> Generating Telegraf configuration..."
sed -e "s/\${TELEGRAF_HOST}/$TELEGRAF_HOST/g" \
    -e "s!\${INFLUXDB_HOST}!$INFLUXDB_IP!g" \
    -e "s/\${INFLUXDB_PORT}/$INFLUXDB_PORT/g" \
    -e "s/\${INFLUXDB_BUCKET}/$INFLUXDB_BUCKET/g" \
    -e "s/\${INFLUXDB_ORG}/$INFLUXDB_ORG/g" \
    -e "s/\${INFLUXDB_TOKEN}/$INFLUXDB_TOKEN/g" \
    -e "s/\${CUSTOMER_ID}/$CUSTOMER_ID/g" \
    -e "s/\${API_SECRET}/$API_SECRET/g" \
    -e "s!\${CERT_FILE}!$CERT_FILE!g" \
    -e "s!\${KEY_FILE}!$KEY_FILE!g" \
    "$CONFIG_TEMPLATE" > "$CONFIG_FILE"

echo "=> Starting Telegraf HTTPS server on port 8443..."
exec telegraf -config /etc/telegraf/telegraf.conf
