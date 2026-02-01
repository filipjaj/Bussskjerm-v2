# BussTider Display

Vis sanntids bussavganger på en liten OLED-skjerm med ESP8266.

## Hardware

| Komponent | Eksempel |
|-----------|----------|
| ESP8266 | NodeMCU, Wemos D1 Mini, eller lignende |
| OLED Display | 128x64 SSD1306 (I2C) |
| Knapp (valgfri) | For manuell sidebytting / reset |

### Kobling

| OLED | ESP8266 |
|------|---------|
| VCC | 3.3V |
| GND | GND |
| SDA | D2 (GPIO4) |
| SCL | D1 (GPIO5) |

**Reset-knapp:** Koble mellom GPIO0 og GND (intern pull-up brukes).

## Arduino IDE Oppsett

### 1. Installer ESP8266 Board Support

1. Åpne **File → Preferences**
2. Legg til denne URL-en i "Additional Board Manager URLs":
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Gå til **Tools → Board → Boards Manager**
4. Søk etter "esp8266" og installer **esp8266 by ESP8266 Community**

### 2. Installer biblioteker

Gå til **Sketch → Include Library → Manage Libraries** og installer:

- `ArduinoJson` (by Benoit Blanchon)
- `Adafruit SSD1306` (by Adafruit)
- `Adafruit GFX Library` (by Adafruit)

### 3. Velg board

Under **Tools**:
- **Board:** NodeMCU 1.0 (eller ditt ESP8266-board)
- **Upload Speed:** 115200
- **Port:** Velg riktig COM-port

## Flash

1. Koble ESP8266 til PC via USB
2. Åpne `.ino`-filen i Arduino IDE
3. Klikk **Upload** (→)

## Bruk

### Første oppstart

1. Displayet starter i **Setup Mode** og oppretter et WiFi-nettverk: `BussTider-Setup`
2. Koble til nettverket med mobil/PC
3. Åpne `192.168.4.1` i nettleseren
4. Velg WiFi-nettverk, skriv inn passord
5. Legg inn API-endpoint (f.eks. `https://departures.filipjohn.workers.dev/d/...`)
6. Velg responsformat og lagre

### Responsformater

| Format | Eksempel |
|--------|----------|
| Simple | `{"31": 5, "74": 2}` |
| List | `[{"line": "31", "minutes": 5}]` |
| Verbose | `{"departures": [{"line": "31", "destination": "Tonsenhagen", "minutes": 5}]}` |

### Knappfunksjoner

- **Kort trykk:** Bytt side manuelt
- **Hold inne ved oppstart:** Reset konfigurasjon og gå til setup mode

## Feilsøking

| Problem | Løsning |
|---------|---------|
| OLED viser ingenting | Sjekk I2C-adresse (standard `0x3C`), sjekk kobling |
| WiFi kobler ikke til | Hold knappen inne ved oppstart for å resette |
| HTTP-feil | Sjekk at endpoint-URL er riktig og returnerer gyldig JSON |

## API

Displayet forventer et JSON-API som returnerer avgangstider. Intervall: hvert 20. sekund.

### Lag ditt eget endepunkt

Gå til [departures.filipjohn.workers.dev](https://departures.filipjohn.workers.dev/) for å opprette et personlig API-endepunkt for din holdeplass.
