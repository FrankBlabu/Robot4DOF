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

//#*********************************************************************************
// Configuration
//#*********************************************************************************

/*
 * General configuration
 */
namespace Configuration
{
  // Servo step width
  static const int step_width = 5;
}

/*
 * Port definitions
 */
namespace Port
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

  // Digital inputs
  static const int button_1 = 13;
  static const int button_2 = 12;
};


//#*********************************************************************************
// CLASS Axis
//#*********************************************************************************

/*
 * This class keeps the position of a single axis and updates it differentially
 * according to the associated controller position.
 */
class Axis
{
  public:
    Axis (const char* name, int joystick_port, int servo_port, int initial_pos, int min_pos, int max_pos);

    void setup ();
    void reset ();
    void process ();

  private:
    const char* _name;
    Servo _servo;
    int _joystick_port;
    int _servo_port;
    int _initial_pos;
    int _middle_pos;
    int _current_pos;
    int _min_pos;
    int _max_pos;
};

/*
 * Constuctor
 * 
 * @param name          Printable axis name (for debugging purposes)
 * @param joystick_port Analog input port which controls the axis
 * @param servo_port    Digital (PWM) port the servo is connected to
 * @param initial_pos   Initial servo position value in degree (0, 180)
 */
Axis::Axis (const char* name, int joystick_port, int servo_port, int initial_pos, int min_pos, int max_pos)
: _name          (name),
  _servo         (),
  _joystick_port (joystick_port),
  _servo_port    (servo_port),
  _initial_pos   (initial_pos),
  _middle_pos    (1024 / 2),
  _current_pos   (initial_pos),
  _min_pos       (min_pos),
  _max_pos       (max_pos)
{
}

/*
 * Setup axis
 */
void Axis::setup ()
{
  pinMode (_joystick_port, INPUT);
  pinMode (_servo_port, OUTPUT);
  
  _servo.attach (_servo_port);  

  // Calibrate by declaring the current joystick position as null position
  _middle_pos = analogRead (_joystick_port);
}

/*
 * Reset axis to home position
 */
 void Axis::reset ()
 {
   _current_pos = _initial_pos;
   _servo.write (_current_pos);
 }

/*
 * Process single step
 */
void Axis::process ()
{
  // Map analog joystick value (0, 1024) into the range (-step_width, +step_width). That is the step width
  // in degree to be added to the current servo position.
  int step = map (_middle_pos - analogRead (_joystick_port), -512, 512, -Configuration::step_width, +Configuration::step_width);
  _current_pos += step;
  _current_pos = constrain (_current_pos, _min_pos, _max_pos);

  // Print some debugging information
  Serial.print (_name);
  Serial.print (": ");
  Serial.print ("R ");
  Serial.print (analogRead (_joystick_port));
  Serial.print ("/S ");
  Serial.print (step);
  Serial.print ("/P ");
  Serial.print (_current_pos);
  Serial.print ("---");
  
  _servo.write (_current_pos);
}


//#*********************************************************************************
// CLASS Button
//#*********************************************************************************

class Button
{
  public:
    Button (int port) : _port (port), _state (false) {}

    void setup ();
    bool is_triggered ();

  private:
    int _port;
    bool _state;
};


void Button::setup ()
{
  pinMode (_port, INPUT_PULLUP);  
}

bool Button::is_triggered ()
{
  bool state = digitalRead (_port) == LOW;
  bool triggered = state && !_state;
  _state = state;
  return triggered;
}


//#*********************************************************************************
// MAIN
//#*********************************************************************************

// Axis setup. The initial axis values are heuristics which may differ from robot
// to robot.
Axis base ("Base", Port::joystick_2_h, Port::servo_base,  90, 70, 140);
Axis arm1 ("Arm1", Port::joystick_1_h, Port::servo_arm1,  20,  0,  90);
Axis arm2 ("Arm2", Port::joystick_1_v, Port::servo_arm2,  90,  0, 180);
Axis hand ("Hand", Port::joystick_2_v, Port::servo_hand, 100, 80, 130);

Button button (Port::button_2);

Axis* axes[] = {&base, &arm1, &arm2, &hand, nullptr};

void setup ()
{
  for (int i=0; axes[i] != nullptr; ++i)
    axes[i]->setup ();

  // Digital input setup
  pinMode (Port::button_1, INPUT_PULLUP);
  pinMode (Port::button_2, INPUT_PULLUP);

  Serial.begin (9600);
}


void loop ()
{
  if (button.is_triggered ())
  {
    Serial.println ("* HOME *");
    
    for (int i=0; axes[i] != nullptr; ++i)
      axes[i]->reset ();
  }

  // Loop and adapt all axes
  for (int i=0; axes[i] != nullptr; ++i)
    axes[i]->process ();

  Serial.print (digitalRead (Port::button_1) == HIGH ? "high" : "low");
  Serial.print ("---");
  Serial.print (digitalRead (Port::button_2) == HIGH ? "high" : "low");
  Serial.print ("---");

  Serial.println ();
  delay (15);
}
