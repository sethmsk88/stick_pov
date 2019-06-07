#include <WString.h> // to be allowed to use .ino String class in a .cpp file
typedef unsigned int uint16_t;
typedef bool boolean;

class RemoteControlRoku
{
    public:
        // Button IR Codes
        static const uint16_t BTN_POWER = 59415;
        static const uint16_t BTN_BACK = 26265;
        static const uint16_t BTN_HOME = 49215;
        static const uint16_t BTN_UP = 39015;
        static const uint16_t BTN_DOWN = 52275;
        static const uint16_t BTN_LEFT = 30855;
        static const uint16_t BTN_RIGHT = 46155;
        static const uint16_t BTN_OK = 21675;
        static const uint16_t BTN_RETURN = 7905;
        static const uint16_t BTN_ASTERISK = 34425;
        static const uint16_t BTN_REWIND = 11475;
        static const uint16_t BTN_PLAYPAUSE = 13005;
        static const uint16_t BTN_FASTFORWARD = 43605;
        static const uint16_t BTN_MEDIA_0 = 19125;
        static const uint16_t BTN_MEDIA_1 = 64195;
        static const uint16_t BTN_MEDIA_2 = 65025;
        static const uint16_t BTN_MEDIA_3 = 4335;
        static const uint16_t BTN_VOL_UP = 61455;
        static const uint16_t BTN_VOL_DOWN = 2295;
        static const uint16_t BTN_MUTE = 1275;

        // Holding Button IR Codes
        static const uint16_t BTN_POWER_HOLD = 59670;
        static const uint16_t BTN_BACK_HOLD = 26520;
        static const uint16_t BTN_HOME_HOLD = 49470;
        static const uint16_t BTN_UP_HOLD = 39270;
        static const uint16_t BTN_DOWN_HOLD = 52530;
        static const uint16_t BTN_LEFT_HOLD = 31110;
        static const uint16_t BTN_RIGHT_HOLD = 46410;
        static const uint16_t BTN_OK_HOLD = 21930;
        static const uint16_t BTN_RETURN_HOLD = 8160;
        static const uint16_t BTN_ASTERISK_HOLD = 34680;
        static const uint16_t BTN_REWIND_HOLD = 11730;
        static const uint16_t BTN_PLAYPAUSE_HOLD = 13260;
        static const uint16_t BTN_FASTFORWARD_HOLD = 43860;
        static const uint16_t BTN_MEDIA_0_HOLD = 19380;
        static const uint16_t BTN_MEDIA_1_HOLD = 54060;
        static const uint16_t BTN_MEDIA_2_HOLD = 65280;
        static const uint16_t BTN_MEDIA_3_HOLD = 4590;
        static const uint16_t BTN_VOL_UP_HOLD = 61710;
        static const uint16_t BTN_VOL_DOWN_HOLD = 2550;
        static const uint16_t BTN_MUTE_HOLD = 1530;

        static String RemoteControlRoku::getBtnDescription(uint16_t btnCode) {
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

      static boolean RemoteControlRoku::isButtonPress(uint16_t btnCode) {
        switch (btnCode) {
            case RemoteControlRoku::BTN_ASTERISK:
            case RemoteControlRoku::BTN_BACK:
            case RemoteControlRoku::BTN_DOWN:
            case RemoteControlRoku::BTN_FASTFORWARD:
            case RemoteControlRoku::BTN_HOME:
            case RemoteControlRoku::BTN_LEFT:
            case RemoteControlRoku::BTN_MEDIA_0:
            case RemoteControlRoku::BTN_MEDIA_1:
            case RemoteControlRoku::BTN_MEDIA_2:
            case RemoteControlRoku::BTN_MEDIA_3:
            case RemoteControlRoku::BTN_MUTE:
            case RemoteControlRoku::BTN_OK:
            case RemoteControlRoku::BTN_PLAYPAUSE:
            case RemoteControlRoku::BTN_POWER:
            case RemoteControlRoku::BTN_RETURN:
            case RemoteControlRoku::BTN_REWIND:
            case RemoteControlRoku::BTN_RIGHT:
            case RemoteControlRoku::BTN_UP:
            case RemoteControlRoku::BTN_VOL_DOWN:
            case RemoteControlRoku::BTN_VOL_UP:
                return true;
                break;
            
            default:
                return false;
                break;
        }
    }

    static boolean RemoteControlRoku::isButtonHold(uint16_t btnCode) {
        switch (btnCode) {
            case RemoteControlRoku::BTN_ASTERISK_HOLD:
            case RemoteControlRoku::BTN_BACK_HOLD:
            case RemoteControlRoku::BTN_DOWN_HOLD:
            case RemoteControlRoku::BTN_FASTFORWARD_HOLD:
            case RemoteControlRoku::BTN_HOME_HOLD:
            case RemoteControlRoku::BTN_LEFT_HOLD:
            case RemoteControlRoku::BTN_MEDIA_0_HOLD:
            case RemoteControlRoku::BTN_MEDIA_1_HOLD:
            case RemoteControlRoku::BTN_MEDIA_2_HOLD:
            case RemoteControlRoku::BTN_MEDIA_3_HOLD:
            case RemoteControlRoku::BTN_MUTE_HOLD:
            case RemoteControlRoku::BTN_OK_HOLD:
            case RemoteControlRoku::BTN_PLAYPAUSE_HOLD:
            case RemoteControlRoku::BTN_POWER_HOLD:
            case RemoteControlRoku::BTN_RETURN_HOLD:
            case RemoteControlRoku::BTN_REWIND_HOLD:
            case RemoteControlRoku::BTN_RIGHT_HOLD:
            case RemoteControlRoku::BTN_UP_HOLD:
            case RemoteControlRoku::BTN_VOL_DOWN_HOLD:
            case RemoteControlRoku::BTN_VOL_UP_HOLD:
                return true;
                break;
            
            default:
                return false;
                break;
        }
    }
};
