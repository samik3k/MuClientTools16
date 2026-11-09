# Skills group: notes and evidence

> Research notes behind `../skills.txt`: evidence for each field (decompiled loaders, value checks), byte-exact
> layouts and tested Python decoders. They were written against a reference decoder with a smaller schema syntax;
> where they call a format OPAQUE, "container" or FALLBACK, or propose an option (`header=count32cap`, `skip=`,
> `counts32`, `trailer=`, `crc_from=`, `probe=`, a magic before the count), MuLegacy now supports it and the
> schema file decodes that format exactly. Paths to the original working folders have been removed.

Tables: Skill.bmd (all variants), MasterSkillTree.bmd, MonsterSkill.bmd, BuffEffect.bmd, Pet.bmd, Mix.bmd.
Schema: `skills.txt` (same folder). Offsets below are relative to the record start unless stated otherwise.
"+N after name" means record offset 32+N.

Tables the schema language cannot express exactly: MasterSkillTree header, BuffEffect, Pet, Mix, and the
0.97.14 DaRKav Skill trailer. The schema has container or approximate variants for them so `--check` passes.
The exact layouts are below, and a tested standalone decoder is at the end of this file. It was run on every
corpus file of these tables, CRCs included.

Suggested schema-language extensions that would make every one of them exact:
- `skip=<N>`: N plain header bytes before the records (Pet: 12).
- `header=counts32:<T>`: T XOR'd i32 counts whose sum is the record count (Mix: T = 6, 8 or 14).
- `crc_from=<offset>`: the CRC covers data[offset:-4] instead of data[:-4] (BuffEffect: 4, Pet: 12).
- `trailer=<N>`: N unknown trailing bytes, so the 0.97.14 Skill file can use `rec=42`.
- MasterSkillTree would also need a second XOR period for its 45-byte header (9 rows of 5 bytes).

---

## Skill.bmd

All variants: XOR FC CF AB restarting at every record. Name is `str32` in every version.
CRC wkey 5A18 over everything before the trailing u32. It is present from 0.97.04 on and absent in 0.68, 0.75 and 0.85.

| size  | variant    | rec | count | CRC  | versions |
|-------|------------|-----|-------|------|----------|
| 2304  | s0_36      | 36  | 64    | none | 0.68.02 (KOR cp949), 0.75.00 (ENG) |
| 2432  | s0_38      | 38  | 64    | none | 0.85.00 (CHS, `--cp gbk`) |
| 2564  | s0_40      | 40  | 64    | 5A18 | 0.97.04 |
| 2692  | darkav_42  | 42  | 64    | 4 unknown bytes | 0.97.14 DaRKav |
| 13316 | s0_52      | 52  | 256   | 5A18 | 0.99.28 |
| 15364 | s1_60      | 60  | 256   | 5A18 | 1.00.15, 1.01.02, 1.01.46, 1.02.44, 1.03.47, 1.04.10 |
| 40804 | s3_68      | 68  | 600   | 5A18 | 1.04.24 |
| 48004 | s4_80      | 80  | 600   | 5A18 | 1.03.16 GMO (cp1252) |

### Code evidence

**0.85 (`main085.c`)**
- `sub_455300` writes `Data\Skill\Skill.bmd` as 2432 bytes, 38 bytes per record.
- The text parser `sub_454F80` reads `Data2\skill(%s).txt` and stores:
  - +32 BYTE Level
  - +33 BYTE Damage
  - +34 WORD Mana
  - +36 BYTE Distance
  - then skips 4 more tokens.
- The record is 37 bytes rounded up to 38 for u16 alignment, so +37 is padding (always 0).

**1.00.13 CHS (`main.c`)**
- `sub_52B7F0` and `sub_52BAC0` read and write 15360 + 4 bytes (`0x3C00`, 60 per record) and check the CRC.
  The constant 23064 is 0x5A18.
