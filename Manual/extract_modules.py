#!/usr/bin/env python3
"""
提取 HTML 說明書中的模組資料到獨立 JSON 檔案
執行一次後可刪除
"""

import re
import json
import os
from html.parser import HTMLParser

INPUT_FILE = 'madzine_modules_compact.html'
OUTPUT_DIR = 'modules'

class CardParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.cards = []
        self.current_card = None
        self.current_tag = None
        self.in_features = False
        self.in_feature_list = False
        self.current_features = []

    def handle_starttag(self, tag, attrs):
        attrs_dict = dict(attrs)
        if tag == 'div' and 'class' in attrs_dict:
            classes = attrs_dict['class']
            if 'card' in classes and 'card-' not in classes:
                self.current_card = {
                    'wide': 'wide-card' in classes,
                    'features': []
                }
            elif 'features' in classes:
                self.in_features = True

        if tag == 'img' and self.current_card:
            src = attrs_dict.get('src', '')
            alt = attrs_dict.get('alt', '')
            if src.startswith('data:image'):
                self.current_card['image'] = src
                if alt:
                    self.current_card['alt'] = alt

        if tag in ['h3', 'span', 'p'] and self.current_card:
            self.current_tag = tag
            if tag == 'span' and attrs_dict.get('class') == 'hp':
                self.current_tag = 'hp'

        if tag == 'ul' and self.in_features:
            self.in_feature_list = True
            self.current_features = []

        if tag == 'li' and self.in_feature_list:
            self.current_tag = 'li'

    def handle_endtag(self, tag):
        if tag == 'div' and self.current_card and not self.in_features:
            # 可能是 card 結束，但我們需要更精確的判斷
            pass

        if tag == 'ul' and self.in_feature_list:
            self.in_feature_list = False
            if self.current_features:
                self.current_card['features'] = self.current_features

        if tag == 'div' and self.in_features:
            self.in_features = False
            if self.current_card and self.current_card.get('name'):
                self.cards.append(self.current_card)
                self.current_card = None

        self.current_tag = None

    def handle_data(self, data):
        data = data.strip()
        if not data or not self.current_card:
            return

        if self.current_tag == 'h3':
            self.current_card['name'] = data
        elif self.current_tag == 'hp':
            self.current_card['hp'] = data
        elif self.current_tag == 'p' and 'description' not in self.current_card:
            self.current_card['description'] = data
        elif self.current_tag == 'li' and self.in_feature_list:
            self.current_features.append(data)

def extract_modules():
    with open(INPUT_FILE, 'r', encoding='utf-8') as f:
        content = f.read()

    # 使用正則表達式更精確地提取
    card_pattern = r'<div class="card(?:\s+wide-card)?">.*?(?=<div class="card|</div>\s*</div>\s*</div>\s*</div>)'
    cards = re.findall(card_pattern, content, re.DOTALL)

    modules = []
    for card_html in cards:
        module = {}

        # 檢查是否是 wide-card
        module['wide'] = 'wide-card' in card_html[:50]

        # 提取圖片
        img_match = re.search(r'<img src="(data:image/png;base64,[^"]+)"(?:\s+alt="([^"]*)")?', card_html)
        if img_match:
            module['image'] = img_match.group(1)
            if img_match.group(2):
                module['alt'] = img_match.group(2)

        # 提取名稱
        name_match = re.search(r'<h3>([^<]+)</h3>', card_html)
        if name_match:
            module['name'] = name_match.group(1).strip()

        # 提取 HP
        hp_match = re.search(r'<span class="hp">([^<]+)</span>', card_html)
        if hp_match:
            module['hp'] = hp_match.group(1).strip()

        # 提取描述
        desc_match = re.search(r'<p>([^<]+)</p>', card_html)
        if desc_match:
            module['description'] = desc_match.group(1).strip()

        # 提取 features
        features = re.findall(r'<li>([^<]+)</li>', card_html)
        if features:
            module['features'] = [f.strip() for f in features]

        if module.get('name'):
            modules.append(module)

    return modules

def save_modules(modules):
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    for i, module in enumerate(modules, 1):
        # 產生檔案名稱
        name = module.get('name', f'module_{i}')
        # 清理檔案名稱
        filename = re.sub(r'[^\w\-]', '_', name.lower())
        filename = re.sub(r'_+', '_', filename).strip('_')

        # 將圖片資料分離
        image_data = module.pop('image', None)

        # 儲存模組資料
        filepath = os.path.join(OUTPUT_DIR, f'{filename}.json')
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(module, f, ensure_ascii=False, indent=2)

        # 儲存圖片資料到獨立檔案
        if image_data:
            img_filepath = os.path.join(OUTPUT_DIR, f'{filename}.img')
            with open(img_filepath, 'w', encoding='utf-8') as f:
                f.write(image_data)

        print(f'已儲存: {filename}.json')

if __name__ == '__main__':
    modules = extract_modules()
    print(f'找到 {len(modules)} 個模組')
    save_modules(modules)
    print('完成!')
