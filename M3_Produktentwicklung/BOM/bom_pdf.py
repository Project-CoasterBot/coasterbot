#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Erzeugt aus bom.csv und versionshistorie.txt ein PDF der Stueckliste.

Aufbau des erzeugten Dokuments:

    Seite 1     Deckblatt (Hochformat, Layout der uebrigen Projektdokumente)
    Seite 2     Versionshistorie (Hochformat)
    ab Seite 3  Stueckliste als Tabelle (Querformat)

Weitere Inhalte enthaelt das Dokument nicht. Die Tabelle zeigt genau die
Spalten und Zeilen der CSV -- weder werden Spalten ergaenzt noch Zeilen
hinzugefuegt.

Die Tabelle wird in normalem LaTeX-Satz gesetzt: booktabs-Linien oben, unter
der Kopfzeile und am Ende, keine senkrechten Linien, Dokumentschrift in
\footnotesize. Der Kopf wird auf jeder Folgeseite wiederholt.

Der Weg fuehrt ueber ein LaTeX-Dokument, das anschliessend mit pdflatex
gesetzt wird. Die erzeugte .tex-Datei bleibt liegen und kann bei Bedarf von
Hand nachbearbeitet werden.

bom.csv wird ausschliesslich gelesen und nie veraendert.

Aufruf:
    python3 bom_pdf.py [bom.csv] [versionshistorie.txt]
