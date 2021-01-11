
#include "pxt.h"

/**
 * Class definition for the Scratch MicroBit More Service.
 * Provides a BLE service to remotely controll Micro:bit from Scratch3.
 */
#include "MbitMoreDevice.h"

/**
 * @brief Compute median value for the array.
 *
 * @param size Length of the array.
 * @param data Array to compute median.
 * @return int Median value.
 */
int median(int size, int *data) {
  int temp;
  int i, j;
  // the following two loops sort the array x in ascending order
  for (i = 0; i < size - 1; i++) {
    for (j = i + 1; j < size; j++) {
      if (data[j] < data[i]) {
        // swap elements
        temp = data[i];
        data[i] = data[j];
        data[j] = temp;
      }
    }
  }
  if (size % 2 == 0) {
    // if there is an even number of elements, return mean of the two elements
    // in the middle
    return ((data[size / 2] + data[size / 2 - 1]) / 2);
  } else {
    // else return the element in the middle
    return data[size / 2];
  }
}

/**
 * Position of data format in a value holder.
 */
#define MBIT_MORE_DATA_FORMAT_INDEX 19

#define MBIT_MORE_BUTTON_PIN_A 5
#define MBIT_MORE_BUTTON_PIN_B 11

/**
 * Constructor.
 * Create a representation of the device for Microbit More service.
 * @param _uBit The instance of a MicroBit runtime.
 */
MbitMoreDevice::MbitMoreDevice(MicroBit &_uBit) : uBit(_uBit) {
  // Reset compass
#if MICROBIT_CODAL
  // On microbit-v2, re-calibration destract compass heading.
#else // NOT MICROBIT_CODAL
  if (uBit.buttonA.isPressed()) {
    uBit.compass.clearCalibration();
  }
#endif // NOT MICROBIT_CODAL

  // Compass must be calibrated before starting bluetooth service.
  if (!uBit.compass.isCalibrated()) {
    uBit.compass.calibrate();
  }

  uBit.messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_EVT_ANY, this,
                         &MbitMoreDevice::onButtonChanged,
                         MESSAGE_BUS_LISTENER_IMMEDIATE);
  uBit.messageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_EVT_ANY, this,
                         &MbitMoreDevice::onButtonChanged,
                         MESSAGE_BUS_LISTENER_IMMEDIATE);
  uBit.messageBus.listen(MICROBIT_ID_GESTURE, MICROBIT_EVT_ANY, this,
                         &MbitMoreDevice::onGestureChanged,
                         MESSAGE_BUS_LISTENER_IMMEDIATE);
}

MbitMoreDevice::~MbitMoreDevice() {
  uBit.messageBus.ignore(MICROBIT_ID_BUTTON_A, MICROBIT_EVT_ANY, this,
                         &MbitMoreDevice::onButtonChanged);
  uBit.messageBus.ignore(MICROBIT_ID_BUTTON_B, MICROBIT_EVT_ANY, this,
                         &MbitMoreDevice::onButtonChanged);
  uBit.messageBus.ignore(MICROBIT_ID_GESTURE, MICROBIT_EVT_ANY, this,
                         &MbitMoreDevice::onGestureChanged);
  uBit.messageBus.ignore(MICROBIT_ID_ANY, MICROBIT_EVT_ANY, this,
                         &MbitMoreDevice::onPinEvent);
  delete basicService;
}

/**
 * @brief Set pin configuration for initial.
 *
 */
void MbitMoreDevice::initialConfiguration() {
  // P0,P1,P2 are pull-up as standerd extension.
  for (size_t i = 0; i < (sizeof(initialPullUp) / sizeof(initialPullUp[0]));
       i++) {
#if MICROBIT_CODAL
    setPullMode(initialPullUp[i], PullMode::Up);
#else // NOT MICROBIT_CODAL
    setPullMode(initialPullUp[i], PinMode::PullUp);
#endif // NOT MICROBIT_CODAL
  }
  uBit.display.stopAnimation(); // To stop display friendly name.
  uBit.display.print("M");
}

