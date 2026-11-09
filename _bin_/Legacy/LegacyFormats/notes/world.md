# Group "world" — Gate.bmd, MoveReq.bmd, Enc1/Dec2.dat, terrain files

> Research notes behind `../world.txt`: evidence for each field (decompiled loaders, value checks), byte-exact
> layouts and tested Python decoders. They were written against a reference decoder with a smaller schema syntax;
> where they call a format OPAQUE, "container" or FALLBACK, or propose an option (`header=count32cap`, `skip=`,
> `counts32`, `trailer=`, `crc_from=`, `probe=`, a magic before the count), MuLegacy now supports it and the
> schema file decodes that format exactly. Paths to the original working folders have been removed.

Schema: `schema/world.txt`. All decoders below were run over the whole corpus (results quoted).

---------------------------------------------------------------------------------------------------
## 1. Gate.bmd

Fixed array, no header, no CRC, FC CF AB restarted per record.

| variant | size | gates × rec | versions |
|---|---|---|---|
| g512x12 | 6144 | 512 × 12 | 1.03.16 GMO, 1.04.24 |
| g256x9  | 2304 | 256 × 9  | 1.00.15, 1.01.02, 1.01.46, 1.02.44, 1.03.47, 1.04.10 |
| g100x9  | 900  | 100 × 9  | 0.75, 0.85, 0.97.04, 0.97.14-darkav, 0.99.28 |

9-byte record: `Flag u8, Map u8, X1 u8, Y1 u8, X2 u8, Y2 u8, Target u8, Dir u8, Level u8`.
12-byte record: same first six bytes, then `Target u16, Dir u16, Level u16` (needed: 1.03.16 has
targets up to 293 and levels 270 / 400).

Evidence:
- 0.85 `sub_454F00("Data\\Gate.bmd")`: loop `fread(9)` + BuxConvert(9) until 900 bytes; the text
  loader `sub_454A40(Data2\\Gate.txt)` reads 9 numbers per line into `rec[0..8]` at `index*9`
  (writer `sub_454E80` re-encodes the same 9 bytes). 1.00.13 CHS `sub_52B250`: same with 2304.
- Field use, 0.85 warp check (~line 108960): `if rec[0]==1 && rec[1]==currentMap && x>=rec[2] &&
  y>=rec[3] && x<=rec[4] && y<=rec[5]` → warp; required level = `rec[8]`, or `2*rec[8]/3` when
  class == 3 (Magic Gladiator) — the well-known MG level reduction.
- Data: entrance gates (Flag 1) always target a Flag 0 or 2 gate (0 violations in 0.75, 1.04.10,
  1.03.16). Known gates check out: gate 1 Lorencia (map 0, 121..123,232..233) → gate 2 Dungeon
  (map 1, 107..110,247), level 20 (1.04.x; 0.75 had 20 too); 17 = Lorencia town (Flag 0, map 0,
  133..151 × 118..135); 22 = Devias town (map 2), 27 = Noria (map 3), 42 = Lost Tower (map 4,
  Flag 0, level 40/80); 49 = Atlans (map 7), 50–52 = Arena (map 6), 57 = Tarkan (map 8);
  1.03.16 267 = Elbeland (map 51), 287 = Raklion (map 57), 294 = Vulcanus (map 63) — exactly the
  gates MoveReq points at.
- `Dir`: only values 0,1,2,3,5,7, set on Flag 0/2 gates → facing direction (0..7) after arrival.
  The name comes from the server Gate.txt convention; the client loaders above don't read it.
- Note: 1.04.10 MoveReq references gates 259 and 262, but its Gate.bmd only has 256 entries
  (client just forwards the move index to the server).

---------------------------------------------------------------------------------------------------
## 2. MoveReq.bmd (`Data/movereq.bmd` in 0.75, `Data/Local/MoveReq.bmd` later)

Layout: `u32 Count` (plain), then `Count` × 80-byte records, FC CF AB restarted per record:

| off | type | field |
|---|---|---|
| 0  | u32   | Index (1-based /move id; 1.03.16 non-sequential: 31,32,32,4,…) |
| 4  | str32 | Name1 — Latin keyword (always ASCII; in the CHS darkav file Name2 is the Chinese name) |
| 36 | str32 | Name2 — localized display name |
| 68 | i32   | Level (min level; 400 for Swamp of Calmness) |
| 72 | i32   | Zen |
| 76 | i32   | Gate (Gate.bmd index of the landing gate — verified, see §1) |

