# Nummernschalter-Zeitmessung mit Arduino


Misst Impuls- und Pausendauer eines angeschlossenen Nummernschalters und gibt die gemessenen Zeiten über USB aus.
Keine externe Beschaltung benötigt bei ```#define inputPullups true``` (Standard).

### Verbinden des Nummernschalters ###
- nsi-Kontakt mit Pin 3 und GND verbinden
- nsa-Kontakt mit Pin 2 und GND verbinden
- Hochladen des Programmes mit der Arduino IDE
- Öffnen des seriellen Monitors
- Nummernschalter aufziehen
- Ergebnis erscheint im seriellen Monitor
