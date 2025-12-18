#!/usr/bin/env python3
"""
詳細翻譯檢查腳本
逐章節比對英文、中文、日文版本的項目數量
"""

import json
import os
import re
from html.parser import HTMLParser

class DetailedParser(HTMLParser):
    """解析 manual HTML 內容，提取章節和項目"""
    def __init__(self):
        super().__init__()
        self.sections = {}  # {章節名: [項目列表]}
        self.current_section = None
        self.current_tag = None
        self.current_text = ""
        self.in_list = False

    def handle_starttag(self, tag, attrs):
        self.current_tag = tag
        self.current_text = ""
        if tag == 'ul':
            self.in_list = True

    def handle_endtag(self, tag):
        if tag == 'h3' and self.current_text.strip():
            self.current_section = self.current_text.strip()
            self.sections[self.current_section] = []
        elif tag == 'li' and self.current_section and self.current_text.strip():
            # 提取項目的標題（粗體部分）
            self.sections[self.current_section].append(self.current_text.strip()[:80])
        elif tag == 'ul':
            self.in_list = False
        self.current_tag = None
        self.current_text = ""

    def handle_data(self, data):
        if self.current_tag in ['h3', 'li', 'b', 'code']:
            self.current_text += data

def parse_manual(html_content):
    """解析 manual HTML"""
    parser = DetailedParser()
    try:
        parser.feed(html_content)
    except:
        pass
    return parser.sections

def check_module_detailed(slug):
    """詳細檢查單個模組"""
    json_path = f'modules/{slug}.json'
    if not os.path.exists(json_path):
        return None

    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    en = parse_manual(data.get('manual', ''))
    zh = parse_manual(data.get('manual_zh', ''))
    ja = parse_manual(data.get('manual_ja', ''))

    issues = []

    # 比較每個英文章節
    for section_en, items_en in en.items():
        # 找到對應的中文和日文章節（按順序）
        en_sections = list(en.keys())
        zh_sections = list(zh.keys())
        ja_sections = list(ja.keys())

        idx = en_sections.index(section_en)

        # 檢查項目數量
        items_zh = []
        items_ja = []

        if idx < len(zh_sections):
            section_zh = zh_sections[idx]
            items_zh = zh.get(section_zh, [])
        if idx < len(ja_sections):
            section_ja = ja_sections[idx]
            items_ja = ja.get(section_ja, [])

        en_count = len(items_en)
        zh_count = len(items_zh)
        ja_count = len(items_ja)

        if en_count != zh_count:
            issues.append(f"  [{section_en}] EN={en_count} ZH={zh_count}")
        if en_count != ja_count:
            issues.append(f"  [{section_en}] EN={en_count} JA={ja_count}")

    return {
        'slug': slug,
        'name': data.get('name', slug),
        'en_sections': en,
        'zh_sections': zh,
        'ja_sections': ja,
        'issues': issues
    }

def main():
    # 取得所有模組
    modules = []
    for f in sorted(os.listdir('modules')):
        if f.endswith('.json'):
            modules.append(f[:-5])

    print("=" * 100)
    print("MADZINE 模組翻譯詳細檢查報告")
    print("=" * 100)
    print()

    problem_modules = []

    for slug in modules:
        result = check_module_detailed(slug)
        if result:
            en_sec = len(result['en_sections'])
            zh_sec = len(result['zh_sections'])
            ja_sec = len(result['ja_sections'])

            en_items = sum(len(v) for v in result['en_sections'].values())
            zh_items = sum(len(v) for v in result['zh_sections'].values())
            ja_items = sum(len(v) for v in result['ja_sections'].values())

            status = "✓" if not result['issues'] else "⚠"

            print(f"{status} {result['name']:<20} 章節: EN={en_sec} ZH={zh_sec} JA={ja_sec} | 項目: EN={en_items} ZH={zh_items} JA={ja_items}")

            if result['issues']:
                problem_modules.append(result)
                for issue in result['issues']:
                    print(f"   {issue}")

    print()
    print("=" * 100)

    if problem_modules:
        print(f"\n需要檢查的模組: {len(problem_modules)} 個")
        print("\n詳細差異:")
        for r in problem_modules:
            print(f"\n【{r['name']}】")
            print("-" * 50)

            en_sections = list(r['en_sections'].keys())
            zh_sections = list(r['zh_sections'].keys())
            ja_sections = list(r['ja_sections'].keys())

            max_len = max(len(en_sections), len(zh_sections), len(ja_sections))

            for i in range(max_len):
                en_s = en_sections[i] if i < len(en_sections) else "(無)"
                zh_s = zh_sections[i] if i < len(zh_sections) else "(無)"
                ja_s = ja_sections[i] if i < len(ja_sections) else "(無)"

                en_c = len(r['en_sections'].get(en_s, []))
                zh_c = len(r['zh_sections'].get(zh_s, []))
                ja_c = len(r['ja_sections'].get(ja_s, []))

                match = "✓" if en_c == zh_c == ja_c else "⚠"

                print(f"{match} {i+1:2}. EN({en_c}): {en_s[:25]:<25} | ZH({zh_c}): {zh_s[:20]:<20} | JA({ja_c}): {ja_s[:20]}")
    else:
        print("\n✓ 所有模組翻譯項目數量一致")

    print()
    print("=" * 100)
    print(f"總計: {len(modules)} 個模組, {len(problem_modules)} 個有差異")
    print("=" * 100)

if __name__ == '__main__':
    main()
