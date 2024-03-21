//////////////////////////////////////////////////////

// Appel des bibliothèques

//////////////////////////////////////////////////////

// Bibliothèque interne d'arduino pour la gestion du temps
#include <Arduino.h>
// Bibliothèque externe pour la gestion du bouton
#include "simpleBouton.h"
// Bibliothèque externe pour la gestion de l'afficheur
#include <TM1637Display.h>
// Fichier externe pour la définition des notes de musique
#include "Melodie.h"

//////////////////////////////////////////////////////

// Définition des pins

//////////////////////////////////////////////////////

// Le buzzer
#define buzzer 2

// L'afficheur
#define CLK 3
#define DIO 4
TM1637Display display(CLK, DIO);

// Les moteurs/pont en H
#define enB 5
#define in4 6
#define in3 7
#define in2 8
#define in1 9
#define enA 10

// Le bouton
#define INTERRUPT_PIN 11
#define LED_PIN 12

// On associe notre bouton à la fonction de la bibliothèque
boutonAction bouton(INTERRUPT_PIN);


//////////////////////////////////////////////////////

// Définition des variables et des constantes

//////////////////////////////////////////////////////

bool heure_reglee = false;
bool alarme_reglee = false;
bool musique = false;
bool etat_clignotement = true;
bool alarmeJouee = false;

unsigned long clic = 0, double_clic = 0, triple_clic = 0;
unsigned long HEURE = 0, HEURE_ALARME = 9999;
unsigned long nbHeures, nbMinutes, nbSecondes;

unsigned long previousMillis = 0;
const unsigned long rafraichissement = 1000, clignotement = 250, clignotement_alarme = 250;

int calculHeure(int i = 0);


//////////////////////////////////////////////////////

// Définition des configurations d'affichages

//////////////////////////////////////////////////////

const uint8_t PLAY[] = {B01110011, B00111000, B01011111, B01101110};

const uint8_t OFF[] = {0, 0, 0, 0};

const uint8_t Min[] = {
  SEG_C | SEG_E | SEG_G,   // m
  SEG_C | SEG_E | SEG_G,   //
  SEG_E ,   // I
  SEG_C | SEG_E | SEG_G,   // n
};

const uint8_t Hour[] = {
  SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,   // H
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // U
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,   // R
};

const uint8_t Heyy[] = {
  SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,   // H
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,   // e
  SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,   // y
  SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,   // y
};

int melody_harry[] = {

  REST, 2, NOTE_D4, 4,
  NOTE_G4, -4, NOTE_AS4, 8, NOTE_A4, 4,
  NOTE_G4, 2, NOTE_D5, 4,
  NOTE_C5, -2,
  NOTE_A4, -2,
  NOTE_G4, -4, NOTE_AS4, 8, NOTE_A4, 4,
  NOTE_F4, 2, NOTE_GS4, 4,
  NOTE_D4, -1,
  NOTE_D4, 4,

  NOTE_G4, -4, NOTE_AS4, 8, NOTE_A4, 4, //10
  NOTE_G4, 2, NOTE_D5, 4,
  NOTE_F5, 2, NOTE_E5, 4,
  NOTE_DS5, 2, NOTE_B4, 4,
  NOTE_DS5, -4, NOTE_D5, 8, NOTE_CS5, 4,
  NOTE_CS4, 2, NOTE_B4, 4,
  NOTE_G4, -1,
  NOTE_AS4, 4,

  NOTE_D5, 2, NOTE_AS4, 4,//18
  NOTE_D5, 2, NOTE_AS4, 4,
  NOTE_DS5, 2, NOTE_D5, 4,
  NOTE_CS5, 2, NOTE_A4, 4,
  NOTE_AS4, -4, NOTE_D5, 8, NOTE_CS5, 4,
  NOTE_CS4, 2, NOTE_D4, 4,
  NOTE_D5, -1,
  REST, 4, NOTE_AS4, 4,

  NOTE_D5, 2, NOTE_AS4, 4,//26
  NOTE_D5, 2, NOTE_AS4, 4,
  NOTE_F5, 2, NOTE_E5, 4,
  NOTE_DS5, 2, NOTE_B4, 4,
  NOTE_DS5, -4, NOTE_D5, 8, NOTE_CS5, 4,
  NOTE_CS4, 2, NOTE_AS4, 4,
  NOTE_G4, -1,

};    


void setup() {
  // Assure la communication entre le moniteur série du PC et la carte Arduino
  Serial.begin(9600);

  // Assignation des pinModes

  pinMode(buzzer, OUTPUT);

  pinMode(INTERRUPT_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  pinMode(enA, OUTPUT); pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);

  // Contraste de l'afficheur

  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 };

  display.setBrightness(0x0f);

  // On affiche Play

  display.setSegments(PLAY);

  delay(2000);

  display.clear();

  // Association des clics grâce à la bibliothèque

  bouton.attacher(simpleClic, doubleClic, tripleClic);

  randomSeed(analogRead(0));   // Lecture de la valeur aléatoire. La broche analogique 0 n'étant pas connectée, le bruit analogique aléatoire
  // provoque l'appel de l'instruction randomSeed() pour générer
  // différent nombre de départ à chaque exécution du programme.
  // randomSeed() brouillera alors la fonction aléatoire.
}

