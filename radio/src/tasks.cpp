/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define TESTPORTAUX2CTS

#if defined(TESTPORTAUX2CTS)
  #if !defined(SIMU)
    #include "opentx.h"

    #define TESTPORT_INIT testPortInit();
    #define TESTPORT_HIGH GPIO_SetBits(BT_EN_GPIO, BT_EN_GPIO_PIN);
    #define TESTPORT_LOW  GPIO_ResetBits(BT_EN_GPIO, BT_EN_GPIO_PIN);

    void testPortInit() {
      static bool testPortInitialized = false;

      if(!testPortInitialized) {
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = BT_EN_GPIO_PIN;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(BT_EN_GPIO, &GPIO_InitStructure);

        testPortInitialized = true;
      }
      
      //TESTPORT_INIT   // nur um zu sehen, ob der Compiler/Linker meckert
      //TESTPORT_HIGH   // nur um zu sehen, ob der Compiler/Linker meckert
      //TESTPORT_LOW    // nur um zu sehen, ob der Compiler/Linker meckert
    }
  #else
    #define TESTPORT_INIT
    #define TESTPORT_HIGH
    #define TESTPORT_LOW
  #endif
#endif

#include "opentx.h"
#include "mixer_scheduler.h"
#include "timers_driver.h"

RTOS_TASK_HANDLE menusTaskId;
RTOS_DEFINE_STACK(menusStack, MENUS_STACK_SIZE);

RTOS_TASK_HANDLE mixerTaskId;
RTOS_DEFINE_STACK(mixerStack, MIXER_STACK_SIZE);

RTOS_TASK_HANDLE audioTaskId;
RTOS_DEFINE_STACK(audioStack, AUDIO_STACK_SIZE);

RTOS_MUTEX_HANDLE audioMutex;
RTOS_MUTEX_HANDLE mixerMutex;

void stackPaint()
{
  menusStack.paint();
  mixerStack.paint();
  audioStack.paint();
#if defined(CLI)
  cliStack.paint();
#endif
}

volatile uint16_t timeForcePowerOffPressed = 0;

bool isForcePowerOffRequested()
{
  if (pwrOffPressed()) {
    if (timeForcePowerOffPressed == 0) {
      timeForcePowerOffPressed = get_tmr10ms();
    }
    else {
      uint16_t delay = (uint16_t)get_tmr10ms() - timeForcePowerOffPressed;
      if (delay > 1000/*10s*/) {
        return true;
      }
    }
  }
  else {
    resetForcePowerOffRequest();
  }
  return false;
}

void sendSynchronousPulsesInternal() {
#if defined(HARDWARE_INTERNAL_MODULE)
  uint32_t nowInternal = getTmr2MHz();
  static uint32_t lastIntPulse = nowInternal;

  //if((nowInternal - lastIntPulse) >= (getMixerSchedulerPeriodInternal()*2)) { 
   if((nowInternal - lastIntPulse) >= (7000*2)) { 
    if(setupPulsesInternalModule())
      intmoduleSendNextFrame();

    lastIntPulse = nowInternal;
  }
#endif
}

void sendSynchronousPulsesExternal() {
  TESTPORT_INIT
#if defined(HARDWARE_EXTERNAL_MODULE)
    
    TESTPORT_HIGH
    if(setupPulsesExternalModule())
      extmoduleSendNextFrame();

    lastExtPulse = nowExternal;
    TESTPORT_LOW
  }
#endif
}

constexpr uint8_t MIXER_FREQUENT_ACTIONS_PERIOD = 5 /*ms*/;
constexpr uint8_t MIXER_MAX_PERIOD = MAX_REFRESH_RATE / 1000 /*ms*/;

void execMixerFrequentActions()
{
#if defined(SBUS_TRAINER)
  // SBUS trainer
  processSbusInput();
#endif

#if defined(IMU)
  gyro.wakeup();
#endif

#if defined(BLUETOOTH)
  bluetooth.wakeup();
#endif

#if defined(SIMU)
  if (!s_pulses_paused) {
    DEBUG_TIMER_START(debugTimerTelemetryWakeup);
    telemetryWakeup();
    DEBUG_TIMER_STOP(debugTimerTelemetryWakeup);
  }
#endif
}

