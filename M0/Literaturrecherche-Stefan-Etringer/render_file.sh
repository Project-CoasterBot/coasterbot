#!/usr/bin/env bash

# To convert mermaid graphs and make them visible in pdfs
# pandoc-kroki-filter needs to be installed via python pip
# pip install git+https://github.com/Barry1/pandoc-kroki-filter

pandoc ./Algorithmen.md --citeproc --bibliography=algo-references.bib --csl=ieee.csl --metadata link-citations=true  -o main.pdf -f markdown-implicit_figures -V geometry:a4paper,left=2.5cm,right=2.5cm,top=3cm,bottom=3cm --pdf-engine=xelatex #-V urlcolor=blue -V citecolor=black
# pandoc ./ASKE01-XX01-N01.md --citeproc --bibliography=references.bib --csl=ieee.csl --metadata link-citations=true  -o main.docx -V geometry:a4paper,margin=3cm -f markdown-implicit_figures #-V urlcolor=blue -V citecolor=black
