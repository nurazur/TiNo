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
- [Beschreibung des Projekts](https://github.com/nurazur/TiNo/blob/master/dokumentation.md#beschreibung-des-projekts)
- [Das Konzept](#das-konzept)
- Wie Funktionierts?
- IDE Einrichten
- Software Kompilieren und Flashen
- Nodes Konfigurieren
- Nachbau
- Elektronik
- Leiterplatten
- Mechanik (Gehäuse)

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
~~Minimale Kosten ergeben sich durch Minimierung der verwendeten Komponenten und durch optimale der Auswahl der Komponenten. Dies kann sich über die Zeit natürlich ändern.~~
Momentan ist die Auswahl des ATMega328 Processors mit dem RFM69CW HF-Modul die kostengünstigste Variante. Weiterhin kann auf LDO's oder DC-DC Konverter verzichtet werden, da alle Module direkt von einer 3V CR2032 Zelle, zwei AAA Batterien oder zwei AA Batterien betrieben werden können. 
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
Der Empfänger oder besser "Gateway" wird für ein Board mit 8 MHz Taktfrequenz kompiliert. Die "Fuses" müssen dementsprechend programmiert werden. 
### Sender 
### Weitere Bibliotheken Installieren  
Folgende Bibliotheken braucht man zusätzlich zur Installation des TiNo Boards:
- SoftwareWire        (für I2C Bus Sensoren)
- HTU21D_SoftwareWire (für den HTU21D Sensor Sketch)
- SHT3x_SoftwareWire  (für den SHT3x Sketch)
- PinChangeInterrupt  (Interrupts werden standardmässig unterstützt)
    
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
nach dem Start eines TiNos wird zunaechst das EEPROM gelesen. Da es verschluesselt ist, wird es zunaechst entschluesselt und die Pruefsumme gebildet. 

Wenn die Pruefsumme mit der aus dem EEPROM gelesenen uebereinstimmt:
Ueber den seriellen Port wird der String "CAL?" gesendet. Egal ob die Pruefsumme stimmt, wenn innerhalb von 250ms ein 'y' zurueckkommt, geht der Node in den Kalibriermodus. 
Kommt keine Antwort, sendet der Node, nur als Debugnachricht, ein "timeout". Also nicht wundern ueber das Timeout, das zeigt an dass alles in Ordnung ist. 

Wenn die Pruefsumme nicht uebereinstimmt:
Der Node geht direkt inden Kalibriermodus.

Das EEPROMer Tool ist in Python geschrieben, man braucht also unter Windows eine Installation von Python. WEnn man das EEPROMer Tool startet, oeffnet es zunaechst den seriellen Port. An einem FTDI Adapter bewirkt das, dass der angeschlossene Node neu startet, Dann wartet das Tool auf das 'CAL?' vom Node, und sendet ggf. das 'y' sofort zurueck um den Kalibriermodus zu erzwingen. 




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
- USB-Seriell Adapter
- ISP-Programmer
- Gateway: etwas das einen seriellen Port öffnen, lesen und schreiben kann (PC, Raspberry Pi,...)
    
## Elektronik 
### Schaltplan erklärt
### Messergebnisse
## Leiterplatten
## Mechanik (Gehäuse)


