#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
mindmap_graphviz.py
===================

Erzeugt aus einer markmap-artigen Markdown-Datei eine farblich akzentuierte
Mind Map und rendert sie mit Graphviz nach SVG (optional PNG/PDF).

Struktur der Markdown-Quelle
----------------------------
    # Wurzeltitel                <- Zentrum der Mind Map (genau eine H1)
    ## Teilsystem                <- eigener Branch, bekommt eine eigene Farbe
    ### Kategorie                <- Unterknoten des Branches
    - Eintrag                    <- Blatt (Listenpunkte, auch verschachtelt)
      - Untereintrag
    #### Feinkategorie           <- optional, tiefere Überschriften
YAML-Frontmatter (--- ... ---) am Dateianfang wird ignoriert.

Voraussetzungen
---------------
    pip install graphviz
    + Graphviz-Binary installiert (dot):  apt install graphviz  /  choco install graphviz

Layout
------
Standard ist eine radiale Anordnung ("rundherum") um den Wurzelknoten
mit der Graphviz-Engine twopi. Mit --engine dot erhält man stattdessen
den klassischen Baum von links nach rechts.

Beispiele
---------
    python3 mindmap_graphviz.py Coasterbot_Elektrik_Mindmap.md          # radial (twopi)
    python3 mindmap_graphviz.py quelle.md --engine circo                # alternative Kreisform
    python3 mindmap_graphviz.py quelle.md --ranksep 1.6,2.4,3.4         # Ringe weiter außen
    python3 mindmap_graphviz.py quelle.md --engine dot --rankdir LR     # klassischer Baum
