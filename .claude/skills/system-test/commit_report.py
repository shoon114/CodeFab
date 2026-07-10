"""현재 브랜치 기준 커밋 히스토리를 파싱해서
  1) 일자별 커밋 개수 (꺾은선그래프)
  2) 모듈별로 수정된 커밋 개수 (막대그래프)
를 하나의 SVG(XML) 리포트로 만든다.

입력은 `git log --pretty=format:'@@COMMIT@@%H|%ad|%s' --date=short --name-only`
결과를 담은 텍스트 파일.
"""
import sys
import re
from collections import defaultdict, OrderedDict

IN_LOG = sys.argv[1]
OUT_SVG = sys.argv[2]

# ---- 모듈 분류 규칙: (파일 경로에 포함되는 패턴, 모듈 이름) 순서대로 첫 매치 사용 ----
MODULE_RULES = [
    (r"^(Tokenizer|Token)\.", "Tokenizer"),
    (r"^(AssemblerUnit|IStatementParser|StatementParserRegistry)\.", "AssemblerUnit(파서 코어)"),
    (r"^(VarDeclareParser|PrintStatementParser|ReturnStatementParser|BlockParser|"
     r"IfStatementParser|ForStmtParser|FunctionStatementParser|ClassStatementParser|"
     r"ImportStatementParser|ExpressionParser)\.", "Statement 파서"),
    (r"^CheckerUnit\.", "CheckerUnit"),
    (r"^(ExecutorUnit|NodeVisitor|SyntaxNode|Value)\.", "ExecutorUnit/AST"),
    (r"^(PromptMode|FileMode|DebugMode|ShellMode|CodeFabInterpreter|SourceFileLoader)\.", "실행 진입점(ShellMode)"),
    (r"^Test.*\.cpp$|MockStatementParser\.h$|TestTokenHelpers\.h$", "유닛 테스트(TC)"),
    (r"^\.claude/skills/", "테스트/자동화 스크립트"),
    (r"^docs/", "문서"),
    (r"\.vcxproj|\.sln|\.slnx|^packages/", "빌드/프로젝트 설정"),
]
OTHER_MODULE = "기타"


def classify(path):
    for pattern, name in MODULE_RULES:
        if re.search(pattern, path):
            return name
    return OTHER_MODULE


commits = []  # list of (hash, date, subject, [files])
with open(IN_LOG, encoding="utf-8") as f:
    cur = None
    for line in f:
        line = line.rstrip("\n")
        if line.startswith("@@COMMIT@@"):
            if cur:
                commits.append(cur)
            _, h, date, subject = line.split("|", 3) if line.count("|") >= 3 else (line, "", "", "")
            # 위 split은 '@@COMMIT@@HASH' 를 첫 필드로 만들어버리므로 다시 정리
            rest = line[len("@@COMMIT@@"):]
            h, date, subject = rest.split("|", 2)
            cur = {"hash": h, "date": date, "subject": subject, "files": []}
        elif line.strip():
            if cur is not None:
                cur["files"].append(line.strip())
    if cur:
        commits.append(cur)

print(f"총 커밋 {len(commits)}개 파싱됨")

# ---- 1) 일자별 커밋 개수 ----
by_date = defaultdict(int)
for c in commits:
    by_date[c["date"]] += 1
dates_sorted = sorted(by_date.keys())
date_counts = [(d, by_date[d]) for d in dates_sorted]

# ---- 2) 모듈별 커밋 개수 (한 커밋이 여러 모듈을 건드리면 각 모듈에 +1, 중복 집계) ----
by_module = defaultdict(int)
for c in commits:
    mods = set(classify(fp) for fp in c["files"])
    if not mods:
        mods = {OTHER_MODULE}
    for m in mods:
        by_module[m] += 1

module_order = [name for _, name in MODULE_RULES] + [OTHER_MODULE]
module_counts = [(m, by_module.get(m, 0)) for m in module_order if by_module.get(m, 0) > 0]
module_counts.sort(key=lambda x: -x[1])

for d, n in date_counts:
    print(f"  {d}: {n}개")
print("--- 모듈별 ---")
for m, n in module_counts:
    print(f"  {m}: {n}개")

# =========================================================================
# SVG 렌더링: 위쪽 꺾은선그래프(일자별), 아래쪽 막대그래프(모듈별)
# =========================================================================
WIDTH = 860
LINE_CHART_H = 300
BAR_ROW_H = 26
BAR_CHART_H = 70 + BAR_ROW_H * len(module_counts) + 40
GAP = 40
HEIGHT = 60 + LINE_CHART_H + GAP + BAR_CHART_H + 40

svg = []
svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" '
            f'viewBox="0 0 {WIDTH} {HEIGHT}" font-family="Segoe UI, Malgun Gothic, sans-serif">')
