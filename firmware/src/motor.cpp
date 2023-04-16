#include "motor.h"
#include "pio_rotary_encoder.h"

Motor::Motor(
  bool isLeft,
  Store* store,
  RotaryEncoder* encoder,
  int in1,
  int in2,
  int pwm,
  int stby
) {
  _isLeft = isLeft;
  _store = store;
  _in1 = in1;
  _in2 = in2;
  _stby = stby;
  _encoder = encoder;
  _pwm = pwm;
}

void Motor::begin() {
  pinMode(_in1, OUTPUT);
  pinMode(_in2, OUTPUT);
  analogWriteFreq(1000);
  analogWriteRange(65535);

  pinMode(_stby, OUTPUT);
  digitalWrite(_stby, LOW);
  pinMode(_pwm, OUTPUT);

  _pos = _store->getLastPosition();
  setpoint = _pos;
  _encoder->set_rotation(0);
  resetPid();
}

int32_t Motor::getPos() {
  return _pos;
}

void Motor::resetPid() {
  speed_previous_error = 0;
  PID_i = 0;
  last_update_time_ms = millis();
  last_non_goal_update_ms = millis();
  last_position_update = getPos();
}

int32_t Motor::getSlowdownSteps() {
  int32_t limit = _store->getBottomLimit();
  float percent = float(_store->getSlowdownPercent()) / 100;
  return limit * percent;
}

void Motor::goToPositionPercent(uint8_t positionPercent) {
  int32_t limit = _store->getBottomLimit();
  float percent = positionPercent / 100.0;
  setpoint = limit * percent;
  Serial.printf("Going to position percent %d, pos: %d, setpoint %d, limit %d\n",
    positionPercent,
    getPos(),
    setpoint,
    limit
  );
  int stepsLeft = abs(setpoint - getPos());

  int slowdownSteps = getSlowdownSteps();
  if (stepsLeft <= slowdownSteps) {
    int approximatePwmSpeed = calculateSlowdownSpeed(stepsLeft, slowdownSteps, 100);
    pwm_speed = getPos() < setpoint ? approximatePwmSpeed : -approximatePwmSpeed;
    max_speed_abs = _store->getDefaultMaxSpeed();
  } else {
    max_speed_abs = -1;
  }

  goal_reached_at = -1;
  goal_reached = false;
}

float Motor::calculateSlowdownSpeed(int32_t stepsLeft, int32_t slowdownSteps, float maxSpeed) {
  if (stepsLeft > slowdownSteps) {
    return maxSpeed;
  }

  float leftRatio = float(stepsLeft) / slowdownSteps;
  return maxSpeed * leftRatio;
}

void Motor::ccw(float duty) {
  digitalWrite(_stby, HIGH);
  digitalWrite(_in1, HIGH);
  digitalWrite(_in2, LOW);
  int duty_16 = int((duty * 65536) / 100);
  analogWrite(_pwm, duty_16);
}
void Motor::cw(float duty) {
  digitalWrite(_stby, HIGH);
  digitalWrite(_in1, LOW);
  digitalWrite(_in2, HIGH);
  int duty_16 = int((duty * 65536) / 100);
  analogWrite(_pwm, duty_16);
}
void Motor::short_break() {
  digitalWrite(_stby, HIGH);
  digitalWrite(_in1, HIGH);
  digitalWrite(_in2, HIGH);
  analogWrite(_pwm, 65536);
}
void Motor::stop() {
  digitalWrite(_in1, LOW);
  digitalWrite(_in2, LOW);
  analogWrite(_pwm, 0);
}

void Motor::up(float speed) {
  if (!_isLeft) {
    ccw(speed);
  } else {
    cw(speed);
  }
}
void Motor::down(float speed) {
  if (!_isLeft) {
    cw(speed);
  } else {
    ccw(speed);
  }
}

void Motor::setSpeed(float speed) {
  pwm_speed = speed;

  if (speed == 0) {
    short_break();
  } else if (speed < 0) {
    up(abs(speed));
  } else if (speed > 0) {
    down(abs(speed));
  }
}

void Motor::goSetupUp() {
  is_setup = true;
  up(_store->getManualSpeedUp());
}

void Motor::stopAndSetTopLimit() {
  setSpeed(0);
  _pos = 0;
  _last_updated_pos = 0;
  _encoder->set_rotation(0);
  _store->storeLastPosition(0);
}

void Motor::goSetupDown() {
  is_setup = true;
  down(_store->getManualSpeedDown());
}

void Motor::stopAndFinishSetup() {
  _store->storeBottomLimit(getPos());
  is_setup = false;
  is_measuring_default_speed = true;
  goToPositionPercent(50);
}

void Motor::storeLastPositionUpdate() {
  if (last_position_store_update_time_ms == 0) {
    last_position_store_update_time_ms = millis();
    return;
  }

  if (millis() - last_position_store_update_time_ms > 1000) {
    last_position_store_update_time_ms = millis();
    _store->storeLastPosition(getPos());
  }
}