"""

import csv
import io
import os
import sys

# ---------------------------------------------------------------------------
# Konfiguration
# ---------------------------------------------------------------------------

CSV_DATEI = "bom.csv"
KODIERUNGEN = ("utf-8-sig", "cp1252", "cp858", "cp850", "latin-1")
HISTORIE_DATEI = "versionshistorie.txt"
TEX_DATEI = "bom.tex"
LOGO_DATEI = "coasterbot.png"          # wird nur eingebunden, wenn vorhanden

# Angaben fuer Deckblatt und Schriftfeld
ORGANISATION = "Wilhelm Büchner University of Applied Sciences"
VERANSTALTUNG = "Projektseminar Master"
PROJEKT = "Coasterbot"
DOKUMENTTITEL = "Stückliste - BOM"
DOKUMENTNUMMER = "CB-DOC-M3-BOM"
ERZEUGNIS = "Coasterbot Prototyp"
AUTOREN = [
    ("Dennis Borutta, B. Eng", "dennis.borutta@outlook.com"),
    ("Stefan Etringer, B. Eng", "mail@stefan.works"),
    ("Gereon Such, M. Sc.", "gereonsuch@gmail.com"),
]

# Seitenmasse in mm
QUER_BREITE, QUER_HOEHE = 297.0, 210.0
QUER_RAND_LINKS, QUER_RAND_RECHTS = 15.0, 15.0
QUER_RAND_OBEN, QUER_RAND_UNTEN = 15.0, 15.0

TABCOLSEP_MM = 2.12                     # LaTeX-Standard (6 pt)

# Breite (mm) und Ausrichtung je Spalte. Gerendert werden ausschliesslich die
# Spalten der CSV, in der Reihenfolge der CSV; hier nicht aufgefuehrte Spalten
# erhalten STANDARDBREITE. Die Breiten sind Richtwerte und werden auf den
# Satzspiegel skaliert.
SPALTENFORMAT = {
    "Position":    (17, "r"),
    "Menge":       (16, "r"),
    "Einheit":     (15, "l"),
    "Benennung":   (45, "l"),
    "Sachnummer":  (26, "l"),
    "Bemerkung":   (78, "l"),
    "Einzelpreis": (22, "r"),
    "Gesamtpreis": (22, "r"),
}
STANDARDBREITE = 22                     # fuer Spalten, die oben nicht stehen

# Schlusszeile: Spalte, ueber die summiert wird, und ihre Beschriftung. Fehlt
# die Spalte oder ist kein Betrag lesbar, entfaellt die Zeile.
SUMMENSPALTE = "Gesamtpreis"
SUMMENBESCHRIFTUNG = "Gesamtbetrag je Prototyp"

AUFRAEUMEN = True                       # .aux und .log nach dem Lauf loeschen


# ---------------------------------------------------------------------------
# Einlesen
# ---------------------------------------------------------------------------

def lies_text(pfad):
    """Liest eine Textdatei und erkennt die Kodierung.

    bom.csv liegt als DOS-Export vor (cp858: 'ä' = 0x84, '€' = 0xD5), wie ihn
    Excel unter 'CSV (MS-DOS)' schreibt; eigene Dateien sind UTF-8. Eine
    Kodierung gilt als passend, wenn sie fehlerfrei dekodiert und keine
    C1-Steuerzeichen liefert -- daran scheitern falsche Zuordnungen, die sonst
    unbemerkt Buchstabensalat ergäben."""
    rohdaten = open(pfad, "rb").read()
    for kodierung in KODIERUNGEN:
        try:
            text = rohdaten.decode(kodierung)
        except UnicodeDecodeError:
            continue
        if not any(0x80 <= ord(z) <= 0x9F for z in text):
            return text
    return rohdaten.decode("latin-1")


def lies_stueckliste(pfad):
    """Liefert (Feldnamen, Zeilen als dict). Die Datei wird nur gelesen."""
    leser = csv.DictReader(io.StringIO(lies_text(pfad)), delimiter=",")
    if leser.fieldnames is None:
        raise SystemExit("FEHLER: %s enthält keine Kopfzeile." % pfad)
    felder = [f.strip() for f in leser.fieldnames]
    zeilen = []
    for satz in leser:
        zeilen.append({k.strip(): (v or "").strip() for k, v in satz.items() if k})
    return felder, zeilen


def lies_versionshistorie(pfad):
    """Format je Zeile:  Version | Datum | Bearbeiter | Änderung
    Leerzeilen und Zeilen ab '#' werden übergangen."""
    eintraege = []
    for nummer, zeile in enumerate(lies_text(pfad).splitlines(), start=1):
        zeile = zeile.strip()
        if not zeile or zeile.startswith("#"):
            continue
        teile = [t.strip() for t in zeile.split("|")]
        if len(teile) != 4:
            raise SystemExit(
                "FEHLER: %s, Zeile %d: erwartet werden vier durch '|' getrennte "
                "Felder (Version | Datum | Bearbeiter | Änderung)." % (pfad, nummer))
        eintraege.append(teile)
    if not eintraege:
        raise SystemExit("FEHLER: %s enthält keinen Eintrag." % pfad)
    return eintraege


# ---------------------------------------------------------------------------
# LaTeX
# ---------------------------------------------------------------------------

SONDERZEICHEN = {
    "\\": r"\textbackslash{}", "&": r"\&", "%": r"\%", "$": r"\$", "#": r"\#",
    "_": r"\_", "{": r"\{", "}": r"\}", "~": r"\textasciitilde{}",
    "^": r"\textasciicircum{}",
}


def tex(text):
    """Maskiert LaTeX-Sonderzeichen im Inhalt der CSV."""
    return "".join(SONDERZEICHEN.get(z, z) for z in text)


def spaltenlayout(felder):
    """Spalten der CSV in ihrer Reihenfolge, Breiten auf den Satzspiegel skaliert."""
    spalten = [(f,) + SPALTENFORMAT.get(f, (STANDARDBREITE, "l")) for f in felder]
    satzspiegel = QUER_BREITE - QUER_RAND_LINKS - QUER_RAND_RECHTS
    verfuegbar = satzspiegel - 2 * TABCOLSEP_MM * (len(spalten) - 1)
    faktor = verfuegbar / float(sum(s[1] for s in spalten))
    return [(name, breite * faktor, richtung) for name, breite, richtung in spalten]


def betrag(text):
    """Liest einen Betrag wie '12.99€' oder '1.234,50 EUR'; None, wenn keiner."""
    ziffern = "".join(z for z in text if z.isdigit() or z in ",.-")
    if not ziffern.strip("-.,"):
        return None
    if "," in ziffern and "." in ziffern:          # gemischt: das letzte Zeichen trennt
        schnitt = max(ziffern.rfind(","), ziffern.rfind("."))
        ziffern = (ziffern[:schnitt].replace(",", "").replace(".", "")
                   + "." + ziffern[schnitt + 1:])
    elif "," in ziffern:
        ziffern = ziffern.replace(",", ".")
    try:
        return float(ziffern)
    except ValueError:
        return None


def summe(zeilen):
    """Summiert SUMMENSPALTE. Liefert (formatierter Betrag, gelesen, gesamt)
    oder None, wenn sich kein Wert lesen lässt."""
    werte = [(satz.get(SUMMENSPALTE, ""), betrag(satz.get(SUMMENSPALTE, "")))
             for satz in zeilen]
    lesbar = [(roh, wert) for roh, wert in werte if wert is not None]
    if not lesbar:
        return None
    muster = lesbar[0][0]
    # Währungszeichen und Dezimaltrennzeichen aus den Daten übernehmen
    waehrung = "".join(z for z in muster
                       if not (z.isdigit() or z in ",.-" or z.isspace()))
    trenner = "," if "," in muster else "."
    gesamt = ("%.2f" % sum(wert for _, wert in lesbar)).replace(".", trenner)
    return gesamt + waehrung, len(lesbar), len(werte)


def deckblatt():
    """Titelseite im Layout der übrigen Projektdokumente."""
    logo = ""
    if os.path.exists(LOGO_DATEI):
        logo = (r"    {\includegraphics[width=0.38\textwidth]{%s}} \\[1em]"
                "\n\n" % LOGO_DATEI)
    autoren = "\n".join(r"%s & %s\\" % (tex(name), tex(mail))
                        for name, mail in AUTOREN)
    return r"""\begin{titlepage}
  \begin{center}
    {\large %(organisation)s} \\[0.3em]

    {\normalsize %(veranstaltung)s}
