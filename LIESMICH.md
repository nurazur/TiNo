# TiNo
![](https://github.com/nurazur/TiNo/blob/master/TiNo_Logo4.png)![](https://github.com/nurazur/TiNo/blob/master/matchbox.jpg)

## Hinweis aus aktuellem Anlass
Der Master-Branch ist als Entwicklungsbranch gedacht, die Verwendung, auch einzelner Dateien, ist auf eigenes Risiko. Die Releases sind jedoch gründlich getetstet. Doch auch hier gilt: ich übernehme keine Garantie dass die Software frei von Fehlern ist.

# Einführung
"**TI**ny **NO**de" : Batteriebetriebener Funksensor oder Funk-Aktor.
Ziel dieses Projekts ist die Entwicklung schnurloser Funk Sensoren,
    die über Batterien versorgt werden und z.B. mit dem Raspberry Pi kommunizieren.
    Die Entwicklung hat zum Ziel:

- minimale Kosten (Stückkosten unter 5 EUR, Stand 2020)
- minimale Grösse (Streichholzschachtel)
- minimaler Stromverbrauch
- maximale Batterielebensdauer (5 Jahre oder mehr)
- maximale Reichweite
- maximal einfach nachzubauen
- Plug&Play Firmware

Als Sensor kann man so ziemlich alles verwenden, ob Temperatur, Luftfeuchtigkeit, Luftdruck, Höhenmesser, Lichtintensität, UV Index,
Anwesenheitssensoren, Magnetschalter, Erschütterungs-Sensoren, Feuchtigkeitsmesser usw also im Prinzip alle Arten von Sensoren. Voraussetzung ist dass der Sensor bis mindestens 2.2V Betriebsspannung spezifiziert ist. Sonst kann die Batterieladung nicht voll ausgenutzt werden.

Die Leiterplatten passen zu im Handel erhältlichen PVC Gehäusen, welche in etwa die Grösse einer Streichholzschachtel haben. Die verwendeten Komponenten sind am Markt eingeführt, jederzeit erhältlich und
dadurch kostengünstig zu beschaffen.

# Warum TiNo?
- kompakte Bauform (Streichholzschachtel).
- Leiterplatten speziell fuer das Gehaeuse.
- Schaltung konsequent auf mimimalem Stromverbrauch optimiert. Batterielaufzeit 5 Jahre oder mehr.
- Konzept der minimalen Kosten. Keine teuren Features die kaum einer braucht.
- Bidirektionale sichere Funkverbindung.

# Features
## Allgemein
- Spannung von ca. 1.8V bis 3.6V
- Betrieb mit CR2032 Zelle bis zu 5 Jahren Lebensdauer
- verschiedene Leiterplatten passend zum im Handel erhältlichen Gehäusen

## Sensoren
- HTU21D
- SHT21, SHT20, SHT25
- SHT30, SHT31, SHT35
- BME280 Luftdruck Sensor
- AM312 PIR Bewegungssensor
- MAX31865 PT100 Temperatur Sensor
- LDR als helligkeitssensor
- I2C Bus Basierte Sensoren leicht konfigurierbar
- 4 digitale GPIOs
- 2/4 analoge GPIO's (abhängig von der Board Version)

## Radio
- RFM69CW, RFM69HCW, RFM95 Module
- bidirektionale Kommunikation
- ISM Band (Europa: 433MHz oder 868MHz, US:315MHz oder 915Mhz)
- 2GFSK Modulation
- Frequenz abstimmbar
- Frequenzkorrektur kalibrierbar
- Sendeleistung -18 dBm (typ.) bis 20dBm (max)
- Link Budget bis 120dB
- Empfindlichkeit -105 dBm typ.
- Reichweite t.b.d., ist aber sehr weit!
- HF Kommunikation verschlüsselt
- FEC (Forward Error Correction)
- Interleaver

## Basisband
- Atmel (Microchip) ATMega328p-au (32k)
- alternativ ATMega644 oder ATMega1284 (mehr Flasch Speicher 64k oder 128k)
- 32 / 64 / 128 kByte Flash
- Ruhestrom < 2uA mit externem Quarz
- Ruhestrom ca. 1uA mit TPL5110 (extern)
- Ruhestrom ca. 4uA mit internem RC Oszillator
- 1 MHz Takt Sender erlaubt Betriebsspannung bis 1.8V
- 8 MHz Takt Empfänger
- I2C für Sensoren
- mindestens 4 weitere GPIO


## System / Software
- Open Source Software C++
- Software kann einfach individuell angepasst werden
- Programmierung mit Arduino IDE
- Konfiguration der Nodes über serielles Interface (FTDI Adapter)
- Konfigurations- und Kalibrierdaten im EEPROM gespeichert.
- EEPROM verschlüsselt
- Flashen
  - mit ISP Adapter oder
  - seriell mit FTDI Adapter über Bootloader
- bis zu 4 externe Interrupts (z.B. 4 Tasten) konfigurierbar
- ab Version 2: [Interface zu IOBroker](https://github.com/bowao/ioBroker.tino/)

## Dokumentation in Deutsch
[deutsche Dkumentation](https://github.com/nurazur/TiNo/blob/master/dokumentation.md)
