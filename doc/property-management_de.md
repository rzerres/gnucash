# Hausverwaltung

Gnucash kann helfen, die Verwaltung Deines Eigentums zu vereinfachen.
Verwendest Du das Programm bereits für die Buchhaltung Deines Hauses,
bzw. Deiner Eigentumswohnung, werden in der aktuellen Version bereits
alle Aufgaben bereitgestellt, die für die hierzu erforderlich sind.
Um auch die Verwaltung von mehreren Wohnungen oder Vermietungsobjekten
in einem Gemeinschaftseigentum umzusetzewn, müssen weiter Aufgaben
bearbeitet werden können.

## Hintergrund

In Deutschland steht es einer Wohnungseigentümer-Gemeinschaften (WEG)
frei, das Gemeinschaftseigentum in Selbstverwaltung betreiben. Die
Aufgaben der zwingend vorgeschriebenen
[Hausverwaltung][property_mgmt], kann per einstimmigem Beschluss in
einer Eigentümer-Versammlung der WEG auf einen der Eigentümer
übertragen werden.

Sicherlich wird es vergleichbare Konstrukte in anderen Ländern
geben. Deren Adaption deren Regeln auf deren spezifische Vorgaben ist
sicher adaptierbar (EU, weltweit).

[property_mgmt]: https://www.hausverwaltung-ratgeber.de/hausverwaltung.html).

## Umsetzung

Den bereits vorhandenen Instrumentarien für Kunden und Lieferanten
folgend wurde eine neuer Typ `Miteigentümer` eingeführt. Dieser erhält
analog zu den anderen Typen folgende Dialoge:

* Erstellen
* Bearbeiten
* Auflisten
* Suchen

Vergleichbar zu Kunden können nunmehr für die `Miteigentümer`

* Rechnungen
* Bestellungen
* Zahlungen
* Abrechungen

erstellt und bearbeitet werden.

Für jeden angelegten Miteigentümer können Attribute ergänzt werden,
die sich je Wohneinheit unterscheiden und anzupassen sind.  Diese
werden in den Aufgaben für die Verwaltung der Wohnungseinheiten
ausgewertet.  Um beispielsweise korrekte Abrechungen je
Wohnungseinheit zu erzeugen, müssen folgende Objektattribute zugewiesen
werden:

* Wohnungsanteil am Gesamtobjekt
* Name der Wohungseinheit
* Anzuwendender Vereilungsschlüssel

Weiterhin muss der verwendete Kontenplan auf die Anforderungen
angepasst werden. Wie gewohnt werden anschließend die eigentlichen
Buchungen auf Unterkonten innerhalb dieser Kontenstruktur abgewickelt.

Die gesetzlichen Vorgaben (zumindest in Deutschland) schreiben eine
jährliche Abrechnung je Miteigentümer vor. Aus den Einzelabrechungen müssen
zumindest folgende anteiligen Positionen hervorgehen:

* Umlagefähige Beiträge
* Nicht-Umlagefähige Beiträge
* Einnahmen
* Individuelle Kosten

Darüber hinaus müssen die Zuführungen- und Abhebungen zur

* Erhaltungsrückstellung

ausgewiesen werden.

## Einzelabrechung der Miteigentümer

Das Ziel dieses neuen Berichts ist die Bereitstellung einer
Einzelabrechung je Miteigentümer für die Abrechungsperiode.

## Fachvokabular innerhalb einer Hausverwaltung

Die nachfolgend unvollständige Aufstellung listet einschlägige Fachbegriffe,
die üblicherweise im Zusammenhang mit einer Hausverwaltung stehen.
Diese sollten bei Übersetzungen Berücksichtigung finden:

Abrechung
Abrechnung des Eigentümers
Zusätzliche Zahlung
Betrag
Beträge, die nicht verteilt werden können
Beitrag zur Erhaltungsrücklage
Guthaben
Verteilungsschlüssel
Verteiler gesamt
Fälligkeitsdatum
Wohneinheit
Individuelle Buchhaltung
Name des Kontos
Name der Hausverwaltung
Anteil des Eigentümers
Eigentum
Verwaltung der Immobilie
Rückstellung für Instandhaltung
Quittung
Rekursiver Saldo
Anfrage
Eigentümer abrechnen
Abrechnungstage
Abrechnungsspitze
Verwaltungskosten insgesamt
Gesamtbetrag
Abrechnung insgesamt
Gesamtergebnis

Klicken Sie auf die Schaltfläche Optionen und wählen Sie den
Eigentümer aus, für den Sie eine Abrechnung erstellen möchten.

Diese Eigentümerabrechnung ist ohne Unterschrift gültig
