# TiNo
![](https://github.com/nurazur/TiNo/blob/master/matchbox.jpg)
"**TI**ny **NO**de" : Batteriebetriebener Funksensor oder Funk-Aktor. 
Ziel dieses Projekts ist die Entwicklung schnurloser Funk Sensoren, 
    die �ber Batterien versorgt werden und z.B. mit dem Raspberry Pi kommunizieren.
    Die Entwicklung hat zum Ziel:
 
- minimale Kosten (St�ckkosten unter 5 EUR)
- minimale Gr�sse (Streichholzschachtel)
- minimaler Stromverbrauch
- maximale Batterielebensdauer (5 Jahre oder mehr)
- maximale Reichweite 
- maximal einfach nachzubauen
- Plug&Play Firmware
   
Als Sensor kann man so ziemlich alles verwenden, ob Temperatur, Luftfeuchtigkeit, Luftdruck, H�henmesser, Lichtintensit�t, UV Index, 
Anwesenheitssensoren, Magnetschalter, Ersch�tterungs-Sensoren, Feuchtigkeitsmesser usw also im Prinzip alle Arten von Sensoren. Voraussetzung ist dass der Sensor bis mindestens 2.2V Betriebsspannung spezifiziert ist. Sonst kann die Batterieladung nicht voll ausgenutzt werden. 

Die Leiterplatten passen zu im Handel erh�ltlichen PVC Geh�usen, welche in etwa die Gr�sse einer Streichholzschachtel haben. Die verwendeten Komponenten sind am Markt eingef�hrt, jederzeit erh�ltlich und
dadurch kosteng�nstig zu beschaffen. 

# Features
## Allgemein
- Spannung von ca. 1.8V bis 3.6V
- Betrieb mit CR2032 Zelle bis zu 5 Jahren Lebensdauer
- verschiedene Leiterplatten passend zum im Handel er�ltlichen Geh�usen 
    
    
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
- HF Kommunikation verschl�sselt
- FEC (Forward Error Correction)
- Interleaver
    
## Basisband
- Atmel (Microchip) ATMega328p-au
- 32kByte Flash
- Ruhestrom < 2uA mit externem Quarz
- Ruhestrom ca. 4uA mit internem RC Oszillator
- 1 MHz Takt Sender erlaubt Betriebsspannung bis 1.8V
- 8 MHz Takt Empf�nger
- I2C f�r Sensoren
- mindestens 4 weitere GPIO
    
## Sensoren
- HTU21D
- SHT21, SHT20, SHT25
- SHT30, SHT31, SHT35
- I2C Bus Basierte Sensoren leicht konfigurierbar
- 4 digitale GPIOs
    
## System / Software
- Open Source Software C++
- Software kann einfach individuell angepasst werden
- Programmierung mit Arduino IDE
- Konfiguration der Nodes �ber serielles Interface (FTDA Adapter)
- Konfigurations- und Kalibrierdaten im EEPROM gespeichert.
- EEPROM verschl�sselt
- Flashen
  - mit ISP Adapter oder 
  - seriell mit FTDA Adapter �ber Bootloader
- bis zu 4 externe Interrupts (z.B. 4 Tasten) konfigurierbar