- The text loader near line 237054 writes:
  - +32 BYTE Level
  - +33 BYTE Damage
  - +34 WORD Mana
  - +36 WORD AG (the 4th numeric column)
  - +38 BYTE Distance
  - +44 BYTE Energy
- Line 22394 is the requirement check: `RequiredEnergy = 4 * Level(+32) * Energy(+44) / 100 + 20`.
  It has hard-coded overrides for summons 30–36 (30, 60, 90, 130, 170, 210, 300), skill 60 (15), and a set of
  zeros. So **Energy is a percentage multiplier, not an absolute requirement**.
- `sub_5325B0` returns byte +48. The tooltip prints `GlobalText[base + value]` unless the value is 0xFF.
  So +48 is the **element Attribute** (0 ice, 1 poison, 2 lightning, 3 fire, 4 earth, 5 wind, 6 water,
  255 none). The data agrees: Ice=0, Poison=1, Lightning=2, Fire Ball/Flame/Hellfire/Inferno=3,
  Meteorite=4, Twister=5, Aqua Beam=6.
- Line 243334 uses +40 as an int denominator for the cooldown overlay (`remaining / *(int*)(+40)`),
  so +40 is **Delay** in milliseconds. Data: Earthshake 10000, Summon 30000, Electric Spike 5000,
  Plasma Storm 1500.
- Line 242210 builds the damage as `skill(+33) + character magic damage`.
- Tooltip near line 291682:
  - If +49 == 2, it prints the name and damage of skill `[+50]`. So +50 is a skill index (**Brand**).
  - If +49 == 1, it prints the number in +51 (**KillCount**).
  - +49 is the **UseType**: 0 normal, 1 kill-count skill, 2 brand skill.
  - In data, the Castle Siege skills 67–72 have UseType 1 and KillCount 4, 2, 4, 5, 2, 8.
  - S3/S4 master skills have UseType 3 (passive) or 4 (strengthened active). Their Brand points at the
    parent skill.

### Field names backed by values (all variants)

- **Level**: the required level. 0.75 check: Fire Ball 5, Power Wave 9, Lightning 13, Teleport 17,
  Meteorite 21, Ice 25, Poison 30, Flame 35, Twister 40, Evil Spirit 50, Hellfire 60. With Energy=100 % the
  formula gives the classic energy requirements 40, 56, 72, 88, 104, 120, 140, 160, 180, 220, 260.
- **Energy** (percent): the formula reproduces the known requirements:

  | skill | requirement |
  |---|---|
  | Aqua Beam | 345 |
  | Cometfall | 500 |
  | Inferno | 724 |
  | Soul Barrier | 408 |
  | Teleport Ally | 644 |
  | Decay | 953 |
  | Ice Storm | 849 |
  | Nova | 1052 |
  | Summon (Dark Lord) | 153 |
  | Increase Critical Damage | 102 |
  | Electric Spike | 126 |

  Data quirks:
  - 1.00.15, 1.01.02, 1.04.24 and 0.99.28 store Energy truncated to 8 bits: Nova 258 → 2,
    Summon Bali 260 → 4, Summon Soldier 280 → 24. The 1.00.13 code reads only a byte.
  - 1.01.46, 1.02.44, 1.03.47 and 1.04.10 store the full u16 (Nova 258), so the field is typed u16.
    The high byte is 0 in older files.
- **Energy in 0.97.04** (+7 after name, u8) is in units of 10 %:
  - Summons 9, 17, 19, 23, 25, 26, 28 equal the DaRKav or 0.99 values 90…280 divided by 10.
  - Aqua Beam 11 gives 345.
  - Blast (level 80, 13) gives 436 and Inferno (level 93, 15) gives 578. These are the old 0.97
    requirements.
  - Mass Teleport and Mana Shield have Level 0 in this version.
- **Damage**: Poison 12, Meteorite 21, Lightning 17, Fire Ball 8, Flame 25, Ice 10, Twister 35,
  Evil Spirit 45, Hellfire 120, Aqua Beam 80, Death Stab 70 (1.00+), Electric Spike 250, Earthshake 150.
