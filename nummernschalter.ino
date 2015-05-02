/**
 * Nummernschalter-Prüfprogramm.
 * Prüft einen angeschlossenen Nummernschalter und gibt das Ergebnis per serieller Schnittstelle aus.
 * 4 / 2015
 * https://github.com/retsifp/NummernschalterMessung
 */

// Zeiteinstellungen
#define lowerThresholdImpulse 55 // Untere Grenze für einen Impuls, in Millisekunden (Optimal: 62 ms)
#define upperThresholdImpulse 72 // Obere Grenze für einen Impuls, in Millisekunden
#define lowerThresholdPause   31 // Untere Grenze für eine Pause, in Millisekunden (Optimal: 38 ms)
#define upperThresholdPause   48 // Obere Grenze für eine Pause, in Millisekunden
#define debounce 2 // Debounce-Zeit, in Millisekunden (Könnte für schlechte Nummernschalter nach oben korrigiert werden müssen)

#define maxImpulses 12 // Maximale Impulse pro Wahl

// Pineinstellungen
#define led 13
#define nsa 2 // Pin des nsa-Kontaktes
#define nsi 3 // Pin des nsi-Kontaktes
#define baudr 115200 // Baud-Rate für Serielle Verbindung
#define inputPullups true

uint8_t nsiImpulses = 0;
uint8_t invalidImpulses = 0;
uint8_t invalidPauses = 0;
uint16_t minMaxImpulse[2] = {-1,0};
uint16_t minMaxPause[2] = {-1,0};
uint16_t impulseTimes[maxImpulses] = {0};
uint16_t pauseTimes[maxImpulses] = {0};

unsigned long nsiImpulseTime = -1;
unsigned long nsiPauseTime = -1;
unsigned long nsaTime = -1;

bool nsaOldState = !inputPullups; //NO
bool nsiOldState = inputPullups; //NC

void setup() {
	pinMode(led, OUTPUT);
#if inputPullups == false // Externe Pull-Down-Widerstände
	pinMode(nsa, INPUT);
	pinMode(nsi, INPUT);
	#define nsaRead() digitalRead(nsa)
	#define nsiRead() digitalRead(nsi)
#else // Interne Pullups. Bisher ungetestet.
	pinMode(nsa, INPUT_PULLUP);
	pinMode(nsi, INPUT_PULLUP);
	#define nsaRead() !digitalRead(nsa)
	#define nsiRead() !digitalRead(nsi)
#endif
	Serial.begin(baudr);
	delay(20);
	Serial.print(F("Nummernschalter-Pruefprogramm.\nnsi-Kontakt an Pin "));
	Serial.print(nsi);
	Serial.print(F(" anklemmen, nsa-Kontakt an Pin "));
	Serial.print(nsa);
	Serial.print(F(" anklemmen.\n"));
}

void loop() {
	if(nsaRead() != nsaOldState) {
		delay(debounce); //Debouncing
		if(nsaRead() != nsaOldState) {
			nsaOldState = !nsaOldState;
			nsaChange();
		}
	}
	if(nsiRead() != nsiOldState) {
		delay(debounce); //Debouncing
		if(nsiRead() != nsiOldState) {
			nsiOldState = !nsiOldState;
			nsiChange();
		}
	}
}

void nsaChange() {
	if(nsaRead()) {
		// Wählvorgang beginnt
		nsiImpulses = 0;
		invalidImpulses = 0;
		invalidPauses = 0;
		deleteArray(impulseTimes);
		deleteArray(pauseTimes);
		nsiImpulseTime = millis();
		nsiPauseTime = millis();
		nsaTime = millis();
		// Serielle Ausgabe
		Serial.print(F("\n\nnsa geschlossen, Waehlvorgang beginnt\n"));
		// Statistik zurücksetzen
		minMaxImpulse[0] = -1;
		minMaxImpulse[1] =  0;
		minMaxPause[0]   = -1;
		minMaxPause[1]   =  0;
	} else if((nsiImpulses + invalidImpulses) > 0) {
		// Wählvorgang beendet
		Serial.print(F("nsa geoeffnet, Waehlvorgang beendet\n"));
		Serial.print(F("Impulse (gesamt/gueltig/ungueltig): ("));
		Serial.print(nsiImpulses+invalidImpulses);
		Serial.print("/");
		Serial.print(nsiImpulses);
		Serial.print("/");
		Serial.print(invalidImpulses);
		Serial.print(F("), nsa Oeffnungszeit: "));
		Serial.print(millis() - nsaTime);
		Serial.print(" ms\n");
		if((nsiImpulses + invalidImpulses) > 1) {
			uint16_t iSumme, pSumme;
			float pAvg, iAvg;
			// Bei einem oder weniger Impulsen macht das keinen Sinn.
			Serial.print(F("Impulsdauer (Min/Max/Avg/StdDev): "));
			Serial.print(minMaxImpulse[0]);
			Serial.print("/");
			Serial.print(minMaxImpulse[1]);
			Serial.print("/");
			Serial.print(iAvg = calcAvg(impulseTimes, &iSumme));
			Serial.print("/");
			Serial.print(calcStdDev(impulseTimes, iAvg));
			Serial.print(F(", Pausendauer (Min/Max/Avg/StdDev): "));
			Serial.print(minMaxPause[0]);
			Serial.print("/");
			Serial.print(minMaxPause[1]);
			Serial.print("/");
			Serial.print(pAvg = calcAvg(pauseTimes, &pSumme));
			Serial.print("/");
			Serial.print(calcStdDev(pauseTimes, pAvg));
			Serial.print("\nBerechnete Gesamt-Ablaufzeit: ");
			Serial.print(iSumme + pSumme + pAvg); // Berechne Gesamt-Ablaufzeit, Methode 1
			Serial.print(" ms\n");
		}
		Serial.print("\n");
	}
}

