#! /bin/bash

pushd $(git rev-parse --show-toplevel) > /dev/null
docker volume create bustub-volume && docker build . -t bustub:unc -f Dockerfile
popd > /dev/null
