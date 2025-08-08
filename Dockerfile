FROM ubuntu:22.04
COPY . /workspace/bustub-unc
VOLUME /workspace/bustub-unc
RUN echo 'y' | /workspace/bustub-unc/build_support/packages.sh
CMD ["sleep","infinity"]
