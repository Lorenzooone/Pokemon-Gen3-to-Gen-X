FROM devkitpro/devkitarm as agbabi

RUN apt-get install -y cmake build-essential gcc-arm-none-eabi && \
    git clone https://github.com/felixjones/agbabi

WORKDIR agbabi

RUN cmake -S . -DCMAKE_TOOLCHAIN_FILE=cross/agb.cmake -B build && \
    cmake --build build && \
    cmake --install build

FROM devkitpro/devkitarm

COPY --from=agbabi /usr/local/ /usr/local/

ENV LIBAGBABI=/usr/local

WORKDIR pokemon_gen3_to_genx

CMD make
