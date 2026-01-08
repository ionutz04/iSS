FROM telegraf:1.24

LABEL author="ionutz04"
LABEL version="1.0.0-beta"
LABEL description="Telegraf HTTPS API server for IoT sensor data ingestion"

USER root

# Install openssl for certificate generation and curl for health checks
RUN apt-get update && apt-get -y install openssl curl && rm -rf /var/lib/apt/lists/*

# Create directory for certificates
RUN mkdir -p /etc/telegraf/certs

ADD telegraf.conf.template /telegraf.conf.template
ADD run.sh /run.sh
RUN chmod +x /run.sh

EXPOSE 8443

ENTRYPOINT ["bash", "/run.sh"]
