FROM debian:latest

WORKDIR /srv/app

RUN APT_INSTALL="apt install -y" && \
  apt update && \
  $APT_INSTALL libc6-dev g++ libopenmpi-dev make openssh-client openssh-server && \
  $APT_INSTALL iproute2 iputils-ping dnsutils

ENV SSH_PATH /root/.ssh
ENV SSH_KEY  $SSH_PATH/key

RUN echo "yes" > /tmp/answers && \
  ssh-keygen -t ed25519 -N "" -f $SSH_KEY < /tmp/answers && \
  cat $SSH_KEY.pub >> $SSH_PATH/authorized_keys && \
  chmod 640 $SSH_PATH/authorized_keys && \
  sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config

# Expose SSH Remote Login Protocol port for using MPI
EXPOSE 22

COPY . .

RUN MAKE_ALGORITHM="make --always-make -C" && \
  $MAKE_ALGORITHM hello && \
  $MAKE_ALGORITHM clock && \
  $MAKE_ALGORITHM mutex && \
  $MAKE_ALGORITHM election

RUN rm -rf /tmp/* /var/log/*

ENV OMPI_ALLOW_RUN_AS_ROOT 1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM 1

ENTRYPOINT service ssh start && bash
