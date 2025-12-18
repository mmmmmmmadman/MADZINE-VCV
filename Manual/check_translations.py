#!/usr/bin/env python3
"""
檢查翻譯完整性腳本
比對英文、中文、日文版本的章節和項目數量
"""

import json
import os
import re
from html.parser import HTMLParser

class ManualParser(HTMLParser):
    """解析 manual HTML 內容"""
    def __init__(self):
        super().__init__()
        self.sections = []  # h3 標題
        self.items = []     # li 項目
        self.current_tag = None
        self.current_text = ""

    def handle_starttag(self, tag, attrs):
        self.current_tag = tag
        self.current_text = ""

    def handle_endtag(self, tag):
        if tag == 'h3' and self.current_text.strip():
            self.sections.append(self.current_text.strip())
        elif tag == 'li' and self.current_text.strip():
            self.items.append(self.current_text.strip()[:50])  # 截取前50字
        self.current_tag = None
        self.current_text = ""

    def handle_data(self, data):
        if self.current_tag in ['h3', 'li', 'b']:
            self.current_text += data

def parse_manual(html_content):
    """解析 manual HTML 並返回統計"""
    parser = ManualParser()
    try:
        parser.feed(html_content)
    except:
        pass
    return {
        'sections': parser.sections,
        'section_count': len(parser.sections),
        'item_count': len(parser.items)
    }

def check_module(slug):
    """檢查單個模組的翻譯完整性"""
    json_path = f'modules/{slug}.json'
    if not os.path.exists(json_path):
        return None

    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    en = parse_manual(data.get('manual', ''))
    zh = parse_manual(data.get('manual_zh', ''))
    ja = parse_manual(data.get('manual_ja', ''))

    issues = []

    # 檢查章節數量
    if en['section_count'] != zh['section_count']:
        issues.append(f"章節數不符: EN={en['section_count']} ZH={zh['section_count']}")
    if en['section_count'] != ja['section_count']:
        issues.append(f"章節數不符: EN={en['section_count']} JA={ja['section_count']}")

    # 檢查項目數量（允許10%誤差）
    en_items = en['item_count']
    zh_items = zh['item_count']
    ja_items = ja['item_count']

    if en_items > 0:
        zh_diff = abs(en_items - zh_items) / en_items
        ja_diff = abs(en_items - ja_items) / en_items

        if zh_diff > 0.1:
            issues.append(f"項目數差異大: EN={en_items} ZH={zh_items} ({zh_diff:.0%})")
        if ja_diff > 0.1:
            issues.append(f"項目數差異大: EN={en_items} JA={ja_items} ({ja_diff:.0%})")

    # 檢查 features 數量
    feat_en = len(data.get('features', []))
    feat_zh = len(data.get('features_zh', []))
    feat_ja = len(data.get('features_ja', []))

    if feat_en != feat_zh:
        issues.append(f"features 數不符: EN={feat_en} ZH={feat_zh}")
    if feat_en != feat_ja:
        issues.append(f"features 數不符: EN={feat_en} JA={feat_ja}")

    return {
        'slug': slug,
        'name': data.get('name', slug),
        'en_sections': en['sections'],
        'zh_sections': zh['sections'],
        'ja_sections': ja['sections'],
        'en_items': en_items,
        'zh_items': zh_items,
        'ja_items': ja_items,
        'feat_en': feat_en,
        'feat_zh': feat_zh,
        'feat_ja': feat_ja,
        'issues': issues
    }

def main():
    # 取得所有模組
    modules = []
    for f in sorted(os.listdir('modules')):
        if f.endswith('.json'):
            modules.append(f[:-5])

    print("=" * 80)
    print("MADZINE 模組翻譯完整性檢查報告")
    print("=" * 80)
    print()

    all_results = []
    problem_modules = []

    for slug in modules:
        result = check_module(slug)
        if result:
            all_results.append(result)
            if result['issues']:
                problem_modules.append(result)

    # 輸出摘要表格
    print(f"{'模組':<20} {'EN章節':>8} {'ZH章節':>8} {'JA章節':>8} {'EN項目':>8} {'ZH項目':>8} {'JA項目':>8} {'狀態':>6}")
    print("-" * 80)

    for r in all_results:
        status = "⚠" if r['issues'] else "✓"
        print(f"{r['name']:<20} {len(r['en_sections']):>8} {len(r['zh_sections']):>8} {len(r['ja_sections']):>8} {r['en_items']:>8} {r['zh_items']:>8} {r['ja_items']:>8} {status:>6}")

    print("-" * 80)
    print()

    # 輸出問題詳情
    if problem_modules:
        print("=" * 80)
        print("需要檢查的模組")
        print("=" * 80)
        for r in problem_modules:
            print(f"\n【{r['name']}】")
            for issue in r['issues']:
                print(f"  - {issue}")

            # 顯示章節對照
            if len(r['en_sections']) != len(r['zh_sections']) or len(r['en_sections']) != len(r['ja_sections']):
                print(f"\n  英文章節 ({len(r['en_sections'])}):")
                for i, s in enumerate(r['en_sections']):
                    print(f"    {i+1}. {s}")
                print(f"\n  中文章節 ({len(r['zh_sections'])}):")
                for i, s in enumerate(r['zh_sections']):
                    print(f"    {i+1}. {s}")
                print(f"\n  日文章節 ({len(r['ja_sections'])}):")
                for i, s in enumerate(r['ja_sections']):
                    print(f"    {i+1}. {s}")
    else:
        print("✓ 所有模組翻譯結構完整")

    print()
    print("=" * 80)
    print(f"總計: {len(all_results)} 個模組, {len(problem_modules)} 個需要檢查")
    print("=" * 80)

if __name__ == '__main__':
    main()
