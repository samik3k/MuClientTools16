# texts group: evidence notes

> Research notes behind `../texts.txt`: evidence for each field (decompiled loaders, value checks), byte-exact
> layouts and tested Python decoders. They were written against a reference decoder with a smaller schema syntax;
> where they call a format OPAQUE, "container" or FALLBACK, or propose an option (`header=count32cap`, `skip=`,
> `counts32`, `trailer=`, `crc_from=`, `probe=`, a magic before the count), MuLegacy now supports it and the
> schema file decodes that format exactly. Paths to the original working folders have been removed.

Schema: `texts.txt`. `--check` over the corpus decodes all 83 files of this group (0 NO VARIANT / ERROR).
Abbreviations: "1.00.13 decomp" = `corpus.ngP6/tmp.6FDq/main.c` (CHS 1.00.13), "0.85 decomp" = the 0.85 client's bundled decompilation.
The 0.68/0.69 decompilations hold no loader for any of these files (no file-name strings, no fread of these sizes),
so 0.68/0.69 rest on data validation only.
`xor3` = FC CF AB. "per record" = the key index restarts at every record.

Six formats cannot be expressed in the reference syntax. Each one has an OPAQUE variant (one row holding the whole
file as `raw<size>`) so that `--check` still matches it. The real layouts are given below, and
`decode_special.py` (inlined at the end) decodes them. It was run against every affected corpus file.

---------------------------------------------------------------------------------------------------------------
## Text.bmd

### Fixed variants: 300-byte entries, xor3, no CRC
| size | entries | versions |
|---|---|---|
| 180000 | 600 | 0.68.02, 0.69.00 (Korean cp949), 0.75.00 (English) |
| 240000 | 800 | 0.85.00 |
| 300000 | 1000 | 0.97.04 |
| 360000 | 1200 | 0.97.14-darkav: Text.bmd, Text(ENG).bmd, Text(CHS).bmd |
| 450000 | 1500 | 0.99.28 |
| 614400 | 2048 | 1.00.15, 1.01.02, 1.01.46 |
| 900000 | 3000 | 1.02.44 |

Evidence:
- 1.00.13 decomp `sub_52A4D0`: `malloc(614400); fread(buf,0x96000,1,f); sub_52A5B0(buf,614400)` (xor3 over the
  whole buffer, one stream). It then copies 300-byte (`0x12C`) slices into the array `byte_76458DC`.
  The writer `sub_52A5F0` xors each 300-byte entry separately. 300 % 3 == 0, so stream xor and per-entry xor
  give the same bytes. The schema uses `xor=record`.
- 0.85 decomp `sub_4544F0`: `new(0x3A980)` = 240000, xor over 240000 bytes, copies 300-byte slices into `byte_7CA4990`.
- Data: in every fixed file, every 300-byte slice is NUL-terminated and is all zero after the first NUL
  (0 violations across 15 files). The only exception is two darkav entries that fill all 300 bytes with no NUL.
  Strings are readable (entry 1 = the "Warning!!! ... hacking" line in every version).
- The xor is per entry only in the trivial sense above: one 3-byte key over 300-byte entries.

### GT indexed format (91800: 1.03.47 and 1.04.10; 98694: 1.03.16 and 1.04.24). OPAQUE in the schema
```
u16  magic   = 0x5447 ('G','T')
u32  count                         (3000 in 91800, 2574 in 98694)
count * { u32 index; u32 len; u8 text[len] }   text: xor3 with the key restarting at every entry; no NUL
EOF exactly after the last entry. No trailing CRC.
```
The reference `layout=text-indexed` reads the count at offset 0. It would need a 2-byte magic before the count, for
example `header=gt` / `magic16`. Validation: both sizes parse to EOF exactly. Indexes are sorted and unique:
0..2999 in 91800, 0..3129 with gaps in 98694. Entry 0 = "Tahoma" (font), entry 1 = "Warning!!! ..."; 752 empty
entries in 91800 and 16 in 98694. The longest entry is 283 bytes.
Note: 91800 is a multiple of 300, so the fixed Text variants use exact sizes. A `*` rule would mis-decode GT files.

