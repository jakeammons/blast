#include <AMIS30543.h>
#include <Servo.h>
#include <SPI.h>

#define DOOR_INTERLOCK 3
#define WINDOW_INTERLOCK 18
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

#define MEDIA_OPEN 10
#define MEDIA_CLOSED 85

#define BASKET_SPEED 600

enum states {
    startup,
    idle,
    manual,
    blast,
    rinse
};

volatile states current_state;
volatile bool interlock_open;

Servo media_servo;
AMIS30543 basket_stepper;

void io_setup()
{
    pinMode(DOOR_INTERLOCK, INPUT_PULLUP);
    pinMode(WINDOW_INTERLOCK, INPUT_PULLUP);
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
    attachInterrupt(digitalPinToInterrupt(DOOR_INTERLOCK), interlock, CHANGE);
    attachInterrupt(digitalPinToInterrupt(WINDOW_INTERLOCK), interlock, CHANGE);
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

void interlock()
{
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 200) 
    {
        interlock_open = digitalRead(DOOR_INTERLOCK) | digitalRead(WINDOW_INTERLOCK);
    }
    last_interrupt_time = interrupt_time;
}

void toggle_startup()
{
    current_state == startup ? current_state = idle : current_state = startup;
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

void startup_state()
{
    Serial.print("Startup State\n");
    // Turn off vacuum
    digitalWrite(VAC, LOW);
    // Turn off air
    digitalWrite(SOL_A, LOW);
    digitalWrite(SOL_B, LOW);
    // Turn off media
    media_servo.write(MEDIA_CLOSED);
    int delay_length = 250;
    for (int i = 0; current_state == startup && i < 10; i++) {
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
    if (current_state != startup) return;
    else toggle_startup();
}

void idle_state()
{
    Serial.print("Idle State\n");
    digitalWrite(MAN_LED, LOW);
    digitalWrite(BLAST_LED, LOW);
    digitalWrite(RINSE_LED, LOW);
    digitalWrite(VAC, LOW);
    digitalWrite(SOL_A, LOW);
    digitalWrite(SOL_B, LOW);
    media_servo.write(MEDIA_CLOSED);
    basket_stepper.disableDriver();
}

void manual_state()
{
    Serial.print("Manual State\n");
    digitalWrite(MAN_LED, HIGH);
    digitalWrite(BLAST_LED, LOW);
    digitalWrite(RINSE_LED, LOW);
    interlock_open ? digitalWrite(VAC, LOW) : digitalWrite(VAC, HIGH);
    digitalWrite(SOL_A, LOW);
    digitalWrite(SOL_B, LOW);
    media_servo.write(MEDIA_CLOSED);
    basket_stepper.disableDriver();
}

void blast_state()
{
    Serial.print("Blast State\n");
    digitalWrite(MAN_LED, LOW);
    digitalWrite(BLAST_LED, HIGH);
    digitalWrite(RINSE_LED, LOW);
    media_servo.write(MEDIA_OPEN);
    basket_stepper.enableDriver();
    unsigned long start_time = millis();
    unsigned long stage_1_duration = 480000; // 8 minutes
    unsigned long stage_2_duration = 120000; // 2 minutes
    unsigned long stage_1_end_time = start_time + stage_1_duration;
    unsigned long stage_2_end_time = start_time + stage_1_duration + stage_2_duration;
    while (current_state == blast && millis() < stage_1_end_time) {
      digitalWrite(VAC, !interlock_open);
      digitalWrite(SOL_B, !interlock_open);
      if (!interlock_open) step(BASKET_SPEED);
    }
    media_servo.write(MEDIA_CLOSED);
    while (current_state == blast && millis() < stage_2_end_time) {
      digitalWrite(VAC, !interlock_open);
      digitalWrite(SOL_B, !interlock_open);
      if (!interlock_open) step(BASKET_SPEED);
    }
    if (current_state != blast) return;
    else toggle_blast();
}

void rinse_state()
{
    Serial.print("Rinse State\n");
    digitalWrite(MAN_LED, LOW);
    digitalWrite(BLAST_LED, LOW);
    digitalWrite(RINSE_LED, HIGH);
    media_servo.write(MEDIA_CLOSED);
    basket_stepper.enableDriver();
    unsigned long start_time = millis();
    unsigned long duration = 300000; // 5 minutes
    unsigned long end_time = start_time + duration;
    while (current_state == rinse && millis() < end_time) {
      digitalWrite(VAC, !interlock_open);
      digitalWrite(SOL_B, !interlock_open);
      if (!interlock_open) step(BASKET_SPEED);
    }
    if (current_state != rinse) return;
    else toggle_rinse();
}

void setup()
{
    current_state = startup;
    io_setup();
    motor_setup();
    interlock_open = digitalRead(DOOR_INTERLOCK) | digitalRead(WINDOW_INTERLOCK);
    Serial.begin(9600);
}

void loop()
{
    switch (current_state) {
        case startup:
            startup_state();
            break;
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
        default:
            idle_state();
            break;
    }
}
