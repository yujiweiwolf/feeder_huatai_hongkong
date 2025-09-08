FROM mycpp:20.04

RUN apt-get update && apt-get install -y libgssapi-krb5-2

# ==================================================
WORKDIR /opt/feeder_huatai_hongkang/bin
CMD ["./feeder_huatai_hongkang"]

# --------------------------------------------------
RUN cd /opt/feeder_huatai_hongkang; mkdir conf log data tmp
COPY feeder_huatai_hongkang *.so* /opt/feeder_huatai_hongkang/bin/

