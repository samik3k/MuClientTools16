# items group — evidence and notes

> Research notes behind `../items.txt`: evidence for each field (decompiled loaders, value checks), byte-exact
> layouts and tested Python decoders. They were written against a reference decoder with a smaller schema syntax;
> where they call a format OPAQUE, "container" or FALLBACK, or propose an option (`header=count32cap`, `skip=`,
> `counts32`, `trailer=`, `crc_from=`, `probe=`, a magic before the count), MuLegacy now supports it and the
> schema file decodes that format exactly. Paths to the original working folders have been removed.

Schema: `items.txt`. Tables: Item, ItemSetType, ItemSetOption, ItemAddOption, JewelOfHarmonyOption,
JewelOfHarmonySmelt, SocketItem. Every file in the corpus decodes under `--check` (output at the end).

Sources used:
- 1.00.13 CHS decompile (`corpus.ngP6/tmp.6FDq/main.c`): Item loader `sub_52C7C0`, item builder at ~line 238240,
  ancient-stat tooltip `sub_40C0E0`, ItemSetType loader `sub_40AB40`, ItemSetOption loader `sub_40AD30`, set
  tooltips `sub_40D310` / `sub_40D0D0` / `sub_40D000`.
- 0.85 decompile (the 0.85 client's bundled decompilation): item.txt parser ~line 61195, bmd loader `sub_455E00`, item builder ~line 61430.
- Disassembly of the kayito mains 1.04.10, 1.04.24, 1.03.16 (`objdump`), used only to settle the 5- vs 6-class
  question for 84-byte Item.bmd.
- Cross-version value checks over every record.

Conventions: XOR key FC CF AB; `xor=record` restarts at each record, `xor=stream` runs over the whole data area.
Offsets are absolute bytes inside a record.

---------------------------------------------------------------------------------------------------------------------

## Item.bmd

| variant | size | versions | record | records | CRC |
|---|---|---|---|---|---|
| cls6_84 | 688132 | 1.04.24, 1.03.16 (GMO) | 84 | 8192 (16x512) | E2F1 |
| cls5_84 | 688132 | 1.02.44, 1.03.47, 1.04.10 | 84 | 8192 | E2F1 |
| s1_80 | 655364 | 1.01.46 | 80 | 8192 | E2F1 |
| s1_76 | 38916 | 0.99.28, 1.00.15, 1.01.02 | 76 | 512 (16x32) | E2F1 |
| darkav_68 | 69636 | 0.97.14-darkav | 68 (+4 trailer) | 1024 (16x64) | none, OPAQUE |
| s0_64 | 32772 | 0.97.04 | 64 | 512 | E2F1 |
| s0_55 | 14080 / 28160 | 0.68.02, 0.75.00 / 0.85.00 | 55 | 256 (16x16) / 512 | none |

Item id = record number (`group * per_group + index`).

### Field evidence

From the code (1.00.13 CHS, 76-byte record; the same fields line up in every other variant through the value checks
below):
- Loader `sub_52C7C0`: `fread(0x9800)` = 512 x 76, CRC with wkey 0xE2F1 (58097), per-record BuxConvert
  (`sub_52A5B0(rec, 76)`), copies into `dword_76E0C50`. It also keeps a second copy where byte `+41` is doubled
  (attack speed anti-hack copy). The 0.85 loader does the same with byte `+39`, which is AttackSpeed in the 55-byte record.
- Item builder: `+30` -> item.TwoHand, `+41` -> speed, `+36/+37` -> DamageMin/Max (excellent formula
  `25*x/Level+5` uses `+31` as Level), `+38` block, `+39` defense, `+40` magic defense, `+42` walk speed,
  `+45` magic power (same excellent formula). Requirements: `+46` u16 Str and `+48` u16 Dex (`3*x*(lvl+..)/100+20`),
  `+50` u16 Ene (`4*x*..`), `+52` u16 copied straight to the requirement slot, with item 926 special-cased as
  `15*x+185` (the Dark Horse command formula), so this is **ReqCommand**. `+54` u8 ReqLevel (`x + 4*itemLevel`,
  `+5` per level for the 2nd-level wings 387..390).
- `sub_40C0E0`: `switch(+60)` with cases 1..4, each printing one of four "+5*n stat" strings = the ancient set stat
  bonus. So **SetAttr** is 1 str, 2 agi, 3 ene, 4 vit. Data agrees: swords 1, bows 2, staffs 3, armor/shields 4,
  rings and pendants mixed, chaos and other non-ancient items 0.
- Other reads: `+34/+35` (width/height, inventory code), `+43` (durability, repair price), `+44` (magic durability),
  `+32` (equip slot of the picked-up item).
- 0.85 item.txt parser (55-byte struct): weapons read Level->31, DmgMin->34, DmgMax->35, Speed->39, Dur->41,
  Str->42, Dex->43, and set TwoHand(30)=1 when Width>=2. Shields: Block->36. Armor: Def->37, MagDef->38.
  Gloves: Speed->39. Boots: Walk->40. Group 12 (orbs/wings): ReqLevel->45, Energy->44. Group 13 (rings):
  Resist 51..54. Group 14: Value->46. Builder: 42/43 Str/Dex `3*x*../10+20`, 44 Energy `4*x*..`, 45 ReqLevel `+4*lvl`.

Value checks (all versions): Kris = Level 6, 1x2, DMG 6-11, speed 50, durability 20 (0.97+), Str 40 / Dex 40;
Wings of Elf/Fairy ReqLevel 180, 2nd wings 215; Bone Blade ReqLevel 380 (84 B); Twisting Slash orb Zen 29000,
Seeds 500000; Ring of Ice -> Resist[0], Ring of Poison [1], Pendant of Lightning [2], Pendant of Fire [3],
Ring of Earth [4], Ring of Wind [5], Pendant of Water [6]; Class columns: staffs only DW, bows only FE, scepters
only DL, Dark Breaker DK=2 (2nd class), Elemental Mace FE=2, Mystery Staff SU only (6-class). Slot: shield 1,
helm 2, armor 3, pants 4, gloves 5, boots 6, wings 7, pets 8, pendants 9, rings 10, orbs/jewels 255.
Skill: swords 19-23 (Falling Slash...Slash), 56 Power Slash, 66 Force (scepters), 49 Dinorant, 62 Dark Horse,
223-225 Summoner books.

Code pages: 0.68.02 KOR (cp949), 0.75.00 English, 0.85.00 **GBK** (the cp949 fallback prints garbage, use `--cp gbk`),
1.03.16 GMO English (cp1252). JewelOfHarmonySmelt names in 1.03.16 are Japanese (**cp932**).

### Variant differences
- 55 B (0.68-0.85): no slot/skill, u8 requirements (no command), Class[4] = DW DK FE MG (4th flag is set together
  with DW on staffs and with DK on swords = MG), Resist[4] = ice poison lightning fire. No Zen, MagicDur or MagicPower.
- 64 B (0.97.04): adds MagicDur(42), MagicPower(43), u16 Str/Dex, **u8 Energy at 48** and ReqLevel at 49 (wings 180
  there, so 48 cannot be a u16), Unk50 (always 0), Value 51, Zen u32 52, Class[4] 56, Resist[4] 60. No SetAttr
  (0.97.04 has no ItemSetType.bmd).
- 68 B (DaRKav): like 64 B but Energy is u16 (Nova Scroll 258, Megatherium Mace 192), ReqLevel 50, Value 51, Zen 52,
  SetAttr 56, Class[4] 57, Resist[7] 61.
- 76 B (0.99.28-1.01.02): adds Slot(32)/Skill(33), ReqCommand, Class[5] (DL), Resist[7].
- 80 B (1.01.46): ReqLevel becomes u16 (56), and an extra u16 appears between Energy and Command (**Unk52**, always 0).
- 84 B: Level becomes u16 (32, pad at 31), ReqLevel u16 (58), Value 60 + 3 pad, Zen 64, SetAttr 68.
  **Unk54** is the same always-zero u16 slot that appeared in 80 B (later clients call this slot "RequireVitality").
  I found no non-zero value and did not trace a reader, so it stays Unk.

### 84-byte: two layouts with the same size and CRC key (cannot be expressed in the schema language)
The mains differ:
- 1.04.10 (`0x5ad4a1`): `cmp byte [esi+ecx+0x45],1 ... cmp ecx,5` (class loop over 5) and resistance loop
  `cmp byte [esi+ebp+0x4A]` -> Class[5] at 69, Resist[7] at 74.
- 1.04.24 (`0x5a1261`) and 1.03.16 (`0x58d661`): class loop `cmp ecx,6`, resistance loop at `+0x4B` -> Class[6] at
  69 (SU at 74), Resist[7] at 75.

Both use the same size and CRC (E2F1), so legacy_ref always picks the first variant (`cls6_84`). The 1.02.44, 1.03.47
and 1.04.10 rows in `--check` are therefore decoded with the wrong layout for bytes 74..81. **A real decoder needs a
content discriminator**, tested on all five files:

```python
def item84_class_count(data):              # data = whole 688132-byte file
    rec = xor3(data[2581*84:2582*84])      # group 5 #21 = Book of Sahamutt (Summoner)
    return 6 if rec[0] and rec[74] == 1 else 5
# -> 1.02.44: 5, 1.03.47: 5, 1.04.10: 5, 1.04.24: 6, 1.03.16: 6
```
Equivalent: count records with byte 74 != 0. That count is 2-3 in 5-class files (resistance rings) and 78/205 in
6-class files. Suggested schema extension for the C++ tool: `probe=<record>:<offset>=<value>` on a variant.

Data quirk: the 1.04.24 Item.bmd was authored with the **5-class resistance offsets**. Its rings and pendants have their
resistance at 74+i (Ring of Ice: byte 74 = 1, byte 75 = 0; Lightning pendant: byte 76), while the 1.04.24 main reads
resistances at 75. Decoded with the correct (6-class) layout, the 1.04.24 Resist columns therefore come out shifted by
one. That is how the file is, not a decoder error. In 1.03.16 the resistances sit correctly at 75+.

Minor quirks: `TwoHand` is also 1 on armor, shields and wings in 0.97.04/1.00.x/1.01.x (data authoring, the client only
reads it for weapons). The "Ring of GM" has 255 in every resist byte, and in 5-class files also in Unk81.

### DaRKav 69636 (OPAQUE in the schema)
1024 x 68 records, then 4 bytes = `FC CF AB FC` = four zero bytes XOR'd with a restarted key. No CRC matches with any
wkey 0..FFFF (brute-forced in C). legacy_ref cannot skip a trailer, so the schema has a one-row raw fallback.
Tested decoder (`items_work/snip/snippets.py`):

```python
import struct
KEY = b'\xfc\xcf\xab'
def xor3(b): return bytes(c ^ KEY[i % 3] for i, c in enumerate(b))
DARKAV = struct.Struct('<30s14B3HBBI B4B7B')   # 68 bytes
COLS = ['Name','TwoHand','Level','Width','Height','DamageMin','DamageMax','SuccessfulBlocking','Defense',
        'MagicDefense','AttackSpeed','WalkSpeed','Durability','MagicDurability','MagicPower','ReqStrength',
        'ReqDexterity','ReqEnergy','ReqLevel','Value','Zen','SetAttr'] + \
       ['Class[%d]' % i for i in range(4)] + ['Resist[%d]' % i for i in range(7)]
def decode_darkav_item(data, cp='cp949'):
    assert len(data) == 69636 and xor3(data[-4:]) == b'\0\0\0\0'
    rows = []
    for k in range(1024):
        v = list(DARKAV.unpack(xor3(data[k*68:(k+1)*68])))
        v[0] = v[0].split(b'\0', 1)[0].decode(cp, 'replace')
        rows.append(dict(zip(COLS, v)))
    return rows
# rows[0]  = Kris 6 dmg 6-11 speed 50 dur 20 str 40 dex 40 SetAttr 1 classes 1,1,1,1
# rows[498]= Nova Scroll ReqEnergy 258 Zen 500000
```
Schema-language equivalent if the tool gains `trailer=4`:
`variant darkav_68 =69636 rec=68 trailer=4 xor=record` with fields Name str30, TwoHand..MagicPower u8 (14),
ReqStrength u16, ReqDexterity u16, ReqEnergy u16, ReqLevel u8, Value u8, Zen u32, SetAttr u8, Class u8*4, Resist u8*7.

---------------------------------------------------------------------------------------------------------------------

## ItemSetType.bmd
- 2052 = 512 x 4 + CRC E5F1 (0.97.14-darkav, 0.99.28, 1.00.15, 1.01.02); 32772 = 8192 x 4 + CRC E5F1 (1.01.46+).
- 1.00.13 loader `sub_40AB40`: `fread(0x800)`, CRC wkey 58865 = 0xE5F1, per-record BuxConvert of 4 bytes, 512 records.
- `sub_40D310`: `setIndex = *(a1 + 4 + 4*itemIndex + (ancientType-1))` with `ancientType = (item.byte28 % 4)`, then
  `option = a1 + 2052 + 76*setIndex`. So bytes 0/1 are the **ItemSetOption record index** for ancient type 1/2
  (`SetIndex[2]`). Values stay below 64; 255 or 0 means none, depending on the file.
- Bytes 2-3 (`Unk2`) are 0, or 255 on fully empty records. No reader found.

## ItemSetOption.bmd
- 4868 = 64 x 76 + CRC A2F1 (0.97.14 - 1.01.46); 6916 = 64 x 108 + CRC (1.02.44, 1.03.47, 1.04.10);
  6980 = 64 x 109 + CRC (1.03.16, 1.04.24).
- 1.00.13 loader `sub_40AD30`: `fread(0x1300)`, CRC wkey 41713 = 0xA2F1, 64 x 76 BuxConvert.
- Tooltip `sub_40D310` (76-byte record): name = `strlen(rec)`; `if (rec[60] >= 0xFF) invalid`; option loop
  `v8 = 0..12`. For v8 < 6 the options are `rec[32+2*v8]`, `rec[33+2*v8]` with values `rec[44+2*v8]`, `rec[45+2*v8]`
  (TierOption/TierValue as 6 pairs). For v8 = 6,7 the option is `rec[50+v8]` (56,57) and the value `rec[52+v8]`
  (58,59) (ExtraOption/ExtraValue). For v8 = 8..12 the option is `rec[53+v8]` (61..65) and the value `rec[58+v8]`
  (66..70). These last five are drawn as active only on the full-set branch (`v29 = ... ? 13 : -1`), so they are
  FullOption/FullValue. 255 = no option.
- 108/109-byte records = the same 44/45-byte body behind a **64-byte name** (byte-identical bodies for the same sets).
- `Unk60`/`Unk92`: 255 marks an unused record (the code checks this). On every used 1.00.15 set it equals the number
  of ItemSetType references to the set + 1 (Warrior 7 -> 8, Hyperion 3 -> 4, ...), but no reader for its value was found.
- `Class` (last 5 or 6 bytes): all 0 except 1.04.10 (all 1). Named from structure only: its width follows the class
  count (5 in 108-byte files, 6 in 109-byte files, matching the Item.bmd class split exactly). No code reader traced.

## ItemAddOption.bmd (no CRC, `xor=stream`)
- 81920 = 8192 x 10 (1.02.44, 1.03.47), 98304 = 8192 x 12 (1.04.10), 131072 = 8192 x 16 (1.03.16, 1.04.24).
- Stream XOR proven: with per-record XOR, 10- and 16-byte records would break, while stream decoding gives clean
  records everywhere.
- u16 fields: Option1, Value1, Option2, Value2, ItemIndex. ItemIndex == record % 512 in all 30/30/118/70/44 used
  records. Used by the 380-level items (weapons (2,200)+(1,10), helms (8,20)+(3,10), armor (7,0)+(3,10), ...),
  pets, seals, scrolls.
- 16-byte: u16 Unk10 (always 0), u32 **Time** at 12: 1800 (Jack O'Lantern scrolls = 30-min buffs), 1200, 3600, and
  864000 (= 10 days) on "Leap of Strength/Agility/...".
- 12-byte: u16 Time at 10 (1800 on the same Jack O'Lantern scrolls).
- Option ids are an enum that is not decoded here. They are left numeric.

## JewelOfHarmonyOption.bmd (5400 = 30 x 180, no CRC, record XOR, which equals stream since 180 % 3 == 0)
- u32 Index, str60 Name, u32 MinLevel, u32 Value[14], u32 Zen[14]. Verified on all 30 records: Value and Zen are 0
  for item levels below MinLevel (MinLevel 0/3/6/9/13) and non-zero from MinLevel on. Records 0-9 are weapon options
  ("Min. attack increase"...), 10-19 staff options ("Wizardry increase"...), 20-29 armor options ("Defense
  increase"...). Unused slots have Index = 0xFFFFFFFF.
- "Zen" (100000..500000 per level) is named from its magnitude. The exact use (refine or restore cost) was not traced.

## JewelOfHarmonySmelt.bmd (557056 = 8192 x 68, no CRC, `xor=stream`; 68 % 3 != 0 and stream gives clean data)
- u32 ItemId (== record number = group*512+index on all 85 used records), str60 Name, u32 Unk64 (= 4 on every used
  record). Names are Japanese/cp932 in 1.03.16 and English in the KOR builds.

## SocketItem.bmd (1.03.16 only, 15600 = 150 x 104, no CRC, record XOR)
- Three 50-record tables: 0-49 seed options (Index 0..49, Element 1..6), 50-99 bonus-socket options (Element 0),
  100-149 socket set options.
- `Element` order 1..6 = fire, water, ice, wind, lightning, earth, the same order as the "Seed (Fire)..Seed (Earth)"
  items 12/60..65 in Item.bmd. Matches the option names (ice -> life on monster kill, lightning -> excellent damage,
  earth -> health).
- `ValueType`: 1 flat (+Max AG 25, crit damage +30), 2 percent (damage reduction 4, excellent rate 10, max life 4),
  3 per character level ("(Level type)", value 20), 4/5 on monster kill.
- `Unk80 u32*4` (400x4 on the level-type option, 49..52 on the kill options, 1x4 elsewhere, 0 in the bonus/set tables)
  and `Unk96 u8*8` are left unnamed. Unk96 is non-zero only on set records 100/101 (`01 01 01 01 01 01` and
  `01 02 00 03 01 02`) and is probably a per-element requirement count, but that is not verified.

---------------------------------------------------------------------------------------------------------------------

## Open questions
1. Item 84 B: the schema language needs a content probe (see the discriminator above) to choose cls5_84 vs cls6_84.
2. DaRKav Item: needs `trailer=4` support, or a dedicated decoder (snippet above).
3. Unnamed slots: Item Unk52/Unk54 (vitality slot?), Unk50 (0.97.04), Unk81 (5-class); ItemSetType Unk2;
   ItemSetOption Unk60/Unk92 and the Class semantics; JoH Smelt Unk64; SocketItem Unk80/Unk96.
4. The Option-id enums (ItemSetOption, ItemAddOption) map to text-table entries. They are not resolved here.

## --check output (items tables)
```
ok Item.s0_55         256 rows  0.68.02-patch/Item.bmd
ok Item.s0_55         256 rows  0.75.00/Data/Item/item.bmd
ok Item.s0_55         512 rows  0.85.00/Data/Item/item.bmd
ok Item.s0_64         512 rows  0.97.04/Data/Local/item.bmd
ok Item.darkav_68       1 rows  0.97.14-darkav/Data/Local/item.bmd      (opaque fallback, see notes)
ok ItemSetOption.s1             64 rows  0.97.14-darkav/Data/Local/itemsetoption.bmd
ok ItemSetType.s1_512        512 rows  0.97.14-darkav/Data/Local/itemsettype.bmd
ok Item.s1_76         512 rows  0.99.28/Data/Local/Item.bmd
ok ItemSetOption.s1             64 rows  0.99.28/Data/Local/itemsetoption.bmd
ok ItemSetType.s1_512        512 rows  0.99.28/Data/Local/itemsettype.bmd
ok Item.s1_76         512 rows  1.00.15/Data/Local/item.bmd
ok ItemSetOption.s1             64 rows  1.00.15/Data/Local/itemsetoption.bmd
ok ItemSetType.s1_512        512 rows  1.00.15/Data/Local/itemsettype.bmd
ok Item.s1_76         512 rows  1.01.02/Data/Local/item.bmd
ok ItemSetOption.s1             64 rows  1.01.02/Data/Local/itemsetoption.bmd
ok ItemSetType.s1_512        512 rows  1.01.02/Data/Local/itemsettype.bmd
ok ItemSetOption.s1             64 rows  1.01.46/Data/Local/ItemSetOption.bmd
ok ItemSetType.s2_8192      8192 rows  1.01.46/Data/Local/ItemSetType.bmd
ok Item.s1_80        8192 rows  1.01.46/Data/Local/item.bmd
ok Item.cls6_84      8192 rows  1.02.44/Data/Local/Item.bmd             (really cls5_84, see discriminator)
ok ItemAddOption.s2_10        8192 rows  1.02.44/Data/Local/ItemAddOption.bmd
ok ItemSetOption.cls5           64 rows  1.02.44/Data/Local/ItemSetOption.bmd
ok ItemSetType.s2_8192      8192 rows  1.02.44/Data/Local/ItemSetType.bmd
ok JewelOfHarmonyOption.s2             30 rows  1.02.44/Data/Local/JewelOfHarmonyOption.bmd
ok JewelOfHarmonySmelt.s2           8192 rows  1.02.44/Data/Local/JewelOfHarmonySmelt.bmd
ok Item.cls6_84      8192 rows  1.03.16/Data/Local/Item.bmd
ok ItemAddOption.s3_16        8192 rows  1.03.16/Data/Local/ItemAddOption.bmd
ok ItemSetOption.cls6           64 rows  1.03.16/Data/Local/ItemSetOption.bmd
ok ItemSetType.s2_8192      8192 rows  1.03.16/Data/Local/ItemSetType.bmd
ok JewelOfHarmonyOption.s2             30 rows  1.03.16/Data/Local/JewelOfHarmonyOption.bmd
ok JewelOfHarmonySmelt.s2           8192 rows  1.03.16/Data/Local/JewelOfHarmonySmelt.bmd
ok SocketItem.s4            150 rows  1.03.16/Data/Local/SocketItem.bmd
ok Item.cls6_84      8192 rows  1.03.47/Data/Local/Item.bmd             (really cls5_84)
ok ItemAddOption.s2_10        8192 rows  1.03.47/Data/Local/ItemAddOption.bmd
ok ItemSetOption.cls5           64 rows  1.03.47/Data/Local/ItemSetOption.bmd
ok ItemSetType.s2_8192      8192 rows  1.03.47/Data/Local/ItemSetType.bmd
ok JewelOfHarmonyOption.s2             30 rows  1.03.47/Data/Local/JewelOfHarmonyOption.bmd
ok JewelOfHarmonySmelt.s2           8192 rows  1.03.47/Data/Local/JewelOfHarmonySmelt.bmd
ok Item.cls6_84      8192 rows  1.04.10/Data/Local/Item.bmd             (really cls5_84)
ok ItemAddOption.s3_12        8192 rows  1.04.10/Data/Local/ItemAddOption.bmd
ok ItemSetOption.cls5           64 rows  1.04.10/Data/Local/ItemSetOption.bmd
ok ItemSetType.s2_8192      8192 rows  1.04.10/Data/Local/ItemSetType.bmd
ok JewelOfHarmonyOption.s2             30 rows  1.04.10/Data/Local/JewelOfHarmonyOption.bmd
ok JewelOfHarmonySmelt.s2           8192 rows  1.04.10/Data/Local/JewelOfHarmonySmelt.bmd
ok JewelOfHarmonyOption.s2             30 rows  1.04.24/Data/Local/JewelOfHarmonyOption.bmd
ok JewelOfHarmonySmelt.s2           8192 rows  1.04.24/Data/Local/JewelOfHarmonySmelt.bmd
ok Item.cls6_84      8192 rows  1.04.24/Data/Local/item.bmd
ok ItemAddOption.s3_16        8192 rows  1.04.24/Data/Local/itemaddoption.bmd
ok ItemSetOption.cls6           64 rows  1.04.24/Data/Local/itemsetoption.bmd
ok ItemSetType.s2_8192      8192 rows  1.04.24/Data/Local/itemsettype.bmd
```
