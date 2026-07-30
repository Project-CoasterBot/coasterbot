# Ressourcen- und Kostenplan – CoasterBot

*Grundsätzlicher Plan, kalkuliert „als wären wir ein Unternehmen". Ausgerichtet an Projektstrukturplan und Vorgangsliste. Bezug zu Arbeitspaket **3040 – Kostenkalkulation durchführen**, sollten wir aber für M0 als Nachtrag vorziehen.*

**Hinweis:** Der Plan folgt der klassischen ingenieurmäßigen Projektkalkulation (WBS-basierte Aufwandsschätzung + Stundensatzermittlung nach dem Zuschlagsverfahren). 

---

## 1. Rahmen und Annahmen

| Punkt | Annahme |
|---|---|
| Team | 3 Ingenieure |
| Charakter | Studien-/Nebenprojekt, kalkuliert wie ein Unternehmen |
| Projektlaufzeit | 15.07.2026 – 16.10.2026 (≈ 13,5 Wochen) |
| Ziel | Prototyp **Maturity Level 1** |
| Werkzeuge | vorhanden (keine Anschaffung; im Gemeinkostensatz berücksichtigt) |
| Software | vollständig Open Source → 0 € Lizenzkosten |
| Material | muss beschafft werden; bisher ca. **40 €** ausgegeben |
| 3D-Druck | Plattform/Chassis steht noch aus |