TASK_FUNCTION(mixerTask)
{
  s_pulses_paused = true;

  mixerSchedulerInit();
  mixerSchedulerStart();

  while (true) {
    int timeout = 0;
    for (; timeout < MIXER_MAX_PERIOD; timeout += MIXER_FREQUENT_ACTIONS_PERIOD) {

      // run periodicals before waiting for the trigger
      // to keep the delay short
      execMixerFrequentActions();

      // mixer flag triggered?
      if (!mixerSchedulerWaitForTrigger(MIXER_FREQUENT_ACTIONS_PERIOD)) {
        break;
      }
    }

#if defined(DEBUG_MIXER_SCHEDULER)
    GPIO_SetBits(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
    GPIO_ResetBits(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
#endif

    // re-enable trigger
    mixerSchedulerEnableTrigger();

#if defined(SIMU)
    if (pwrCheck() == e_power_off) {
      TASK_RETURN();
    }
#else
    if (isForcePowerOffRequested()) {
      boardOff();
    }
#endif

    if (!s_pulses_paused) {
      uint16_t t0 = getTmr2MHz();

      DEBUG_TIMER_START(debugTimerMixer);
      RTOS_LOCK_MUTEX(mixerMutex);

      doMixerCalculations();
      sendSynchronousPulsesInternal();
      sendSynchronousPulsesExternal();
      doMixerPeriodicUpdates();

      DEBUG_TIMER_START(debugTimerMixerCalcToUsage);
      DEBUG_TIMER_SAMPLE(debugTimerMixerIterval);
      RTOS_UNLOCK_MUTEX(mixerMutex);
      DEBUG_TIMER_STOP(debugTimerMixer);

#if defined(STM32) && !defined(SIMU)
      if (getSelectedUsbMode() == USB_JOYSTICK_MODE) {
        usbJoystickUpdate();
      }
#endif

      if (heartbeat == HEART_WDT_CHECK) {
        WDG_RESET();
        heartbeat = 0;
      }

      t0 = getTmr2MHz() - t0;
      if (t0 > maxMixerDuration)
        maxMixerDuration = t0;
    }
  }
}


#define MENU_TASK_PERIOD_TICKS         (50 / RTOS_MS_PER_TICK)    // 50ms

#if defined(COLORLCD) && defined(CLI)
bool perMainEnabled = true;
#endif

TASK_FUNCTION(menusTask)
{
#if defined(LIBOPENUI)
  LvglWrapper::instance();
#endif

#if defined(SPLASH) && !defined(STARTUP_ANIMATION)
  if (!UNEXPECTED_SHUTDOWN()) {
    drawSplash();
    TRACE("drawSplash() completed");
  }
#endif

#if defined(HARDWARE_TOUCH) && !defined(PCBFLYSKY) && !defined(SIMU)
  touchPanelInit();
#endif

#if defined(IMU)
  gyroInit();
#endif
  
  opentxInit();

#if defined(PWR_BUTTON_PRESS)
  while (true) {
    uint32_t pwr_check = pwrCheck();
    if (pwr_check == e_power_off) {
      break;
    }
    else if (pwr_check == e_power_press) {
      RTOS_WAIT_TICKS(MENU_TASK_PERIOD_TICKS);
      continue;
    }
#else
  while (pwrCheck() != e_power_off) {
#endif
    uint32_t start = (uint32_t)RTOS_GET_TIME();
    DEBUG_TIMER_START(debugTimerPerMain);
#if defined(COLORLCD) && defined(CLI)
    if (perMainEnabled) {
      perMain();
    }
#else
    perMain();
#endif
    DEBUG_TIMER_STOP(debugTimerPerMain);
    // TODO remove completely massstorage from sky9x firmware
    uint32_t runtime = ((uint32_t)RTOS_GET_TIME() - start);
    // deduct the thread run-time from the wait, if run-time was more than
    // desired period, then skip the wait all together
    if (runtime < MENU_TASK_PERIOD_TICKS) {
      RTOS_WAIT_TICKS(MENU_TASK_PERIOD_TICKS - runtime);
    }

    resetForcePowerOffRequest();
  }

#if defined(PCBX9E)
  toplcdOff();
#endif

#if defined(PCBHORUS)
  ledOff();
#endif

  drawSleepBitmap();
  opentxClose();
  boardOff(); // Only turn power off if necessary

  TASK_RETURN();
}

void tasksStart()
{
  RTOS_CREATE_MUTEX(audioMutex);
  RTOS_CREATE_MUTEX(mixerMutex);

#if defined(CLI)
  cliStart();
#endif

  RTOS_CREATE_TASK(mixerTaskId, mixerTask, "mixer", mixerStack,
                   MIXER_STACK_SIZE, MIXER_TASK_PRIO);
  RTOS_CREATE_TASK(menusTaskId, menusTask, "menus", menusStack,
                   MENUS_STACK_SIZE, MENUS_TASK_PRIO);

#if !defined(SIMU)
  RTOS_CREATE_TASK(audioTaskId, audioTask, "audio", audioStack,
                   AUDIO_STACK_SIZE, AUDIO_TASK_PRIO);
#endif

  RTOS_START();
}