---------------------------------------------------------------------------------------------------------------
## Filter.bmd / FilterName.bmd: 20-byte strings, xor3 per record
| table | size | layout | versions |
|---|---|---|---|
| Filter | 2000 | 100 x 20, no CRC | 0.75.00 |
| Filter | 20000 | 1000 x 20, no CRC | 0.85.00 |
| Filter | 20004 | 1000 x 20 + CRC wkey 3E7D | 0.97.04 - 1.04.24 (all CRCs verified) |
| FilterName | 2000 | 100 x 20, no CRC | 0.85.00 |
| FilterName | 20004 | 1000 x 20 + CRC wkey 2BC1 | 0.97.04 - 1.01.46 |
| FilterName | 10004 | 500 x 20 + CRC wkey 2BC1 | 1.02.44 - 1.04.24 |
| FilterName | 20004 | 1000 x 20 + invalid trailer | 0.97.14-darkav. OPAQUE (see below) |

Evidence:
- 1.00.13 decomp `sub_52AB50` (Filter) and `sub_52AE90` (FilterName): `fread(buf,0x4E20)` reads 20000 bytes, then
  `fread(&crc,4)`. The inline CRC uses `+15997` (0x3E7D) and `+11201` (0x2BC1) respectively. Then, for each
  20-byte record: `sub_52A5B0(rec,20)` (xor3 restarts per record), memcpy, and **stop at the first record whose
  first byte is 0**. That index becomes the count (`dword_76E0C58` / `dword_76E0C5C`). The schema still lists all
  rows, so a reader should stop at the first empty one, as the client does.
- 0.85 decomp `sub_454770` (Filter) and `sub_454950` (FilterName): `fread(20)` + xor3(20) per record into a
  1000-slot array, stop at the first empty record. No CRC read. The 2000-byte files therefore stop at EOF after
  100 records.
- Some files are empty after the first record: 1.04.10 Filter and FilterName, 1.04.24 Filter, and 1.01.46
  FilterName are all-empty; 1.03.16 Filter has 13 empty leading rows. For these files the client's
  "stop at the first empty record" rule loads nothing. This is a property of the data, not a decoding error.
- 0.97.14-darkav FilterName.bmd: the trailing u32 matches **no** wkey in 0..0xFFFF (brute-forced; the same tool
  finds 2BC1 for 0.97.04). The file was hand-edited. It cannot be expressed: rec=20 fails on the 4-byte trailer,
  and one big record cannot restart xor every 20 bytes. Decode it as 1000 x 20 xor3-per-record and ignore the last
  4 bytes (`filter_nocrc` below; it gives "Destruct", "DESTRUCT", ...).

---------------------------------------------------------------------------------------------------------------
## Dialog.bmd: 204800 = 200 x 1024, xor3 per record (1024 % 3 != 0, so the restart matters). No CRC.
Versions: 0.97.04 - 1.04.24, including darkav.
```
+0    char Text[300]
+300  i32  AnswerCount
+304  i32  AnswerLink[10]     next Dialog index shown for answer k; AnswerLink[AnswerCount] = -1
+344  i32  AnswerAction[10]   action code of answer k (1/2/3 handled; -1 = none)
+384  char Answer[10][64]
```
Evidence:
- 1.00.13 decomp `sub_52C0A0`: loop `fread(buf,0x400)`, `sub_52A5B0(buf,1024)`, memcpy into `byte_76111C0 + k*1024`
  for 200 records.
- The Data2 text parser `sub_52BE80` writes into the same array: Text via strcpy at +0, the count at
  `dword_76112EC` (+0x12C), `dword_76112F0[(rec<<8)+k]` (+0x130, Link), `dword_7611318[...]` (+0x158, Action),
  `byte_7611340[64*(k+16*rec)]` (+0x180, Answer). It ends the list with Link = -1.
- At runtime, `sub_4172xx` (decomp line ~31985) switches on `dword_7611318[...]` with values 1, 2 and 3 (this is
  AnswerAction). Line ~32260 passes `dword_76112F0[...]` to `sub_416C80(link)`, which shows Dialog record `link`
  (this is AnswerLink).
- Data: in all 11 files every used record satisfies 0 <= AnswerCount <= 10, Link[AnswerCount] == -1, and has
  non-empty answers 0..AnswerCount-1. Example: record 53 = the Scroll of Emperor story, links 52/54/53, actions
  -1/1/2.

