#!/usr/bin/env python3
"""
Update manual_zh and manual_ja to match detailed English manuals
Traditional Chinese keeps synthesizer terms in English
Japanese uses natural, fluent language
"""

import json
import os

# HTML style templates
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

# Detailed translations matching English manuals
DETAILED_TRANSLATIONS = {
    "runshow": {
        "manual_zh": f"""{h3("概述")}
{p("與 jan0ritter 共同開發的演出計時與 Performance Sequencer。結合實時 Clock 計數、多重計時系統與可變長度 Bar Sequencing 及 Morphable Waveform 生成。專為現場演出計時設計，具備視覺進度追蹤與精確 Trigger 生成。")}

{h3("Transport 控制")}
{ul([
    f"{b('Start/Stop')}: 手動或 CV 控制（Gate/Trigger）",
    f"{b('Reset')}: 手動或 CV 控制以重啟所有計時",
    f"{b('Clock Input')}: 外部 16 分音符 Clock（4 個 Clock = 1 拍）"
])}

{h3("Timer 控制")}
{ul([
    f"{b('Pulse Width')}: 全域 Pulse Width（1-99%）",
    f"{b('Waveform')}: 在 5 種形狀間 Morph - Ramp Up、Triangle、Saw Down、Sine、Pulse",
    f"{b('Bar 1-4')}: 獨立 Bar 長度設定（各 1-16 Clock）"
])}

{h3("視覺顯示")}
{ul([
    "Time Code: M:SS:CC（分:秒:百分秒）",
    "位置: BBB:B:T（小節:拍:Tick）",
    "Beat 指示 LED 含自動 Decay",
    "6 段進度條（5 分鐘 Timer、1 分鐘 Timer、Bar 1-4）"
])}

{h3("輸出")}
{ul([
    f"{b('5Min Timer')}: 30 分鐘內每 5 分鐘觸發",
    f"{b('1Min Timer')}: 15 分鐘內每 1 分鐘觸發",
    f"{b('Bar 1-4')}: Morphable Waveform Output（0-10V）"
])}

{h3("規格")}
{ul([
    "Clock Threshold: >1V",
    "CV Input: ±10V 範圍",
    "Output 範圍: 0-10V",
    "Bar 長度: 每 Bar 1-16 Clock（整數步進）",
    "處理: 32-bit 浮點計時"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("jan0ritterと共同開発したショータイミングとパフォーマンスシーケンサー。リアルタイムクロックカウント、複数タイマーシステム、可変長バーシーケンスとモーファブル波形生成を統合。視覚的進行追跡と正確なトリガー生成によるライブパフォーマンスタイミング向けに設計。")}

{h3("トランスポートコントロール")}
{ul([
    f"{b('Start/Stop')}: 手動またはCV制御（ゲート/トリガー）",
    f"{b('Reset')}: 全タイミングを再開する手動またはCV制御",
    f"{b('Clock Input')}: 外部16分音符クロック（4クロック = 1拍）"
])}

{h3("タイマーコントロール")}
{ul([
    f"{b('Pulse Width')}: 全出力のグローバルパルス幅（1-99%）",
    f"{b('Waveform')}: 5形状間モーフィング - Ramp Up、Triangle、Saw Down、Sine、Pulse",
    f"{b('Bar 1-4')}: 独立バー長設定（各1-16クロック）"
])}

{h3("視覚表示")}
{ul([
    "タイムコード: M:SS:CC（分:秒:センチ秒）",
    "位置: BBB:B:T（小節:拍:ティック）",
    "自動ディケイ付きビートインジケーターLED",
    "6セグメント進行バー（5分タイマー、1分タイマー、Bar 1-4）"
])}

{h3("出力")}
{ul([
    f"{b('5Min Timer')}: 30分間で5分毎にトリガー",
    f"{b('1Min Timer')}: 15分間で1分毎にトリガー",
    f"{b('Bar 1-4')}: モーファブル波形出力（0-10V）"
])}

{h3("仕様")}
{ul([
    "クロック閾値: >1V",
    "CV入力: ±10V範囲",
    "出力範囲: 0-10V",
    "バー長: バー毎1-16クロック（整数ステップ）",
    "処理: 32ビット浮動小数点タイミング"
], last=True)}"""
    },

    "observer": {
        "manual_zh": f"""{h3("概述")}
{p("8 軌 Waveform Oscilloscope 用於視覺訊號監測。各軌自動採用所連接 Cable 的顏色，方便同時識別與比較多個訊號。")}

{h3("控制")}
{ul([
    f"{b('Time Scale')}: 在顯示區點擊拖曳調整（每畫面 5ms - 50ms）",
    f"{b('Trigger LED')}: 點擊切換 - 白色: 自由運行、粉色: 同步至第一個連接的輸入"
])}

{h3("輸入")}
{ul([
    "Track 1-4: 上排（4x2 格局）",
    "Track 5-8: 下排",
    "第一個連接的輸入作為 Trigger 來源"
])}

{h3("規格")}
{ul([
    "Input 範圍: ±10V",
    "Time Scale: 每畫面 5ms - 50ms",
    "Buffer: 每軌 256 Samples",
    "模組寬度: 8HP"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("視覚的信号モニタリング用8トラック波形オシロスコープ。各トラックは接続されたケーブルの色を自動的に採用し、複数信号の識別と比較が容易。")}

{h3("コントロール")}
{ul([
    f"{b('Time Scale')}: 表示部をクリック＆ドラッグで調整（画面毎5ms - 50ms）",
    f"{b('Trigger LED')}: クリックで切替 - 白: フリーラン、ピンク: 最初の接続入力に同期"
])}

{h3("入力")}
{ul([
    "Track 1-4: 上段（4x2グリッドレイアウト）",
    "Track 5-8: 下段",
    "最初の接続入力をトリガーソースとして使用"
])}

{h3("仕様")}
{ul([
    "入力範囲: ±10V",
    "Time Scale: 画面毎5ms - 50ms",
    "バッファ: トラック毎256サンプル",
    "モジュール幅: 8HP"
], last=True)}"""
    },

    "obserfour": {
        "manual_zh": f"""{h3("概述")}
{p("4 軌 Waveform Oscilloscope，具備 8 個輸入，專為訊號比較設計。每軌疊加兩個輸入，Waveform 自動採用連接 Cable 的顏色，方便直覺比較訊號。")}

{h3("顯示配置")}
{ul([
    "Track 1: Input 1 + Input 5 疊加",
    "Track 2: Input 2 + Input 6 疊加",
    "Track 3: Input 3 + Input 7 疊加",
    "Track 4: Input 4 + Input 8 疊加"
])}

{h3("控制")}
{ul([
    f"{b('Time Scale')}: 在顯示區點擊拖曳（每畫面 5ms - 50ms）",
    f"{b('Trigger LED')}: 白色: 自由運行、粉色: 同步至第一輸入"
])}

{h3("規格")}
{ul([
    "Input 範圍: ±10V",
    "Buffer: 每軌 256 Samples",
    "模組寬度: 8HP"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("信号比較用に設計された8入力付き4トラック波形オシロスコープ。各トラックに2入力を重ねて表示し、波形は接続ケーブルの色を自動採用して直感的な信号比較が可能。")}

{h3("表示レイアウト")}
{ul([
    "Track 1: Input 1 + Input 5 重畳",
    "Track 2: Input 2 + Input 6 重畳",
    "Track 3: Input 3 + Input 7 重畳",
    "Track 4: Input 4 + Input 8 重畳"
])}

{h3("コントロール")}
{ul([
    f"{b('Time Scale')}: 表示部をクリック＆ドラッグ（画面毎5ms - 50ms）",
    f"{b('Trigger LED')}: 白: フリーラン、ピンク: 最初の入力に同期"
])}

{h3("仕様")}
{ul([
    "入力範囲: ±10V",
    "バッファ: トラック毎256サンプル",
    "モジュール幅: 8HP"
], last=True)}"""
    },

    "twnc_2": {
        "manual_zh": f"""{h3("概述")}
{p("三軌 Drum Machine，具備專用 Bass Drum（BD）、Snare Drum（SN）與 Hi-Hat（HH）合成引擎。功能包含 CV 控制、Accent 系統、Ducking 與 10-bit 量化以獲得復古特色。")}

{h3("Bass Drum（BD）")}
{ul([
    f"{b('VOL')}: 音量（0-100%）",
    f"{b('ENV')}: Envelope Trigger CV 輸入",
    f"{b('TUNE')}: 頻率（24-500 Hz，對數）",
    f"{b('FM')}: FM 調變量",
    f"{b('PUNCH')}: Saturation 控制",
    f"{b('[ACCNT]')}: Accent CV 輸入"
])}

{h3("Snare Drum（SN）")}
{ul([
    f"{b('VOL')}: 音量（0-100%）",
    f"{b('N.BPF')}: Noise Bandpass Filter 音色",
    f"{b('TUNE')}: Body 頻率（100-300 Hz）",
    f"{b('N.MIX')}: Noise/Body 混合平衡"
])}

{h3("Hi-Hat（HH）")}
{ul([
    f"{b('VOL')}: 音量（0-100%）",
    f"{b('TONE')}: Filter 頻率",
    f"{b('DECAY')}: Envelope Decay 時間",
    f"{b('EXT')}: 外部 Audio 輸入"
])}

{h3("全域")}
{ul([
    f"{b('[DUCK]')}: Sidechain Ducking 量（0-100%）"
])}

{h3("輸出")}
{ul([
    "BD、SN、HH 獨立輸出",
    "L/R Stereo Mix（R 含 20ms Delay）"
])}

{h3("規格")}
{ul([
    "Input/Output: +/-10V",
    "量化: 10-bit（復古特色）",
    "HH 引擎: 6 個 Triangle Oscillator 含頻率偏移"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("バスドラム（BD）、スネアドラム（SN）、ハイハット（HH）専用合成エンジンを備えた3トラックドラムマシン。CV制御、アクセントシステム、ダッキング、ビンテージキャラクター用10ビット量子化を搭載。")}

{h3("バスドラム（BD）")}
{ul([
    f"{b('VOL')}: 音量（0-100%）",
    f"{b('ENV')}: エンベロープトリガーCV入力",
    f"{b('TUNE')}: 周波数（24-500 Hz、対数）",
    f"{b('FM')}: FM変調量",
    f"{b('PUNCH')}: サチュレーション制御",
    f"{b('[ACCNT]')}: アクセントCV入力"
])}

{h3("スネアドラム（SN）")}
{ul([
    f"{b('VOL')}: 音量（0-100%）",
    f"{b('N.BPF')}: ノイズバンドパスフィルター音色",
    f"{b('TUNE')}: ボディ周波数（100-300 Hz）",
    f"{b('N.MIX')}: ノイズ/ボディミックスバランス"
])}

{h3("ハイハット（HH）")}
{ul([
    f"{b('VOL')}: 音量（0-100%）",
    f"{b('TONE')}: フィルター周波数",
    f"{b('DECAY')}: エンベロープディケイ時間",
    f"{b('EXT')}: 外部オーディオ入力"
])}

{h3("グローバル")}
{ul([
    f"{b('[DUCK]')}: サイドチェインダッキング量（0-100%）"
])}

{h3("出力")}
{ul([
    "BD、SN、HH個別出力",
    "L/Rステレオミックス（Rは20msディレイ付き）"
])}

{h3("仕様")}
{ul([
    "入出力: +/-10V",
    "量子化: 10ビット（ビンテージキャラクター）",
    "HHエンジン: 周波数オフセット付き6トライアングルオシレーター"
], last=True)}"""
    },

    "twnc_light": {
        "manual_zh": f"""{h3("概述")}
{p("雙軌 Euclidean Rhythm Envelope 生成器，具備 Accent 系統。輸出 Envelope 訊號用於控制外部音源。")}

{h3("功能")}
{ul([
    "雙軌 Euclidean Rhythm 引擎",
    "Quarter-note Accent 系統，可調 Shift",
    "Track 2 Clock Division/Multiplication",
    "Decay 時間 CV 調變",
    "Attack-Decay Envelope 含 Shape 控制"
])}

{h3("輸入")}
{ul([
    f"{b('CLK')}: Clock 輸入（0.1V-2V Trigger）",
    f"{b('DRUM FREQ CV')}: Drum 頻率 CV 調變",
    f"{b('DRUM DECAY CV')}: Drum Decay CV 調變",
    f"{b('HATS FREQ CV')}: Hi-hat 頻率 CV 調變",
    f"{b('HATS DECAY CV')}: Hi-hat Decay CV 調變"
])}

{h3("全域控制")}
{ul([
    f"{b('LEN')}: 序列長度（1-32 步）"
])}

{h3("Drum Track（Track 1）")}
{ul([
    f"{b('FILL')}: 節奏密度（0-100%）",
    f"{b('DECAY')}: Envelope Decay（0.01-2s）",
    f"{b('ACCNT')}: Accent Shift（1-7 步）",
    f"{b('A.DEC')}: Accent Envelope Decay",
    f"{b('SHAPE')}: Envelope Shape（0.0-0.99）"
])}

{h3("HATs Track（Track 2）")}
{ul([
    f"{b('FILL')}: 節奏密度（0-100%）",
    f"{b('D/M')}: Clock Div/Mult（1/4x、1/2x、1x、1.5x、2x）",
    f"{b('DECAY')}: Envelope Decay（0.01-2s）",
    f"{b('SHAPE')}: Envelope Shape（0.0-0.99）"
])}

{h3("輸出")}
{ul([
    "Accent VCA Envelope（0-10V）",
    "Track 1 FM Envelope（0-10V）",
    "Track 2 VCA Envelope（0-10V）"
])}

{h3("規格")}
{ul([
    "Input: +/-10V（Trigger 與 CV）",
    "Output: 0-10V",
    "Attack: 1ms（固定）",
    "演算法: Bjorklund Euclidean 分佈"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("アクセントシステム付きデュアルトラックユークリッドリズムエンベロープ生成器。外部音源制御用エンベロープ信号を出力。")}

{h3("機能")}
{ul([
    "デュアルトラックユークリッドリズムエンジン",
    "調整可能シフト付き4分音符アクセントシステム",
    "Track 2クロック分割/倍増",
    "ディケイ時間CV変調",
    "シェイプ制御付きADエンベロープ"
])}

{h3("入力")}
{ul([
    f"{b('CLK')}: クロック入力（0.1V-2Vトリガー）",
    f"{b('DRUM FREQ CV')}: ドラム周波数CV変調",
    f"{b('DRUM DECAY CV')}: ドラムディケイCV変調",
    f"{b('HATS FREQ CV')}: ハイハット周波数CV変調",
    f"{b('HATS DECAY CV')}: ハイハットディケイCV変調"
])}

{h3("グローバルコントロール")}
{ul([
    f"{b('LEN')}: シーケンス長（1-32ステップ）"
])}

{h3("Drum Track（Track 1）")}
{ul([
    f"{b('FILL')}: リズム密度（0-100%）",
    f"{b('DECAY')}: エンベロープディケイ（0.01-2s）",
    f"{b('ACCNT')}: アクセントシフト（1-7ステップ）",
    f"{b('A.DEC')}: アクセントエンベロープディケイ",
    f"{b('SHAPE')}: エンベロープシェイプ（0.0-0.99）"
])}

{h3("HATs Track（Track 2）")}
{ul([
    f"{b('FILL')}: リズム密度（0-100%）",
    f"{b('D/M')}: クロックDiv/Mult（1/4x、1/2x、1x、1.5x、2x）",
    f"{b('DECAY')}: エンベロープディケイ（0.01-2s）",
    f"{b('SHAPE')}: エンベロープシェイプ（0.0-0.99）"
])}

{h3("出力")}
{ul([
    "アクセントVCAエンベロープ（0-10V）",
    "Track 1 FMエンベロープ（0-10V）",
    "Track 2 VCAエンベロープ（0-10V）"
])}

{h3("仕様")}
{ul([
    "入力: +/-10V（トリガーとCV）",
    "出力: 0-10V",
    "アタック: 1ms（固定）",
    "アルゴリズム: Bjorklundユークリッド分布"
], last=True)}"""
    },

    "kimo": {
        "manual_zh": f"""{h3("概述")}
{p("單軌 Kick Drum 合成器，具備 Euclidean Rhythm 生成與 Quarter-note Accent 系統。結合 Pattern 序列與 Sine Wave VCO、FM 調變及 Envelope 造型。")}

{h3("Clock 與 Rhythm")}
{ul([
    f"{b('CLK')}: 外部 Clock 輸入",
    f"{b('FILL')}: 活躍步進百分比（0-100%），固定 16 步 Pattern"
])}

{h3("Accent")}
{ul([
    f"{b('ACCENT')}: Quarter-note Accent Shift 時機（1-7 步）",
    f"{b('DELAY')}: Accent Envelope Decay（0.01-2.0s）"
])}

{h3("合成")}
{ul([
    f"{b('TUNE')}: 基頻（24-500 Hz，對數）",
    f"{b('FM')}: 頻率調變量（0.0-1.0）",
    f"{b('PUNCH')}: Saturation/Harmonic Distortion（0.0-1.0）",
    f"{b('DECAY')}: VCA Envelope Decay（0.01-2.0s，對數）",
    f"{b('SHAPE')}: Envelope 曲線造型（0.0-0.99）",
    "CV 輸入: Fill、Tune、FM、Punch、Decay"
])}

{h3("輸出")}
{ul([
    f"{b('VCA/FM/ACCENT ENV')}: Envelope 訊號（0-10V）",
    f"{b('AUDIO')}: Kick Drum Output（±10V）"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("ユークリッドリズム生成と4分音符アクセントシステムを備えた単一トラックキックドラム合成器。パターンシーケンス、サイン波VCO、FM変調、エンベロープシェイピングを統合。")}

{h3("クロックとリズム")}
{ul([
    f"{b('CLK')}: 外部クロック入力",
    f"{b('FILL')}: アクティブステップ割合（0-100%）、固定16ステップパターン"
])}

{h3("アクセント")}
{ul([
    f"{b('ACCENT')}: 4分音符アクセントシフトタイミング（1-7ステップ）",
    f"{b('DELAY')}: アクセントエンベロープディケイ（0.01-2.0s）"
])}

{h3("合成")}
{ul([
    f"{b('TUNE')}: 基本周波数（24-500 Hz、対数）",
    f"{b('FM')}: 周波数変調量（0.0-1.0）",
    f"{b('PUNCH')}: サチュレーション/ハーモニックディストーション（0.0-1.0）",
    f"{b('DECAY')}: VCAエンベロープディケイ（0.01-2.0s、対数）",
    f"{b('SHAPE')}: エンベロープカーブシェイピング（0.0-0.99）",
    "CV入力: Fill、Tune、FM、Punch、Decay"
])}

{h3("出力")}
{ul([
    f"{b('VCA/FM/ACCENT ENV')}: エンベロープ信号（0-10V）",
    f"{b('AUDIO')}: キックドラム出力（±10V）"
], last=True)}"""
    },

    "pinpple": {
        "manual_zh": f"""{h3("概述")}
{p("Ping Filter 合成器，具備動態 FM 調變。基於 Mutable Instruments Ripples BPF 設計，結合 Trigger 驅動 Envelope 生成與 LPG 系統處理 FM。透過內建隨機化創造具有有機特色變化的金屬打擊音效。")}

{h3("主要控制")}
{ul([
    f"{b('MUTE')}: Mute/Unmute 切換，含 LED 指示",
    f"{b('VOLUME')}: Output Gain（0.0-1.0，預設: 1.0）",
    f"{b('NOISE MIX')}: Noise 訊號混合量（0.0-1.0，預設: 1.0）",
    f"{b('FREQ')}: 中心頻率（20Hz-20kHz，對數）含 1V/Oct CV 輸入與 Attenuverter",
    f"{b('DECAY')}: Envelope Decay 時間（0.0-1.0）含 CV 輸入與 Attenuverter",
    f"{b('FM AMT')}: FM 調變量（0.0-1.0）含 CV 輸入與 Attenuverter"
])}

{h3("隨機化系統")}
{ul([
    "每次 Trigger 對頻率與 Decay 參數施加隨機偏移",
    "高斯分佈，標準差 0.00006",
    "視覺回饋: 隨機化旋鈕顯示當前偏移值"
])}

{h3("I/O")}
{ul([
    f"{b('TRIG IN')}: Envelope 與隨機化的 Trigger 輸入",
    f"{b('LPG IN')}: FM 調變處理的 Audio 輸入",
    f"{b('OUTPUT')}: 主 Audio Output"
])}

{h3("規格")}
{ul([
    "BPF 範圍: 20Hz - 20kHz",
    "LPG Decay: 0.01 - 0.51 秒",
    "LPG Cutoff 範圍: 200Hz - 18.2kHz（Envelope 控制）",
    "Trigger 閾值: 2.0V（Schmitt Trigger）",
    "處理: 32-bit 浮點含 Anti-aliasing",
    "Oversampling: 自動（基於 Sample Rate 1x-15x）",
    "Monophonic"
])}

{h3("右鍵選單")}
{ul([
    f"{b('VCA Attack Time')}: 調整 VCA Envelope Attack 時間（0.5-20ms，預設: 6ms）"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("ダイナミックFM変調付きピングフィルター合成器。Mutable Instruments Ripples BPF設計に基づき、トリガー駆動エンベロープ生成とFM処理用LPGシステムを統合。内蔵ランダム化により有機的なキャラクター変化を持つ金属的パーカッションサウンドを生成。")}

{h3("メインコントロール")}
{ul([
    f"{b('MUTE')}: LEDインジケーター付きミュート/アンミュート切替",
    f"{b('VOLUME')}: 出力ゲイン（0.0-1.0、デフォルト: 1.0）",
    f"{b('NOISE MIX')}: ノイズ信号ミックス量（0.0-1.0、デフォルト: 1.0）",
    f"{b('FREQ')}: 中心周波数（20Hz-20kHz、対数）1V/Oct CV入力とアッテヌバーター付き",
    f"{b('DECAY')}: エンベロープディケイ時間（0.0-1.0）CV入力とアッテヌバーター付き",
    f"{b('FM AMT')}: FM変調量（0.0-1.0）CV入力とアッテヌバーター付き"
])}

{h3("ランダム化システム")}
{ul([
    "各トリガーで周波数とディケイパラメータにランダムオフセットを適用",
    "ガウス分布、標準偏差0.00006",
    "視覚フィードバック: ランダム化ノブが現在のオフセット値を表示"
])}

{h3("I/O")}
{ul([
    f"{b('TRIG IN')}: エンベロープとランダム化用トリガー入力",
    f"{b('LPG IN')}: FM変調処理用オーディオ入力",
    f"{b('OUTPUT')}: メインオーディオ出力"
])}

{h3("仕様")}
{ul([
    "BPF範囲: 20Hz - 20kHz",
    "LPGディケイ: 0.01 - 0.51秒",
    "LPGカットオフ範囲: 200Hz - 18.2kHz（エンベロープ制御）",
    "トリガー閾値: 2.0V（シュミットトリガー）",
    "処理: アンチエイリアシング付き32ビット浮動小数点",
    "オーバーサンプリング: 自動（サンプルレートに基づき1x-15x）",
    "モノフォニック"
])}

{h3("右クリックメニュー")}
{ul([
    f"{b('VCA Attack Time')}: VCAエンベロープアタック時間調整（0.5-20ms、デフォルト: 6ms）"
], last=True)}"""
    },

    "swing_lfo": {
        "manual_zh": f"""{h3("概述")}
{p("雙相位 LFO，具備 Swing 與 Shape 控制。生成兩個相位偏移的 Oscillator，可獨立造型與混合，為 Patch 增添 Groove 與動態的複雜調變源。")}

{h3("控制")}
{ul([
    f"{b('FREQ')}: 基頻（0.125Hz - 128Hz，指數刻度）含 CV 輸入與 Attenuverter",
    f"{b('SWING')}: Oscillator 間相位偏移（0° - 90°）含 CV 輸入與 Attenuverter",
    f"{b('SHAPE')}: SAW Output 在 Ramp → Triangle → Saw 間變形；PULSE Output 控制寬度（1% - 30%）含 CV",
    f"{b('MIX')}: 混合主相位（0%）至 Swing 相位（100%）含 CV 輸入與 Attenuverter",
    f"{b('RST')}: 外部 Clock 同步的 Reset 輸入"
])}

{h3("輸出")}
{ul([
    f"{b('SAW')}: Ramp/Triangle/Saw Waveform（±10V）",
    f"{b('PULSE')}: 可變寬度 Pulse Waveform（0-10V）"
])}

{h3("規格")}
{ul([
    "CV 輸入範圍: ±10V",
    "頻率範圍: 0.125Hz - 128Hz",
    "相位偏移: 0° - 90°",
    "Pulse 寬度: 1% - 30%",
    "Reset 閾值: 2.0V",
    "處理: 32-bit 浮點"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("スウィングとシェイプ制御を備えたデュアルフェーズLFO。位相オフセットした2つのオシレーターを生成し、独立してシェイピング・ミックス可能。パッチにグルーブと動きを加える複雑な変調ソースを作成。")}

{h3("コントロール")}
{ul([
    f"{b('FREQ')}: 基本周波数（0.125Hz - 128Hz、指数スケール）CV入力とアッテヌバーター付き",
    f"{b('SWING')}: オシレーター間位相オフセット（0° - 90°）CV入力とアッテヌバーター付き",
    f"{b('SHAPE')}: SAW出力はランプ→三角→ノコギリ間モーフィング、PULSE出力は幅制御（1% - 30%）CV付き",
    f"{b('MIX')}: メインフェーズ（0%）からスウィングフェーズ（100%）をブレンド、CV入力とアッテヌバーター付き",
    f"{b('RST')}: 外部クロック同期用リセット入力"
])}

{h3("出力")}
{ul([
    f"{b('SAW')}: ランプ/三角/ノコギリ波形（±10V）",
    f"{b('PULSE')}: 可変幅パルス波形（0-10V）"
])}

{h3("仕様")}
{ul([
    "CV入力範囲: ±10V",
    "周波数範囲: 0.125Hz - 128Hz",
    "位相オフセット: 0° - 90°",
    "パルス幅: 1% - 30%",
    "リセット閾値: 2.0V",
    "処理: 32ビット浮動小数点"
], last=True)}"""
    },

    "euclidean_rhythm": {
        "manual_zh": f"""{h3("概述")}
{p("三軌 Euclidean Rhythm 生成器，具備 Sequencing 與 Chaining 功能。使用 Euclidean 演算法將 Beat 均勻分佈於序列長度 - 這種 Pattern 可在世界各地傳統音樂中發現。")}

{h3("全域")}
{ul([
    f"{b('CLK')}: 所有軌道的全域 Clock 輸入",
    f"{b('RST')}: 全域 Reset 輸入 + 手動按鈕"
])}

{h3("各軌控制（T1、T2、T3）")}
{ul([
    f"{b('LEN')}: 序列長度（1-32 步，預設: 16）",
    f"{b('FILL')}: 活躍步進百分比（0-100%，預設: 25%）",
    f"{b('SHFT')}: Pattern 旋轉（0-31 步）",
    f"{b('D/M')}: Clock Division/Multiplication（-3 至 +3: 1/4x 至 4x）",
    "Length、Fill、Shift 的 CV 輸入含 Attenuverter"
])}

{h3("輸出")}
{ul([
    f"{b('OUT 1/2/3')}: 獨立軌道 Trigger（0-10V）",
    f"{b('1+2')}: T1↔T2 順序切換",
    f"{b('2+3')}: T2↔T3 順序切換",
    f"{b('1+2+3')}: T1→T2→T3 順序切換",
    f"{b('MASTER')}: 所有活躍軌道 Trigger 總和"
])}

{h3("規格")}
{ul([
    "Input: ±10V（Clock 與 CV）",
    "Output: 0-10V Trigger，10ms Pulse",
    "處理: 32-bit 浮點"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("シーケンスとチェーン機能を備えた3トラックユークリッドリズム生成器。ユークリッドアルゴリズムでビートをシーケンス長に均等分配 - 世界各地の伝統音楽に見られるパターン。")}

{h3("グローバル")}
{ul([
    f"{b('CLK')}: 全トラック用グローバルクロック入力",
    f"{b('RST')}: グローバルリセット入力 + 手動ボタン"
])}

{h3("トラック別コントロール（T1、T2、T3）")}
{ul([
    f"{b('LEN')}: シーケンス長（1-32ステップ、デフォルト: 16）",
    f"{b('FILL')}: アクティブステップ割合（0-100%、デフォルト: 25%）",
    f"{b('SHFT')}: パターンローテーション（0-31ステップ）",
    f"{b('D/M')}: クロック分割/倍増（-3〜+3: 1/4x〜4x）",
    "Length、Fill、ShiftのCV入力とアッテヌバーター"
])}

{h3("出力")}
{ul([
    f"{b('OUT 1/2/3')}: 個別トラックトリガー（0-10V）",
    f"{b('1+2')}: T1↔T2順次切替",
    f"{b('2+3')}: T2↔T3順次切替",
    f"{b('1+2+3')}: T1→T2→T3順次切替",
    f"{b('MASTER')}: 全アクティブトラックトリガーの合計"
])}

{h3("仕様")}
{ul([
    "入力: ±10V（クロックとCV）",
    "出力: 0-10Vトリガー、10msパルス",
    "処理: 32ビット浮動小数点"
], last=True)}"""
    },

    "ad_generator": {
        "manual_zh": f"""{h3("概述")}
{p("三軌 Attack-Decay Envelope 生成器，具備雙處理模式。功能包含獨立 Envelope 控制、全域參數、Auto-routing，以及 Hybrid Trigger+Follower 處理的 Follower 系統。")}

{h3("全域控制")}
{ul([
    f"{b('ATK ALL')}: 全域 Attack 時間偏移（-1.0 至 +1.0）",
    f"{b('DEC ALL')}: 全域 Decay 時間偏移（-1.0 至 +1.0）",
    f"{b('ROUTE')}: 自動將 Track 1 輸入路由至所有軌道"
])}

{h3("各軌控制")}
{ul([
    f"{b('IN')}: Trigger/Audio 輸入",
    f"{b('ATK')}: Attack 時間（0.0-1.0，指數，約 1ms 至 1000s）",
    f"{b('DEC')}: Decay 時間（0.0-1.0，指數，約 1ms 至 1000s）",
    f"{b('CURV')}: Envelope 曲線形狀（-0.99 至 +0.99）",
    f"{b('Follower')}: 啟用 BPF 與 Envelope Follower",
    f"{b('FREQ')}: BPF 頻率（20Hz-8kHz，預設: 200Hz/1kHz/5kHz）",
    f"{b('GAIN')}: BPF 輸出增益（0.1x-100x）"
])}

{h3("處理模式")}
{ul([
    f"{b('Follower Off')}: 僅 Trigger 模式，回應 9.5V 以上上升沿",
    f"{b('Follower On')}: Hybrid 模式，輸出 Trigger Envelope 與含 BPF 的 Amplitude Follower 最大值"
])}

{h3("輸出")}
{ul([
    f"{b('1、2、3')}: 獨立軌道輸出（0-10V）",
    f"{b('MIYA')}: 含自動電平調整的所有軌道總和"
])}

{h3("規格")}
{ul([
    "Input: +/-10V（Trigger 與 Audio）",
    "Output: 0-10V",
    "處理: 32-bit 浮點"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("デュアル処理モード付き3トラックADエンベロープ生成器。独立エンベロープ制御、グローバルパラメータ、オートルーティング、ハイブリッドトリガー+フォロワー処理用フォロワーシステムを搭載。")}

{h3("グローバルコントロール")}
{ul([
    f"{b('ATK ALL')}: グローバルアタック時間オフセット（-1.0〜+1.0）",
    f"{b('DEC ALL')}: グローバルディケイ時間オフセット（-1.0〜+1.0）",
    f"{b('ROUTE')}: Track 1入力を全トラックに自動ルーティング"
])}

{h3("トラック別コントロール")}
{ul([
    f"{b('IN')}: トリガー/オーディオ入力",
    f"{b('ATK')}: アタック時間（0.0-1.0、指数、約1ms〜1000s）",
    f"{b('DEC')}: ディケイ時間（0.0-1.0、指数、約1ms〜1000s）",
    f"{b('CURV')}: エンベロープカーブ形状（-0.99〜+0.99）",
    f"{b('Follower')}: BPFとエンベロープフォロワーを有効化",
    f"{b('FREQ')}: BPF周波数（20Hz-8kHz、デフォルト: 200Hz/1kHz/5kHz）",
    f"{b('GAIN')}: BPF出力ゲイン（0.1x-100x）"
])}

{h3("処理モード")}
{ul([
    f"{b('Follower Off')}: トリガーのみモード、9.5V以上の立ち上がりエッジに反応",
    f"{b('Follower On')}: ハイブリッドモード、トリガーエンベロープとBPF付きアンプリチュードフォロワーの最大値を出力"
])}

{h3("出力")}
{ul([
    f"{b('1、2、3')}: 個別トラック出力（0-10V）",
    f"{b('MIYA')}: 自動レベル調整付き全トラックの合計"
])}

{h3("仕様")}
{ul([
    "入力: +/-10V（トリガーとオーディオ）",
    "出力: 0-10V",
    "処理: 32ビット浮動小数点"
], last=True)}"""
    },

    "q_q": {
        "manual_zh": f"""{h3("概述")}
{p("三軌 Decay Envelope 生成器，具備即時 Oscilloscope 顯示。功能包含可調 Attack（透過右鍵選單 0.5-20ms）、含 CV 調變的可調 Decay，以及各軌曲線造型。")}

{h3("各軌控制（T1、T2、T3）")}
{ul([
    f"{b('Trigger')}: 軌道 Trigger 輸入",
    f"{b('DECAY')}: Decay 時間（0.01-2 秒，指數）",
    f"{b('SHAPE')}: Envelope 曲線（-0.99 至 +0.99）",
    f"{b('CV')}: Decay 時間調變輸入",
    f"{b('Attenuator')}: 隱藏 CV Attenuator（0-100%）"
])}

{h3("Scope 顯示")}
{ul([
    "即時三軌 Waveform 視覺化（128 點 Buffer）",
    "Waveform 顏色匹配輸入 Cable 顏色",
    "隱藏時間軸調整控制"
])}

{h3("輸出")}
{ul([
    f"{b('1、2、3')}: 獨立軌道輸出（0-10V）"
])}

{h3("規格")}
{ul([
    "Input: +/-10V（Trigger 與 CV）",
    "Output: 0-10V",
    "Attack: 0.5-20ms（透過右鍵選單調整）",
    "Trigger 閾值: 9.5V"
])}

{h3("右鍵選單")}
{ul([
    f"{b('Attack Time（各軌）')}: 調整 Attack 時間 0.5-20ms",
    f"{b('Retrigger')}: 啟用時 Envelope 從當前值重啟而非零點"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("リアルタイムオシロスコープ表示付き3トラックディケイエンベロープ生成器。調整可能アタック（右クリックメニューで0.5-20ms）、CV変調対応調整可能ディケイ、トラック別カーブシェイピングを搭載。")}

{h3("トラック別コントロール（T1、T2、T3）")}
{ul([
    f"{b('Trigger')}: トラックトリガー入力",
    f"{b('DECAY')}: ディケイ時間（0.01-2秒、指数）",
    f"{b('SHAPE')}: エンベロープカーブ（-0.99〜+0.99）",
    f"{b('CV')}: ディケイ時間変調入力",
    f"{b('Attenuator')}: 隠しCVアッテヌエーター（0-100%）"
])}

{h3("スコープ表示")}
{ul([
    "リアルタイム3トラック波形視覚化（128ポイントバッファ）",
    "波形色は入力ケーブル色に一致",
    "隠し時間軸調整コントロール"
])}

{h3("出力")}
{ul([
    f"{b('1、2、3')}: 個別トラック出力（0-10V）"
])}

{h3("仕様")}
{ul([
    "入力: +/-10V（トリガーとCV）",
    "出力: 0-10V",
    "アタック: 0.5-20ms（右クリックメニューで調整）",
    "トリガー閾値: 9.5V"
])}

{h3("右クリックメニュー")}
{ul([
    f"{b('Attack Time（トラック別）')}: アタック時間0.5-20ms調整",
    f"{b('Retrigger')}: 有効時、エンベロープはゼロではなく現在値から再開"
], last=True)}"""
    },

    "ppattterning": {
        "manual_zh": f"""{h3("概述")}
{p("Pattern 型 CV Sequencer，具備 Style 與 Density 控制。使用五個電壓旋鈕與三種 Sequencing Style 生成 CV 序列，含 Shift Register 與 CVD 可變 Delay 實現 Polyrhythmic 變化。")}

{h3("控制")}
{ul([
    f"{b('CLK')}: 序列推進的 Clock 輸入",
    f"{b('RST')}: 重置至步進 1",
    f"{b('1-5')}: 電壓旋鈕（-10V 至 +10V）",
    f"{b('MODE')}: Sequential（紅）、Minimalism（綠）、Jump（藍）",
    f"{b('DENSITY')}: 序列複雜度與旋鈕使用（0.0-1.0）",
    f"{b('CHAOS')}: 即時隨機化（0.0-1.0）",
    f"{b('T2.DLY')}: Shift Register Delay（0-5 步）",
    f"{b('CVD')}: 可變 Delay（0-1000ms）或 CV 調變量"
])}

{h3("Sequencing Style")}
{ul([
    f"{b('Sequential')}: 依序循環旋鈕，Density 控制範圍",
    f"{b('Minimalism')}: 32 步 Pattern 含重複動機",
    f"{b('Jump')}: 交替遠端旋鈕（1,3,5,2,4）"
])}

{h3("輸出")}
{ul([
    f"{b('CV OUT')}: 主序列 CV Output",
    f"{b('TRIG')}: CV 變化時 Trigger",
    f"{b('CV2')}: 延遲序列（Shift Register + CVD）",
    f"{b('TRIG2')}: 延遲 Trigger Output"
])}

{h3("規格")}
{ul([
    "Input/Output: +/-10V",
    "Delay: 0-1000ms（CVD）+ 0-5 步（Shift Register）",
    "處理: 32-bit 浮點"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("スタイルと密度制御を備えたパターン型CVシーケンサー。5つの電圧ノブと3つのシーケンススタイルでCVシーケンスを生成、シフトレジスタとCVD可変ディレイでポリリズム変化を実現。")}

{h3("コントロール")}
{ul([
    f"{b('CLK')}: シーケンス進行用クロック入力",
    f"{b('RST')}: ステップ1にリセット",
    f"{b('1-5')}: 電圧ノブ（-10V〜+10V）",
    f"{b('MODE')}: Sequential（赤）、Minimalism（緑）、Jump（青）",
    f"{b('DENSITY')}: シーケンス複雑度とノブ使用（0.0-1.0）",
    f"{b('CHAOS')}: リアルタイムランダム化（0.0-1.0）",
    f"{b('T2.DLY')}: シフトレジスタディレイ（0-5ステップ）",
    f"{b('CVD')}: 可変ディレイ（0-1000ms）またはCV変調量"
])}

{h3("シーケンススタイル")}
{ul([
    f"{b('Sequential')}: ノブを順に循環、密度が範囲を制御",
    f"{b('Minimalism')}: 反復モチーフ付き32ステップパターン",
    f"{b('Jump')}: 遠いノブを交互に（1,3,5,2,4）"
])}

{h3("出力")}
{ul([
    f"{b('CV OUT')}: メインシーケンスCV出力",
    f"{b('TRIG')}: CV変化時トリガー",
    f"{b('CV2')}: 遅延シーケンス（シフトレジスタ + CVD）",
    f"{b('TRIG2')}: 遅延トリガー出力"
])}

{h3("仕様")}
{ul([
    "入出力: +/-10V",
    "ディレイ: 0-1000ms（CVD）+ 0-5ステップ（シフトレジスタ）",
    "処理: 32ビット浮動小数点"
], last=True)}"""
    },

    "ellen_ripley": {
        "manual_zh": f"""{h3("概述")}
{p("串聯多效果處理器，包含三個串聯階段: Delay、Granular Synthesis（GRATCH）與 Reverb。每個階段包含基於 Lorenz Attractor 的 Chaos 調變，具備獨立啟用按鈕。")}

{h3("訊號流程")}
{p("Input → Delay → GRATCH（Granular）→ Reverb → Output")}

{h3("DELAY 區段")}
{ul([
    f"{b('TIME L/R')}: 獨立 Delay 時間（1ms - 2s），CV ±200ms",
    f"{b('FDBK')}: Feedback 量（0-95%），CV ±10%",
    f"{b('C 按鈕')}: 啟用 Chaos 調變（±100ms 時間擺動）"
])}

{h3("GRATCH 區段（Granular）")}
{ul([
    f"{b('SIZE')}: Grain 大小（1-100ms），小 = Glitchy，大 = 平滑",
    f"{b('BREAK')}: Grain 密度/Glitch 量（0-100%）",
    f"{b('SHIFT')}: Buffer 位置（0-100%），Chaos 加入反轉與 Pitch Shift",
    f"{b('C 按鈕')}: 啟用 Chaos（反向 Grain、Pitch 變化、位置打亂）",
    "最多 16 同時 Grain，約 170ms Buffer，Raised Cosine Envelope"
])}

{h3("REVERB 區段")}
{ul([
    f"{b('ROOM')}: 空間大小（0-100%），影響 Delay Tap 位置",
    f"{b('TONE')}: Damping/亮度（0-100%）",
    f"{b('DECAY')}: 尾部長度（0-100%），最長 10+ 秒",
    f"{b('C 按鈕')}: 啟用 Chaos（動態 Feedback 與反射）",
    "8 並聯 Comb Filter、4 串聯 Allpass Filter"
])}

{h3("CHAOS 區段")}
{ul([
    f"{b('RATE')}: 0.01-1.0 Hz（平滑）或 1.0-10.0 Hz（階梯）",
    f"{b('SHAPE')}: 切換平滑/階梯 Chaos",
    f"{b('AMOUNT')}: 全域 Chaos 強度（0-100%）",
    f"{b('CHAOS OUT')}: CV Output（±5V）"
])}

{h3("I/O")}
{ul([
    "Stereo Audio In/Out（Right 正規化至 Left）",
    "大多數參數的 CV 輸入",
    "各階段 Wet/Dry Mix 控制"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("3ステージ直列のシリアルマルチエフェクトプロセッサー: Delay、Granular Synthesis（GRATCH）、Reverb。各ステージにローレンツアトラクターベースのカオス変調と独立イネーブルボタンを搭載。")}

{h3("信号フロー")}
{p("Input → Delay → GRATCH（Granular）→ Reverb → Output")}

{h3("DELAYセクション")}
{ul([
    f"{b('TIME L/R')}: 独立ディレイ時間（1ms - 2s）、CV ±200ms",
    f"{b('FDBK')}: フィードバック量（0-95%）、CV ±10%",
    f"{b('Cボタン')}: カオス変調を有効化（±100ms時間ウォブル）"
])}

{h3("GRATCHセクション（Granular）")}
{ul([
    f"{b('SIZE')}: グレインサイズ（1-100ms）、小 = グリッチー、大 = スムース",
    f"{b('BREAK')}: グレイン密度/グリッチ量（0-100%）",
    f"{b('SHIFT')}: バッファ位置（0-100%）、カオスがリバースとピッチシフトを追加",
    f"{b('Cボタン')}: カオス有効化（リバースグレイン、ピッチ変化、位置スクランブル）",
    "最大16同時グレイン、約170msバッファ、レイズドコサインエンベロープ"
])}

{h3("REVERBセクション")}
{ul([
    f"{b('ROOM')}: ルームサイズ（0-100%）、ディレイタップ位置に影響",
    f"{b('TONE')}: ダンピング/明るさ（0-100%）",
    f"{b('DECAY')}: テール長（0-100%）、最大10秒以上",
    f"{b('Cボタン')}: カオス有効化（ダイナミックフィードバックと反射）",
    "8並列コムフィルター、4直列オールパスフィルター"
])}

{h3("CHAOSセクション")}
{ul([
    f"{b('RATE')}: 0.01-1.0 Hz（スムース）または1.0-10.0 Hz（ステップ）",
    f"{b('SHAPE')}: スムース/ステップカオス切替",
    f"{b('AMOUNT')}: グローバルカオス強度（0-100%）",
    f"{b('CHAOS OUT')}: CV出力（±5V）"
])}

{h3("I/O")}
{ul([
    "ステレオオーディオIn/Out（RightはLeftに正規化）",
    "ほとんどのパラメータのCV入力",
    "ステージ別ウェット/ドライミックス制御"
], last=True)}"""
    },

    "env_vca_6": {
        "manual_zh": f"""{h3("概述")}
{p("與 offthesky 共同開發的六聲道 Envelope 控制 VCA 系統。功能包含 Hybrid AD Envelope 含 Audio-following 功能、Stereo 處理、Sum Bus 路由至 Channel 6，以及用於現場演出的手動 Gate Trigger。")}

{h3("各聲道控制")}
{ul([
    f"{b('Attack')}: Attack 時間（1ms-1s，指數刻度）",
    f"{b('Release')}: Release 時間（1ms-1s，指數刻度）",
    f"{b('Out Vol')}: Output 音量（0-100%，預設: 80%）",
    f"{b('Manual 按鈕')}: 瞬時 Gate Trigger - 按住啟動、放開停止（粉色 LED）",
    f"{b('AHR 按鈕')}: 切換 AD/AHR 模式 - AHR 在 Attack 與 Release 間加入 Hold 階段",
    f"{b('Sum 按鈕')}: 將聲道路由至 Channel 6 Sum Bus"
])}

{h3("各聲道 I/O")}
{ul([
    f"{b('In L/R')}: Stereo Audio 輸入（±5V，自動 Mono-to-Stereo）",
    f"{b('Gate')}: Trigger 輸入（>9.5V 閾值）",
    f"{b('Vol Ctrl')}: 音量 CV（0-10V）",
    f"{b('Gate Out')}: 三種模式透過選單 - 完整週期 Gate、End Trigger（1ms）、Start+End Trigger",
    f"{b('Env Out')}: Envelope 電壓（0-10V）",
    f"{b('Out L/R')}: 處理後 Stereo Output（±5V）"
])}

{h3("Sum Bus 系統")}
{ul([
    "Channel 1-5 可透過 Sum 按鈕路由至 Channel 6",
    "Audio 加總: 每聲道 0.3x 縮放防止過載",
    "Envelope 加總: 基於 RMS 的組合"
])}

{h3("規格")}
{ul([
    "Audio: ±5V 輸入/輸出",
    "CV: 0-10V（音量）、0-10V（Envelope Out）",
    "Gate 閾值: 9.5V 含 Schmitt Trigger",
    "Envelope 曲線: 固定 -0.9（Log Attack、Exp Decay）",
    "處理: 32-bit 浮點"
], last=True)}""",

        "manual_ja": f"""{h3("概要")}
{p("offtheskyと共同開発した6チャンネルエンベロープ制御VCAシステム。オーディオフォロー機能付きハイブリッドADエンベロープ、ステレオ処理、Channel 6へのサムバスルーティング、ライブパフォーマンス用手動ゲートトリガーを搭載。")}

{h3("チャンネル別コントロール")}
{ul([
    f"{b('Attack')}: アタック時間（1ms-1s、指数スケール）",
    f"{b('Release')}: リリース時間（1ms-1s、指数スケール）",
    f"{b('Out Vol')}: 出力音量（0-100%、デフォルト: 80%）",
    f"{b('Manualボタン')}: モーメンタリーゲートトリガー - 長押しで起動、離すと停止（ピンクLED）",
    f"{b('AHRボタン')}: AD/AHRモード切替 - AHRはアタックとリリース間にホールドステージ追加",
    f"{b('Sumボタン')}: チャンネルをChannel 6サムバスにルーティング"
])}

{h3("チャンネル別I/O")}
{ul([
    f"{b('In L/R')}: ステレオオーディオ入力（±5V、自動モノ→ステレオ）",
    f"{b('Gate')}: トリガー入力（>9.5V閾値）",
    f"{b('Vol Ctrl')}: 音量CV（0-10V）",
    f"{b('Gate Out')}: メニューで3モード - フルサイクルゲート、エンドトリガー（1ms）、スタート+エンドトリガー",
    f"{b('Env Out')}: エンベロープ電圧（0-10V）",
    f"{b('Out L/R')}: 処理済みステレオ出力（±5V）"
])}

{h3("サムバスシステム")}
{ul([
    "Channel 1-5はSumボタンでChannel 6にルーティング可能",
    "オーディオ加算: チャンネル毎0.3xスケーリングでオーバーロード防止",
    "エンベロープ加算: RMSベースの組み合わせ"
])}

{h3("仕様")}
{ul([
    "オーディオ: ±5V入出力",
    "CV: 0-10V（音量）、0-10V（エンベロープ出力）",
    "ゲート閾値: シュミットトリガー付き9.5V",
    "エンベロープカーブ: 固定-0.9（ログアタック、指数ディケイ）",
    "処理: 32ビット浮動小数点"
], last=True)}"""
    }
}

def update_translations():
    """Update manual_zh and manual_ja in module JSON files"""
    modules_dir = '/Users/madzine/Documents/VCV-Dev/MADZINE/Manual/modules'

    for slug, trans in DETAILED_TRANSLATIONS.items():
        json_path = os.path.join(modules_dir, f'{slug}.json')

        if not os.path.exists(json_path):
            print(f"Not found: {json_path}")
            continue

        with open(json_path, 'r', encoding='utf-8') as f:
            module = json.load(f)

        # Update manual translations
        if 'manual_zh' in trans:
            module['manual_zh'] = trans['manual_zh']
        if 'manual_ja' in trans:
            module['manual_ja'] = trans['manual_ja']

        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(module, f, ensure_ascii=False, indent=2)

        print(f"Updated: {slug}")

if __name__ == '__main__':
    update_translations()
    print("\nTranslation update complete!")