/**
 * @brief Set pin configuration for release.
 *
 */
void MbitMoreDevice::releaseConfiguration() {
  uBit.display.stopAnimation();
  // All GPIO is digital input pull-none mode.
#if MICROBIT_CODAL
  for (size_t i = 0; i < (sizeof(gpioPin) / sizeof(gpioPin[0])); i++) {
    setPullMode(gpioPin[i], PullMode::None);
  }
#else // NOT MICROBIT_CODAL
// Error 020 (no free memory)
// for (size_t i = 0; i < (sizeof(gpioPin) / sizeof(gpioPin[0])); i++) {
//   setPullMode(gpioPin[i], PinMode::PullNone);
// }
#endif // NOT MICROBIT_CODAL
}

/**
 * @brief Call when a command was received.
 *
 * @param data
 * @param length
 */
void MbitMoreDevice::onCommandReceived(uint8_t *data, size_t length) {
  const int command = (data[0] >> 5);
  if (command == MbitMoreCommand::CMD_DISPLAY) {
    const int displayCommand = data[0] & 0b11111;
    if (displayCommand == MbitMoreDisplayCommand::TEXT) {
      char text[length - 1] = {0};
      memcpy(text, &(data[2]), length - 2);
      displayText(text, (data[1] * 10));
    } else if (displayCommand == MbitMoreDisplayCommand::PIXELS_0) {
      setPixelsShadowLine(0, &data[1]);
      setPixelsShadowLine(1, &data[6]);
      setPixelsShadowLine(2, &data[11]);
    } else if (displayCommand == MbitMoreDisplayCommand::PIXELS_1) {
      setPixelsShadowLine(3, &data[1]);
      setPixelsShadowLine(4, &data[6]);
      displayShadowPixels();
    }
  } else if (command == MbitMoreCommand::CMD_PIN) {
    const int pinCommand = data[0] & 0b11111;
    if (pinCommand == MbitMorePinCommand::SET_PULL) {
#if MICROBIT_CODAL
      switch (data[2]) {
      case MbitMorePullMode::None:
        setPullMode(data[1], PullMode::None);
        break;
      case MbitMorePullMode::Up:
        setPullMode(data[1], PullMode::Up);
        break;
      case MbitMorePullMode::Down:
        setPullMode(data[1], PullMode::Down);
        break;

      default:
        break;
      }
#else // NOT MICROBIT_CODAL
      switch (data[2]) {
      case MbitMorePullMode::None:
        setPullMode(data[1], PinMode::PullNone);
        break;
      case MbitMorePullMode::Up:
        setPullMode(data[1], PinMode::PullUp);
        break;
      case MbitMorePullMode::Down:
        setPullMode(data[1], PinMode::PullDown);
        break;

      default:
        break;
      }
#endif // NOT MICROBIT_CODAL
    } else if (pinCommand == MbitMorePinCommand::SET_TOUCH) {
      setPinModeTouch(data[1]);
    } else if (pinCommand == MbitMorePinCommand::SET_OUTPUT) {
      setDigitalValue(data[1], data[2]);
    } else if (pinCommand == MbitMorePinCommand::SET_PWM) {
      // value is read as uint16_t little-endian.
      uint16_t value;
      memcpy(&value, &(data[2]), 2);
      setAnalogValue(data[1], value);
    } else if (pinCommand == MbitMorePinCommand::SET_SERVO) {
      int pinIndex = (int)data[1];
      // angle is read as uint16_t little-endian.
      uint16_t angle;
      memcpy(&angle, &(data[2]), 2);
      // range is read as uint16_t little-endian.
      uint16_t range;
      memcpy(&range, &(data[5]), 2);
      // center is read as uint16_t little-endian.
      uint16_t center;
      memcpy(&center, &(data[7]), 2);
      if (range == 0) {
        uBit.io.pin[pinIndex].setServoValue(angle);
      } else if (center == 0) {
        uBit.io.pin[pinIndex].setServoValue(angle, range);
      } else {
        uBit.io.pin[pinIndex].setServoValue(angle, range, center);
      }
    } else if (pinCommand == MbitMorePinCommand::SET_EVENT) {
      listenPinEventOn((int)data[2], (int)data[1]);
    }
  } else if (command == MbitMoreCommand::CMD_SHARED_DATA) {
    // value is read as int16_t little-endian.
    int16_t value;
    memcpy(&value, &(data[2]), 2);
    sharedData[data[1]] = value;
  }
}

