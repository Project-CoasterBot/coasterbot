#!/bin/sh
# Baut Testreport.pdf.
# Das Dokument bindet keine SVGs ein -> weder -shell-escape noch Inkscape noetig.
# Es ist auch keine Bibliografie aktiv (\nocite/\addbibresource auskommentiert),
# daher entfaellt der biber-Lauf.
set -e

cd "$(dirname "$0")"

DOC=Testreport
LATEX="pdflatex -synctex=1 -interaction=nonstopmode"

$LATEX $DOC.tex
$LATEX $DOC.tex
$LATEX $DOC.tex

echo "Fertig: $DOC.pdf"
