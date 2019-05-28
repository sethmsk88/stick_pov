typedef unsigned long uint32_t;

class RemoteControl
{
    public:
        static const uint32_t BTN_UP = 16718055; // (0xFF18E7)
        static const uint32_t BTN_DOWN = 16730805; // (0xFF4AB5)
        static const uint32_t BTN_LEFT = 16716015; // (0xFF10EF)
        static const uint32_t BTN_RIGHT = 16734885; // 0xFF5AA5
        static const uint32_t BTN_OK = 16726215; // 0xFF38C7
        static const uint32_t BTN_ASTERISK = 16738455; // 0xFF6897
        static const uint32_t BTN_POUND = 16756815; // 0xFFB04F
        static const uint32_t BTN_0 = 16750695; // 0xFF9867
        static const uint32_t BTN_1 = 16753245; // 0xFFA25D
        static const uint32_t BTN_2 = 16736925; // 0xFF629D
        static const uint32_t BTN_3 = 16769565; // 0xFFE21D
        static const uint32_t BTN_4 = 16720605; // 0xFF22DD
        static const uint32_t BTN_5 = 16712445; // 0xFF02FD
        static const uint32_t BTN_6 = 16761405; // 0xFFC23D
        static const uint32_t BTN_7 = 16769055; // 0xFFE01F
        static const uint32_t BTN_8 = 16754775; // 0xFFA857
        static const uint32_t BTN_9 = 16748655; // 0xFF906F
        static const uint32_t BTN_REPEAT = 0xFFFFFFFF; // This IR value is sent when a button is being held down
};
