#!/usr/bin/env python3
"""
MADZINE Manual 合併腳本
將模組資料檔案合併成完整的 HTML 說明書

用法:
    python3 build_manual.py

輸入:
    - modules/*.json  : 模組資料
    - modules/*.img   : 模組圖片 (base64)
    - categories.json : 分類配置

輸出:
    - madzine_modules_compact.html
"""

import json
import os
from datetime import datetime

# 分類配置（多語言）
CATEGORIES = {
    "signature": {
        "name": {
            "en": "Signature Series",
            "zh": "Signature 系列",
            "ja": "代表シリーズ"
        },
        "intro": {
            "en": "Distinctive modules showcasing MADZINE's unique approach to synthesis and signal processing.",
            "zh": "展現 MADZINE 獨特合成與訊號處理風格的模組。",
            "ja": "MADZINEならではの合成とシグナル処理を実現する代表的モジュール。"
        },
        "modules": ["maddy", "maddy_", "nigoq", "weiii_documenta", "universal_rhythm"]
    },
    "drum": {
        "name": {
            "en": "Drum Machines",
            "zh": "Drum Machine 鼓機",
            "ja": "ドラムマシン"
        },
        "intro": {
            "en": "The TWNC series combines <span style='color:#f0a000;'>Euclidean Rhythm</span> with a triple-envelope system for drum synthesis. Each hit triggers an <span style='color:#f0a000;'>FM envelope</span> for pitch sweep and a <span style='color:#f0a000;'>Track VCA envelope</span> for amplitude shaping. A slower <span style='color:#4a4;'>Accent VCA envelope</span> running at 1/4 clock speed gates the output, creating delay and rumble textures. The <span style='color:#4a4;'>Accent parameter</span> offsets the Accent VCA's starting beat, working with <span style='color:#f0a000;'>Euclidean Fill</span> to generate varied rhythmic patterns.<br><br><div style='display:flex; gap:0.3rem;'><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig1.png' style='width:50%;'><small><span style='color:#f0a000;'>FM</span>/<span style='color:#f0a000;'>VCA Env</span> vs.<br><span style='color:#4a4;'>Accent Env</span></small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig2.png' style='width:50%;'><small><span style='color:#f0a000;'>Euclidean</span><br>80% Fill Pattern</small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig3.png' style='width:50%;'><small>80% Fill +<br><span style='color:#4a4;'>Accent Shift</span></small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig4.png' style='width:50%;'><small><span style='color:#4a4;'>Accent VCA</span><br>as Rumble Effect</small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig5.png' style='width:50%;'><small>Rhythm Using<br><span style='color:#f0a000;'>Fill</span> + <span style='color:#4a4;'>Accent</span></small></div></div>",
            "zh": "TWNC 系列結合 <span style='color:#f0a000;'>Euclidean Rhythm</span> 與三層 Envelope 架構。每次觸發時，<span style='color:#f0a000;'>FM Envelope</span> 控制 Pitch Sweep，<span style='color:#f0a000;'>Track VCA Envelope</span> 控制音量變化。<span style='color:#4a4;'>Accent VCA Envelope</span> 以 1/4 Clock 速度運作，週期性地控制音量，營造類似 Delay 與 Rumble 的質感。<span style='color:#4a4;'>Accent 參數</span>可調整 Accent VCA 的起始位置，搭配 <span style='color:#f0a000;'>Euclidean Fill</span> 創造豐富的節奏變化。<br><br><div style='display:flex; gap:0.3rem;'><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig1.png' style='width:50%;'><small><span style='color:#f0a000;'>FM</span>/<span style='color:#f0a000;'>VCA Env</span> vs.<br><span style='color:#4a4;'>Accent Env</span></small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig2.png' style='width:50%;'><small><span style='color:#f0a000;'>Euclidean</span><br>80% Fill Pattern</small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig3.png' style='width:50%;'><small>80% Fill +<br><span style='color:#4a4;'>Accent Shift</span></small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig4.png' style='width:50%;'><small><span style='color:#4a4;'>Accent VCA</span><br>Rumble 效果</small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig5.png' style='width:50%;'><small><span style='color:#f0a000;'>Fill</span> + <span style='color:#4a4;'>Accent</span><br>節奏範例</small></div></div>",
            "ja": "TWNC シリーズは <span style='color:#f0a000;'>Euclidean Rhythm</span> と3層の Envelope を組み合わせたドラムシンセ。トリガーごとに <span style='color:#f0a000;'>FM Envelope</span> が Pitch Sweep を、<span style='color:#f0a000;'>Track VCA Envelope</span> が音量変化を制御。<span style='color:#4a4;'>Accent VCA Envelope</span> は 1/4 Clock 速度で動作し、周期的に音量を制御し、Delay や Rumble のような質感を生成。<span style='color:#4a4;'>Accent パラメータ</span>で Accent VCA の開始位置を調整し、<span style='color:#f0a000;'>Euclidean Fill</span> と組み合わせて多彩なリズムパターンを作成。<br><br><div style='display:flex; gap:0.3rem;'><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig1.png' style='width:50%;'><small><span style='color:#f0a000;'>FM</span>/<span style='color:#f0a000;'>VCA Env</span> vs.<br><span style='color:#4a4;'>Accent Env</span></small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig2.png' style='width:50%;'><small><span style='color:#f0a000;'>Euclidean</span><br>80% Fill Pattern</small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig3.png' style='width:50%;'><small>80% Fill +<br><span style='color:#4a4;'>Accent Shift</span></small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig4.png' style='width:50%;'><small><span style='color:#4a4;'>Accent VCA</span><br>Rumble 効果</small></div><div style='display:flex; align-items:center; gap:0.3rem;'><img src='img/twnc_fig5.png' style='width:50%;'><small><span style='color:#f0a000;'>Fill</span> + <span style='color:#4a4;'>Accent</span><br>リズム例</small></div></div>"
        },
        "modules": ["twnc", "twnc_2", "twnc_light", "kimo", "pinpple"]
    },
    "rhythm": {
        "name": {
            "en": "Rhythm & Envelope",
            "zh": "Rhythm 與 Envelope",
            "ja": "リズム＆エンベロープ"
        },
        "intro": {
            "en": "Rhythm generation and envelope tools.",
            "zh": "節奏生成與 Envelope 工具。",
            "ja": "リズム生成とエンベロープツール。"
        },
        "modules": ["swing_lfo", "euclidean_rhythm", "ad_generator", "q_q", "ppattterning"]
    },
    "mixer": {
        "name": {
            "en": "Mixer & Signal Processing",
            "zh": "Mixer 與訊號處理",
            "ja": "ミキサー＆シグナル処理"
        },
        "intro": {
            "en": "Mixing, routing and signal processing utilities.",
            "zh": "混音、Routing 與訊號處理工具。",
            "ja": "ミキシング、ルーティング、シグナル処理ツール。"
        },
        "modules": ["u8", "yamanote", "ellen_ripley", "env_vca_6"]
    },
    "pyramid": {
        "name": {
            "en": "3D Spatial Audio",
            "zh": "3D 空間音訊",
            "ja": "3D空間オーディオ"
        },
        "intro": {
            "en": "3D panning and binaural processing.",
            "zh": "3D Panning 與 Binaural 處理。",
            "ja": "3Dパンニングとバイノーラル処理。"
        },
        "modules": ["ken", "pyramid", "decapyramid"]
    },
    "utility": {
        "name": {
            "en": "Utility",
            "zh": "Utility 工具",
            "ja": "ユーティリティ"
        },
        "intro": {
            "en": "General utility modules.",
            "zh": "工具模組。",
            "ja": "汎用ツールモジュール。"
        },
        "modules": ["quantizer", "runshow", "observer", "obserfour"]
    }
}

