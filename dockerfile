FROM rcarmo/alpine-python:3.6 as base
RUN apk update
RUN apk add linux-headers 
RUN git clone https://github.com/maekos/libcsp.git /opt/libcsp
WORKDIR /opt/libcsp/
RUN ./waf configure --with-os=posix --enable-examples --enable-if-can --install-csp --prefix=install
RUN ./waf build install

FROM base as socketcand
RUN apk add autoconf automake coreutils 
RUN git clone https://github.com/linux-can/socketcand.git /opt/socketcand
WORKDIR /opt/socketcand
RUN ./autogen.sh
RUN ./configure
RUN make && make install
#ENTRYPOINT ["socketcand", "-l", "eth0", "&"]
#RUN bash -c "socketcand -l eth0 -v&"
