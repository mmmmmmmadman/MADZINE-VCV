# WorldRhythm 預打包音色庫規劃

## 授權策略

為確保合法分發，本音色庫**僅採用以下授權**：
- **CC0 (Creative Commons Zero)** - 公有領域，無任何限制
- **Public Domain** - 公有領域
- **CC-BY** - 需署名（將在模組文件中統一署名）

---

## 可用 CC0/PD 音色來源

### 1. 主要來源

| 來源 | 網址 | 授權 | 說明 |
|------|------|------|------|
| **VCSL** | [github.com/sgossner/VCSL](https://github.com/sgossner/VCSL) | CC0 | Versilian Community Sample Library，包含打擊樂器 |
| **sfzinstruments** | [github.com/sfzinstruments](https://github.com/sfzinstruments) | CC0 | 多種 CC0 打擊樂器 |
| **Virtuosity Drums** | sfzinstruments repo | CC0-1.0 | 完整鼓組 |
| **SM Drums** | sfzinstruments repo | Public Domain | 深度取樣鼓組 |
| **Pixabay** | [pixabay.com/sound-effects](https://pixabay.com/sound-effects/) | Pixabay License (類CC0) | 免版稅、無需署名 |
| **Producer Space** | [producerspace.com](https://producerspace.com/) | CC0 | 2000+ 音色 |

### 2. Freesound CC0 音色

需手動篩選 CC0 授權的音色：

| 搜尋關鍵字 | Freesound 連結 |
|-----------|---------------|
| Djembe | [freesound.org/search/?q=djembe&f=license:"CC0"](https://freesound.org/search/?q=djembe&f=license:%22Creative+Commons+0%22) |
| Conga | [freesound.org/search/?q=conga&f=license:"CC0"](https://freesound.org/search/?q=conga&f=license:%22Creative+Commons+0%22) |
| Bongo | [freesound.org/search/?q=bongo&f=license:"CC0"](https://freesound.org/search/?q=bongo&f=license:%22Creative+Commons+0%22) |
| Gamelan | [freesound.org/search/?q=gamelan&f=license:"CC0"](https://freesound.org/search/?q=gamelan&f=license:%22Creative+Commons+0%22) |
| Tabla | [freesound.org/search/?q=tabla&f=license:"CC0"](https://freesound.org/search/?q=tabla&f=license:%22Creative+Commons+0%22) |

---

## 風格-音色映射表

### 每個風格需要的樂器

| 風格 | Timeline | Foundation | Groove | Lead |
|------|----------|------------|--------|------|
| **西非** | Iron Bell (Gankogui) | Djembe Bass | Djembe Tone/Slap | Djembe Solo |
| **古巴** | Clave | Conga Tumba | Conga/Bongo | Timbales |
| **巴西** | Agogo | Surdo | Tamborim | Repinique |
| **巴爾幹** | Tarabuka Rim | Tapan | Tarabuka | Darbuka |
| **印度** | Manjira | Tabla Baya | Tabla Daya | Tabla Roll |
| **甘美朗** | Kenong | Gong | Bonang | Gender |
| **爵士** | Ride Cymbal | Kick | Snare Brush | Hi-Hat |
| **電子** | Hi-Hat | Kick 808 | Clap/Snare | Perc FX |
| **碎拍** | Hi-Hat | Kick (Amen) | Snare (Amen) | Break Slice |
| **Techno** | Hi-Hat | Kick 909 | Clap | Rim |

---

## 建議的音色庫結構

```
WorldRhythm_Samples/
├── LICENSE.txt                    # 所有音色授權說明
├── CREDITS.txt                    # CC-BY 音色署名
│
├── WestAfrican/
│   ├── bell_high.wav             # Gankogui high
│   ├── bell_low.wav              # Gankogui low
│   ├── djembe_bass.wav           # Djembe bass (低音)
│   ├── djembe_tone.wav           # Djembe tone (中音)
│   ├── djembe_slap.wav           # Djembe slap (高音)
│   └── shaker.wav                # Shekere
│
├── AfroCuban/
│   ├── clave_son.wav             # Son clave hit
│   ├── conga_low.wav             # Tumba (低音 conga)
│   ├── conga_open.wav            # Conga open tone
│   ├── conga_slap.wav            # Conga slap
│   ├── bongo_high.wav            # Bongo macho
│   ├── bongo_low.wav             # Bongo hembra
│   └── cowbell.wav               # Campana
│
├── Brazilian/
│   ├── agogo_high.wav            # Agogo high
│   ├── agogo_low.wav             # Agogo low
│   ├── surdo_open.wav            # Surdo open
│   ├── surdo_muted.wav           # Surdo muted
│   ├── tamborim.wav              # Tamborim
│   └── cuica.wav                 # Cuica (optional)
│
├── Balkan/
│   ├── tapan_bass.wav            # Tapan bass
│   ├── tapan_rim.wav             # Tapan rim
│   ├── tarabuka_doum.wav         # Tarabuka doum
│   ├── tarabuka_tek.wav          # Tarabuka tek
│   └── tarabuka_ka.wav           # Tarabuka ka
│
├── Indian/
│   ├── tabla_ge.wav              # Tabla Ge (baya)
│   ├── tabla_na.wav              # Tabla Na (daya)
│   ├── tabla_tin.wav             # Tabla Tin
│   ├── tabla_tun.wav             # Tabla Tun
│   └── manjira.wav               # Manjira (小鈸)
│
├── Gamelan/
│   ├── gong_ageng.wav            # 大鑼
│   ├── kenong.wav                # Kenong
│   ├── bonang_high.wav           # Bonang panerus
│   ├── bonang_low.wav            # Bonang barung
│   └── gender.wav                # Gender
│
├── Jazz/
│   ├── kick_jazz.wav             # Jazz kick
│   ├── snare_brush_hit.wav       # Brush hit
│   ├── snare_brush_swirl.wav     # Brush swirl
│   ├── ride_tip.wav              # Ride tip
│   ├── ride_bell.wav             # Ride bell
│   └── hihat_foot.wav            # Hi-hat foot
│
├── Electronic/
│   ├── kick_808.wav              # TR-808 kick
│   ├── snare_808.wav             # TR-808 snare
│   ├── clap_808.wav              # TR-808 clap
│   ├── hihat_closed_808.wav      # TR-808 closed hat
│   ├── hihat_open_808.wav        # TR-808 open hat
│   └── cowbell_808.wav           # TR-808 cowbell
│
├── Breakbeat/
│   ├── kick_amen.wav             # Amen break kick
│   ├── snare_amen.wav            # Amen break snare
│   ├── hihat_amen.wav            # Amen break hat
│   └── ghost_amen.wav            # Amen ghost note
│
└── Techno/
    ├── kick_909.wav              # TR-909 kick
    ├── snare_909.wav             # TR-909 snare
    ├── clap_909.wav              # TR-909 clap
    ├── hihat_closed_909.wav      # TR-909 closed hat
    ├── hihat_open_909.wav        # TR-909 open hat
    └── rim_909.wav               # TR-909 rim
```

---

## 具體可下載的 CC0 音色

### 已確認 CC0 的來源

#### 1. Virtuosity Drums (CC0-1.0)
- 來源：[sfzinstruments/virtuosity_drums](https://github.com/sfzinstruments/virtuosity_drums)
- 包含：完整爵士/搖滾鼓組
- 適用風格：Jazz, Breakbeat

#### 2. SM Drums (Public Domain)
- 來源：sfzinstruments
- 包含：深度取樣鼓組
- 適用風格：多種

#### 3. Dustyroom Fake Acoustic Drums (CC0)
- 來源：[Producer Spot](https://www.producerspot.com/500-free-drum-samples-created-by-dustyroom/)
- 包含：504 音色 (kicks, snares, hats, toms, shakers)
- 適用風格：Electronic, Techno

#### 4. Pixabay Gamelan (Pixabay License)
- 來源：[Pixabay Gamelan](https://pixabay.com/sound-effects/search/gamelan/)
- 包含：Gong, Gamelan loops
- 適用風格：Gamelan

#### 5. Amen Break (Public Domain)
- 來源：[Sample Focus](https://samplefocus.com/tag/amen-break)
- 說明：原始 Amen Break 已被認定為事實上的公有領域
- 適用風格：Breakbeat

---

## 缺口分析

以下風格的 CC0 音色**較難找到**，需要替代方案：

| 風格 | 缺少樂器 | 替代方案 |
|------|----------|----------|
| **西非** | Djembe, Gankogui | 使用 Freesound CC0 搜尋，或合成模擬 |
| **古巴** | Clave, Conga, Timbales | Freesound CC0 搜尋 |
| **巴西** | Surdo, Tamborim, Agogo | Freesound CC0 搜尋 |
| **巴爾幹** | Tapan, Tarabuka | 可能需要自行錄製或使用通用打擊 |
| **印度** | Tabla | Freesound CC0 搜尋，或使用 Tablaradio (需確認授權) |

---

## 實作步驟

### Phase 1：收集確定可用的 CC0 音色
1. 下載 Virtuosity Drums (Jazz/General)
2. 下載 Dustyroom 504 samples (Electronic/Techno)
3. 下載 Pixabay Gamelan sounds
4. 收集 Amen Break samples

### Phase 2：搜尋 Freesound CC0 音色
1. 系統性搜尋每種樂器的 CC0 版本
2. 記錄每個音色的來源與授權
3. 建立 CREDITS.txt

### Phase 3：填補缺口
1. 使用通用打擊音色替代
2. 考慮合成模擬某些音色
3. 或標記為「需使用者自行提供」

### Phase 4：整合到模組
1. 設計音色載入機制
2. 建立風格→音色自動對應
3. 提供使用者自訂音色的選項

---

## 授權聲明模板

```
WorldRhythm Sample Library
==========================

This sample library is compiled for use with the WorldRhythm VCV Rack module.
All samples are released under permissive open-source licenses.

CC0 / Public Domain Samples:
- Virtuosity Drums by Versilian Studios & Karoryfer Samples (CC0-1.0)
- SM Drums (Public Domain)
- Dustyroom Fake Acoustic Drums (CC0)
- Amen Break samples (Public Domain)
- Pixabay sound effects (Pixabay License - similar to CC0)

For detailed credits, see CREDITS.txt

These samples may be freely used, modified, and redistributed
for any purpose, including commercial use, without attribution required.
```

---

## 下一步行動

1. **你要我直接下載並整理這些音色嗎？**
   - 我可以提供具體的下載連結和整理腳本

2. **你想要我先建立一個最小可用版本？**
   - 只用現有確定 CC0 的音色（約 4-5 個風格）

3. **你有現成的錄音設備嗎？**
   - 對於缺口較大的風格，可以考慮自行錄製並以 CC0 發布
