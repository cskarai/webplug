#include "RF.h"
#include <RCSwitch.h>
#include "WebPlug.h"

RF rf;

RCSwitch rfListener;

void RF::start() {
  rfListener.enableReceive(RF_RECEIVER);
}

void RF::loop() {
  if (rfListener.available()) {
    uint32_t value = rfListener.getReceivedValue();

    if( ( rfListener.getReceivedProtocol() == 1 ) && ( rfListener.getReceivedBitlength()  == 24 ) )
    {
      if( value == lastRfCode )
      {
        if( millis() - lastRfTimestamp < 500 )
        {
          lastRfTimestamp = millis();
          rfListener.resetAvailable();
          return;
        }
      }

      lastRfCode = value;
      lastRfTimestamp = millis();
      
      DBG("Radio received: %d\n", value);

      // TODO
    }

    rfListener.resetAvailable();
  }
}

