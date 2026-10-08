"""Tests for the cipher model, including a bit-exact check against the
original decrypt() in firmware/base_station/Base_Station.c (compiled on the host)."""
import random
import re
import shutil
import subprocess
from pathlib import Path

import pytest

from wsn_cipher import decrypt, encrypt

ROOT = Path(__file__).resolve().parents[2]
BASE_STATION = ROOT / "firmware" / "base_station" / "Base_Station.c"
KEYS = (0x01234567, 0x89ABCDEF, 0x0F1E2D3C, 0x4B5A6978)


def test_roundtrip():
    rng = random.Random(1)
    for _ in range(500):
        keys = tuple(rng.getrandbits(32) for _ in range(4))
        block = (rng.getrandbits(32), rng.getrandbits(32))
        assert decrypt(*encrypt(*block, keys), keys) == block


def test_ciphertext_differs_and_depends_on_key():
    block = (0x2A7, 0x3)
    c1 = encrypt(*block, KEYS)
    c2 = encrypt(*block, (KEYS[0] ^ 1,) + KEYS[1:])
    assert c1 != block and c1 != c2


@pytest.mark.skipif(shutil.which("gcc") is None, reason="gcc not available")
def test_matches_original_firmware_decrypt(tmp_path):
    """Extract decrypt() verbatim from the base-station firmware, compile it
    with a small host harness and compare with the Python model."""
    src = BASE_STATION.read_text()
    body = re.search(r"void decrypt\(\)\s*\{.*?\n\}\n", src, re.S).group(0)
    body = body.replace("printf(", "(void)(")  # silence the firmware's debug prints
    harness = f"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t k1,k2,k3,k4, si = 0, delta = 0x9E3779B9, fun = 0, r1i = 0, ri, li;
{body}
int main(int argc, char **argv) {{
    k1 = strtoul(argv[1],0,16); k2 = strtoul(argv[2],0,16);
    k3 = strtoul(argv[3],0,16); k4 = strtoul(argv[4],0,16);
    li = strtoul(argv[5],0,16); ri = strtoul(argv[6],0,16);
    decrypt();
    printf("%08x %08x\\n", li, ri);
    return 0;
}}
"""
    c_file = tmp_path / "harness.c"
    c_file.write_text(harness)
    exe = tmp_path / "harness"
    subprocess.run(["gcc", "-w", "-O1", str(c_file), "-o", str(exe)], check=True)

    rng = random.Random(7)
    for _ in range(50):
        keys = tuple(rng.getrandbits(32) for _ in range(4))
        plain = (rng.getrandbits(32), rng.getrandbits(32))
        cipher = encrypt(*plain, keys)
        out = subprocess.run([str(exe), *(f"{k:x}" for k in keys), f"{cipher[0]:x}", f"{cipher[1]:x}"],
                             capture_output=True, text=True, check=True).stdout.split()
        assert (int(out[0], 16), int(out[1], 16)) == plain
