#include <AMIS30543.h>
#include <Servo.h>
#include <SPI.h>

#define MAN_BUTT 38
#define MAN_LED 39
#define BLAST_BUTT 40
#define BLAST_LED 41
#define RINSE_BUTT 42
#define RINSE_LED 43
#define SOL_A 44
#define SOL_B 45
#define MEDIA_SIG 46
#define VAC 47
#define STEP_NEXT 49
#define STEP_DIR 50
#define STEP_SLAVE 50

#define MEDIA_OPEN 20
#define MEDIA_CLOSED 90

enum states {
  idle,
  manual,
  blast,
  rinse
};

volatile states current_state;

Servo media_servo;
AMIS30543 basket_stepper;

void io_setup()
{
    pinMode(MAN_BUTT, INPUT_PULLUP);
    pinMode(MAN_LED, OUTPUT);
    pinMode(BLAST_BUTT, INPUT_PULLUP);
    pinMode(BLAST_LED, OUTPUT);
    pinMode(RINSE_BUTT, INPUT_PULLUP);
    pinMode(RINSE_LED, OUTPUT);
    pinMode(SOL_A, OUTPUT);
    pinMode(SOL_B, OUTPUT);
    pinMode(MEDIA_SIG, OUTPUT);
    pinMode(VAC, OUTPUT);
    pinMode(STEP_NEXT, OUTPUT);
    pinMode(STEP_DIR, OUTPUT);
    pinMode(STEP_SLAVE, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(MAN_BUTT), toggle_manual, RISING);
    attachInterrupt(digitalPinToInterrupt(BLAST_BUTT), toggle_blast, RISING);
    attachInterrupt(digitalPinToInterrupt(RINSE_BUTT), toggle_rinse, RISING);
}

void servo_setup() {
  media_servo.attach(MEDIA_SIG);
}

void stepper_setup()
{
    SPI.begin();
    basket_stepper.init(STEP_SLAVE);
    digitalWrite(STEP_NEXT, LOW);
    digitalWrite(STEP_DIR, LOW);
    delay(1);
    basket_stepper.resetSettings();
    basket_stepper.setCurrentMilliamps(2000);
    basket_stepper.setStepMode(0);
    basket_stepper.enableDriver();
    delayMicroseconds(1);
    digitalWrite(STEP_DIR, LOW);
    delayMicroseconds(1);
}

void step(int speed_delay)
{
    digitalWrite(STEP_NEXT, HIGH);
    delayMicroseconds(3);
    digitalWrite(STEP_NEXT, LOW);
    delayMicroseconds(3);
    delayMicroseconds(speed_delay);
}


void toggle_manual() {current_state == manual ? current_state = idle : current_state = manual;}

void toggle_blast() {current_state == blast ? current_state = idle : current_state = blast;}

void toggle_rinse() {current_state == rinse ? current_state = idle : current_state = rinse;}

void idle_state()
{
    Serial.print("Idle State\n");
    digitalWrite(MAN_LED, LOW);
    digitalWrite(BLAST_LED, LOW);
    digitalWrite(RINSE_LED, LOW);
    // Turn off vacuum
    digitalWrite(VAC, LOW);
    // Turn off air
    digitalWrite(SOL_A, LOW);
    digitalWrite(SOL_B, LOW);
    // Turn off media
    media_servo.write(MEDIA_CLOSED);
}

void manual_state()
{
    Serial.print("Manual State\n");
    digitalWrite(MAN_LED, HIGH);
    digitalWrite(BLAST_LED, LOW);
    digitalWrite(RINSE_LED, LOW);
    // Turn on vacuum
    digitalWrite(VAC, HIGH);
    // Turn off air
    digitalWrite(SOL_A, LOW);
    digitalWrite(SOL_B, LOW);
    // Turn off media
    media_servo.write(MEDIA_CLOSED);
}

void blast_state()
{
    Serial.print("Blast State\n");
    digitalWrite(MAN_LED, LOW);
    digitalWrite(BLAST_LED, HIGH);
    digitalWrite(RINSE_LED, LOW);
    // Turn on vacuum
    digitalWrite(VAC, HIGH);
    // Turn on air
    digitalWrite(SOL_A, LOW);
    digitalWrite(SOL_B, HIGH);
    // Turn on media
    media_servo.write(MEDIA_OPEN);
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
    digitalWrite(MAN_LED, LOW);
    digitalWrite(BLAST_LED, LOW);
    digitalWrite(RINSE_LED, HIGH);
    // Turn on vacuum
    digitalWrite(VAC, HIGH);
    // Turn on air
    digitalWrite(SOL_A, LOW);
    digitalWrite(SOL_B, HIGH);
    // Turn off media
    media_servo.write(MEDIA_CLOSED);
    unsigned long start_time = millis();
    unsigned long end_time = start_time + 1000 * 60 * 5;
    while (millis() < end_time) {step(200);}
    toggle_rinse();
}

void setup()
{
    current_state = blast;
    io_setup();
    servo_setup();
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