bool Motor::isGoalReached() {
  if (goal_reached) return true;

  float tolerance = _store->getBottomLimit() * 0.005;
  // int bottomLimit = _store->getBottomLimit();
  // Serial.print("Tolerance: ");
  // Serial.print(tolerance, 2);
  // Serial.printf(", bottom limit: %d, diff: %d\n", bottomLimit, abs(_pos - setpoint));

  // TODO reenable tolerance
  if (abs(_pos - setpoint) <= tolerance) {
  //if (abs(getPos() - setpoint) == 0) {
    if (goal_reached_at != -1 && millis() - goal_reached_at > 1000) {
      stop();
      goal_reached = true;
      Serial.printf("GOAL stabilised");
    } else {
      short_break();
      if (goal_reached_at == -1) {
        goal_reached_at = millis();
      }
    }

    return true;
  }

  return false;
}

void Motor::updatePidSpeed(long timeDiffMs, float actualSpeed, float desiredSpeed) {
  float speedError = desiredSpeed - actualSpeed;
  float PID_p = _store->getPidKp() * speedError;
  float PID_d = speed_previous_error != 0 ? _store->getPidKd() * (float(speedError - speed_previous_error) / timeDiffMs) : 0;
  PID_i = PID_i + (_store->getPidKi() * speedError);

  speed_previous_error = speedError;
  float PID_total = PID_p + PID_i + PID_d;

  float calculatedPwmSpeed;
  if (PID_total > 0) {
    float totalAdjustment = min(5, PID_total);
    calculatedPwmSpeed = min(100, pwm_speed + totalAdjustment);
  } else {
    float totalAdjustment = max(-5, PID_total);
    calculatedPwmSpeed = max(-100, pwm_speed + totalAdjustment);
  }

  // Serial.printf("%d,%d,", getPos(), setpoint);
  // Serial.print(desiredSpeed, 4);
  // Serial.print(",");
  // Serial.print(actualSpeed, 4);
  // Serial.print(",");
  // Serial.print(pwm_speed);
  // Serial.print(",");
  // Serial.print(calculatedPwmSpeed, 2);
  // Serial.print(",");
  // Serial.print(PID_total, 2);
  // Serial.print(",");
  // Serial.print(PID_p, 2);
  // Serial.print(",");
  // Serial.print(PID_i, 2);
  // Serial.print(",");
  // Serial.print(PID_d, 2);
  // Serial.println("");

  setSpeed(calculatedPwmSpeed);
}

void Motor::updatePos() {
  unsigned long time = millis();
  if (time - last_pos_update_time_ms > 5) {
    last_pos_update_time_ms = time;
    int current_pos = _isLeft ? -_encoder->get_rotation() : _encoder->get_rotation();
    int diff = current_pos - _last_updated_pos;
    _pos = _pos + diff;
    
    _last_updated_pos = current_pos;
  }
}

void Motor::update() {
  updatePos();
  storeLastPositionUpdate();
  if (is_setup) return;
  if (isGoalReached()) return;

  if (last_update_time_ms == 0) {
    last_update_time_ms = millis();
  }

  unsigned long time_diff_ms = millis() - last_update_time_ms;
  if (time_diff_ms < 40) return;

  last_update_time_ms = millis();

  int position_diff = getPos() - last_position_update;

  last_position_update = getPos();

  // once moved 10% up
  if (is_measuring_default_speed && float(getPos()) / _store->getBottomLimit() < 0.9) {
    is_measuring_default_speed = false;
    float actualSpeed = abs(float(position_diff) / time_diff_ms);
    Serial.println("Storing default max speed");
    _store->storeDefaultMaxSpeed(actualSpeed);
  }

  int steps_left = setpoint - getPos();

  if (abs(steps_left) <= getSlowdownSteps()) {
    float actualSpeed = float(position_diff) / time_diff_ms;
    
    // after switching from 100% speed to slowdown set max speed to the current one for smooth transition
    if (max_speed_abs == -1) {
      max_speed_abs = abs(actualSpeed);
    }
    float desiredSpeed = calculateSlowdownSpeed(abs(steps_left), getSlowdownSteps(), max_speed_abs);

    // Serial.print("Actual speed ");
    // Serial.print(actualSpeed, 2);
    //Serial.print(", Desired Speed ");
    //Serial.print(desiredSpeed, 2);
    // Serial.print(", Max speed ");
    // Serial.print(max_speed_abs, 2);      
    //Serial.println("");

    // getPos() < setpoint ? _store->getDefaultMaxSpeed() : -_store->getDefaultMaxSpeed();
    updatePidSpeed(time_diff_ms, actualSpeed, getPos() < setpoint ? desiredSpeed : -desiredSpeed);
  } else {
    setSpeed(getPos() < setpoint ? 100 : -100);
  }
}

int Motor::writeStatus(uint8_t* buffer) {
  int32_t pos = getPos();
  memcpy(&buffer[0], &pos, 4);
  memcpy(&buffer[4], &setpoint, 4);
  buffer[8] = goal_reached;
  return 9;
}