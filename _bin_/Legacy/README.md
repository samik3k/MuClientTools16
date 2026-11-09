# MuLegacy — reader for old MU Online clients (0.68 – 1.04, Season 0 – 4)

The Season 16 tools in this repo expect Season 16 data. Old clients use other table layouts, unencrypted or
differently encrypted terrain, and a few odd files, so those tools reject them or silently write garbage.
MuLegacy reads every data file found in the 0.68b, 0.69, 0.75, 0.85, 0.93d, 0.97, 0.99, 1.00 – 1.04 and GMO 1.03.16 clients.

## Usage

```
MuLegacy [-s <schema>] <file or folder> [output folder]
MuLegacy pack [-s <schema>] <table.txt> [output file]
```

- Unpack a whole client: `MuLegacy "Client 0.97.04"` writes `Client 0.97.04_out/` with the same folder structure,
  plus `MuLegacy.log` (one line per file) and a summary.
- `pack` rebuilds a data table from its `.txt`. Unchanged text gives back the original file byte for byte,
  including CRCs, padding and garbage bytes the original tools left behind.
- The schema is `LegacyFormats/` next to the exe (all `*.txt` files in it). Edit it to rename fields or add a layout;
  no rebuild needed.

## What it reads

| Files | Output |
|---|---|
| Data tables (`Item.bmd`, `Skill.bmd`, `Text.bmd`, `Gate.bmd`, `Quest.bmd`, … — see below) | `<name>.txt`: tab-separated, one row per record, first line holds what `pack` needs |
| `*.OZJ` `*.OZT` `*.OZB` `*.OZP` `*.OZD` `*.OZG` | `.jpg` `.tga` `.bmp` `.png` `.dds` `.gfx`, picked from the real content |
| BMD models (v0x0A plain, v0x0C encrypted) | `<name>.txt` structure (meshes + textures, actions, bones); v0x0C also as a decrypted v0x0A `.bmd` that plain-model tools (e.g. Bmd2Fbx) can open |
| `Terrain*.att` / `EncTerrain*.att` | `.tta` (decrypted) + `.att.txt` (256×256 grid of cell flags) |
| `Terrain*.map` / `EncTerrain*.map` | `.pam` (decrypted) + `.map.txt` (layer 1, layer 2, alpha grids) |
| `Terrain*.obj` / `EncTerrain*.obj` | `.jbo` (decrypted) + `.obj.txt` (type, position, angle, scale per object) |
| `Enc1.dat` / `Dec2.dat` | `.txt` with the packet-cipher keys (modulus, encryption/decryption key, xor key) |

Other files (`.wav`, `.mp3`, `.txt`, `.dll`, …) are left alone and counted in the summary.

### Data tables and versions

| Table | Layouts (file size → versions) |
|---|---|
| Item.bmd | 14080/28160 (0.68–0.85, 55 B), 28672 (0.93d CHS, 56 B), 32772 (0.97, 64 B), 38916 (0.99–1.01.02, 76 B), 655364 (1.01.46, 80 B), 688132 (1.02.44–1.04.10 five classes, 1.03.16/1.04.24 six classes — told apart by content), 69636 (DaRKav 0.97.14) |
| Skill.bmd | 2304 (0.68/0.75), 2432 (0.85), 2564 (0.97), 13316 (0.99), 15364 (1.00–1.04.10), 40804 (1.04.24), 48004 (1.03.16), 2692 (DaRKav) |
| Text.bmd | fixed 300-byte entries (180000 … 900000, 0.68–1.02.44); "GT" indexed format (1.03.16, 1.03.47, 1.04.x) |
| Gate.bmd, MoveReq.bmd | all versions (Gate 9- and 12-byte records; MoveReq keeps the unused slots) |
| Filter.bmd, FilterName.bmd, Dialog.bmd, Quest.bmd, Slide.bmd, Credit.bmd | all versions present |
| ItemSetType, ItemSetOption, ItemAddOption, JewelOfHarmonyOption, JewelOfHarmonySmelt, SocketItem | 0.99 – 1.04 |
| MasterSkillTree, MonsterSkill, BuffEffect, Pet, Mix (Chaos Machine recipes) | 1.02 – 1.04 / 1.03.16 |
| ShopCategoryItems, ShopUI (`Data/Local/partCharge1`) | 1.03.16 |
| ServerInfo.bmd | kayito's custom-main config (1.04.10 package), header fields decoded |

