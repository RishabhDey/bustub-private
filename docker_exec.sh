#! /bin/bash
if [[ -z $(docker images --format json bustub:unc) ]] 
then
    ./build_support/docker.sh
fi

if [[ -z $(docker ps | grep bustub:unc) ]]
then
    CONTAINER_ID=$(docker run -d -v bustub-volume:/workspace/bustub-unc bustub:unc)
else
    CONTAINER_ID=$(docker ps -f ancestor=bustub:unc --format="{{.ID}}")
fi
docker exec -it -w /workspace/bustub-unc "${CONTAINER_ID}" /bin/bash
