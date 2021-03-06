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

#undef DEBUG_POSITION
//#define DEBUG_POSITION

/*
 * General configuration
 */
namespace Configuration
{
  // Maximum servo step width in degree if the joystick is pulled to max.
  static const int step_width = 5;

  // Servo delay in milliseconds between move actions
  static const int servo_delay = 30;

  // Assumed noise of joystick position in input steps (0, 1024)
  static const int joystick_noise = 10;
}

/*
 * Port definitions
 */
namespace Port
{  
  // Digital (PWM) output ports
  static const int servo_base = 3;
  static const int servo_arm1 = 5;
  static const int servo_arm2 = 6;
  static const int servo_hand = 9;

  // Analog input ports
  static const int joystick_1_h = A1;
  static const int joystick_1_v = A0;

  static const int joystick_2_h = A3;
  static const int joystick_2_v = A2;

  // Digital inputs
  static const int button_1 = 10;
  static const int button_2 = 11;

  // LEDs
  static const int led = LED_BUILTIN;
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
    enum class Direction { POS, NEG };
  
  public:
    Axis (const char* name, int joystick_port, int servo_port, int initial_pos, 
          int min_pos, int max_pos, Direction direction);

    void setup ();
    void calibrate ();
    void reset ();
    bool process ();

    void setLink (Axis* axis) { _link = axis; }

  protected:
    void move (int step);

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
    Direction _direction;

    Axis* _link;
};

/*
 * Constuctor
 * 
 * @param name          Printable axis name (for debugging purposes)
 * @param joystick_port Analog input port which controls the axis
 * @param servo_port    Digital (PWM) port the servo is connected to
 * @param initial_pos   Initial servo position value in degree (0, 180)
 * @param min_pos       Minimum possible servo position in degree
 * @param max_pos       Maximum possible servo position in degree
 * @param direction     Set to NEG to invert the joystick
 */
Axis::Axis (const char* name, int joystick_port, int servo_port, int initial_pos, 
            int min_pos, int max_pos, Direction direction)
: _name          (name),
  _servo         (),
  _joystick_port (joystick_port),
  _servo_port    (servo_port),
  _initial_pos   (initial_pos),
  _middle_pos    (1024 / 2),
  _current_pos   (initial_pos),
  _min_pos       (min_pos),
  _max_pos       (max_pos),
  _direction     (direction),
  _link          (nullptr)
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

  calibrate ();
  reset ();
}

/*
 * Calibrate joystick middle position
 */
 void Axis::calibrate ()
 {
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
bool Axis::process ()
{
  // Map analog joystick value (0, 1024) into the range (-step_width, +step_width). That is the step width
  // in degree to be added to the current servo position.
  int value = _middle_pos - analogRead (_joystick_port);
  
  int step = map (value, 
                  -512 + Configuration::joystick_noise, 512 - Configuration::joystick_noise,
                  -Configuration::step_width, +Configuration::step_width);

  if (abs (value) < Configuration::joystick_noise)
    step = 0;

  move (step);

  if (_link)
    _link->move (step);

  return step != 0;
}

/*
 * Move axis for a given step
 */
void Axis::move (int step)
{
  switch (_direction)
  {
    case Direction::POS:
      _current_pos += step;
      break;                
      
    case Direction::NEG:
      _current_pos -= step;
      break;                
  }
  
  _current_pos = constrain (_current_pos, _min_pos, _max_pos);

#ifdef DEBUG_POSITION
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
#endif
  
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
Axis base ("Base", Port::joystick_2_h, Port::servo_base,  90,  0, 180, Axis::Direction::POS);
Axis arm1 ("Arm1", Port::joystick_1_v, Port::servo_arm1,  20,  0,  90, Axis::Direction::POS);
Axis arm2 ("Arm2", Port::joystick_1_h, Port::servo_arm2,  90, 30, 180, Axis::Direction::NEG);
Axis hand ("Hand", Port::joystick_2_v, Port::servo_hand, 100, 80, 130, Axis::Direction::NEG);

Button button_calibrate (Port::button_1);
Button button_home (Port::button_2);

Axis* axes[] = {&base, &arm1, &arm2, &hand, nullptr};

void setup ()
{
  arm1.setLink (&arm2);
  
  for (int i=0; axes[i] != nullptr; ++i)
    axes[i]->setup ();

  // Digital input setup
  pinMode (Port::button_1, INPUT_PULLUP);
  pinMode (Port::button_2, INPUT_PULLUP);

  // LED port setup
  pinMode (Port::led, OUTPUT);
  digitalWrite (Port::led, LOW);

  Serial.begin (9600);
}


void loop ()
{
  if (button_calibrate.is_triggered ())
  {
#ifdef DEBUG_POSITION
    Serial.println ("* CALIBRATE *");
#endif
    
    for (int i=0; axes[i] != nullptr; ++i)
      axes[i]->calibrate ();
  }
  
  if (button_home.is_triggered ())
  {
#ifdef DEBUG_POSITION
    Serial.println ("* HOME *");
#endif
    
    for (int i=0; axes[i] != nullptr; ++i)
      axes[i]->reset ();
  }

  // Loop and adapt all axes
  bool moved = false;
  
  for (int i=0; axes[i] != nullptr; ++i)
    moved |= axes[i]->process ();

  digitalWrite (Port::led, moved);

#ifdef DEBUG_POSITION
  Serial.print (digitalRead (Port::button_1) == HIGH ? "high" : "low");
  Serial.print ("---");
  Serial.print (digitalRead (Port::button_2) == HIGH ? "high" : "low");
  Serial.print ("---");

  Serial.println ();
#endif
  
  delay (Configuration::servo_delay);
}
