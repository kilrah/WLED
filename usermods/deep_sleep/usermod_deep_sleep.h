#pragma once

#ifdef ESP8266
#error The "Deep Sleep" usermod does not support ESP8266
#endif

// check if all required definitions are set
#if !defined(DEEPSLEEP_WAKEUPPIN)
#error DEEPSLEEP_WAKEUPPIN is required for "Deep Sleep" usermod
#endif

#include "wled.h"
#include "driver/rtc_io.h"

RTC_DATA_ATTR bool bootup = true; // variable in RTC data persists on a reboot

class DeepSleepUsermod : public Usermod {

  private:

    // Private class members. You can declare variables and functions only accessible to your usermod here
    bool enabled = true;
    //bool initDone = false;
    unsigned wakeupPin = DEEPSLEEP_WAKEUPPIN;

    // string that are used multiple time (this will save some flash memory)
    //static const char _name[];
    //static const char _enabled[];

    bool pin_is_valid(void) {
    #ifdef CONFIG_IDF_TARGET_ESP32 //ESP32: GPIOs 0,2,4, 12-15, 25-39 can be used for wake-up
      if (wakeupPin == 0 || wakeupPin == 2 || wakeupPin == 4 || (wakeupPin >= 12 && wakeupPin <= 15) || (wakeupPin >= 25 && wakeupPin <= 27) || (wakeupPin >= 32 && wakeupPin <= 39)) {
          return true;
      }
    #endif
    #if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2) //ESP32 S3 & S3: GPIOs 0-21 can be used for wake-up
      if (wakeupPin <= 21) {
          return true;
      }
    #endif
    #ifdef CONFIG_IDF_TARGET_ESP32C3 // ESP32 C3: GPIOs 0-5 can be used for wake-up
      if (wakeupPin <= 5) {
          return true;
      }
    #endif
      DEBUG_PRINTLN(F("Error: unsupported deep sleep wake-up pin"));
      return false;
    }

  public:

    inline void enable(bool enable) { enabled = enable; } // Enable/Disable the usermod
    inline bool isEnabled() { return enabled; } //Get usermod enabled/disabled state

    // setup is called at boot (or in this case after every exit of sleep mode)
    void setup() {
      //TODO: if the de-init of RTC pins is required to do it could be done here
      //rtc_gpio_deinit(wakeupPin);
      if(bootup == false) offMode = false; //not first bootup, turn LEDs on (overrides Turn LEDs on after power up/reset' at reboot)
      //initDone = true;
    }

    void loop() {
      if (!enabled || !offMode) return; // disabled or LEDs are on
      bootup = false; // turn leds on in all subsequent bootups
      if(!pin_is_valid()) return;
      DEBUG_PRINTLN(F("Error: unsupported deep sleep wake-up pin"));

      pinMode(wakeupPin, INPUT); // make sure GPIO is input with pullup/pulldown disabled
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL); //disable all wake-up sources (just in case)

      #ifdef DEEPSLEEP_WAKEUPINTEVAL
      esp_sleep_enable_timer_wakeup(DEEPSLEEP_WAKEUPINTEVAL * 1e6); //sleep for x seconds
      #endif

  #if defined(ARDUINO_ARCH_ESP32S3) || defined(ARDUINO_ARCH_ESP32S2) || defined(ARDUINO_ARCH_ESP32C3) // ESP32 S2, S3, C3
    #if defined(DEEPSLEEP_DISABLEPULL)
      gpio_sleep_set_pull_mode((gpio_num_t)wakeupPin, GPIO_FLOATING);
    #else
      #ifdef DEEPSLEEP_WAKEWHENHIGH
      gpio_sleep_set_pull_mode((gpio_num_t)wakeupPin, GPIO_PULLDOWN_ONLY);
      #else
      gpio_sleep_set_pull_mode((gpio_num_t)wakeupPin, GPIO_PULLUP_ONLY);
      #endif
    #endif
      esp_deep_sleep_enable_gpio_wakeup(1<<wakeupPin, ESP_GPIO_WAKEUP_GPIO_LOW);
  #else // ESP32
    #ifndef DEEPSLEEP_DISABLEPULL
      #ifdef DEEPSLEEP_WAKEWHENHIGH
      rtc_gpio_pulldown_en((gpio_num_t)wakeupPin);
      #else
      rtc_gpio_pullup_en((gpio_num_t)wakeupPin);
      #endif
    #endif
      esp_sleep_enable_ext0_wakeup((gpio_num_t)wakeupPin, LOW); //Only RTC pins can be used: 0,2,4,12-15,25-27,32-39. 
  #endif

      delay(1); // wait for pin to be ready
      esp_deep_sleep_start(); // go into deep sleep
    }

    //void connected() {} //unused, this is called every time the WiFi is (re)connected

    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId() {
        return USERMOD_ID_DEEP_SLEEP;
    }

};

// add more strings here to reduce flash memory usage
//const char DeepSleepUsermod::_name[]    PROGMEM = "DeepSleep";
//const char DeepSleepUsermod::_enabled[] PROGMEM = "enabled";