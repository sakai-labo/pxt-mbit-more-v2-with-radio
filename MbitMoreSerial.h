#ifndef MBIT_MORE_SERIAL_H
#define MBIT_MORE_SERIAL_H

// #include "MicroBit.h"

#include "MbitMoreCommon.h"
#include "MbitMoreDevice.h"

#define MM_SFD 0xff

// // Forward declaration
class MbitMoreDevice;

/**
 * Class definition for main logics of Micribit More Service except bluetooth connectivity.
 *
 */
class MbitMoreSerial {
private:
  enum ChRequest
  {
    REQ_READ = 0x01,
    REQ_WRITE = 0x10,
    REQ_WRITE_RESPONSE = 0x11,
    REQ_NOTIFY_STOP = 0x20,
    REQ_NOTIFY_START = 0x21,
  };

  enum ChResponse
  {
    RES_READ = 0x01,
    RES_WRITE = 0x11,
    RES_NOTIFY = 0x21,
  };

  /**
   * @brief Wait until rxBufferedSize is over the length
   * 
   * @param length size to be exceed
   * @return true the length is exceeded
   * @return false timeout before the length exceeded
   */
  bool waitRxBufferedSize(size_t length);

public:
  /**
   * @brief Microbit More object.
   *
   */
  MbitMoreDevice &mbitMore;

  /**
   * @brief Construct a new Microbit More Serial service
   * 
   * @param _mbitMore An instance of Microbit More device controller
   */
  MbitMoreSerial(MbitMoreDevice &_mbitMore);

  void readResponseOnSerial(uint16_t ch, uint8_t *dataBuffer, size_t len);

  void writeResponseOnSerial(uint16_t ch, bool response);

  void notifyOnSerial(uint16_t ch, uint8_t *dataBuffer, size_t len);

  /**
   * @brief Start continuous receiving process from serial port.
   * 
   */
  void startSerialReceiving();

  /**
   * @brief Start continuous updating process to serial port.
   * 
   */
  void startSerialUpdating();
};
#endif // MBIT_MORE_SERIAL_H