\vspace{5em}

    {\LARGE\bfseries %(projekt)s} \\[0.5em]

    {\large %(titel)s} \\[1em]

%(logo)s\vspace{4em}

    {\normalsize
\begin{tabular}{ll}
%(autoren)s
\end{tabular}
    }
  \end{center}
\end{titlepage}""" % dict(
        organisation=tex(ORGANISATION), veranstaltung=tex(VERANSTALTUNG),
        projekt=tex(PROJEKT), titel=tex(DOKUMENTTITEL), logo=logo,
        autoren=autoren)


def baue_tex(felder, zeilen, historie):
    spalten = spaltenlayout(felder)
    version, datum, bearbeiter = historie[-1][:3]

    t = [r"""% Automatisch erzeugt von bom_pdf.py -- Änderungen hier gehen beim
% nächsten Lauf verloren. Inhalte in bom.csv und versionshistorie.txt pflegen.
\documentclass[11pt,a4paper]{scrartcl}
\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage[german]{babel}
\usepackage{lmodern}
\usepackage{textcomp}
\usepackage{geometry}
\usepackage{longtable}
\usepackage{array}
\usepackage{graphicx}
\usepackage{booktabs}
\geometry{a4paper,left=25mm,right=25mm,top=30mm,bottom=25mm}
\setlength{\parindent}{0pt}
\pagestyle{plain}
\begin{document}"""]

    t.append(deckblatt())

    # --- Versionshistorie --------------------------------------------------
    t.append(r"""
\section*{Versionshistorie}
\begin{tabular}{@{}p{1.6cm}p{2.4cm}p{3.0cm}>{\raggedright\arraybackslash}p{7.0cm}@{}}
\toprule
\textbf{Version} & \textbf{Datum} & \textbf{Bearbeiter} & \textbf{Änderung}\\
\midrule""")
    for eintrag in historie:
        t.append(" & ".join(tex(feld) for feld in eintrag) + r"\\")
    t.append(r"""\bottomrule
\end{tabular}
\clearpage""")

    # --- Querformat ab hier ------------------------------------------------
    t.append(r"""
