#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MADZINE VCV Rack 模組日文版說明書結構分析工具
分析所有26個模組的manual_ja欄位HTML內容結構
"""

import json
import os
import re
from html.parser import HTMLParser
from typing import List, Dict, Tuple

class HTMLStructureParser(HTMLParser):
    """解析HTML結構的Parser"""

    def __init__(self):
        super().__init__()
        self.structure = []
        self.current_tag = None
        self.current_data = []

    def handle_starttag(self, tag, attrs):
        if tag in ['h3', 'p', 'li']:
            self.current_tag = tag
            self.current_data = []

    def handle_data(self, data):
        if self.current_tag and data.strip():
            self.current_data.append(data.strip())

    def handle_endtag(self, tag):
        if tag == self.current_tag and self.current_data:
            content = ' '.join(self.current_data)
            # 取前50字
            preview = content[:50] + ('...' if len(content) > 50 else '')
            self.structure.append({
                'tag': self.current_tag.upper(),
                'content': content,
                'preview': preview
            })
            self.current_tag = None
            self.current_data = []

def analyze_module_structure(module_data: Dict) -> Dict:
    """分析單個模組的日文結構"""

    name = module_data.get('name', 'Unknown')
    manual_ja = module_data.get('manual_ja', '')

    # 解析HTML結構
    parser = HTMLStructureParser()
    parser.feed(manual_ja)

    return {
        'name': name,
        'hp': module_data.get('hp', 'N/A'),
        'total_items': len(parser.structure),
        'structure': parser.structure
    }

def main():
    """主程式"""

    modules_dir = '/Users/madzine/Documents/VCV-Dev/MADZINE/Manual/modules'

    # 獲取所有JSON檔案
    json_files = sorted([f for f in os.listdir(modules_dir) if f.endswith('.json')])

    print("=" * 80)
    print("MADZINE VCV Rack 模組日文版說明書結構分析")
    print("=" * 80)
    print(f"\n總共找到 {len(json_files)} 個模組\n")

    all_results = []

    # 分析每個模組
    for idx, filename in enumerate(json_files, 1):
        filepath = os.path.join(modules_dir, filename)

        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                module_data = json.load(f)

            result = analyze_module_structure(module_data)
            all_results.append(result)

            # 輸出結果
            print(f"{idx}. 模組名稱: {result['name']}")
            print(f"   HP: {result['hp']}")
            print(f"   總項目數: {result['total_items']}")
            print(f"   結構:")

            for item_idx, item in enumerate(result['structure'], 1):
                print(f"   {item_idx}. [{item['tag']}] {item['preview']}")

            print()

        except Exception as e:
            print(f"錯誤: 無法處理 {filename}: {e}")
            print()

    # 統計摘要
    print("=" * 80)
    print("統計摘要")
    print("=" * 80)

    total_h3 = sum(len([s for s in r['structure'] if s['tag'] == 'H3']) for r in all_results)
    total_p = sum(len([s for s in r['structure'] if s['tag'] == 'P']) for r in all_results)
    total_li = sum(len([s for s in r['structure'] if s['tag'] == 'LI']) for r in all_results)

    print(f"\n總計:")
    print(f"  - 模組數: {len(all_results)}")
    print(f"  - H3 標題總數: {total_h3}")
    print(f"  - P 段落總數: {total_p}")
    print(f"  - LI 項目總數: {total_li}")
    print(f"  - 總項目數: {total_h3 + total_p + total_li}")

    # 輸出每個模組的項目數
    print(f"\n各模組項目數分布:")
    for result in sorted(all_results, key=lambda x: x['total_items'], reverse=True):
        h3_count = len([s for s in result['structure'] if s['tag'] == 'H3'])
        p_count = len([s for s in result['structure'] if s['tag'] == 'P'])
        li_count = len([s for s in result['structure'] if s['tag'] == 'LI'])
        print(f"  {result['name']:20} - 總計:{result['total_items']:3} (H3:{h3_count:2}, P:{p_count:2}, LI:{li_count:3})")

if __name__ == '__main__':
    main()