- **Mana**: Poison 42, Evil Spirit 90, Hellfire 160, Summon Soldier 350.
- **AG**: Twisting Slash 10, Rageful Blow 20, Death Stab 12, Greater Fortitude 24, Earthshake 50,
  Electric Spike 100.
- **Distance**: 6 for spells, 2–3 melee, Ice Arrow 8, Earthshake/Electric Spike 10. It is u8 + pad in the
  60/52-byte layouts and u32 in the 68/80-byte layouts.
- **Leadership** (Command), u16:
  - 1.00+: Summon 400, Increase Critical Damage 300, Electric Spike 340, Fire Scream 70. These match the
    Dark Lord scroll requirements.
  - 1.04.24 KOR has 100, 100, 200, 70 instead.
- **ReqClass[N]**: per class 0 = cannot, 1 = base class, 2 = 2nd class, 3 = 3rd class or master. Index order
  is DW, DK, Elf, MG, DL, then Summoner in S3+ (6 entries). Evidence:
  - Poison: DW+MG.
  - Heal: Elf.
  - Force: DL.
  - Teleport Ally: DW=2 (Soul Master).
  - Rageful Blow: DK=2.
  - Ice Arrow: Elf=2.
  - Plasma Storm: 2,2,2,1,1,2 (Fenrir rule: 2nd class, MG/DL base).
  - Drain Life, Chain Lightning, Sleep, Reflex: Summoner only.
  - Master skills: 3 for the owning class.
  - Note: in 1.04.24 and 1.03.16, Meteorite, Fire Ball, Ice, Power Wave and Mana Glaive also have the
    Summoner flag set.
- **ReqGuildStatus[3]**: nonzero only for the Castle Siege skills 67–72 and the Illusion Temple skills
  210–213. Stun/Swell Mana: 0,0,1; Invisibility: 1,1,0; Cancel Magic: 1,0,0. The values look like
  guild-master, assistant and battle-master flags. That meaning is **not proven by code**: the column order
  matches the server `ReqStatus` columns, but treat the name as provisional.

### Per-variant layouts

**s0_36 (0.68 / 0.75)**: Name, Level u8, Damage u8, Mana u8, Distance u8. Hellfire mana 160 and Summon
Soldier 250 fit in u8.

**s0_38 (0.85)**: Name, Level u8, Damage u8, Mana u16, Distance u8, pad 1.

**s0_40 (0.97.04)**:

| record offset | type | field |
|---|---|---|
| +32 | u8 | Level |
| +33 | u8 | Damage |
| +34 | u16 | Mana |
| +36 | u16 | AG |
| +38 | u8 | Distance |
| +39 | u8 | Energy, in 10 % units |

**darkav_42 (0.97.14)**: same as s0_40, but +39 is Energy in percent and +40 is Attribute (u8), followed by
1 byte of padding (always 0). The file ends with 4 bytes that are not a MuCrypto CRC: all 65536 wkeys were
brute-forced with no match. 42 % 3 == 0, so a continuous XOR equals the per-record XOR, which is why the
schema can decode the file as one 2692-byte row (S00_…S63_ columns plus `Trailer`). The decoder at the end
prints it as 64 normal rows.

**s0_52 (0.99.28)**:

| after name | type | field |
|---|---|---|
| +0 | u8 | Level |
| +1 | u8 | Damage |
| +2 | u16 | Mana |
| +4 | u16 | AG |
| +6 | u8 | Distance |
| +7 | pad 1 | |
| +8 | u32 | Delay |
| +12 | u16 | Energy |
| +14 | u16 | Leadership |
| +16 | u8 | Attribute |
| +17 | pad 3 | always 0; struct padded to 4 |

The 0.99.28 Delay values are odd: Teleport 5120, Earthquake 1018, Summon 48, Electric Spark 256. The field
position matches 1.00, but the values look stale. Open question.

