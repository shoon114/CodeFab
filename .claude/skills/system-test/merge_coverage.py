"""UnitTC(Debug exe)/SystemTC(Release exe) 커버리지를 담은 cobertura XML(두 개의
<package>)을 읽어, 소스 파일(모듈)별로 두 패키지의 라인 히트를 합집합으로 병합한
뒤, 모듈별 커버리지 막대그래프를 SVG(XML)로 만든다.
"""
import sys
import xml.etree.ElementTree as ET

IN_XML = sys.argv[1]
OUT_SVG = sys.argv[2]

# 그래프에 표시할 "모듈"(프로덕션 .cpp 파일)과 표시 순서/그룹.
MODULES = [
    ("Tokenizer.cpp", "어휘 분석"),
    ("AssemblerUnit.cpp", "파서"),
    ("StatementParserRegistry.cpp", "파서"),
    ("VarDeclareParser.cpp", "파서"),
    ("PrintStatementParser.cpp", "파서"),
    ("ReturnStatementParser.cpp", "파서"),
    ("BlockParser.cpp", "파서"),
    ("IfStatementParser.cpp", "파서"),
    ("ForStmtParser.cpp", "파서"),
    ("FunctionStatementParser.cpp", "파서"),
    ("ClassStatementParser.cpp", "파서"),
    ("ImportStatementParser.cpp", "파서"),
    ("ExpressionParser.cpp", "파서"),
    ("CheckerUnit.cpp", "정적 검사"),
    ("ExecutorUnit.cpp", "실행"),
    ("NodeVisitor.cpp", "실행"),
    ("SourceFileLoader.cpp", "실행 진입점"),
    ("PromptMode.cpp", "실행 진입점"),
    ("FileMode.cpp", "실행 진입점"),
    ("DebugMode.cpp", "실행 진입점"),
    ("CodeFabInterpreter.cpp", "실행 진입점"),
]

tree = ET.parse(IN_XML)
root = tree.getroot()

# filename(파일명만) -> {line_no: hits}
merged = {}

for package in root.iter("package"):
    for cls in package.iter("class"):
        filename = cls.get("filename")
        base = filename.replace("/", "\\").split("\\")[-1]
        lines_el = cls.find("lines")
        if lines_el is None:
            continue
        bucket = merged.setdefault(base, {})
        for line_el in lines_el.findall("line"):
            n = int(line_el.get("number"))
            hits = int(line_el.get("hits"))
            bucket[n] = bucket.get(n, 0) + hits

results = []
for base, group in MODULES:
    hits_by_line = merged.get(base)
    if not hits_by_line:
        results.append((base, group, 0, 0, 0.0))
        continue
    total = len(hits_by_line)
    covered = sum(1 for h in hits_by_line.values() if h > 0)
    rate = (covered / total * 100.0) if total else 0.0
    results.append((base, group, total, covered, rate))

overall_total = sum(r[2] for r in results)
overall_covered = sum(r[3] for r in results)
overall_rate = (overall_covered / overall_total * 100.0) if overall_total else 0.0

# ---- SVG 생성 ----
row_h = 26
top_margin = 70
left_margin = 230
bar_max_w = 480
right_margin = 90
width = left_margin + bar_max_w + right_margin
height = top_margin + row_h * len(results) + 60

def color_for(rate):
    if rate >= 90:
        return "#2ecc71"
    if rate >= 70:
        return "#f1c40f"
    if rate >= 40:
        return "#e67e22"
    return "#e74c3c"

svg_parts = []
svg_parts.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
                  f'viewBox="0 0 {width} {height}" font-family="Segoe UI, Malgun Gothic, sans-serif">')
svg_parts.append('<title>CodeFab 모듈별 테스트 커버리지 (UnitTC + SystemTC)</title>')
svg_parts.append(f'<rect x="0" y="0" width="{width}" height="{height}" fill="#ffffff"/>')
svg_parts.append(f'<text x="{width/2}" y="30" text-anchor="middle" font-size="20" font-weight="700" fill="#1b1e26">'
                  f'CodeFab 모듈별 테스트 커버리지</text>')
svg_parts.append(f'<text x="{width/2}" y="52" text-anchor="middle" font-size="13" fill="#6b7280">'
                  f'UnitTC(324) + SystemTC(77) 종합 · 라인 커버리지 · 전체 평균 {overall_rate:.1f}%</text>')

for i, (base, group, total, covered, rate) in enumerate(results):
    y = top_margin + i * row_h
    bar_w = bar_max_w * (rate / 100.0)
    color = color_for(rate)
    label = f"{base}"
    svg_parts.append(f'<text x="{left_margin - 10}" y="{y + row_h*0.68}" text-anchor="end" '
                      f'font-size="12.5" fill="#1b1e26">{label}</text>')
    svg_parts.append(f'<rect x="{left_margin}" y="{y + 4}" width="{bar_max_w}" height="{row_h - 10}" '
                      f'fill="#e2e4ea"/>')
    svg_parts.append(f'<rect x="{left_margin}" y="{y + 4}" width="{bar_w:.1f}" height="{row_h - 10}" '
                      f'fill="{color}"/>')
    svg_parts.append(f'<text x="{left_margin + bar_max_w + 10}" y="{y + row_h*0.68}" '
                      f'font-size="12" fill="#1b1e26">{rate:.0f}% ({covered}/{total})</text>')

legend_y = height - 34
legend_items = [("90%+", "#2ecc71"), ("70~89%", "#f1c40f"), ("40~69%", "#e67e22"), ("0~39%", "#e74c3c")]
lx = left_margin
for name, color in legend_items:
    svg_parts.append(f'<rect x="{lx}" y="{legend_y}" width="14" height="14" fill="{color}"/>')
    svg_parts.append(f'<text x="{lx + 20}" y="{legend_y + 12}" font-size="12" fill="#1b1e26">{name}</text>')
    lx += 110

svg_parts.append('</svg>')

with open(OUT_SVG, "w", encoding="utf-8") as f:
    f.write("\n".join(svg_parts))

print(f"전체 평균 라인 커버리지: {overall_rate:.1f}% ({overall_covered}/{overall_total})")
for base, group, total, covered, rate in results:
    print(f"  {base:32s} [{group:8s}] {rate:6.1f}%  ({covered}/{total})")
