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

## Verteilerlisten

Verteilerlisten ermöglichen die Definition von Parametern, die für die
Berechnung von Abrechungswerten zum jeweiligen Miteigentumsanteil erforderlich sind.

Der Typ der Verteilerliste gruppiert relevante Attribute.

### Shares

Die Verteilung und Berechnung erfolgt über die numerischen Werte der
Eigentumsanteile im Verhältnis zur Summe aller Anteile.

* LabelSettlement: Die in einer Abrechung ausgewiesene Kennung
* SharesTotal: Summe aller Anteile (Default: 1000 Anteile)
* Eigentümer: Liste der einzubeziehenden Eigentümer Entitäten
  * Eigentümer-Id: Name/Id
  * Eigentümer-Anteil: Anteilswert

### Percentage shares
Die Verteilung und Berechnung erfolgt über die prozentualen Werte der Eigentumsanteile
im Verhältins zur Summe der prozentualen Anteilswerte.

* LabelSettlement: Die in einer Abrechung ausgewiesene Kennung
* SharesPercentage: Summe der prozentualen Anteilswerte (Default: 100%).
* Eigentümer: Liste der einzubeziehenden Eigentümer Entitäten
  * Eigentümer-Id: Name/Id
  * Eigentümer-Anteil: protentualer Anteilswert

## Konten-Zuweisung

In den Einzelabrechungen wird nur eine Teilmenge von Konten aus dem Kontenplan
ausgewiesen. Daher ist es notwendig solche Konten zu kennzeichnen, die in
einer Abrechnung herangezogen werden sollen.

Ziel ist eine flexible Definition dieser Abrechnungskonten. Leider
reicht die in GnuCash vorhandene Typisierung der Konten für eine
automatisierte Vorauswahl der in Gruppen auszuweisenden Konten nicht aus.

Vorhandene Type:

* Aktiva
  * Bank-Konten (Bank)
  * Bargeld-Konten (Cash)
  * Kreditkarten-Konten (Credit Card)
  * Investment-Konten (Mutual Fund)
  * Stock (Aktien)
  * Aktien-Handel (Trading)
* Passiva
  * Debitoren (Accounts Payable)
  * Kreditoren (Accounts Receivable)

* Eigenkapital (Equity)
* Forderung
* Verbindlichkeiten

* Aufwand
* Ertrag

Daher wurde die Liste der Kontentypen wie folgt ergänzt:

* Passiva
  * Erhaltungs-Rückstellungen

* Aufwand
  * Bewirtschaftungskosten umlagefähig
  * Bewirtschaftungskosten nicht umlagefähig
* Ertrag
  * Bewirtschaftungserträge umlagefähige
  * Bewirtschaftungserträge nicht umlagefähig

Damit ist eine Diffenzierung auch nachträglich in bestehenden Kontenplänen möglich.

## Einzelabrechung der Miteigentümer

Der neue Bericht definiert und erzeugt eine Einzelabrechung, die
folgende Auswahlkriterien berücksichtigt:

* je Miteigentümer
* der jeweils gültige Eingentumsanteile
* die selektierten, gruppierten Abrechungskonten

Wie in anderen Berichten üblich, kann dabei eine maximale
Kontenhierarchie-Tiefe eingestellt werden. Wird diese Tiefe überschritten,
werden alle Unterkonten als kummulierter Wert in deren hierarchisch
übergeordnetem Konto (parent-account) ausgewiesen.

Bei der Berichtsdefinition können die als default-Auswahl
vorgeschlagenen Konten individuell durch Anpassung der Kontenauswahl
angepasst werden.  Die default-Auswahl ergibt sich aus den im
Kontenrahmen angepaßten Kontentypen.

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