void loop() {

  // Actualisation de l'état du bouton et donc de la valeur des clics
  bouton.actualiser();

  // La fonction millis renvoie le temps passé depuis la mise sous tension de la carte en ms
  unsigned long temps_ms = millis();
  unsigned long temps1, temps2, tempsPasser;

  // Clignotement de l'heure chaque seconde
  if (temps_ms - previousMillis >= rafraichissement) {
    previousMillis = temps_ms;
    HEURE = calculHeure();
    display.showNumberDecEx(HEURE, 0b11100000, true, 4, 0);
  }

  // Réglage de l'heure au triple clic
  if (triple_clic) {
    reglageHeure();
  }

  // Réglage de l'alarme au double clic
  if (double_clic) {
    temps1 = millis();
    reglageAlarme();
    temps2 = millis();
    // Calcul du temps passé pendant le réglage de l'alarme
    tempsPasser = temps2 - temps1;
    ajoutTemps(tempsPasser);
    triple_clic = 0;
  }

  if (!alarmeJouee) {

    if (HEURE == HEURE_ALARME) {
      triple_clic = 0;
      temps1 = millis();
      // Appel de la fonction de l'alarme
      fctAlarme();
      temps2 = millis();
      // Calcul du temps passé pendant le réveil
      tempsPasser = temps2 - temps1;
      ajoutTemps(tempsPasser);
      alarmeJouee = true;
      triple_clic = 0;
    }
  }

  else if (HEURE != HEURE_ALARME)
    alarmeJouee = false;

}

// Incrémentation des variables clic grâce aux fonctions
void simpleClic() {
  clic++;
}

void doubleClic() {
  double_clic++;
}

void tripleClic() {
  triple_clic++;
}

// Fonction qui calcule l'heure pour convertir les s/min en modulo 60 et les h en modulo 24
int calculHeure(int i) {

  int heureTotal;
  if (i == 0)
    nbSecondes++;

  if (nbSecondes > 59) {
    nbSecondes = 0;
    nbMinutes++;
  }

  if (nbMinutes > 59) {
    nbMinutes = 0;
    nbHeures++;
  }
  if (nbHeures > 23)
    nbHeures = 0;

  heureTotal = nbHeures * 100 + nbMinutes;
  return heureTotal;
}


// Fonction réglage de l'heure
void reglageHeure() {

  clic = 0;
  triple_clic = 0;
  previousMillis = millis();
  nbHeures = 0;
  nbMinutes = 0;


  display.clear();
  display.setSegments(Hour); // Affiche : Hour
  delay(1000);
  display.clear();

  // Tant que l'heure n'est pas réglée
  while (heure_reglee == false) {
    // On actualise l'état du bouton pour détecter les clics
    bouton.actualiser();

    unsigned long temps_ms = millis();
    // L'heure que l'on règle clignote
    if (temps_ms - previousMillis >= clignotement) {
      previousMillis = temps_ms;
      if (etat_clignotement == true) {
        display.showNumberDecEx(HEURE, 0b11100000, true, 4, 0);
      }
      if (etat_clignotement == false) {
        display.clear();
      }
      etat_clignotement = !etat_clignotement;
    }

    // On incrémente une heure à chaque clic
    if (clic > 0 and double_clic == 0) {
      nbHeures++;
      nbHeures %= 24;
      clic = 0;
    }

    // On valide les heures et on passe aux minutes
    if (double_clic == 1) {
      display.clear();
      display.setSegments(Min); // Affiche : min
      delay(1000);
      double_clic++;
    }

    // On incrémente une minute à chaque clic
    if (clic > 0 and double_clic == 2) {
      nbMinutes++;
      nbMinutes %= 60;
      clic = 0;
    }

    // On valide les minutes et donc l'heure est réglée
    if (double_clic > 2) {
      heure_reglee = true;
      double_clic = 0;
    }
    HEURE = nbHeures * 100 + nbMinutes;
  }
  // On réinitialise la valeure heure_reglee pour pouvoir re-régler l'heure si besoin
  heure_reglee = false;
  previousMillis = 0;
}


