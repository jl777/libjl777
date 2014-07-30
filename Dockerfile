FROM        ubuntu:saucy

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update
RUN apt-get -y install build-essential gcc-4.8 g++-4.8 libboost1.53-all-dev git cmake unzip clang libz-dev libcurl4-openssl-dev imagemagick
ADD . /usr/src/pnxt/
WORKDIR /usr/src/pnxt/src/
RUN ./m_unix
CMD ls -la ../build/release/src/ 