Files are dumped from a fixed buffer, so they are larger than `4 + 80*Count`:

| size | slots | Count | versions |
|---|---|---|---|
| 2724 | 34 | 34 | 1.04.24 (exact → schema variant `exact`) |
| 2884 | 36 | 23/12/23/24/24/27/28/28/31 | 0.75, 0.97.14-darkav, 0.99.28, 1.00.15, 1.01.02, 1.01.46, 1.02.44, 1.03.47, 1.04.10 |
| 6404 | 80 | 35 | 1.03.16 GMO |

Evidence: 1.00.13 CHS `sub_4380C0`: `fread(&count,4)`, then `count` times `fread(80)` +
`sub_4393F0(rec,80)` (BuxConvert) into a list; nothing after that is read. The slack slots are
garbage (0.75: slots 7–22 have index 11..23 / names "0"; Count 23 counts them) — treat
`Count` as authoritative like the client.

Schema limitation: `header=count32` demands `count*rec == body`, so the padded files only get a
`*_FALLBACK` variant (Count + raw blob). Proposed option for the C++ tool: `header=count32cap`
(accept `count*rec <= body`, ignore the rest). Tested decoder (all 11 files):

```python
def decode_movereq(d, cp='cp949'):
    cnt = struct.unpack_from('<I', d)[0]; out = []
    for k in range(cnt):
        r = bytes(c ^ b'\xfc\xcf\xab'[j % 3] for j, c in enumerate(d[4+80*k:84+80*k]))
        idx, = struct.unpack_from('<I', r)
        n1 = r[4:36].split(b'\0')[0].decode(cp, 'replace'); n2 = r[36:68].split(b'\0')[0].decode(cp, 'replace')
        lvl, zen, gate = struct.unpack_from('<iii', r, 68)
        out.append((idx, n1, n2, lvl, zen, gate))
    return out
# 1.04.24 -> 34 rows (1,'Arena','Arena',50,2000,50) .. (33,'SwampofCalmness','SwampOfCalmness',400,15000,273)
# 1.03.16 -> 35 rows, cp1252; darkav -> 12 rows, Name2 in gbk
```

0.75's main.exe has no "movereq" string (and its Enc*/Terrain7/8 files are also unused, see §5):
the 0.75 movereq.bmd is a leftover from the private-server package.

---------------------------------------------------------------------------------------------------
## 3. Enc1.dat / Dec2.dat (SimpleModulus packet-cipher keys)

Loader 0.85 `sub_52E9F0(this, file, 0x1112, mod, enc, dec, xor)` (called as
`Enc1.dat: (1,1,0,1)`, `Dec2.dat: (1,0,1,1)`):

```
u16 Magic   = 0x1112          ; must match
u32 Size    = 16*(mod+enc+dec+xor)*W/4 + 6   ; must equal file size (0.85+: 54, 0.75: 198)
[if mod] u32 Modulus[W]
[if enc] u32 EncryptionKey[W]
[if dec] u32 DecryptionKey[W]
[if xor] u32 XorKey[W]
each u32[i] ^= FIXKEY[i]
```
W = 4 for 0.85 … 1.04.24 (54 B), W = 16 for 0.75 (198 B; the 0.75 main at 0x46A8A2 checks
`Size == (sum of flags << 6) + 6` and reads 0x40 per array).
`FIXKEY` = dword table at 0x594220 (0.85) / 0x483D80 (0.75), little-endian:
`3F08A79B E25CC287 93D27AB9 20DEA7BF` (0.85+); 0.75 continues with
`837A9BC7 1FFEA89B E836579C D89A0924 9D8A73CF 4DEA98CE 4DC9EF0F 2BAC890A F32AFE54 D8902A1E 3FDEF8F9 5A827990`.
No FC CF AB, no CRC.

So Enc1 = Modulus, EncryptionKey, XorKey; Dec2 = Modulus, DecryptionKey, XorKey.
Distinct files: 0.75 (own pair), a common pair shared by 0.85–1.03.47 / 1.04.24 (Enc1 md5 e42a…,
Dec2 cb34…), and a different pair in 1.01.46 and 1.04.10 (kayito re-keyed: Enc1 and Dec2 share
modulus 72619/72691/73136/78576 and `enc*dec mod m == 1` for all four lanes — confirms the
modulus/key order). In every file key < modulus < 2^18.

