#include <util/atomic.h>
#include <LiquidCrystal_I2C.h>

#define LED_RED 6
#define LED_GREEN 5
#define LED_BLUE 3
#define GREEN_BUTTON 4
#define ENCODER1 A2
#define ENCODER2 A3
#define DEBOUNCING_PERIOD 50

LiquidCrystal_I2C lcd(0x27, 16, 2);

int previousButtonState = HIGH;
bool buttonPressed = false;
volatile unsigned long buttonTimestamp = 0UL;
unsigned long previousButtonTimestamp = 0UL;
volatile unsigned long encoderTimestamp = 0UL;
unsigned long previousEncoderTimestamp = 0UL;

const char* menuItems[] = {"Red Brightness", "Green Brightness", "Blue Brightness", "Turn Off RGB"};
const int menuLength = 4;
int menuPosition = 0;

struct LEDState {
    int pin;
    int brightness;
};

LEDState redLed = {LED_RED, 0};
LEDState greenLed = {LED_GREEN, 0};
LEDState blueLed = {LED_BLUE, 0};

ISR(PCINT1_vect) {
    encoderTimestamp = millis();
}

ISR(PCINT2_vect) {
    buttonTimestamp = millis();
}

void initRGB() {
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    analogWrite(LED_RED, 0);
    analogWrite(LED_GREEN, 0);
    analogWrite(LED_BLUE, 0);
    redLed.brightness = 0;
    blueLed.brightness = 0;
    greenLed.brightness = 0;
}

void updateRGB() {
    analogWrite(LED_RED, redLed.brightness);
    analogWrite(LED_GREEN, greenLed.brightness);
    analogWrite(LED_BLUE, blueLed.brightness);
}

void displayMenu() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(menuItems[menuPosition]);
    lcd.setCursor(0, 1);
    lcd.print(menuItems[(menuPosition + 1) % menuLength]);
    lcd.setCursor(0, 0);
    lcd.cursor();
    lcd.blink();
}

void printAdjustBrightnessMenu(LEDState &led) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (led.pin == LED_RED)
        lcd.print("RED");
    else if (led.pin == LED_GREEN)
        lcd.print("GREEN");
    else
        lcd.print("BLUE");
    lcd.setCursor(0, 1);
    lcd.print("Value: ");
    alignAndPrintNumbers(led);
}

void alignAndPrintNumbers(LEDState &led) {
    lcd.setCursor(7, 1);
    if (led.brightness < 100) {
        lcd.print("0");
        if (led.brightness == 0)
            lcd.print("0");
    }
    lcd.print(led.brightness);
}

void changeBrightnessValue(LEDState &led) {
  unsigned long timestamp;
      int enc1 = digitalRead(ENCODER1);
      int enc2 = digitalRead(ENCODER2);

      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
          timestamp = encoderTimestamp;
      }

      if (enc1 == LOW && timestamp - previousEncoderTimestamp > DEBOUNCING_PERIOD) {
          if (enc2 == HIGH)
              led.brightness = min(255, led.brightness + 15);
          else
              led.brightness = max(0, led.brightness - 15);

          updateRGB();
          alignAndPrintNumbers(led);

          previousEncoderTimestamp = timestamp;    
      }
}

void adjustBrightness(LEDState &led) {
    printAdjustBrightnessMenu(led);

    while (!buttonPressed) {
      changeBrightnessValue(led);
      wasButtonPressed(); 
    }
    
    buttonPressed = false;
    displayMenu();
}

void wasButtonPressed() {
    unsigned long timestamp;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        timestamp = buttonTimestamp;
    }

    if (digitalRead(GREEN_BUTTON) == LOW && previousButtonState == HIGH) {
        if (timestamp != previousButtonTimestamp && millis() - timestamp > DEBOUNCING_PERIOD) {
          buttonPressed = true;
          previousButtonState = LOW;            
          previousButtonTimestamp = timestamp;
        }
    } else if (digitalRead(GREEN_BUTTON) == HIGH && previousButtonState == LOW) {
        if (timestamp != previousButtonTimestamp && millis() - timestamp > DEBOUNCING_PERIOD) {
          previousButtonState = HIGH;     
          previousButtonTimestamp = timestamp;
        }
    }
}

void changeMenuPosition() {
    unsigned long timestamp;
    int enc1 = digitalRead(ENCODER1);
    int enc2 = digitalRead(ENCODER2);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        timestamp = encoderTimestamp;
    }

    if (enc1 == LOW && timestamp - previousEncoderTimestamp > DEBOUNCING_PERIOD) {
        menuPosition = (enc2 == HIGH) ? (menuPosition + 1) % menuLength : (menuPosition - 1 + menuLength) % menuLength;
        displayMenu();
        previousEncoderTimestamp = timestamp;
    }
}

void selectMenuPosition() {
    wasButtonPressed();
    if (buttonPressed) {
      buttonPressed = false;
      switch (menuPosition) {
          case 0: adjustBrightness(redLed); break;
          case 1: adjustBrightness(greenLed); break;
          case 2: adjustBrightness(blueLed); break;
          case 3: initRGB(); break;
      }
    }
}

void setup() {
    pinMode(ENCODER1, INPUT_PULLUP);
    pinMode(ENCODER2, INPUT_PULLUP);
    pinMode(GREEN_BUTTON, INPUT_PULLUP);

    lcd.init();
    lcd.backlight();
    displayMenu();

    PCICR |= (1 << PCIE1) | (1 << PCIE2);
    PCMSK1 |= (1 << PCINT10) | (1 << PCINT11);
    PCMSK2 |= (1 << PCINT20);

    initRGB();
}

void loop() {
    changeMenuPosition();
    selectMenuPosition();
}
