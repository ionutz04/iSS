FROM influxdb:2.7

LABEL author="ionutz04"
LABEL version="2.0.0-beta"
LABEL description="InfluxDB 2.x docker image for IoT sensor data"

USER root

ADD run.sh /run.sh
RUN chmod +x /run.sh

EXPOSE 8086

ENTRYPOINT ["bash", "/run.sh"]
