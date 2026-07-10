"""GitHub REST API로 가져온 PR review comment(코드 diff에 달린 인라인 리뷰
코멘트) JSON 페이지들을 합쳐서, 작성 일자별 개수를 꺾은선그래프 SVG(XML)로
만든다.

입력: `GET /repos/{owner}/{repo}/pulls/comments` 페이지네이션 결과를 저장한
JSON 파일들(여러 개). created_at은 UTC 기준.
"""
import sys
import json
from collections import defaultdict

IN_FILES = sys.argv[1:-1]
OUT_SVG = sys.argv[-1]

comments = []
for path in IN_FILES:
    with open(path, encoding="utf-8") as f:
        comments.extend(json.load(f))

print(f"총 review comment {len(comments)}개 로드됨")

by_date = defaultdict(int)
for c in comments:
    date = c["created_at"][:10]  # YYYY-MM-DD (UTC)
    by_date[date] += 1

dates_sorted = sorted(by_date.keys())
date_counts = [(d, by_date[d]) for d in dates_sorted]

for d, n in date_counts:
    print(f"  {d}: {n}개")
print(f"합계: {sum(n for _, n in date_counts)}개")

# ---- SVG 렌더링 (꺾은선그래프 하나) ----
WIDTH = 860
HEIGHT = 380
chart_top = 70
chart_left = 60
chart_right = WIDTH - 40
chart_bottom = HEIGHT - 60
plot_w = chart_right - chart_left
plot_h = chart_bottom - chart_top

svg = []
svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" '
            f'viewBox="0 0 {WIDTH} {HEIGHT}" font-family="Segoe UI, Malgun Gothic, sans-serif">')
svg.append('<title>CodeFab PR 리뷰 코멘트 일자별 작성 횟수</title>')
svg.append(f'<rect x="0" y="0" width="{WIDTH}" height="{HEIGHT}" fill="#ffffff"/>')
svg.append(f'<text x="{WIDTH/2}" y="30" text-anchor="middle" font-size="20" font-weight="700" '
            f'fill="#1b1e26">CodeFab PR 리뷰 코멘트 일자별 작성 횟수</text>')
svg.append(f'<text x="{WIDTH/2}" y="50" text-anchor="middle" font-size="13" fill="#6b7280">'
            f'GitHub REST API 기준(인라인 코드 리뷰 코멘트), 총 {len(comments)}개, 날짜는 UTC</text>')

max_count = max((n for _, n in date_counts), default=1)
max_count = max(max_count, 1)
n_points = len(date_counts)

def x_for(i):
    if n_points == 1:
        return chart_left + plot_w / 2
    return chart_left + plot_w * i / (n_points - 1)

def y_for(v):
    return chart_bottom - (plot_h * v / max_count)

svg.append(f'<line x1="{chart_left}" y1="{chart_bottom}" x2="{chart_right}" y2="{chart_bottom}" stroke="#c7cad1"/>')
svg.append(f'<line x1="{chart_left}" y1="{chart_top}" x2="{chart_left}" y2="{chart_bottom}" stroke="#c7cad1"/>')

for frac in (0, 0.5, 1):
    v = max_count * frac
    y = y_for(v)
    svg.append(f'<line x1="{chart_left-4}" y1="{y:.1f}" x2="{chart_right}" y2="{y:.1f}" '
                f'stroke="#eef0f4" stroke-dasharray="3,3"/>')
    svg.append(f'<text x="{chart_left-8}" y="{y+4:.1f}" text-anchor="end" font-size="11" '
                f'fill="#6b7280">{v:.0f}</text>')

points_str = " ".join(f"{x_for(i):.1f},{y_for(n):.1f}" for i, (_, n) in enumerate(date_counts))
svg.append(f'<polyline points="{points_str}" fill="none" stroke="#e63946" stroke-width="2.5"/>')

for i, (d, n) in enumerate(date_counts):
    x = x_for(i)
    y = y_for(n)
    svg.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4.5" fill="#e63946"/>')
    svg.append(f'<text x="{x:.1f}" y="{y-10:.1f}" text-anchor="middle" font-size="12" '
                f'font-weight="600" fill="#1b1e26">{n}</text>')
    svg.append(f'<text x="{x:.1f}" y="{chart_bottom+20}" text-anchor="middle" font-size="11" '
                f'fill="#6b7280">{d}</text>')

svg.append('</svg>')

with open(OUT_SVG, "w", encoding="utf-8") as f:
    f.write("\n".join(svg))

print(f"저장됨: {OUT_SVG}")
