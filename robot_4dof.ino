/*
 * robot_4dof.ino - Arduino code for the GearBest 4DOF robot (which does not seem to have a name)
 * 
 * MIT License
 *
 * Copyright (c) 2018 FrankBlabu
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Servo.h>

/*
 * Port definitions
 */
struct Port
{
  // Digital (PWM) output ports
  static const int servo_base = 5;
  static const int servo_arm1 = 9;
  static const int servo_arm2 = 10;
  static const int servo_hand = 3;

  // Analog input ports
  static const int joystick_1_h = A4;
  static const int joystick_1_v = A5;

  static const int joystick_2_h = A3;
  static const int joystick_2_v = A2;
};

/*
 * This class keeps the position of a single axis and updates it differentially
 * according to the associated controller position.
 */
class Axis
{
  public:
    Axis (const char* name);

    void setup (int joystick_port, int servo_port, int initial_value);
    void process ();

  private:
    const char* _name;
    Servo _servo;
    int _joystick_port;
    int _middle_pos;
    int _current_pos;
};

/*
 * Constuctor
 * 
 * @param name Printable axis name (for debugging purposes)
 */
Axis::Axis (const char* name)
: _name          (name),
  _servo         (),
  _joystick_port (-1),
  _middle_pos    (1024 / 2),
  _current_pos   (90)
{
}

/*
 * Setup axis during system setup
 * 
 * @param joystick_port Analog input port which controls the axis
 * @param servo_port    Digital (PWM) port the servo is connected to
 * @param initial_value Initial servo position value in degree (0, 180)
 */
void Axis::setup (int joystick_port, int servo_port, int initial_value)
{
  _joystick_port = joystick_port;
  _current_pos = initial_value;
  
  pinMode (joystick_port, INPUT);
  pinMode (servo_port, OUTPUT);
  
  _servo.attach (servo_port);  

  // Calibrate by declaring the current joystick position as null position
  _middle_pos = analogRead (joystick_port);
}

/*
 * Process single step
 */
void Axis::process ()
{
  // Map analog joystick value (0, 1024) into the range (-5, +5). That is the step width
  // in degree to be added to the current servo position.
  int step = (_middle_pos - analogRead (_joystick_port)) / 100;
  _current_pos += step;

  // Usually, servos are expecting the position to be in the range of 0 to 180 degree.
  if (_current_pos < 0)
    _current_pos = 0;
  if (_current_pos > 180)
    _current_pos = 180;

  // Print some debugging information
  Serial.print (_name);
  Serial.print (": ");
  Serial.print ("R ");
  Serial.print (analogRead (_joystick_port));
  Serial.print ("/S ");
  Serial.print (step);
  Serial.print ("/P ");
  Serial.print (_current_pos);
  
  _servo.write (_current_pos);
}


Axis base ("Base");
Axis arm1 ("Arm1");
Axis arm2 ("Arm2");
Axis hand ("Hand");


void setup ()
{
  // Axis setup. The initial axis values are heuristics which may differ from robot
  // to robot.
  base.setup (Port::joystick_2_h, Port::servo_base, 90);
  arm1.setup (Port::joystick_1_h, Port::servo_arm1, 20);
  arm2.setup (Port::joystick_1_v, Port::servo_arm2, 90);
  hand.setup (Port::joystick_2_v, Port::servo_hand, 100);

  Serial.begin (9600);
}


void loop ()
{
  // Loop and adapt all axes. We are using C++11 here (just because we can...).
  Axis* axis[] = {&base, &arm1, &arm2, &hand, nullptr};

  for (int i=0; axis[i] != nullptr; ++i)
  {
    Axis* a = axis[i];
    a->process ();

    Serial.print ("---");

    delay (15);
  }

  Serial.println ();
}
