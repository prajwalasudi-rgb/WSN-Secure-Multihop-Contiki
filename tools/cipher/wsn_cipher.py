"""Reference model of the block cipher used between sensor nodes and the base station.

The firmware uses a 64-bit Feistel cipher in the style of TEA (Tiny Encryption
Algorithm): 32 rounds, the TEA constant delta = 0x9E3779B9, and four 32-bit
round keys k1..k4 that come from the Diffie-Hellman key exchange.

Round i (1..32), with the block split into left (L) and right (R) halves:

    d_i      = floor((i-1)/2) * delta                    (mod 2^32)
    key idx  = d_i & 3                 if i is odd
               rotr(d_i, 11) & 3       if i is even
    F(x)     = (((x << 4) ^ rotr(x, 5)) + d_i) ^ x) + (d_i ^ k[key idx])

    encrypt: (L, R) -> (R, L ^ F(R))       for i = 1, 2, ..., 32
    decrypt: (L, R) -> (R ^ F(L), L)       for i = 32, 31, ..., 1

This mirrors encrypt() in the 2013 TinyOS sensor code and decrypt() in
firmware/base_station/Base_Station.c bit for bit (the firmware rotates with a
shift-and-xor loop; rotr() below is the same operation).
"""
from __future__ import annotations

MASK = 0xFFFFFFFF
DELTA = 0x9E3779B9
ROUNDS = 32


def rotr(x: int, n: int) -> int:
    return ((x >> n) | (x << (32 - n))) & MASK


def _round_values(i: int, keys: tuple[int, int, int, int]) -> tuple[int, int]:
    d = (((i - 1) // 2) * DELTA) & MASK
    idx = (d & 3) if i % 2 == 1 else (rotr(d, 11) & 3)
    return d, keys[idx]


def _f(x: int, d: int, k: int) -> int:
    r1 = ((x << 4) & MASK) ^ rotr(x, 5)
    return ((((r1 + d) & MASK) ^ x) + (d ^ k)) & MASK


def encrypt(left: int, right: int, keys: tuple[int, int, int, int]) -> tuple[int, int]:
    for i in range(1, ROUNDS + 1):
        d, k = _round_values(i, keys)
        left, right = right, left ^ _f(right, d, k)
    return left, right


def decrypt(left: int, right: int, keys: tuple[int, int, int, int]) -> tuple[int, int]:
    for i in range(ROUNDS, 0, -1):
        d, k = _round_values(i, keys)
        left, right = right ^ _f(left, d, k), left
    return left, right


if __name__ == "__main__":
    keys = (0x01234567, 0x89ABCDEF, 0x0F1E2D3C, 0x4B5A6978)
    plain = (0x000002A7, 0x00000003)          # e.g. sensor reading, node id
    cipher = encrypt(*plain, keys)
    print(f"plain  {plain[0]:08x} {plain[1]:08x}")
    print(f"cipher {cipher[0]:08x} {cipher[1]:08x}")
    print(f"back   {decrypt(*cipher, keys)[0]:08x} {decrypt(*cipher, keys)[1]:08x}")