**s1_60 (1.00.15 – 1.04.10)**: the s0_52 layout with the following after Attribute:

| after name | type | field |
|---|---|---|
| +17 | u8 | UseType |
| +18 | u8 | Brand |
| +19 | u8 | KillCount |
| +20 | u8×3 | ReqGuildStatus |
| +23 | u8×5 | ReqClass |

Every offset is code-proven except ReqGuildStatus and ReqClass, which are value-proven.

**s3_68 (1.04.24)**: Level, Damage, Mana and AG widened to u16; Distance u32, Delay u32, Energy u16,
Leadership u16, Attribute, UseType, Brand, KillCount, ReqGuildStatus[3], ReqClass[6], then:
- **Unk65** (u8): always 0.
- **Unk66** (u16): 0 for normal skills. For master skills it is a per-group id shared by the 5 levels of a
  master skill, from 2 to 0x2D. It is probably the master-skill icon or group, but this is not proven.

Data quirks in 1.04.24:
- Records 48 and 49 have their names swapped relative to every other version. "Fire Breath" carries
  Greater Fortitude's stats: level 120, mana 22, AG 24.
- Brand is stored as a u8 truncated skill index: 441 → 0xB8 (440 & 0xFF), 513 → 0x00 (512 & 0xFF).

**s4_80 (1.03.16 GMO)**: like s3_68, but Energy is u32 and Leadership is at +20 after name. After ReqClass:
- **Unk67** (u8): always 0.
- **Unk68** (u16): same as Unk66 in 1.04.24.
- **Unk70** (u8):
  - 1 for self or party buffs: Soul Barrier, Greater Defense, Greater Damage, Greater Fortitude, Swell Mana,
    Infinity Arrow, Reflection.
  - 2 for debuffs: Poison, Ice, Decay, Sleep, Stun, Fire Slash.
  - 3 for Heal.
  - Weakness and Innovation are 0, so the meaning is unproven.
- pad 1.
- **Unk72** and **Unk76** (u32): always 0. They are probably Str/Agi requirements (the S6 client struct has
  `int Strength, Dexterity` here), but no value proves it.

---

## MasterSkillTree.bmd

2689 bytes, identical layout in 1.03.16 and 1.04.24. The contents differ: 1.04.24 KOR keeps only the common
skills, so its class blocks are identical. CRC wkey 2BC1 covers data[:-4], i.e. all 2685 data bytes.

```
off 0    : 9 rows x 5 u8         XOR restarts every 5 bytes (row r = xor3(d[5r:5r+5]))
           1.03.16 = 1.04.24 = [1,1,1,1,1] [1,1,1,1,2] [1,1,1,2,2] [1,1,2,2,2] [1,2,2,2,2]
                               [1,2,2,2,3] [1,2,2,3,3] [1,2,3,3,3] [1,3,3,3,3]   (meaning unknown)
off 45   : 5 classes (DW, DK, Elf, MG, DL) x 4 trees x { i32 Unk0; i32 Cell[32]; }   = 2640 B
           XOR continuous from byte 45; per-132-byte restarts give the same result because 132 % 3 == 0
off 2685 : u32 CRC (wkey 2BC1)
```

- **Cell values** are a skill index plus 10000×k, with k in {0, 2, 4}. Examples:
  40330 = skill 330, k=4; 20355 = skill 355, k=2. The cells hold the ids of master skills 300–5xx.
  - k=2 appears on the left cell of a horizontally linked pair: 20355 Max Life next to 360 Max AG;
    20315 next to 320.
  - k=4 appears on the Resistance and Max SD chains.
  - These are probably connector or prerequisite markers, but that is not proven.
  - -1 and 0 are both "empty". The difference between them is unknown.
- **Grid shape**: the 32 cells are probably 8 rows × 4 columns, judging by where the non-empty cells cluster.
  Unk0 per tree is 4, 2, 2, 3. Neither is proven.
