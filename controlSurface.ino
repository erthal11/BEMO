#include <CapacitiveSensor.h>
#include "MIDIUSB.h"
#include "avr/interrupt.h"
#include <RotaryEncoder.h>

const int ROTARY_INPUT_PIN = A5;
const int BUTTON_INPUT_PIN = A2;
const int FADER_INPUT_PIN = A0;

const int ENCA = 11;
const int ENCB = 12;

#define BUTTON_UP 9
#define BUTTON_LEFT 4
#define BUTTON_DOWN 5
#define BUTTON_RIGHT 13
#define BUTTON_IN 10

const int MOTOR_UP = 7;
const int MOTOR_DOWN = 8;
const int PWM = 6;

bool touchingFader = false;
const int touchSend    = 3;
const int touchReceive = 2;
CapacitiveSensor touchLine = CapacitiveSensor(touchSend, touchReceive);

int lastRotaryValue = 0;
int lastButtonValue = 0;

bool logicIsPlaying = false;
int faderLogicValue;
int faderValue;

midiEventPacket_t rx;

int ppqn = 0;
bool timerRunning = false;

//for encoder logic
int val1 = 0, val2 = 0;
int oldVal1 = 0, oldVal2 = 0;
int pos = 0, oldPos = 0;
int turn = 0, oldTurn = 0, turnCount = 0;

void setup() {
  Serial.println("help");

  Serial.begin(115200);

  pinMode(MOTOR_UP, OUTPUT);
  pinMode(MOTOR_DOWN, OUTPUT);
  pinMode(PWM, OUTPUT);

  pinMode(FADER_INPUT_PIN, INPUT);

  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_IN, INPUT_PULLUP);

  pinMode(ENCA, INPUT);
  digitalWrite(ENCA, HIGH);
  pinMode(ENCB, INPUT);
  digitalWrite(ENCB, HIGH);

  readFaderValue();
  readLogicsInfo();

}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void rotButtons() {
  if (!timerRunning) {

    if (!digitalRead(BUTTON_UP)) {
      Serial.println("Button Up");
//      controlChange(0, 102, 1);
//      MidiUSB.flush();
      timerRunning = true;
    }
    if (! digitalRead(BUTTON_LEFT)) {
      Serial.println("Button Left");
//      controlChange(0, 103, 1);
//      MidiUSB.flush();
      timerRunning = true;
    }
    if (! digitalRead(BUTTON_DOWN)) {
      Serial.println("Button Down");
//      controlChange(0, 104, 1);
//      MidiUSB.flush();
      timerRunning = true;
    }
    if (! digitalRead(BUTTON_RIGHT)) {
      Serial.println("Button Right");
//      controlChange(0, 105, 1);
//      MidiUSB.flush();
      timerRunning = true;
    }
    if (! digitalRead(BUTTON_IN)) {
      Serial.println("Button In");
//      controlChange(0, 106, 1);
//      MidiUSB.flush();
      timerRunning = true;
    }

  }
  if (timerRunning) ppqn++;
  if (ppqn == 300) {
    ppqn = 0;
    timerRunning = false;
  }
}


void rotEncoder() {

  val1 = digitalRead(ENCA);
  val2 = digitalRead(ENCB);
  // Detect changes
  if ( val1 != oldVal1 || val2 != oldVal2) {
    //for each pair there's a position out of four
    if      ( val1 == 1 && val2 == 1 ) // stationary position
      pos = 0;
    else if ( val1 == 0 && val2 == 1 )
      pos = 1;
    else if ( val1 == 0 && val2 == 0 )
      pos = 2;
    else if ( val1 == 1 && val2 == 0 )
      pos = 3;

    turn = pos - oldPos;

    if (abs(turn) != 2) // impossible to understand where it's turning otherwise.
      if (turn == -1 || turn == 3)
        turnCount++;
      else if (turn == 1 || turn == -3)
        turnCount--;

    if (pos == 0 || pos == 2) { // only assume a complete step on stationary position
      if (turnCount > 0){
        Serial.print("<");
//        controlChange(0, 106, 1);
//        MidiUSB.flush();
      }
      else if (turnCount < 0){
        Serial.print(">");
//        controlChange(0, 107, 1);
//        MidiUSB.flush();
      }
      turnCount = 0;
    }

    oldVal1 = val1;
    oldVal2 = val2;
    oldPos  = pos;
    oldTurn = turn;
  }
}


void readFaderValue() {
  faderValue = map(analogRead(FADER_INPUT_PIN), 0, 1023, 0, 127);
}

void checkTouch() {
  touchingFader = touchLine.capacitiveSensor(30) > 200; //700 is arbitrary and may need to be changed depending on the fader cap used (if any).
  //Serial.println(touchingFader);
}


void updateFader(int position) {
  int diff = abs(position - faderValue);
  diff = map(diff, 0, 127, 100, 255);
  analogWrite(PWM, diff);

  while (position > faderValue) {
    digitalWrite(MOTOR_UP, HIGH);
    readFaderValue();
  }
  digitalWrite(MOTOR_UP, LOW);

  while (position < faderValue) {
    digitalWrite(MOTOR_DOWN, HIGH);
    readFaderValue();
  }
  digitalWrite(MOTOR_DOWN, LOW);
}

void readLogicsInfo() {

  do {
    rx = MidiUSB.read();
    if (rx.header != 0) {
      if (rx.header == 11 && rx.byte1 == 176 && rx.byte2 == 7) {
        faderLogicValue = rx.byte3;
      }
      if (rx.header == 11 && rx.byte1 == 176 && rx.byte2 == 14 && rx.byte3 != 0)
      {
        logicIsPlaying = true;
      }
      else {
        logicIsPlaying = false;
      }
      //      Serial.print("Received: ");
      //      Serial.print(rx.header);
      //      Serial.print("-");
      //      Serial.print(rx.byte1);
      //      Serial.print("-");
      //      Serial.print(rx.byte2);
      //      Serial.print("-");
      //      Serial.println(rx.byte3);
    }
  } while (rx.header != 0);

}

void sendFaderValueToLogic() {
  controlChange(0, 7, faderValue);
  MidiUSB.flush();
}


void loop() {

  readFaderValue();

  readLogicsInfo();

  checkTouch();

  if (!touchingFader && faderLogicValue != faderValue)
    updateFader(faderLogicValue);

  if (touchingFader)
    sendFaderValueToLogic();

  rotEncoder();

  rotButtons();


  //  //eihter 0 or more
  //  int buttonAnalogValue = analogRead(BUTTON_INPUT_PIN);
  //  if (buttonAnalogValue > 0) buttonAnalogValue = 1;
  //
  //  int sliderAnalogValue = analogRead(SLIDER_INPUT_PIN);
  //  int sliderVoltage = map(sliderAnalogValue, 0, 696, 0, 127);
  //
  //  int rotaryAnalogValue = analogRead(ROTARY_INPUT_PIN);
  //  int rotaryVoltage = map(rotaryAnalogValue, 0, 696, 0, 127);
  //
  //
  //  if (buttonAnalogValue != lastButtonValue) {
  //    controlChange(0, 14, 1);
  //    MidiUSB.flush();
  //    lastButtonValue = buttonAnalogValue;
  //    //logicIsPlaying = !logicIsPlaying;
  //  }



}
