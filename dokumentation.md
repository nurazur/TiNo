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
- maximal einfach nachzubauen
- Plug&Play Firmware

Als Sensor kann man so ziemlich alles verwenden, ob Temperatur, Luftfeuchtigkeit, Luftdruck, Höhenmesser, Lichtintensität, UV Index, 
Anwesenheitssensoren, Magnetschalter, Erschütterungs-Sensoren, Feuchtigkeitsmesser usw also im Prinzip alle Arten von Sensoren.

Die Leiterplatten passen in im Handel erhältlichen PVC Gehäusen, welche in etwa die Grösse einer Streichholzschachtel haben.


## Beschreibung des Projekts
Der TiNo Sensor macht periodisch eine Messung, z.B. der Umgebungstemperatur und sendet das Ergebnis mit einer kurzen HF Aktivität ("Puls") an eine Basisstation. Diese breitet das Signal auf und speichert das Ergebnis in einer Datenbank, die z.B. von einem Cloud Service ausgewertet werden kann. Der TiNo kann gleichzeitig auch Ueberwachungsaufgaben übernehmen, wie z.B das Oeffnen einer Türe melden. 

Die Idee zu dieser Entwicklung wurde bei mir durch ein Projekt im [deutschen Raspberry Pi Forum](https://forum-raspberrypi.de/forum/thread/7472-batteriebetriebene-funk-sensoren/) ausgeloest.
Ich moechte mich an dieser Stelle nocheinmal herzlich bei allen Beteiligten fuer die harte Arbeit die dahinter steckt bedanken. 
Da wurde mit viel Energie und Enthusiasmus ein System zum Senden und Empfangen von Sensordaten "zusammengebaut", was zur Entwicklung der "TinyTx4" und "TinyRX3" Platinen auf der Basis des ATtiny84a Prozessors gefuehrt hat. Ich habe lange Zeit mit diesen Platinen experimentiert, versucht sie mit Sensoren kombiniert in anschauliche (also mit hohem WAF) Boxen zu montieren und zu betreiben, kam aber schnell an die Grenzen dessen was ich mir eigentlich vorgestellt hatte. Mangelndes Talent für mechanische Dinge kommt hinzu. 

Für Ein Endprodukt eignen sich die TinyTx Platinen leider nicht, aber als Experimentier- und Lernplatform haben sie mir unschätzbare Dienste geleistet. Zunächst ging es mir um technische Verbesserungen: Optimierung des Link-Budgets, ein binäres, wesentlich kuerzeres Protokoll, Verbesserungen beim Stromverbrauch und beim Ruhestrom, ich experimentierte HF-seitig unter anderem mit FEC (Forward Error Correction), Interleavern, Baud Raten und Filterbandbreiten, und schliesslich war klar: ein anderer Prozessor muss her, der ATtiny mit seinen 8kB Flash kann nicht mithalten.

Ich begann von vorne. Mein Ansatzpunkt war aber nicht erst eine Platine zu entwerfen und mich dann um die Mechanik zu kuemmern, sondern umgekehrt. Und so habe ich die TiNo Platinen an das Gehäuse und ihren Zweck angepasst und nicht umgekehrt. 
Herausgekommen ist ein in allen Parametern optimierter Funksensor in der Groesse einer Streichholzschachtel, welchen ich Tino = **TI**ny **NO**de nenne. Spaeter kann man daraus dann das **TI**ny **N**etw**O**rk machen, denn auf der Softwareseite, speziell am Gateway gibt es noch unendlich viel Arbeit.
Auch das erklaerte Ziel der Einfachheit des Systems ist noch nicht erreicht. Das Einrichten der IDE, das Flashen und das Individualisieren der jeweiligen Nodes soll in Zukunft in einem einzigen Schritt erfolgen.
## Konzept
### Minimale Kosten:
Minimale Kosten ergeben sich durch Minimierung der verwendeten Komponenten und durch optimale der Auswahl der Komponenten. Dies kann sich über die Zeit natürlich ändern.
Momentan ist die Auswahl des ATMega328 Processors mit dem RFM69CW HF-Modul die kostengünstigste Variante. Weiterhin kann auf LDO's oder DC-DC Konverter verzichtet werden, da alle Module direkt von einer 3V CR2032 Zelle, zwei AAA Batterien oder zwei AA Batterien betrieben werden können. 
### minimale Grösse
Aufgrund der verwendeten Frequenzen kann man die Baugrösse eines Sensors nicht beliebig verkleinern ohne grosse Zugeständnisse an die Performance zu machen. 
Mit Platinenabmessungen von ca. 35 x 50 mm habe ich im 868MHz ISM Band gute Erfahrungen gemacht. Im 433MHz ISM Band funktioniert das auch durchaus zufriedenstellend. 
### minimaler Stromverbrauch
Konsequente Umsetzung der Möglichkeiten des ATMega328 Prozessors: Mit externem Uhrenquarz 32.678MHz arbeitet der Prozessor im Sleep Modus wie eine RTC (Real Time Clock), ich habe Ruheströme von 1.2µA gemessen. Ohne den Uhrenquarz verwendet man den internen RC Oszillator, da messe ich ca. 4µA, das ist aber immer noch ein sehr guter Wert.
### maximale Batterielebensdauer
Die Lebensdauer der Batterie setzt sich im Wesentlichen aus den vier folgenden Komponenten zusammen: 
1. der Batterie. Hier wird eine CR2032 Knopfzelle verwendet, mit geschätzten 170mAh Kapazität.
2. der Energie die bei einem Sendeimpuls verbraucht wird. Hier habe ich die Dauer des Impulses sowie seine Leistung optimiert. Sehr komplexes Thema.
3. dem Ruhestrom. 
4. der Anzahl der Sendepulse pro Zeit. bei einer Rate von einem Puls pro Minute überwiegt der Stromverbrauch der Sendepulse. Bei einer Rate von einem Puls pro Stunde überwiegt der Stromverbrauch durch den Ruhestrom.
 
    - maximale Reichweite
    - maximale Sicherheit
    - maximal einfach nachzubauen
    - Plug&Play Firmware
##Vorbereitung der Entwicklungsumgebung
 
##IDE Einrichten
    ###Empfänger (Gateway)
    Der Empfänger oder besser "Gateway" wird fuer ein Board mit 8 MHz Taktfrequenz kompiliert. Die "Fuses" muessen dem entsprechend programmiert werden. 
    ###Sender 
    ###Bibliotheken Installieren
    Die Bibliotheken koennen alle im Ordner "TiNo""stehen.
    Folgende Bibliotheken braucht man:
        calibration
        configuration
        Mac
        RFM69
        HTU21D_SoftwareWire
        SHT3x_SoftwareWire
    
    ausserdem braucht man
        SoftwareWire
        PinChangeInterrupt
            
    ###boards.txt Editieren
    Ein Board wird in Kuerze zur Verfuegung gestellt. 
    
##Kompilieren und Flashen
    ###mit ISP Adapter und Programmer
    mit FTDA und USB-Seriall Adapter
    
Konfigurieren
    EEPROM Speicher erklaert
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
    Tool EEPROMer Python tool

Vorausetzungen: Was braucht man?
    USB-Seriell Adapter
    ISP-Programmer
    Gateway: etwas das einen seriellen Port oeffnen, lesen und schreiben kann (PC, Raspberry Pi,...)
    
Hardware
    Elektronik - Schaltplan erklaert
    Leiterplatten
    Mechanik (Gehäuse)


