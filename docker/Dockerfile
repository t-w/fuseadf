FROM debian:stable

RUN apt-get update && \
    apt-get install -y \
        gcc \
        make \
        pkg-config \
        libfuse-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

COPY *.deb /tmp/

RUN dpkg -i /tmp/*.deb

#EXPOSE 4000

WORKDIR /tmp

#ENTRYPOINT [ "bundle", "exec", "" ]

CMD ["/sbin/bash","","",""]