- **Class order** was verified by the skills each class block contains:
  - DW: Hellfire, Evil Spirit, Soul Barrier strengtheners.
  - DK: Twisting Slash, Rageful Blow, Swell Life.
  - Elf: Heal, Triple Shot.
  - MG: Blast, Power Slash, Blood Attack.
  - DL: Earthshake, Fire Burst, Fire Scream.
- **Schema**: one 2685-byte record. The body columns are correct, but `HeaderRaw` is the mis-phased XOR of
  the header. Use the decoder below for the header.

---

## MonsterSkill.bmd

The u32 count is plain. Records use XOR restarting at every record. There is no CRC: the brief's key search
found none, and the size equals 4 + count × rec exactly.

**s3_28** (452 bytes, 16 × 28; 1.02.44 / 1.03.47 / 1.04.10 / 1.04.24, all identical):

| record offset | type | field |
|---|---|---|
| +0 | i32 | MonsterIndex |
| +4 | i32×5 | ids, -1 = empty |
| +24 | i32 | always 0 |

- The MonsterIndex values are 304, 305, 309, 340, 341, 344, 345, 349, 355, 358–364. These are Kalima,
  Crywolf and Kanturu monster indices: 362 and 363 (the two Maya hands) share the same ids (23, 24), and
  364 is Maya.
- The meaning of the 5 ids (skill-unit or effect ids?) is unknown.

**s4_48** (1108 bytes, 23 × 48; 1.03.16 GMO):
- Each record holds 11 i32 plus a final i32 that is always 0.
- The 11 values are a continuous stream of the same (monster, 5 ids) 6-int tuples as s3_28. A record
  boundary does not line up with a monster: `304 4 1 -1 -1 -1 305 5 -1 -1 -1 | 0 | -1 309 2 3 …`.
- From record 8 on, the tuples belong to the S4 monsters 388–403 (28, 27), 412 and 440 (8), and
  454–459 (29–37). The last record is `0 36 38 39 40 41 42 43 43 43 43`.
- To recover tuples, concatenate ints[0:11] of every record and split into groups of 6.
- Columns are Unk because the per-record columns have no stable meaning.

---

## BuffEffect.bmd

16440 bytes, identical in 1.03.16 and 1.04.24.

```
u32 Count (plain) = 104
Count x 158-byte record, XOR restarting per record:
  +0   u16  Index        1..104
  +2   u8   Group        buffs of one group replace each other: seals 21/24, scrolls 25, leaps 26
  +3   u8   ItemType     255 = not an item buff
  +4   u8   ItemIndex    Seal of Ascension 13/43, Wealth 13/44, Sustenance 13/45, Mobility 13/59,
                         Scroll of Quickness..Mana 14/72..77, Leap of Str..Control 14/78..82,
                         Jack O'Lantern 14/46..50, Talismans 13/81..82
  +5   str50 Name        garbage after the NUL
  +55  u8   IsDebuff     1 for Poison, Ice, Iced Arrow, Stun, Sleep, Blind, Weakness, Innovation, ...
                         0 for buffs
  +56  u8   Unk56        0/1, unknown
  +57  u8   Unk57        always 1
  +58  str100 Description
u32 CRC: MuCrypto CRC with wkey E2F1 over the 104*158 record bytes only (data[4:-4])
```

The CRC was verified on both files. The generic `crc=` option hashes data[:-4], which includes the count,
so the schema only has a container variant.

---

## Pet.bmd

2096 bytes in 1.04.24 and 41616 bytes in 1.03.16.

```
u32 MaxRecords = 100   (plain)
u32 ActionSlots = 50   (plain; equals the length of both arrays below)
u32 Count              (plain) 1.04.24: 5 (5 records stored); 1.03.16: 99 (100 records stored, only 5 used)
N x 416-byte record, XOR restarting per record:
  +0    i32  ItemIndex   pet item index in group 13: 64 Demon, 65 Spirit of Guardian, 67 Pet Rudolf,
                         80 Pet Panda, 106 Pet Unicorn
  +4    i32  Unk4        -1 or 0
  +8    f32  Unk8        0.2 / 0.4 / 0.8 / 0.8 / 0.6
  +12   i32  ActionCount 5
  +16   i32  Action[50]  e.g. 3,3,3,3,3 / 1,2,3,3,3
  +216  f32  Speed[50]   0.5 or 0.8 for the used entries
u32 CRC: wkey 7F1D over the record bytes only (data[12:-4])
```

