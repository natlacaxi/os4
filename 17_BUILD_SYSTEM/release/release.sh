#!/bin/sh
set -e
make clean && make test && make image
sha256sum build/talco-os3.tar.gz > build/talco-os3.tar.gz.sha256
echo "Release $(cat VERSION) prete."
