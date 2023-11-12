#include <Arduino.h> //Bibliothèque Arduino

//Bibliothèques de la clock
    #include <Wire.h> //Bibliothèque pour l'écran OLED et l'horloge RTC
    #include <Adafruit_SSD1306.h> //Bibliothèque pour l'écran OLED
    #include <Adafruit_GFX.h> //Bibliothèque pour l'écran OLED
    #include <RtcDS1302.h> //Bibliothèque pour l'horloge RTC

//Macros du capteur
    define X 50 //Taille du tableau de moyenne de valeurs lues
    #define Y 10 //Taille du tableau de valeurs de temps où les battements ont lieux
    #define SEUIL 1 //seuil de détection de battement

//Macros de la clock
    #define SCREEN_WIDTH 128 //Largeur de l'écran (en pixels)
    #define SCREEN_HEIGHT 64 //Hauteur de l'écran (en pixels)

//Variables du capteur

    //Définition du temps (exprimé en millisecondes)
        unsigned long temps = 0; //Le temps qui s'est écoulé depuis la mise en marche du programme

    //Lecture et écriture des valeurs dans les variables et le tableau
        const int capteur = A0; //Port de lecture du capteur
        unsigned int reading = 0; //Valeur lue sur le port du capteur
        float valeurUtile = 0; //Correspond a la valeur renvoyée par le capteur à laquelle on soustrait "moyenne"
    
    //Première moyenne / Moyenne glissante
        unsigned int readed[X] = {0}; //Le tableau de valeurs lues par le capteur
        unsigned int c = 0; //L'incrémanteur pour le tableau de valeurs lues par le capteur
        double d = 0; //La validation des X premières valeurs
        float moyenne = 0; //La moyenne des valeurs du tableau
        
    
    //Moyenne max
        int tableauVU[X] = {0}; //Le tableau dans lequel on stocke "valeurUtile"
        int tableauMax[Y] = {0}; //Le tableau dans lequel on stocke "max"
        int max = 0; //La valeur maximale qui est lue dans "tableauVU" 
        int e = 0; //L'incrémenteur de "tableauMax"
        int f = 0; //Somme des valeur de "tableauMax"

    //Détection battement
        unsigned int tempsPrecedent = 0; //Evolue avec les valeurs du tableau de temps mais avec un tour de retard
        unsigned long b = 0; //L'incrémanteur pour le tableau de temps où sont mesurés les battements
    
    //Calcul du bpm 
        unsigned long tempsBattements[Y] = {0}; //Le tableau de temps où sont mesurés les battements
        float moyenneMax = 0; //La valeur moyenne des pics de variation de flux sanguin
        float tempsMoyen = 0.0; //Le temps moyen entre deux battements captés
        double bpm = 0.0; //Les battements par minute
    
//Variables de la clock
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); //Définition de l'écran OLED
    ThreeWire myWire(3,2,4); //Définition des broches de l'horloge RTC
    RtcDS1302<ThreeWire> Rtc(myWire); //Définition de l'horloge RTC

//Variables des LEDs
    const int led_verte = 1 ; //définition de la broche 1 comme étant la led verte
    const int led_jaune = 2 ; //définition de la broche 2 comme étant la led jaune
    const int led_rouge = 3 ; //définition de la broche 3 comme étant la led rouge
    const int pindetecteurdepoul = A0; //définition de la broche A0 comme étant le détecteur de pouls
    int Pouls; //définition de la variable Pouls

//Variables du beeper
    const int beeperPin = 4; //Pin beeper
    const int pindetecteurdepoul = A0; //Pin du détecteur de pouls
    float Pouls; //Variable de la valeur du pouls
    float BATT_S; //Variable de la valeur de la battement par seconde
    const int boutonPin = 2; //Pin du bouton
    volatile bool etatProgramme = false; //Variable de l'état du programme
    int etatBouton = 0; //Variable de l'état du bouton
    int frequence = 0; //variable de la fréquence émise par le beeper (en hertz)
    int periodeMicros = 0; //Correspond à 1/"frequence" (en microsecondes)
    int DemiPeriode = 0; //Correspond à "frequence"/2 (en microsecondes)
    int duree = 0; //Variable de durée d'activation du beeper (en millisecondes)

