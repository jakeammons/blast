#include <AMIS30543.h>
#include <Servo.h>
#include <SPI.h>

#define MAN_BUTT 19
#define MAN_LED 39
#define BLAST_BUTT 20
#define BLAST_LED 41
#define RINSE_BUTT 21
#define RINSE_LED 43
#define SOL_A 44
#define SOL_B 45
#define MEDIA_SIG 46
#define VAC 47
#define STEP_NEXT 49
#define STEP_DIR 50
#define STEP_SLAVE 53

#define MEDIA_OPEN 90
#define MEDIA_CLOSED 20

#define BASKET_SPEED 600

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
    attachInterrupt(digitalPinToInterrupt(MAN_BUTT), toggle_manual, LOW);
    attachInterrupt(digitalPinToInterrupt(BLAST_BUTT), toggle_blast, LOW);
    attachInterrupt(digitalPinToInterrupt(RINSE_BUTT), toggle_rinse, LOW);
}

void motor_setup()
{
    media_servo.attach(MEDIA_SIG);
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

void start_up()
{
    int delay_length = 250;
    for (int i = 0; i < 10; i++) {
        digitalWrite(MAN_LED, HIGH);
        delay(delay_length);
        digitalWrite(MAN_LED, LOW);
        digitalWrite(BLAST_LED, HIGH);
        delay(delay_length);
        digitalWrite(BLAST_LED, LOW);
        digitalWrite(RINSE_LED, HIGH);
        delay(delay_length);
        digitalWrite(RINSE_LED, LOW);
        digitalWrite(BLAST_LED, HIGH);
        delay(delay_length);
        digitalWrite(BLAST_LED, LOW);
    }
}

void step(int speed_delay)
{
    digitalWrite(STEP_NEXT, HIGH);
    delayMicroseconds(3);
    digitalWrite(STEP_NEXT, LOW);
    delayMicroseconds(3);
    delayMicroseconds(speed_delay);
}


void toggle_manual()
{
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 200) 
    {
        current_state == manual ? current_state = idle : current_state = manual;
    }
    last_interrupt_time = interrupt_time;
}

void toggle_blast()
{
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 200) 
    {
        current_state == blast ? current_state = idle : current_state = blast;
    }
    last_interrupt_time = interrupt_time;
}

void toggle_rinse()
{
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 200) 
    {
        current_state == rinse ? current_state = idle : current_state = rinse;
    }
    last_interrupt_time = interrupt_time;
}

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
    unsigned long stage_1_duration = 480000; // 8 minutes
    unsigned long stage_2_duration = 120000; // 2 minutes
    unsigned long stage_1_end_time = start_time + stage_1_duration;
    unsigned long stage_2_end_time = start_time + stage_1_duration + stage_2_duration;
    while (current_state == blast && millis() < stage_1_end_time) {step(BASKET_SPEED);}
    // Turn off media
    media_servo.write(MEDIA_CLOSED);
    while (current_state == blast && millis() < stage_2_end_time) {step(BASKET_SPEED);}
    if (current_state != blast) return;
    else toggle_blast();
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
    unsigned long duration = 300000; // 5 minutes
    unsigned long end_time = start_time + duration;
    while (current_state == rinse && millis() < end_time) {step(BASKET_SPEED);}
    if (current_state != rinse) return;
    else toggle_rinse();
}

void setup()
{
    current_state = idle;
    io_setup();
    motor_setup();
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