HTML_HEAD = '''<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>MADZINE Modules</title>
  <style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
  font-family: 'Segoe UI', sans-serif;
  background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
  color: #eaeaea;
  line-height: 1.5;
  font-size: 1.5rem;
}
header {
  background: rgba(15,15,35,0.95);
  padding: 1.5rem;
  border-bottom: 2px solid #ff6b9d;
}
.header-content {
  max-width: 1600px;
  margin: 0 auto;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 1rem;
}
.header-title {
  text-align: left;
}
header h1 { font-size: 3.75rem; color: #FFC800; letter-spacing: 4px; }
header p { color: #c0c0c0; }
.lang-selector {
  display: flex;
  gap: 0.5rem;
}
.lang-btn {
  padding: 0.5rem 1rem;
  border: 2px solid #2a2a4a;
  background: transparent;
  color: #eaeaea;
  cursor: pointer;
  border-radius: 25px;
  font-size: 0.9rem;
  transition: all 0.3s ease;
}
.lang-btn:hover {
  border-color: #ff6b9d;
  color: #ff6b9d;
}
.lang-btn.active {
  background: #ff6b9d;
  border-color: #ff6b9d;
  color: white;
}
main { max-width: 100%; margin: 0; padding: 1rem 0.5rem; }

.category {
  background: #0f0f23;
  border-radius: 16px;
  margin-bottom: 2rem;
  border: 1px solid #2a2a4a;
  overflow: hidden;
}
.cat-header {
  background: linear-gradient(135deg, #ff6b9d 0%, #ff8fab 100%);
  padding: 1rem 1.5rem;
}
.cat-header h2 {
  display: flex;
  justify-content: space-between;
  font-size: 2.25rem;
}
.cat-header .ja { font-size: 1.5rem; opacity: 0.9; }
.cat-intro {
  padding: 1rem 1.5rem;
  background: rgba(255,255,255,0.02);
  border-bottom: 1px solid #2a2a4a;
}
.cat-intro p { color: #c0c0c0; margin-bottom: 0.3rem; font-size: 1.2rem; }

.grid {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  gap: 1rem;
  padding: 1rem;
}
@media (max-width: 1600px) { .grid { grid-template-columns: repeat(4, 1fr); } }
@media (max-width: 1200px) { .grid { grid-template-columns: repeat(3, 1fr); } }
@media (max-width: 900px) { .grid { grid-template-columns: repeat(2, 1fr); } }
@media (max-width: 600px) { .grid { grid-template-columns: 1fr; } }

.card {
  background: rgba(255,255,255,0.03);
  border-radius: 10px;
  border: 1px solid #2a2a4a;
  overflow: hidden;
  transition: all 0.25s ease;
}
.card:hover {
  transform: translateY(-5px) scale(1.02);
  box-shadow: 0 15px 40px rgba(255,107,157,0.25);
  border-color: #ff6b9d;
}
.card-top {
  display: flex;
  gap: 0.8rem;
  padding: 0.8rem;
}
.card-img img {
  height: 180px;
  width: auto;
  border-radius: 6px;
}
.card-info { flex: 1; }
.card-info h3 { font-size: 1.5rem; color: #FFC800; margin-bottom: 0.3rem; }
.card-info .hp { font-size: 0.85rem; color: #ff6b9d; display: block; margin-bottom: 0.4rem; }
.card-info p { font-size: 0.95rem; color: #c0c0c0; }
.features { padding: 0.5rem 0.8rem 0.8rem; }
.features h4 { font-size: 0.9rem; color: #ff6b9d; margin-bottom: 0.3rem; }
.features ul { padding-left: 1rem; }
.features li { font-size: 0.85rem; color: #c0c0c0; margin-bottom: 0.2rem; }

.wide-card {
  grid-column: span 1;
}
.wide-card .card-top {
  position: relative;
  flex-direction: row;
  align-items: flex-start;
}
.wide-card .card-img {
  position: relative;
  flex-shrink: 0;
}
.wide-card .card-img::after {
  content: '';
  position: absolute;
  top: 0;
  left: 20%;
  right: 0;
  bottom: 0;
  background: linear-gradient(to right, rgba(15, 15, 35, 0) 0%, rgba(15, 15, 35, 0.85) 30%, rgba(15, 15, 35, 0.95) 100%);
  border-radius: 0 8px 8px 0;
  pointer-events: none;
}
.wide-card .card-img img {
  height: 160px;
  width: auto;
}
.wide-card .card-info {
  position: absolute;
  top: 0.8rem;
  left: 25%;
  right: 0.8rem;
  bottom: 0;
  text-align: left;
  padding: 0;
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  z-index: 1;
}
.wide-card .features {
  margin-top: -0.5rem;
}

footer {
  text-align: center;
  padding: 2rem;
  color: #666;
  font-size: 0.9rem;
}
  </style>
</head>
<body>
  <header>
    <div class="header-content">
      <div class="header-title">
        <h1>MADZINE</h1>
        <p data-i18n="header.subtitle">VCV Rack Modules</p>
      </div>
      <div class="lang-selector">
        <button class="lang-btn active" data-lang="en">English</button>
        <button class="lang-btn" data-lang="zh">中文</button>
        <button class="lang-btn" data-lang="ja">日本語</button>
      </div>
    </div>
  </header>
  <main>
'''

