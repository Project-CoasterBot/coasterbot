#!/bin/sh
# Baut 2026_coasterbot_m0.pdf inklusive der via \includesvg eingebundenen SVGs.
# Das svg-Paket ruft dafuer Inkscape auf -> pdflatex MUSS mit -shell-escape laufen.
set -e

# Immer im Verzeichnis dieses Skripts arbeiten (relative Pfade der SVGs).
cd "$(dirname "$0")"

DOC=2026_coasterbot_m0

# Inkscape wird vom svg-Paket zum Konvertieren der SVGs benoetigt.
if ! command -v inkscape >/dev/null 2>&1; then
  echo "FEHLER: inkscape ist nicht im PATH. SVGs koennen nicht gebaut werden." >&2
  exit 1
fi

# Inkscape-Cache leeren, damit die SVGs frisch aus den .svg-Dateien gebaut werden.
rm -rf svg-inkscape

# -shell-escape ist zwingend, sonst werden die \includesvg-Grafiken nicht erzeugt.
LATEX="pdflatex -synctex=1 -interaction=nonstopmode -shell-escape"

$LATEX $DOC.tex
biber $DOC
$LATEX $DOC.tex
$LATEX $DOC.tex

echo "Fertig: $DOC.pdf"
