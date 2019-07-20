#include <SPI.h>
#include <AMIS30543.h>

#define MANUAL_PIN 18
#define BLAST_PIN 19
#define RINSE_PIN 20

const uint8_t amisDirPin = 2;
const uint8_t amisStepPin = 3;
const uint8_t amisSlaveSelect = 4;

AMIS30543 stepper;

void interrupt_setup()
{
    pinMode(MANUAL_PIN, INPUT_PULLUP);
    pinMode(BLAST_PIN, INPUT_PULLUP);
    pinMode(RINSE_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(MANUAL_PIN), toggle_manual, FALLING);
    attachInterrupt(digitalPinToInterrupt(BLAST_PIN), toggle_blast, FALLING);
    attachInterrupt(digitalPinToInterrupt(RINSE_PIN), toggle_rinse, FALLING);
}

void stepper_setup()
{
    SPI.begin();
    stepper.init(amisSlaveSelect);

    // Drive the NXT/STEP and DIR pins low initially.
    digitalWrite(amisStepPin, LOW);
    pinMode(amisStepPin, OUTPUT);
    digitalWrite(amisDirPin, LOW);
    pinMode(amisDirPin, OUTPUT);

    // Give the driver some time to power up.
    delay(1);

    // Reset the driver to its default settings.
    stepper.resetSettings();

    // Set the current limit.  You should change the number here to
    // an appropriate value for your particular system.
    stepper.setCurrentMilliamps(2000);

    // Set the number of microsteps that correspond to one full step.
    stepper.setStepMode(0);

    // Enable the motor outputs.
    stepper.enableDriver();

    // The NXT/STEP pin must not change for at least 0.5
    // microseconds before and after changing the DIR pin.
    delayMicroseconds(1);
    digitalWrite(amisDirPin, LOW);
    delayMicroseconds(1);
}

// Sends a pulse on the NXT/STEP pin to tell the driver to take
// one step, and also delays to control the speed of the motor.
void step(int delay_length)
{
    // The NXT/STEP minimum high pulse width is 2 microseconds.
    digitalWrite(amisStepPin, HIGH);
    delayMicroseconds(3);
    digitalWrite(amisStepPin, LOW);
    delayMicroseconds(3);

    // The delay here controls the stepper motor's speed.  You can
    // increase the delay to make the stepper motor go slower.  If
    // you decrease the delay, the stepper motor will go fast, but
    // there is a limit to how fast it can go before it starts
    // missing steps.
    delayMicroseconds(delay_length);
}

enum states {
  idle,
  manual,
  blast,
  rinse
};

volatile states current_state;

void toggle_manual() {current_state == manual ? current_state = idle : current_state = manual;}

void toggle_blast() {current_state == blast ? current_state = idle : current_state = blast;}

void toggle_rinse() {current_state == rinse ? current_state = idle : current_state = rinse;}

void idle_state()
{
    Serial.print("Idle State\n");
    // Turn off vacuum
    // Turn off air
    // Turn off media
}

void manual_state()
{
    Serial.print("Manual State\n");
    // Turn on vacuum
    // Turn off air
    // Turn off media
}

void blast_state()
{
    Serial.print("Blast State\n");
    // Turn on vacuum
    // Turn on air
    // Turn on media
    unsigned long start_time = millis();
    unsigned long stage_1_end_time = start_time + 1000 * 60 * 8;
    unsigned long stage_2_end_time = start_time + 1000 * 60 * 10;
    while (millis() < stage_1_end_time) {step(200);}
    // Turn off media
    while (millis() < stage_2_end_time) {step(200);}
    toggle_blast();
}

void rinse_state()
{
    Serial.print("Rinse State\n");
    // Turn on vacuum
    // Turn on air
    // Turn off media
    unsigned long start_time = millis();
    unsigned long end_time = start_time + 1000 * 60 * 5;
    while (millis() < end_time) {step(200);}
    toggle_rinse();
}

void setup()
{
    current_state = idle;
    interrupt_setup();
    stepper_setup();
    Serial.begin(9600);
}

void loop()
{
    switch (current_state) {
        case idle:
            idle_state();
            break;
        case manual:
            manual_state();
            break;
        case blast:
            blast_state();
            break;
        case rinse:
            rinse_state();
            break;
    }
    
}