// Fonction du réglage de l'alarme, même principe que pour l'heure
void reglageAlarme() {

  double_clic = 0;
  clic = 0;
  previousMillis = millis();

  display.clear();
  display.setSegments(Hour); // Affiche : Hour
  delay(1000);
  display.clear();

  int minutesAlarme = 0, heuresAlarme = 0;

  while (alarme_reglee == false) {

    bouton.actualiser();

    unsigned long temps = millis();

    if (temps - previousMillis >= clignotement) {
      previousMillis = temps;
      if (etat_clignotement == true) {
        display.showNumberDecEx(HEURE_ALARME, 0b11100000, true, 4, 0);
      }
      if (etat_clignotement == false) {
        display.clear();
      }
      etat_clignotement = !etat_clignotement;
    }

    if (clic > 0 and double_clic == 0) {
      heuresAlarme++;
      clic = 0;
    }

    if (double_clic == 1) {
      display.clear();
      display.setSegments(Min); // Affiche : min
      delay(1000);
      double_clic++;
    }

    if (clic > 0 and double_clic == 2) {
      minutesAlarme++;
      clic = 0;
    }

    if (double_clic > 2) {
      alarme_reglee = true;
      double_clic = 0;
    }
    HEURE_ALARME = heuresAlarme * 100 + minutesAlarme;
  }
  previousMillis = 0;
  alarme_reglee = false;
}


// Fonction qui ajoute à l'heure le temps passé pendant le réglage de l'alarme et le réveil
void ajoutTemps(unsigned long temp) {
  // temp correspondant au temps passé en ms on le décrémente en secondes/min/heures pour l'ajouter à l'heure affichée
  while (temp > 1000) {
    temp -= 1000;
    nbSecondes++;

    if (nbSecondes > 59) {
      nbSecondes = 0;
      nbMinutes++;
    }

    if (nbMinutes > 59) {
      nbMinutes = 0;
      nbHeures++;
    }

    if (nbHeures > 23) {
      nbHeures = 0;
    }
  }
  HEURE = calculHeure(1);
}

// Fonction des moteurs assignant chaque mouvement possible à un cas préçis
void fctMoteur(char type) {

  switch (type) {
    case 'A':
      Serial.println("Moteur vers l'avant");
      digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
      digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
      break;
    case 'D':
      Serial.println("Moteur vers la droite");
      digitalWrite(in1, HIGH); digitalWrite(in2, LOW);
      digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
      break;
    case 'G':
      Serial.println("Moteur vers la gauche");
      digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
      digitalWrite(in3, LOW); digitalWrite(in4, HIGH);
      break;
    case 'R':
      Serial.println("Moteur vers l'arrière");
      digitalWrite(in1, LOW); digitalWrite(in2, HIGH);
      digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
      break;
  }
  analogWrite(enA, 255); analogWrite(enB, 255);
}

// Fonction de l'alarme
void fctAlarme() {

  // Initialisation des constantes de la musique
  int tempo = 144;
  // int notes = sizeof(melodie) / sizeof(melodie[0]) / 2;
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;
  int thisNote = 0;


  unsigned long previousMillis_moteurs = millis(), previousMillis_melodie = millis();
  previousMillis = millis();
  int intervalle = 0;
  bool etatMoteur = true;

  // Tant qu'on n'a pas effectué de triple clic
  while (!triple_clic) {

    bouton.actualiser();

    unsigned long temps_ms = millis();

    if (temps_ms - previousMillis >= clignotement_alarme) {
      previousMillis = temps_ms;
      if (etat_clignotement == true) {
        display.setSegments(Heyy); // Affiche : Heyy
      }
      if (etat_clignotement == false) {
        display.clear();
      }
      etat_clignotement = !etat_clignotement;
    }
    int notes = sizeof(melody_harry) / sizeof(melody_harry[0]) / 2;
    // Musique !
    if (temps_ms - previousMillis_melodie >= noteDuration) {
      if (thisNote >= notes )
        thisNote = 0;
      noTone(buzzer);
      divider = melody_harry[thisNote + 1];
      if (divider > 0) {
        noteDuration = (wholenote) / divider;
      } else if (divider < 0) {
        noteDuration = (wholenote) / abs(divider);
        noteDuration *= 1.5;
      }
      tone(buzzer, melody_harry[thisNote], noteDuration * 0.9);
      previousMillis_melodie = millis();
      thisNote += 2;
    }

    // Assignation aléatoire d'un mouvement aux moteurs

    if (temps_ms - previousMillis_moteurs >= intervalle) {
      if (etatMoteur) {
        int randNumber1 = random(0, 3);
        switch (randNumber1) {
          case 0:
            fctMoteur('D');
            intervalle = 3000;
            break;
          case 1:
            fctMoteur('G');
            intervalle = 3000;
            break;
          case 2:
            intervalle = 0;
            break;
          default:
            break;
        }
        etatMoteur = !etatMoteur;
      }
      else {
        int randNumber2 = random(0, 2);

        if (randNumber2 == 0)
          fctMoteur('A');
        else
          fctMoteur('R');
        intervalle = 6000;
        etatMoteur = !etatMoteur;
      }
      previousMillis_moteurs = millis();
    }
  }
  analogWrite(enA, 0); analogWrite(enB, 0);
  noTone(buzzer);
  delay(100);
}