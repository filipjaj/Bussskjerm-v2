#!/bin/bash
cd "$(dirname "$0")"
echo "🔌 Kobler til ESP8266..."
echo "   Sørg for at enheten er koblet til via USB."
echo ""
pio run -t upload
