# MosaicOS 0.50 Aero — entorno reproducible para compilar la ISO i386.
# Construcción de la imagen (desde la carpeta desktop):
#   docker build -f docker/Dockerfile -t mosaicos-builder .
# Ejecución (desde la raíz del proyecto):
#   docker run --rm -v "${PWD}:/work" mosaicos-builder
FROM debian:bookworm

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
      live-build \
      debootstrap \
      squashfs-tools \
      xorriso \
      mtools \
      dosfstools \
      cpio \
      isolinux \
      syslinux-common \
      syslinux-utils \
      file \
      ca-certificates \
      curl \
      git \
      python3 \
      qemu-user-static \
      binfmt-support \
    && rm -rf /var/lib/apt/lists/*

COPY docker/entrypoint.sh /usr/local/bin/mosaic-build
RUN chmod 0755 /usr/local/bin/mosaic-build

WORKDIR /work
ENTRYPOINT ["/usr/local/bin/mosaic-build"]