---------------------------------------------------------------------------------------------------------------
## Quest.bmd: 200 records, xor3 per record, no CRC
| size | rec | act size | classes | versions |
|---|---|---|---|---|
| 116800 | 584 | 18 | 4 | 0.97.04 |
| 136000 | 680 | 20 | 5 | 0.97.14-darkav, 0.99.28, 1.00.15, 1.01.02, 1.01.46, 1.02.44, 1.03.47 |
| 142400 | 712 | 22 | 6 | 1.03.16, 1.04.10, 1.04.24 |

Every record = header (38 bytes) + 16 Acts + pad 2 to 4-byte alignment + 16 Requests.
The schema flattens the nested arrays as `A00_..A15_` and `R00_..R15_` columns. Only the first ActCount acts and
the first RequestCount requests are live. Stale rows can follow; 0.97.04 has 3 such rows.

Header:
- 584 and 680: `i16 ActCount, i16 RequestCount, u8 NpcType @4, char Name[32] @5, pad 1 @37` (byte 37 is always 0).
- 712: `i16 ActCount, i16 RequestCount, u16 NpcType @4, char Name[32] @6`. The u16 is needed: NPC 406 (Apostle
  Devin, 3rd-class quests) is stored as `96 01`. 1.03.16 quest 5 fills all 32 name bytes with no NUL.

Act (680, from code):
```
+0 u8 Unk0 (always 1)   +1 u8 Type (1 = item; 2 = monster kill, seen in 712)   +2 u8 ItemType   +3 u8 ItemIndex
+4 u8 ItemLevel   +5 u8 ItemNum   +6 u8 RequestType   +7 u8 Class[5]   +12 i16 DialogIndex[4]
```
- 584: `+2 ItemType +3 ItemIndex +4 ItemNum +5 RequestType +6 Class[4] +10 DialogIndex[4]`. There is no level byte.
  The value 1 sits where 680 has ItemNum = 1 for the same quests (Scroll / Three Treasures). This is evidence from
  matching values only; no 0.97 code was read.
- 712: `+2 u16 ItemTypeOrMonster +4 ItemIndex +5 ItemLevel +6 ItemNum +7 RequestType +8 Class[6] +14 DialogIndex[4]`.
  This is aligned with 680 by matching values. Type 2 acts have values 409/410/411/412 = the Balgass Barracks
  monsters in "Infiltration of 'Barrack'", and ItemNum then holds the kill count 10 or 20. Class[5] is set only
  for Summoner-era acts.

Request (680 and 712, 20 bytes; 584 has 16 bytes without Unk8):
```
+0 u8 Unk0 (always 1)  +1 u8 Type (matched against Act.RequestType, 0xFF = any)  +2 i16 CompleteQuest (-1 = none)
+4 u16 LevelMin  +6 u16 LevelMax (0 = no max)  +8 u32 Unk8 (680/712 only; always 0, never read)
+12 u32 Zen  +16 i16 ErrorDialog  +18 pad 2
```
Evidence (1.00.13 decomp; the quest object is `dword_6A3E28`, records at this+12):
- Loader `sub_416280`: 200 times `fread(680)`, `sub_416350(buf,680)` (xor3 per record).
- `sub_416A30` / `sub_416520`: loop over `*(WORD*)rec` acts with stride 20, and `*(WORD*)(rec+2)` requests at
  rec+360 with stride 20. The class gate is `act[7 + (heroClass & 7)] != 0`. The item test is
  `act[1] == 1 && sub_5381F0(32*act[2] + act[3], act[5], act[4])`. `sub_5381F0(item, count, level)` counts
  inventory items whose `(level>>3)&15 == act[4]`, so act[4] = ItemLevel and act[5] = ItemNum.
  RequestType: `act[6]` is compared with `req[1]` or 0xFF.
- Request checks in `sub_416520`: `req[2..3] != -1` triggers a quest-state check (`sub_416B30`).
  `req[4..5] > hero level (char+14)` fails. `req[6..7]`, when nonzero, fails if less than the level.
  `*(DWORD*)(req+12)` is compared with money. On failure, `*(WORD*)(req+16)` goes to the "current dialog" field.