/**
 * @brief Set the pattern on the line of the shadow pixels.
 *
 * @param line Index of the lines to set.
 * @param pattern Array of brightness(0..255) according columns.
 */
void MbitMoreDevice::setPixelsShadowLine(int line, uint8_t *pattern) {
  for (size_t col = 0; col < 5; col++) {
    shadowPixcels[line][col] = pattern[col];
  }
}

/**
 * @brief Display the shadow pixels on the LED.
 *
 */
void MbitMoreDevice::displayShadowPixels() {
  uBit.display.stopAnimation();
  for (size_t y = 0; y < 5; y++) {
    for (size_t x = 0; x < 5; x++) {
      uBit.display.image.setPixelValue(x, y, shadowPixcels[y][x]);
    }
  }
}

/**
 * Make it listen events of the event type on the pin.
 * Remove listener if the event type is MICROBIT_PIN_EVENT_NONE.
 */
void MbitMoreDevice::listenPinEventOn(int pinIndex, int eventType) {
  int componentID;  // ID of the MicroBit Component that generated the event.
  switch (pinIndex) // Index of pin to set event.
  {
  case 0:
    componentID = MICROBIT_ID_IO_P0;
    break;
  case 1:
    componentID = MICROBIT_ID_IO_P1;
    break;
  case 2:
    componentID = MICROBIT_ID_IO_P2;
    break;
  case 8:
    componentID = MICROBIT_ID_IO_P8;
    break;
  case 12:
    componentID = MICROBIT_ID_IO_P12;
    break;
  case 13:
    componentID = MICROBIT_ID_IO_P13;
    break;
  case 14:
    componentID = MICROBIT_ID_IO_P14;
    break;
  case 15:
    componentID = MICROBIT_ID_IO_P15;
    break;
  case 16:
    componentID = MICROBIT_ID_IO_P16;
    break;

  default:
    return;
  }
  if (eventType == MbitMorePinEventType::NONE) {
    uBit.messageBus.ignore(componentID, MICROBIT_EVT_ANY, this,
                           &MbitMoreDevice::onPinEvent);
    uBit.io.pin[pinIndex].eventOn(MICROBIT_PIN_EVENT_NONE);
  } else {
    uBit.messageBus.listen(componentID, MICROBIT_EVT_ANY, this,
                           &MbitMoreDevice::onPinEvent,
                           MESSAGE_BUS_LISTENER_DROP_IF_BUSY);
    if (eventType == MbitMorePinEventType::ON_EDGE) {
      uBit.io.pin[pinIndex].eventOn(MICROBIT_PIN_EVENT_ON_EDGE);
    } else if (eventType == MbitMorePinEventType::ON_PULSE) {
      uBit.io.pin[pinIndex].eventOn(MICROBIT_PIN_EVENT_ON_PULSE);
    } else if (eventType == MbitMorePinEventType::ON_TOUCH) {
      uBit.io.pin[pinIndex].eventOn(MICROBIT_PIN_EVENT_ON_TOUCH);
    }
  }
}

/**
 * Callback. Invoked when a pin event sent.
 */