void setup() {
    //Setup du capteur
        pinMode(capteur,INPUT); //Paramétrage du port de lecture du capteur en entrée
        Serial.begin(9600); //Fréquence de l'information en bits/s

    //Setup de la clock
        Serial.begin(9600); //Initialisation de la communication série et taux de raffraichissement
        display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Initialisation de l'écran OLED
        display.setTextSize(2); //Définition de la taille du texte
        display.setTextColor(WHITE); //Définition de la couleur du texte
        Rtc.Begin(); //Initialisation de l'horloge RTC
    
    //Setup des LED
        pinMode(pindetecteurdepoul,INPUT); //définition de la broche A0 comme étant une entrée
        pinMode(led_verte,OUTPUT); //définition de la broche 1 comme étant une sortie
        pinMode(led_jaune,OUTPUT); //définition de la broche 2 comme étant une sortie
        pinMode(led_rouge,OUTPUT); //définition de la broche 3 comme étant une sortie
    
    //Setup du beeper
        pinMode(beeperPin, OUTPUT); //Définition de la pin beeper en sortie
        pinMode(pindetecteurdepoul,INPUT); //Définition de la pin du détecteur de pouls en entrée
        pinMode(boutonPin,INPUT); //Définition de la pin du bouton en entrée
        attachInterrupt(digitalPinToInterrupt(boutonPin),gestionnaireInterrupt, FALLING); //Définition de l'interruption du bouton
}

//Fonctions interruption beeper
    void gestionnaireInterrupt() {
        // Bascule l'état du programme lorsque l'interruption est déclenchée
        etatProgramme = !etatProgramme;

        // Si le programme devient inactif, arrêter la tonalité
        if(!etatProgramme) {
            digitalWrite(beeperPin, LOW);
        }
    }