- `sub_4164B0`: returns `*(WORD*)(rec + 50 + 20*act + 2*state)`, i.e. act+12 holds 4 i16 values per quest state.
  `sub_416DB0` / `sub_416C80` show that value as a Dialog.bmd record, hence `DialogIndex` and `ErrorDialog`.
- `rec+4` (u8) goes to `sub_52F4E0` (NPC lookup). `rec+5` is the name string. Values 235 (Sevina) and 229 (Marlon)
  match the quest NPCs.
- Data sanity: Scroll of Emperor quest: level 150, 1,000,000 zen, item (14,23). Marlon quest: level 220,
  3,000,000 zen, requires quest 1.

Open: the meaning of Class values. Nonzero = allowed; 2 = needs the 2nd class? This is not proven.
Also open: Unk0 of acts and requests (always 1), and Unk8.

---------------------------------------------------------------------------------------------------------------
## Slide.bmd: 41008 bytes, a single record, xor3 over the whole file. No CRC.
Versions: 0.97.14-darkav, 0.99.28 - 1.04.24.
```
+0    u32 IntervalSec          SetTimer(0x3EB, 1000*IntervalSec)
+4    f32 Unk4                 always 2.5; copied to this+4632 and passed to the slide renderer sub_459770 (unproven role)
+8    5 x { u32 MaxLevel; u32 Count; char Text[32][256]; }   (8200 bytes per band)
```
Evidence: 1.00.13 decomp `sub_459F30`: `fread(&v24, 0xA030)`, `sub_4AF4B0(buf, 41008)`, then a loop of 5 with stride
8200. In each band it takes `*(p-4)` (MaxLevel) and `*p` (Count), and Count strings at p+4 with stride 256.
`sub_45A200(level)` picks the first band with `level <= MaxLevel[i]`, then a random line `< Count[i]`. Its caller
passes `*(WORD*)(hero+14)` (Level). `sub_459B90` sets `SetTimer(..., 1000 * *(this+4524))`.
Data: bands 15/30/50 with 28/.. lines, and bands 3-4 have 0/0. Single-record decoding makes stream xor and
record xor identical.

---------------------------------------------------------------------------------------------------------------
## Credit.bmd: 13200 = 400 x 33, xor3 (33 % 3 == 0, so record and stream are identical). No CRC found.
Versions: 1.02.44, 1.03.16, 1.03.47, 1.04.10, 1.04.24.
`u8 LineType; char Text[32]`. There is no decompilation for 1.02+. The name comes from the data: all 244 used rows
(×5 files) follow 1 = top section title ("STAFF", ...), 2 = role heading ("Game Design"), 3 = person or company
name. 0 marks unused rows. In 1.03.16 the unused rows and the tails after each NUL are filled with 0xCD (MSVC heap
fill). The fill restarts at each 33-byte boundary, which confirms the record size.

---------------------------------------------------------------------------------------------------------------
## ServerInfo.bmd (1.04.10, 169120 bytes). OPAQUE: not a Webzen file
This is the kayito custom-main configuration. Cipher (derived from the raw data: the keystream repeats every 1280 =
lcm(20, 256); zero regions decrypt to zero and every string is legible):
```
plain[i] = enc[i] ^ K20[i % 20] ^ (i & 0xFF)
K20 = 9e 98 12 2b b6 76 f6 5b 76 4d 08 f2 ab df 91 b6 4c 1c 93 2f
```
Header fields, located from the decrypted values (names match the decrypted content):
`@33 char[32]` customer ("Nico"), `@65 char[32]` IP ("192.168.0.64"), `@98 u16` port (44405),
`@100 char[8]` client version ("1.04.10"), `@108 char[17]` client serial, `@125 char[32]` window title,
`@157 char[50]` screenshot path format, `@207 char[32]` exe name ("Main.exe"). After that come custom map entries
(texture / name / music paths, from ~@352) and custom jewel entries ("Jewel180".."Jewel187" @144048+). These were
not decoded further. Strings carry stale bytes after their NUL. The tool did not clear its buffers.

