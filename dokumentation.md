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
- [Software Kompilieren und Flashen](#Kompilieren-und-Flashen)
- [Nodes Konfigurieren](#nodes-konfigurieren)
- [Nachbau](#nachbau)
- [Elektronik](#elektronik)
- [Leiterplatten](#leiterplatten)
- [Mechanik](#mechanik) (Gehäuse)

## Beschreibung des Projekts
Der TiNo Sensor macht periodisch eine Messung, z.B. der Umgebungstemperatur und sendet das Ergebnis mit einer kurzen HF Aktivität ("Puls") an eine Basisstation. Diese bereitet das Signal auf und speichert das Ergebnis in einer Datenbank, die z.B. von einem Cloud Service ausgewertet werden kann. Der TiNo kann gleichzeitig auch Ueberwachungsaufgaben übernehmen, wie z.B das Oeffnen einer Türe melden.

Die Idee zu dieser Entwicklung wurde bei mir durch ein Projekt im [deutschen Raspberry Pi Forum](https://forum-raspberrypi.de/forum/thread/7472-batteriebetriebene-funk-sensoren/) ausgelöst.
Ich möchte mich an dieser Stelle nocheinmal herzlich bei allen Beteiligten für die harte Arbeit die dahinter steckt bedanken.
Da wurde mit viel Energie und Enthusiasmus ein System zum Senden und Empfangen von Sensordaten "zusammengebaut", was zur Entwicklung der "TinyTx4" und "TinyRX3" Platinen auf der Basis des ATtiny84a Prozessors geführt hat.

Zunächst ging es mir um technische Verbesserungen: Optimierung des Link-Budgets, ein binäres, wesentlich kürzeres Protokoll, Verbesserungen beim Stromverbrauch und beim Ruhestrom, ich experimentierte HF-seitig unter anderem mit FEC (Forward Error Correction), Interleavern, Baud Raten und Filterbandbreiten, und schliesslich war klar: ein anderer Prozessor muss her, der ATtiny mit seinen 8kB Flash kann nicht mithalten.

Ich begann von vorne. Bei der Gelegenheit habe ich die TiNo Platinen an das Gehäuse und ihren Zweck angepasst und nicht umgekehrt.
Herausgekommen ist ein in allen Parametern optimierter Funksensor in der Grösse einer Streichholzschachtel, welchen ich TINO = **TI**ny **NO**de nenne. Später kann man daraus dann das **TI**ny **N**etw**O**rk machen, denn auf der Softwareseite, speziell am Gateway gibt es noch unendlich viel Arbeit.

## Das Konzept
### Minimale Kosten:
Momentan ist die Auswahl des ATMega328 Processors mit dem RFM69CW HF-Modul die kostengünstigste Variante. Weiterhin kann auf LDO's oder DC-DC Konverter verzichtet werden, da alle Module direkt von einer 3V CR2032 Zelle, zwei AAA Batterien oder zwei AA Batterien betrieben werden können. Weitere Kosten können durch verkleinern der Leiterplatte erzielt werden, so kann man sog. Nutzen (Panels) erzeugen bei denen das selbe Layout 4/6/8 mal auf eine Leiterplatte gebracht wird.
### minimale Grösse
Aufgrund der verwendeten Frequenzen kann man die Baugrösse eines Sensors nicht beliebig verkleinern ohne grosse Zugeständnisse an die Performance zu machen.
Mit Platinenabmessungen von ca. 35 x 50 mm habe ich im 868MHz ISM Band gute Erfahrungen gemacht. Im 433MHz ISM Band funktioniert das ebenfalls durchaus zufriedenstellend.
### minimaler Stromverbrauch
Konsequente Umsetzung der Möglichkeiten des ATMega328 Prozessors. Optimierung der HF Parameter. Optimierung der Stromversorgung. Verwendung stromsparender Sensoren.
### maximale Batterielebensdauer
Die Lebensdauer der Batterie hängt vom Einsatzfall ab. Sie setzt sich im Wesentlichen aus den vier folgenden Komponenten zusammen:
1. die Batterie. Der TiNo hat einen CR2032 Knopfzellenhalter eingebaut. Eine CR2032 hat geschätzt 200mAh nutzbare Kapazität.
2. die Energie die bei einem Sendeimpuls verbraucht wird. In diesem Projekt habe ich die Dauer des Impulses sowie seine Leistung optimiert.
3. der Ruhestrom. Auch der Ruhestrom wurde optimiert. Mit externem Uhrenquarz 32.678MHz arbeitet der Prozessor im Sleep Modus wie eine RTC (Real Time Clock), ich habe Ruheströme von 1.2µA gemessen. Ohne den Uhrenquarz verwendet man den internen RC Oszillator, da messe ich ca. 4µA, das ist aber immer noch ein sehr guter Wert.
4. die Anzahl der Sendepulse pro Zeit. bei einer Rate von einem Puls pro Minute überwiegt der Stromverbrauch der Sendepulse. Bei einer Rate von einem Puls pro Stunde überwiegt der Stromverbrauch durch den Ruhestrom.
Bei einem typischen Temperatur/Luftfeuchte Sensor, der alle 30 Minuten eine Messung sendet kann eine CR2032 Zelle 5 Jahre oder länger halten.
[In meinem Blog](https://nurazur.wordpress.com/2018/02/25/batterielebensdauer-eines-tx-nodes-verlaengern-mit-einem-32-768-khz-quarz/) gibts das Ganze sehr ausführlich erklärt.
### maximale Reichweite
Optimierung des Layouts. Optimierung der HF Parameter. Dadurch verringert sich die Eingangsempfindlichkeit drastisch. Optimierung des HF Treibers um maximale Sendeleistung zu erreichen.
### maximale Sicherheit
Das Sendeprotokoll ist verschlüsselt. Der Schlüssel kann nicht aus dem Flash gelesen werden wenn das Basisband versperrt wurde. Um zu verhindern dass ein einmal aufgezeichnetes Signal bei Wiederholung etwas auslöst wird ein "rolling code" verwendet, der sicherstellt dass jedes Protokoll "einmalig" ist.
### Einfachheit
Da gibts noch Raum für Verbesserung.
#### Plug&Play Firmware
Es wird ein Modul im Boards Manager der Arduino IDE bereitgestellt.
#### Unkomplizierte Hardware
## Wie Funktionierts?
Ein TiNo ist ein Sensor der periodisch eine Messung vornimmt und diese per Funksignal an ein Gateway überträgt.  Dieses Gateway besteht normalerweise aus einem als Empfänger konfigurierten TiNo der an einen Raspberry Pi über dessen seriellen Port angeschlossen ist. Das Gateway empfängt die Funksignale, dekodiert sie und wenn die Daten sinnvoll sind werden diese an den Raspberry Pi weitergegeben. Ausserdem quittiert das Gateway die Nachricht auf Aufforderung. Das zwischen Gateway und Raspberry von mir verwendete Protokoll ist einfach, könnte aber im Prinzip jedem beliebigen Standard folgen, z.B. RFLink.
TiNos können standardmässig auch auf externe Signale reagieren, z.B Tueröffner Kontakte.
Auf dem Raspberry Pi läuft ein Python Programm, welches die vom Gateway ankommenden Pakete aufbereitet und in eine Datenbank schreibt. Eine Web Applikation kann dann auf diese Daten zugreifen und daraus Graphiken etc machen. Dies ist aber (noch) nicht Gegenstand des TiNo Projekts.
Um eine Funkverbindung herzustellen braucht man zwei TiNos, einen als Sensor konfigurierten und einen als Gateway konfigurierten.

### Hardwarearchitektur
Die TiNo Boards sind so einfach wie möglich aufgebaut:
- direkt von einer 3V  Batterie gespeist
- ATMega328p-au Prozessor
- RFM69CW oder RFM69HCW oder RFM95 Modul von HopeRF
- Footprint für einen HTU21D/SHT20/SHT21/SHT25 ist auf der Leiterplatte
- Anschlussmöglichkeiten für einen I2C Sensor beliebiger Wahl.
- Ein Batteriehalter
- ISP (in-System-Programming) Adapter, 2x3 Pin Leiste
- FTDI Adapter, 1x6 Pin Leiste
- je nach Board verschiedene GPIO's die man für alle möglichen digitalen Ereignisse (z.B. Tastendruck) konfigurieren kann
- Status LED
- auf manchen Boards ist eine optionale SMA Buchse vorgesehen (externe Antenne)
- alle Boards sind jeweils für ein bestimmtes Gehäuse konzipiert, können aber auch anderweitig eingesetzt werden.
![TiNo HP Anschlüsse Oberseite](https://github.com/nurazur/TiNo/blob/master/TiNo-HP-Top-Pinout.jpg)
![TiNo HP Anschlüsse Unterseite](https://github.com/nurazur/TiNo/blob/master/TiNo-HP-Bottom-Pinout.jpg)
### Softwarearchitektur
![TiNo Sensor Software Architektur](https://github.com/nurazur/TiNo/blob/master/sw_flow.jpg)


## IDE Einrichten
- Arduino IDE starten.
- `File->Preferences` öffnen.
- Unter `Additional Boards Manager URL's` diesen Link eintragen:
`https://raw.githubusercontent.com/nurazur/TiNo/master/package_tino_index.json`
- Navigiere zu `Tools->Board`: eine lange Spalte öffnet sich. Ganz oben auf `Boards Manager...` klicken
- Im Boards Manager nach `Tiny Node AVR Boards` suchen.
- die neueste Freigabe auswählen.
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

Alternativ kann man, wenn man ein Poweruser ist, das Tool *avrdude* einrichten und direkt von der Kommandozeile aus verwenden. Dies hat eine Reihe von Vorteilen, da man damit einfach die Fuses abfragen kann, das EEPROM löschen/flashen oder auch fertig kompilierte .hex Dateien direkt auf das Board flashen kann ohne mühsam durch die Arduino IDE klicken zu müssen.
### Sender
Der Sender oder besser "Node" (Knoten in einem Netzwerk) wird für ein Board mit 1 MHz Takt kompiliert. Die "Fuses" müssen dementsprechend programmiert werden. Der Vorgang ist mit dem beim Empfänger identisch.

### Weitere Bibliotheken Installieren
Die notwendigen Bibliotheken für die Sensoren sind im TiNo Package enthalten. Sie stehen auch im Github Repository.
Folgende Bibliotheken braucht man zusätzlich zur Installation des TiNo Boards:
- *DallasTemperature*   (für den DS18B20 Sketch)
- *OneWire*             (für den DS18B20 Sketch)

Diese Bibliotheken sind ab Version 3.0.3 mit im TiNo Package enthalten.

## Kompilieren und Flashen
Dazu braucht man, zumindest kurzfristig, einen Programmer mit [ISP Adapter](https://www.arduino.cc/en/Tutorial/ArduinoISP).
Zum Flashen der Boards gibt es zwei Konzepte. In beiden Fällen braucht man einen Programmer mit [ISP Adapter](https://www.arduino.cc/en/Tutorial/ArduinoISP).
Diesen kann man sich leicht mit einem Arduino UNO oder einem Arduino Nano selbst herstellen. Die Alternativen sind:
1. Einmalig mit dem ISP Programmer auf das Board einen Bootloader flashen. Der eigentliche Sketch wird dann über das serielle Interface des Boards geladen. Vorteil: Während der Entwicklungsphase kann man mit dem selben Interface flashen und testen, ohne das Interface wechseln zu müssen.
2. Den Sketch direkt ohne Bootloader mit einem ISP Adapter flashen. Vorteil: In der Produktionsphase können Nodes in einem einzigen Arbeitsschritt geflasht werden. Da kein Bootloader vorhanden ist, hat man auch mehr Flash Speicher zur Verfügung.

### mit FTDI und USB-Seriell Adapter
Einmaliger Vorgang, wenn man das Board zum ersten mal startet:
- Wenn man einen Arduino (z.B.UNO) als Programmer benutzt: Bei `Tools->Programmer` "Arduino as ISP" auswählen (nicht "ArduinoISP"!)
- Ansonsten den Programmer seiner Wahl auswählen
- COM Port des Programmers einstellen unter `Tools->Port`
- Setup auswählen: "bootloader", je nach Anwendung ist das 'Sensor' oder 'Gateway'.
- `Tools->Burn Bootloader` klicken. Jetzt wird das Board mit den zuvor gewählten Parametern konfiguriert und der Bootloader geflasht.

Normales Flashen:
- COM Port des seriellen Interfaces auswählen: Tools -> Port:
- in der IDE Sketch -> Upload klicken. Jetzt wirdder Sketch kompiliert und geflasht.

### Flashen mit ISP Adapter und Programmer
Einmaliger Vorgang, wenn man das Board zum ersten mal startet:
- Setup je nach Anwendung auswählen, "Sensor" oder "Gateway". Die Option ohne Bootloader wählen.
- den Programmer seiner Wahl auswählen
- COM Port des Programmers einstellen unter `Tools->Port`
- `Tools->Burn Bootloader` klicken.

Normales Flashen:
- Wenn man einen Arduino (z.B.UNO) als Programmer benutzt: Bei `Tools->Programmer` "Arduino as ISP" auswählen (nicht "ArduinoISP"!)
- Ansonsten den Programmer seiner Wahl auswählen
- COM Port einstellen unter `Tools->Port`
- In der Arduino IDE `sketch->Upload using Programmer` klicken. Jetzt kompiliert der Sketch, eventuell mit Warnungen (die man aber ignorieren kann) und lädt den binären Code auf das Board hoch.


## Nodes Konfigurieren
Nach dem Flashen sind die Nodes und die Gateways noch nicht betriebsbereit (leider, wird verbessert). Das EEPROM muss zuerst mit sinnvollen Daten gefüllt, "kalibriert" werden.
### EEPROMer Python tool
nach dem Start eines TiNos liest das TiNo Board das EEPROM. Da die Daten verschlüsselt sind, werden sie zunächst entschlüsselt und die Prüfsumme gebildet.
Wenn die Prüfsumme mit der aus dem EEPROM gelesenen übereinstimmt:
- über den seriellen Port wird der String "CAL?" gesendet. Wenn innerhalb von 250ms ein 'y' zurückkommt, geht der TiNo in den Kalibriermodus.
Kommt keine Antwort, sendet der TiNo, nur als Debugnachricht, ein "timeout". Also nicht wundern über das Timeout, das zeigt an dass alles in Ordnung ist. Man sieht es allerdings nur wenn man das TiNo Board direkt mit einem Terminalprogram (z.B. minicom oder TeraTerm) verbindet.

Wenn die Prüfsumme nicht übereinstimmt:
- Der TiNo geht direkt inden Kalibriermodus.

Da serielle Ports von Computer zu Computer verschieden sind gibt es eine Kommandozeilenoption für den Port. Die Baudrate für den Sender TiNo ist 4800 Baud. Für ein Gateway braucht man moeglichst hohe Baudraten; im Moment ist sie auf 38400 Baud festgelegt. Für beide Konfigurationen kann das selbe Eepromer Tool verwendet werden.

**Wichtige Hinweise:**
Das EEPROMer Tool ist geht mit Python 2.7 oder Python 3.x und wurde auf Linux (Debian) und Windows10 getetstet.
Das EEPROMer Tool tinocal der Version 1 ist in Python 2.7 geschrieben aber läuft unter Python 3 nicht.
Die Python Tools der Versionen 1, Version 2 und Version 3 sind untereinander nicht kompatibel! Firmware der Version 1 muss man mit dem Tool der Version 1 abgleichen, Firmware der Version 2 und später mit dem tool tinocal_v009.py, Firmware der Version 3 mit tinocal_v010.py

Wenn man das EEPROMer Tool startet, öffnet es zunächst den seriellen Port. An einem FTDI Adapter (mit herausgefuehrter DTR Leitung) bewirkt das, dass der angeschlossene TiNo neu startet. Wer keinen Adapter mit DTR Leitung zur Verfügung hat, muss zum Neustart des TiNo die DTR Leitung kurz auf Masse legen und wieder freigeben. Dann wartet das Tool auf das 'CAL?' vom TiNo, und sendet ggf. das 'y' sofort zurück um den Kalibriermodus zu erzwingen.
Sobald das Tool meldet dass man im Kalibriermodus ist, muss man das Passwort eingeben. Dies ist mit dem *KEY* Parameter im Source Code identisch. Derselbe KEY wird auch zum Verschlüsseln des HF Pakets benutzt. Das EEPROM ist verschlüsselt, weil sonst ein Dieb einen TiNo ohne weiteres komplett umkonfigurieren könnte und damit wild in der Gegend herumfunken kann (oder noch Schlimmeres anrichten kann), ohne dass er das Passwort kennen müsste.

Passwort eingeben:
- entweder das im Eepromer Tool hinterlegte Passwort verwenden: `pwd<Enter>` eingeben
- oder ein anderes Passwort verwenden (muss aber immer mit dem kompilierten Passwort identisch sein): `pw,<Passwort><Enter>` eingeben

Nachdem das Tool "Pass OK" meldet, kann man mit dem Konfigurieren beginnen.
Folgende Syntax wird von dem Tool verstanden:
`help` listet die Optionen auf:

| Kommando | Beschreibung  |
|:-------|:--------------|
|`exit` | terminate program
|`help`  or `?` | print this help text
|`c` | measure ADC and store in EEPROM.
|`copy` or `cp` | copy file content to EEPROM. syntax: cp, <filename>
|`cs`| verify checksum.
|`ft` | write temperature / frequency-correction table to EEPROM
|`g` or `get` | store eeprom content to file. Syntax: `g(et),<filename>`
|`ls`| Liste die  EEPROM Konfigurationsdaten.
|`m` | Measure VCC with calibrated values
|`pw`| Passwort. Syntax: `pw,<passwort>`
|`pwd`| Passwort des Auslieferungszustands (default)
|`quit` | beende das Programm
|`read`  or `r` | read from EEPROM. Syntax: `r(ead),<addr>`
|`ri`| read 16 bit integer from EEPROM. Syntax: `ri(ead),<addr>`
|`rf`| read float from EEPROM. Syntax: `ri(ead),<addr>`
|`s` | request checksum update and store in EEPROM.
|`t` | send a test RF packet
|`to`| start sending radio OOK signal
|`ts`| put radio into sleep mode
|`tt`| read temperature from RFM69 device
|`vddcal` | calibrate VCC measurement. Syntax: `v(ddcal),<VCC at device in mV>`
|`write` or `w` |  write value to to EEPROM.  Syntax: `w(rite),<addr>,<value>`
|`wf` | write float value to EEPROM. Syntax: `wf,<addr>,<value>`
|`wl` | write long int value to to EEPROM.  Syntax: `wl,<addr>,<value>`, value format can be hex
|`wu` | write unsigned int value to EEPROM. Syntax: `wu,<addr>,<value>`
|`x` | exit calibration mode and continue with loop()

Das Tool ist ursprünglich interaktiv, d.h. man greift "manuell" auf das EEPROM zu und kann es so konfigurieren. Allerdings ist das nur bis zu einem bestimmten Grad praktisch, z.B. wenn man nur mal schnell eine ID ändern will, oder wenn man nur die Integrität des EEPROMs feststellen will.
Wenn man mehr machen will/muss, wäre eine automatisierte Version praktisch. Ich arbeite daran.

Im Moment kann man die wichtigsten Aktionen durch die Kommandozeile auslösen.
Unterstützt werden zur Zeit folgende Optionen:

| Option | Beschreibung  |
|:-------|:--------------|
-pwd | Sende das im Tool hinterlegte Passwort
-cs|   lies die Prüfsumme vom EEPROM
-ls | Liste der EEPROM Werte
-cp,<Dateiname>| Kopieren des Inhalts einer Konfigurationsdatei zum TiNo
-s | berechnen und speichern der Prüfsumme
-x | EEPROM verschlüsseln und Daten abspeichern
-q | Tool beenden

eine Kommandozeile sieht dann beispielsweise so aus:

`python tinocal_v010.py COM8 38400 -pwd -cp,receive_eeprom.cfg -ls -cs -x -q`

In diesem Fall verbindet sich das Eepromer Tool mit dem TiNo Board auf COM8, 38400 Baud und arbeitet dann die Liste der Optionen in der Reihenfolge ab, also:
1. `-pwd` sendet das im Programm hinterlegte Passwort an da TiNo Board
2. `-cp,receive_eeprom.cfg` kopiert den Inhalt der Datei `receive_eeprom.cfg` vom PC auf das TiNo Board
2. `-ls` listet den Inhalt des EEPROMs.
3. `-cs` liest die Prüfsumme und prüft sie.
4. `-x` verlässt den Kalibriermodus
5. `-q` beendet das Tool

### EEPROM Speicher erklärt:
Diese Parameter sind derzeit im EEPROM gespeichert:

| Parameter |  Wert | Beschreibung |
|:----|:----|:----|
NODEID| 0-255 | die Identitfizierung des TiNo
NETWORKID | 0-255 | Identifizierung des Netzwerks, typisch 210 *)
GATEWAYID | 0-255 | Das Ziel (Gateway) zu dem Nachrichten gesendet werden
VCCatCAL | typ. 3300 mV | Wert der Versorgungsspannung in mV zum Zeitpunkt der Kalibrierung
VCCADC_CAL | typ. 350 | der ADC Wert der bei Anliegen von VCCatCAL kalibriert wurde
SENDDELAY | 0 - 65535 | Zeit in Sekunden/8 die zwischen zwei Messungen vergehen soll (Maximal 145 Stunden, ca 6 Tage). Bei einem Wert von 0 wird der RTC timer deaktiviert. **)
FREQBAND | 43, 86 | 43 für das 433MHz Band, 86 für das 868MHz Band
FREQ_CENTER | z.B. 865.000 | die genaue Mittenfrequenz des Senders (muss für das gesamte Netzwerk identisch sein)
TXPOWER | 0-31 | 31 = maximale Sendeleistung, 0 = minimale Sendeleistung in 1dB Schritten
REQUESTACK | 0 oder 1 | legt fest ob ein empfangenes Telegramm quittiert werden soll (1) oder nicht (0)
LEDCOUNT | 0 - 255 | legt fest ob ein gesendetes Telegramm von einem kurzen Blinken der LED begleitet wird und wie oft ***)
LEDPIN | 0, 8 | Pin an dem die LED hängt (beim TiNo Pin D8). Bei einem Wert von Null wird die LED gar nicht verwendet und ist frei für einen Interrupt verwendbar
RXPIN | 0 | ohne Bedeutung, es wird das Modul *Serial* verwendet.
TXPIN | 1 | ohne Bedeutung.
SDAPIN | 0 - 21 | normalerweise 18 (A4)  Pin der für den SDA des I2C Busses verwendet wird
SCLPIN= | 0 - 21 | normalerweise 19 (A5) Pin der für den SCL des I2C Busses verwendet wird
I2CPOWERPIN | 0- 21 | normalerweise 9 (D9). Pin der für die VCC der I2C Komponenten verwendet wird
PCI0PIN | 0 -21, 128 | normalerweise 3 - 9, Pin der für den Interrupt PCI0 benutzt wird
PCI0TRIGGER | 0b0000xxxx | Triggerart des Interrupts. Wird unten erklärt
PCI1PIN | 0 -21, 128 | ein nicht benutzter externer Interrupt wird mit dem Wert 128 angezeigt
PCI1TRIGGER |  0b0000xxxx|
PCI2PIN | 0 -21, 128 |
PCI2TRIGGER|  0b0000xxxx|
PCI3PIN| 0 -21, 128 |
PCI3TRIGGER|  0b0000xxxx|
USE_CRYSTAL_RTC | 0 oder 1  | Version 2: nicht editieren! wird vom Sketch eingetragen. Ab Version 3:  editierbar
ENCRYPTION_ENABLE | 0 oder 1 | legt fest ob das Telegramm verschlüsselt werden soll
FEC_ENABLE | 0 oder 1 | legt fest ob Forward Error Correction eingesetzt werden soll
INTERLEAVER_ENABLE | 0 oder 1 | legt fest ob ein Interleaver zum Einsatz kommt.
EEPROM_VERSION_NUMBER | auto | **nicht editieren!** wird vom Sketch eingetragen
SOFTWAREVERSION_NUMBER| auto | **nicht editieren!** wird vom Sketch eingetragen
TXGAUSS_SHAPING | 0,1,2,3 | normalerweise 0. Legt fest ob und mit welchem BT Gauss Shaping vorgenommen wird (fortgeschritten)
SERIAL_ENABLE | 0 oder 1 | normal 1. legt fest ob standardmässig der Serielle Port aktiviert sein soll
IS_RFM69HW | 0 oder 1 | 0 : es handelt sich um einen RFM69CW. 1: eshandelt sich um einen RFM69**H**CW
PABOOST | 0, 1, 2, 3| normalerweise 0. nur für RFM69HCW: legt die High-Power Parameter fest (fortgeschritten)
FDEV_STEPS | +/- | Frequenzkorrektur bei Raumtemperatur (einfache Kalibrierung des 32 MHz Quarzes des RFM)
CHECKSUM | auto | wird beim Konfigurieren berechnet und dann eingetragen (Option 's')

Neue Parameter ab Version 3:

| Parameter |  Wert | Beschreibung |
|:----|:----|:----|
SENSORCONFIG|0-63|Bitfeld welches festlegt welche Sensoren von der Hardware unterstützt werden
RADIO_T_OFFSET|-128 ... 127| Offset in Grad*10 um die genauigkeit des RFM Temperatur sensors zu kalibrieren
USE_RADIO_T_COMP| 0 oder 1 | Temperaturabhängige Frequenzkorrektur aktivieren
LDRPIN|14,15,16,17| analoger GPIO Pin wo der LDR angeschlossen ist
PIRPOWERPIN|0-21|GPIO wo der + Anschluss des PIR verbunden ist. Datenpin geht an PCI1PIN
PIRDEADTIME|0-255|Zeit in der der PIR nach dem Auslösen nicht reagiert.
ONEWIREDATAPIN|0-21| Pin wo der DS18B20 Data Pin angeschlossen ist.
PCIxGATEWAYID| 0-255 |Ziel Node wenn ein PCIx Interrupt ausgeloest wurde. x= 0 ... 3

*) 210 ist um mit dem RFM12B Modul kompatibel zu sein. Wert kann bei diesem Modul nicht geändert werden.

**) Wird SENDELAY=0 gesetzt, wird der Timer deaktiviert und  der Sleep Modus des Prozessors aktiviert. Dieser wacht jetzt nur noch bei externen Interrupts ("PCI") auf.

***) Da die LED eigentlich nicht gebraucht wird und nur zum Test einer einwandfreien Funktion dient, kann eingestellt werden ob die LED beim Versenden eines Telegramms aufleuchten soll. Eine Zahl > 0 stellt ein bei wie vielen Telegrammen nach dem Start die LED noch blinken soll. Normalerweise auf 1 gesetzt. Wird LEDCOUNT=255 gesetzt, blinkt die LED immer wenn ein Paket gesendet wird.

PCI Trigger Byte Bitbelegung:

PCIxTrigger bits 0 and 1:

| Bitbelegung | Bedeutung |
|---|---|
0b0000xx00 | LOW
0b0000xx01 | CHANGE
0b0000xx10 | FALLING (Normaleinstellung)
0b0000xx11 | RISING

PCIxTrigger bits 2 and 3:

| Bitbelegung | Bedeutung |
|---|---|
0b000000xx | INPUT
0b000001xx | OUTPUT
0b000010xx | INPUT_PULLUP  (Normaleinstellung)

Beispiel:
0b00001010 = 0x0A - 10 (Dec) = INPUT_PULLUP und FALLING
Dies ist die Normaleinstellung. Der interne Pullup ist Teil der Entprellschaltung mit einem externen Widerstand und einem Kondensator.

---> Bild mit Entprellungsschaltung <----
## Nachbau
Aufgrund der überschaubaren Stückliste und des einfachen Aufbaus ist der Nachbau wirklich kinderleicht, ein wenig Lötfertigkeit vorausgesetzt.
### Vorausetzungen: Was braucht man?
#### Hardware
- USB-Seriell Adapter (FTDI oder kompatibel, CH340 geht auch aber auf das Pinning achten!, und immer den Jumper oder den Schalter auf 3.3V einstellen!)
- ISP-Programmer
- Gateway: etwas das einen seriellen Port öffnen, lesen, schreiben, anzeigen und speichern kann (PC, Raspberry Pi, ESP8266,...)
- Lötkolben und Zubehör. Es sollte ein feiner Lötkolben für Elektronik sein, nicht gerade einer der mit Gas betrieben wird.

#### Software
- Python (am Raspberry Pi bereits vorinstalliert)
- Arduino IDE

### Leiterplatten
Die Leiterplatten bestelle ich gerne bei [seeedstudio](https://www.seeedstudio.com/fusion_pcb.html). Das dauert zwar 3 Wochen von der Bestellung bis zur Lieferung, dafür ist die Qualität aber sehr gut zum vernünftigen Preis. Die Layouts wurden mit [Autodesk Eagle](https://www.autodesk.com/products/eagle/overview) entworfen und sind [hier](https://github.com/nurazur/TiNo/tree/master/eagle) hinterlegt.
### Mechanik (Gehäuse)
ich stelle hier zwei verschiedene Leiterplattendesigns vor:
Die erste Leiterplatte verwendet die RFM69HCW bzw. RFM95 Pinbelegung und ist für das [Strapubox SP2043 Gehäuse](http://strapubox.de/modules/uploadmanager11/admin/index.php?action=file_download&file_id=163&location_id=0) konzipiert. Damit kann man im Prinzip auch einen LoRa Node verwirklichen, entsprechende Software gibt es passend im Netz.
Die zweite Leiterplatte ist mit der Pinbelegung des RFM69CW (kompatibel mit RFM12B) und für das [Strapubox MG307 Gehäuse](https://www.elv.de/strapubox-kunststoff-gehaeuse-mg-307-abs-45-x-30-x-22-mm-grau.html). Dies ist die kostensparendste Variante, da das Gehäuse weniger als 1 EUR kostet und auch der RFM69CW deutlich günstiger zu haben ist als ein RFM69HCW.
## Elektronik
### Schaltplan erklärt
Das Besondere am TiNo ist dass die Schaltung wirklich nicht kompliziert ist.

Das Herzstück ist der Prozessor mit dem HF Modul. Das HF Modul kommuniziert über den SPI Bus, das sind die GPIO's D10(SS) D11(MOSI), D12(MISO) und D13(SCK). Ausserdem benutzt der Treiber einen Interrupt an GPIO D2, der auslöst wenn Daten empfangen werden. Derselbe GPIO D2 wird auch benutzt um das Ende einer Sendesequenz zu signalisieren.
Die selben GPIO's werden vom ISP Adapter benutzt, denn die Programmierung des Prozessors erfolgt ebenso mit SPI Bus. Damit sich das HF Modul beim Programmieren über ISP nicht angesprochen fühlt, braucht es einen 10kOhm Pullup an der SS Leitung (D10). Im Sleep Mode beeinflusst dieser Widerstand den Ruhestrom nicht.

Optional kann der Prozessor im Sleep Modus mit einem externen Uhrenquarz (32.768 KHz) bestückt werden. Der Quarz benötigt noch zwei Lastkondensatoren von je 6pF oder 12pF, je nach Bauart des Quarzes.

Bei der Inbetriebnahme und zum Testen ist eine LED unglaublich hilfreich. Diese wird an GPIO D8 angeschlossen. je nach TiNo Boardausführung kann die LED in SMD und/oder bedrahteter Bauform eingesetzt werden.

Der Bequemlichkeit halber gibt es einen FTDI Adapter. Das Pinout des Adapters ist mit der Pinbelegung eines Arduino Pro Mini identisch, deshalb gibt es auch jede Menge USB-TTL Konverter im Netz mit genau diesem Pinout.

Der I2C Bus wird auf den GPIO Ports A4 (SCL) und A5(SCK) angeschlossen. I2C Bus Komponenten werden durch GPIO D9 versorgt, damit sie im Sleep Modus keinen Strom verbrauchen. Beide Leitungen des I2C Busses brauchen Pullup Widerstände. Wird anstelle des HTU21D/SHT2x Chips ein Modul verwendet, braucht man diese Widerstände nicht da sie auf dem Modul bereits montiert sind.


### Stückliste
Die Preise für Bauteile schwanken stark. Daher sind die angegebenen Preise nur als Anhaltspunkt zu verstehen

| Bauteil | Preis | Bemerkung |
|---|---|----|
ATMega328p-au | ca. 1.20 EUR | März 2022: ca 7.00 EUR
RFM69CW | ca. 1.50 EUR  |
HTU21D Sensor | ca. 1.30 EUR | IC's im DFN-6 Gehäuse kosten fast das selbe (ausser im 50er Pack)
Gehäuse | ca. 0.70  - 1.20 | je nach Typ
Batteriehalter | 0.05 |
Kleinteile |0.05 | Widerstände, Kondensatoren, LED in SMD Bauform *)

*)Ich habe darauf geachtet dass man nicht allzu viele verschiedene Werte benoetigt.

### Messergebnisse
