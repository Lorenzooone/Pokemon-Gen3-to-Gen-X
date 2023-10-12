#!/bin/bash
docker image rm lorenzooone/pokemon_gen3_to_genx:nds_builder
docker build --target ds-build . -t lorenzooone/pokemon_gen3_to_genx:nds_builder