```python
SMKEY = [0x3F08A79B,0xE25CC287,0x93D27AB9,0x20DEA7BF,0x837A9BC7,0x1FFEA89B,0xE836579C,0xD89A0924,
         0x9D8A73CF,0x4DEA98CE,0x4DC9EF0F,0x2BAC890A,0xF32AFE54,0xD8902A1E,0x3FDEF8F9,0x5A827990]
def decode_simple_modulus(d):
    magic, size = struct.unpack_from('<HI', d)
    if magic != 0x1112 or size != len(d) or (size - 6) % 48: raise ValueError('not a key file')
    n = (size - 6) // 12                       # u32 per array
    v = struct.unpack_from('<%dI' % (3*n), d, 6)
    a = [[v[k*n+i] ^ SMKEY[i] for i in range(n)] for k in range(3)]
    return {'modulus': a[0], 'key': a[1], 'xor_key': a[2]}
# 0.85 Enc1: modulus [128079,164742,70235,106898] key [23489,11911,19816,13647] xor [48413,46165,15171,37433]
# 0.85 Dec2: modulus [73326,109989,98843,171058] key [18035,30340,24701,11141] xor [62004,64409,35374,64599]
```
(The schema variants k4/k16 show the stored, still-XORed u32s.)

---------------------------------------------------------------------------------------------------
## 4. Terrain files — formats

Grid: always 256×256, row-major, index = y*256 + x. Crypto primitives:
- **xor3** = BuxConvert FC CF AB, key index = *file offset* % 3 (header included, one stream).
- **MapFileDecrypt**: `out[i] = (in[i] ^ K[i%16]) - seed; seed = in[i] + 0x3D` (seed from the
  *ciphertext* byte, initial 0x5E), K = `D1 73 52 F6 D2 9A CB 27 3E AF 59 31 37 B3 E7 A2`.
  Confirmed in 1.00.13 CHS `sub_5CD600` (att), `sub_5CDB40` (map), `sub_5DB100` (obj): identical
  loop with the 16 key bytes on the stack and `v=94` / `v=*src+61`. Because the seed is the
  previous ciphertext byte, a corrupt byte only damages itself.
- No `ATT\1` / `MAP\1` (ModulusDecrypt) files exist anywhere in the corpus (those are S6+).

### ATT (walk attributes)

| variant | size | crypto | header | cell |
|---|---|---|---|---|
| att-plain  | 65539  | none | `u8 ver=0, u8 w=255, u8 h=255` | u8 |
| att-plain (2-byte) | 131075 | none | same 3 bytes | u16 — only tool dumps (darkav World31/32/39 `terrain.att`, 1.02.44/1.03.47 World38/39/40/55/56 `Terrain.att`) |
| att-xor    | 65539  | xor3 over whole file | `ver, w, h` | u8 |
| att-enc    | 65540  | MapFileDecrypt, then xor3 over whole file | `u8 ver=0, u8 map, u8 w=255, u8 h=255` | u8 |
| att-enc (2-byte) | 131076 | same | same 4 bytes | u16 LE |

Client checks: 0.85 `sub_4E7F10` requires size == 65539, ver 0, w/h 255, every cell < 0x80,
plus anti-hack probes on maps 0–4 (cell at a fixed offset must be 5/4). 1.00.13 `sub_5CD600`
accepts 65540 or 131076; for 131076 it copies u16 cells, then (as decompiled) keeps only the
low byte and rejects any low byte ≥ 0x80; returns header byte[1] (map) which the caller
compares with the world number. Cell bits observed: 1-byte files use the full 0..0xFF range only
in `TerrainServer.att` (server copy); client files use ≤ 0x5F; in 2-byte files the high byte is
0 everywhere except bit 0x100 in Valley of Loren (World31 EncTerrain31/312, 348 cells).

### MAP (tile layers)

| variant | size | crypto | layout |
|---|---|---|---|
| map-plain  | 196609 | none | `u8 ver` + layer1[65536] + layer2[65536] + alpha[65536] |
| map-enc    | 196610 | MapFileDecrypt (no xor3) | `u8 ver, u8 map` + layer1 + layer2 + alpha |
| map-plain4 | 196610 | none (decrypted dump; raw byte0 = 0x00, an encrypted ver 0 is always 0x8F) | as map-enc |

layer1/layer2 = tile texture index (0..~13 for real maps; 255 = none in layer2), alpha byte → the
client stores `byte/255.0` (0.85 `sub_4E80B0` copies `+1`, `+65537`, alpha at `+131073`;
1.00.13 `sub_5CDB40` copies `+2`, `+65538`, alpha at `+131074`, returns byte[1]).