void nsiChange() {
	unsigned long diff = -1;
	if(!nsiRead()) {
		// Impuls Start
		diff = millis() - nsiPauseTime; // Impulslänge berechnen; Debounce-Zeit wird nicht berücksichtigt, da bei steigender und fallender Flanke debounced wird
		if((nsiImpulses + invalidImpulses) != 0) {
			//MinMax Funktion
			if(diff < minMaxPause[0]) {
				minMaxPause[0] = diff;
			}
			if(diff > minMaxPause[1]) {
				minMaxPause[1] = diff;
			}
		}
		if(diff >= lowerThresholdPause && diff <= upperThresholdPause) {
			// Gültige Pause
			Serial.print(F("\tGueltige Pause. Laenge:\t\t"));
		} else if((nsiImpulses + invalidImpulses) != 0) {
			// Ungültige Pause
			invalidPauses++;
			Serial.print(F("\tUngueltige Pause. Laenge:\t"));
		}
		if((nsiImpulses + invalidImpulses) != 0) {
			pauseTimes[(nsiImpulses + invalidImpulses) - 1] = diff;
			Serial.print(diff);
			Serial.print(" ms\t");
			Serial.print("Periodendauer: ");
			Serial.print(pauseTimes[(nsiImpulses + invalidImpulses) - 1] + impulseTimes[(nsiImpulses + invalidImpulses) - 1]); // Gesamte Periodendauer
			Serial.print(" ms");
			Serial.print("\tPuls-Pausenverhaeltnis:  1 : ");
			Serial.println(1.0 *(pauseTimes[(nsiImpulses + invalidImpulses) - 1] + impulseTimes[(nsiImpulses + invalidImpulses) - 1]) / impulseTimes[(nsiImpulses + invalidImpulses) - 1]);
		}
		nsiImpulseTime = millis();
	} else {
		// Pause Start
		diff = millis() - nsiImpulseTime;
		if(diff < minMaxImpulse[0]) {
			minMaxImpulse[0] = diff;
		}
		if(diff > minMaxImpulse[1]) {
			minMaxImpulse[1] = diff;
		}
		if(diff >= lowerThresholdImpulse && diff <= upperThresholdImpulse) {
			// Gültiger Impuls
			nsiImpulses++;
			Serial.print(F("\tGueltiger Impuls. Laenge:\t"));
		} else {
			invalidImpulses++;
			Serial.print(F("\tUngueltiger Impuls. Laenge:\t"));
		}
		impulseTimes[nsiImpulses + invalidImpulses - 1] = diff;
		nsiPauseTime = millis();
		Serial.print(diff);
		Serial.print(" ms\n");
	}
}

void deleteArray(uint16_t array[]) {
	for(uint8_t i = 0;i < maxImpulses;i++) {
		array[i] = 0;
	}
}

float calcAvg(uint16_t array[], uint16_t *summe) {
	uint16_t sum = 0;
	uint8_t i = 0;
	for(;i < maxImpulses;i++) {
		if(array[i] == 0) {
			break;
		}
		sum += array[i];
	}
	*summe = sum;
	return 1.0 * sum / i;
}

double calcStdDev(uint16_t array[], float avg) {
	uint8_t i = 0;
	double stdDev = 0.0;
	for(;i < maxImpulses;i++) {
		if(array[i] == 0) {
			break;
		}
		stdDev += pow((1.0 * array[i] - avg), 2);
	}
	return 1.0 * sqrt((1.0/(i-1)) * stdDev);
}
