# WorldRhythm 音色庫 - 最終確認清單

## 研究結論

經過詳細搜尋，**真正可以合法重新分發（預打包）的 CC0/Public Domain 世界音樂打擊樂音色非常有限**。

大多數「免費」音色實際上是：
- **Royalty-free**（免版稅）- 可用於作品，但不可重新分發
- **Free to use**（免費使用）- 授權條款不明確
- **CC-BY**（需署名）- 可用但必須署名

只有 **CC0** 和 **Public Domain** 可以無條件預打包分發。

---

## 確認可預打包的音色來源

### 等級 A：確定 CC0/PD，高品質

| 來源 | 授權 | 內容 | 下載連結 | 品質評價 |
|------|------|------|----------|----------|
| **Virtuosity Drums** | CC0-1.0 | 完整鼓組（kick, snare, toms, cymbals） | [GitHub](https://github.com/sfzinstruments/virtuosity_drums) | 專業品質，Versilian Studios 製作 |
| **Gogodze Phu Vol I** | CC0-1.0 | 迦納 Bobobo 鼓 + Cajon | [sfzinstruments](https://sfzinstruments.github.io/percussion/) | 真實西非鼓錄音 |
| **Casa da Música Gamelan** | Artistic License 2.0 | 完整爪哇甘美朗（Pelog/Slendro） | [GitHub](https://github.com/Digitopia/CDM-GAMELAN-SAMPLE-LIBRARY) | 專業博物館級錄音 |
| **Dustyroom Drums** | CC0 | 504 電子鼓音色 | [Producer Spot](https://www.producerspot.com/500-free-drum-samples-created-by-dustyroom/) | 24-bit WAV，合成鼓 |
| **SM Drums** | Public Domain | 深度取樣鼓組 | [sfzinstruments](https://sfzinstruments.github.io/drums/sm_drums/) | 多速度層、多 round robin |

### 等級 B：Freesound CC0 個別音色（需手動收集）

| 音色 | Freesound 連結 | 說明 |
|------|---------------|------|
| **巴西嘉年華打擊** | [dacarpe/334469](https://freesound.org/people/dacarpe/sounds/334469/) | CC0, 包含 Surdo + Cuíca + Tamborim |
| **電子鼓 CC0** | [deadrobotmusic/32405](https://freesound.org/people/deadrobotmusic/packs/32405/) | CC0 snare 單擊 |

### 等級 C：Pixabay License（類 CC0，可商用無需署名）

| 內容 | 連結 |
|------|------|
| Gamelan 音效 | [Pixabay Gamelan](https://pixabay.com/sound-effects/search/gamelan/) |
| Gong 音效 | [Pixabay Gong](https://pixabay.com/sound-effects/search/gong/) |
| Djembe 音效 | [Pixabay Djembe](https://pixabay.com/sound-effects/search/djembe/) |

---

## 無法找到 CC0 音色的風格

| 風格 | 缺少的樂器 | 現況 |
|------|-----------|------|
| **古巴** | Clave, Conga, Bongo, Timbales | 無 CC0 專業錄音 |
| **巴西** | Agogo, Repinique | 僅有一個 CC0 loop |
| **巴爾幹** | Tapan, Tarabuka, Darbuka | 完全無 CC0 音色 |
| **印度** | Tabla (完整發音組) | 無專業 CC0 音色 |
| **爵士刷鼓** | Brush snare/swirl | CC-BY 有，CC0 無 |
| **808/909** | 原始機器音色 | 授權不明（hyperreal.org 未標示） |
| **Amen Break** | 原始錄音 | **法律上仍有版權**，並非真正 PD |

---

## 重要法律澄清

### Amen Break 不是公有領域

搜尋結果確認：
- 原始錄音版權屬於 The Winstons / 唱片公司
- 「廣泛使用」不等於「公有領域」
- 從未有正式訴訟 ≠ 合法
- **建議：不要預打包原始 Amen Break**

### 808/909 音色授權不明

- machines.hyperreal.org 未標示授權
- 大多數「免費 808/909」是 royalty-free，不可重新分發
- Roland 未釋出官方 CC0 版本

---

## 建議的實作策略

### 策略 A：最小可行音色庫（推薦）

只使用 100% 確認可分發的音色：

```
WorldRhythm_Samples/
├── LICENSE.txt
├── CREDITS.txt
│
├── General/                    # Virtuosity Drums (CC0)
│   ├── kick.wav
│   ├── snare.wav
│   ├── hihat_closed.wav
│   ├── hihat_open.wav
│   ├── tom_high.wav
│   ├── tom_mid.wav
│   ├── tom_low.wav
│   ├── crash.wav
│   └── ride.wav
│
├── WestAfrican/               # Gogodze Phu (CC0)
│   ├── bobobo_1.wav
│   ├── bobobo_2.wav
│   ├── bobobo_3.wav
│   ├── bobobo_4.wav
│   ├── bobobo_5.wav
│   └── cajon.wav
│
├── Gamelan/                   # Casa da Música (Artistic License 2.0)
│   ├── gong_ageng.wav
│   ├── kenong.wav
│   ├── bonang_panerus.wav
│   ├── bonang_barung.wav
│   ├── saron_pelog.wav
│   ├── saron_slendro.wav
│   └── gender.wav
│
└── Electronic/                # Dustyroom (CC0)
    ├── kick_synth_01.wav
    ├── kick_synth_02.wav
    ├── snare_synth_01.wav
    ├── clap_01.wav
    ├── hihat_closed_01.wav
    └── hihat_open_01.wav
```

**覆蓋風格：4/10**
- 西非 (Gogodze Phu)
- 甘美朗 (Casa da Música)
- 爵士/搖滾 (Virtuosity Drums)
- 電子 (Dustyroom)

### 策略 B：使用者自備音色

對於無 CC0 音色的風格，模組顯示：
```
"No sample loaded for Afro-Cuban style.
 Please load your own samples or download from:
 - Freesound.org (filter by CC0)
 - Your own recordings"
```

### 策略 C：合成替代

使用程式合成模擬某些音色：
- 808/909 kick/snare/hat（這些本來就是合成音色）
- Clave（木塊撞擊音）
- 基本打擊音效

---

## 下載腳本

```bash
#!/bin/bash
# download_cc0_samples.sh

mkdir -p WorldRhythm_Samples/{General,WestAfrican,Gamelan,Electronic}

# 1. Virtuosity Drums
git clone https://github.com/sfzinstruments/virtuosity_drums.git temp_virtuosity
# 提取 WAV 檔案...

# 2. Gogodze Phu
# 從 sfzinstruments 下載...

# 3. Casa da Música Gamelan
git clone https://github.com/Digitopia/CDM-GAMELAN-SAMPLE-LIBRARY.git temp_gamelan
# 提取 WAV 檔案...

# 4. Dustyroom
# 從 Producer Spot 手動下載（需登入）

echo "Done! Please verify licenses before distribution."
```

---

## 最終建議

**誠實面對現實**：真正可以合法預打包的世界音樂打擊樂 CC0 音色非常有限。

**建議做法**：

1. **預打包確認的 4 個風格**（西非、甘美朗、通用鼓組、電子）
2. **其他風格提供「推薦音色」連結**，讓使用者自行下載
3. **設計良好的音色載入介面**，方便使用者載入自己的音色
4. **考慮自行錄製**並以 CC0 發布，填補市場空缺

這樣做的好處：
- 100% 合法
- 不會有授權爭議
- 使用者可以選擇自己喜歡的音色
- 長期可持續

---

## 確認的下載連結

| 音色庫 | 直接下載連結 |
|-------|-------------|
| Virtuosity Drums | `git clone https://github.com/sfzinstruments/virtuosity_drums.git` |
| Gogodze Phu | 透過 sfzinstruments 頁面下載 |
| Casa da Música Gamelan | `git clone https://github.com/Digitopia/CDM-GAMELAN-SAMPLE-LIBRARY.git` |
| Dustyroom | [Producer Spot](https://www.producerspot.com/500-free-drum-samples-created-by-dustyroom/) (需登入) |
| Freesound CC0 巴西打擊 | [直接連結](https://freesound.org/people/dacarpe/sounds/334469/) |