### OBJ (static objects)

| variant | header | records |
|---|---|---|
| obj-plain | `u8 ver=0, i16 count` (3 B) | count × 30 B |
| obj-enc (MapFileDecrypt whole file) | `u8 ver=0, u8 map, i16 count` (4 B) | count × 30 B |

Record (30 B, both variants): `i16 Type, f32 PosX, PosY, PosZ, f32 AngleX, AngleY, AngleZ, f32 Scale`
(0.85 `sub_4EEE00`: count = `*(i16*)(buf+1)`, step 30, `type=*(u16*)p, pos=p+2..+13,
angle=p+14..+25, scale=p+26` → `CreateObject(type,pos,angle,scale)`; 1.00.13 `sub_5DB100`
identical with count at +2, records at +4). Size check: every corpus file satisfies
`size == hdr + 30*count`. Positions are world units (0..25600 = 256 cells × 100), angles in
degrees (0/90/180/270 typical), Z ≈ terrain height, scale ≈ 0.2..4.

Genuine junk records exist in WebZen data and must not be treated as errors: World7 Atlans
record 18 (type −515, denormal floats) in every version from 0.75 to 1.04.24; Noria World3 2
records; 1.04.10 World52 two NaN Y; `EncTerrain12.obj` (0.99.28–1.04.10) record 914.

### Header quirks (do not validate these strictly)
- `ver` byte ≠ 0 after decryption: `EncTerrain12.obj` 0.99.28–1.04.10 (raw `79`, ver 0x4A) and
  darkav `World7/EncTerrain7.map` (raw `4A`, ver 0x3D); the rest of both files decodes correctly.
- `map` byte ≠ folder number: ATT World6/7/11/12 hold N−1 (5/6/10/11) in 0.99.28–1.04.10
  (fixed in 1.03.16/1.04.24); MAP/OBJ World6 hold 32 (copy of Land of Trials); 1.04.10
  World36 → 41, World52 → 80, World53 → 65; darkav copies (encterrain9/13 inside World31/32, EncTerrain19.att map 1).
  The map byte is informational for a converter.

### TerrainHeight.OZB
OZB = 4 prefix bytes + BMP. The client ignores the BMP header: classic loader (0.85
`sub_4E83B0`, 1.00.13 `sub_5CE3B0`, 1.03.16 at 0x5A8ACC) does `fseek(4)`, `fread(0x10438)`,
heights = `buf[0x438 + y*256 + x] * 1.5` → **grid at file offset 1084**, 8-bit
(1.03.16: factor 3.0 when map == 55). Extension string "OZB" is `off_58D2DC`/`off_67194C`
(= 0x00425A4F).

| size | header | versions / note |
|---|---|---|
| 66620 | `BM…` copy + 8-bit BMP, bfOffBits 1078, bfSize 66616 | most maps, all versions. The BMP's own pixels start at 1082, so the client's grid is the BMP image shifted by 2 pixels (2 trailing zero bytes fill the end) |
| 66620 | first 1084 bytes zero | World19 (0.99.28+) |
| 66618 | first 1082 bytes zero ("zeroed header", 2 bytes short) | World3/6/7/12 0.97.04+ ; checked vs the 66620 Noria file: matches at offset 1084 (63357/65536 equal), not at 1082 (13093) → grid still at 1084, last 2 cells missing (client reads stale memory; use 0) |
| 66620 | header claims 24-bit / bfOffBits 54 or 534 | 0.97.04 World11, 1.03.16/1.04.24 World42 — header junk, client still reads 1084 |
| 196666 | 4 + 24-bit BMP (54-byte header) | 1.03.16 / 1.04.24 World43 (map 42): S4 loader 0x5A8F4C (chosen when map == 42 or another predicate) reads cell i at `58 + 3*i`, height = `(R | G<<8 | B<<16) − 500.0` (bytes stored B,G,R) |

### TerrainLight.OZJ (also TerrainLight1/2)
24-byte prefix + JPEG (256×256 light map). The prefix is usually a copy of the JPEG's first 24
bytes; in two maps per version (0.97.04+) it starts `FF 4B 6D 4B` (junk) — always skip 24 bytes.

---------------------------------------------------------------------------------------------------
## 5. Version → variant

What the main.exe asks for (strings / decompile) and what the corpus holds:

