FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive
MAINTAINER "Iliya Iliev"

# Install dependencies
RUN apt-get update && \
	apt-get install --no-install-recommends --no-install-suggests -y sudo git python3-pip cmake \
	libclang-14-dev build-essential locales locales-all

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.utf8
ENV LANGUAGE en_US.UTF-8

# Install libxdp & libbpf
RUN mkdir /home/libcsp_sphinx_docs && \
	cd /home/libcsp_sphinx_docs && \
	git clone https://github.com/IlievIliya92/libcsp.git --branch develop && \
	cd libcsp/doc && \
	pip3 install -r requirements.txt && \
	mkdir build && \
	cd build && \
	cmake .. && \
	make

# Set working directory
WORKDIR /home/libcsp_sphinx_docs
CMD /bin/bash