Field names come from the decompiled 0.68/0.69/0.85/1.00.13 mains, disassembly of the 1.03.16, 1.04.10 and
1.04.24 mains, and value checks against known items, skills, gates and recipes. Bytes whose meaning is not
proven are named `Unk<offset>`. The evidence is in `LegacyFormats/notes/`.

### Things to know about the output

- **Text cells keep the client's code page** (KOR cp949, CHS gbk, GMO cp1252, Japanese cp932 in one 1.03.16 table).
  Open the `.txt` with that encoding.
- **`\0` inside a text cell** marks leftover bytes after the string terminator. The game ignores them;
  they are kept so `pack` reproduces the file exactly. A float shown as `0x7fc00001` is a garbage value kept as raw bits.
- **Repaired images**: `.OZJ` files that hold TGA data become `.tga`; `.ozt` files that are Windows icons become `.ico`;
  TGAs with image type 0 get type 2; `TerrainHeight.OZB` files with a zeroed or junk BMP header are rebuilt from the
  grid at offset 1084, which is where the client reads it. The log says when this happens.
- **Skipped**: empty files and the 4 empty placeholder models (`Object3/Object25/27/33/90.bmd`, "BMD" + zeros).
- **Terrain headers**: the map byte often differs from the World folder number, and two files have a corrupt version
  byte. Both are WebZen data quirks; the rest of those files is valid.

## Schema syntax (`LegacyFormats/*.txt`)

```
table   <Name> <glob>[,<glob>...]            file names, case-insensitive (e.g. item.bmd, text(*).bmd)
variant <id> <=size,size,...|*> [options]    tried in order; the first whose size rule, CRC and probe match wins
field   <name> <type>                        u8 i8 u16 i16 u32 i32 f32 str<N> raw<N>; arrays: <type>*<count>
pad     <n>                                  n bytes kept as zero
```

| Option | Meaning |
|---|---|
| `rec=<n>` | record size |
| `xor=record\|stream\|none\|kayito` | FC CF AB restarting every record / running over the data / none / kayito's ServerInfo cipher |
| `crc=<hex>` | trailing u32 CRC (MuCrypto::CalculateCRC) with that wkey; `crccheck=no` keeps a trailer that is not a valid CRC; `crcscope=body` = CRC over the records only |
| `header=count32\|count32cap\|counts32:<T>` | u32 record count / count + unused slots kept as is / T xor'd counts whose sum is the record count |
| `skip=<n>` `skipxor=<p>` | n bytes before the header, kept as is (shown decoded when xor'd in p-byte blocks) |
| `layout=text-indexed` `magic=<text>` | count + (index, length, text) entries, optionally after a magic |
| `probe=<record>:<offset>:<value>` | the decoded byte must match (tells apart layouts with the same size and CRC) |
| `label="..."` | versions this layout was seen in |

## Building

- Windows: `MuLegacy` project in `_src_/ClientTools16.sln` (no dependency on Core.lib or 3rd-party libraries).
- macOS / Linux: `cmake -S _src_/Tools/Legacy -B build && cmake --build build` (copies the exe here).

## Open questions

Listed at the end of each notes file: e.g. MasterSkillTree header and the 10000×k cell flag, the five ids per
monster in MonsterSkill, Quest class-flag values 1/2, a few Unk bytes in Skill (1.03.16 / 1.04.24), Mix and Pet,
and how the client picks the alternate terrain files (`EncTerrain312`, `351/352`, `401`).
