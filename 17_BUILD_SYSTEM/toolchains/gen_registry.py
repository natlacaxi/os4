#!/usr/bin/env python3
"""Genere le registre des modules : build/registry.c (usage : gen_registry.py <racine> <sortie>)."""
import os, re, sys
root, out = sys.argv[1], sys.argv[2]
mods = []
for top in sorted(os.listdir(root)):
    if not re.match(r"(0[1-9]|1[0-5])_", top): continue
    for d, subs, files in os.walk(os.path.join(root, top)):
        if any(f.endswith("_module.c") for f in files):
            rel = os.path.relpath(d, root)
            mods.append((top, rel[len(top)+1:], re.sub(r"\W", "_", rel)))
mods.sort()
lines = ['#include "talco.h"']
lines += [f"int talco_init_{i}(void);" for _, _, i in mods]
lines.append("const talco_module_t talco_modules[] = {")
lines += [f'    {{"{t}", "{n}", talco_init_{i}}},' for t, n, i in mods]
lines += ["};", f"const int talco_module_count = {len(mods)};"]
os.makedirs(os.path.dirname(out), exist_ok=True)
open(out, "w").write("\n".join(lines) + "\n")
