// voeg de libraries in deze volgorde aan je code toe. Kans is vrij groot dat je anders een foutmelding krijgt
#define BLYNK_TEMPLATE_ID "user3"
#define BLYNK_TEMPLATE_NAME "user3@server.wyns.it"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

/* === HARDWARE CONFIGURATIE === */
// PSEUDOCODE: Definieer fysieke aansluitingen
#define DHTPIN 4       // DHT11 data pin
#define DHTTYPE DHT11  // Sensortype
const int led1Pin = 23; // Auto-mode indicator
const int led2Pin = 16; // Handmatige modus LED
const int led3Pin = 17; // Meldings-LED (15 sec)

DHT dht(DHTPIN, DHTTYPE); // Initialiseer sensor

/* === NETWERKCONFIGURATIE === */
//Authenticatiegegevens
char auth[] = "gcuRYHN6OGti4-xxxxxxxxxxxx"; // Blynk token
char ssid[] = "x"; // WiFi netwerknaam
char pass[] = "x"; // WiFi wachtwoord

/* === GLOBALE VARIABELEN === */
//Toestandsvariabelen
float humidity, temperature; // Sensorwaarden
int humidityThreshold = 50;  // Aanpasbare drempel (%)

// PSEUDOCODE: Besturingsvariabelen
bool button1State = false;   // Knop 1 status
bool button2State = false;   // Knop 2 status
bool led2State = false;      // LED2 geheugen
unsigned long led3StartTime = 0; // Timer voor LED3
const unsigned long led3Duration = 15000; // 15 seconden

BlynkTimer timer; // Voor periodieke taken

/* === FUNCTIE: UPDATE BLYNK DATA === */
// PSEUDOCODE:
// VOER ELKE 500ms UIT:
//   1. Verzend sensorwaarden naar cloud
//   2. Verzend LED-statussen
//   3. Print debug naar serial
void updateBlynkData() {
  // Stuur sensorwaarden
  Blynk.virtualWrite(V6, humidity); // V6 = Vochtigheid
  Blynk.virtualWrite(V7, temperature); // V7 = Temperatuur
  
  // Stuur instellingen
  Blynk.virtualWrite(V8, humidityThreshold); // V8 = Drempel
  
  // Stuur LED-statussen (255=aan, 0=uit)
  Blynk.virtualWrite(V10, digitalRead(led1Pin) ? 255 : 0);
  Blynk.virtualWrite(V11, digitalRead(led2Pin) ? 255 : 0);
  Blynk.virtualWrite(V12, digitalRead(led3Pin) ? 255 : 0);

  // Debug output
  Serial.printf("Drempel: %d%%, Vochtigheid: %.1f%%, Temp: %.1f°C\n", 
               humidityThreshold, humidity, temperature);
}

/* === FUNCTIE: INITIALISATIE === */

// 1. Start serial communicatie
// 2. Zet pinmodes voor LED's
// 3. Start sensor
// 4. Verbind met WiFi
// 5. Initialiseer Blynk
// 6. Stel timers in
void setup() {
  Serial.begin(115200);
  
  // Configureer LED-pinnen als uitvoer
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  digitalWrite(led3Pin, LOW);
  
  dht.begin(); // Start DHT sensor
  
  //Verbind met WiFi
  Serial.print("Verbinden met WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Verbonden!");
  
  Blynk.begin(auth, ssid, pass, "server.wyns.it", 8081);
  
  // Stel periodieke taken in
  timer.setInterval(2000L, readSensorData); // Lees sensor elke 2 sec
  timer.setInterval(500L, updateBlynkData); // Update Blynk elke 0.5 sec
}

/* === HOOFDLUS === */
// 
// 1. Verwerk Blynk events
// 2. Voer timertaken uit
// 3. Beheer LED3 timer
void loop() {
  Blynk.run(); // Onderhoud Blynk verbinding
  timer.run(); // Voer timertaken uit
  
  //  Controleer LED3 timer
  if (led3StartTime > 0 && millis() - led3StartTime >= led3Duration) {
    digitalWrite(led3Pin, LOW); // Zet LED3 uit na 15 sec
    led3StartTime = 0; // Reset timer
    updateBlynkData(); // Stuur update
  }
}

/* === FUNCTIE: LEES SENSORDATA === */
// 
// 1. Lees vochtigheid en temperatuur
// 2. Als meting geldig:
//    a. Update LED1 op basis van drempel
//    b. Reset LED2 als onder drempel
void readSensorData() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  if (!isnan(humidity) && !isnan(temperature)) {
    //  Regel LED1 (auto-modus)
    digitalWrite(led1Pin, humidity > humidityThreshold ? HIGH : LOW);
    
    //  Reset LED2 als vochtigheid te laag
    if (humidity <= humidityThreshold) led2State = false;
    digitalWrite(led2Pin, led2State ? HIGH : LOW);
  }
}

/* === BLYNK SLIDER VOOR DRIMPELWAARDE (V5) === */

// WANNEER SLIDER WORDT AANGEPAST:
//   1. Update drempelwaarde
//   2. Log nieuwe waarde
BLYNK_WRITE(V5) {
  humidityThreshold = param.asInt(); // Ontvang nieuwe waarde
  Serial.print("Nieuwe drempel: ");
  Serial.println(humidityThreshold);
}

/* === BLYNK KNOP 1 HANDLER (V0) === */

// WANNEER KNOP WORDT INGEDRUKT:
//   1. Onthoud knopstatus
//   2. Als vochtigheid > drempel:
//      a. Zet LED2 aan
//      b. Onthoud status
BLYNK_WRITE(V0) {
  button1State = param.asInt();
  if (humidity > humidityThreshold && button1State) {
    led2State = true;
    digitalWrite(led2Pin, HIGH);
  }
}

/* === BLYNK KNOP 2 HANDLER (V1) === */
// WANNEER KNOP WORDT INGEDRUKT:
//   1. Zet LED2 uit
//   2. Activeer LED3 voor 15 sec
//   3. Start timer
BLYNK_WRITE(V1) {
  if (param.asInt()) {
    led2State = false;
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, HIGH);
    led3StartTime = millis(); // Start 15 sec timer
  }
}