HTML_FOOT = '''
  </main>
  <footer>
    <p data-i18n="footer.text">MADZINE VCV Rack Modules</p>
  </footer>

  <!-- Modal -->
  <div id="modal" class="modal">
    <div class="modal-content">
      <span class="modal-close">&times;</span>
      <button class="modal-nav modal-prev">&#10094;</button>
      <button class="modal-nav modal-next">&#10095;</button>
      <div class="modal-body">
        <div class="modal-left">
          <div class="modal-img"></div>
          <div class="modal-overview"></div>
        </div>
        <div class="modal-info">
          <h2 class="modal-title"></h2>
          <span class="modal-hp"></span>
          <div class="card-stack">
            <div class="stacked-card card-front active" data-card="manual">
              <div class="card-tab">Manual</div>
              <div class="modal-manual"></div>
            </div>
            <div class="stacked-card card-back" data-card="quickref">
              <div class="card-tab" data-i18n="label.quickref">Quick Reference</div>
              <div class="modal-quickref"></div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <style>
.modal {
  display: none;
  position: fixed;
  z-index: 1000;
  left: 0;
  top: 0;
  width: 100%;
  height: 100%;
  background: rgba(0,0,0,0.85);
  backdrop-filter: blur(5px);
}
.modal-content {
  position: relative;
  background: #0f0f23;
  margin: 2% auto;
  padding: 1.5rem;
  width: 95vw;
  max-width: 1600px;
  max-height: 90vh;
  border-radius: 20px;
  border: 2px solid #ff6b9d;
  box-shadow: 0 20px 60px rgba(255,107,157,0.4);
  overflow: hidden;
}
.modal-close {
  position: absolute;
  top: 1rem;
  right: 1.5rem;
  font-size: 2rem;
  color: #ff6b9d;
  cursor: pointer;
  transition: color 0.2s;
  z-index: 10;
}
.modal-close:hover { color: #FFC800; }
.modal-nav {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  background: rgba(255,107,157,0.3);
  border: 2px solid #ff6b9d;
  color: #fff;
  font-size: 1.5rem;
  padding: 1rem 0.8rem;
  cursor: pointer;
  border-radius: 8px;
  transition: all 0.2s;
  z-index: 10;
}
.modal-nav:hover {
  background: #ff6b9d;
  color: #000;
}
.modal-prev { left: -60px; }
.modal-next { right: -60px; }
@media (max-width: 1100px) {
  .modal-prev { left: 0.5rem; }
  .modal-next { right: 0.5rem; }
}
.modal-body {
  display: grid;
  grid-template-columns: auto 1fr;
  gap: 1.5rem;
  align-items: start;
  max-height: calc(90vh - 4rem);
  overflow: hidden;
}
.modal-body.wide-layout {
  grid-template-columns: auto 1fr;
}
.modal-body.wide-layout .modal-img img {
  height: auto;
  max-height: calc(54vh - 3rem);
  max-width: 30vw;
}
.modal-img img {
  height: 70vh;
  max-height: 600px;
  width: auto;
  border-radius: 12px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.5);
}
.modal-left {
  display: flex;
  flex-direction: column;
  gap: 1rem;
  flex-shrink: 0;
}
.modal-overview {
  display: none;
  color: #d0d0d0;
  font-size: 0.95rem;
  line-height: 1.6;
  padding: 1rem;
  background: rgba(0,0,0,0.3);
  border-radius: 12px;
  border-left: 3px solid #ff6b9d;
  max-width: 30vw;
}
.modal-overview h3 {
  color: #ff6b9d;
  margin-bottom: 0.5rem;
  font-size: 1.1rem;
}
.modal-overview p {
  margin: 0;
}
.modal-info {
  overflow-y: auto;
  max-height: calc(90vh - 5rem);
  padding-right: 1rem;
}
.modal-title {
  font-size: 2rem;
  color: #FFC800;
  margin-bottom: 0.5rem;
}
.modal-hp {
  display: block;
  color: #ff6b9d;
  font-size: 1rem;
  margin-bottom: 1rem;
}
.modal-manual {
  color: #d0d0d0;
  font-size: 1rem;
  line-height: 1.6;
  column-count: 2;
  column-gap: 3rem;
}
.modal-manual .section-block {
  break-inside: avoid;
  margin-bottom: 1rem;
}
.modal-manual h3 {
  color: #ff6b9d;
  margin-bottom: 0.8rem;
  border-bottom: 2px solid #2a2a4a;
  padding-bottom: 0.5rem;
}
.modal-manual ul, .modal-manual p {
  break-inside: avoid;
}
.modal-manual p {
  margin-bottom: 1rem;
}
.modal-manual ul {
  padding-left: 1.5rem;
  margin-bottom: 1rem;
}
.modal-manual li {
  margin-bottom: 0.3rem;
}
.modal-manual b {
  color: #FFC800;
}
@media (max-width: 700px) {
  .modal-body { flex-direction: column; align-items: center; }
  .modal-img img { max-height: 250px; }
}

/* Card Stack for two-card layout */
.card-stack {
  position: relative;
  perspective: 1000px;
}
.card-stack.has-quickref {
  min-height: 100%;
}
.stacked-card {
  position: relative;
  background: #0f0f23;
  border-radius: 12px;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
}
.stacked-card.card-front {
  z-index: 2;
}
.card-stack.has-quickref .stacked-card.card-front {
  border: 2px solid #ff6b9d;
  box-shadow: 4px 4px 20px rgba(255, 107, 157, 0.3);
  transform: scale(0.92);
  transform-origin: top left;
}
.stacked-card.card-back.active {
  width: auto;
  height: auto;
  transform: scale(0.92);
  transform-origin: top left;
}
.stacked-card.card-back {
  position: absolute;
  top: 25px;
  left: 25px;
  width: 92%;
  height: 92%;
  z-index: 1;
  background: #1a1a2e;
  border: 2px solid #4a9eff;
  box-shadow: 4px 4px 20px rgba(74, 158, 255, 0.3);
  overflow: visible;
  cursor: pointer;
  display: none;
  transform: translate(5px, 5px);
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}
.card-stack.has-quickref .stacked-card.card-back {
  display: block;
}
.stacked-card.card-back .card-tab {
  position: absolute;
  right: 0;
  top: 150px;
  transform: rotate(90deg) translateY(-100%);
  transform-origin: right top;
  background: #4a9eff;
  color: #000;
  padding: 0.5rem 1rem;
  font-weight: 600;
  font-size: 0.9rem;
  border-radius: 8px 8px 0 0;
  white-space: nowrap;
  box-shadow: 0 0 10px rgba(74, 158, 255, 0.5);
  animation: tab-pulse 2s ease-in-out infinite;
}
@keyframes tab-pulse {
  0%, 100% { box-shadow: 0 0 10px rgba(74, 158, 255, 0.5); }
  50% { box-shadow: 0 0 20px rgba(74, 158, 255, 0.8); }
}
.stacked-card.card-back:not(.active) .card-tab {
  display: block;
}
.stacked-card.card-back:hover {
  transform: translate(15px, 15px);
  box-shadow: 8px 8px 30px rgba(74, 158, 255, 0.5);
}
.stacked-card.card-back:hover .card-tab {
  background: #FFC800;
  animation: none;
}
/* When back card is active (brought to front) */
.stacked-card.card-back.active {
  position: relative;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 2;
  cursor: default;
  transform: scale(0.92);
  transform-origin: top left;
  border: 2px solid #4a9eff;
  box-shadow: 4px 4px 20px rgba(74, 158, 255, 0.3);
}
.stacked-card.card-back.active .card-tab {
  display: none;
}
.stacked-card.card-back.active .modal-quickref {
  display: block;
  column-count: 2;
  column-gap: 3rem;
}
.card-stack.has-quickref .stacked-card.card-front.inactive {
  position: absolute;
  top: 12px;
  left: 12px;
  right: -12px;
  bottom: -12px;
  z-index: 1;
  background: #1a1a2e;
  border: 2px solid #ff6b9d;
  box-shadow: 4px 4px 20px rgba(255, 107, 157, 0.3);
  overflow: visible;
  cursor: pointer;
  transform: scale(0.92) translate(3px, 3px);
  transform-origin: top left;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}
.stacked-card.card-front.inactive .card-tab {
  position: absolute;
  right: 0;
  top: 150px;
  transform: rotate(90deg) translateY(-100%);
  transform-origin: right top;
  background: #ff6b9d;
  color: #000;
  padding: 0.5rem 1rem;
  font-weight: 600;
  font-size: 0.9rem;
  border-radius: 8px 8px 0 0;
  white-space: nowrap;
  display: block;
  box-shadow: 0 0 10px rgba(255, 107, 157, 0.5);
  animation: tab-pulse-pink 2s ease-in-out infinite;
}
@keyframes tab-pulse-pink {
  0%, 100% { box-shadow: 0 0 10px rgba(255, 107, 157, 0.5); }
  50% { box-shadow: 0 0 20px rgba(255, 107, 157, 0.8); }
}
.card-stack.has-quickref .stacked-card.card-front.inactive:hover {
  transform: scale(0.92) translate(25px, 25px);
  box-shadow: 8px 8px 30px rgba(255, 107, 157, 0.5);
}
.stacked-card.card-front.inactive:hover .card-tab {
  background: #FFC800;
  animation: none;
}
.stacked-card.card-front.inactive .modal-manual {
  display: none;
}
.stacked-card .card-tab {
  display: none;
}
.modal-quickref {
  display: none;
  color: #d0d0d0;
  font-size: 1rem;
  line-height: 1.6;
}
.modal-quickref h3 {
  color: #4a9eff;
  margin-bottom: 0.8rem;
  border-bottom: 2px solid #2a2a4a;
  padding-bottom: 0.5rem;
}
.modal-quickref table {
  width: 100%;
  border-collapse: collapse;
  margin-bottom: 1rem;
}
.modal-quickref td {
  padding: 0.4rem;
  border-bottom: 1px solid #2a2a4a;
}
  </style>

  <script>
// Translations
const translations = TRANSLATIONS_PLACEHOLDER;

let currentLang = 'en';

// Language Switcher
document.querySelectorAll('.lang-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.lang-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    currentLang = btn.dataset.lang;
    updateLanguage();
    // Re-render modal if open
    if (modal.style.display === 'block') {
      showCard(currentIndex);
    }
  });
});

function updateLanguage() {
  // Update text content
  document.querySelectorAll('[data-i18n]').forEach(el => {
    const key = el.dataset.i18n;
    if (translations[currentLang] && translations[currentLang][key]) {
      el.textContent = translations[currentLang][key];
    }
  });
  // Update HTML content (for manual sections)
  document.querySelectorAll('[data-i18n-html]').forEach(el => {
    const key = el.dataset.i18nHtml;
    if (translations[currentLang] && translations[currentLang][key]) {
      el.innerHTML = translations[currentLang][key];
    }
  });
}

// Modal functionality
(function() {
  const modal = document.getElementById('modal');
  const closeBtn = modal.querySelector('.modal-close');
  const prevBtn = modal.querySelector('.modal-prev');
  const nextBtn = modal.querySelector('.modal-next');
  const cards = Array.from(document.querySelectorAll('.card'));
  window.currentIndex = 0;

  window.showCard = function(index) {
    if (index < 0) index = cards.length - 1;
    if (index >= cards.length) index = 0;
    window.currentIndex = index;

    const card = cards[index];
    const img = card.querySelector('.card-img img');
    const name = card.querySelector('.card-info h3');
    const hp = card.querySelector('.card-info .hp');
    const manual = card.querySelector('.manual-content');
    const slug = card.dataset.slug;
    const isWide = card.dataset.wide === 'true';

    const modalBody = modal.querySelector('.modal-body');
    if (isWide) {
      modalBody.classList.add('wide-layout');
    } else {
      modalBody.classList.remove('wide-layout');
    }

    modal.querySelector('.modal-img').innerHTML = img ? `<img src="${img.src}" alt="${img.alt}">` : '';
    modal.querySelector('.modal-title').textContent = name ? name.textContent : '';
    modal.querySelector('.modal-hp').textContent = hp ? hp.textContent : '';

    // Get translated manual content
    const manualKey = `mod.${slug}.manual`;
    const manualEl = modal.querySelector('.modal-manual');
    if (translations[currentLang] && translations[currentLang][manualKey]) {
      manualEl.innerHTML = translations[currentLang][manualKey];
    } else {
      manualEl.innerHTML = manual ? manual.innerHTML : '';
    }
    // Wrap h3 + following content into section-block divs
    wrapSections(manualEl);

    // Handle quickref for modules with two-card layout
    const cardStack = modal.querySelector('.card-stack');
    const quickrefEl = modal.querySelector('.modal-quickref');
    const cardFront = modal.querySelector('.stacked-card.card-front');
    const cardBack = modal.querySelector('.stacked-card.card-back');

    // Check for quickref content
    const quickrefKey = `mod.${slug}.quickref`;
    let hasQuickref = false;

    if (translations[currentLang] && translations[currentLang][quickrefKey]) {
      quickrefEl.innerHTML = translations[currentLang][quickrefKey];
      hasQuickref = true;
    } else {
      quickrefEl.innerHTML = '';
    }

    // Handle overview section
    const overviewEl = modal.querySelector('.modal-overview');

    // Extract overview from manual and move to left column (for 40HP modules)
    if (isWide) {
      const firstH3 = manualEl.querySelector('h3');
      const firstP = manualEl.querySelector('p');
      if (firstH3 && firstP) {
        overviewEl.innerHTML = `<h3>${firstH3.textContent}</h3><p>${firstP.textContent}</p>`;
        overviewEl.style.display = 'block';
        // Remove overview section from manual
        if (firstH3.parentElement && firstH3.parentElement.classList.contains('section-block')) {
          firstH3.parentElement.remove();
        } else {
          firstH3.remove();
          firstP.remove();
        }
      }
    } else {
      overviewEl.style.display = 'none';
      overviewEl.innerHTML = '';
    }

    // Toggle card stack mode
    if (hasQuickref) {
      cardStack.classList.add('has-quickref');
    } else {
      cardStack.classList.remove('has-quickref');
    }
    // Always reset to default state (manual in front)
    cardFront.classList.add('active');
    cardFront.classList.remove('inactive');
    cardBack.classList.remove('active');
  }

  // Card stack click handlers
  document.addEventListener('click', function(e) {
    const cardBack = e.target.closest('.stacked-card.card-back:not(.active)');
    const cardFront = e.target.closest('.stacked-card.card-front.inactive');

    if (cardBack) {
      // Bring quickref to front
      const stack = cardBack.closest('.card-stack');
      const front = stack.querySelector('.card-front');
      front.classList.remove('active');
      front.classList.add('inactive');
      cardBack.classList.add('active');
    } else if (cardFront) {
      // Bring manual to front
      const stack = cardFront.closest('.card-stack');
      const back = stack.querySelector('.card-back');
      back.classList.remove('active');
      cardFront.classList.remove('inactive');
      cardFront.classList.add('active');
    }
  });

  function wrapSections(container) {
    const children = Array.from(container.childNodes);
    container.innerHTML = '';
    let currentBlock = null;

    children.forEach(node => {
      if (node.nodeType === 1 && node.tagName === 'H3') {
        // Start new block
        if (currentBlock) container.appendChild(currentBlock);
        currentBlock = document.createElement('div');
        currentBlock.className = 'section-block';
        currentBlock.appendChild(node);
      } else if (currentBlock) {
        currentBlock.appendChild(node);
      } else {
        container.appendChild(node);
      }
    });
    if (currentBlock) container.appendChild(currentBlock);
  }

  function openModal(index) {
    showCard(index);
    modal.style.display = 'block';
    document.body.style.overflow = 'hidden';
  }

  function closeModal() {
    modal.style.display = 'none';
    document.body.style.overflow = '';
  }

  cards.forEach((card, index) => {
    card.style.cursor = 'pointer';
    card.addEventListener('click', () => openModal(index));
  });

  closeBtn.addEventListener('click', closeModal);
  prevBtn.addEventListener('click', () => showCard(window.currentIndex - 1));
  nextBtn.addEventListener('click', () => showCard(window.currentIndex + 1));

  modal.addEventListener('click', (e) => {
    if (e.target === modal) closeModal();
  });

  document.addEventListener('keydown', (e) => {
    if (modal.style.display !== 'block') return;
    if (e.key === 'Escape') closeModal();
    if (e.key === 'ArrowLeft') showCard(window.currentIndex - 1);
    if (e.key === 'ArrowRight') showCard(window.currentIndex + 1);
  });
})();

// Initialize language
updateLanguage();
  </script>
</body>
</html>
'''

