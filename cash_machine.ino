#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <Key.h>
#include <Keypad.h>

#include "Adafruit_Thermal.h"
#include "SoftwareSerial.h"

#include "banana.h"

#define TX_PIN 10
#define RX_PIN 11

#define PIEZO_PIN 12

#define DEBUG 1

SoftwareSerial SoftSerial(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Thermal printer(&SoftSerial);

#define CATALOGO_SIZE 13
#define SCONTRINO_SIZE 10

int catalog_index = 0;
int scontrino_index = 0;

struct Product {
  String text;
  int value;
};

Product catalogo[CATALOGO_SIZE] = {
  {"Pasta", 6},
  {"Fagioli", 5},
  {"Mele", 6},
  {"Banane", 3},
  {"Carciofi", 7},
  {"Ciliegie", 5},
  {"Fragole", 2},
  {"Hamburger",6},
  {"Insalata",1},
  {"Libro",15},
  {"Pesche",4},
  {"Albicocche",5},
  {"Biglie",10}
};

Product scontrino[SCONTRINO_SIZE];


const byte rows = 4; //four rows
const byte cols = 4; //three columns
char keys[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[rows] = {9, 8, 7, 6}; //Rows 0 to 3
byte colPins[cols] = {5, 4, 3, 2}; //Columns 0 to 3

Keypad  keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DISPLAY_ROWS 2
#define DISPLAY_COLS 16
char display_rows[DISPLAY_ROWS][DISPLAY_COLS];

void startMessage() {
  memcpy(display_rows[0] , "Il Negozio", 10);
  memcpy(display_rows[1] , "di Marty & Ricky", 16);
}

void beep(int delay) {
  analogWrite(PIEZO_PIN, 500);
  delayMicroseconds(delay);
  analogWrite(PIEZO_PIN, 0);
  delayMicroseconds(delay);
}


void initDisplay() {
  lcd.begin();
  lcd.backlight();

  startMessage();

  updateDisplay();
}

void updateDisplay() {
  lcd.clear();
  for (int r = 0; r < DISPLAY_ROWS; r++) {
    lcd.setCursor(0, r);
    lcd.print(display_rows[r]);
  }
}

void clearRowsBuffer() {
  memset(display_rows[0], 0, DISPLAY_COLS);
  memset(display_rows[1], 0, DISPLAY_COLS);
}

void updateProdDisplay() {
  clearRowsBuffer();
  char prezzo[5] = "";
  itoa(catalogo[catalog_index].value, prezzo, 10);

  memcpy(display_rows[0] , catalogo[catalog_index].text.c_str(), catalogo[catalog_index].text.length());
  memcpy(display_rows[1] , prezzo, strlen(prezzo));
}

void prodToScontrino() {
  if (scontrino_index < SCONTRINO_SIZE - 1) {
    memcpy(&scontrino[scontrino_index], &catalogo[catalog_index], sizeof(catalogo[catalog_index]));
  }
  else {
    scontrino_index = 0;
  }
  scontrino_index++;
}

void updateTotal() {
  int tot = 0;
  for (int i = 0; i < scontrino_index; i++) {
    Product riga = scontrino[i];
    tot += riga.value;
  }

  if (tot > 0) {
    lcd.setCursor(10, 1);
    lcd.print("TOT ");
    lcd.print(tot);
  }
}

void setup() {
  Serial.begin(9600);
  SoftSerial.begin(9600);
  printer.begin();
  randomSeed(analogRead(0));

  initDisplay();
  pinMode(PIEZO_PIN, OUTPUT);
}

#if DEBUG
void debugScontrino() {
  Serial.println("DEBUG SCONTRINO");
  for (int i = 0; i < scontrino_index; i++) {
    Product riga = scontrino[i];
    Serial.println(riga.text + " EURO " + riga.value);
  }
}
#endif


void loop() {

  // put your main code here, to run repeatedly:
  char key = keypad.getKey();

  if (key) {
    switch (key) {
      case '#':
        break;
      case '*':
        prodToScontrino();
        break;
      // UP catalogo prodotti
      case 'A':
        if (catalog_index < CATALOGO_SIZE - 1) {
          catalog_index++;
        }
        else {
          catalog_index = 0;
        }
        updateProdDisplay();
        break;
      // DOWN catalogo prodotti
      case 'B':
        if (catalog_index > 0) {
          catalog_index--;
        }
        else {
          catalog_index = CATALOGO_SIZE - 1;
        }
        updateProdDisplay();
        break;
      case 'C':
        clearRowsBuffer();
        startMessage();
        clearScontrino();
        lcd.clear();
        break;
      case 'D':
        printScontrino();
        clearScontrino();
        clearRowsBuffer();
        startMessage();
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        int new_price =  key - '0';
        catalogo[catalog_index].value = new_price;
        updateProdDisplay();
        break;

    }
    updateDisplay();
    updateTotal();
    beep(300);
#if DEBUG
    Serial.println(key);
    debugScontrino();
#endif
  }
}

void printScontrino() {
  printer.printBitmap(20, 24, banana);
  printer.println();
  printer.boldOn();
  printer.justify('C');
  printer.println(F("IL NEGOZIO"));
  printer.println(F("di"));
  printer.println(F("MARTY & RICKY"));
  printer.boldOff();
  printer.println();
  printer.println();

  printer.justify('L');
  printer.setSize('M');
  int tot = 0;
  for (int i = 0; i < scontrino_index; i++) {
    Product riga = scontrino[i];

    printer.println(riga.text + " EURO " + riga.value);
    if (DEBUG) {
      Serial.println(riga.text + " EURO " + riga.value);
    }

    tot += riga.value;
  }

  printer.println();
  printer.print(F("TOTALE "));
  String strTotal = String(tot);
  printer.println(strTotal);
  char totBuffer[5];
  strTotal.toCharArray(totBuffer, 5);
  printer.printBarcode(totBuffer, CODE39);

  printer.feed(3);
  printer.setDefault();
}

void clearScontrino() {
  scontrino_index = 0;
  memset(scontrino, 0, sizeof(Product)*SCONTRINO_SIZE);
}


