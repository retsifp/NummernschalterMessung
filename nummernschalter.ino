/**
 * Nummernschalter-Prüfprogramm.
 * Prüft einen angeschlossenen Nummernschalter und gibt das Ergebnis per serieller Schnittstelle aus.
 * 5 / 2015
 * https://github.com/retsifp/NummernschalterMessung
 */

// Zeiteinstellungen
#define perfectImpulse 62.0 // Perfekte Impulsdauer, in Millisekunden
#define perfectPause 38.0 // Perfekte Pausendauer
#define offsetGood 7.0 // Perfekte Abweichung, in Prozent
#define offsetAcceptable 10.0 // Akzeptable Abweichung, in Prozent
#define deviationGood 1.5 // Standardabweichung für perfekten Nummernschalter, absolut
#define deviationAcceptable 3.5 // Akzeptable Standardabweichung, absolut
#define debounce 2 // Debounce-Zeit, in Millisekunden (Könnte für schlechte Nummernschalter nach oben korrigiert werden müssen)

#define maxImpulses 12 // Maximale Impulse pro Wahl

// Pineinstellungen
#define nsa 2 // Pin des nsa-Kontaktes
#define nsi 3 // Pin des nsi-Kontaktes
#define baudr 115200 // Baud-Rate für Serielle Verbindung
#define inputPullups true

uint8_t nsiImpulses = 0;
uint8_t invalidPauses = 0;
float minMaxImpulse[2] = {1000,0};
float minMaxPause[2] = {1000,0};
float impulseTimes[maxImpulses] = {0};
float pauseTimes[maxImpulses] = {0};

unsigned long nsiImpulseTime = -1;
unsigned long nsiPauseTime = -1;
unsigned long nsaTime = -1;

bool nsaOldState = !inputPullups; //NO
bool nsiOldState = inputPullups; //NC

void setup() {
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
		invalidPauses = 0;
		deleteArray(impulseTimes);
		deleteArray(pauseTimes);
		nsiImpulseTime = micros();
		nsiPauseTime = micros();
		nsaTime = micros();
		// Serielle Ausgabe
		Serial.print(F("\n\nnsa geschlossen, Waehlvorgang beginnt\n"));
		// Statistik zurücksetzen
		minMaxImpulse[0] = 1000;
		minMaxImpulse[1] =  0;
		minMaxPause[0]   = 1000;
		minMaxPause[1]   =  0;
	} else if(nsiImpulses > 0) {
		// Wählvorgang beendet
		Serial.print(F("nsa geoeffnet, Waehlvorgang beendet\n"));
		Serial.print(F("Impulse: "));
		Serial.print(nsiImpulses);
		Serial.print(F(", nsa Oeffnungszeit: "));
		Serial.print((micros() - nsaTime) / 1000);
		Serial.print(" ms\n");
		if(nsiImpulses > 1) {
			double iSumme, pSumme;
			float pAvg, iAvg, iStdDev, pStdDev, iVar, pVar;
			// Bei einem oder weniger Impulsen macht das keinen Sinn.
			Serial.print(F("Impulsdauer (Min/Max/Avg/StdDev): "));
			Serial.print(minMaxImpulse[0]);
			Serial.print("/");
			Serial.print(minMaxImpulse[1]);
			Serial.print("/");
			Serial.print(iAvg = calcAvg(impulseTimes, &iSumme));
			Serial.print("/");
			Serial.print(iStdDev = calcStdDev(impulseTimes, iAvg));
			Serial.print(F(", Pausendauer (Min/Max/Avg/StdDev): "));
			Serial.print(minMaxPause[0]);
			Serial.print("/");
			Serial.print(minMaxPause[1]);
			Serial.print("/");
			Serial.print(pAvg = calcAvg(pauseTimes, &pSumme));
			Serial.print("/");
			Serial.print(pStdDev = calcStdDev(pauseTimes, pAvg));
			Serial.print("\nBerechnete Gesamt-Ablaufzeit: ");
			Serial.print(iSumme + pSumme + pAvg); // Berechne Gesamt-Ablaufzeit, Methode 1
			Serial.print(" ms\n");
			Serial.print("\nBewertung des Nummernschalters:\n");
			Serial.print(F("Der Mittelwert der Impulsdauer ist "));
			if((iVar = (1.0 * iAvg / perfectImpulse - 1.0) * 100) <= offsetGood) {
				Serial.print(F("sehr gut"));
			} else if(iVar <= offsetAcceptable) {
				Serial.print(F("akzeptabel"));
			} else {
				Serial.print(F("schlecht"));
			}
			Serial.print(F(". Abweichung zum Optimalwert: "));
			Serial.print(iVar);
			Serial.print(" %\n");
			Serial.print(F("Der Mittelwert der Pausendauer ist "));
			if((pVar = (1.0 * pAvg / perfectPause - 1.0) * 100) <= offsetGood) {
				Serial.print(F("sehr gut"));
			} else if(pVar <= offsetAcceptable) {
				Serial.print(F("akzeptabel"));
			} else {
				Serial.print(F("schlecht"));
			}
			Serial.print(F(". Abweichung zum Optimalwert: "));
			Serial.print(pVar);
			Serial.print(" %\n");
			Serial.print(F("Die Standardabweichung der Impulsdauer ist "));
			if(iStdDev < deviationGood) {
				Serial.print(F("sehr gut ("));
			} else if(iStdDev < deviationAcceptable) {
				Serial.print(F("akzeptabel ("));
			} else {
				Serial.print(F("schlecht ("));
			}
			Serial.print(iStdDev);
			if(isnan(pStdDev) == 0) { // Bei weniger als zwei Pausen kann keine Standardabweichung berechnet werden
				Serial.print(F(")\nDie Standardabweichung der Pausendauer ist "));
				if(pStdDev < deviationGood) {
					Serial.print(F("sehr gut ("));
				} else if(pStdDev < deviationAcceptable) {
					Serial.print(F("akzeptabel ("));
				} else {
					Serial.print(F("schlecht ("));
				}
				Serial.print(pStdDev);
			}
			Serial.print(")");
		}
		Serial.print("\n");
	}
}

