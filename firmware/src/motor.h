#ifndef MOTOR_H
#define MOTOR_H

#include "store.h"

class Motor {
public:
  Motor(
    bool isLeft, 
    Store* store,
    int in1,
    int in2,
    int pwm,
    int stby,
    int enc1,
    int enc2
    );
  int getEnc1();
  void handleEnc1();
  int getPos();

  int getFalseTriggers();
  int getDirectionChanges();
  void goToPositionPercent(uint8_t positionPercent);
  void stop();
  void up(float speed);
  void down(float speed);
  void goSetupUp();
  void stopAndSetTopLimit();
  void goSetupDown();
  void stopAndFinishSetup();
  void update();
  int writeStatus(uint8_t* buffer);
  void begin();
private:
  bool _isLeft;
  Store* _store;
  int _in1;
  int _in2;
  int _pwm;
  int _stby;
  int _enc1;
  int _enc2;
  int _pos;
  int _false_triggers;
  int _direction_changes;
  bool _going_down;
  unsigned long last_update_time_ms = -1;
  unsigned long last_position_store_update_time_ms = -1;
  unsigned long last_non_goal_update_ms = -1;
  bool is_setup;
  bool is_measuring_default_speed;

  unsigned long goal_reached_at = -1;
  bool goal_reached = true;
  int setpoint;

  int last_position_update;
  float speed_previous_error;
  float PID_i;
  int pwm_speed = 0;
  float max_speed;

  void resetPid();
  int getSlowdownSteps();
  
  void ccw(float duty);
  void cw(float duty);
  void short_break();
  void setSpeed(float speed);
  void storeLastPositionUpdate();
  bool isGoalReached();
  float calculateSlowdownSpeed(int stepsLeft, int slowdownSteps, float maxSpeed);
  void updatePidSpeed(long timeDiffMs, float actualSpeed, float desiredSpeed);
};
#endif