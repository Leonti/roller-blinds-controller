from app.motor import Motor
from app.store import Store

store_0 = Store(0)
store_1 = Store(1)
m0 = Motor(True, store_0, {
  'in1': 9,
  'in2': 8,
  'pwm': 10,
  'stby': 16,
  'enc2': 21,
  'enc1': 20,
})
m1 = Motor(True, store_1, {
  'in1': 14,
  'in2': 15,
  'pwm': 12,
  'stby': 16,
  'enc2': 19,
  'enc1': 18,
})