void loop() {

//Code du capteur

    //Définition du temps (exprimé en millisecondes)
        temps = millis();

    //Lecture et écriture des valeurs dans les variables et le tableau
        reading = analogRead(capteur); //Lecture du capteur
        valeurUtile = (float)reading - moyenne;

    //Moyenne glissante
        if(d != 0) { //Si la moyenne a déjà été calculée
            d -= readed[c];
            readed[c] = reading;
            d += readed[c];
            moyenne = (float)d / (float)(X);
        } 

    max = tableauVU[0];
    tableauVU[c] = valeurUtile;
    
    //Moyenne max
        if(d != 0) { //Si la moyenne a déjà été calculée
            for(int i = 0; i < X; i++) {
                max = (tableauVU[i] > max) ? tableauVU[i] : max;
            }
            f -= tableauMax[e];
            tableauMax[e] = max;
            f += tableauMax[e];
            moyenneMax = (float)f / (float)X;
            e = (e + 1) % Y;
        }

    Serial.println(bpm); //Affichage du bpm calculé
    readed[c] = reading; //Placement de la valeur lue dans le tableau
    c = (c + 1) % X; //Incrémentation

    //Première moyenne
        if(c == 0 && d == 0) { //Si l'incrémenteur vaut X et le tableau n'a encore jamais été rempli
            for(int i = 0; i < X; i++) { //Pour chaque valeur du tableau
                d += readed[i]; //Somme des valeurs du tableau
            }
            moyenne = d / X; //Division de la somme des valeurs par le nombre de valeurs présentes dans le tableau
        }

    //Détection battement
        if(valeurUtile >= moyenneMax + SEUIL) {
            if(temps - tempsPrecedent >= 300) {
                tempsBattements[b] = temps;
                tempsPrecedent = tempsBattements[b];
                b++;
                if(b == Y) b = 0;
            }
        }

    //Calcul du bpm  
        if(b == 0) {
            for(int i = 0; i < Y-1; i++) b += abs(tempsBattements[i+1] - tempsBattements[i]);
            tempsMoyen = (float)b / (float)(Y-1);
            bpm = (100 * 60) / tempsMoyen;
            bpm *= 10;
            b = 0;
        }

//Code de la clock

    RtcDateTime now = Rtc.GetDateTime(); //Récupération de la date et de l'heure à chaque début de boucle
    display.clearDisplay(); //Effacement de l'écran OLED
    display.setCursor(0,1);

    //Affichage de la date sur l'écrna OLED
        display.println("Date: ");
        display.print(now.Day()); //Affichage du jour
        display.print("/");
        display.print(now.Month()); //Affichage du mois
        display.print("/");
        display.println(now.Year()); //Affichage de l'année

    //Affichage de l'heure sur l'écran OLED
        display.println("Time: ");
        display.print(now.Hour()); //Affichage de l'heure
        display.print(":");
        display.print(now.Minute()); //Affichage des minutes
        display.print(":");
        display.print(now.Second()); //Affichage des secondes
        display.display(); //Affichage de l'écran OLED

//Code des LED

    //lecture de la valeur du détecteur de pouls
        Pouls = analogRead(pindetecteurdepoul);

    //si la valeur du détecteur de pouls est comprise entre 100 et 170
        if(Pouls>100 && Pouls<=160) { 
            digitalWrite(led_rouge,HIGH); //la led rouge s'allume
            digitalWrite(led_jaune,LOW); //la led jaune s'éteint
            digitalWrite(led_verte,LOW); //la led verte s'éteint
        }

    //si la valeur du détecteur de pouls est comprise entre 30 et 50
        else if(Pouls<50 && Pouls >= 30) {
            digitalWrite(led_rouge,LOW); //la led rouge s'éteint
            digitalWrite(led_jaune,HIGH); //la led jaune s'allume
            digitalWrite(led_verte,LOW); // la led verte s'éteint
        }

    //si la valeur du détecteur de pouls est comprise entre 50 et 100
        else if(Pouls>=50 && Pouls<=100) { 
        digitalWrite(led_rouge,LOW); //la led rouge s'éteint
        digitalWrite(led_jaune,LOW); //la led jaune s'éteint
        digitalWrite(led_verte,HIGH); //la led verte s'allume
        }

    //si la valeur du détecteur de pouls n'est pas comprise entre 30 et 170
        else { 
            digitalWrite(led_rouge,LOW); //la led rouge s'éteint
            digitalWrite(led_jaune,LOW); //la led jaune s'éteint
            digitalWrite(led_verte,LOW); //la led verte s'éteint
        }

//Code du beeper

    Pouls=analogRead(pindetecteurdepoul); //Lecture de la valeur du pouls
    BATT_S  = (60/Pouls)*1000 ; //Calcul de la valeur de la battement par seconde
    etatBouton = digitalRead(boutonPin); //Lecture de l'état du bouton

    if(etatProgramme) {
        if(Pouls>100) {
            // Génère une tonalité à 1200 Hz pendant 1 seconde
                frequence = 1200; // Fréquence en Hertz
                duree = 1000; // Durée en millisecondes

            periodeMicros = (1000000 / frequence); // Période en microsecondes
            DemiPeriode = periodeMicros / 2; // Demi-période en microsecondes

            for(int k = 0; k < duree * 1000; k += periodeMicros) { //Boucle de la tonalité
                // Cycle ON
                    digitalWrite(beeperPin, HIGH); //Définition du beeper en état haut
                    delayMicroseconds(DemiPeriode); //Demi-période en microsecondes

                // Cycle OFF
                    digitalWrite(beeperPin, LOW); //Définition du beeper en état bas
                    delayMicroseconds(DemiPeriode); //Demi-période en microsecondes
            }
        delay(BATT_S); //Pause de la tonalité
        }

        else if (Pouls<50) { // Le "else if" et le "else" fonctionne de la même manière que le "if"
            // Génère une tonalité à 300 Hz pendant 1 seconde
                frequence = 300;
                duree = 1000;
 
            periodeMicros = (1000000 / frequence); // Période en microsecondes
            DemiPeriode = periodeMicros / 2; // Demi-période en microsecondes

            for (int k = 0; k < duree  * 1000; k += periodeMicros) {
                // Cycle ON
                    digitalWrite(beeperPin, HIGH);
                    delayMicroseconds(DemiPeriode);

                // Cycle OFF
                    digitalWrite(beeperPin, LOW);
                    delayMicroseconds(DemiPeriode);
            }
        delay(BATT_S);
        }

        else  {
            // Génère une tonalité à 600 Hz pendant 1 seconde
                frequence = 600;
                duree = 1000;
 
            periodeMicros = (1000000 / frequence); // Période en microsecondes
            DemiPeriode = periodeMicros / 2; // Demi-période en microsecondes

            for (int k = 0; k < duree * 1000; k += periodeMicros) {
                // Cycle ON
                    digitalWrite(beeperPin, HIGH);
                    delayMicroseconds(DemiPeriode);

                // Cycle OFF
                    digitalWrite(beeperPin, LOW);
                    delayMicroseconds(DemiPeriode);
            }
            delay(BATT_S);
        }
    }
}