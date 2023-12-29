################################################################################
###  Build agbabi                                                            ###
################################################################################
FROM debian:stable-slim as agbabi
RUN apt-get update && apt-get install -y build-essential cmake gcc-arm-none-eabi git
RUN git clone https://github.com/felixjones/agbabi && \
    cd agbabi && \
    git checkout 2adf1e6c01f9d4acd6f8e728892c5e18aae5332e && \
    cmake -S . -DCMAKE_TOOLCHAIN_FILE=cross/agb.cmake -DCMAKE_BUILD_TYPE=MinSizeRel -B build && \
    cmake --build build && \
    cmake --install build

###############################################################################
###  NDS Builder                                                            ###
###############################################################################
FROM skylyrac/blocksds:slim-latest as ds-build
RUN mkdir -p /home/builder/pokemon_gen3_to_genx
RUN mkdir -p /home/builder/building
RUN chown -R ubuntu:ubuntu /home/builder
COPY --from=agbabi /usr/local/ /opt/agbabi/
ENV LIBAGBABI=/opt/agbabi
WORKDIR /home/builder/building
USER ubuntu
CMD /home/builder/pokemon_gen3_to_genx/docker-scripts/docker_build_nds

###############################################################################
###  GBA Builder                                                            ###
###############################################################################
FROM devkitpro/devkitarm as gba-build
RUN useradd builder -m
RUN mkdir -p /home/builder/pokemon_gen3_to_genx
RUN mkdir -p /home/builder/building
RUN chown -R builder:builder /home/builder
COPY --from=agbabi /usr/local/ /opt/agbabi/
ENV LIBAGBABI=/opt/agbabi
WORKDIR /home/builder/building
USER builder
CMD /home/builder/pokemon_gen3_to_genx/docker-scripts/docker_build_gba
