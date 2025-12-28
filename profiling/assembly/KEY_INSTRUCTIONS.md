# Key Assembly Instructions Analysis

Generated from mirror_hash ARM64 assembly on M3 Max Pro MacBook.

## 1. Core Mixing Primitives

### rapidhash: 128-bit Multiply (3 instructions per 16 bytes)
```asm
mul     x17, x16, x11      ; low 64-bits of a*b (3-4 cycles)
umulh   x16, x16, x11      ; high 64-bits of a*b (3-4 cycles, can overlap)
eor     x0, x17, x16       ; XOR high and low (1 cycle)
```
**Total: ~4-5 cycles for 16 bytes = 0.25-0.31 cycles/byte**

### mirror_hash: AES Round (2 instructions per 16 bytes)
```asm
aese.16b    v0, v1         ; AES single round (SubBytes+ShiftRows+AddRoundKey)
aesmc.16b   v0, v0         ; AES MixColumns
```
**Total: ~2 cycles for 16 bytes = 0.125 cycles/byte** (fused on Apple Silicon)

---

## 2. 4-Way Parallel AES (64-128 bytes)

From lines 121-128 of mirror_hash_arm64.s:
```asm
aese.16b    v16, v7        ; Lane 0: mix with key
aesmc.16b   v16, v16
aese.16b    v17, v6        ; Lane 1: independent
aesmc.16b   v17, v17
aese.16b    v18, v5        ; Lane 2: independent
aesmc.16b   v18, v18
aese.16b    v19, v0        ; Lane 3: independent
aesmc.16b   v19, v19
```
**4 lanes × 16 bytes = 64 bytes processed in parallel**
**Hides latency: while one lane waits, others execute**

---

## 3. 8-Way Sequential Compression (129+ bytes)

From lines 291-308:
```asm
aese.16b    v3, v4         ; Block 0
aesmc.16b   v3, v3
aese.16b    v3, v5         ; Block 1 - dependent on previous
aesmc.16b   v3, v3
aese.16b    v3, v6         ; Block 2
aesmc.16b   v3, v3
aese.16b    v3, v7         ; Block 3
aesmc.16b   v3, v3
aese.16b    v3, v16        ; Block 4
aesmc.16b   v3, v3
aese.16b    v3, v17        ; Block 5
aesmc.16b   v3, v3
aese.16b    v3, v18        ; Block 6
aesmc.16b   v3, v3
aese.16b    v3, v0         ; Block 7
aesmc.16b   v3, v3
aese.16b    v3, v1         ; Final mix
aesmc.16b   v3, v3
```
**8 blocks × 16 bytes = 128 bytes compressed to one state**
**18 instructions for 128 bytes = 0.14 instructions/byte**

---

## 4. rapidhash Multiply Chain

From lines 473-485:
```asm
; First multiply: mix data with secret
mul     x17, x16, x11
umulh   x16, x16, x11

; Second multiply: further mixing
mul     x16, x15, x13
umulh   x13, x15, x13

; Third multiply: combine results
mul     x16, x13, x15
umulh   x13, x13, x15
```
**Each multiply pair: ~4-5 cycles**
**3 multiply pairs = ~12-15 cycles**

---

## Instruction Count Comparison (512 bytes)

| Algorithm | Instructions | Cycles (approx) | Cycles/Byte |
|-----------|-------------|-----------------|-------------|
| rapidhash | ~96 mul/umulh pairs | ~384-480 | 0.75-0.94 |
| mirror_hash AES | ~64 aese+aesmc pairs | ~128 | 0.25 |

**Winner: AES is 3-4x more efficient in instruction count**