def load_module(slug):
    """載入模組資料和圖片"""
    json_path = f'modules/{slug}.json'
    img_path = f'modules/{slug}.img'

    if not os.path.exists(json_path):
        print(f"警告: 找不到 {json_path}")
        return None

    with open(json_path, 'r', encoding='utf-8') as f:
        module = json.load(f)

    if os.path.exists(img_path):
        with open(img_path, 'r', encoding='utf-8') as f:
            module['image'] = f.read()

    return module

def render_card(module, slug):
    """渲染單個模組卡片"""
    wide_class = ' wide-card' if module.get('wide') else ''
    image = module.get('image', '')
    alt = module.get('alt', module.get('name', ''))
    name = module.get('name', '')
    hp = module.get('hp', '')
    desc = module.get('description', '')
    features = module.get('features', [])
    manual = module.get('manual', '')

    features_html = ''
    if features:
        items = ''.join(f'<li data-i18n="mod.{slug}.feat{i}">{f}</li>' for i, f in enumerate(features))
        features_html = f'''
      <div class="features">
        <div class="feat-col"><h4 data-i18n="label.features">Features</h4><ul>{items}</ul></div>
        </div>'''

    # Escape manual for safe embedding
    manual_escaped = manual.replace('`', '\\`').replace('$', '\\$')

    wide_attr = ' data-wide="true"' if module.get('wide') else ''
    return f'''
    <div class="card{wide_class}"{wide_attr} data-slug="{slug}">
      <div class="card-top">
        <div class="card-img"><img src="{image}" alt="{alt}"></div>
        <div class="card-info">
          <h3>{name}</h3>
          <span class="hp">{hp}</span>
          <p data-i18n="mod.{slug}.desc">{desc}</p>
        </div>
      </div>{features_html}
      <div class="manual-content" style="display:none;" data-i18n-html="mod.{slug}.manual">{manual}</div>
    </div>'''