| version | ATT | MAP | OBJ | client file names |
|---|---|---|---|---|
| 0.68.02, 0.69.00 (patches) | att-plain 65539 | – | – | `Terrain.att` (only the ATT is in the patches) |
| 0.75 | att-plain 65539 (13) | map-plain (13) | obj-plain (14) | main: `Data\%s\Terrain.att/.map`, `terrain.obj` (no Enc strings). Extra unused files: World7/8 `Terrain7.att`/`Terrain8.att` (att-xor), `EncTerrain7/8.*` (att-enc, map-enc, obj-enc), World7 `Terrain1.obj` |
| 0.85 | att-plain (8) | map-plain (8) | obj-plain (9) | decompile `sub_4E7F10/4E80B0/4EEE00`; World9 `Terrain1.obj` extra |
| 0.97.04 | att-xor (11, `Terrain%d.att`) | map-plain 196609 (`Terrain.map` 8 + `Terrain%d.map` 2) | obj-plain (`terrain.obj` 9 + `terrain%d.obj` 2) | main strings: `Terrain%d.att`, `Terrain%d.map`, `Terrain.map`, `terrain%d.obj`, `terrain.obj` |
| 0.97.14-darkav (mix) | att-enc 65540/131076, att-plain 65539/131075, att-xor | map-enc, map-plain, 1 map-plain4, 1 map-enc(bad-ver-byte) | obj-enc, obj-plain | private-server mix of every generation, plus `EncTerrain19.att a`, `EncTerrain7.attbak` (both att-enc 65540), `terrainserver.att` (att-plain, server file), lowercase `encterrain*` copies |
| 0.99.28 | att-enc 65540 (14) | map-enc (14) | obj-enc (14) | `EncTerrain%d.*` |
| 1.00.15, 1.01.02 | att-enc 65540 (14) + 131076 (World31, World31 `EncTerrain312`, World32) | map-enc | obj-enc | 1.00.13 decompile: `EncTerrain%d.att/.map/.obj`; the World31 ATT name is chosen by a separate branch for map 30 (`sub_4225E0`) — `EncTerrain312.att` is presumably that alternate (open question) |
| 1.01.46 | att-enc 65540/131076 (+World34/35) | map-enc | obj-enc | + plain `Terrain34.att`, `Terrain35.att` (att-plain 65539, unused copies) |
| 1.02.44, 1.03.47 | att-enc 65540 (14) / 131076 (11, incl. `EncTerrain401.att`) | map-enc (24) + map-plain (5) | obj-enc + obj-plain (5) | plain `Terrain.att` (131075) / `.map` / `.obj` in World38/39/40/55/56 and `Terrain34/35.att` are decrypted copies; `EncTerrain401.*` = alternate Crywolf (World40) set, all three files valid |
| 1.04.10 | att-enc 65540 (17) / 131076 (15) | map-enc (31) | obj-enc | + `Terrain34/35.att` plain |
| 1.03.16 GMO, 1.04.24 | att-enc 131076 (30) / 65540 (8) | map-enc (35) | obj-enc | + `EncTerrainTest3.*` (valid, Noria test), `EncTerrain351/352.att` (World35, att-enc 131076), `World79/TerrainServer.att` (att-plain 65539, server copy) |

Corpus sweep with the decoder below (924 terrain files): att-enc 252, att-plain 59,
att-xor 24, map-enc 234, map-enc(bad-ver-byte) 1, map-plain 56, map-plain4 1, obj-enc 235,
obj-plain 62 — 0 failures. Height/light (not in the corpus; pulled from the client archives
0.75, 0.85, 0.97.04, 0.99.28, 1.00.15, 1.02.44, 1.03.16, 1.04.24): 151 OZB, all decode.

Detection rule for a converter (by size, then content):
- ATT 65539/131075: if bytes[0..2] == `00 FF FF` → plain, elif xor3 gives `00 FF FF` → att-xor.
- ATT 65540/131076: MapFileDecrypt + xor3 → `00 ?? FF FF` (raw starts `8B`).
- MAP 196609 → plain; 196610 → raw[0]==0x00 ? plain4 : map-enc (raw[0] is 0x8F for ver 0).
- OBJ: raw[0]==0 and `3+30*u16(raw+1)==size` → plain; else MapFileDecrypt and require
  `4+30*u16(dec+2)==size`.

---------------------------------------------------------------------------------------------------
## 6. Tested decoder (terrain + keys + movereq)