svg.append('<title>CodeFab 커밋 히스토리 리포트</title>')
svg.append(f'<rect x="0" y="0" width="{WIDTH}" height="{HEIGHT}" fill="#ffffff"/>')
svg.append(f'<text x="{WIDTH/2}" y="30" text-anchor="middle" font-size="20" font-weight="700" '
            f'fill="#1b1e26">CodeFab 커밋 히스토리 리포트 (현재 브랜치 기준, 총 {len(commits)}개)</text>')

# ---- 꺾은선그래프 ----
chart_top = 60
chart_left = 60
chart_right = WIDTH - 40
chart_bottom = chart_top + LINE_CHART_H - 50
plot_w = chart_right - chart_left
plot_h = chart_bottom - chart_top

svg.append(f'<text x="{chart_left}" y="{chart_top - 10}" font-size="14" font-weight="700" '
            f'fill="#1b1e26">1. 일자별 커밋 개수</text>')

max_count = max((n for _, n in date_counts), default=1)
max_count = max(max_count, 1)
n_points = len(date_counts)

def x_for(i):
    if n_points == 1:
        return chart_left + plot_w / 2
    return chart_left + plot_w * i / (n_points - 1)

def y_for(v):
    return chart_bottom - (plot_h * v / max_count)

# 축
svg.append(f'<line x1="{chart_left}" y1="{chart_bottom}" x2="{chart_right}" y2="{chart_bottom}" stroke="#c7cad1"/>')
svg.append(f'<line x1="{chart_left}" y1="{chart_top}" x2="{chart_left}" y2="{chart_bottom}" stroke="#c7cad1"/>')

# y축 눈금 (0, max)
for frac in (0, 0.5, 1):
    v = max_count * frac
    y = y_for(v)
    svg.append(f'<line x1="{chart_left-4}" y1="{y:.1f}" x2="{chart_right}" y2="{y:.1f}" '
                f'stroke="#eef0f4" stroke-dasharray="3,3"/>')
    svg.append(f'<text x="{chart_left-8}" y="{y+4:.1f}" text-anchor="end" font-size="11" '
                f'fill="#6b7280">{v:.0f}</text>')

points_str = " ".join(f"{x_for(i):.1f},{y_for(n):.1f}" for i, (_, n) in enumerate(date_counts))
svg.append(f'<polyline points="{points_str}" fill="none" stroke="#4361ee" stroke-width="2.5"/>')

for i, (d, n) in enumerate(date_counts):
    x = x_for(i)
    y = y_for(n)
    svg.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4.5" fill="#4361ee"/>')
    svg.append(f'<text x="{x:.1f}" y="{y-10:.1f}" text-anchor="middle" font-size="12" '
                f'font-weight="600" fill="#1b1e26">{n}</text>')
    svg.append(f'<text x="{x:.1f}" y="{chart_bottom+20}" text-anchor="middle" font-size="11" '
                f'fill="#6b7280">{d}</text>')

# ---- 막대그래프 ----
bar_top = chart_top + LINE_CHART_H + GAP
svg.append(f'<text x="{chart_left}" y="{bar_top - 10}" font-size="14" font-weight="700" '
            f'fill="#1b1e26">2. 모듈별 수정 커밋 개수</text>')

bar_left_label_w = 190
bar_left = chart_left + bar_left_label_w
bar_max_w = WIDTH - 40 - bar_left
max_mod_count = max((n for _, n in module_counts), default=1)

palette = ["#4361ee", "#3a86ff", "#2ec4b6", "#ff9f1c", "#e63946",
           "#8338ec", "#ffbe0b", "#06a77d", "#e76f51", "#457b9d", "#6c757d"]

for i, (m, n) in enumerate(module_counts):
    y = bar_top + 10 + i * BAR_ROW_H
    bar_w = bar_max_w * (n / max_mod_count) if max_mod_count else 0
    color = palette[i % len(palette)]
    svg.append(f'<text x="{bar_left - 10}" y="{y + BAR_ROW_H*0.65:.1f}" text-anchor="end" '
                f'font-size="12.5" fill="#1b1e26">{m}</text>')
    svg.append(f'<rect x="{bar_left}" y="{y}" width="{bar_max_w:.1f}" height="{BAR_ROW_H-8}" fill="#e2e4ea"/>')
    svg.append(f'<rect x="{bar_left}" y="{y}" width="{bar_w:.1f}" height="{BAR_ROW_H-8}" fill="{color}"/>')
    svg.append(f'<text x="{bar_left + bar_max_w + 10:.1f}" y="{y + BAR_ROW_H*0.65:.1f}" '
                f'font-size="12" fill="#1b1e26">{n}</text>')

svg.append('</svg>')

with open(OUT_SVG, "w", encoding="utf-8") as f:
    f.write("\n".join(svg))

print(f"저장됨: {OUT_SVG}")
