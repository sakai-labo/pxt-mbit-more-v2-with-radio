#include "MbitMoreCommon.h"
#ifndef MBIT_MORE_RADIO_H
#define MBIT_MORE_RADIO_H

#include "MbitMoreDevice.h"

#include "pxt.h"

class MbitMoreDevice;


enum MbitMoreRadioPacketState
{
    NUM = 0x00,
    STRING_AND_NUMBER = 0x01,
    STRING  = 0x02,
    info = 0x03 , //not use
    DOUBLE = 0x04,
    value = 0x05
   

}; 

enum MbitMoreRadioControlCommand
{ SETGROUP = 0,
  SETSIGNALPOWER = 1,
  SENDSTRING = 2,
  SENDNUMBER = 3,
  SENDVALUE = 4,
  GETLASTPACKET = 5,
  GETLASTPACKETSIGNAL = 6

};

#define RADIOPACKETSIZE  32 

#define PACKETSTATEINFO  0



class  MbitMoreRadio {
    private:

    uint8_t a = 1;
 

   
public:

MbitMoreDevice &mbitMore;

 MbitMoreRadio(MbitMoreDevice &_mbitMore);
  uint8_t RECEIVEDLASTPACKET[RADIOPACKETSIZE] ;

  void Radiosetgroup(int group);

  void Radiosetsignalpower(int signalpower);

  void onRadioreceived(MicroBitEvent e );

  void sendrawpacket(uint8_t buf[],int len);

  ~MbitMoreRadio();

  

 



};

#endif