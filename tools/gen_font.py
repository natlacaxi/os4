#!/usr/bin/env python3
"""Genere ui/font_inter.h : atlas de glyphes anti-aliases (alpha 8 bits) a partir de la police Inter (licence SIL OFL 1.1).
Deux atlas : texte (Inter Medium, 40 px) et grands chiffres (Inter Display Light, 150 px). Usage : python3 tools/gen_font.py"""
import os
from PIL import Image, ImageDraw, ImageFont
D = "/usr/share/fonts/opentype/inter/"
def pick(*names):
    for n in names:
        if os.path.exists(D + n): return D + n
    raise SystemExit("police Inter introuvable")
TEXT = [chr(c) for c in range(32, 127)] + list("éèêëàâäçîïôöùûüÿœÉÈÀÇÊ’‘“”«»…°·•–—€×÷−±")
DIG = list("0123456789:")
def atlas(name, path, size, chars):
    font = ImageFont.truetype(path, size)
    rows, data = [], bytearray()
    for ch in chars:
        l, t, r, b = font.getbbox(ch, anchor="ls")
        w, h = max(0, r - l), max(0, b - t)
        off = len(data)
        if w and h:
            img = Image.new("L", (w, h), 0)
            ImageDraw.Draw(img).text((-l, -t), ch, font=font, fill=255, anchor="ls")
            data += img.tobytes()
        rows.append((ord(ch), off, w, h, l, t, round(font.getlength(ch) * 16)))
    g = ",\n".join("    {%d,%d,%d,%d,%d,%d,%d}" % r for r in rows)
    d = ",".join(str(x) for x in data)
    return (f"static const unsigned char {name}_data[] = {{{d}}};\n"
            f"static const fglyph_t {name}_g[] = {{\n{g}\n}};\n"
            f"static const fatlas_t {name} = {{{size}, {name}_g, {len(rows)}, {name}_data}};\n")
out = ("/* Genere par tools/gen_font.py -- police Inter, SIL Open Font License 1.1 (voir LICENSES/OFL-Inter.txt). */\n"
       "#ifndef FONT_INTER_H\n#define FONT_INTER_H\n"
       "typedef struct { unsigned short cp; unsigned int off; unsigned short w, h; short xo, yo; unsigned short adv16; } fglyph_t;\n"
       "typedef struct { int size; const fglyph_t *g; int n; const unsigned char *data; } fatlas_t;\n")
out += atlas("atlas_text", pick("Inter-Medium.otf", "Inter-Regular.otf"), 40, TEXT)
out += atlas("atlas_big", pick("InterDisplay-Light.otf", "Inter-Light.otf", "Inter-Thin.otf"), 150, DIG)
out += "#endif\n"
open("05_TALCO_EXPERIENCE/launcher/ui/font_inter.h", "w").write(out)
print("font_inter.h :", len(out) // 1024, "Ko")