"""

import argparse
import re
import sys
import textwrap
from pathlib import Path

try:
    import graphviz
except ImportError:
    sys.exit("Fehlt: python -m pip install graphviz  (und Graphviz-Binary installieren)")


# ---------------------------------------------------------------------------
# Farbpalette: (solid, tint, dark) — wird pro Branch reihum vergeben.
# ---------------------------------------------------------------------------
PALETTE = [
    ("#BA7517", "#FAEEDA", "#633806"),  # amber
    ("#185FA5", "#E6F1FB", "#0C447C"),  # blue
    ("#0F6E56", "#E1F5EE", "#085041"),  # teal
    ("#D85A30", "#FAECE7", "#712B13"),  # coral
    ("#534AB7", "#EEEDFE", "#3C3489"),  # purple
    ("#639922", "#EAF3DE", "#27500A"),  # green
    ("#D4537E", "#FBEAF0", "#72243E"),  # pink
    ("#E24B4A", "#FCEBEB", "#791F1F"),  # red
    ("#5F5E5A", "#F1EFE8", "#2C2C2A"),  # gray
]
ROOT_FILL = "#2C2C2A"


# ---------------------------------------------------------------------------
# Baumknoten
# ---------------------------------------------------------------------------
class Node:
    __slots__ = ("title", "level", "children", "kind")

    def __init__(self, title, level, kind):
        self.title = title
        self.level = level          # Überschriftsebene bzw. Listen-Tiefe
        self.kind = kind            # 'root' | 'branch' | 'heading' | 'leaf'
        self.children = []


# ---------------------------------------------------------------------------
# Markdown-Parser (Überschriften + verschachtelte Listen)
# ---------------------------------------------------------------------------
def strip_frontmatter(text):
    if text.startswith("---"):
        end = text.find("\n---", 3)
        if end != -1:
            nl = text.find("\n", end + 1)
            return text[nl + 1:] if nl != -1 else ""
    return text


def parse_markdown(text):
    text = strip_frontmatter(text)
    root = None
    # Stack aus (level, Node); level: H1=1 ... H6=6, Listen ab 100 + Einrücktiefe
    stack = []
    heading_re = re.compile(r"^(#{1,6})\s+(.*\S)\s*$")
    list_re = re.compile(r"^(\s*)[-*+]\s+(.*\S)\s*$")

    def attach(node):
        # passenden Elternknoten anhand des Levels finden
        while stack and stack[-1][0] >= node.level:
            stack.pop()
        if stack:
            stack[-1][1].children.append(node)
        stack.append((node.level, node))

    for raw in text.splitlines():
        if not raw.strip():
            continue
        m = heading_re.match(raw)
        if m:
            level = len(m.group(1))
            title = m.group(2).strip()
            if level == 1:
                root = Node(title, 1, "root")
                stack = [(1, root)]
                continue
            kind = "branch" if level == 2 else "heading"
            attach(Node(title, level, kind))
            continue
        m = list_re.match(raw)
        if m:
            indent = len(m.group(1).expandtabs(4))
            title = m.group(2).strip()
            # Listen liegen immer unter der zuletzt geöffneten Überschrift
            level = 100 + indent
            attach(Node(title, level, "leaf"))
            continue
        # sonstige Zeilen ignorieren
    if root is None:
        sys.exit("Keine H1-Überschrift (# ...) gefunden — kein Wurzelknoten.")
    return root


# ---------------------------------------------------------------------------
# Graphviz-Aufbau
# ---------------------------------------------------------------------------
def wrap(label, width):
    return "\n".join(textwrap.wrap(label, width=width)) or label


def build_graph(root, engine="twopi", rankdir="LR", wrap_width=22, ranksep=None):
    # twopi/circo/neato ordnen die Knoten radial ("rundherum") an,
    # dot erzeugt den klassischen Baum von links nach rechts.
    radial = engine in ("twopi", "circo", "neato", "fdp", "sfdp")
    if radial:
        graph_attr = {
            "bgcolor": "white",
            "overlap": "false",                    # Überlappungen auflösen
            "splines": "true",                     # gebogene Kanten
            "sep": "+12",                          # Mindestabstand zwischen Knoten
            "ranksep": ranksep or "1.4,2.1,3.0",   # Radien der konzentrischen Ringe
            "pad": "0.4",
        }
    else:
        graph_attr = {
            "rankdir": rankdir,
            "bgcolor": "white",
            "splines": "spline",
            "overlap": "false",
            "ranksep": ranksep or "0.55",
            "nodesep": "0.22",
            "pad": "0.3",
        }
    g = graphviz.Graph(
        "mindmap",
        engine=engine,
        graph_attr=graph_attr,
        node_attr={"fontname": "Helvetica", "fontsize": "12"},
        edge_attr={"penwidth": "1.4"},
    )

    counter = {"i": 0}

    def nid():
        counter["i"] += 1
        return f"n{counter['i']}"

    # Wurzel
    root_id = nid()
    g.node(
        root_id,
        wrap(root.title.replace(" – ", "\n"), wrap_width),
        shape="box", style="rounded,filled",
        fillcolor=ROOT_FILL, fontcolor="white",
        color=ROOT_FILL, penwidth="1.5", margin="0.18,0.12",
    )
    if radial:
        g.attr(root=root_id)          # Zentrum der radialen Anordnung

    def add_leaves(node, parent_id, solid, tint, dark):
        for child in node.children:
            cid = nid()
            g.node(
                cid, wrap(child.title, wrap_width),
                shape="box", style="rounded,filled",
                fillcolor="white", color=solid, fontcolor=dark,
                penwidth="0.8", margin="0.12,0.06",
            )
            g.edge(parent_id, cid, color=solid, penwidth="1.0")
            if child.children:
                add_leaves(child, cid, solid, tint, dark)

    for idx, branch in enumerate(root.children):
        solid, tint, dark = PALETTE[idx % len(PALETTE)]
        bid = nid()
        g.node(
            bid, wrap(branch.title, wrap_width),
            shape="box", style="rounded,filled",
            fillcolor=solid, fontcolor="white", color=solid,
            penwidth="1.2", margin="0.16,0.10",
        )
        g.edge(root_id, bid, color=solid, penwidth="2.2")

        for cat in branch.children:
            if cat.kind == "heading":
                cid = nid()
                g.node(
                    cid, wrap(cat.title, wrap_width),
                    shape="box", style="rounded,filled",
                    fillcolor=tint, color=solid, fontcolor=dark,
                    penwidth="0.9", margin="0.14,0.08",
                )
                g.edge(bid, cid, color=solid, penwidth="1.6")
                add_leaves(cat, cid, solid, tint, dark)
            else:
                # Blätter direkt unter dem Branch (falls keine Kategorie)
                add_leaves(branch, bid, solid, tint, dark)
                break
    return g


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description="Markdown -> farbige Graphviz-Mindmap")
    ap.add_argument("markdown", nargs="?", default="Coasterbot_Elektrik_Mindmap.md",
                    help="Eingabe-Markdown (markmap-Struktur)")
    ap.add_argument("-o", "--output", default=None,
                    help="Ausgabe-Basisname ohne Endung (Standard: wie Eingabe)")
    ap.add_argument("--engine", default="twopi",
                    choices=["twopi", "circo", "dot", "neato", "fdp", "sfdp"],
                    help="Graphviz-Layout: twopi/circo=radial (rundherum), dot=Baum")
    ap.add_argument("--rankdir", default="LR", choices=["LR", "TB", "RL", "BT"],
                    help="Flussrichtung nur bei engine=dot")
    ap.add_argument("--ranksep", default=None,
                    help="Ringradien (twopi), z.B. '1.4,2.1,3.0', oder Rangabstand (dot), z.B. '0.6'")
    ap.add_argument("--format", default="svg",
                    choices=["svg", "png", "pdf"], help="Ausgabeformat")
    ap.add_argument("--wrap", type=int, default=22,
                    help="Zeichen pro Zeile im Label (Umbruch)")
    args = ap.parse_args()

    src = Path(args.markdown)
    if not src.exists():
        sys.exit(f"Datei nicht gefunden: {src}")

    root = parse_markdown(src.read_text(encoding="utf-8"))
    g = build_graph(root, engine=args.engine, rankdir=args.rankdir,
                    wrap_width=args.wrap, ranksep=args.ranksep)

    base = args.output or str(src.with_suffix(""))
    out = g.render(filename=base, format=args.format, cleanup=True)
    print(f"Geschrieben: {out}")


if __name__ == "__main__":
    main()