void nsiChange() {
	float diff;
	if(!nsiRead()) {
		// Impuls Start
		nsiImpulseTime = micros();
		diff = (micros() - nsiPauseTime) / 1000.0; // Impulslänge berechnen; Debounce-Zeit wird nicht berücksichtigt, da bei steigender und fallender Flanke debounced wird
		if(nsiImpulses != 0) {
			//MinMax Funktion
			if(diff < minMaxPause[0]) {
				minMaxPause[0] = diff;
			}
			if(diff > minMaxPause[1]) {
				minMaxPause[1] = diff;
			}
		}
		if(nsiImpulses != 0) {
			Serial.print(F("\tPause, Laenge:\t"));
			pauseTimes[nsiImpulses - 1] = diff;
			Serial.print(diff);
			Serial.print(" ms\t");
			Serial.print("Periodendauer: ");
			Serial.print(pauseTimes[nsiImpulses - 1] + impulseTimes[nsiImpulses - 1]); // Gesamte Periodendauer
			Serial.print(" ms");
			Serial.print("\t\tPuls-Pausenverhaeltnis:  1 : ");
			Serial.println(1.0 *(pauseTimes[nsiImpulses - 1] + impulseTimes[nsiImpulses - 1]) / impulseTimes[nsiImpulses - 1]);
		}
	} else {
		// Pause Start
		nsiPauseTime = micros();
		diff = (micros() - nsiImpulseTime) / 1000.0;
		if(diff < minMaxImpulse[0]) {
			minMaxImpulse[0] = diff;
		}
		if(diff > minMaxImpulse[1]) {
			minMaxImpulse[1] = diff;
		}
		Serial.print(F("\tImpuls, Laenge:\t"));
		nsiImpulses++;
		impulseTimes[nsiImpulses - 1] = diff;
		Serial.print(diff);
		Serial.print(" ms\n");
	}
}

void deleteArray(float array[]) {
	for(uint8_t i = 0;i < maxImpulses;i++) {
		array[i] = 0;
	}
}

float calcAvg(float array[], double *summe) {
	double sum = 0;
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

double calcStdDev(float array[], float avg) {
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
