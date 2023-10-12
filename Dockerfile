## blocksnds and agbabi required different versions of arm-none-eabi for some
## reason. Therefore we use a multistage docker build
FROM devkitpro/devkitarm as base
RUN apt-get install -y build-essential

################################################################################
###  Build agbabi                                                            ###
################################################################################
FROM base as agbabi

RUN apt-get install -y cmake gcc-arm-none-eabi

RUN git clone https://github.com/felixjones/agbabi && \
    cd agbabi && \
    git checkout v2.1.4 && \
    cmake -S . -DCMAKE_TOOLCHAIN_FILE=cross/agb.cmake -B build && \
    cmake --build build && \
    cmake --install build

###############################################################################
###  Build blocksds required for NDS                                        ###
###############################################################################
FROM base as blocksds
#### Reference: https://github.com/blocksds/sdk/blob/master/docker/Dockerfile

RUN apt-get install -y libfreeimage-dev

RUN mkdir -p /opt/wonderful/ && \
    export ARCH=$(lscpu | grep -oP 'Architecture:\s*\K.+') && \
    curl -SL https://wonderful.asie.pl/bootstrap/wf-bootstrap-${ARCH}.tar.gz | tar -xzf - -C /opt/wonderful/

ENV PATH /opt/wonderful/toolchain/gcc-arm-none-eabi/bin/:/opt/wonderful/bin:$PATH

RUN cd etc && \
    ln -sf ../proc/self/mounts mtab && \
    wf-pacman -Syu --noconfirm && \
    wf-pacman -S --noconfirm toolchain-gcc-arm-none-eabi

RUN git clone --recurse-submodules https://github.com/blocksds/sdk.git && \
    cd sdk && \
    git checkout v0.8.1 && \
    BLOCKSDS=$PWD make -j`nproc` && \
    mkdir /opt/blocksds/ && sudo chown $USER:$USER /opt/blocksds && \
    mkdir /opt/blocksds/external && \
    make install

COPY --from=agbabi /usr/local/ /usr/local/

ENV LIBAGBABI=/usr/local

WORKDIR pokemon_gen3_to_genx

CMD make
