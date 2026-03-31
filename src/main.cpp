#include <Arduino.h>
#include <ESP32Servo.h>

const int SERVO_PIN = 1;   // 如果你接的是真 GPIO1，就保持 1
                           // 如果你接的是板子丝印 D1，需要改成对应引脚号

Servo myServo;

// 以 90° 为中位，在两边各摆 30°
const int CENTER_ANGLE = 90;
const int LEFT_ANGLE   = 60;
const int RIGHT_ANGLE  = 120;

void moveSmoothly(int fromAngle, int toAngle, int stepDelayMs) {
    if (fromAngle < toAngle) {
        for (int angle = fromAngle; angle <= toAngle; angle++) {
            myServo.write(angle);
            delay(stepDelayMs);
        }
    } else {
        for (int angle = fromAngle; angle >= toAngle; angle--) {
            myServo.write(angle);
            delay(stepDelayMs);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Servo test start");

    // 设置 PWM 范围，常见舵机通常可用
    myServo.setPeriodHertz(50);  // 舵机一般是 50Hz
    myServo.attach(SERVO_PIN, 500, 2400);

    // 先回中
    myServo.write(CENTER_ANGLE);
    delay(1000);
}

void loop() {
    Serial.println("Move to 60 deg");
    moveSmoothly(CENTER_ANGLE, LEFT_ANGLE, 15);
    delay(800);

    Serial.println("Move to 120 deg");
    moveSmoothly(LEFT_ANGLE, RIGHT_ANGLE, 15);
    delay(800);

    Serial.println("Back to center");
    moveSmoothly(RIGHT_ANGLE, CENTER_ANGLE, 15);
    delay(1000);
}