#include "motor.h"

Motor::Motor(
  bool isLeft,
  Store* store,
  int in1,
  int in2,
  int pwm,
  int stby,
  int enc1,
  int enc2
) {
  _isLeft = isLeft;
  _store = store;
  pinMode(in1, OUTPUT);
  _in1 = in1;
  pinMode(in2, OUTPUT);
  _in2 = in2;

  analogWriteFreq(1000);
  analogWriteRange(65535);

  pinMode(stby, OUTPUT);
  digitalWrite(stby, LOW);
  _stby = stby;

  pinMode(enc1, INPUT);
  _enc1 = enc1;
  pinMode(enc2, INPUT);
  _enc2 = enc2;

  pinMode(pwm, OUTPUT);
  _pwm = pwm;

  setpoint = _pos;
  resetPid();
}

void Motor::begin() {
  _pos = _store->getLastPosition();
}

int Motor::getEnc1() {
  return _enc1;
}
void Motor::handleEnc1() {
  // debug only
  if (digitalRead(_enc1) != HIGH) {
    _false_triggers++;
  }

  if (digitalRead(_enc2) == HIGH) {
    // debug only
    if (!_going_down) {
      _direction_changes++;
    }
    _going_down = true;

    _pos = _isLeft ? _pos - 1 : _pos + 1;
  } else {
    // debug only
    if (_going_down) {
      _direction_changes++;
    }
    _going_down = false;

    _pos = _isLeft ? _pos + 1 : _pos - 1;
  }
}

int Motor::getPos() {
  return _pos;
}

int Motor::getFalseTriggers() {
  return _false_triggers;
}

int Motor::getDirectionChanges() {
  return _direction_changes;
}

void Motor::resetPid() {
  speed_previous_error = 0;
  PID_i = 0;
  last_update_time_ms = millis();
  last_non_goal_update_ms = millis();
  last_position_update = _pos;
}

int Motor::getSlowdownSteps() {
  int limit = _store->getBottomLimit();
  return (limit * _store->getSlowdownPercent()) / 100;
}

void Motor::goToPositionPercent(uint8_t positionPercent) {
  int limit = _store->getBottomLimit();
  setpoint = (limit * positionPercent) / 100;
  Serial.printf("Going to position percent %d, pos: %d, setpoint %d, limit %d\n",
    positionPercent,
    _pos,
    setpoint,
    limit
  );
  int stepsLeft = abs(setpoint - _pos);

  int slowdownSteps = getSlowdownSteps();
  if (stepsLeft <= slowdownSteps) {
    int approximatePwmSpeed = calculateSlowdownSpeed(stepsLeft, slowdownSteps, 100);
    pwm_speed = _pos < setpoint ? approximatePwmSpeed : -approximatePwmSpeed;
    max_speed = _pos < setpoint ? _store->getDefaultMaxSpeed() : -_store->getDefaultMaxSpeed();
  } else {
    max_speed = -1;
  }

  goal_reached_at = -1;
  goal_reached = false;
}

float Motor::calculateSlowdownSpeed(int stepsLeft, int slowdownSteps, float maxSpeed) {
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
  _direction_changes = 0;
  up(_store->getManualSpeedUp());
}

void Motor::stopAndSetTopLimit() {
  setSpeed(0);
  _pos = 0;
  _store->storeLastPosition(0);
  _direction_changes = 0;
}

void Motor::goSetupDown() {
  is_setup = true;
  _direction_changes = 0;
  down(_store->getManualSpeedDown());
}

void Motor::stopAndFinishSetup() {
  _store->storeBottomLimit(_pos);
  is_setup = false;
  is_measuring_default_speed = true;
  _direction_changes = 0;
  goToPositionPercent(50);
}

void Motor::storeLastPositionUpdate() {
  if (last_position_store_update_time_ms == -1) {
    last_position_store_update_time_ms = millis();
    return;
  }

  if (millis() - last_position_store_update_time_ms > 1000) {
    last_position_store_update_time_ms = millis();
    
    // TODO reenable later 
    _store->storeLastPosition(_pos);
  }
}

bool Motor::isGoalReached() {
  if (goal_reached) return true;

  float tolerance = _store->getBottomLimit() * 0.005;
  int bottomLimit = _store->getBottomLimit();
  // Serial.print("Tolerance: ");
  // Serial.print(tolerance, 2);
  // Serial.printf(", bottom limit: %d, diff: %d\n", bottomLimit, abs(_pos - setpoint));

  if (abs(_pos - setpoint) <= tolerance) {
    if (goal_reached_at != -1 && millis() - goal_reached_at > 1000) {
      stop();
      goal_reached = true;
      Serial.printf("GOAL stabilised, direction changes: %d", _direction_changes);
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
  //Serial.printf("pwm speed: %d", newSpeed);

  setSpeed(calculatedPwmSpeed);
}

void Motor::update() {
  storeLastPositionUpdate();
  if (is_setup) return;
  if (isGoalReached()) return;

  if (last_update_time_ms == -1) {
    last_update_time_ms = millis();
  }

  unsigned long time_diff_ms = millis() - last_update_time_ms;
  if (time_diff_ms < 40) return;

  last_update_time_ms = millis();

  int position_diff = _pos - last_position_update;

  last_position_update = _pos;

  // once moved 10% up
  if (is_measuring_default_speed && float(_pos) / _store->getBottomLimit() < 0.9) {
    is_measuring_default_speed = false;
    float actualSpeed = abs(float(position_diff) / time_diff_ms);
    Serial.println("Storing default max speed");
    _store->storeDefaultMaxSpeed(actualSpeed);
  }

  int steps_left = setpoint - _pos;

  if (abs(steps_left) <= getSlowdownSteps()) {
    float actualSpeed = float(position_diff) / time_diff_ms;
    
    // after switching from 100% speed to slowdown set max speed to the current one for smooth transition
    if (max_speed == -1) {
      max_speed = actualSpeed;
    }
    float desiredSpeed = calculateSlowdownSpeed(abs(steps_left), getSlowdownSteps(), max_speed);

    // Serial.print("Actual speed ");
    // Serial.print(actualSpeed, 2);
    //Serial.print(", Desired Speed ");
    //Serial.print(desiredSpeed, 2);
    // Serial.print(", Max speed ");
    // Serial.print(max_speed, 2);      
    //Serial.println("");
    updatePidSpeed(time_diff_ms, actualSpeed, desiredSpeed);
  } else {
    setSpeed(_pos < setpoint ? 100 : -100);
  }
}

int Motor::writeStatus(uint8_t* buffer) {
  buffer[0] = highByte(_pos);
  buffer[1] = lowByte(_pos);
  buffer[2] = highByte(setpoint);
  buffer[3] = lowByte(setpoint);
  buffer[4] = goal_reached;
  buffer[5] = highByte(_direction_changes);
  buffer[6] = lowByte(_direction_changes);
  buffer[7] = highByte(_false_triggers);
  buffer[8] = lowByte(_false_triggers);

  return 9;
}