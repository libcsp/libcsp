FROM ubuntu:latest
ARG DEBIAN_FRONTEND=noninteractive
MAINTAINER "Iliya Iliev"

# Install dependencies
RUN apt-get update && \
	apt-get install --no-install-recommends --no-install-suggests -y sudo git python3-pip cmake \
	libclang-dev build-essential locales locales-all

ENV LC_ALL en_US.UTF-8
ENV LANG en_US.utf8
ENV LANGUAGE en_US.UTF-8

RUN mkdir /home/libcsp_sphinx_docs && \
	cd /home/libcsp_sphinx_docs && \
	git clone https://github.com/libcsp/libcsp.git --branch develop && \
	cd libcsp && \
	pip3 install -r doc/requirements.txt --break-system-packages && \
	cmake -B build-docs -S doc && \
	cmake --build build-docs

# Set working directory
WORKDIR /home/libcsp_sphinx_docs
CMD /bin/bash
