#ifndef RF_H
#define RF_H

class RF {
private:
  int     lastRfCode = 0;
  time_t  lastRfTimestamp = 0;

public:
  void start();
  void loop();
};

extern RF rf;

#endif /* RF_H */

