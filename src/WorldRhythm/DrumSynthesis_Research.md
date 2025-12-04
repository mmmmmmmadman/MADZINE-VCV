# WorldRhythm 合成音色研究

## 結論：合成方法完全可行

使用演算法合成打擊樂音色是**完全可行且無版權問題**的方案。以下是研究成果整理。

---

## 開源合成引擎參考

### 1. Geonkick (GPL-3.0)
- **GitHub**: [Geonkick-Synthesizer/geonkick](https://github.com/Geonkick-Synthesizer/geonkick)
- **語言**: C++ (77.8%), C (19.6%)
- **功能**: kicks, snares, claps, hi-hats, shakers
- **合成方法**:
  - Subtractive (振盪器 + 濾波器 + 包絡)
  - FM (OSC1→OSC2)
  - Distortion
- **波形**: sine, square, triangle, sawtooth, noise (White/Brownian)

### 2. Chow Kick (GPL-3.0)
- **GitHub**: [Chowdhury-DSP/ChowKick](https://github.com/Chowdhury-DSP/ChowKick)
- **語言**: C++ (JUCE)
- **功能**: Kick drum 專用
- **合成方法**: 物理建模老式鼓機電路
  - Pulse Shaper (可調脈衝)
  - Resonant Filter (多種非線性模式)
  - Noise Generator

### 3. STK - Synthesis ToolKit (MIT-like)
- **GitHub**: [thestk/stk](https://github.com/thestk/stk)
- **語言**: C++
- **功能**: 完整合成工具箱，包含打擊樂
- **包含**: Bass, Snare, Toms, Hi-Hat, Crash, Ride, Cowbell, Tambourine

### 4. Faust synths.lib (LGPL)
- **文件**: [faustlibraries.grame.fr/libs/synths](https://faustlibraries.grame.fr/libs/synths/)
- **功能**:
  - `kick(pitch, click, attack, decay, drive, gate)` - 音高掃頻正弦波
  - `clap(tone, attack, decay, gate)` - 濾波白噪音
  - `hat(pitch, tone, attack, decay, gate)` - 相位調變

### 5. SVModular DrumKit (GPL-3.0)
- **GitHub**: [SVModular/DrumKit](https://github.com/SVModular/DrumKit)
- **語言**: C++ (VCV Rack)
- **功能**: VCV Rack 專用鼓模組

---

## 各種鼓音色的合成演算法

### Kick Drum (大鼓)

**方法 A: 音高掃頻正弦波 (最常用)**
```
1. 正弦振盪器，起始頻率 150-300 Hz
2. 音高包絡快速下降到 40-80 Hz
3. 振幅包絡: 快攻擊 (<5ms), 中等衰減 (100-500ms)
4. 可選: 加入少量諧波失真
```

**方法 B: 共振濾波器**
```
1. 短脈衝觸發共振濾波器
2. 濾波器設在略低於自激振盪點
3. 脈衝使濾波器「鳴響」
```

**808 風格參數**:
- 起始頻率: ~160 Hz
- 結束頻率: ~50 Hz
- 衰減時間: 250-500 ms
- 波形: 純正弦波

### Snare Drum (小鼓)

**方法: TR-909 電路模擬**
```
1. 兩個三角波振盪器 (模擬鼓膜震動模態)
   - OSC1: ~180 Hz
   - OSC2: ~330 Hz
2. 白噪音通過 lowpass → highpass 濾波鏈 (模擬響弦)
3. 振盪器和噪音分別有獨立包絡
4. 混合輸出
```

**參數建議**:
- 音調部分: 160-200 Hz，快速衰減 (~50ms)
- 噪音部分: BP 濾波器 1-4 kHz，衰減 100-200ms
- 混合比例: 30-50% 音調，50-70% 噪音

### Hi-Hat (踩鈸)

**方法 A: 相位調變**
```
1. 正弦振盪器，頻率 300-400 Hz
2. 相位調變產生金屬質感
3. 高通濾波器 (>5 kHz)
4. 包絡控制開/閉:
   - Closed: 衰減 20-50 ms
   - Open: 衰減 200-500 ms
```

**方法 B: 多振盪器 (TR-808 風格)**
```
1. 6 個方波振盪器，非諧波頻率
2. 混合後通過兩個帶通濾波器
3. 三個不同時間常數的「門控」電路
4. 三個高通濾波器
```

### Clap (拍掌)

```
1. 白噪音源
2. 帶通濾波器 (1-3 kHz)
3. 4 個快速脈衝觸發，間隔約 11ms (模擬殘響)
4. 整體包絡: 快攻擊，中等衰減
```

### Clave/Woodblock (響棒/木魚)

```
1. 正弦或三角波振盪器
2. 頻率: 800-2500 Hz
3. 極短衰減包絡 (30-80 ms)
4. 可選: 輕微帶通濾波增加木質感
```

### Conga/Bongo (康加鼓/邦哥鼓)

**方法: 膜振動模態合成**
```
1. 多個正弦振盪器，頻率比例接近圓膜模態:
   - 基頻 f0 (如 200 Hz)
   - 1.59 × f0
   - 2.14 × f0
   - 2.30 × f0
2. 各模態獨立衰減 (高頻衰減更快)
3. 音高包絡: 起始略高，快速下滑
4. Slap 音色: 加入高頻噪音成分
```

### Tom (通通鼓)

```
1. 類似 Kick，但頻率更高
2. 基頻: 80-300 Hz (依大小)
3. 音高下滑幅度較小
4. 衰減時間: 200-400 ms
```

### Bell/Gong (鈴/鑼) - 甘美朗風格

**方法 A: 加法合成**
```
1. 多個正弦振盪器，非諧波頻率關係
2. 典型比例: 1.0, 2.0, 3.0, 4.2, 5.4, 6.8...
3. 各部分獨立衰減包絡
4. 長衰減時間 (2-10 秒)
5. 可選: LFO 調製產生「波動」效果
```

**方法 B: FM 合成**
```
1. 載波/調變頻率比為非整數
2. 調變指數控制「金屬感」
3. 包絡同時作用於振幅和調變指數
```

**方法 C: 環形調變**
```
1. 兩個振盪器相乘
2. 產生和頻與差頻
3. "3 諧波 × 3 諧波 = 18 成分"
```

### Tabla (塔布拉鼓) - 印度風格

```
1. 基頻振盪器 (Baya: 60-100 Hz, Daya: 200-400 Hz)
2. 多個諧波模態
3. 獨特的「彎音」效果: 起始音高高，快速下滑
4. 不同擊打位置產生不同音色:
   - Ge: 低沉，長衰減
   - Na: 清脆，短衰減
   - Tin: 高音，金屬質感
```

---

## 風格→合成方法映射

| 風格 | Timeline | Foundation | Groove | Lead |
|------|----------|------------|--------|------|
| **西非** | Bell (FM) | 膜合成低音 | 膜合成中高音 | 膜合成 + 噪音 |
| **古巴** | Clave (正弦短衰減) | Conga 低音 | Conga/Bongo | Timbales (金屬) |
| **巴西** | Bell (雙音) | 膜合成低音 | 膜合成高音 | 膜合成 |
| **巴爾幹** | 金屬敲擊 | 膜合成 | 膜合成 | 手鼓 |
| **印度** | Manjira (FM 鈴) | Tabla Baya | Tabla Daya | Tabla 特殊 |
| **甘美朗** | Kenong (FM) | Gong (加法) | Bonang (FM) | Gender (加法) |
| **爵士** | Ride (噪音+濾波) | Kick | Snare (噪音) | Hi-Hat |
| **電子** | Hi-Hat | Kick (音高掃頻) | Clap/Snare | 效果音 |
| **碎拍** | Hi-Hat | Kick | Snare | 切片效果 |
| **Techno** | Hi-Hat | Kick 909 | Clap | Rim |

---

## 實作建議

### Phase 1: 核心合成引擎

```cpp
class DrumSynthEngine {
    // 基礎振盪器
    float sineOsc(float phase);
    float triOsc(float phase);
    float noiseWhite();
    float noiseBrown();

    // 包絡
    float envAD(float attack, float decay, float time);
    float envADSR(...);

    // 濾波器
    float lowpass(float input, float cutoff, float resonance);
    float highpass(float input, float cutoff);
    float bandpass(float input, float center, float bandwidth);

    // 合成器
    float synthKick(float pitch, float decay, float click);
    float synthSnare(float tone, float snap, float decay);
    float synthHiHat(float tone, bool open);
    float synthClap(float tone, float decay);
    float synthMembrane(float pitch, float tension, float decay);
    float synthBell(float pitch, float inharmonicity, float decay);
};
```

### Phase 2: 風格預設

每個風格定義一組合成參數：

```cpp
struct StyleSynthPreset {
    // Timeline 音色
    DrumSynthParams timeline;
    // Foundation 音色
    DrumSynthParams foundation;
    // Groove 音色
    DrumSynthParams groove;
    // Lead 音色
    DrumSynthParams lead;
};

// 預設範例
StyleSynthPreset westAfricanPreset = {
    .timeline = {.type = BELL_FM, .pitch = 800, .decay = 0.3},
    .foundation = {.type = MEMBRANE, .pitch = 100, .decay = 0.4},
    .groove = {.type = MEMBRANE, .pitch = 250, .decay = 0.2},
    .lead = {.type = MEMBRANE_SLAP, .pitch = 400, .decay = 0.15}
};
```

### Phase 3: 參數調整 UI

讓使用者微調每個音色的參數，或選擇完全自訂。

---

## 優勢

1. **零版權問題** - 演算法生成，完全原創
2. **無限變化** - 參數可調，每次略有不同
3. **檔案極小** - 不需要儲存音色檔案
4. **即時調整** - 可以根據 BPM、力度即時變化
5. **教育價值** - 使用者可以學習合成原理

---

## 參考資源

- [McGill Percussion Synthesis](https://cim.mcgill.ca/~clark/nordmodularbook/nm_percussion.html)
- [Stanford CCRMA Drum Synthesis](https://ccrma.stanford.edu/~sdill/220A-project/drums.html)
- [Geonkick Source Code](https://github.com/Geonkick-Synthesizer/geonkick)
- [Chow Kick Source Code](https://github.com/Chowdhury-DSP/ChowKick)
- [Faust synths.lib](https://faustlibraries.grame.fr/libs/synths/)
- [STK Synthesis ToolKit](https://github.com/thestk/stk)
