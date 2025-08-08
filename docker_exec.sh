#! /bin/bash
if [[ -z $(docker ps | grep bustub:unc) ]]
then
    CONTAINER_ID=$(docker run -d -v bustub-volume:/workspace/bustub-unc bustub:unc)
fi
docker exec -it -w /workspace/bustub-unc "${CONTAINER_ID}" /bin/bash