```python
import struct
MAPKEY = bytes([0xD1,0x73,0x52,0xF6,0xD2,0x9A,0xCB,0x27,0x3E,0xAF,0x59,0x31,0x37,0xB3,0xE7,0xA2])
BUX = b'\xfc\xcf\xab'

def map_decrypt(b):
    out = bytearray(len(b)); seed = 0x5E
    for i, c in enumerate(b):
        out[i] = ((c ^ MAPKEY[i & 15]) - seed) & 0xFF
        seed = (c + 0x3D) & 0xFF
    return bytes(out)

def xor3(b):
    return bytes(c ^ BUX[i % 3] for i, c in enumerate(b))

def _cells(b, cs):
    return list(b) if cs == 1 else list(struct.unpack('<65536H', b))

def decode_att(d):
    n = len(d)
    if n in (65539, 131075):
        for name, b in (('att-plain', d), ('att-xor', xor3(d))):
            if b[0] == 0 and b[1] == 255 and b[2] == 255:
                cs = (n - 3) // 65536
                return name, {'ver': b[0], 'w': b[1], 'h': b[2]}, _cells(b[3:], cs), cs
    if n in (65540, 131076):
        b = xor3(map_decrypt(d))
        if b[0] == 0 and b[2] == 255 and b[3] == 255:
            cs = (n - 4) // 65536
            return 'att-enc', {'ver': b[0], 'map': b[1], 'w': b[2], 'h': b[3]}, _cells(b[4:], cs), cs
    raise ValueError('unknown ATT variant (%d bytes)' % n)

def decode_map(d):
    n = len(d)
    if n == 196609:
        return 'map-plain', {'ver': d[0]}, d[1:65537], d[65537:131073], d[131073:]
    if n == 196610:
        if d[0] == 0x00:
            b, v = d, 'map-plain4'
        else:
            b, v = map_decrypt(d), 'map-enc'
            if b[0] != 0: v = 'map-enc(bad-ver-byte)'
        return v, {'ver': b[0], 'map': b[1]}, b[2:65538], b[65538:131074], b[131074:]
    raise ValueError('unknown MAP variant (%d bytes)' % n)

OBJREC = struct.Struct('<h3f3ff')   # Type, Pos xyz, Angle xyz, Scale = 30 bytes
def decode_obj(d):
    n = len(d)
    if d[0] == 0 and 3 + 30 * struct.unpack_from('<H', d, 1)[0] == n:
        b, v, p = d, 'obj-plain', 3
        hdr = {'ver': b[0], 'count': struct.unpack_from('<H', b, 1)[0]}
    else:
        b = map_decrypt(d); cnt = struct.unpack_from('<H', b, 2)[0]
        if 4 + 30 * cnt != n: raise ValueError('unknown OBJ variant (%d bytes)' % n)
        v, p = 'obj-enc', 4
        hdr = {'ver': b[0], 'map': b[1], 'count': cnt}
    return v, hdr, [OBJREC.unpack_from(b, p + 30 * i) for i in range(hdr['count'])]

def decode_height(d, new24=False):          # new24: 196666-byte S4 files
    if new24:
        return [(d[58+3*i+2] | d[58+3*i+1] << 8 | d[58+3*i] << 16) - 500.0 for i in range(65536)]
    g = d[1084:1084 + 65536]; g = g + bytes(65536 - len(g))
    return [x * 1.5 for x in g]

def ozj_to_jpeg(d):
    return d[24:]
```
(`decode_simple_modulus` and `decode_movereq` are in §3 and §2.)

---------------------------------------------------------------------------------------------------
## 7. Open questions
- 1.00.13 CHS compares the ATT/MAP/OBJ header map byte with the world number and shows
  "file corrupt" on mismatch, yet 0.99.28–1.04.10 KOR ship ATT files whose map byte is N−1
  (World6/7/11/12). Either the KOR mains don't check ATT or those worlds load under another
  number; not checked in KOR disassembly.
- Which client path loads `EncTerrain312.att` (World31) and `EncTerrain351/352.att`,
  `EncTerrain401.*` (alternate state sets: castle siege / Crywolf?) — the 1.00.13 decompile
  collapses the format arguments of the map-30 branch.
- 1.00.13 truncates u16 ATT cells to their low byte as decompiled; the bit 0x100 used in
  World31 would be lost there — possibly a decompiler artefact; later mains not checked.
- ATT attribute bit meanings are not named here (no loader-side evidence gathered).
- 0.68/0.69 decompilations carry no string names, so their terrain loaders were not traced; the
  patch `Terrain.att` files are att-plain like 0.75.