- The CRC wkey was found by brute force on the 1.04.24 file and confirmed on 1.03.16.
- Records 5–99 of 1.03.16 have ItemIndex 0, but their Speed[0..4] is set to 0.8.
- The `Action` and `Speed` names come from the parallel-array shape: 50 ints followed by 50 floats.
  They are provisional.

---

## Mix.bmd (Chaos Machine recipes)

No CRC. Sizes: 30200 (1.03.47, 1.04.10, identical), 37424 (1.04.24), 76152 (1.03.16).

```
i32 Count[T]   XOR'd as one block (key restarts at 0)
               T = 6  -> 44,1,1,0,0,0                       (46 recipes, 1.03.47/1.04.10)
               T = 8  -> 48,1,1,1,2,2,1,1                   (57 recipes, 1.04.24)
               T = 14 -> 95,1,1,1,2,2,1,1,4,3,1,1,2,1       (116 recipes, 1.03.16)
then for each mix type t, Count[t] recipes of 656 bytes, XOR restarting per recipe:
  +0    i32  MixIndex       0..Count[t]-1 within its type
  +4    i32  MixID          3 = +10 item, 4 = +11, 22 = +12, 6 = Fruit, ...
  +8    i32  Name[3]        GlobalText ids (613, 604, 605, ...)
  +20   i32  Desc[3]        GlobalText ids
  +32   i32  Advice[3]      GlobalText ids
  +44   i32  Unk44          -1, 1, 2, 5  (paired with +48; looks like a W x H size, not proven)
  +48   i32  Unk48          -1, 1, 4
  +52   i32  Unk52          always 10
  +56   u8   Unk56          'A', 'B' or 'C' (0x41..0x43); bytes +57..+59 are garbage in 1.03.16/1.04.24
  +60   u32  RequiredZen    Fruit 3,000,000; +10 2,000,000; +11 4,000,000; +12 6,000,000
  +64   i32  RateTokenCount 0..32
  +68   32 x { i32 Op; f32 Value; }
        Op 0 = constant (Value float: 50.0, 45.0, 4.0, 3.0, 10.0, 4e6, ...), 1 '+', 2 '-', 3 '*', 4 '/',
        5 '(', 6 ')', 7 function or rounding?, and 32..38, 64 = variables. Example: "50 + 7 ( ( 38 - 4 ) / 3 ) * 10".
        Unused tokens are 0 in 1.04.10 and stack garbage in 1.03.16/1.04.24.
  +324  i32  Unk324         90 / 75 / 70 / 100 / 65 / 60 / 40; probably the maximum success rate
  +328  u8   Unk328         'A'..'F'; +329..+331 are garbage in some files
  +332  8 x source item, 40 bytes each:
        u16 TypeMin, u16 TypeMax   item code = group*512 + index
                                   e.g. 7190 = 14/22 Jewel of Creation, 6159 = 12/15 Jewel of Chaos,
                                   7181 = 14/13 Bless, 7182 = 14/14 Soul,
                                   0..7167 = any item of groups 0-13, 0..6143 = weapons and armour
        i32 LevelMin, LevelMax     +10 recipe: 9..9; +11: 10..10; +12: 11..11
        i32 OptMin, OptMax         Chaos weapon / wing recipes: 4..255
        i32 Unk(min), Unk(max)     0..255
        i32 CountMin, CountMax     +11 recipe: Bless 2..2, Soul 2..2; +12: 3..3
        i32 Unk flag               0..4
  +652  i32  NumSources     2 / 4 / 5 / 3 / 7, matching the number of filled source slots
```