void MbitMoreDevice::onPinEvent(MicroBitEvent evt) {
  uint8_t *data = moreService->pinEventChBuffer;
  // pinIndex is sent as uint8_t.
  switch (evt.source) // ID of the MicroBit Component (uint16_t).
  {
  case MICROBIT_ID_IO_P0:
    data[0] = 0;
    break;
  case MICROBIT_ID_IO_P1:
    data[0] = 1;
    break;
  case MICROBIT_ID_IO_P2:
    data[0] = 2;
    break;
  case MICROBIT_ID_IO_P8:
    data[0] = 8;
    break;
  case MICROBIT_ID_IO_P12:
    data[0] = 12;
    break;
  case MICROBIT_ID_IO_P13:
    data[0] = 13;
    break;
  case MICROBIT_ID_IO_P14:
    data[0] = 14;
    break;
  case MICROBIT_ID_IO_P15:
    data[0] = 15;
    break;
  case MICROBIT_ID_IO_P16:
    data[0] = 16;
    break;

  default:
    break;
  }

  // event ID is sent as uint8_t.
  switch (evt.value) {
  case MICROBIT_PIN_EVT_RISE:
    data[1] = MbitMorePinEvent::RISE;
    break;
  case MICROBIT_PIN_EVT_FALL:
    data[1] = MbitMorePinEvent::FALL;
    break;
  case MICROBIT_PIN_EVT_PULSE_HI:
    data[1] = MbitMorePinEvent::PULSE_HIGH;
    break;
  case MICROBIT_PIN_EVT_PULSE_LO:
    data[1] = MbitMorePinEvent::PULSE_LOW;
    break;

  default:
    break;
  }

  // event timestamp is sent as uint32_t little-endian
  // coerced from uint64_t value.
  uint32_t timestamp = (uint32_t)evt.timestamp;
  memcpy(&(data[2]), &timestamp, 4);
  data[MBIT_MORE_DATA_FORMAT_INDEX] = MbitMoreDataFormat::PIN_EVENT;
  moreService->notifyPinEvent();
}

/**
 * Button update callback
 */
void MbitMoreDevice::onButtonChanged(MicroBitEvent evt) {
  uint8_t *data = moreService->actionEventChBuffer;
  data[0] = MbitMoreActionEvent::BUTTON;
  // Component ID that generated the event.
  switch (evt.source) {
  case MICROBIT_ID_BUTTON_A:
    data[1] = MBIT_MORE_BUTTON_PIN_A;
    break;

  case MICROBIT_ID_BUTTON_B:
    data[1] = MBIT_MORE_BUTTON_PIN_B;
    break;

  default:
    data[1] = evt.source;
    break;
  }
  // Event ID.
  switch (evt.value) {
  case MICROBIT_BUTTON_EVT_DOWN:
    data[2] = MbitMoreButtonEvent::DOWN;
    break;

  case MICROBIT_BUTTON_EVT_UP:
    data[2] = MbitMoreButtonEvent::UP;
    break;

  case MICROBIT_BUTTON_EVT_CLICK:
    data[2] = MbitMoreButtonEvent::CLICK;
    break;

  case MICROBIT_BUTTON_EVT_LONG_CLICK:
    data[2] = MbitMoreButtonEvent::LONG_CLICK;
    break;

  case MICROBIT_BUTTON_EVT_HOLD:
    data[2] = MbitMoreButtonEvent::HOLD;
    break;

  case MICROBIT_BUTTON_EVT_DOUBLE_CLICK:
    data[2] = MbitMoreButtonEvent::DOUBLE_CLICK;
    break;

  default:
    data[2] = evt.value;
    break;
  }
  // Timestamp of the event.
  memcpy(&(data[3]), &evt.timestamp, 4);
  data[MBIT_MORE_DATA_FORMAT_INDEX] = MbitMoreDataFormat::ACTION_EVENT;
  moreService->notifyActionEvent();
}

