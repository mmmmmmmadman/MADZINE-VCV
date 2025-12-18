#!/usr/bin/env python3
"""
Fix incorrect translations and descriptions in module JSON files.
This script updates manual_zh, manual_ja, description_zh/ja, features_zh/ja
to match the actual module functionality.
"""

import json
import os

H3_STYLE = 'style="color: #ff6b9d; margin-bottom: 0.8rem; border-bottom: 2px solid #2a2a4a; padding-bottom: 0.5rem;"'
P_STYLE = 'style="margin-bottom: 1rem;"'
UL_STYLE = 'style="padding-left: 1.5rem; margin-bottom: 1rem;"'
UL_STYLE_LAST = 'style="padding-left: 1.5rem;"'

def h3(text): return f'<h3 {H3_STYLE}>{text}</h3>'
def p(text): return f'<p {P_STYLE}>{text}</p>'
def ul(items, last=False):
    style = UL_STYLE_LAST if last else UL_STYLE
    return f'<ul {style}>\n' + '\n'.join(f'<li>{item}</li>' for item in items) + '\n</ul>'
def b(text): return f'<b>{text}</b>'

FIXES = {
    # Quantizer - detailed manual_zh/ja to match English
    "quantizer": {
        "manual_zh": (
            h3("概述") +
            p("具備各音符 Microtune 控制與 10 種文化音階預設的 Chromatic Quantizer。功能包含視覺化鋼琴界面、Polyphonic 處理與音符移位功能供移調使用。") +
            h3("控制") +
            ul([
                f"{b('Piano Display')}: 點擊啟用/停用各 Chromatic 音符",
                f"{b('12 Microtune 旋鈕')}: 各音符調音（-50 至 +50 Cent）",
                f"{b('OFFSET')}: Quantize 前 Pitch 偏移（-1 至 +1 半音）",
                f"{b('CV IN')}: Offset 調變（+/-10V）"
            ]) +
            h3("Scale Preset") +
            ul([
                "Equal Temperament、Just Intonation、Pythagorean",
                "Arabic Maqam、Indian Raga、Gamelan Pelog",
                "Japanese Gagaku、Turkish Makam、Persian Dastgah",
                "Quarter Tone"
            ]) +
            h3("I/O") +
            ul([
                "3 CV Input 與 3 Quantized Output",
                "Offset CV Input"
            ]) +
            h3("右鍵選單") +
            ul([
                f"{b('Scale Preset')}: 16 種預設音階（Chromatic、Major、Minor、Pentatonic Major/Minor、Dorian、Phrygian、Lydian、Mixolydian、Locrian、Major Triad、Minor Triad、Blues、Arabic、Japanese、Whole Tone）",
                f"{b('Microtune Preset')}: 如上列文化調音系統"
            ], last=True)
        ),
        "manual_ja": (
            h3("概要") +
            p("ノート別マイクロチューン制御と10種の文化スケールプリセットを備えたクロマチッククオンタイザー。視覚的ピアノインターフェース、ポリフォニック処理、移調用ノートシフト機能を搭載。") +
            h3("コントロール") +
            ul([
                f"{b('Piano Display')}: クリックで各クロマチックノートを有効/無効",
                f"{b('12 Microtuneノブ')}: ノート別チューニング（-50〜+50セント）",
                f"{b('OFFSET')}: クオンタイズ前ピッチオフセット（-1〜+1半音）",
                f"{b('CV IN')}: オフセット変調（+/-10V）"
            ]) +
            h3("スケールプリセット") +
            ul([
                "平均律、純正律、ピタゴラス",
                "アラビアマカーム、インドラーガ、ガムランペロッグ",
                "日本雅楽、トルコマカーム、ペルシャダストガー",
                "クォータートーン"
            ]) +
            h3("I/O") +
            ul([
                "3 CV入力と3量子化出力",
                "オフセットCV入力"
            ]) +
            h3("右クリックメニュー") +
            ul([
                f"{b('Scale Preset')}: 16プリセットスケール（Chromatic、Major、Minor、Pentatonic Major/Minor、Dorian、Phrygian、Lydian、Mixolydian、Locrian、Major Triad、Minor Triad、Blues、Arabic、Japanese、Whole Tone）",
                f"{b('Microtune Preset')}: 上記文化チューニングシステム"
            ], last=True)
        ),
        "description_zh": "12 音 Chromatic Quantizer，具備各音符 Microtune 控制與 10 種文化音階預設（Arabic、Indian、Gamelan 等）",
        "description_ja": "ノート別マイクロチューン制御と10種の文化スケールプリセット（アラビア、インド、ガムラン等）を備えた12音クロマチッククオンタイザー",
        "features_zh": [
            "12 音 Chromatic Quantizer",
            "各音符 Microtune 控制",
            "10 種文化音階預設",
            "CV Offset 輸入",
            "4HP 精簡設計"
        ],
        "features_ja": [
            "12音クロマチッククオンタイザー",
            "ノート別マイクロチューン制御",
            "10種の文化スケールプリセット",
            "CVオフセット入力",
            "4HPコンパクト設計"
        ]
    },

    # TWNC - fix incorrect "6軌" description
    "twnc": {
        "description_zh": "雙軌 Euclidean Drum Machine，具備內建 FM 合成、Accent 系統與 10-bit 復古量化特色",
        "description_ja": "内蔵FM合成、アクセントシステム、10ビットビンテージ量子化を備えたデュアルトラックユークリッドドラムマシン",
        "features_zh": [
            "雙軌 Euclidean Drum Machine",
            "內建 FM Drum 合成",
            "四分音符 Accent 系統",
            "10-bit 復古量化",
            "外部輸入混合"
        ],
        "features_ja": [
            "デュアルトラックユークリッドドラムマシン",
            "内蔵FMドラム合成",
            "四分音符アクセントシステム",
            "10ビットビンテージ量子化",
            "外部入力ミキシング"
        ]
    },

    # Pyramid - fix incorrect "binaural" and "4-point" description
    "pyramid": {
        "description_zh": "3D 空間 Panner，具備 X/Y/Z 位置控制、8 聲道 Speaker Output 與所有軸向的 CV 調變",
        "description_ja": "X/Y/Z位置制御、8スピーカー出力、全軸CV変調を備えた3D空間パンナー",
        "features_zh": [
            "XYZ 控制的 3D Panner",
            "8 聲道 Speaker 配置輸出",
            "所有軸向 CV 控制",
            "內建 Bipolar Filter",
            "Pyramid Series 一員"
        ],
        "features_ja": [
            "XYZ制御3Dパンナー",
            "8スピーカー構成出力",
            "全軸CV制御",
            "内蔵バイポーラフィルター",
            "Pyramid Seriesの一部"
        ],
        "manual_zh": (
            h3("概述") +
            p("單聲道 3D Router（6HP）用於空間 Audio 定位。Pyramid Series 一員 - 提供 X/Y/Z 定位含 CV 控制、Bipolar Filtering 與外部效果器 Send/Return。") +
            h3("控制") +
            ul([
                f"{b('X')}: 左右定位（-1.0 至 +1.0）",
                f"{b('Y')}: 前後定位（-1.0 至 +1.0）",
                f"{b('Z')}: 上下定位（-1.0 至 +1.0）",
                f"{b('Level')}: 通道音量（0.0 至 1.0，預設: 0.7）",
                f"{b('Filter')}: Bipolar Biquad Filter 含 CV 輸入（-1.0 至 +1.0）",
                f"{b('Send')}: 效果器 Send Level"
            ]) +
            h3("I/O") +
            ul([
                f"{b('Audio Input')}: 含 Level 控制的 Mono 輸入",
                f"{b('X/Y/Z CV')}: 位置調變（+/-10V）",
                f"{b('Filter CV')}: 含 Attenuator 的 Filter 調變",
                f"{b('8 Output')}: 上層（FL、FR、BL、BR）+ 下層（FL、FR、BL、BR）",
                f"{b('Send/Return')}: 外部效果器的 Stereo Return 輸入"
            ]) +
            h3("規格") +
            ul([
                "Input/Output: +/-10V",
                "處理: 32-bit 浮點",
                "Latency: <1ms"
            ]) +
            h3("右鍵選單") +
            ul([
                f"{b('Send Pre-Level')}: 切換 Send 訊號在 Level Fader 前/後"
            ], last=True)
        ),
        "manual_ja": (
            h3("概要") +
            p("空間オーディオ定位用シングルチャンネル3Dルーター（6HP）。Pyramid Seriesの一部 - CV制御付きX/Y/Z定位、バイポーラフィルタリング、外部エフェクト用Send/Returnを提供。") +
            h3("コントロール") +
            ul([
                f"{b('X')}: 左右定位（-1.0〜+1.0）",
                f"{b('Y')}: 前後定位（-1.0〜+1.0）",
                f"{b('Z')}: 上下定位（-1.0〜+1.0）",
                f"{b('Level')}: チャンネル音量（0.0〜1.0、デフォルト: 0.7）",
                f"{b('Filter')}: CV入力付きバイポーラBiquadフィルター（-1.0〜+1.0）",
                f"{b('Send')}: エフェクトセンドレベル"
            ]) +
            h3("I/O") +
            ul([
                f"{b('Audio Input')}: レベル制御付きモノ入力",
                f"{b('X/Y/Z CV')}: 位置変調（+/-10V）",
                f"{b('Filter CV')}: アッテヌエーター付きフィルター変調",
                f"{b('8 Output')}: 上層（FL、FR、BL、BR）+ 下層（FL、FR、BL、BR）",
                f"{b('Send/Return')}: 外部エフェクト用ステレオリターン入力"
            ]) +
            h3("仕様") +
            ul([
                "入出力: +/-10V",
                "処理: 32ビット浮動小数点",
                "レイテンシー: <1ms"
            ]) +
            h3("右クリックメニュー") +
            ul([
                f"{b('Send Pre-Level')}: センド信号をレベルフェーダー前/後で切替"
            ], last=True)
        )
    },

    # KEN - fix incorrect controls description
    "ken": {
        "manual_zh": (
            h3("概述") +
            p("使用 HRTF（頭部相關傳輸函數）處理的 8-to-2 Binaural Processor，將 8 聲道空間 Audio 轉換為 Stereo 耳機輸出。Pyramid Series 一員 - 接收來自 Pyramid 或 DECAPyramid 模組的空間 Audio。") +
            h3("HRTF 處理") +
            ul([
                f"{b('ITD')}: 各聲道 Delay 的 Interaural Time Difference 模擬",
                f"{b('ILD')}: 位置相關 Gain 的 Interaural Level Difference",
                f"{b('Distance Filtering')}: 基於 Speaker 距離的 Lowpass Filter",
                f"{b('Elevation Filtering')}: 基於垂直位置的頻率造型",
                f"{b('Reverb Filtering')}: 基於距離的 Reverb 模擬"
            ]) +
            h3("Speaker 映射（立方體）") +
            ul([
                f"{b('1-4')}: 上層（FL/FR/BL/BR，仰角 +30°）",
                f"{b('5-8')}: 下層（FL/FR/BL/BR，仰角 -30°）",
                "方位角: +/-45°（前）、+/-135°（後）"
            ]) +
            h3("控制") +
            ul([
                f"{b('Level')}: Output Level（0-100%，預設: 70%）"
            ]) +
            h3("I/O") +
            ul([
                f"{b('Input 1-8')}: 來自 Pyramid/DECAPyramid 的 Speaker 聲道",
                f"{b('L/R Output')}: 耳機用 Binaural Stereo"
            ]) +
            h3("規格") +
            ul([
                "Input/Output: +/-10V",
                "頭部模型寬度: 18cm",
                "ITD Delay Buffer: 128 Samples",
                "模組寬度: 4HP"
            ], last=True)
        ),
        "manual_ja": (
            h3("概要") +
            p("HRTF（頭部伝達関数）処理を使用した8-to-2バイノーラルプロセッサー、8チャンネル空間オーディオをステレオヘッドフォン出力に変換。Pyramid Seriesの一部 - PyramidまたはDECAPyramidモジュールから空間オーディオを受信。") +
            h3("HRTF処理") +
            ul([
                f"{b('ITD')}: チャンネル別ディレイによる両耳間時間差シミュレーション",
                f"{b('ILD')}: 位置ベースゲインによる両耳間音圧差",
                f"{b('Distance Filtering')}: スピーカー距離ベースローパスフィルター",
                f"{b('Elevation Filtering')}: 垂直位置ベース周波数シェイピング",
                f"{b('Reverb Filtering')}: 距離ベースリバーブシミュレーション"
            ]) +
            h3("スピーカーマッピング（キューブ）") +
            ul([
                f"{b('1-4')}: 上層（FL/FR/BL/BR、仰角+30°）",
                f"{b('5-8')}: 下層（FL/FR/BL/BR、仰角-30°）",
                "方位角: +/-45°（前）、+/-135°（後）"
            ]) +
            h3("コントロール") +
            ul([
                f"{b('Level')}: 出力レベル（0-100%、デフォルト: 70%）"
            ]) +
            h3("I/O") +
            ul([
                f"{b('Input 1-8')}: Pyramid/DECAPyramidからのスピーカーチャンネル",
                f"{b('L/R Output')}: ヘッドフォン用バイノーラルステレオ"
            ]) +
            h3("仕様") +
            ul([
                "入出力: +/-10V",
                "頭部モデル幅: 18cm",
                "ITDディレイバッファ: 128サンプル",
                "モジュール幅: 4HP"
            ], last=True)
        )
    },

    # Q_Q - fix incorrect "Quantizer" description
    "q_q": {
        "description_zh": "三軌 Decay Envelope 生成器，具備可調 S-curve Shaping 與即時 3 軌 Oscilloscope 顯示",
        "description_ja": "調整可能なS-curveシェイピングとリアルタイム3トラックオシロスコープ表示を備えたトリプルディケイエンベロープ生成器",
        "features_zh": [
            "三軌 Decay Envelope",
            "即時 Oscilloscope 顯示",
            "可調曲線造型",
            "各軌獨立 Trigger",
            "視覺 Waveform 比較"
        ],
        "features_ja": [
            "3トラックディケイエンベロープ",
            "リアルタイムオシロスコープ表示",
            "調整可能カーブシェイピング",
            "トラック別トリガー",
            "視覚的波形比較"
        ]
    },

    # Runshow - fix incorrect "場景切換" description
    "runshow": {
        "description_zh": "演出計時器，具備即時 Clock、Bar/Beat 計數器、雙 Interval Timer 與四個可變長度 Bar Sequencer",
        "description_ja": "リアルタイムクロック、バー/ビートカウンター、デュアルインターバルタイマー、4つの可変長バーシーケンサーを備えたパフォーマンスタイマー",
        "features_zh": [
            "即時演出計時顯示",
            "Bar:Beat:Tick 計數器",
            "雙 Interval Timer",
            "四個可變長度 Sequencer",
            "5 種 Morphable Waveform Shape"
        ],
        "features_ja": [
            "リアルタイムショータイマー表示",
            "バー:ビート:ティックカウンター",
            "デュアルインターバルタイマー",
            "4つの可変長シーケンサー",
            "5種モーファブル波形シェイプ"
        ]
    }
}

def update_module(module_name, fixes):
    """Update a single module JSON file with fixes."""
    json_path = f"modules/{module_name}.json"

    if not os.path.exists(json_path):
        print(f"Not found: {json_path}")
        return False

    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    # Apply fixes
    for key, value in fixes.items():
        data[key] = value

    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)

    print(f"Updated: {module_name}")
    return True

def main():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    for module_name, fixes in FIXES.items():
        update_module(module_name, fixes)

    print("\nTranslation fixes complete!")

if __name__ == "__main__":
    main()