def render_category(cat_id, cat_data):
    """渲染分類區塊"""
    name_data = cat_data.get('name', {})
    intro_data = cat_data.get('intro', {})
    modules = cat_data.get('modules', [])

    # 使用英文作為預設顯示
    name_en = name_data.get('en', '') if isinstance(name_data, dict) else name_data
    intro_en = intro_data.get('en', '') if isinstance(intro_data, dict) else intro_data

    cards_html = ''
    for slug in modules:
        module = load_module(slug)
        if module:
            cards_html += render_card(module, slug)

    return f'''
  <section class="category" id="{cat_id}">
    <div class="cat-header">
      <h2 data-i18n="cat.{cat_id}.name">{name_en}</h2>
    </div>
    <div class="cat-intro">
      <p data-i18n-html="cat.{cat_id}.intro"></p>
    </div>
    <div class="grid">{cards_html}
    </div>
  </section>'''

def build_translations():
    """建立翻譯資料"""
    translations = {
        'en': {
            'header.subtitle': 'VCV Rack Modules',
            'footer.text': 'MADZINE VCV Rack Modules',
            'label.features': 'Features',
            'label.quickref': 'Quick Reference'
        },
        'zh': {
            'header.subtitle': 'VCV Rack 模組',
            'footer.text': 'MADZINE VCV Rack 模組',
            'label.features': '功能特色',
            'label.quickref': '快速參考'
        },
        'ja': {
            'header.subtitle': 'VCV Rack モジュール',
            'footer.text': 'MADZINE VCV Rack モジュール',
            'label.features': '機能',
            'label.quickref': 'クイックリファレンス'
        }
    }

    # 加入分類翻譯
    for cat_id, cat_data in CATEGORIES.items():
        name_data = cat_data.get('name', {})
        intro_data = cat_data.get('intro', {})
        if isinstance(name_data, dict):
            for lang in ['en', 'zh', 'ja']:
                translations[lang][f'cat.{cat_id}.name'] = name_data.get(lang, name_data.get('en', ''))
                translations[lang][f'cat.{cat_id}.intro'] = intro_data.get(lang, intro_data.get('en', ''))

    # 加入模組翻譯
    all_slugs = []
    for cat_data in CATEGORIES.values():
        all_slugs.extend(cat_data.get('modules', []))

    for slug in all_slugs:
        module = load_module(slug)
        if not module:
            continue

        # 英文使用原始資料
        translations['en'][f'mod.{slug}.desc'] = module.get('description', '')
        translations['en'][f'mod.{slug}.manual'] = module.get('manual', '')
        features = module.get('features', [])
        for i, feat in enumerate(features):
            translations['en'][f'mod.{slug}.feat{i}'] = feat

        # 加入 quickref（如果有的話）
        if module.get('quickref'):
            translations['en'][f'mod.{slug}.quickref'] = module.get('quickref', '')
            translations['zh'][f'mod.{slug}.quickref'] = module.get('quickref_zh', module.get('quickref', ''))
            translations['ja'][f'mod.{slug}.quickref'] = module.get('quickref_ja', module.get('quickref', ''))

        # 中文與日文翻譯（從 module 的 translations 欄位讀取，若無則使用英文）
        desc_zh = module.get('description_zh', module.get('description', ''))
        desc_ja = module.get('description_ja', module.get('description', ''))
        manual_zh = module.get('manual_zh', module.get('manual', ''))
        manual_ja = module.get('manual_ja', module.get('manual', ''))
        features_zh = module.get('features_zh', features)
        features_ja = module.get('features_ja', features)

        translations['zh'][f'mod.{slug}.desc'] = desc_zh
        translations['zh'][f'mod.{slug}.manual'] = manual_zh
        translations['ja'][f'mod.{slug}.desc'] = desc_ja
        translations['ja'][f'mod.{slug}.manual'] = manual_ja

        for i in range(len(features)):
            translations['zh'][f'mod.{slug}.feat{i}'] = features_zh[i] if i < len(features_zh) else features[i]
            translations['ja'][f'mod.{slug}.feat{i}'] = features_ja[i] if i < len(features_ja) else features[i]

    return translations

def build():
    """建置完整 HTML"""
    html = HTML_HEAD

    for cat_id, cat_data in CATEGORIES.items():
        html += render_category(cat_id, cat_data)

    # 建立翻譯資料並替換 placeholder
    translations = build_translations()
    html_foot = HTML_FOOT.replace('TRANSLATIONS_PLACEHOLDER', json.dumps(translations, ensure_ascii=False, indent=2))

    html += html_foot

    with open('madzine_modules_compact_v3.8.html', 'w', encoding='utf-8') as f:
        f.write(html)

    print("已生成: madzine_modules_compact_v3.8.html")

if __name__ == '__main__':
    build()
