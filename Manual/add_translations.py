#!/usr/bin/env python3
"""
為所有模組 JSON 檔案添加中文和日文翻譯
繁體中文保留合成器專有名詞使用英文
"""

import json
import os

# 翻譯資料
TRANSLATIONS = {
    "maddy": {
        "description_zh": "整合 Swing Clock、3 軌 Euclidean Sequencer 與 Pattern CV 生成的節奏工作站",
        "description_ja": "スウィングクロック、3トラックユークリッドシーケンサー、パターンCV生成を統合したリズムワークステーション",
        "features_zh": [
            "內建 Swing Clock 生成器",
            "3 軌 Euclidean Rhythm 引擎",
            "5 步 CV Sequencer，3 種模式",
            "各軌獨立 Clock Division",
            "Pattern 變形與隨機化"
        ],
        "features_ja": [
            "内蔵スウィングクロック生成",
            "3トラックユークリッドリズムエンジン",
            "3モード対応5ステップCVシーケンサー",
            "トラック別クロック分割",
            "パターンモーフィングとランダム化"
        ]
    },
    "maddy_": {
        "description_zh": "MADDY 進階版，具備三組獨立 CV Sequencer，支援右鍵選單自訂 Pattern",
        "description_ja": "3系統の独立CVシーケンサーを搭載したMADDY拡張版、コンテキストメニューでカスタムパターン対応",
        "features_zh": [
            "三組獨立 CV Sequencer",
            "每組 5 個電壓旋鈕",
            "Sequential、Minimalism、Jump 模式",
            "Density 與 Chaos 控制",
            "右鍵選單自訂 Pattern"
        ],
        "features_ja": [
            "3系統の独立CVシーケンサー",
            "各系統5つの電圧ノブ",
            "Sequential、Minimalism、Jumpモード",
            "DensityとChaosコントロール",
            "コンテキストメニューでカスタムパターン"
        ]
    },
    "nigoq": {
        "description_zh": "雙聲道 Filter 模組，具備獨立 Cutoff、Resonance 與多種 Filter 類型選擇",
        "description_ja": "独立カットオフ・レゾナンス制御と複数フィルタータイプを備えたデュアルチャンネルフィルター",
        "features_zh": [
            "雙聲道獨立 Filter 處理",
            "Low-pass、High-pass、Band-pass 類型",
            "獨立 Cutoff CV 調變",
            "可調 Resonance 與 Drive",
            "Stereo Link 模式"
        ],
        "features_ja": [
            "デュアルチャンネル独立フィルター処理",
            "ローパス、ハイパス、バンドパス対応",
            "独立カットオフCV変調",
            "調整可能なレゾナンスとドライブ",
            "ステレオリンクモード"
        ]
    },
    "weiii_documenta": {
        "description_zh": "8 層錄音 Sampler，模擬 No-input Mixer 回授效果，具備即時 Transient 切片、3 段 EQ 與混沌參數變形",
        "description_ja": "ノーインプットミキサーのフィードバックを再現する8レイヤーサンプラー、リアルタイムトランジェントスライス、3バンドEQ、カオスパラメータモーフィング搭載",
        "features_zh": [
            "8 層錄音 Sampler",
            "即時 Transient 切片",
            "混沌參數變形",
            "3 段 EQ 處理",
            "最多 8 聲 Polyphonic Playback"
        ],
        "features_ja": [
            "8レイヤー録音サンプラー",
            "リアルタイムトランジェントスライス",
            "カオスパラメータモーフィング",
            "3バンドEQ処理",
            "最大8ボイスポリフォニック再生"
        ]
    },
    "universal_rhythm": {
        "description_zh": "跨文化節奏生成器，具備 10 種世界音樂風格、4 種節奏角色，以及 8 聲 Polyphonic Drum Synthesis",
        "description_ja": "10種のワールドミュージックスタイル、4つのリズムロール、8ボイスポリフォニックドラム合成を備えたクロスカルチャーリズム生成器",
        "features_zh": [
            "10 種世界音樂節奏風格",
            "每種風格 4 種節奏角色",
            "8 聲 Polyphonic Drum Synthesis",
            "各角色獨立 Density 與 Humanize",
            "即時 Pattern Variation"
        ],
        "features_ja": [
            "10種のワールドミュージックリズムスタイル",
            "スタイル別4つのリズムロール",
            "8ボイスポリフォニックドラム合成",
            "ロール別Density・Humanize",
            "リアルタイムパターンバリエーション"
        ]
    },
    "twnc": {
        "description_zh": "6 軌 Drum Machine，具備 Swing 與 Shuffle 時序控制、Pattern Sequencer 與內建 Drum Synthesis",
        "description_ja": "スウィング・シャッフルタイミング制御、パターンシーケンサー、内蔵ドラム合成を備えた6トラックドラムマシン",
        "features_zh": [
            "6 軌 Drum 合成",
            "Swing 與 Shuffle 時序",
            "16 步 Pattern Sequencer",
            "各軌獨立 Gate Output",
            "Fill 與 Accent 功能"
        ],
        "features_ja": [
            "6トラックドラム合成",
            "スウィング・シャッフルタイミング",
            "16ステップパターンシーケンサー",
            "トラック別ゲート出力",
            "フィル・アクセント機能"
        ]
    },
    "twnc_2": {
        "description_zh": "TWNC 進階版，具備擴展 Pattern 編輯功能、多種 Variation 模式與即時 Pattern 切換",
        "description_ja": "拡張パターン編集、複数バリエーションモード、リアルタイムパターン切替を備えたTWNC拡張版",
        "features_zh": [
            "擴展 Pattern 編輯",
            "多種 Variation 模式",
            "即時 Pattern 切換",
            "Pattern Chain 功能",
            "改進的 Trigger 與 Accent"
        ],
        "features_ja": [
            "拡張パターン編集",
            "複数バリエーションモード",
            "リアルタイムパターン切替",
            "パターンチェーン機能",
            "改良トリガーとアクセント"
        ]
    },
    "twnc_light": {
        "description_zh": "TWNC 精簡版，保留核心功能於更小的面板尺寸",
        "description_ja": "コア機能をコンパクトパネルに凝縮したTWNCライト版",
        "features_zh": [
            "精簡 4 軌 Drum",
            "基本 Pattern Sequencer",
            "Swing 時序控制",
            "混合 Audio Output",
            "節省空間設計"
        ],
        "features_ja": [
            "コンパクト4トラックドラム",
            "基本パターンシーケンサー",
            "スウィングタイミング制御",
            "ミックスオーディオ出力",
            "省スペース設計"
        ]
    },
    "kimo": {
        "description_zh": "Pattern 型 Drum Machine，具備可程式化 Rhythm Pattern、獨立 Drum Synthesis 與 Accent 序列",
        "description_ja": "プログラマブルリズムパターン、独立ドラム合成、アクセントシーケンスを備えたパターン型ドラムマシン",
        "features_zh": [
            "可程式化 Rhythm Pattern",
            "4 聲 Drum Synthesis",
            "各聲獨立 Accent 序列",
            "Pattern Fill 變化",
            "CV 調變輸入"
        ],
        "features_ja": [
            "プログラマブルリズムパターン",
            "4ボイスドラム合成",
            "ボイス別アクセントシーケンス",
            "パターンフィルバリエーション",
            "CV変調入力"
        ]
    },
    "pinpple": {
        "description_zh": "Percussion 合成器，具備多種打擊樂音色、可調 Decay 與 Tune，以及 CV 調變",
        "description_ja": "複数パーカッション音色、調整可能なディケイとチューン、CV変調を備えたパーカッション合成器",
        "features_zh": [
            "多種 Percussion 音色",
            "可調 Decay 時間",
            "Tune 控制",
            "CV 調變輸入",
            "混合 Output"
        ],
        "features_ja": [
            "複数パーカッション音色",
            "調整可能なディケイ",
            "チューンコントロール",
            "CV変調入力",
            "ミックス出力"
        ]
    },
    "swing_lfo": {
        "description_zh": "Swing 時序 LFO，具備可調 Swing 量、多種波形與同步選項",
        "description_ja": "調整可能なスウィング量、複数波形、同期オプションを備えたスウィングタイミングLFO",
        "features_zh": [
            "可調 Swing 時序",
            "多種 LFO 波形",
            "Clock 同步",
            "Rate CV 輸入",
            "雙 Output"
        ],
        "features_ja": [
            "調整可能なスウィングタイミング",
            "複数LFO波形",
            "クロック同期",
            "レートCV入力",
            "デュアル出力"
        ]
    },
    "euclidean_rhythm": {
        "description_zh": "Euclidean Rhythm 生成器，具備可調 Steps、Fills 與 Rotation",
        "description_ja": "調整可能なステップ、フィル、ローテーションを備えたユークリッドリズム生成器",
        "features_zh": [
            "Euclidean Algorithm 節奏",
            "可調 Steps 數量",
            "可調 Fills 密度",
            "Pattern Rotation",
            "多 Output"
        ],
        "features_ja": [
            "ユークリッドアルゴリズムリズム",
            "調整可能なステップ数",
            "調整可能なフィル密度",
            "パターンローテーション",
            "複数出力"
        ]
    },
    "ad_generator": {
        "description_zh": "Attack-Decay Envelope 生成器，具備可調 Attack 與 Decay 時間、曲線形狀選擇",
        "description_ja": "調整可能なアタック・ディケイ時間、カーブ形状選択を備えたADエンベロープ生成器",
        "features_zh": [
            "可調 Attack 時間",
            "可調 Decay 時間",
            "多種曲線形狀",
            "Trigger 與 Gate 輸入",
            "CV Output"
        ],
        "features_ja": [
            "調整可能なアタック時間",
            "調整可能なディケイ時間",
            "複数カーブ形状",
            "トリガー・ゲート入力",
            "CV出力"
        ]
    },
    "q_q": {
        "description_zh": "雙 Quantizer 模組，具備可選音階與 Root Note",
        "description_ja": "選択可能なスケールとルートノートを備えたデュアルクオンタイザー",
        "features_zh": [
            "雙通道 Quantizer",
            "多種音階選擇",
            "可調 Root Note",
            "Trigger Output",
            "CV 直通"
        ],
        "features_ja": [
            "デュアルチャンネルクオンタイザー",
            "複数スケール選択",
            "調整可能なルートノート",
            "トリガー出力",
            "CVスルー"
        ]
    },
    "ppattterning": {
        "description_zh": "Pattern 生成器，具備可程式化序列與隨機化功能",
        "description_ja": "プログラマブルシーケンスとランダム化機能を備えたパターン生成器",
        "features_zh": [
            "可程式化 Pattern 序列",
            "隨機化控制",
            "Clock 同步",
            "多 Output",
            "Pattern 儲存"
        ],
        "features_ja": [
            "プログラマブルパターンシーケンス",
            "ランダム化コントロール",
            "クロック同期",
            "複数出力",
            "パターン保存"
        ]
    },
    "u8": {
        "description_zh": "Stereo Channel Strip，具備自動 Mono-to-Stereo 轉換、Sidechain Ducking 與 Chain Bus 整合",
        "description_ja": "自動モノ→ステレオ変換、サイドチェインダッキング、チェーンバス統合を備えたステレオチャンネルストリップ",
        "features_zh": [
            "8 聲道 Stereo Mixer",
            "20ms Mono-to-Stereo Delay",
            "各聲道 Sidechain Ducking",
            "Chain 輸入擴展",
            "獨立 Level 控制"
        ],
        "features_ja": [
            "8チャンネルステレオミキサー",
            "20msモノ→ステレオディレイ",
            "チャンネル別サイドチェインダッキング",
            "チェーン入力拡張",
            "独立レベルコントロール"
        ]
    },
    "yamanote": {
        "description_zh": "8 聲道 Stereo Mixer，具備雙 Aux Send Bus、Stereo Return 與 Chain I/O 擴展",
        "description_ja": "デュアルAuxセンドバス、ステレオリターン、チェーンI/O拡張を備えた8チャンネルステレオミキサー",
        "features_zh": [
            "8 聲道 Stereo 輸入 Mixer",
            "雙 Aux Send Bus",
            "各聲道 Level 與 Pan",
            "Chain 輸入/輸出擴展",
            "Master Output 含電平表"
        ],
        "features_ja": [
            "8チャンネルステレオ入力ミキサー",
            "デュアルAuxセンドバス",
            "チャンネル別レベル・パン",
            "チェーン入出力拡張",
            "メーター付きマスター出力"
        ]
    },
    "ellen_ripley": {
        "description_zh": "訊號處理工具，具備 Waveshaping、Distortion 與動態處理",
        "description_ja": "ウェーブシェイピング、ディストーション、ダイナミクス処理を備えたシグナル処理ツール",
        "features_zh": [
            "多種 Waveshaping 演算法",
            "可調 Distortion",
            "動態處理",
            "Dry/Wet Mix",
            "CV 調變"
        ],
        "features_ja": [
            "複数ウェーブシェイピングアルゴリズム",
            "調整可能なディストーション",
            "ダイナミクス処理",
            "ドライ/ウェットミックス",
            "CV変調"
        ]
    },
    "env_vca_6": {
        "description_zh": "6 聲道 Envelope 與 VCA 組合模組",
        "description_ja": "6チャンネルエンベロープとVCAの複合モジュール",
        "features_zh": [
            "6 聲道 VCA",
            "各聲道 Envelope",
            "可調 Attack/Decay",
            "CV 調變",
            "混合 Output"
        ],
        "features_ja": [
            "6チャンネルVCA",
            "チャンネル別エンベロープ",
            "調整可能なアタック/ディケイ",
            "CV変調",
            "ミックス出力"
        ]
    },
    "ken": {
        "description_zh": "3D 空間定位模組，具備 Binaural Panning 與 HRTF 處理",
        "description_ja": "バイノーラルパンニングとHRTF処理を備えた3D空間定位モジュール",
        "features_zh": [
            "3D 空間定位",
            "Binaural Panning",
            "HRTF 處理",
            "Azimuth 與 Elevation 控制",
            "Distance 衰減"
        ],
        "features_ja": [
            "3D空間定位",
            "バイノーラルパンニング",
            "HRTF処理",
            "方位角・仰角コントロール",
            "距離減衰"
        ]
    },
    "pyramid": {
        "description_zh": "四面體 3D Panner，具備 4 點空間定位與 Binaural Output",
        "description_ja": "4点空間定位とバイノーラル出力を備えた四面体3Dパンナー",
        "features_zh": [
            "四面體 3D Panning",
            "4 點空間定位",
            "Binaural Output",
            "XYZ 座標控制",
            "CV 調變"
        ],
        "features_ja": [
            "四面体3Dパンニング",
            "4点空間定位",
            "バイノーラル出力",
            "XYZ座標コントロール",
            "CV変調"
        ]
    },
    "decapyramid": {
        "description_zh": "10 聲道 3D Spatial Mixer，具備 Binaural 處理與視覺化 3D 定位顯示",
        "description_ja": "バイノーラル処理と視覚的3D定位表示を備えた10チャンネル3D空間ミキサー",
        "features_zh": [
            "10 聲道 3D Spatial Mixer",
            "Binaural 處理",
            "3D 定位視覺化",
            "各聲道獨立空間位置",
            "Master Binaural Output"
        ],
        "features_ja": [
            "10チャンネル3D空間ミキサー",
            "バイノーラル処理",
            "3D定位視覚化",
            "チャンネル別独立空間位置",
            "マスターバイノーラル出力"
        ]
    },
    "quantizer": {
        "description_zh": "CV Quantizer，具備多種音階選擇與 Root Note 設定",
        "description_ja": "複数スケール選択とルートノート設定を備えたCVクオンタイザー",
        "features_zh": [
            "CV Quantizer",
            "多種音階選擇",
            "可調 Root Note",
            "Trigger Output",
            "Bypass 開關"
        ],
        "features_ja": [
            "CVクオンタイザー",
            "複数スケール選択",
            "調整可能なルートノート",
            "トリガー出力",
            "バイパススイッチ"
        ]
    },
    "runshow": {
        "description_zh": "演出控制模組，具備場景切換與自動化功能",
        "description_ja": "シーン切替と自動化機能を備えたパフォーマンスコントロールモジュール",
        "features_zh": [
            "場景切換控制",
            "自動化序列",
            "Trigger 輸入/輸出",
            "場景儲存",
            "即時控制"
        ],
        "features_ja": [
            "シーン切替コントロール",
            "自動化シーケンス",
            "トリガー入出力",
            "シーン保存",
            "リアルタイムコントロール"
        ]
    },
    "observer": {
        "description_zh": "訊號監測模組，具備電平顯示與視覺化",
        "description_ja": "レベル表示と視覚化を備えたシグナル監視モジュール",
        "features_zh": [
            "訊號電平監測",
            "視覺化顯示",
            "Peak Hold",
            "多通道輸入",
            "即時更新"
        ],
        "features_ja": [
            "シグナルレベル監視",
            "視覚化表示",
            "ピークホールド",
            "マルチチャンネル入力",
            "リアルタイム更新"
        ]
    },
    "obserfour": {
        "description_zh": "4 通道訊號監測模組，具備獨立電平顯示",
        "description_ja": "独立レベル表示を備えた4チャンネルシグナル監視モジュール",
        "features_zh": [
            "4 通道監測",
            "獨立電平顯示",
            "Peak Hold",
            "緊湊面板設計",
            "即時視覺化"
        ],
        "features_ja": [
            "4チャンネル監視",
            "独立レベル表示",
            "ピークホールド",
            "コンパクトパネル設計",
            "リアルタイム視覚化"
        ]
    }
}

def add_translations():
    """為所有模組 JSON 檔案添加翻譯"""
    modules_dir = 'modules'

    for slug, trans in TRANSLATIONS.items():
        json_path = os.path.join(modules_dir, f'{slug}.json')

        if not os.path.exists(json_path):
            print(f"找不到: {json_path}")
            continue

        with open(json_path, 'r', encoding='utf-8') as f:
            module = json.load(f)

        # 添加翻譯
        module['description_zh'] = trans.get('description_zh', module.get('description', ''))
        module['description_ja'] = trans.get('description_ja', module.get('description', ''))
        module['features_zh'] = trans.get('features_zh', module.get('features', []))
        module['features_ja'] = trans.get('features_ja', module.get('features', []))

        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(module, f, ensure_ascii=False, indent=2)

        print(f"已更新: {slug}")

if __name__ == '__main__':
    add_translations()
    print("翻譯添加完成！")