%% Querformat: geometry kann die Papiergröße nachträglich nicht ändern,
%% daher werden Seitenmaße und Satzspiegel hier direkt gesetzt.
\paperwidth=%(breite).1fmm
\paperheight=%(hoehe).1fmm
\pdfpagewidth=\paperwidth
\pdfpageheight=\paperheight
\setlength{\hoffset}{0pt}
\setlength{\voffset}{0pt}
\setlength{\headheight}{0pt}
\setlength{\headsep}{0pt}
\setlength{\oddsidemargin}{%(seitenrand).2fmm}
\setlength{\evensidemargin}{%(seitenrand).2fmm}
\setlength{\topmargin}{%(oberrand).2fmm}
\setlength{\textwidth}{%(textbreite).2fmm}
\setlength{\textheight}{%(texthoehe).2fmm}
%% Satzspiegel-Änderungen mitten im Dokument wirken nur, wenn die davon
%% abgeleiteten Maße mitgezogen werden.
\setlength{\columnwidth}{\textwidth}
\setlength{\linewidth}{\textwidth}
\setlength{\hsize}{\textwidth}
\setlength{\vsize}{\textheight}
\makeatletter
\global\@colht=\textheight
\global\@colroom=\textheight
\makeatother
\setlength{\tabcolsep}{%(tabcolsep).2fmm}
\renewcommand{\arraystretch}{1.2}
\footnotesize""" % dict(breite=QUER_BREITE, hoehe=QUER_HOEHE,
                 links=QUER_RAND_LINKS, rechts=QUER_RAND_RECHTS,
                 seitenrand=QUER_RAND_LINKS - 25.4,
                 oberrand=QUER_RAND_OBEN - 25.4,
                 textbreite=QUER_BREITE - QUER_RAND_LINKS - QUER_RAND_RECHTS,
                 texthoehe=QUER_HOEHE - QUER_RAND_OBEN - QUER_RAND_UNTEN,
                 tabcolsep=TABCOLSEP_MM))

    # --- Tabelle --------------------------------------------------------
    ausrichtung = {"l": r">{\raggedright\arraybackslash}",
                   "r": r">{\raggedleft\arraybackslash}"}
    spezifikation = "@{}" + "".join(
        "%sp{%.2fmm}" % (ausrichtung[richtung], breite)
        for _, breite, richtung in spalten) + "@{}"
    kopfzeile = " & ".join(r"\textbf{%s}" % tex(name) for name, _, _ in spalten)
    kopfblock = [r"\toprule", kopfzeile + r"\\", r"\midrule"]

    t.append(r"\begin{longtable}{%s}" % spezifikation)
    t.extend(kopfblock)
    t.append(r"\endfirsthead")
    t.extend(kopfblock)
    t.append(r"\endhead")
    t.append(r"\bottomrule")
    t.append(r"\endfoot")
    t.append(r"\bottomrule")
    t.append(r"\endlastfoot")
    for satz in zeilen:
        t.append(" & ".join(tex(satz.get(name, "")) for name, _, _ in spalten) + r"\\")

    # Schlusszeile mit dem aufsummierten Gesamtbetrag
    namen = [name for name, _, _ in spalten]
    ergebnis = summe(zeilen) if SUMMENSPALTE in namen else None
    if ergebnis:
        gesamt = ergebnis[0]
        stelle = namen.index(SUMMENSPALTE)
        zellen = []
        if stelle:
            zellen.append(r"\multicolumn{%d}{@{}r}{\textbf{%s}}"
                          % (stelle, tex(SUMMENBESCHRIFTUNG)))
        zellen.append(r"\textbf{%s}" % tex(gesamt))
        zellen += [""] * (len(spalten) - stelle - 1)
        t.append(r"\midrule")
        t.append(" & ".join(zellen) + r"\\")
    t.append(r"\end{longtable}")
    t.append(r"\end{document}")
    return "\n".join(t) + "\n"


# ---------------------------------------------------------------------------
# Aufruf von pdflatex
# ---------------------------------------------------------------------------

def rendere(tex_datei):
    grundname = os.path.splitext(tex_datei)[0]
    befehl = 'pdflatex -interaction=nonstopmode -halt-on-error "%s" > /dev/null 2>&1' % tex_datei
    for _ in range(2):                  # zweiter Lauf für Seitenzahl und Blattnummer
        ergebnis = os.system(befehl)
    if ergebnis != 0 or not os.path.exists(grundname + ".pdf"):
        raise SystemExit("FEHLER: pdflatex ist fehlgeschlagen. Einzelheiten in %s.log"
                         % grundname)
    if AUFRAEUMEN:
        for endung in (".aux", ".log", ".out"):
            if os.path.exists(grundname + endung):
                os.remove(grundname + endung)
    return grundname + ".pdf"


def main():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    csv_datei = sys.argv[1] if len(sys.argv) > 1 else CSV_DATEI
    historie_datei = sys.argv[2] if len(sys.argv) > 2 else HISTORIE_DATEI

    for pfad in (csv_datei, historie_datei):
        if not os.path.exists(pfad):
            raise SystemExit("FEHLER: %s nicht gefunden." % pfad)

    felder, zeilen = lies_stueckliste(csv_datei)
    historie = lies_versionshistorie(historie_datei)

    open(TEX_DATEI, "w", encoding="utf-8").write(baue_tex(felder, zeilen, historie))
    pdf = rendere(TEX_DATEI)
    meldung = "Fertig: %s (%d Positionen, Version %s" % (pdf, len(zeilen), historie[-1][0])
    ergebnis = summe(zeilen)
    if ergebnis:
        gesamt, gelesen, insgesamt = ergebnis
        meldung += ", %s = %s aus %d von %d Werten" % (
            SUMMENSPALTE, gesamt, gelesen, insgesamt)
    print(meldung + ")")


if __name__ == "__main__":
    main()
