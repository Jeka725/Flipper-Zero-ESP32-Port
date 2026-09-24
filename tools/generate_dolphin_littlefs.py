#!/usr/bin/env python3
import re
import sys
from pathlib import Path

SRC = Path(sys.argv[1])
OUT = Path(sys.argv[2])
text = SRC.read_text(encoding="utf-8")
OUT.mkdir(parents=True, exist_ok=True)

for i in range(8):
    name = f"_A_L1_Tv_128x47_{i}"
    m = re.search(rf"const uint8_t {re.escape(name)}\[\] = \{{(.*?)\}};", text, re.S)
    if not m:
        raise SystemExit(f"missing {name}")
    values = [int(x, 0) for x in re.findall(r"0x[0-9a-fA-F]+|\b\d+\b", m.group(1))]
    (OUT / f"frame_{i}.bm").write_bytes(bytes(values))

print(f"Generated {len(list(OUT.glob('frame_*.bm')))} Dolphin frames in {OUT}")
