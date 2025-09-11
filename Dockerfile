FROM mycpp:20.04

RUN apt-get update && apt-get install -y libgssapi-krb5-2

# ==================================================
WORKDIR /opt/feeder_huatai_hongkong/bin
CMD ["./feeder_huatai_hongkong"]

# --------------------------------------------------
RUN cd /opt/feeder_huatai_hongkong; mkdir conf log data tmp
COPY feeder_huatai_hongkong *.so* /opt/feeder_huatai_hongkong/bin/

