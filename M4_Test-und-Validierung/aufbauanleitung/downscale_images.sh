#!/usr/bin/env bash

gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.5 \
  -dPDFSETTINGS=/ebook \
  -dColorImageResolution=150 -dGrayImageResolution=150 -dMonoImageResolution=150 \
  -dDownsampleColorImages=true -dColorImageDownsampleType=/Bicubic \
  -dAutoRotatePages=/None \
  -dNOPAUSE -dBATCH -dQUIET \
  -sOutputFile=Aufbauanleitung_compressed.pdf Aufbauanleitung.pdf
