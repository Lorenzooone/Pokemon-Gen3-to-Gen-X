#!/bin/bash
docker image rm lorenzooone/pokemon_gen3_to_genx:gba_builder
docker build --target gba-build . -t lorenzooone/pokemon_gen3_to_genx:gba_builder
