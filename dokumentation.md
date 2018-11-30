# Projekt Tino
"**TI**ny **NO**de" : Batteriebetriebener Funksensor oder Funk-Aktor. 
Ziel dieses Projekts ist die Entwicklung schnurloser Funk Sensoren, 
die über Batterien versorgt werden und z.B. mit dem Raspberry Pi kommunizieren.
Die Entwicklung hat zum Ziel:

- minimale Kosten
- minimale Grösse
- minimaler Stromverbrauch
- maximale Batterielebensdauer
- maximale Reichweite
- maximale Sicherheit
- maximale Einfachheit
- Plug&Play Firmware

Als Sensor kann man so ziemlich alles verwenden, ob Temperatur, Luftfeuchtigkeit, Luftdruck, Höhenmesser, Lichtintensität, UV Index, 
Anwesenheitssensoren, Magnetschalter, Erschütterungs-Sensoren, Feuchtigkeitsmesser usw also im Prinzip alle Arten von Sensoren.

Die Leiterplatten passen in im Handel erhältlichen PVC Gehäusen, welche in etwa die Grösse einer Streichholzschachtel haben.
## Inhalt
- [Beschreibung des Projekts](#beschreibung-des-projekts)
- [Das Konzept](#das-konzept)
- [Wie Funktionierts?](#wie-funktionierts)
- [IDE Einrichten](#ide-einrichten)
- [Software Kompilieren und Flashen](#software-kompilieren-und-flashen)
- [Nodes Konfigurieren](#nodes-konfigurieren)
- [Nachbau](#nachbau)
- [Elektronik](#elektronik)
- [Leiterplatten](#leiterplatten)
- [Mechanik](#mechanik) (Gehäuse)

## Beschreibung des Projekts
Der TiNo Sensor macht periodisch eine Messung, z.B. der Umgebungstemperatur und sendet das Ergebnis mit einer kurzen HF Aktivität ("Puls") an eine Basisstation. Diese breitet das Signal auf und speichert das Ergebnis in einer Datenbank, die z.B. von einem Cloud Service ausgewertet werden kann. Der TiNo kann gleichzeitig auch Ueberwachungsaufgaben übernehmen, wie z.B das Oeffnen einer Türe melden. 

Die Idee zu dieser Entwicklung wurde bei mir durch ein Projekt im [deutschen Raspberry Pi Forum](https://forum-raspberrypi.de/forum/thread/7472-batteriebetriebene-funk-sensoren/) ausgelöst.
Ich möchte mich an dieser Stelle nocheinmal herzlich bei allen Beteiligten für die harte Arbeit die dahinter steckt bedanken. 
Da wurde mit viel Energie und Enthusiasmus ein System zum Senden und Empfangen von Sensordaten "zusammengebaut", was zur Entwicklung der "TinyTx4" und "TinyRX3" Platinen auf der Basis des ATtiny84a Prozessors geführt hat. ~~Ich habe lange Zeit mit diesen Platinen experimentiert, versucht sie mit Sensoren kombiniert in anschauliche (also mit hohem WAF) Boxen zu montieren und zu betreiben, kam aber schnell an die Grenzen dessen was ich mir eigentlich vorgestellt hatte. Mangelndes Talent für mechanische Dinge kommt hinzu.~~

Für Ein Endprodukt eignen sich die TinyTx Platinen leider nicht, aber als Experimentier- und Lernplatform haben sie mir unschätzbare Dienste geleistet. Zunächst ging es mir um technische Verbesserungen: Optimierung des Link-Budgets, ein binäres, wesentlich kürzeres Protokoll, Verbesserungen beim Stromverbrauch und beim Ruhestrom, ich experimentierte HF-seitig unter anderem mit FEC (Forward Error Correction), Interleavern, Baud Raten und Filterbandbreiten, und schliesslich war klar: ein anderer Prozessor muss her, der ATtiny mit seinen 8kB Flash kann nicht mithalten.

Ich begann von vorne. Mein Ansatzpunkt war aber nicht erst eine Platine zu entwerfen und mich dann um die Mechanik zu kümmern, sondern umgekehrt. Und so habe ich die TiNo Platinen an das Gehäuse und ihren Zweck angepasst und nicht umgekehrt. 
Herausgekommen ist ein in allen Parametern optimierter Funksensor in der Grösse einer Streichholzschachtel, welchen ich Tino = **TI**ny **NO**de nenne. Später kann man daraus dann das **TI**ny **N**etw**O**rk machen, denn auf der Softwareseite, speziell am Gateway gibt es noch unendlich viel Arbeit.
Auch das erklärte Ziel der Einfachheit des Systems ist noch nicht erreicht. Das Einrichten der IDE, das Flashen und das Individualisieren der jeweiligen Nodes soll in Zukunft in einem einzigen Schritt erfolgen.
## Das Konzept
### Minimale Kosten:
Momentan ist die Auswahl des ATMega328 Processors mit dem RFM69CW HF-Modul die kostengünstigste Variante. Weiterhin kann auf LDO's oder DC-DC Konverter verzichtet werden, da alle Module direkt von einer 3V CR2032 Zelle, zwei AAA Batterien oder zwei AA Batterien betrieben werden können. Weitere Kosten können durch verkleinern der Leiterplatte erzielt werden, so kann man sog. Nutzen (Panels) erzeugen bei denen das selbe Layout 4/6/8 mal auf eine Leiterplatte gebracht wird. 
### minimale Grösse
Aufgrund der verwendeten Frequenzen kann man die Baugrösse eines Sensors nicht beliebig verkleinern ohne grosse Zugeständnisse an die Performance zu machen. 
Mit Platinenabmessungen von ca. 35 x 50 mm habe ich im 868MHz ISM Band gute Erfahrungen gemacht. Im 433MHz ISM Band funktioniert das auch durchaus zufriedenstellend. 
### minimaler Stromverbrauch
Konsequente Umsetzung der Möglichkeiten des ATMega328 Prozessors. Optimierung der HF Parameter. Optimierung der Stromversorgung. 
### maximale Batterielebensdauer
Die Lebensdauer der Batterie hängt vom Einsatzfall ab. Sie setzt sich im Wesentlichen aus den vier folgenden Komponenten zusammen: 
1. der Batterie. Hier wird eine CR2032 Knopfzelle verwendet, mit geschätzten 170mAh nutzbarer Kapazität.
2. der Energie die bei einem Sendeimpuls verbraucht wird. In diesem Projekt habe ich die Dauer des Impulses sowie seine Leistung optimiert. Sehr komplexes Thema.
3. dem Ruhestrom. Auch der Ruhestrom wurde optimiert. Mit externem Uhrenquarz 32.678MHz arbeitet der Prozessor im Sleep Modus wie eine RTC (Real Time Clock), ich habe Ruheströme von 1.2µA gemessen. Ohne den Uhrenquarz verwendet man den internen RC Oszillator, da messe ich ca. 4µA, das ist aber immer noch ein sehr guter Wert.
4. der Anzahl der Sendepulse pro Zeit. bei einer Rate von einem Puls pro Minute überwiegt der Stromverbrauch der Sendepulse. Bei einer Rate von einem Puls pro Stunde überwiegt der Stromverbrauch durch den Ruhestrom. 
Bei einem typischen Temperatur/Luftfeuchte Sensor, der alle 30 Minuten eine Messung sendet kann eine CR2032 Zelle 5 Jahre oder länger halten. 
### maximale Reichweite
Optimierung des Layouts. Optimierung der HF Parameter. Dadurch verringert sich die Eingangsempfindlichkeit drastisch. Optimierung des HF treibers um maximale Sendeleistung zu erreichen.
### maximale Sicherheit
Das Sendeprotokoll ist verschlüsselt. Der Schlüssel kann nicht aus dem Flash gelesen werden wenn das Basisband versperrt wurde. Um zu  verhindern dass ein einmal aufgezeichnetes Signal bei Wiederholung etwas auslöst wird ein "rolling code" verwendet, der sicherstellt dass jedes Protokoll "einmalig" ist.
### Einfachheit
Da gibts noch Raum für Verbesserung.
#### Plug&Play Firmware
Es wird ein Modul im Boards Manager der Arduino IDE bereitgestellt.

## Wie Funktionierts?
### Softwarearchitektur
### Hardwarearchitektur

## IDE Einrichten
- Arduino IDE starten.
- `File->Preferences` öffnen.
- Unter `Additional Boards Manager URL's` diesen Link eintragen:
https://raw.githubusercontent.com/nurazur/TiNo/master/package_tino_index.json
- Navigiere zu `Tools->Board`: eine lange Spalte öffnet sich. Ganz oben auf `Boards Manager...` klicken
- Im Boards Manager nach `Tiny Node AVR Boards` suchen.
- Auf `Install` klicken. 
Dies installiert die Bibliotheken die zum Betrieb des Funkprotokolls gebraucht werden. 
### Empfänger (Gateway)
Der Empfänger oder besser "Gateway" wird für ein Board mit 8 MHz Taktfrequenz kompiliert. 
hierzu je nach Wahl die Version `Tools->Setup->Gateway, 8MHz, internal Osc` (ohne Bootloader) oder `Setup->Gateway, 8MHz, int.Osc., bootloader` (mit Bootloader)
 auswählen. Dann müssen die "Fuses" dementsprechend programmiert werden. 
Dies sind drei Byte geschützter Speicher im Prozessor, die das Verhalten des Prozessors bestimmen; was die einzelnen Bits im Einzelnen bedeuten steht im Datenblatt des Prozessors.
Auf [dieser Webseite](http://www.engbedded.com/fusecalc) kann man die Fuses bestimmen, bzw. dekodieren was sie bedeuten. Im Wesentlichen stellen wir hier ein mit welchem Takt der Prozessor arbeiten soll, ob mit externem Quarz oder internem Taktgeber und ob ein Bootloader verwendet werden soll oder nicht.
In der Arduino IDE wird dieser Vorgang zusammen mit dem Flashen des Bootloaders vorgenommen. Dies macht man für jedes Board genau einmal, danach kann man Sketches flashen ohne die Fuses brennen zu müssen. Der Ausdruck "Burn Bootloader" ist etwas verwirrend, denn wenn wir die Variante ohne Bootloader wählen, müssen wir diesen Vorgang dennoch durchführen um die Fuses zu "brennen".
Fuses flashen (einmaliger Vorgang):
1. Programmer an den ISP des Boards anschliessen
2. COM Port des Programmers auswählen `Tools->Port`
3. Klick auf `Tools->Burn Bootloader` 

Alternativ kann man, wenn man ein Poweruser ist, das Tool avrdude einrichten und direkt von der Kommandozeile aus verwenden. Dies hat eine Reihe von Vorteilen, da man damit einfach die Fuses abfragen kann, das EEPROM löschen/flashen oder auch fertig kompilierte .hex Dateien direkt auf das Board flashen kann ohne mühsam durch die Arduino IDE klicken zu müssen. 
### Sender 
Der Sender oder besser "Node" (Knoten in einem Netzwerk) wird für ein Board mit 1 MHz Takt kompiliert. Die "Fuses" müssen dementsprechend programmiert werden. Der Vorgang ist mit dem beim Empfänger identisch.
 
### Weitere Bibliotheken Installieren  
Folgende Bibliotheken braucht man zusätzlich zur Installation des TiNo Boards:
- SoftwareWire        (für I2C Bus Sensoren)
- HTU21D_SoftwareWire (für den HTU21D Sensor Sketch)
- SHT3x_SoftwareWire  (für den SHT3x Sketch)
- PinChangeInterrupt  (Interrupts werden standardmässig unterstützt)
- Lowpower             (wenn man einen externen Uhrenquarz benutzt)

Diese Bibliotheken sind nicht mit im TiNo Package enthalten (derzeit)
## Kompilieren und Flashen
Dazu braucht man, zumindest kurzfristig, einen Programmer mit [ISP Adapter](https://www.arduino.cc/en/Tutorial/ArduinoISP)
Zum Flashen der Boards gibt es zwei Konzepte:
1. Einmalig mit dem ISP Programmer auf das Board einen Bootloader flashen. Der eigentliche Sketch wird dann über das serielle Interface des Boards geladen. Vorteil: Während der Entwicklungsphase kann man mit dem selben Interface flashen und testen, ohne das Interface wechseln zu müssen.
2. Den Sketch direkt ohne Bootloader mit einem ISP Adapter flashen. Vorteil: In der Produktionsphase können Nodes in einem einzigen Arbeitsschritt geflasht werden. Da kein Bootloader vorhanden ist, hat man auch mehr Flash Speicher zur Verfügung.
In beiden Fällen braucht man einen Programmer mit [ISP Adapter](https://www.arduino.cc/en/Tutorial/ArduinoISP).
Diesen kann man sich leicht mit einem Arduino UNO oder einem Arduino Nano selbst herstellen. 
### mit FTDI und USB-Seriell Adapter
Einmaliger Vorgang, wenn man das Board zum ersten mal startet:
- Wenn man einen Arduino (z.B.UNO) als Programmer benutzt: Bei Tools -> Programmer "Arduino as ISP" auswählen (nicht "ArduinoISP"!)
- Ansonsten den Programmer seiner Wahl auswählen
- COM Port des Programmers einstellen unter Tools->Port
- Setup auswählen: "bootloader", je nach Anwendung ist das 'Sensor' oder 'Gateway'. 
- Tools-> Burn Bootloader klicken. Jetzt wiird das Board mit den zuvor gewählten Parametern konfiguriert und der Bootloader geflasht.

Normales Flashen:
- COM Port des seriellen Interfaces auswählen: Tools -> Port:
- in der IDE Sketch -> Upload klicken. Jetzt wirdder Sketch kompiliert und geflasht.

### Flashen mit ISP Adapter und Programmer
Einmaliger Vorgang, wenn man das Board zum ersten mal startet:
-Setup je nach Anwendung auswählen, "Sensor" oder "Gateway". Die Option ohne Bootloader wählen.
Normales Flashen:
- Wenn man einen Arduino (z.B.UNO) als Programmer benutzt: Bei Tools -> Programmer "Arduino as ISP" auswählen (nicht "ArduinoISP"!)
- Ansonsten den Programmer seiner Wahl auswählen
- COM Port einstellen unter Tools->Port
- In der Arduino IDE "sketch -> Upload using Programmer" klicken. Jetzt kompiliert der Sketch, eventuell mit Warnungen (die man aber ignorieren kann) und lädt den binären Code auf das Board hoch. 

  
## Nodes Konfigurieren
Nach dem Flashen sind die Nodes und die Gateways noch nicht betriebsbereit (leider, wird verbessert). Das EEPROM muss zuerst mit sinnvollen Daten gefüllt, "kalibriert" werden. 
### EEPROMer Python tool
nach dem Start eines TiNos liest das TiNo Board das EEPROM. Da die Daten verschlüsselt sind, werden sie zunächst entschlüsselt und die Prüfsumme gebildet. 

Wenn die Prüfsumme mit der aus dem EEPROM gelesenen übereinstimmt:
Ueber den seriellen Port wird der String "CAL?" gesendet. Egal ob die Prüfsumme stimmt, wenn innerhalb von 250ms ein 'y' zurückkommt, geht der Node in den Kalibriermodus. 
Kommt keine Antwort, sendet der Node, nur als Debugnachricht, ein "timeout". Also nicht wundern über das Timeout, das zeigt an dass alles in Ordnung ist. Man sieht es allerdings nur wenn man das TiNo Board direkt mit einem Terminalprogram (z.B. minicom oder TeraTerm) verbindet.

Wenn die Prüfsumme nicht übereinstimmt:
Der Node geht direkt inden Kalibriermodus.

Das EEPROMer Tool ist in Python geschrieben, man braucht also unter Windows eine Installation von Python. Wenn man das EEPROMer Tool startet, öffnet es zunächst den seriellen Port. An einem FTDI Adapter bewirkt das, dass der angeschlossene Node neu startet, Dann wartet das Tool auf das 'CAL?' vom Node, und sendet ggf. das 'y' sofort zurück um den Kalibriermodus zu erzwingen. 
Sobald das Tool meldet dass man im Kalibriermodus ist, muss man das Passwort eingeben. Dies ist mit dem KEY Parameter im Source Code identisch. Derselbe KEY wird auch zum Verschluesseln des HF Pakets benutzt. Das EEPROM ist verschluesselt, weil sonst ein Dieb einen Node ohne weiteres komplett umkonfigurieren koennte und damit wild in der Gegend herumfunken kann (oder noch Schlimmeres anrichten kann), ohne dass er das Passwort kennen muestte. 

Passwort eingeben:
- entweder das im Eepromer Tool hinterlegte Passwort verwenden: `pwd<Enter>` eingeben
- oder ein anderes Passwort verwenden (muss aber immer mit dem kompilierten Passwor identisch sein): `pw,<Passwort><Enter>` eingeben

Nachdem das TOOL "Pass OK" meldet, kann man mit dem Konfigurieren beginnen. 
Folgende Syntax wird von dem Tool verstanden:
- help listet die Optionen auf

'exit'          terminate program
'help'  or '?'  print this help text
'c'             measure ADC and store in EEPROM.
'copy' or 'cp'  copy file content to EEPROM. syntax: cp, <filename>
'cs'            verify checksum.
'fe'            receive 10 packets from external source, calculate mean and store in EEPROM
'g' or 'get'    store eeprom content to file. Syntax: 'g(et),<filename>'
'ls'            List EEPROM content.
'm'             Measure VCC with calibrated values
'quit'          terminate program
'read'  or 'r'  read from EEPROM. Syntax: 'r(ead),<addr>'
'ri'            read 16 bit integer from EEPROM. Syntax: 'ri(ead),<addr>'
'rf'            read float from EEPROM. Syntax: 'ri(ead),<addr>'
's'             request checksum update and store in EEPROM.
'vddcal'        calibrate VCC measurement. Syntax: 'v(ddcal),<VCC at device in mV>'
'write' or 'w'  write value to to EEPROM.  Syntax: 'w(rite),<addr>,<value>'
'wf'            write float value to EEPROM. Syntax: 'wf,<addr>,<value>'
'wl'            write long int value to to EEPROM.  Syntax: 'wl,<addr>,<value>', value format can be hex
'wu'            write unsigned int value to EEPROM. Syntax: 'wu,<addr>,<value>'
'x'             exit calibration mode and continue with loop()

Das Tool ist zuallererst interaktiv, d.h. man greift von Hand auf da EEPROM zu und kann es so konfigurieren. Allerdings ist das nur bis zu einem bestimmten Grad praktisch, z.B. wenn man nur mal schnell eine ID aendern wiill, oder wenn man nur die Integritaet des EEPROMs feststellen will. 
Wenn man mehr machen will/muss, waere eine automatisierte Version praktisch. Ich arbeite daran. 

Im Moment kann man die wichtigsten Aktionen durch die Kommandozeile ausloesen. 
Unterstuetzt werden zur Zeit folgende Optionen:
-pwd            Sende das im Tool hinterlegte Passwort
-cs             lies die Pruefsumme vom EEPROM
-ls             Liste der EEPROM Werte
-cp,<Dateiname> Kopieren des Inhalts einer Konfigurationsdatei in das EEPROM
-s              berechnen und speichern der Pruefsumme
-x              EEPROM verschluesseln und Daten abspeichern
-q              Tool beenden

eine Kommandozeile sieht dann beispielsweise so aus:

`python eepromer_win_v007.py COM8 38400 -pwd -cp,receive_eeprom.cfg -ls -cs -x -q`

In diesem Fall verbindet sich der Eepromer Tool mit dem TiNo Board auf COM8, 38400 Baud und arbeitet dann die liste der Optionen in der Reihenfolge ab, also:
1. `-pwd` sendet das im Programm hinterlegte Passwort an da TiNo Board
2. `-cp,receive_eeprom.cfg` kopiert den Inhalt der Datei `-cp,receive_eeprom.cfg` vom PC auf das TiNo Board
2. `-ls` listet den Inhalt des EEPROMs.
3. `-cs` liest die Pruefsumme und prueft sie.
4. `-x` verlaesst den Kalibriermodus
5. `-q` beendet das Tool

Eine Vewrsion fuer denRaspberry Pi gibt es auch, wird demnaechst geliefert.
### EEPROM Speicher erklärt:
    PCI Trigger Byte Bitbelegung
        PCIxTrigger bits 0 and 1:
            0bxx00 LOW
            0bxx01 CHANGE
            0bxx10 FALLING (normal use case)
            0bxx11 RISING
            
            PCIxTrigger bits 2 and 3:
            0b00xx INPUT
            0b01xx OUTPUT
            0b10xx INPUT_PULLUP    
## Nachbau
### Vorausetzungen: Was braucht man?
Hardware
- USB-Seriell Adapter (FTDI oder kompatibel, CH340 geht auch aber auf das Pinning achten!)
- ISP-Programmer
- Gateway: etwas das einen seriellen Port öffnen, lesen, schreiben, anzeigen und speichern kann (PC, Raspberry Pi,...)

Software
- Python
- Arduino IDE    
## Elektronik 
### Schaltplan erklärt
### Stueckliste
### Messergebnisse
## Leiterplatten
## Mechanik (Gehäuse)