---------------------------------------------------------------------------------------------------------------
## partCharge1/ShopCategoryItems.bmd (1008) and ShopUI.bmd (1376), 1.03.16. OPAQUE
```
u32 count
count * rec bytes     xor3 restarting at every record (records start at file offset 4)
u32 crc               MuCrypto CRC, wkey E2F1, computed over the records ONLY (bytes 4 .. end-4, count excluded)
```
The reference `crc=` covers everything before the trailer, count included, so it does not match. Before that,
brute force found no wkey for a CRC that includes the count. A `crc-skip-header` option would make this
expressible as `header=count32 crc=E2F1 rec=20|72`.
- ShopCategoryItems: count 50, rec 20: `u16 Unk0` (1 in row 0, 50 elsewhere), `u16 ItemGroup`, `u16 ItemIndex`,
  14 zero bytes. Values include (14,53), (14,54), (13,43..48), (13,54..65), (14,70..82), (14,91..98), (12,60..65),
  (13,69..70): groups 12/13/14 only, in the ranges of the Season 4 cash-shop items. The group/index naming comes from those values; there is no code evidence.
  Row 49 is ten u16 values of 50 and looks like filler.
- ShopUI: count 19, rec 72: `u32 Unk0` (0/1), `u32 Unk4` (1..3), `u32 Unk8` (0..4), `char Name[60]`
  ("A_button_01".."A_button_04", "Shop", "Normal", "Potion", "Accessory", "Special", "WishList", "Cart").

---------------------------------------------------------------------------------------------------------------
## Monster50.bmd (0.97.14-darkav, 178778). OPAQUE: not a table
`"BMD"` + version byte 0x0C + u32 0x0002BA52 (= 178770 = file size - 8) + an encrypted body. This is a version-12
(encrypted) BMD 3D model misplaced in Data/Local. It belongs to the model decoder, not to this schema.

---------------------------------------------------------------------------------------------------------------
## decode_special.py (tested)
Command lines run:
`text` on 1.03.16/1.04.10 Text.bmd, `shopcat`, `shopui`, `filternocrc` on darkav FilterName, `serverinfo`.
Outputs:
`2574 (0,'Tahoma') ... (3129,'Please buy this item again ...')`, `3000 ... (2999,'')`,
`50 [(1,14,53),(50,14,54),(50,13,43),(50,13,44)]` (CRC asserted),
`19 [((0,1,1),'A_button_01'),...]` (CRC asserted), `1000 ['Destruct','DESTRUCT',...]`,
`'Nico' '192.168.0.64' 44405 '1.04.10' ...`.
```python
import struct
KEY = b'\xfc\xcf\xab'
def xor3(b): return bytes(c ^ KEY[i % 3] for i, c in enumerate(b))
def mucrc(buf, wkey):
    c = (wkey << 9) & 0xffffffff
    for i in range(0, len(buf) - 3, 4):
        t = struct.unpack_from('<I', buf, i)[0]
        c = (c + t) & 0xffffffff if (wkey + (i >> 2)) % 2 == 1 else c ^ t
        if i % 16 == 0: c ^= ((c + wkey) & 0xffffffff) >> ((i >> 2) % 8 + 1)
    return c

def text_gt(d):            # Text.bmd 1.03.16/1.03.47/1.04.x
    magic, count = struct.unpack_from('<HI', d, 0)
    assert magic == 0x5447
    p, out = 6, []
    for _ in range(count):
        idx, ln = struct.unpack_from('<II', d, p); p += 8
        out.append((idx, xor3(d[p:p + ln]))); p += ln
    assert p == len(d)
    return out

def counted_crc_records(d, rec, wkey=0xE2F1):   # ShopCategoryItems rec=20, ShopUI rec=72
    count = struct.unpack_from('<I', d, 0)[0]
    body = d[4:4 + count * rec]
    assert 4 + count * rec + 4 == len(d)
    assert mucrc(body, wkey) == struct.unpack_from('<I', d, len(d) - 4)[0]
    return [xor3(body[i * rec:(i + 1) * rec]) for i in range(count)]

def filter_nocrc(d, rec=20):   # 0.97.14-darkav FilterName (invalid CRC trailer ignored)
    return [xor3(d[i * rec:(i + 1) * rec]) for i in range((len(d) - 4) // rec)]

SI_K20 = bytes.fromhex('9e98122bb676f65b764d08f2abdf91b64c1c932f')
def serverinfo(d):             # kayito ServerInfo.bmd
    return bytes(c ^ SI_K20[i % 20] ^ (i & 0xFF) for i, c in enumerate(d))
```
