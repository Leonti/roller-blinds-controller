#ifndef MOTOR_H
#define MOTOR_H

#include "store.h"
#include "pio_rotary_encoder.h"

class Motor {
public:
  Motor(
    bool isLeft, 
    Store* store,
    RotaryEncoder* encoder,
    int in1,
    int in2,
    int pwm,
    int stby   
    );
  int32_t getPos();

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
  bool isGoalReached();
private:
  bool _isLeft;
  Store* _store;
  RotaryEncoder* _encoder;
  int _in1;
  int _in2;
  int _pwm;
  int _stby;
  int32_t _pos;
  int _last_updated_pos;

  volatile bool _going_down;
  uint32_t last_update_time_ms = 0;
  uint32_t last_position_store_update_time_ms = 0;
  uint32_t last_non_goal_update_ms = 0;
  uint32_t last_pos_update_time_ms = 0;
  bool is_setup;
  bool is_measuring_default_speed;

  uint32_t goal_reached_at = 0;
  bool goal_reached = true;
  int32_t setpoint;

  int last_position_update;
  float speed_previous_error;
  float PID_i;
  int pwm_speed = 0;
  float max_speed_abs;

  void resetPid();
  int32_t getSlowdownSteps();
  
  void ccw(float duty);
  void cw(float duty);
  void short_break();
  void setSpeed(float speed);
  void storeLastPositionUpdate();

  float calculateSlowdownSpeed(int32_t stepsLeft, int32_t slowdownSteps, float maxSpeed);
  void updatePidSpeed(long timeDiffMs, float actualSpeed, float desiredSpeed);
  void updatePos();
};
#endif