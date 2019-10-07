#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "OneLED.h"
#include "AccelStepper.h"

// Pins (digital)
const uint8_t DIRECTION_PIN = 2;
const uint8_t STEP_PIN = 3;
const uint8_t BUZZER_PIN = 8;

// Pins (analog)
const uint8_t POTI_PIN = A0;

// I2C
// Board	        I2C / TWI pins
// -------------------------------------
// Uno, Ethernet	A4 (SDA),   A5 (SCL)
// Mega2560	        20 (SDA),   21 (SCL)
// Leonardo	        02 (SDA),   03 (SCL)
//
// SDA - Lila
// SCL - Blau

const uint8_t OLED_ADDRESS = 0x3c;
const uint8_t SDA_PIN = A4;
const uint8_t SCL_PIN = A5;

// Globals
int32_t stepCounter = 0;
int32_t prevSpeed = 0;
long oldMillis = 0;
long currentMillis = 0;

enum class States {
    Error, Idle,
    MotorOn, MotorOff
};

enum class Position { Top, Bottom, Undefined };

enum class Direction { Up, Down } direction { Direction::Up };

const int STEPS_PER_REVOLUTION = 200;
const uint8_t MotorType = 1;

// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to.
//
// Die Pin-Nummerierung steht auf der H-Bridge
// Stepper stepper(STEPS_PER_REVOLUTION, 8, 9,10,11);
// AccelStepper stepper = AccelStepper(MotorType4Wires, 8, 9, 10, 11);
// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(MotorType, STEP_PIN, DIRECTION_PIN);

OneLED ledMotor { LED_BUILTIN };

const uint8_t SCREEN_WIDTH = 128; // OLED display width, in pixels
const uint8_t SCREEN_HEIGHT = 64; // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
const uint8_t OLED_RESET  = -1; // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void clearLine(uint8_t line);

/// Hauptstatus
/// Per default fährt die Türe hoch und initialisiert sich.
States currentState = States::MotorOff;

// Poti + Filter (Exponential Moving Average algorithm)
//      Mehr: https://www.norwegiancreations.com/2015/10/tutorial-potentiometers-with-arduino-and-filtering/
int EMA_S = 0;          //initialization of EMA S
int32_t readPotti();

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    ledMotor.off();

    // Init Buzzer
    pinMode(BUZZER_PIN, OUTPUT);

    // Init Stepper-Pins
    // Declare pins as output:
    // pinMode(STEP_PIN, OUTPUT);
    // pinMode(DIRECTION_PIN, OUTPUT);


    // Basic Stepper settings
    // Set the speed of the motor to x RPMs
    pinMode(POTI_PIN, INPUT);
    stepper.setMaxSpeed(1000);
    EMA_S = analogRead(POTI_PIN);  //set EMA S for t=1
    // stepper.setSpeed(1000);

    // OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    delay(2000);
    display.setRotation(2);
    display.clearDisplay();

    //  1 character height will be 7 pixel.
    display.setTextSize(1);
    display.setTextColor(WHITE, BLACK);
    display.setCursor(25, 55);
    // Display static text
    display.print("Stepper Test, 1.0");
    display.display();

    oldMillis = millis();
}


// the loop function runs over and over again forever

void loop() {
    currentMillis = millis();

    // read the input pin (max 1024)
    auto val = readPotti();
    currentState = States::MotorOff;
    if(val > 2 || val < -2) {
        currentState = States::MotorOn;
    }

    switch(currentState) {
        case States::MotorOn:
            ledMotor.on();

            // Basic Stepper settings
            // Set the speed of the motor to x RPMs
            if(prevSpeed <= val - 2 || prevSpeed >= val + 2) {
                prevSpeed = val;

                const int32_t speed = val * 1000 / 49;

                // Serial.println("Val:" + String(val) + ", Speed: " + String(speed));
                display.setCursor(0, 0);
                clearLine(0);
                
                display.println("Val:" + String(val) + ", Speed: " + String(speed));
                display.display();

                stepper.setSpeed(speed);

                tone(BUZZER_PIN, 700, 20);
                // delay(100);
            }

            // stepper.step(direction == Direction::Up ? 6400 : -6400);
            // stepper.step(direction == Direction::Up ? 50 : -50);

            // Val ist -49 - +49 (links bzw. rechts)
            // stepper.step(val <= 0 ? -STEPS_PER_REVOLUTION : STEPS_PER_REVOLUTION);
            // stepper.step(val);
            stepper.runSpeed();
            
            stepCounter++;
            break;

        case States::MotorOff:
            display.setCursor(0, 0);
            clearLine(0);

            display.println("Motor off!");
            display.display();

            ledMotor.off();
            //digitalWrite(DIRECTION_PIN,direction == Direction::Up ? HIGH : LOW);

            currentState = States::Idle;
            stepCounter = 0;
            break;

        case States::Error:
        case States::Idle:
            break;
    }

    // delay(1);
}

/**
 * Liest den Potti auf POTTI_PIN und gibt einen Wert zwischen -49 und + 49 zurück
 *
 * @return Wert zwischen -49 und +49
 */
int32_t readPotti() {
    const float EMA_a = 0.6;      //initialization of EMA alpha
    const int maxReads = 1;
    
    // read the input pin (max 1024)
    int val = analogRead(POTI_PIN);
    EMA_S = (EMA_a * val) + ((1 - EMA_a) * EMA_S);

    val = EMA_S;

    if(val >= 512) {
        val = (val - 512) / 10;
    } else {
        val = ((512 - val) / 10) * -1;
    }
    val = min(val, 49);

    return round(val);
}

void clearLine(uint8_t line) {
    for (int y = line; y <= line + 6; y++)
    {
        for (int x = 0; x < 127; x++)
        {
            display.drawPixel(x, y, BLACK);
        }
    }
}
