#include <WString.h>
typedef unsigned long uint32_t;

class RemoteControlRoku
{
    public:
        // Button IR Codes
        static const uint32_t BTN_POWER = 1474553879;
        static const uint32_t BTN_BACK = 1474520729;
        static const uint32_t BTN_HOME = 1474543679;
        static const uint32_t BTN_UP = 1474533479;
        static const uint32_t BTN_DOWN = 1474546739;
        static const uint32_t BTN_LEFT = 1474525319;
        static const uint32_t BTN_RIGHT = 1474540619;
        static const uint32_t BTN_OK = 1474516139;
        static const uint32_t BTN_RETURN = 1474502369;
        static const uint32_t BTN_ASTERISK = 1474528889;
        static const uint32_t BTN_REWIND = 1474505939;
        static const uint32_t BTN_PLAYPAUSE = 1474507469;
        static const uint32_t BTN_FASTFORWARD = 1474538069;
        static const uint32_t BTN_MEDIA_0 = 1474513589;
        static const uint32_t BTN_MEDIA_1 = 1474548269;
        static const uint32_t BTN_MEDIA_2 = 1474559489;
        static const uint32_t BTN_MEDIA_3 = 1474498799;
        static const uint32_t BTN_VOL_UP = 1474555919;
        static const uint32_t BTN_VOL_DOWN = 1474496759;
        static const uint32_t BTN_MUTE = 1474495739;

        // Holding Button IR Codes
        static const uint32_t BTN_POWER_HOLD = 1474554134;
        static const uint32_t BTN_BACK_HOLD = 1474520984;
        static const uint32_t BTN_HOME_HOLD = 1474543934;
        static const uint32_t BTN_UP_HOLD = 1474533734;
        static const uint32_t BTN_DOWN_HOLD = 1474546994;
        static const uint32_t BTN_LEFT_HOLD = 1474525574;
        static const uint32_t BTN_RIGHT_HOLD = 1474540874;
        static const uint32_t BTN_OK_HOLD = 1474516394;
        static const uint32_t BTN_RETURN_HOLD = 1474502624;
        static const uint32_t BTN_ASTERISK_HOLD = 1474529144;
        static const uint32_t BTN_REWIND_HOLD = 1474506194;
        static const uint32_t BTN_PLAYPAUSE_HOLD = 1474507724;
        static const uint32_t BTN_FASTFORWARD_HOLD = 1474538324;
        static const uint32_t BTN_MEDIA_0_HOLD = 1474513844;
        static const uint32_t BTN_MEDIA_1_HOLD = 1474548524;
        static const uint32_t BTN_MEDIA_2_HOLD = 1474559744;
        static const uint32_t BTN_MEDIA_3_HOLD = 1474499054;
        static const uint32_t BTN_VOL_UP_HOLD = 1474556174;
        static const uint32_t BTN_VOL_DOWN_HOLD = 1474497014;
        static const uint32_t BTN_MUTE_HOLD = 1474495994;

        static String RemoteControlRoku::getBtnDescription(uint32_t btnCode) {
          switch(btnCode) {
              case RemoteControlRoku::BTN_ASTERISK:
                  return "Asterisk";
              case RemoteControlRoku::BTN_ASTERISK_HOLD:
                  return "Asterisk Hold";
              case RemoteControlRoku::BTN_BACK:
                  return "Back";
              case RemoteControlRoku::BTN_BACK_HOLD:
                  return "Back Hold";
              case RemoteControlRoku::BTN_DOWN:
                  return "Down";
              case RemoteControlRoku::BTN_DOWN_HOLD:
                  return "Down Hold";
              case RemoteControlRoku::BTN_FASTFORWARD:
                  return "Fast Forward";
              case RemoteControlRoku::BTN_FASTFORWARD_HOLD:
                  return "Fast Forward Hold";
              case RemoteControlRoku::BTN_HOME:
                  return "Home";
              case RemoteControlRoku::BTN_HOME_HOLD:
                  return "Home Hold";
              case RemoteControlRoku::BTN_LEFT:
                  return "Left";
              case RemoteControlRoku::BTN_LEFT_HOLD:
                  return "Left Hold";
              case RemoteControlRoku::BTN_MEDIA_0:
                  return "Media 0";
              case RemoteControlRoku::BTN_MEDIA_0_HOLD:
                  return "Media 0 Hold";
              case RemoteControlRoku::BTN_MEDIA_1:
                  return "Media 1";
              case RemoteControlRoku::BTN_MEDIA_1_HOLD:
                  return "Media 1 Hold";
              case RemoteControlRoku::BTN_MEDIA_2:
                  return "Media 2";
              case RemoteControlRoku::BTN_MEDIA_2_HOLD:
                  return "Media 2 Hold";
              case RemoteControlRoku::BTN_MEDIA_3:
                  return "Media 3";
              case RemoteControlRoku::BTN_MEDIA_3_HOLD:
                  return "Media 3 Hold";
              case RemoteControlRoku::BTN_MUTE:
                  return "Mute";
              case RemoteControlRoku::BTN_MUTE_HOLD:
                  return "Mute Hold";
              case RemoteControlRoku::BTN_OK:
                  return "OK";
              case RemoteControlRoku::BTN_OK_HOLD:
                  return "OK Hold";
              case RemoteControlRoku::BTN_PLAYPAUSE:
                  return "Play/Pause";
              case RemoteControlRoku::BTN_PLAYPAUSE_HOLD:
                  return "Play/Pause Hold";
              case RemoteControlRoku::BTN_POWER:
                  return "Power";
              case RemoteControlRoku::BTN_POWER_HOLD:
                  return "Power Hold";
              case RemoteControlRoku::BTN_RETURN:
                  return "Return";
              case RemoteControlRoku::BTN_RETURN_HOLD:
                  return "Return Hold";
              case RemoteControlRoku::BTN_REWIND:
                  return "Rewind";
              case RemoteControlRoku::BTN_REWIND_HOLD:
                  return "Rewind Hold";
              case RemoteControlRoku::BTN_RIGHT:
                  return "Right";
              case RemoteControlRoku::BTN_RIGHT_HOLD:
                  return "Right Hold";
              case RemoteControlRoku::BTN_UP:
                  return "Up";
              case RemoteControlRoku::BTN_UP_HOLD:
                  return "Up Hold";
              case RemoteControlRoku::BTN_VOL_DOWN:
                  return "Vol Down";
              case RemoteControlRoku::BTN_VOL_DOWN_HOLD:
                  return "Vol Down Hold";
              case RemoteControlRoku::BTN_VOL_UP:
                  return "Vol Up";
              case RemoteControlRoku::BTN_VOL_UP_HOLD:
                  return "Vol Up Hold";
          }
          return "";
      }
};
