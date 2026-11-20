#!/bin/bash
set -euo pipefail; mkdir -p out
sudo apt-get update -qq && sudo apt-get install -y -qq python3-pil
make; make test; make ui; make image
python3 -c "from PIL import Image; [Image.open(f'build/{n}.ppm').save(f'out/{n}.png') for n in ('phone','tablet')]"
cp build/talco-os3.tar.gz out/
