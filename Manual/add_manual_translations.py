#!/usr/bin/env python3
"""
為所有模組 JSON 檔案添加 manual 的中文和日文翻譯
繁體中文保留合成器專有名詞使用英文
日文使用流暢自然的語感
"""

import json
import os

# HTML 樣式模板
H3_STYLE = 'style="color: #ff6b9d; margin-bottom: 0.8rem; border-bottom: 2px solid #2a2a4a; padding-bottom: 0.5rem;"'
P_STYLE = 'style="margin-bottom: 1rem;"'
UL_STYLE = 'style="padding-left: 1.5rem; margin-bottom: 1rem;"'
UL_STYLE_LAST = 'style="padding-left: 1.5rem;"'

def h3(text):
    return f'<h3 {H3_STYLE}>{text}</h3>'

def p(text):
    return f'<p {P_STYLE}>{text}</p>'

def ul(items, last=False):
    style = UL_STYLE_LAST if last else UL_STYLE
    li_items = '\n'.join(f'<li>{item}</li>' for item in items)
    return f'<ul {style}>\n{li_items}\n</ul>'

def b(text):
    return f'<b>{text}</b>'

# Manual 翻譯資料
MANUAL_TRANSLATIONS = {
    "maddy": {
        "manual_zh": f"""{h3("概述")}
{p("整合 Swing Clock 生成、3 軌 Euclidean Rhythm 生成器與 Pattern CV Sequencer 的節奏工作站。適合建立完整的節奏與旋律 Pattern。")}

{h3("功能")}
{ul([
    "可調 Swing 量的 Clock 生成器",
    "3 軌 Euclidean Rhythm 生成器（BD、SN、HH）",
    "16 步 Pattern CV Sequencer",
    "各軌獨立 Clock Division/Multiplication",
    "Pattern 變形與隨機化",
    "Sync 與 Reset 功能"
])}

{h3("Clock 區段")}
{ul([
    f"{b('BPM')}: Tempo 控制（20-300 BPM）",
    f"{b('SWING')}: Swing 量（0-100%）",
    f"{b('CLK SRC')}: Clock 來源選擇（Internal/External 模式）",
    f"{b('RESET')}: 重置所有序列"
])}

{h3("Euclidean 軌道（BD、SN、HH）")}
{ul([
    f"{b('FILL')}: 各軌節奏密度",
    f"{b('SHIFT')}: Pattern 旋轉偏移",
    f"{b('DIV')}: Clock Division（1、2、4、8）",
    f"{b('LEN')}: Pattern 長度（1-32 步）"
])}

{h3("CV Sequencer")}
{ul([
    f"{b('K1-K5')}: 五個 CV 電壓旋鈕（-10V 至 +10V）定義旋律調色盤",
    f"{b('MODE')}: 序列模式選擇（Sequential / Minimalism / Jump）",
    f"{b('DENSITY')}: 控制使用多少 K 旋鈕與序列長度",
    f"{b('CHAOS')}: 影響序列長度與步進映射的隨機量"
])}

{h3("輸出")}
{ul([
    "CLK: Clock Output",
    "BD、SN、HH: 獨立 Trigger Output",
    "CHAIN 12、23、123: 組合 Trigger Output",
    "CV: Sequencer CV Output",
    "TRIG: Sequencer Trigger Output"
])}

{h3("規格")}
{ul([
    "Clock: Internal BPM 或 External",
    "Output: 0-10V（Trigger）、+/-10V（CV）",
    "演算法: Bjorklund Euclidean 分佈"
])}

{h3("右鍵選單")}
{ul([
    f"{b('Attack Time')}: 調整 Envelope Attack 時間（0.5-20ms）",
    f"{b('Track Shift')}: 各軌 Shift 偏移（BD、SN、HH 各 0-4 步）"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("スウィングクロック生成、3トラックユークリッドリズム生成、パターンCVシーケンスを統合したリズムワークステーション。完全なリズム・メロディパターンの作成に適しています。")}

{h3("機能")}
{ul([
    "調整可能なスウィング量のクロック生成",
    "3トラックユークリッドリズム生成（BD、SN、HH）",
    "16ステップパターンCVシーケンサー",
    "トラック別クロック分割・倍増",
    "パターンモーフィングとランダム化",
    "同期・リセット機能"
])}

{h3("クロックセクション")}
{ul([
    f"{b('BPM')}: テンポ制御（20-300 BPM）",
    f"{b('SWING')}: スウィング量（0-100%）",
    f"{b('CLK SRC')}: クロックソース選択（内部/外部モード）",
    f"{b('RESET')}: 全シーケンスをリセット"
])}

{h3("ユークリッドトラック（BD、SN、HH）")}
{ul([
    f"{b('FILL')}: トラック別リズム密度",
    f"{b('SHIFT')}: パターン回転オフセット",
    f"{b('DIV')}: クロック分割（1、2、4、8）",
    f"{b('LEN')}: パターン長（1-32ステップ）"
])}

{h3("CVシーケンサー")}
{ul([
    f"{b('K1-K5')}: 5つのCV電圧ノブ（-10V〜+10V）でメロディパレットを定義",
    f"{b('MODE')}: シーケンスモード選択（Sequential / Minimalism / Jump）",
    f"{b('DENSITY')}: 使用するKノブ数とシーケンス長を制御",
    f"{b('CHAOS')}: シーケンス長とステップマッピングに影響するランダム量"
])}

{h3("出力")}
{ul([
    "CLK: クロック出力",
    "BD、SN、HH: 個別トリガー出力",
    "CHAIN 12、23、123: 組み合わせトリガー出力",
    "CV: シーケンサーCV出力",
    "TRIG: シーケンサートリガー出力"
])}

{h3("仕様")}
{ul([
    "クロック: 内部BPMまたは外部",
    "出力: 0-10V（トリガー）、+/-10V（CV）",
    "アルゴリズム: Bjorklundユークリッド分布"
])}

{h3("右クリックメニュー")}
{ul([
    f"{b('Attack Time')}: エンベロープアタック時間調整（0.5-20ms）",
    f"{b('Track Shift')}: トラック別シフトオフセット（BD、SN、HH各0-4ステップ）"
], last=True)}"""
    },

    "maddy_": {
        "manual_zh": f"""{h3("概述")}
{p("進階多通道 Sequencer，整合 Swing Clock 生成、3 軌 Euclidean Rhythm 生成器與三組獨立 Pattern CV Sequencer。MADDY 的擴展版本，具備額外 CV 通道。")}

{h3("功能")}
{ul([
    "可調 Swing 量的 Clock 生成器",
    "3 軌 Euclidean Rhythm 生成器（BD、SN、HH）",
    "三組獨立 16 步 CV Sequencer",
    "各軌獨立 Clock Division/Multiplication",
    "各 CV 通道獨立 Pattern 變形與隨機化",
    "Sync 與 Reset 功能"
])}

{h3("Clock 區段")}
{ul([
    f"{b('BPM')}: Tempo 控制（20-300 BPM）",
    f"{b('SWING')}: Swing 量（0-100%）",
    f"{b('CLK SRC')}: Clock 來源選擇（Internal/External 模式）",
    f"{b('RESET')}: 重置所有序列"
])}

{h3("CV 輸入")}
{ul([
    f"{b('CLK CV')}: Clock Rate CV 調變",
    f"{b('CH2 CV')}: CV2 通道調變輸入",
    f"{b('CH3 CV')}: CV3 通道調變輸入"
])}

{h3("Euclidean 軌道（BD、SN、HH）")}
{ul([
    f"{b('FILL')}: 各軌節奏密度",
    f"{b('SHIFT')}: Pattern 旋轉偏移",
    f"{b('DIV')}: Clock Division（1、2、4、8）",
    f"{b('LEN')}: Pattern 長度（1-32 步）"
])}

{h3("CV Sequencer（主通道）")}
{ul([
    f"{b('K1-K5')}: 五個 CV 電壓旋鈕（-10V 至 +10V）供所有 CV 通道共用",
    f"{b('MODE')}: 主通道序列模式選擇",
    f"{b('DENSITY')}: 控制使用多少 K 旋鈕與序列長度",
    f"{b('CHAOS')}: 影響所有通道的隨機量"
])}

{h3("額外 CV 通道（CV2、CV3）")}
{ul([
    f"{b('CH2/CH3 MODE')}: 各額外通道的獨立模式選擇",
    f"{b('DELAY')}: 各通道的步進延遲偏移",
    "各通道從相同的 K1-K5 電壓調色盤衍生 Pattern"
])}

{h3("輸出")}
{ul([
    "CLK: Clock Output",
    "RESET: Reset Output",
    "BD、SN、HH: 獨立 Trigger Output",
    "CHAIN 12、23、123: 組合 Trigger Output",
    "CV1、CV2、CV3: Sequencer CV Output",
    "TRIG1、TRIG2、TRIG3: Sequencer Trigger Output"
])}

{h3("規格")}
{ul([
    "Clock: Internal BPM 或 External",
    "Output: 0-10V（Trigger）、+/-10V（CV）",
    "CV 通道: 3 組獨立 Sequencer 共用電壓調色盤",
    "演算法: Bjorklund Euclidean 分佈"
])}

{h3("右鍵選單")}
{ul([
    f"{b('Attack Time')}: 調整 Envelope Attack 時間（0.5-20ms）",
    f"{b('Custom Pattern')}: 自訂步進 Pattern 輸入欄位",
    f"{b('Reset to Default')}: 重置自訂 Pattern 為預設值"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("スウィングクロック生成、3トラックユークリッドリズム生成、3系統の独立パターンCVシーケンサーを統合した高度なマルチチャンネルシーケンサー。追加CVチャンネルを備えたMADDYの拡張版。")}

{h3("機能")}
{ul([
    "調整可能なスウィング量のクロック生成",
    "3トラックユークリッドリズム生成（BD、SN、HH）",
    "3系統の独立16ステップCVシーケンサー",
    "トラック別クロック分割・倍増",
    "CVチャンネル別パターンモーフィングとランダム化",
    "同期・リセット機能"
])}

{h3("クロックセクション")}
{ul([
    f"{b('BPM')}: テンポ制御（20-300 BPM）",
    f"{b('SWING')}: スウィング量（0-100%）",
    f"{b('CLK SRC')}: クロックソース選択（内部/外部モード）",
    f"{b('RESET')}: 全シーケンスをリセット"
])}

{h3("CV入力")}
{ul([
    f"{b('CLK CV')}: クロックレートCV変調",
    f"{b('CH2 CV')}: CV2チャンネル変調入力",
    f"{b('CH3 CV')}: CV3チャンネル変調入力"
])}

{h3("ユークリッドトラック（BD、SN、HH）")}
{ul([
    f"{b('FILL')}: トラック別リズム密度",
    f"{b('SHIFT')}: パターン回転オフセット",
    f"{b('DIV')}: クロック分割（1、2、4、8）",
    f"{b('LEN')}: パターン長（1-32ステップ）"
])}

{h3("CVシーケンサー（メイン）")}
{ul([
    f"{b('K1-K5')}: 全CVチャンネル共有の5つのCV電圧ノブ（-10V〜+10V）",
    f"{b('MODE')}: メインチャンネルのシーケンスモード選択",
    f"{b('DENSITY')}: 使用するKノブ数とシーケンス長を制御",
    f"{b('CHAOS')}: 全チャンネルに影響するランダム量"
])}

{h3("追加CVチャンネル（CV2、CV3）")}
{ul([
    f"{b('CH2/CH3 MODE')}: 各追加チャンネルの独立モード選択",
    f"{b('DELAY')}: チャンネル別ステップ遅延オフセット",
    "各チャンネルは同じK1-K5電圧パレットからパターンを生成"
])}

{h3("出力")}
{ul([
    "CLK: クロック出力",
    "RESET: リセット出力",
    "BD、SN、HH: 個別トリガー出力",
    "CHAIN 12、23、123: 組み合わせトリガー出力",
    "CV1、CV2、CV3: シーケンサーCV出力",
    "TRIG1、TRIG2、TRIG3: シーケンサートリガー出力"
])}

{h3("仕様")}
{ul([
    "クロック: 内部BPMまたは外部",
    "出力: 0-10V（トリガー）、+/-10V（CV）",
    "CVチャンネル: 電圧パレット共有の3系統独立シーケンサー",
    "アルゴリズム: Bjorklundユークリッド分布"
])}

{h3("右クリックメニュー")}
{ul([
    f"{b('Attack Time')}: エンベロープアタック時間調整（0.5-20ms）",
    f"{b('Custom Pattern')}: カスタムステップパターン入力欄",
    f"{b('Reset to Default')}: カスタムパターンをデフォルトにリセット"
], last=True)}"""
    },

    "nigoq": {
        "manual_zh": f"""{h3("概述")}
{p("雙 Oscillator 合成器 Voice，具備 Through-zero Linear FM、Wavefolding、非對稱 Rectification 與整合雙軌 Oscilloscope。具備 Buchla 風格 Harmonic Principal Oscillator 與變形 Modulation Oscillator。")}

{h3("Oscillator")}
{ul([
    f"{b('MOD FREQ')}: Modulation Oscillator（0.001Hz-6kHz）",
    f"{b('FINAL FREQ')}: Principal Oscillator 含 2nd/3rd Harmonic（20Hz-8kHz）",
    f"{b('WAVE')}: Sine-Triangle-Saw-Pulse 變形，可變 Width",
    f"{b('SYNC')}: Off、Soft、Hard（FINAL 同步 MOD）",
    f"{b('EXT IN')}: 雙 Oscillator 的外部輸入"
])}

{h3("處理")}
{ul([
    f"{b('FM')}: Through-zero Linear FM（內部 0-4x、外部 10x）",
    f"{b('FOLD')}: Cosine-based Wavefolding 含 Harmonic 生成",
    f"{b('RECTIFY')}: 非對稱 Rectification 含 DC Blocking",
    f"{b('LPF')}: 雙極 Lowpass Filter（10Hz-20kHz）",
    f"{b('TM')}: 動態 Wavefolding 的 Timbre Modulation"
])}

{h3("Envelope 與 Output")}
{ul([
    f"{b('DECAY')}: AD Envelope（1ms Attack、0-3s Decay、>3s Drone 模式）",
    f"{b('BASS')}: Clean Sine Wave Mix 含 Soft Clipping"
])}

{h3("輸出")}
{ul([
    f"{b('MOD')}: Modulation 訊號（0-10V Unipolar 含 VCA）",
    f"{b('SINE')}: Clean Sine Output（+/-5V 含 VCA）",
    f"{b('FINAL')}: 處理後 Output 含 Bass 控制（+/-5V）"
])}

{h3("規格")}
{ul([
    "Input: +/-10V（CV/Audio）、9.5V Trigger",
    "Oversampling: 1x-32x 可選",
    "處理: 32-bit 含 PolyBLEP Anti-aliasing"
])}

{h3("右鍵選單")}
{ul([
    f"{b('2x Oversample')}: 切換 2x Oversampling 以提升音質"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("スルーゼロリニアFM、ウェーブフォールディング、非対称レクティファイ、統合デュアルトラックオシロスコープを備えたデュアルオシレーター合成ボイス。Buchlaスタイルのハーモニックプリンシパルオシレーターとモーフィングモジュレーションオシレーターを搭載。")}

{h3("オシレーター")}
{ul([
    f"{b('MOD FREQ')}: モジュレーションオシレーター（0.001Hz-6kHz）",
    f"{b('FINAL FREQ')}: 2nd/3rdハーモニクス付きプリンシパルオシレーター（20Hz-8kHz）",
    f"{b('WAVE')}: サイン-三角-ノコギリ-パルス間モーフィング、可変幅",
    f"{b('SYNC')}: Off、Soft、Hard（FINALがMODに同期）",
    f"{b('EXT IN')}: 両オシレーターへの外部入力"
])}

{h3("処理")}
{ul([
    f"{b('FM')}: スルーゼロリニアFM（内部0-4x、外部10x）",
    f"{b('FOLD')}: ハーモニクス生成付きコサインベースウェーブフォールディング",
    f"{b('RECTIFY')}: DCブロッキング付き非対称レクティファイ",
    f"{b('LPF')}: 2ポールローパスフィルター（10Hz-20kHz）",
    f"{b('TM')}: ダイナミックウェーブフォールディング用ティンバーモジュレーション"
])}

{h3("エンベロープと出力")}
{ul([
    f"{b('DECAY')}: ADエンベロープ（1msアタック、0-3sディケイ、>3sドローンモード）",
    f"{b('BASS')}: ソフトクリッピング付きクリーンサイン波ミックス"
])}

{h3("出力")}
{ul([
    f"{b('MOD')}: モジュレーション信号（VCA付き0-10Vユニポーラ）",
    f"{b('SINE')}: クリーンサイン出力（VCA付き+/-5V）",
    f"{b('FINAL')}: バス制御付き処理済み出力（+/-5V）"
])}

{h3("仕様")}
{ul([
    "入力: +/-10V（CV/オーディオ）、9.5Vトリガー",
    "オーバーサンプリング: 1x-32x選択可能",
    "処理: PolyBLEPアンチエイリアシング付き32ビット"
])}

{h3("右クリックメニュー")}
{ul([
    f"{b('2x Oversample')}: 音質向上のため2xオーバーサンプリングを切替"
], last=True)}"""
    },

    "weiii_documenta": {
        "manual_zh": f"""{h3("概述")}
{p("8 層錄音 Sampler，具備混沌參數變形、Feedback Matrix 與 Slice 引擎。結合即時錄音與自動 Transient 偵測、Polyphonic Playback、3 段 EQ、Sample and Hold Modulation 與參數變形。適合實驗性音效設計與現場演出。")}

{h3("錄音控制")}
{ul([
    f"{b('REC')}: 切換錄音（錄音時 LED 紅色）",
    f"{b('PLAY')}: 切換 Play/Loop 模式（綠色：前進至下一 Slice、紅色：Loop 當前 Slice）",
    f"{b('CLEAR')}: 短按 = 停止、長按 2 秒 = 清除 Buffer",
    f"{b('THRESHOLD')}: Transient 偵測閾值（0-10V）",
    f"{b('MIN SLICE')}: 最小 Slice 時間（0.001-1.0s）"
])}

{h3("播放控制")}
{ul([
    f"{b('SPEED')}: -8x 至 +8x（負值 = 反向）",
    f"{b('POLY')}: 1-8 聲，隨機 Slice 切換",
    f"{b('SCAN')}: 手動 Slice 選擇（0.0-1.0）",
    f"{b('LOOP END')}: 設定 Loop 終點（0.0-1.0）",
    f"{b('FEEDBACK')}: 類比風格飽和的 No-input Feedback"
])}

{h3("處理")}
{ul([
    f"{b('EQ')}: 3 段（80Hz Low Shelf、2.5kHz Mid Peak、12kHz High Shelf）+/-12dB",
    f"{b('S&H')}: Slew（0-1s）、Amount（0-5x）、Rate（0.01-100Hz）",
    f"{b('MORPH')}: 按住可將參數變形至隨機目標（可透過右鍵選單設定）"
])}

{h3("I/O")}
{ul([
    "Stereo Audio In/Out 含 Send/Return 供外部效果器",
    "Main L/R Stereo Output",
    "S&H CV Output（+/-10V）",
    "所有主要參數的 CV 輸入"
])}

{h3("規格")}
{ul([
    "I/O 範圍: +/-10V",
    "Output Limiter: -3dB Soft Limiter（7.07V 閾值）",
    "Slice Crossfade: 0.1ms Fade In/Out（最大 Oscillator 頻率約 5kHz）",
    "狀態儲存: Buffer 與 Slice 隨 Patch 保存"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("カオスパラメータモーフィング、フィードバックマトリクス、スライスエンジンを備えた8レイヤー録音サンプラー。リアルタイム録音と自動トランジェント検出、ポリフォニック再生、3バンドEQ、サンプル＆ホールドモジュレーション、パラメータモーフィングを統合。実験的サウンドデザインとライブパフォーマンス向け。")}

{h3("録音コントロール")}
{ul([
    f"{b('REC')}: 録音切替（録音中はLED赤点灯）",
    f"{b('PLAY')}: Play/Loopモード切替（緑：次のスライスへ進む、赤：現在のスライスをループ）",
    f"{b('CLEAR')}: 短押し = 停止、2秒長押し = バッファクリア",
    f"{b('THRESHOLD')}: トランジェント検出閾値（0-10V）",
    f"{b('MIN SLICE')}: 最小スライス時間（0.001-1.0s）"
])}

{h3("再生コントロール")}
{ul([
    f"{b('SPEED')}: -8x〜+8x（負値 = 逆再生）",
    f"{b('POLY')}: 1-8ボイス、ランダムスライス切替",
    f"{b('SCAN')}: 手動スライス選択（0.0-1.0）",
    f"{b('LOOP END')}: ループ終点設定（0.0-1.0）",
    f"{b('FEEDBACK')}: アナログスタイル飽和のノーインプットフィードバック"
])}

{h3("処理")}
{ul([
    f"{b('EQ')}: 3バンド（80Hz Low Shelf、2.5kHz Mid Peak、12kHz High Shelf）+/-12dB",
    f"{b('S&H')}: Slew（0-1s）、Amount（0-5x）、Rate（0.01-100Hz）",
    f"{b('MORPH')}: 長押しでパラメータをランダムターゲットへモーフィング（右クリックで設定可能）"
])}

{h3("I/O")}
{ul([
    "外部エフェクト用Send/Return付きステレオオーディオIn/Out",
    "メインL/Rステレオ出力",
    "S&H CV出力（+/-10V）",
    "全主要パラメータのCV入力"
])}

{h3("仕様")}
{ul([
    "I/O範囲: +/-10V",
    "出力リミッター: -3dBソフトリミッター（7.07V閾値）",
    "スライスクロスフェード: 0.1ms フェードイン/アウト（最大オシレーター周波数約5kHz）",
    "状態保存: バッファとスライスはパッチと共に保存"
], last=True)}"""
    },

    "universal_rhythm": {
        "manual_zh": f"""{h3("概述")}
{p("跨文化節奏生成器，具備 10 種世界音樂風格與 8 聲整合 Drum Synthesis。")}

{h3("風格")}
{p("West African、Afro-Cuban、Brazilian、Balkan、Indian、Gamelan、Jazz、Electronic、Breakbeat、Techno")}

{h3("4 角色系統（各 2 聲）")}
{ul([
    f"{b('Timeline')}: 計時（Bell、Hi-hat Pattern）",
    f"{b('Foundation')}: Bass 層（Kick、低音鼓）",
    f"{b('Groove')}: 中頻（Snare、Clap Pattern）",
    f"{b('Lead')}: 高頻裝飾音"
])}

{h3("各角色控制")}
{ul([
    f"{b('STYLE')}: 節奏風格（0-9）",
    f"{b('DENSITY')}: Pattern 密度（0-90%）",
    f"{b('LENGTH')}: Pattern 長度（4-32 步）",
    f"{b('FREQ/DECAY')}: 合成參數",
    f"{b('MIX')}: 各角色 Internal/External Mix（0-100%）",
    f"{b('EXT IN')}: 外部 Audio 輸入"
])}

{h3("全域參數")}
{ul([
    f"{b('VAR')}: Pattern 變化（0-100%）",
    f"{b('HUM')}: 時序/力度 Humanize（0-100%）",
    f"{b('SWG')}: Swing 量（0-100%）",
    f"{b('RST')}: 休止機率（0-100%）",
    f"{b('FILL')}: Fill 機率與強度（0-100%）",
    f"{b('REGENERATE')}: 重新生成所有 Pattern 的按鈕/Trigger"
])}

{h3("Articulation 控制")}
{ul([
    f"{b('ARTICULATION')}: Articulation 類型機率（0-100%）",
    f"{b('GHOST')}: Ghost Note 機率 - 在打擊間加入細微音符（0-100%）",
    f"{b('ACCENT')}: Accent 機率 - 強調特定打擊（0-100%）",
    f"{b('SPREAD')}: Stereo Spread 量（0-100%）"
])}

{h3("Articulation 類型")}
{ul([
    "8 種類型: Normal、Ghost、Accent、Rim、Flam、Drag、Buzz、Ruff",
    "7 種 Groove 模板: Auto、Straight、Swing、African、Latin、LaidBack、Pushed"
])}

{h3("輸出")}
{ul([
    "Stereo MIX L/R",
    "各聲: Audio、Gate、Pitch CV（1V/Oct、C4=0V）、Velocity CV"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("10種のワールドミュージックスタイルと8ボイス統合ドラム合成を備えたクロスカルチャーリズム生成器。")}

{h3("スタイル")}
{p("西アフリカ、アフロキューバン、ブラジリアン、バルカン、インド、ガムラン、ジャズ、エレクトロニック、ブレイクビート、テクノ")}

{h3("4ロールシステム（各2ボイス）")}
{ul([
    f"{b('Timeline')}: タイムキーピング（ベル、ハイハットパターン）",
    f"{b('Foundation')}: ベースレイヤー（キック、低音ドラム）",
    f"{b('Groove')}: 中域（スネア、クラップパターン）",
    f"{b('Lead')}: 高域装飾音"
])}

{h3("ロール別コントロール")}
{ul([
    f"{b('STYLE')}: リズムスタイル（0-9）",
    f"{b('DENSITY')}: パターン密度（0-90%）",
    f"{b('LENGTH')}: パターン長（4-32ステップ）",
    f"{b('FREQ/DECAY')}: 合成パラメータ",
    f"{b('MIX')}: ロール別内部/外部ミックス（0-100%）",
    f"{b('EXT IN')}: 外部オーディオ入力"
])}

{h3("グローバルパラメータ")}
{ul([
    f"{b('VAR')}: パターンバリエーション（0-100%）",
    f"{b('HUM')}: タイミング/ベロシティヒューマナイズ（0-100%）",
    f"{b('SWG')}: スウィング量（0-100%）",
    f"{b('RST')}: 休符確率（0-100%）",
    f"{b('FILL')}: フィル確率と強度（0-100%）",
    f"{b('REGENERATE')}: 全パターン再生成ボタン/トリガー"
])}

{h3("アーティキュレーションコントロール")}
{ul([
    f"{b('ARTICULATION')}: アーティキュレーションタイプ確率（0-100%）",
    f"{b('GHOST')}: ゴーストノート確率 - ヒット間に微細な音を追加（0-100%）",
    f"{b('ACCENT')}: アクセント確率 - 特定のヒットを強調（0-100%）",
    f"{b('SPREAD')}: ステレオスプレッド量（0-100%）"
])}

{h3("アーティキュレーションタイプ")}
{ul([
    "8タイプ: Normal、Ghost、Accent、Rim、Flam、Drag、Buzz、Ruff",
    "7グルーブテンプレート: Auto、Straight、Swing、African、Latin、LaidBack、Pushed"
])}

{h3("出力")}
{ul([
    "ステレオMIX L/R",
    "ボイス別: オーディオ、ゲート、ピッチCV（1V/Oct、C4=0V）、ベロシティCV"
], last=True)}"""
    },

    "twnc": {
        "manual_zh": f"""{h3("概述")}
{p("雙軌 Euclidean Rhythm 生成器，具備內建 Drum 與 Hi-hat 合成。結合節奏 Pattern 生成與 Envelope 生成器及 CV 調變。")}

{h3("功能")}
{ul([
    "兩軌獨立 Euclidean Rhythm",
    "內建 Drum 合成（Sine Oscillator + FM + Noise）",
    "內建 Hi-hat 合成（Noise + 頻率控制）",
    "全域 Clock 輸入與序列長度（1-32 步）",
    "Envelope 生成器含 Decay 與 Shape 控制",
    "Accent 系統含 VCA Shift"
])}

{h3("軌道 1（Drum）")}
{ul([
    f"{b('FILL')}: 活躍步進百分比（0-100%）",
    f"{b('FREQ')}: Oscillator 頻率（20Hz-20kHz）",
    f"{b('FM')}: 頻率調變量",
    f"{b('NOISE')}: Noise Mix 音量",
    f"{b('ACCENT')}: Accent Pattern 的 VCA Shift（1-7）",
    f"{b('DECAY/SHAPE')}: Envelope 控制"
])}

{h3("軌道 2（HATs）")}
{ul([
    f"{b('SHIFT')}: Pattern Shift 偏移（0-7 步）",
    f"{b('FILL')}: 活躍步進百分比",
    f"{b('DIV/MULT')}: Clock Division/Multiplication",
    f"{b('FREQ')}: Hi-hat 頻率",
    f"{b('DECAY/SHAPE')}: Envelope 控制"
])}

{h3("輸入")}
{ul([
    f"{b('CLK')}: 全域 Clock 輸入",
    f"{b('RESET')}: 重置序列至步進 1",
    f"{b('DRUM FREQ CV')}: Drum 頻率 CV 調變",
    f"{b('DRUM DECAY CV')}: Drum Decay CV 調變",
    f"{b('HATS FREQ CV')}: Hi-hat 頻率 CV 調變",
    f"{b('HATS DECAY CV')}: Hi-hat Decay CV 調變"
])}

{h3("輸出")}
{ul([
    f"{b('TRACK 1')}: Drum Audio Output",
    f"{b('TRACK 2')}: Hi-hat Audio Output",
    f"{b('VCA ENV')}: 主 VCA Envelope（0-10V）",
    f"{b('DRUM FM ENV')}: Drum FM Envelope（0-10V）",
    f"{b('HATS VCA ENV')}: Hi-hat VCA Envelope（0-10V）"
])}

{h3("規格")}
{ul([
    "Input: +/-10V（CV）、0-10V（Trigger）",
    "Output: +/-5V（Audio）、0-10V（Envelope）",
    "處理: 32-bit 浮點含 Oversampling",
    "演算法: Bjorklund Euclidean 分佈"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("内蔵ドラム・ハイハット合成を備えたデュアルトラックユークリッドリズム生成器。リズムパターン生成とエンベロープ生成器、CV変調を統合。")}

{h3("機能")}
{ul([
    "2トラック独立ユークリッドリズム",
    "内蔵ドラム合成（サインオシレーター + FM + ノイズ）",
    "内蔵ハイハット合成（ノイズ + 周波数制御）",
    "グローバルクロック入力とシーケンス長（1-32ステップ）",
    "ディケイ・シェイプ制御付きエンベロープ生成器",
    "VCAシフト付きアクセントシステム"
])}

{h3("トラック1（ドラム）")}
{ul([
    f"{b('FILL')}: アクティブステップ割合（0-100%）",
    f"{b('FREQ')}: オシレーター周波数（20Hz-20kHz）",
    f"{b('FM')}: 周波数変調量",
    f"{b('NOISE')}: ノイズミックスレベル",
    f"{b('ACCENT')}: アクセントパターンのVCAシフト（1-7）",
    f"{b('DECAY/SHAPE')}: エンベロープ制御"
])}

{h3("トラック2（ハイハット）")}
{ul([
    f"{b('SHIFT')}: パターンシフトオフセット（0-7ステップ）",
    f"{b('FILL')}: アクティブステップ割合",
    f"{b('DIV/MULT')}: クロック分割・倍増",
    f"{b('FREQ')}: ハイハット周波数",
    f"{b('DECAY/SHAPE')}: エンベロープ制御"
])}

{h3("入力")}
{ul([
    f"{b('CLK')}: グローバルクロック入力",
    f"{b('RESET')}: シーケンスをステップ1にリセット",
    f"{b('DRUM FREQ CV')}: ドラム周波数CV変調",
    f"{b('DRUM DECAY CV')}: ドラムディケイCV変調",
    f"{b('HATS FREQ CV')}: ハイハット周波数CV変調",
    f"{b('HATS DECAY CV')}: ハイハットディケイCV変調"
])}

{h3("出力")}
{ul([
    f"{b('TRACK 1')}: ドラムオーディオ出力",
    f"{b('TRACK 2')}: ハイハットオーディオ出力",
    f"{b('VCA ENV')}: メインVCAエンベロープ（0-10V）",
    f"{b('DRUM FM ENV')}: ドラムFMエンベロープ（0-10V）",
    f"{b('HATS VCA ENV')}: ハイハットVCAエンベロープ（0-10V）"
])}

{h3("仕様")}
{ul([
    "入力: +/-10V（CV）、0-10V（トリガー）",
    "出力: +/-5V（オーディオ）、0-10V（エンベロープ）",
    "処理: オーバーサンプリング付き32ビット浮動小数点",
    "アルゴリズム: Bjorklundユークリッド分布"
], last=True)}"""
    },

    "u8": {
        "manual_zh": f"""{h3("概述")}
{p("Stereo Mixer Channel 模組，具備 Level 控制、Sidechain Ducking 與 Mute 功能。設計用於 Mixer Bus 整合與 Chain 輸入。")}

{h3("功能")}
{ul([
    "Stereo 處理，自動 20ms Delay 用於 Mono-to-Stereo 轉換",
    "Level 控制含 CV 調變（0.0-2.0x Gain）",
    "Sidechain Ducking 含可調深度（0-3x 衰減）",
    "Mute 功能含 Trigger 輸入與 LED 指示",
    "Chain 輸入供 Mixer Bus 整合",
    "Bypass 功能維持 Chain 訊號流"
])}

{h3("控制")}
{ul([
    f"{b('LEVEL')}: 通道 Gain（0.0-2.0x）",
    f"{b('LEVEL CV')}: Level 調變輸入（0-10V）",
    f"{b('DUCK')}: Sidechain Ducking 量（0.0-1.0）",
    f"{b('MUTE')}: 通道 Mute 含 LED 指示",
    f"{b('MUTE TRIG')}: 外部 Mute Trigger 輸入"
])}

{h3("I/O")}
{ul([
    f"{b('LEFT/RIGHT')}: Stereo Audio 輸入（Mono 使用 Left 含 20ms Delay）",
    f"{b('CHAIN L/R')}: Mixer Chain 輸入",
    f"{b('OUT L/R')}: 處理後 Stereo Output"
])}

{h3("規格")}
{ul([
    "Input/Output: +/-10V（Audio）、+/-5V（CV）",
    "處理: 32-bit 浮點",
    "Delay Buffer: 2048 Samples（44.1kHz 下 20ms）"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("レベル制御、サイドチェインダッキング、ミュート機能を備えたステレオミキサーチャンネルモジュール。チェーン入力でミキサーバス統合向けに設計。")}

{h3("機能")}
{ul([
    "モノ→ステレオ変換用自動20msディレイ付きステレオ処理",
    "CV変調対応レベル制御（0.0-2.0xゲイン）",
    "調整可能な深度のサイドチェインダッキング（0-3x減衰）",
    "トリガー入力とLEDインジケーター付きミュート機能",
    "ミキサーバス統合用チェーン入力",
    "チェーン信号フローを維持するバイパス機能"
])}

{h3("コントロール")}
{ul([
    f"{b('LEVEL')}: チャンネルゲイン（0.0-2.0x）",
    f"{b('LEVEL CV')}: レベル変調入力（0-10V）",
    f"{b('DUCK')}: サイドチェインダッキング量（0.0-1.0）",
    f"{b('MUTE')}: LEDインジケーター付きチャンネルミュート",
    f"{b('MUTE TRIG')}: 外部ミュートトリガー入力"
])}

{h3("I/O")}
{ul([
    f"{b('LEFT/RIGHT')}: ステレオオーディオ入力（モノはLeft使用、20msディレイ付き）",
    f"{b('CHAIN L/R')}: ミキサーチェーン入力",
    f"{b('OUT L/R')}: 処理済みステレオ出力"
])}

{h3("仕様")}
{ul([
    "入出力: +/-10V（オーディオ）、+/-5V（CV）",
    "処理: 32ビット浮動小数点",
    "ディレイバッファ: 2048サンプル（44.1kHzで20ms）"
], last=True)}"""
    },

    "yamanote": {
        "manual_zh": f"""{h3("概述")}
{p("8 聲道 Stereo Send Mixer，具備雙 Aux Send Bus 與整合 Return 處理。U8 的配套模組，用於完整 Mixing 系統。")}

{h3("功能")}
{ul([
    "8 聲道 Stereo 輸入 Mixer 含各聲道獨立處理",
    "雙 Aux Send Bus（Send A 與 Send B）含獨立 Level 控制",
    "Send A 與 Send B 處理的 Stereo Return 輸入",
    "用於 Mixer 擴展的 Chain Input/Output",
    "各聲道自動 Mono-to-Stereo 轉換",
    "Mix Output 結合所有 Return 與 Chain 訊號"
])}

{h3("各聲道控制（CH1-CH8）")}
{ul([
    f"{b('L/R Input')}: Stereo 輸入（R 未接時使用 Left）",
    f"{b('Send A')}: 至 Aux Bus A 的 Send Level（0.0-1.0）",
    f"{b('Send B')}: 至 Aux Bus B 的 Send Level（0.0-1.0）"
])}

{h3("Send/Return 區段")}
{ul([
    f"{b('SEND A/B L/R')}: Aux Send Stereo Output",
    f"{b('RETURN A/B L/R')}: Aux Return Stereo Input"
])}

{h3("Master 區段")}
{ul([
    f"{b('CHAIN L/R')}: 用於 Mixer 擴展的 Chain Input",
    f"{b('MIX L/R')}: 最終 Stereo Mix Output"
])}

{h3("規格")}
{ul([
    "Input/Output: +/-10V",
    "處理: 32-bit 浮點",
    "聲道: 8 Stereo 輸入聲道",
    "Send Bus: 2 Stereo Aux Send",
    "Mix 架構: Post-return Summing"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("デュアルAuxセンドバスと統合リターン処理を備えた8チャンネルステレオセンドミキサー。総合ミキシングシステム用のU8コンパニオンモジュール。")}

{h3("機能")}
{ul([
    "チャンネル別処理付き8チャンネルステレオ入力ミキサー",
    "個別レベル制御付きデュアルAuxセンドバス（Send AとSend B）",
    "Send AとSend B処理用ステレオリターン入力",
    "ミキサー拡張用チェーン入出力",
    "チャンネル別自動モノ→ステレオ変換",
    "全リターンとチェーン信号を統合したミックス出力"
])}

{h3("チャンネル別コントロール（CH1-CH8）")}
{ul([
    f"{b('L/R Input')}: ステレオ入力（R未接続時はLeft使用）",
    f"{b('Send A')}: Auxバス Aへのセンドレベル（0.0-1.0）",
    f"{b('Send B')}: Auxバス Bへのセンドレベル（0.0-1.0）"
])}

{h3("Send/Returnセクション")}
{ul([
    f"{b('SEND A/B L/R')}: Auxセンドステレオ出力",
    f"{b('RETURN A/B L/R')}: Auxリターンステレオ入力"
])}

{h3("マスターセクション")}
{ul([
    f"{b('CHAIN L/R')}: ミキサー拡張用チェーン入力",
    f"{b('MIX L/R')}: 最終ステレオミックス出力"
])}

{h3("仕様")}
{ul([
    "入出力: +/-10V",
    "処理: 32ビット浮動小数点",
    "チャンネル: 8ステレオ入力チャンネル",
    "センドバス: 2ステレオAuxセンド",
    "ミックス構成: ポストリターンサミング"
], last=True)}"""
    },

    "ken": {
        "manual_zh": f"""{h3("概述")}
{p("3D 空間定位模組，具備 Binaural Panning 與 HRTF 處理。")}

{h3("功能")}
{ul([
    "3D 空間定位",
    "Binaural Panning",
    "HRTF 處理",
    "Azimuth 與 Elevation 控制",
    "Distance 衰減"
])}

{h3("控制")}
{ul([
    f"{b('AZIMUTH')}: 水平角度（-180° 至 +180°）",
    f"{b('ELEVATION')}: 仰角（-90° 至 +90°）",
    f"{b('DISTANCE')}: 距離衰減控制",
    f"{b('CV 輸入')}: 所有參數支援 CV 調變"
])}

{h3("I/O")}
{ul([
    "Mono Input",
    "Binaural Stereo Output"
])}

{h3("規格")}
{ul([
    "HRTF: MIT KEMAR 資料集",
    "處理: 32-bit 浮點"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("バイノーラルパンニングとHRTF処理を備えた3D空間定位モジュール。")}

{h3("機能")}
{ul([
    "3D空間定位",
    "バイノーラルパンニング",
    "HRTF処理",
    "方位角・仰角コントロール",
    "距離減衰"
])}

{h3("コントロール")}
{ul([
    f"{b('AZIMUTH')}: 水平角度（-180°〜+180°）",
    f"{b('ELEVATION')}: 仰角（-90°〜+90°）",
    f"{b('DISTANCE')}: 距離減衰コントロール",
    f"{b('CV入力')}: 全パラメータCV変調対応"
])}

{h3("I/O")}
{ul([
    "モノ入力",
    "バイノーラルステレオ出力"
])}

{h3("仕様")}
{ul([
    "HRTF: MIT KEMARデータセット",
    "処理: 32ビット浮動小数点"
], last=True)}"""
    },

    "pyramid": {
        "manual_zh": f"""{h3("概述")}
{p("四面體 3D Panner，具備 4 點空間定位與 Binaural Output。")}

{h3("功能")}
{ul([
    "四面體 3D Panning",
    "4 點空間定位",
    "Binaural Output",
    "XYZ 座標控制",
    "CV 調變"
])}

{h3("控制")}
{ul([
    f"{b('X')}: 左右位置",
    f"{b('Y')}: 前後位置",
    f"{b('Z')}: 上下位置",
    f"{b('CV 輸入')}: XYZ 各軸 CV 調變"
])}

{h3("I/O")}
{ul([
    "Mono Input",
    "4 聲道 Spatial Output",
    "Binaural Stereo Output"
])}

{h3("規格")}
{ul([
    "處理: 32-bit 浮點",
    "空間模型: 四面體"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("4点空間定位とバイノーラル出力を備えた四面体3Dパンナー。")}

{h3("機能")}
{ul([
    "四面体3Dパンニング",
    "4点空間定位",
    "バイノーラル出力",
    "XYZ座標コントロール",
    "CV変調"
])}

{h3("コントロール")}
{ul([
    f"{b('X')}: 左右位置",
    f"{b('Y')}: 前後位置",
    f"{b('Z')}: 上下位置",
    f"{b('CV入力')}: XYZ各軸CV変調"
])}

{h3("I/O")}
{ul([
    "モノ入力",
    "4チャンネル空間出力",
    "バイノーラルステレオ出力"
])}

{h3("仕様")}
{ul([
    "処理: 32ビット浮動小数点",
    "空間モデル: 四面体"
], last=True)}"""
    },

    "decapyramid": {
        "manual_zh": f"""{h3("概述")}
{p("10 聲道 3D Spatial Mixer，具備 Binaural 處理與視覺化 3D 定位顯示。")}

{h3("功能")}
{ul([
    "10 聲道 3D Spatial Mixer",
    "Binaural 處理",
    "3D 定位視覺化",
    "各聲道獨立空間位置",
    "Master Binaural Output"
])}

{h3("各聲道控制")}
{ul([
    f"{b('X')}: 左右位置",
    f"{b('Y')}: 前後位置",
    f"{b('Z')}: 上下位置",
    f"{b('LEVEL')}: 聲道音量"
])}

{h3("I/O")}
{ul([
    "10 Mono Input",
    "Binaural Stereo Output",
    "各聲道 CV 輸入"
])}

{h3("規格")}
{ul([
    "處理: 32-bit 浮點",
    "HRTF: MIT KEMAR 資料集",
    "視覺化: 即時 3D 顯示"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("バイノーラル処理と視覚的3D定位表示を備えた10チャンネル3D空間ミキサー。")}

{h3("機能")}
{ul([
    "10チャンネル3D空間ミキサー",
    "バイノーラル処理",
    "3D定位視覚化",
    "チャンネル別独立空間位置",
    "マスターバイノーラル出力"
])}

{h3("チャンネル別コントロール")}
{ul([
    f"{b('X')}: 左右位置",
    f"{b('Y')}: 前後位置",
    f"{b('Z')}: 上下位置",
    f"{b('LEVEL')}: チャンネル音量"
])}

{h3("I/O")}
{ul([
    "10モノ入力",
    "バイノーラルステレオ出力",
    "チャンネル別CV入力"
])}

{h3("仕様")}
{ul([
    "処理: 32ビット浮動小数点",
    "HRTF: MIT KEMARデータセット",
    "視覚化: リアルタイム3D表示"
], last=True)}"""
    },

    "quantizer": {
        "manual_zh": f"""{h3("概述")}
{p("CV Quantizer，將連續 CV 訊號量化至選定音階。")}

{h3("功能")}
{ul([
    "CV Quantizer",
    "多種音階選擇",
    "可調 Root Note",
    "Trigger Output",
    "Bypass 開關"
])}

{h3("控制")}
{ul([
    f"{b('SCALE')}: 音階選擇",
    f"{b('ROOT')}: Root Note（C-B）",
    f"{b('BYPASS')}: 繞過 Quantizer"
])}

{h3("I/O")}
{ul([
    "CV Input",
    "Quantized CV Output",
    "Trigger Output（音符變化時輸出）"
])}

{h3("規格")}
{ul([
    "Input/Output: +/-10V",
    "音階: Major、Minor、Pentatonic 等"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("連続CV信号を選択したスケールに量子化するCVクオンタイザー。")}

{h3("機能")}
{ul([
    "CVクオンタイザー",
    "複数スケール選択",
    "調整可能なルートノート",
    "トリガー出力",
    "バイパススイッチ"
])}

{h3("コントロール")}
{ul([
    f"{b('SCALE')}: スケール選択",
    f"{b('ROOT')}: ルートノート（C-B）",
    f"{b('BYPASS')}: クオンタイザーをバイパス"
])}

{h3("I/O")}
{ul([
    "CV入力",
    "量子化CV出力",
    "トリガー出力（ノート変化時に出力）"
])}

{h3("仕様")}
{ul([
    "入出力: +/-10V",
    "スケール: メジャー、マイナー、ペンタトニック等"
], last=True)}"""
    }
}

