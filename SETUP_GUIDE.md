# BussTider — Oppsettguide

Gratulerer med din nye sanntids-avgangstavle! Denne guiden hjelper deg med å koble til WiFi og komme i gang.

---

## Du trenger

- **USB-C kabel og lader** (5V, minst 500mA)
- **Telefon eller PC** med WiFi
- **WiFi-nettverk** (2.4 GHz — 5 GHz støttes ikke)
- **Tilgang til [avganger.filipjohn.com](https://avganger.filipjohn.com/)** for å opprette endepunkt

---

## Før du starter

Gå til **[avganger.filipjohn.com](https://avganger.filipjohn.com/)** på telefonen eller PC-en og opprett et endepunkt for ditt stoppested. Kopier URL-en du får — du trenger den i steg 5.

---

## Oppsett

### 1. Koble til strøm
Koble USB-C-kabelen til displayet og en lader. Skjermen viser "Starter..." og går deretter til Setup Mode.

### 2. Koble til WiFi-nettverket
På telefonen eller PC-en, gå til WiFi-innstillinger og koble til nettverket **BussTider-Setup**. Det krever ikke passord.

### 3. Åpne oppsett-siden
En nettside skal åpne seg automatisk (captive portal). Hvis ikke, åpne en nettleser og gå til **192.168.4.1**

### 4. Velg WiFi-nettverk
Trykk på ditt hjemmenettverk i listen. Skriv inn WiFi-passordet.

### 5. Legg inn API-endepunkt
Lim inn URL-en du har fått tilsendt i "API Endpoint"-feltet.

### 6. Lagre
Trykk **"Lagre og koble til"**. Displayet starter på nytt og kobler seg til WiFi. Etter noen sekunder vises avgangstider.

---

## Lese displayet

| Element | Beskrivelse |
|---------|-------------|
| **Linjenummer** | Vises i en invertert boks til venstre (f.eks. `54`) |
| **Destinasjon** | Navnet på endestasjonen (f.eks. `Kjelsaas`) |
| **Minutter** | Tid til avgang (f.eks. `7m`). `naa` = bussen er på holdeplassen |
| **Sider** | Flere enn 5 avganger? Displayet bytter side automatisk hvert 5. sekund. Trykk på knappen for å bla manuelt. |

---

## Feilsøking

| Problem | Løsning |
|---------|---------|
| Skjermen er svart | Sjekk USB-kabel og at laderen gir minst 500mA |
| "WiFi feilet!" | Feil passord eller nettverk utenfor rekkevidde. Går automatisk til Setup Mode — prøv igjen |
| "HTTP feil" | API-endepunktet svarer ikke. Sjekk URL-en. Reset og konfigurer på nytt |
| Finner ikke "BussTider-Setup" | Koble fra/til strøm. Allerede konfigurert? Hold inne Flash-knappen under oppstart for å resette |

---

## Tilbakestille

For å slette all konfigurasjon og starte på nytt:

1. Trykk på **Reset**-knappen
2. Mens enheten restarter, **hold inne Flash-knappen**
3. Skjermen viser "Reset config..." og displayet går tilbake til Setup Mode

---

*Data fra [Entur](https://entur.no)*