How this was established:
- The record size and the header length come from the XOR phase. The key restarts at
  `4*T + 656*k` for every k.
- The header length is proven by the counts: the sum of `Count[T]` times 656, plus 4*T, equals the file size
  exactly for all four files.
- MixIndex, the source item codes, the level ranges and the counts check out against the classic
  Chaos Machine recipes:
  - Fruit = Creation + Chaos, 3 M zen.
  - +10 = item+9 + Chaos + Bless + Soul, 2 M zen.
  - +11 = item+10 + Chaos + 2 Bless + 2 Soul, 4 M zen.
  - +12 = item+11 + 3 Bless + 3 Soul, 6 M zen.

---

## Standalone decoder (tested on every corpus file of these tables)

Usage: `python3 -I decoder.py <file>`. The script picks the format from the file name.

```python
#!/usr/bin/env python3
import os, struct, sys

KEY = b'\xfc\xcf\xab'


def xor3(b):
    return bytes(c ^ KEY[i % 3] for i, c in enumerate(b))


def crc(buf, wkey):
    c = (wkey << 9) & 0xffffffff
    for i in range(0, len(buf) - 3, 4):
        t = struct.unpack_from('<I', buf, i)[0]
        c = (c + t) & 0xffffffff if (wkey + (i >> 2)) % 2 == 1 else c ^ t
        if i % 16 == 0:
            c ^= ((c + wkey) & 0xffffffff) >> ((i >> 2) % 8 + 1)
    return c


def cstr(b, cp='cp949'):
    return b.split(b'\0', 1)[0].decode(cp, 'replace')


def buffeffect(d):
    n = struct.unpack_from('<I', d)[0]
    assert 4 + n * 158 + 4 == len(d)
    assert crc(d[4:-4], 0xE2F1) == struct.unpack('<I', d[-4:])[0], 'CRC'
    print('Index\tGroup\tItemType\tItemIndex\tName\tIsDebuff\tUnk56\tUnk57\tDescription')
    for k in range(n):
        r = xor3(d[4 + k * 158:4 + (k + 1) * 158])
        idx, grp, it, ii = struct.unpack_from('<HBBB', r)
        print(idx, grp, it, ii, cstr(r[5:55]), r[55], r[56], r[57], cstr(r[58:158]), sep='\t')


def pet(d):
    cap, slots, cnt = struct.unpack_from('<3I', d)
    body = d[12:-4]
    assert len(body) % 416 == 0 and crc(body, 0x7F1D) == struct.unpack('<I', d[-4:])[0], 'CRC'
    print(f'# MaxRecords={cap} ActionSlots={slots} Count={cnt} stored={len(body)//416}')
    print('#\tItemIndex\tUnk4\tUnk8(f32)\tActionCount\tAction[0..]\tSpeed[0..]')
    for k in range(len(body) // 416):
        r = xor3(body[k * 416:(k + 1) * 416])
        item, unk4 = struct.unpack_from('<ii', r)
        unk8, = struct.unpack_from('<f', r, 8)
        ac, = struct.unpack_from('<i', r, 12)
        acts = struct.unpack_from('<50i', r, 16)
        spd = struct.unpack_from('<50f', r, 216)
        n = max(0, min(ac, 50))
        print(k, item, unk4, f'{unk8:g}', ac, ','.join(map(str, acts[:n])), ','.join(f'{s:g}' for s in spd[:n]), sep='\t')


def mix(d):
    t = None
    for cand in (6, 8, 14):  # T such that the header sums match the file size
        cnts = struct.unpack('<%di' % cand, xor3(d[:cand * 4]))
        if cand * 4 + sum(cnts) * 656 == len(d):
            t = cand
            break
    assert t, 'unknown Mix layout'
    print('# MixTypes=%d Counts=%s' % (t, ','.join(map(str, cnts))))
    print('Type\tMixIndex\tMixID\tName[3]\tDesc[3]\tAdvice[3]\tWidth\tHeight\tUnk52\tUnk56\tReqZen\tRateTokens\tUnk324\tUnk328\tNumSources\tSources')
    p = t * 4
    for mt, c in enumerate(cnts):
        for _ in range(c):
            r = xor3(d[p:p + 656])
            p += 656
            v = struct.unpack_from('<17i', r)
            ntok = v[16] if 0 <= v[16] <= 32 else 0
            toks = []
            for i in range(ntok):
                op, = struct.unpack_from('<i', r, 68 + i * 8)
                val, = struct.unpack_from('<f', r, 72 + i * 8)
                toks.append(f'{op}:{val:g}' if op == 0 else str(op))
            u324, = struct.unpack_from('<i', r, 324)
            nsrc, = struct.unpack_from('<i', r, 652)
            src = []
            for i in range(min(max(nsrc, 0), 8)):
                f = struct.unpack_from('<HH9i', r, 332 + i * 40)
                src.append('%d-%d/L%d-%d/O%d-%d/U%d-%d/C%d-%d/F%d' % f)
            print(mt, v[0], v[1], ','.join(map(str, v[2:5])), ','.join(map(str, v[5:8])), ','.join(map(str, v[8:11])),
                  v[11], v[12], v[13], r[56], struct.unpack_from('<I', r, 60)[0], ' '.join(toks), u324, r[328],
                  nsrc, ' '.join(src), sep='\t')


def masterskilltree(d):
    assert len(d) == 2689 and crc(d[:-4], 0x2BC1) == struct.unpack('<I', d[-4:])[0]
    for r in range(9):
        print('Header', r, *xor3(d[r * 5:r * 5 + 5]), sep='\t')
    body = struct.unpack('<660i', xor3(d[45:-4]))
    for c in range(5):
        for t in range(4):
            g = body[(c * 4 + t) * 33:(c * 4 + t + 1) * 33]
            print('Class%d' % c, 'Tree%d' % t, g[0], ','.join(map(str, g[1:])), sep='\t')


def skill_darkav(d):
    print('#\tName\tLevel\tDamage\tMana\tAG\tDistance\tEnergy\tAttribute')
    for k in range(64):
        r = xor3(d[k * 42:(k + 1) * 42])
        lv, dmg, mana, ag, dist, en, attr = struct.unpack_from('<BBHHBBB', r, 32)
        print(k, cstr(r[:32]), lv, dmg, mana, ag, dist, en, attr, sep='\t')


def main():
    p = sys.argv[1]
    d = open(p, 'rb').read()
    b = os.path.basename(p).lower()
    if b == 'buffeffect.bmd':
        buffeffect(d)
    elif b == 'pet.bmd':
        pet(d)
    elif b == 'mix.bmd':
        mix(d)
    elif b == 'masterskilltree.bmd':
        masterskilltree(d)
    elif b == 'skill.bmd' and len(d) == 2692:
        skill_darkav(d)
    else:
        sys.exit('not handled: ' + p)


if __name__ == '__main__':
    main()
```

## Open questions
- Skill: the meaning of Unk65/Unk66 (S3) and Unk67/Unk68/Unk70/Unk72/Unk76 (S4); the ReqGuildStatus
  semantics; the odd 0.99.28 Delay values; the 0.97.14 trailer.
- MasterSkillTree: the 9×5 header, the per-tree Unk0, and the meaning of the 10000×k flag and of -1 vs 0.
- MonsterSkill: the meaning of the 5 ids per monster; why 1.03.16 is a stream inside 48-byte records.
- BuffEffect: Unk56 and Unk57. Pet: Unk4, Unk8, the Action/Speed semantics. Mix: Unk44/48 (size?),
  Unk52, Unk56/Unk328 (letter codes), Unk324 (max rate?), and the token op codes above 7.
- Disassembling the 1.04.24 and 1.03.16 exes would settle most of these. That was not done here.