# 為其他模組添加簡化翻譯
SIMPLE_MODULES = {
    "twnc_2": {
        "manual_zh": f"""{h3("概述")}
{p("TWNC 進階版，具備擴展 Pattern 編輯功能與多種 Variation 模式。")}

{h3("功能")}
{ul([
    "擴展 Pattern 編輯",
    "多種 Variation 模式",
    "即時 Pattern 切換",
    "Pattern Chain 功能",
    "改進的 Trigger 與 Accent"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("拡張パターン編集と複数バリエーションモードを備えたTWNC拡張版。")}

{h3("機能")}
{ul([
    "拡張パターン編集",
    "複数バリエーションモード",
    "リアルタイムパターン切替",
    "パターンチェーン機能",
    "改良トリガーとアクセント"
], last=True)}"""
    },
    "twnc_light": {
        "manual_zh": f"""{h3("概述")}
{p("TWNC 精簡版，保留核心功能於更小的面板尺寸。")}

{h3("功能")}
{ul([
    "精簡 4 軌 Drum",
    "基本 Pattern Sequencer",
    "Swing 時序控制",
    "混合 Audio Output",
    "節省空間設計"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("コア機能をコンパクトパネルに凝縮したTWNCライト版。")}

{h3("機能")}
{ul([
    "コンパクト4トラックドラム",
    "基本パターンシーケンサー",
    "スウィングタイミング制御",
    "ミックスオーディオ出力",
    "省スペース設計"
], last=True)}"""
    },
    "kimo": {
        "manual_zh": f"""{h3("概述")}
{p("Pattern 型 Drum Machine，具備可程式化 Rhythm Pattern 與獨立 Drum Synthesis。")}

{h3("功能")}
{ul([
    "可程式化 Rhythm Pattern",
    "4 聲 Drum Synthesis",
    "各聲獨立 Accent 序列",
    "Pattern Fill 變化",
    "CV 調變輸入"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("プログラマブルリズムパターンと独立ドラム合成を備えたパターン型ドラムマシン。")}

{h3("機能")}
{ul([
    "プログラマブルリズムパターン",
    "4ボイスドラム合成",
    "ボイス別アクセントシーケンス",
    "パターンフィルバリエーション",
    "CV変調入力"
], last=True)}"""
    },
    "pinpple": {
        "manual_zh": f"""{h3("概述")}
{p("Percussion 合成器，具備多種打擊樂音色與可調參數。")}

{h3("功能")}
{ul([
    "多種 Percussion 音色",
    "可調 Decay 時間",
    "Tune 控制",
    "CV 調變輸入",
    "混合 Output"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("複数パーカッション音色と調整可能なパラメータを備えたパーカッション合成器。")}

{h3("機能")}
{ul([
    "複数パーカッション音色",
    "調整可能なディケイ",
    "チューンコントロール",
    "CV変調入力",
    "ミックス出力"
], last=True)}"""
    },
    "swing_lfo": {
        "manual_zh": f"""{h3("概述")}
{p("Swing 時序 LFO，具備可調 Swing 量與多種波形。")}

{h3("功能")}
{ul([
    "可調 Swing 時序",
    "多種 LFO 波形",
    "Clock 同步",
    "Rate CV 輸入",
    "雙 Output"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("調整可能なスウィング量と複数波形を備えたスウィングタイミングLFO。")}

{h3("機能")}
{ul([
    "調整可能なスウィングタイミング",
    "複数LFO波形",
    "クロック同期",
    "レートCV入力",
    "デュアル出力"
], last=True)}"""
    },
    "euclidean_rhythm": {
        "manual_zh": f"""{h3("概述")}
{p("Euclidean Rhythm 生成器，基於 Bjorklund 演算法。")}

{h3("功能")}
{ul([
    "Euclidean Algorithm 節奏",
    "可調 Steps 數量",
    "可調 Fills 密度",
    "Pattern Rotation",
    "多 Output"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("Bjorklundアルゴリズムに基づくユークリッドリズム生成器。")}

{h3("機能")}
{ul([
    "ユークリッドアルゴリズムリズム",
    "調整可能なステップ数",
    "調整可能なフィル密度",
    "パターンローテーション",
    "複数出力"
], last=True)}"""
    },
    "ad_generator": {
        "manual_zh": f"""{h3("概述")}
{p("Attack-Decay Envelope 生成器，具備可調時間與曲線形狀。")}

{h3("功能")}
{ul([
    "可調 Attack 時間",
    "可調 Decay 時間",
    "多種曲線形狀",
    "Trigger 與 Gate 輸入",
    "CV Output"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("調整可能な時間とカーブ形状を備えたADエンベロープ生成器。")}

{h3("機能")}
{ul([
    "調整可能なアタック時間",
    "調整可能なディケイ時間",
    "複数カーブ形状",
    "トリガー・ゲート入力",
    "CV出力"
], last=True)}"""
    },
    "q_q": {
        "manual_zh": f"""{h3("概述")}
{p("雙 Quantizer 模組，具備可選音階與 Root Note。")}

{h3("功能")}
{ul([
    "雙通道 Quantizer",
    "多種音階選擇",
    "可調 Root Note",
    "Trigger Output",
    "CV 直通"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("選択可能なスケールとルートノートを備えたデュアルクオンタイザー。")}

{h3("機能")}
{ul([
    "デュアルチャンネルクオンタイザー",
    "複数スケール選択",
    "調整可能なルートノート",
    "トリガー出力",
    "CVスルー"
], last=True)}"""
    },
    "ppattterning": {
        "manual_zh": f"""{h3("概述")}
{p("Pattern 生成器，具備可程式化序列與隨機化功能。")}

{h3("功能")}
{ul([
    "可程式化 Pattern 序列",
    "隨機化控制",
    "Clock 同步",
    "多 Output",
    "Pattern 儲存"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("プログラマブルシーケンスとランダム化機能を備えたパターン生成器。")}

{h3("機能")}
{ul([
    "プログラマブルパターンシーケンス",
    "ランダム化コントロール",
    "クロック同期",
    "複数出力",
    "パターン保存"
], last=True)}"""
    },
    "ellen_ripley": {
        "manual_zh": f"""{h3("概述")}
{p("訊號處理工具，具備 Waveshaping、Distortion 與動態處理。")}

{h3("功能")}
{ul([
    "多種 Waveshaping 演算法",
    "可調 Distortion",
    "動態處理",
    "Dry/Wet Mix",
    "CV 調變"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("ウェーブシェイピング、ディストーション、ダイナミクス処理を備えたシグナル処理ツール。")}

{h3("機能")}
{ul([
    "複数ウェーブシェイピングアルゴリズム",
    "調整可能なディストーション",
    "ダイナミクス処理",
    "ドライ/ウェットミックス",
    "CV変調"
], last=True)}"""
    },
    "env_vca_6": {
        "manual_zh": f"""{h3("概述")}
{p("6 聲道 Envelope 與 VCA 組合模組。")}

{h3("功能")}
{ul([
    "6 聲道 VCA",
    "各聲道 Envelope",
    "可調 Attack/Decay",
    "CV 調變",
    "混合 Output"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("6チャンネルエンベロープとVCAの複合モジュール。")}

{h3("機能")}
{ul([
    "6チャンネルVCA",
    "チャンネル別エンベロープ",
    "調整可能なアタック/ディケイ",
    "CV変調",
    "ミックス出力"
], last=True)}"""
    },
    "runshow": {
        "manual_zh": f"""{h3("概述")}
{p("演出控制模組，具備場景切換與自動化功能。")}

{h3("功能")}
{ul([
    "場景切換控制",
    "自動化序列",
    "Trigger 輸入/輸出",
    "場景儲存",
    "即時控制"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("シーン切替と自動化機能を備えたパフォーマンスコントロールモジュール。")}

{h3("機能")}
{ul([
    "シーン切替コントロール",
    "自動化シーケンス",
    "トリガー入出力",
    "シーン保存",
    "リアルタイムコントロール"
], last=True)}"""
    },
    "observer": {
        "manual_zh": f"""{h3("概述")}
{p("訊號監測模組，具備電平顯示與視覺化。")}

{h3("功能")}
{ul([
    "訊號電平監測",
    "視覺化顯示",
    "Peak Hold",
    "多通道輸入",
    "即時更新"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("レベル表示と視覚化を備えたシグナル監視モジュール。")}

{h3("機能")}
{ul([
    "シグナルレベル監視",
    "視覚化表示",
    "ピークホールド",
    "マルチチャンネル入力",
    "リアルタイム更新"
], last=True)}"""
    },
    "obserfour": {
        "manual_zh": f"""{h3("概述")}
{p("4 通道訊號監測模組，具備獨立電平顯示。")}

{h3("功能")}
{ul([
    "4 通道監測",
    "獨立電平顯示",
    "Peak Hold",
    "緊湊面板設計",
    "即時視覺化"
], last=True)}""",
        "manual_ja": f"""{h3("概要")}
{p("独立レベル表示を備えた4チャンネルシグナル監視モジュール。")}

{h3("機能")}
{ul([
    "4チャンネル監視",
    "独立レベル表示",
    "ピークホールド",
    "コンパクトパネル設計",
    "リアルタイム視覚化"
], last=True)}"""
    }
}

# 合併所有翻譯
ALL_TRANSLATIONS = {**MANUAL_TRANSLATIONS, **SIMPLE_MODULES}

def add_manual_translations():
    """為所有模組 JSON 檔案添加 manual 翻譯"""
    modules_dir = 'modules'

    for slug, trans in ALL_TRANSLATIONS.items():
        json_path = os.path.join(modules_dir, f'{slug}.json')

        if not os.path.exists(json_path):
            print(f"找不到: {json_path}")
            continue

        with open(json_path, 'r', encoding='utf-8') as f:
            module = json.load(f)

        # 添加 manual 翻譯
        if 'manual_zh' in trans:
            module['manual_zh'] = trans['manual_zh']
        if 'manual_ja' in trans:
            module['manual_ja'] = trans['manual_ja']

        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(module, f, ensure_ascii=False, indent=2)

        print(f"已更新 manual: {slug}")

if __name__ == '__main__':
    add_manual_translations()
    print("Manual 翻譯添加完成！")