void MbitMoreDevice::onGestureChanged(MicroBitEvent evt) {
  uint8_t *data = moreService->actionEventChBuffer;
  data[0] = MbitMoreActionEvent::GESTURE;
  switch (evt.value) {
  case MICROBIT_ACCELEROMETER_EVT_TILT_UP:
    data[1] = MbitMoreGestureEvent::TILT_UP;
    break;

  case MICROBIT_ACCELEROMETER_EVT_TILT_DOWN:
    data[1] = MbitMoreGestureEvent::TILT_DOWN;
    break;

  case MICROBIT_ACCELEROMETER_EVT_TILT_LEFT:
    data[1] = MbitMoreGestureEvent::TILT_LEFT;
    break;

  case MICROBIT_ACCELEROMETER_EVT_TILT_RIGHT:
    data[1] = MbitMoreGestureEvent::TILT_RIGHT;
    break;

  case MICROBIT_ACCELEROMETER_EVT_FACE_UP:
    data[1] = MbitMoreGestureEvent::FACE_UP;
    break;

  case MICROBIT_ACCELEROMETER_EVT_FACE_DOWN:
    data[1] = MbitMoreGestureEvent::FACE_DOWN;
    break;

  case MICROBIT_ACCELEROMETER_EVT_FREEFALL:
    data[1] = MbitMoreGestureEvent::FREEFALL;
    break;

  case MICROBIT_ACCELEROMETER_EVT_3G:
    data[1] = MbitMoreGestureEvent::G3;
    break;

  case MICROBIT_ACCELEROMETER_EVT_6G:
    data[1] = MbitMoreGestureEvent::G6;
    break;

  case MICROBIT_ACCELEROMETER_EVT_8G:
    data[1] = MbitMoreGestureEvent::G8;
    break;

  case MICROBIT_ACCELEROMETER_EVT_SHAKE:
    data[1] = MbitMoreGestureEvent::SHAKE;
    break;

  default:
    data[1] = evt.value;
    break;
  }
  // Timestamp of the event.
  memcpy(&(data[2]), &evt.timestamp, 4);
  data[MBIT_MORE_DATA_FORMAT_INDEX] = MbitMoreDataFormat::ACTION_EVENT;
  moreService->notifyActionEvent();
}

/**
 * Normalize angle in upside down.
 */
int MbitMoreDevice::normalizeCompassHeading(int heading) {
  if (uBit.accelerometer.getZ() > 0) {
    if (heading <= 180) {
      heading = 180 - heading;
    } else {
      heading = 360 - (heading - 180);
    }
  }
  return heading;
}

/**
 * Convert roll/pitch radians to Scratch extension value (-1000 to 1000).
 */
int MbitMoreDevice::convertToTilt(float radians) {
  float degrees = (360.0f * radians) / (2.0f * PI);
  float tilt = degrees * 1.0f / 90.0f;
  if (degrees > 0) {
    if (tilt > 1.0f)
      tilt = 2.0f - tilt;
  } else {
    if (tilt < -1.0f)
      tilt = -2.0f - tilt;
  }
  return (int)(tilt * 1000.0f);
}