Zwei Sichten werden getrennt ausgewiesen:
- **Unternehmenssicht (Vollkosten):** Personal wird mit kalkulatorischem Stundensatz bewertet (was das Projekt „kosten würde").
- **Reale Auslage:** nur tatsächliche Ausgaben der Gruppe (Material), da Arbeit, Werkzeuge und Software während Projektstart nicht bezahlt werden.

---

## 2. Ressourcenplan

### 2.1 Personal
| Rolle | Anzahl | Einsatz |
|---|---|---|
| Ingenieur (Entwicklung, HW/SW, Test) | 3 | anteilig „nebenbei" über die Laufzeit |

### 2.2 Sachmittel
| Ressource | Status | Kostenrelevanz |
|---|---|---|
| Werkzeuge (Werkstatt, Lötausrüstung, 3D-Drucker) | vorhanden | 0 € (im Overhead) |
| Software (IDE, Simulation, FreeCAD, KiCAD) | Open Source | 0 € |
| Elektronik/Mechanik-Material Prototyp | zu beschaffen | siehe §5 |
| 3D-Druck-Filament | zu beschaffen | siehe §5 |
| Rechner/Arbeitsplatz/IT | vorhanden | im Overhead |

---

## 3. Stundensatzermittlung (Zuschlagskalkulation)

Kalkulatorischer Stundensatz eines Ingenieurs, hergeleitet über Personal(voll)kosten, produktive Stunden, Gemeinkosten sowie Wagnis & Gewinn.

**Produktive Stunden pro Jahr:**

| Position | Stunden |
|---|---:|
| Bruttoarbeitszeit (52 Wo × 40 h) | 2.080 |
| − Urlaub (30 AT) | −240 |
| − Feiertage (≈ 11 Tage) | −88 |
| − Krankheit (≈ 8 AT) | −64 |
| = Anwesenheit | 1.688 |
| × Produktivitätsgrad ≈ 83 % (verrechenbar) | **≈ 1.400** |

**Stundensatz:**

| Position | Wert |
|---|---:|
| Jahresbruttogehalt Ingenieur (Annahme) | 54.000 € |
| + Arbeitgeber-Lohnnebenkosten (21 %) | 11.340 € |
| **= Arbeitgeber-Personalkosten p. a.** | **65.340 €** |
| ÷ produktive Stunden p. a. | 1.400 h |
| **= Personalkostensatz** | **46,70 €/h** |
| + Gemeinkostenzuschlag (90 % – Arbeitsplatz, IT, Werkzeuge/Abschreibung, Verwaltung, Leitung) | 42,00 €/h |
| **= Selbstkostensatz (Vollkosten)** | **≈ 89 €/h** |
| + Wagnis & Gewinn (12 %) | 10,60 €/h |
| **= Angebots-/Verrechnungssatz** | **≈ 100 €/h** |

> Für die interne Projektbewertung wird der **Selbstkostensatz 89 €/h** verwendet.
> Der Verrechnungssatz 100 €/h zeigt den Marktwert (Opportunitätskosten).
> Alle Gehalts-/Zuschlagswerte sind Annahmen und in der Tabelle leicht anpassbar.

---

## 4. Aufwandsschätzung je Arbeitspaket

Personenstunden je Arbeitspaket (AP-IDs analog `M0/Coasterbot_Vorgangsliste_v4.csv`).
Die dortige „Dauer" ist Vorgangsdauer in Tagen; hier wird der **Personalaufwand** geschätzt.

| AP | Arbeitspaket | Aufwand [h] |
|---|---|---:|
| 1001 | Steckbriefdefinition | 4 |
| 1002 | Projektorganisation und Planung | 12 |
| 1010 | Algorithmik recherchieren | 20 |
| 1020 | Marktsichtung durchführen | 20 |
| 1030 | Stand Mobile Robotik recherchieren | 20 |
| 101 | Dokumente zusammentragen | 6 |
| 2010 | Anwendungsfälle definieren | 10 |
| 2020 | Maturity Level definieren | 10 |
| 2030 | Lastenheft erstellen | 24 |
| 3010 | Testfälle definieren | 16 |
| 3020 | Sensorik/Aktorik auswählen | 20 |
| 3030 | Softwarearchitektur erarbeiten | 24 |
| 3040 | Kostenkalkulation durchführen | 12 |
| 3050 | Pflichtenheft erstellen | 30 |
| 201 | Dokumente zusammentragen | 4 |
| 4010 | Bauteile beschaffen | 10 |
| 4020 | Softwaretest erstellen | 40 |
| 4030 | Software implementieren | 120 |
| 4040 | Prototyp aufbauen | 50 |
| 5000 | Test durchführen | 40 |
| 6000 | Maturity Level feststellen | 12 |
| 301 | Dokumente zusammentragen | 6 |
| 401 | Endbericht erstellen und abgeben | 40 |
| 7000 | Präsentation ausarbeiten | 30 |
| | **Summe** | **580 h** |

**Aufwand je Phase:**

| Phase | Aufwand [h] | Anteil |
|---|---:|---:|
| Initialisierung & Recherche (10xx, 101) | 82 | 14 % |
| Anforderungen / Lastenheft (20xx) | 44 | 8 % |
| Konzept & Pflichtenheft (30xx, 201) | 106 | 18 % |
| Realisierung (40xx) | 220 | 38 % |
| Test & Abschluss (5000–7000, 301, 401) | 128 | 22 % |
| **Gesamt** | **580** | 100 % |

**Kapazitätsabgleich:** 580 h ÷ 3 Personen ≈ **193 h/Person** über 13,5 Wochen 
≈ **14–15 h/Woche/Person**. 

---

## 5. Materialkosten (Prototyp)

| Position | Betrag |
|---|---:|
| Bereits beschafft (Sensoren, TT-Motoren, TB6612-Treiber, Servos lt. Empfehlung) | 40 € |
| 3D-Druck Chassis/Plattform (Filament inkl. Iteration/Ausschuss) | 15 € |
| Ergänzung ML-1-Vollständigkeit (Encoder/IMU, zusätzl. Ultraschall, Microswitches, Status-LED/Buzzer, Not-Aus) | 90 € |
| Energieversorgung (Akkupack/Batteriehalter, Hauptschalter) | 15 € |
| Verbrauchsmaterial/Kleinteile (Kabel, Stecker, Lötzinn, Schrauben, Standoffs, Kabelbinder, Breadboard/Lochraster) | 20 € |
| **Zwischensumme** | **180 €** |
| Risikopuffer (15 %) | 27 € |
| **Materialkosten gesamt** | **≈ 207 €** |

> 3D-Druck als Eigenfertigung angesetzt (Drucker unter „vorhandene Werkzeuge").

---

## 6. Kostenzusammenfassung

### 6.1 Unternehmenssicht (Vollkosten)
| Kostenart | Berechnung | Betrag |
|---|---|---:|
| Personalkosten | 580 h × 89 €/h | **51.620 €** |
| Materialkosten | siehe §5 | 207 € |
| Software (Open Source) | — | 0 € |
| Werkzeuge (vorhanden) | im Overhead | 0 € |
| **Projektkosten gesamt (Vollkosten)** | | **≈ 51.800 €** |

*Zum Vergleich – Marktwert der Leistung: 580 h × 100 €/h ≈ **58.000 €** (Opportunitätskosten).*

### 6.2 Reale Auslage der Gruppe
| Kostenart | Betrag |
|---|---:|
| Material gesamt (§5) | 207 € |
| − bereits ausgegeben | −40 € |
| **Noch zu beschaffen (out-of-pocket)** | **≈ 167 €** |

Arbeitszeit, Werkzeuge und Software verursachen keine realen Ausgaben.

---

## 7. Annahmen, Risiken und Puffer

- **Aufwandsschätzung** beruht auf Erfahrungswerten; größte Unsicherheit bei
  *4030 Software implementieren* (120 h) und *4020/5000 Test*. Abweichung ±25 % möglich.
- **Kapazitätsrisiko:** ~15 h/Woche/Person sind für ein Nebenprojekt sportlich;
  Verzug wirkt direkt auf den kritischen Pfad (vgl. Netzplan `M0/`).
- **Materialrisiko:** Nachbeschaffung für vollständige ML-1-Abdeckung (Encoder/IMU
  etc.) noch nicht final entschieden → +/- im Materialbudget; 15 % Puffer angesetzt.
- **3D-Druck:** Iterationen (Passung Motorhalter, Reservoirs) können 1–2
  Nachdrucke erfordern → im Filamentansatz grob berücksichtigt.
- **Stundensatz-Parameter** (Gehalt, Zuschläge, produktive Stunden) sind Annahmen
  und sollten bei Bedarf durch reale/vorgegebene Werte ersetzt werden.
