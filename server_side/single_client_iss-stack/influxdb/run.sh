#!/bin/bash

set -e

# Derive bucket name from CUSTOMER_ID
INFLUXDB_BUCKET="customer_${CUSTOMER_ID}"

echo "========================================"
echo "  InfluxDB 2.x Setup"
echo "  Customer ID: ${CUSTOMER_ID}"
echo "  Bucket: ${INFLUXDB_BUCKET}"
echo "  Organization: ${INFLUXDB_ORG}"
echo "========================================"

# Check if InfluxDB is already set up
if [ -f /var/lib/influxdb2/.setup-complete ]; then
    echo "=> InfluxDB already configured, starting..."
    exec influxd
fi

# Start influxd in background for initial setup
influxd &
INFLUXD_PID=$!

# Wait for InfluxDB to be ready
echo "=> Waiting for InfluxDB to start..."
until curl -s http://localhost:8086/health | grep -q '"status":"pass"'; do
    sleep 1
done
echo "=> InfluxDB is ready!"

# Run initial setup
echo "=> Running initial setup..."
influx setup \
    --username "${INFLUXDB_ADMIN_USER}" \
    --password "${INFLUXDB_ADMIN_PASSWORD}" \
    --org "${INFLUXDB_ORG}" \
    --bucket "${INFLUXDB_BUCKET}" \
    --token "${INFLUXDB_TOKEN}" \
    --force

echo "=> Setup complete!"
echo "=> Bucket '${INFLUXDB_BUCKET}' created for customer ${CUSTOMER_ID}"

# Mark setup as complete
touch /var/lib/influxdb2/.setup-complete

# Stop background influxd and restart in foreground
kill $INFLUXD_PID 2>/dev/null || true
sleep 2

echo "=> Starting InfluxDB in foreground..."
exec influxd
