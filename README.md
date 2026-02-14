#BussTider ESP8266

Sanntids-avgangstavle for kollektivtransport på en OLED-skjerm (SSD1306 128x64), drevet av en ESP8266 (NodeMCU v2).

## Hardware

- ESP8266 NodeMCU v2
- SSD1306 OLED 128x64 (I2C)
  - SDA → GPIO14 (D5)
  - SCL → GPIO12 (D6)
- Flash-knapp → GPIO0 (D3)

## Oppsett (utvikling)

Prosjektet bruker [PlatformIO](https://platformio.org/).

### Installer PlatformIO

```bash
pip3 install platformio
```

### Kompiler

```bash
pio run
```

### Flash til enhet

Koble til ESP8266 via USB og kjør:

```bash
pio run -t upload
```

Eller bruk scriptet:

```bash
./flash.sh
```

### Seriell monitor

```bash
pio device monitor
```

## Prosjektstruktur

```
busstider_esp8266_v4/
├── src/
│   └── busstider_esp8266_v4.ino   # Hovedkode
├── platformio.ini                  # PlatformIO-konfigurasjon
├── flash.sh                        # Hurtig-flash script
├── SETUP_GUIDE.md                  # Brukerveiledning
├── BussTider_Oppsettguide.pdf      # Brukerveiledning (PDF)
└── README.md
```

## Hvordan det fungerer

1. Ved første oppstart (eller etter reset) starter enheten i **Setup Mode** — en WiFi access point (`BussTider-Setup`) med captive portal
2. Brukeren kobler til, velger WiFi-nettverk og legger inn API-endepunkt fra [avganger.filipjohn.com](https://avganger.filipjohn.com/)
3. Enheten kobler til WiFi og henter avganger i verbose-format hvert 20. sekund
4. Displayet viser linjenummer, destinasjon og minutter til avgang
5. Paginering skjer automatisk hvert 5. sekund, eller manuelt via Flash-knappen

## Reset

1. Trykk **Reset**
2. Hold inne **Flash** mens enheten restarter
3. Konfigurasjon slettes og Setup Mode starter

## API-format

Endepunktet må returnere verbose-format:

```json
{
  "departures": [
    { "line": "54", "destination": "Kjelsaas", "minutes": 7 },
    { "line": "54", "destination": "Kjelsaas", "minutes": 22 }
  ]
}
```

## Data

Avgangstider fra [Entur](https://entur.no).