#if MICROBIT_CODAL
void MbitMoreDevice::setPullMode(int pinIndex, PullMode pull) {
#else // NOT MICROBIT_CODAL
void MbitMoreDevice::setPullMode(int pinIndex, PinMode pull) {
#endif // NOT MICROBIT_CODAL
  uBit.io.pin[pinIndex].getDigitalValue(pull);
  pullMode[pinIndex] = pull;
}

void MbitMoreDevice::setDigitalValue(int pinIndex, int value) {
  uBit.io.pin[pinIndex].setDigitalValue(value);
}

void MbitMoreDevice::setAnalogValue(int pinIndex, int value) {
#if MICROBIT_CODAL
  // stable level is 0 .. 1022 in micro:bit v2,
  int validValue = value > 1022 ? 1022 : value;
#else // NOT MICROBIT_CODAL
  // stable level is 0 .. 1021 in micro:bit v1.5,
  int validValue = value > 1021 ? 1021 : value;
#endif // NOT MICROBIT_CODAL
  uBit.io.pin[pinIndex].setAnalogValue(validValue);
}

void MbitMoreDevice::setServoValue(int pinIndex, int angle, int range,
                                   int center) {
  uBit.io.pin[pinIndex].setServoValue(angle, range, center);
}

void MbitMoreDevice::setPinModeTouch(int pinIndex) {
  uBit.io.pin[pinIndex].isTouched(); // Configure to touch mode then the
                                     // return value is not used.
}

// /**
//  * Notify shared data to Scratch3
//  */
// void MbitMoreDevice::notifySharedData() {
// for (size_t i = 0; i < sizeof(sharedData) / sizeof(sharedData[0]); i++) {
//   memcpy(&(sharedBuffer[(i * 2)]), &sharedData[i], 2);
// }
// moreService->notifySharedData(
//     (uint8_t *)&sharedBuffer,
//     sizeof(sharedBuffer) / sizeof(sharedBuffer[0]));
// }

/**
 * @brief Display text on LED.
 *
 * @param text Contents to display with null termination.
 * @param delay The time to delay between characters, in milliseconds.
 */
void MbitMoreDevice::displayText(char *text, int delay) {
  ManagedString mstr(text);
  uBit.display.stopAnimation();
  // Interval=120 is corresponding with the standard extension.
  uBit.display.scrollAsync(mstr, delay);
}

/**
 * @brief Get data of the sensors.
 *
 * @param data Buffer for BLE characteristics.
 */
void MbitMoreDevice::updateSensors(uint8_t *data) {
  uint32_t digitalLevels = 0;
  for (size_t i = 0; i < sizeof(gpioPin) / sizeof(gpioPin[0]); i++) {
    if (uBit.io.pin[gpioPin[i]].isDigital()) {
      if (uBit.io.pin[gpioPin[i]].isInput()) {
        digitalLevels =
            digitalLevels |
            (uBit.io.pin[gpioPin[i]].getDigitalValue() << gpioPin[i]);
      }
    }
  }
  // DAL can not read the buttons state as digital input. (CODAL can do that)
#if MICROBIT_CODAL
  digitalLevels =
      digitalLevels | (uBit.io.pin[MBIT_MORE_BUTTON_PIN_A].getDigitalValue()
                       << MBIT_MORE_BUTTON_PIN_A);
  digitalLevels =
      digitalLevels | (uBit.io.pin[MBIT_MORE_BUTTON_PIN_B].getDigitalValue()
                       << MBIT_MORE_BUTTON_PIN_B);
#else // NOT MICROBIT_CODAL
  digitalLevels =
      digitalLevels | (!uBit.buttonA.isPressed() << MBIT_MORE_BUTTON_PIN_A);
  digitalLevels =
      digitalLevels | (!uBit.buttonB.isPressed() << MBIT_MORE_BUTTON_PIN_B);
#endif // NOT MICROBIT_CODAL
  memcpy(data, (uint8_t *)&digitalLevels, 4);
  data[4] = sampleLigthLevel();
  data[5] = (uint8_t)(uBit.thermometer.getTemperature() + 128);
#if MICROBIT_CODAL
  // data[6] = uBit.microphone.soundLevel(); // This is not implemented yet.
#endif // MICROBIT_CODAL
}

/**
 * @brief Update data of direction.
 *
 * @param data Buffer for BLE characteristics.
 */
void MbitMoreDevice::updateDirection(uint8_t *data) {
  // Accelerometer
  int16_t rot;
  // Pitch (radians / 1000) is sent as int16_t little-endian [0..1].
  rot = (int16_t)(uBit.accelerometer.getPitchRadians() * 1000);
  memcpy(&(data[0]), &rot, 2);
  // Roll (radians / 1000) is sent as int16_t little-endian [2..3].
  rot = (int16_t)(uBit.accelerometer.getRollRadians() * 1000);
  memcpy(&(data[2]), &rot, 2);

  int16_t acc;
  // Acceleration X [milli-g] is sent as int16_t little-endian [4..5].
  acc = (int16_t)-uBit.accelerometer.getX(); // Face side is positive in Z-axis.
  memcpy(&(data[4]), &acc, 2);
  // Acceleration Y [milli-g] is sent as int16_t little-endian [6..7].
  acc = (int16_t)uBit.accelerometer.getY();
  memcpy(&(data[6]), &acc, 2);
  // Acceleration Z [milli-g] is sent as int16_t little-endian [8..9].
  acc = (int16_t)-uBit.accelerometer.getZ(); // Face side is positive in Z-axis.
  memcpy(&(data[8]), &acc, 2);

  // Magnetometer
  // Compass Heading is sent as uint16_t little-endian [10..11]
  uint16_t heading = (uint16_t)normalizeCompassHeading(uBit.compass.heading());
  memcpy(&(data[10]), &heading, 2);

  int16_t force;
  // Magnetic force X (micro-teslas) is sent as int16_t little-endian[12..13].
  force = (int16_t)(uBit.compass.getX() / 1000);
  memcpy(&(data[12]), &force, 2);
  // Magnetic force Y (micro-teslas) is sent as int16_t little-endian[14..15].
  force = (int16_t)(uBit.compass.getY() / 1000);
  memcpy(&(data[14]), &force, 2);
  // Magnetic force Z (micro-teslas) is sent as int16_t little-endian[16..17].
  force = (int16_t)(uBit.compass.getZ() / 1000);
  memcpy(&(data[16]), &force, 2);
}

/**
 * @brief Get data of analog input of the pin.
 *
 * @param data Buffer for BLE characteristics.
 * @param pinIndex Index of the pin [0, 1, 2].
 */
void MbitMoreDevice::updateAnalogIn(uint8_t *data, size_t pinIndex) {
  if (uBit.io.pin[pinIndex].isInput()) {
#if MICROBIT_CODAL
    uBit.io.pin[pinIndex].setPull(PullMode::None);
    // value = (uint16_t)uBit.io.pin[pinIndex].getAnalogValue();
#else // NOT MICROBIT_CODAL
    uBit.io.pin[pinIndex].setPull(PinMode::PullNone);
#endif // NOT MICROBIT_CODAL

    // median filter
    for (size_t i = 0; i < ANALOG_IN_SAMPLES_SIZE; i++) {
      analogInSamples[pinIndex][i] = uBit.io.pin[pinIndex].getAnalogValue();
    }
    uint16_t value = median(ANALOG_IN_SAMPLES_SIZE, analogInSamples[pinIndex]);

    // analog value (0 to 1023) is sent as uint16_t little-endian.
    memcpy(&(data[0]), &value, 2);
    setPullMode(pinIndex, pullMode[pinIndex]);
  }
}

/**
 * @brief Sample current light level and return median.
 *
 * @return int Median filterd light level.
 */
int MbitMoreDevice::sampleLigthLevel() {
  lightLevelSamples[lightLevelSamplesLast] = uBit.display.readLightLevel();
  lightLevelSamplesLast++;
  if (lightLevelSamplesLast == LIGHT_LEVEL_SAMPLES_SIZE) {
    lightLevelSamplesLast = 0;
  }
  return median(LIGHT_LEVEL_SAMPLES_SIZE, lightLevelSamples);
}

/**
 * Set value to shared data.
 * shared data (0, 1, 2, 3)
 */
void MbitMoreDevice::setSharedData(int index, int value) {
  // value (-32768 to 32767) is sent as int16_t little-endian.
  int16_t data = (int16_t)value;
  sharedData[index] = data;
  // notifySharedData();
}

/**
 * Get value of a shared data.
 * shared data (0, 1, 2, 3)
 */
int MbitMoreDevice::getSharedData(int index) {
  return (int)(sharedData[index]);
}

// /**
//  * Write shared data characteristics.
//  */
// void MbitMoreDevice::writeSharedData() {
// for (size_t i = 0; i < sizeof(sharedData) / sizeof(sharedData[0]); i++) {
//   memcpy(&(sharedBuffer[(i * 2)]), &sharedData[i], 2);
// }

// moreService->writeSharedData((uint8_t *)&sharedBuffer,
//                              sizeof(sharedBuffer) /
//                              sizeof(sharedBuffer[0]));
// }

void MbitMoreDevice::displayFriendlyName() {
  ManagedString version(" -M 0.6.0- ");
  uBit.display.scrollAsync(ManagedString(microbit_friendly_name()) + version,
                           120);
}
