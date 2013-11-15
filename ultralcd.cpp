#include "temperature.h"
#include "ultralcd.h"
#ifdef ULTRA_LCD
#include "Marlin.h"
#include "language.h"
#include "cardreader.h"
#include "temperature.h"
#include "stepper.h"
#include "ConfigurationStore.h"

/* Configuration settings */
int plaPreheatHotendTemp;
int plaPreheatHPBTemp;
int plaPreheatFanSpeed;

int absPreheatHotendTemp;
int absPreheatHPBTemp;
int absPreheatFanSpeed;
/* !Configuration settings */

//Function pointer to menu functions.
typedef void (*menuFunc_t)();

uint8_t lcd_status_message_level;
char lcd_status_message[LCD_WIDTH+1] = WELCOME_MSG;

#ifdef DOGLCD
#include "dogm_lcd_implementation.h"
#elif defined(ULTIBOARD_V2_CONTROLLER)
#include "ultralcd_implementation_ultiboard_v2.h"
#else
#include "ultralcd_implementation_hitachi_HD44780.h"
#endif

#ifdef SoftwareAutoLevel
#include "fitting_bed.h"
#endif

/** forward declerations **/

void copy_and_scalePID_i();
void copy_and_scalePID_d();

/* Different menus */
static void lcd_status_screen();
#ifdef ULTIPANEL
static void lcd_main_menu();
static void lcd_tune_menu();
static void lcd_prepare_menu();
static void lcd_move_menu();
static void lcd_control_menu();
static void lcd_control_temperature_menu();
static void lcd_control_temperature_preheat_pla_settings_menu();
static void lcd_control_temperature_preheat_abs_settings_menu();
static void lcd_control_motion_menu();
static void lcd_control_retract_menu();
static void lcd_sdcard_menu();

//static
#ifdef DreamMaker
void lcd_quick_feedback();//Cause an LCD refresh, and give the user visual or audiable feedback that something has happend
#else
static void lcd_quick_feedback();//Cause an LCD refresh, and give the user visual or audiable feedback that something has happend

#endif

/* Different types of actions that can be used in menuitems. */
static void menu_action_back(menuFunc_t data);
static void menu_action_submenu(menuFunc_t data);
static void menu_action_gcode(const char* pgcode);
static void menu_action_function(menuFunc_t data);
static void menu_action_sdfile(const char* filename, char* longFilename);
static void menu_action_sddirectory(const char* filename, char* longFilename);
static void menu_action_setting_edit_bool(const char* pstr, bool* ptr);
static void menu_action_setting_edit_byte(const char* pstr, uint8_t* ptr, uint8_t minValue, uint8_t maxValue);
static void menu_action_setting_edit_int3(const char* pstr, int* ptr, int minValue, int maxValue);
static void menu_action_setting_edit_int4(const char* pstr, int* ptr, int minValue, int maxValue);
static void menu_action_setting_edit_float3(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_float32(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_float5(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_float51(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_float52(const char* pstr, float* ptr, float minValue, float maxValue);
static void menu_action_setting_edit_long5(const char* pstr, unsigned long* ptr, unsigned long minValue, unsigned long maxValue);
static void menu_action_setting_edit_callback_bool(const char* pstr, bool* ptr, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_uchar(const char* pstr, int* ptr, int minValue, int maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_int3(const char* pstr, int* ptr, int minValue, int maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_int4(const char* pstr, int* ptr, int minValue, int maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float3(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float32(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float5(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float51(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_float52(const char* pstr, float* ptr, float minValue, float maxValue, menuFunc_t callbackFunc);
static void menu_action_setting_edit_callback_long5(const char* pstr, unsigned long* ptr, unsigned long minValue, unsigned long maxValue, menuFunc_t callbackFunc);

#define ENCODER_FEEDRATE_DEADZONE 10

#ifdef DreamMaker

  #define ENCODER_STEPS_PER_MENU_ITEM 1
#else

#if !defined(LCD_I2C_VIKI)
  #define ENCODER_STEPS_PER_MENU_ITEM 5
#else
  #define ENCODER_STEPS_PER_MENU_ITEM 2 // VIKI LCD rotary encoder uses a different number of steps per rotation
#endif

#endif

/* Helper macros for menus */
#define START_MENU() do { \
if (encoderPosition > 0x8000) encoderPosition = 0; \
if (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM < currentMenuViewOffset) currentMenuViewOffset = encoderPosition / ENCODER_STEPS_PER_MENU_ITEM;\
uint8_t _lineNr = currentMenuViewOffset, _menuItemNr; \
for(uint8_t _drawLineNr = 0; _drawLineNr < LCD_HEIGHT; _drawLineNr++, _lineNr++) { \
_menuItemNr = 0;
#define MENU_ITEM(type, label, args...) do { \
if (_menuItemNr == _lineNr) { \
if (lcdDrawUpdate) { \
const char* _label_pstr = PSTR(label); \
if ((encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) { \
lcd_implementation_drawmenu_ ## type ## _selected (_drawLineNr, _label_pstr , ## args ); \
}else{\
lcd_implementation_drawmenu_ ## type (_drawLineNr, _label_pstr , ## args ); \
}\
}\
if (LCD_CLICKED && (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) == _menuItemNr) {\
lcd_quick_feedback(); \
menu_action_ ## type ( args ); \
return;\
}\
}\
_menuItemNr++;\
} while(0)
#define MENU_ITEM_DUMMY() do { _menuItemNr++; } while(0)
#define MENU_ITEM_EDIT(type, label, args...) MENU_ITEM(setting_edit_ ## type, label, PSTR(label) , ## args )
#define MENU_ITEM_EDIT_CALLBACK(type, label, args...) MENU_ITEM(setting_edit_callback_ ## type, label, PSTR(label) , ## args )
#define END_MENU() \
if (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM >= _menuItemNr) encoderPosition = _menuItemNr * ENCODER_STEPS_PER_MENU_ITEM - 1; \
if ((uint8_t)(encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) >= currentMenuViewOffset + LCD_HEIGHT) { currentMenuViewOffset = (encoderPosition / ENCODER_STEPS_PER_MENU_ITEM) - LCD_HEIGHT + 1; lcdDrawUpdate = 1; _lineNr = currentMenuViewOffset - 1; _drawLineNr = -1; } \
} } while(0)

/** Used variables to keep track of the menu */
#ifndef REPRAPWORLD_KEYPAD
volatile uint8_t buttons;//Contains the bits of the currently pressed buttons.
#else
volatile uint16_t buttons;//Contains the bits of the currently pressed buttons (extended).
#endif

uint8_t currentMenuViewOffset;              /* scroll offset in the current menu */
uint32_t blocking_enc;
uint8_t lastEncoderBits;
volatile int16_t encoderDiff; /* encoderDiff is updated from interrupt context and added to encoderPosition every LCD update */
uint32_t encoderPosition;
#if (SDCARDDETECT > 0)
bool lcd_oldcardstatus;
#endif
#endif//ULTIPANEL

menuFunc_t currentMenu = lcd_status_screen; /* function pointer to the currently active menu */
uint32_t lcd_next_update_millis;
uint8_t lcd_status_update_delay;
uint8_t lcdDrawUpdate = 2;                  /* Set to none-zero when the LCD needs to draw, decreased after every draw. Set to 2 in LCD routines so the LCD gets atleast 1 full redraw (first redraw is partial) */

//prevMenu and prevEncoderPosition are used to store the previous menu location when editing settings.
menuFunc_t prevMenu = NULL;
uint16_t prevEncoderPosition;
//Variables used when editing values.
const char* editLabel;
void* editValue;
int32_t minEditValue, maxEditValue;
menuFunc_t callbackFunc;

// placeholders for Ki and Kd edits
float raw_Ki, raw_Kd;

#ifdef DreamMaker
static bool isButtonReversed = false;
#endif


/* Main status screen. It's up to the implementation specific part to show what is needed. As this is very display dependend */
static void lcd_status_screen()
{
  isButtonReversed=true;
  if (lcd_status_update_delay)
    lcd_status_update_delay--;
  else
    lcdDrawUpdate = 1;
  if (lcdDrawUpdate)
  {
    lcd_implementation_status_screen();
    lcd_status_update_delay = 10;   /* redraw the main screen every second. This is easier then trying keep track of all things that change on the screen */
  }
#ifdef ULTIPANEL
  if (LCD_CLICKED)
  {
#ifdef DreamMaker
    if (EnterMainMenu) {
      isButtonReversed=false;
      lcd_quick_feedback();
      currentMenu = lcd_main_menu;
    }
#else
    lcd_quick_feedback();
    currentMenu = lcd_main_menu;
#endif
    encoderPosition = 0;

  }
  
  // Dead zone at 100% feedrate
    if ((feedmultiply < 100 && (feedmultiply + int(encoderPosition)) > 100) ||
            (feedmultiply > 100 && (feedmultiply + int(encoderPosition)) < 100))
  {
    encoderPosition = 0;
    feedmultiply = 100;
  }
  
  if (feedmultiply == 100 && int(encoderPosition) > ENCODER_FEEDRATE_DEADZONE)
  {
    feedmultiply += int(encoderPosition) - ENCODER_FEEDRATE_DEADZONE;
    encoderPosition = 0;
  }
  else if (feedmultiply == 100 && int(encoderPosition) < -ENCODER_FEEDRATE_DEADZONE)
  {
    feedmultiply += int(encoderPosition) + ENCODER_FEEDRATE_DEADZONE;
    encoderPosition = 0;
  }
  else if (feedmultiply != 100)
  {
    feedmultiply += int(encoderPosition);
    encoderPosition = 0;
  }
  
  if (feedmultiply < 10)
    feedmultiply = 10;
  if (feedmultiply > 999)
    feedmultiply = 999;
#endif//ULTIPANEL
}

#ifdef ULTIPANEL
static void lcd_return_to_status()
{
  encoderPosition = 0;
  currentMenu = lcd_status_screen;
}

static void lcd_sdcard_pause()
{
#ifdef SmartSDPause
  enquecommand_P(PSTR("M704"));
  lcd_return_to_status();
#else
  card.pauseSDPrint();
#endif
}

static void lcd_sdcard_resume()
{
#ifdef SmartSDPause
  enquecommand_P(PSTR("M705"));
  lcd_return_to_status();
#else
  card.startFileprint();
#endif
  lcd_return_to_status();
}

#ifdef DreamMaker
static void lcd_dummy()
{
  
}
#endif
static void lcd_sdcard_stop()
{
  card.sdprinting = false;
  card.closefile();
  discardEnqueueingCommand();
  quickStop();
  if(SD_FINISHED_STEPPERRELEASE)
  {
    enquecommand_P(PSTR(SD_FINISHED_RELEASECOMMAND));
  }
#ifdef DreamMaker
  disable_heater();
  enquecommand_P(PSTR("M117 SD Stopped"));
  lcd_return_to_status();
#else
  autotempShutdown();
#endif

}





/* Menu implementation */
static void lcd_main_menu()
{
  START_MENU();
  MENU_ITEM(back, MSG_WATCH, lcd_status_screen);
  if (movesplanned() || IS_SD_PRINTING)
  {
    MENU_ITEM(submenu, MSG_TUNE, lcd_tune_menu);
  }else{
    MENU_ITEM(submenu, MSG_PREPARE, lcd_prepare_menu);
  }
  MENU_ITEM(submenu, MSG_CONTROL, lcd_control_menu);
#ifdef SDSUPPORT
  if (card.isOk())
  {
    if (card.isFileOpen())
    {
      if (card.sdprinting)
        MENU_ITEM(function, MSG_PAUSE_PRINT, lcd_sdcard_pause);
      else
        MENU_ITEM(function, MSG_RESUME_PRINT, lcd_sdcard_resume);
      MENU_ITEM(function, MSG_STOP_PRINT, lcd_sdcard_stop);
    }else{
      
      MENU_ITEM(submenu, MSG_CARD_MENU, lcd_sdcard_menu);
#ifdef SmartSDPause
      MENU_ITEM(function, MSG_RESUME_PRINT, lcd_sdcard_resume);
#endif
#if SDCARDDETECT < 1
      MENU_ITEM(gcode, MSG_CNG_SDCARD, PSTR("M21"));  // SD-card changed by user
#endif
    }
  }else{
    MENU_ITEM(submenu, MSG_NO_CARD, lcd_sdcard_menu);
#if SDCARDDETECT < 1
    MENU_ITEM(gcode, MSG_INIT_SDCARD, PSTR("M21")); // Manually initialize the SD-card via user interface
#endif
  }
#endif
  

  END_MENU();
}

#ifdef SDSUPPORT
static void lcd_autostart_sd()
{
  card.lastnr=0;
  card.setroot();
  card.checkautostart(true);
}
#endif

void lcd_preheat_pla()
{
  setTargetHotend0(plaPreheatHotendTemp);
  setTargetHotend1(plaPreheatHotendTemp);
  setTargetHotend2(plaPreheatHotendTemp);
  setTargetBed(plaPreheatHPBTemp);
  fanSpeed = plaPreheatFanSpeed;
  lcd_return_to_status();
  setWatch(); // heater sanity check timer
}

void lcd_preheat_abs()
{
  setTargetHotend0(absPreheatHotendTemp);
  setTargetHotend1(absPreheatHotendTemp);
  setTargetHotend2(absPreheatHotendTemp);
  setTargetBed(absPreheatHPBTemp);
  fanSpeed = absPreheatFanSpeed;
  lcd_return_to_status();
  setWatch(); // heater sanity check timer
}

static void lcd_cooldown()
{
  setTargetHotend0(0);
  setTargetHotend1(0);
  setTargetHotend2(0);
  setTargetBed(0);
  lcd_return_to_status();
}

static void lcd_tune_menu()
{
  START_MENU();
  MENU_ITEM(back, MSG_MAIN, lcd_main_menu);
  MENU_ITEM_EDIT(int3, MSG_SPEED, &feedmultiply, 10, 999);
  MENU_ITEM_EDIT(int3, MSG_NOZZLE, &target_temperature[0], 0, HEATER_0_MAXTEMP - 15);
#if TEMP_SENSOR_1 != 0
  MENU_ITEM_EDIT(int3, MSG_NOZZLE1, &target_temperature[1], 0, HEATER_1_MAXTEMP - 15);
#endif
#if TEMP_SENSOR_2 != 0
  MENU_ITEM_EDIT(int3, MSG_NOZZLE2, &target_temperature[2], 0, HEATER_2_MAXTEMP - 15);
#endif
#if TEMP_SENSOR_BED != 0
  MENU_ITEM_EDIT(int3, MSG_BED, &target_temperature_bed, 0, BED_MAXTEMP - 15);
#endif
  /////////////////////////change here
//    MENU_ITEM_EDIT(byte, MSG_FAN_SPEED, &fanSpeed, 0, 255);

  MENU_ITEM_EDIT(int3, MSG_FLOW, &extrudemultiply[0], 10, 999);
#ifdef FILAMENTCHANGEENABLE
  MENU_ITEM(gcode, MSG_FILAMENTCHANGE, PSTR("M600"));
#endif
  END_MENU();
}

static void lcd_feed_filament()
{
#ifdef FilamentSensor
#ifdef AMY
    enquecommand_P(PSTR("M109 S220.000000\nM117 Heating...\nM0 This is a filament feed program. Press button to continue...\nM117 Moving platform down\nG21\nG91\nM107\nG1 Z10 F180\nG90\nM400\nM703 Please insert filament properly and the stepper will run\nM117 Filament detected\nG92 E0\nG1 F80000 E80\nM400\nM0 Check the Hot End. Press button to continue...\nM117 More extruding\nG92 E0\nG1 F200 E15\nM400\nG92 E0\nM84\nM104 S0\nM106\nM0 Open the fan to cool down. Press button to finish...\nM107\nM117 Feed Filament End"));
#else
  enquecommand_P(PSTR("M109 S220.000000\nM117 Heating...\nM0 This is a filament feed program. Press button to continue...\nM117 Moving platform down\nG21\nG91\nM107\nG1 Z10 F180\nG90\nM400\nM703 Please insert filament properly and the stepper will run\nM117 Filament detected\nG92 E0\nG1 F80000 E80\nM400\nG92 E0\nM0 Let the filament extrude. Press button to continue...\nM117 Feeding filament...\nG92 E0\nG1 F320000 E200\nG92 E0\nG1 F320000 E200\nG92 E0\nG1 F320000 E180\nG92 E0\nG1 F160000 E100\nG92 E0\nM400\nM0 Check the Hot End. Press button to continue...\nM117 More extruding\nG92 E0\nG1 F100 E15\nM400\nG92 E0\nM84\nM104 S0\nM106\nM0 Open the fan to cool down. Press button to finish...\nM107\nM117 Feed Filament End"));
#endif
#else
  enquecommand_P(PSTR("M109 S220.000000\nM117 Heating...\nM0 This is a filament feed program. Press button to continue...\nM117 Moving platform down\nG21\nG91\nM107\nG1 Z10 F180\nG90\nM400\nM0 Please insert filament properly\nG92 E0\nG1 F80000 E80\nM400\nG92 E0\nM0 Let the filament extrude. Press button to continue...\nM117 Feeding filament...\nG92 E0\nG1 F320000 E200\nG92 E0\nG1 F320000 E200\nG92 E0\nG1 F320000 E180\nG92 E0\nG1 F160000 E100\nG92 E0\nM400\nM0 Check the Hot End. Press button to continue...\nM117 More extruding\nG92 E0\nG1 F100 E15\nM400\nG92 E0\nM84\nM104 S0\nM106\nM0 Open the fan to cool down. Press button to finish...\nM107\nM117 Feed Filament End"));

#endif
  lcd_return_to_status();
}
//;feed filament v1.0
//;by Rockets Xia
//;DFRobot
//
//
//M109 S220.000000;wait until 220
//M117 Heating...
//
//
//M0 This is a filament feed program. Press button to continue...
//
//G21        ;metric values
//G90        ;absolute positioning
//M107       ;start with the fan off
//
//M703 Please insert filament properly and the stepper will run
//
//G1 Z10 F180    ;move the platform up 10mm
//G1 Z40.0 F180  ;move the platform down 50mm
//
//G92 E0                  ;zero the extruded length
//G1 F80000 E80              ;extrude 200mm
//
//G92 E0                  ;zero the extruded length again
//
//M0 Let the filament extrude. Press button to continue...
//
//G92 E0                  ;zero the extruded length
//G1 F320000 E200              ;extrude
//G92 E0                  ;zero the extruded length again
//G1 F320000 E200              ;extrude
//G92 E0                  ;zero the extruded length again
//G1 F320000 E180              ;extrude
//G92 E0                  ;zero the extruded length again
//G1 F160000 E100              ;extrude
//G92 E0                  ;zero the extruded length again
//
//M0 Check the Hot End. Press button to continue...
//
//G92 E0                  ;zero the extruded length again
//G1 F100 E15              ;extrude
//G92 E0                  ;zero the extruded length again
//
//;EOF
//M84								;disable the stepper
//M104 S0 					;cool down
//M106							;open the fan
//M0 Open the fan to cool down. Press button to finish...
//M107							;close the fan



static void lcd_remove_filament()
{
  enquecommand_P(PSTR("M109 S220.000000\nG21\nM107\nG91\nM117 Removing filament\nG1 Z10 F180\nG90\nG92 E0\nG1 F100 E10\nG92 E0\nG1 F640000 E-20\nG92 E0\nG1 F320000 E-200\nG92 E0\nG1 F320000 E-200\nG92 E0\nG1 F320000 E-200\nG92 E0\nG1 F160000 E-100\nG92 E0\nG1 F80000 E-100\nM400\nG92 E0\nM84\nM104 S0\nM106\nM0 Open the fan to cool down. Press button to finish...\nM107\nM117 Remove Filament End"));
  lcd_return_to_status();
}
//;remove filament
//;by Rockets Xia
//;DFRobot
//
//
//M109 S220.000000;wait until 220
//
//G21        ;metric values
//M107       ;start with the fan off
//G91
//G1 Z10 F180    ;move the platform up 10mm
//G90        ;absolute positioning
//
//
//G92 E0                  ;zero the extruded length
//G1 F100 E10
//G4 P2000
//G92 E0                  ;zero the extruded length again
//G1 F640000 E-20              ;extrude
//G92 E0                  ;zero the extruded length again
//G1 F320000 E-200              ;extrude
//G92 E0                  ;zero the extruded length again
//G1 F320000 E-200              ;extrude
//G92 E0
//G1 F320000 E-200              ;extrude
//G92 E0                  ;zero the extruded length again
//G1 F160000 E-100              ;extrude
//G92 E0                  ;zero the extruded length again
//G1 F80000 E-100              ;extrude
//G92 E0                  ;zero the extruded length again
//M117 END
//
//;EOF
//M84								;disable the stepper
//M104 S0 					;cool down
//M106							;open the fan
//M0 Open the fan to cool down. Press button to finish...
//M107							;close the fan



static void lcd_level_assistant()
{
  
#ifdef ApproachSwitch
  #ifdef SoftwareAutoLevel
  
  enquecommand_P(PSTR("M707\nM0 This is a level assistant program. Press button to continue...\nG21\nG90\nG1 Z10\nG28 X0 Y0\nG1 Z10\nM117 Continue...\nG28 Z0\nG1 Z15.0 F180"));
  
  char enqueCommandStructer[50];
  
  strcpy_P(enqueCommandStructer, PSTR("G92 X0 Y0 Z"));
  strcat(enqueCommandStructer, ftostr32(15-(ApproachSwitchDistance - approachSwitchOffset[Z_AXIS])));
  strcat_P(enqueCommandStructer, PSTR(" E0"));
  
  enquecommand(enqueCommandStructer);
  
  enquecommand_P(PSTR("G1 X114 Y90 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform, and press button...\nM117 Continue...\nG1 F9000\nG1 Z15\nG1 X37 Y0 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X187 Y0 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X114 Y180 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform, and press button...\nM117 Double check...\nG1 F9000\nG1 Z5\nG1 X37 Y0 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X187 Y0 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X114 Y180 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X114 Y90 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X0 Y0 F9000\nG1 Z0 F180\nG1 Z5\nM400\nM117 Level assistant End\nM84\nM706"));
  
  #else
  enquecommand_P(PSTR("M0 This is a level assistant program. Press button to continue...\nG21\nG90\nG1 Z10\nG28 X0 Y0\nG1 Z10\nM117 Continue...\nG28 Z0\nG1 Z15.0 F180"));
  
  char enqueCommandStructer[50];
  
  strcpy_P(enqueCommandStructer, PSTR("G92 X0 Y0 Z"));
  strcat(enqueCommandStructer, ftostr32(15-(ApproachSwitchDistance - approachSwitchOffset[Z_AXIS])));
  strcat_P(enqueCommandStructer, PSTR(" E0"));

  enquecommand(enqueCommandStructer);
  
  enquecommand_P(PSTR("G1 X114 Y90 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform, and press button...\nM117 Continue...\nG1 F9000\nG1 Z15\nG1 X37 Y0 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X187 Y0 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X114 Y180 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform, and press button...\nM117 Double check...\nG1 F9000\nG1 Z5\nG1 X37 Y0 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X187 Y0 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X114 Y180 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X114 Y90 F9000\nG1 Z0 F180\nM400\nM702 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X0 Y0 F9000\nG1 Z0 F180\nG1 Z5\nM400\nM117 Level assistant End\nM84"));

  #endif
#else
  enquecommand_P(PSTR("M0 This is a level assistant program. Press button to continue...\nG21\nG90\nG1 Z10\nG28 X0 Y0\nG1 Z10\nM117 Continue...\nG28 Z0\nG92 X0 Y0 Z0 E0\nG1 Z15.0 F180\nG1 F9000\nG1 X100 Y100 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform, and press button...\nM117 Continue...\nG1 F9000\nG1 Z15\nG1 X0 Y0 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X180 Y15 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X180 Y180 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform, and press button...\nM117 Double check...\nG1 F9000\nG1 Z5\nG1 X0 Y200 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X0 Y0 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X180 Y15 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X180 Y180 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X0 Y200 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X100 Y100 F9000\nG1 Z0 F180\nM400\nM0 Adjust the platform again, and press button...\nM117 Continue...\nG1 F9000\nG1 Z5\nG1 X0 Y0 F9000\nG1 Z0 F180\nG1 Z5\nM400\nM117 Level assistant End\nM84"));
#endif
  
//  
//#ifdef SoftwareAutoLevel
//  enquecommand_P(PSTR("M707"));
//  
//
//#endif
//#ifdef 
//  #ifdef SoftwareAutoLevel
//  enquecommand_P(PSTR("M706"));
//#endif
//  
  
  lcd_return_to_status();
//  ;level v1.6
//  ;by Rockets Xia
//  ;modify by Angelo Qiao
//  ;DFRobot
//  
//  G21        ;metric values
//  G90        ;absolute positioning
//  G1 Z10
//  G28 X0 Y0  ;move X/Y to min endstops
//  G1 Z10
//  
//  M0 This is a level assistant program. Press button to continue...;wait for confirm
//    
//    
//    G28 Z0     ;move Z to min endstops
//  
//  
//  G92 X0 Y0 Z0 E0         ;reset software position to front/left/z=0.0
//  
//  G1 Z15.0 F180;move down
//  
//  
//  ;to the center check
//  G1 F9000
//  G1 X100 Y100 F9000
//  G1 Z0 F180
//  
//  M0 Adjust the platform, and press button...;wait for confirm
//    M117 Continue...
//    
//    ;to the home corner
//  G1 F9000
//  G1 Z15
//  G1 X0 Y0 F9000
//  G1 Z0 F180
//  
//  M0 Adjust the platform, and press button...;wait for confirm
//    M117 Continue...
//    
//    ;to the right down corner
//  G1 F9000
//  G1 Z5
//  G1 X180 Y15 F9000
//  G1 Z0 F180
//  
//  M0 Adjust the platform, and press button...;wait for confirm
//    M117 Continue...
//    
//    ;to the right up corner
//  G1 F9000
//  G1 Z5
//  G1 X180 Y180 F9000
//  G1 Z0 F180
//  
//  M0 Adjust the platform, and press button...;wait for confirm
//    M117 Double check...
//    
//    ;to the left up corner
//  G1 F9000
//  G1 Z5
//  G1 X0 Y200 F9000
//  G1 Z0 F180
//  
//  ;double check
//  M0 Adjust the platform again, and press button...;wait for confirm
//    M117 Continue...
//    
//    ;to the left down corner
//  G1 F9000
//  G1 Z5
//  G1 X0 Y0 F9000
//  G1 Z0 F180
//  
//  M0 Adjust the platform again, and press button...;wait for confirm
//    M117 Continue...
//    
//    ;to the right down corner
//  G1 F9000
//  G1 Z5
//  G1 X180 Y15 F9000
//  G1 Z0 F180
//  
//  M0 Adjust the platform again, and press button...;wait for confirm
//    M117 Continue...
//    
//    ;to the right up corner
//  G1 F9000
//  G1 Z5
//  G1 X180 Y180 F9000
//  G1 Z0 F180
//  
//  M0 Adjust the platform again, and press button...;wait for confirm
//    M117 Continue...
//    
//    ;to the left up corner
//  G1 F9000
//  G1 Z5
//  G1 X0 Y200 F9000
//  G1 Z0 F180
//  
//  M0 Adjust the platform again, and press button...;wait for confirm
//    M117 Continue...
//    
//    ;to the center
//  G1 F9000
//  G1 Z5
//  G1 X100 Y100 F9000
//  G1 Z0 F180
//  
//  M0 Adjust the platform again, and press button...;wait for confirm
//    M117 Continue...
//    
//    ;to the home
//  G1 F9000
//  G1 Z5
//  G1 X0 Y0 F9000
//  G1 Z0 F180
//  G1 Z5
//  
//  M0 Adjust the platform again, and press button...;wait for confirm
//    M117 Level assistant End
//    M84 

}



static void lcd_prepare_menu()
{
  START_MENU();
  MENU_ITEM(back, MSG_MAIN, lcd_main_menu);
#ifdef SDSUPPORT
  //MENU_ITEM(function, MSG_AUTOSTART, lcd_autostart_sd);
#endif
  MENU_ITEM(gcode, MSG_DISABLE_STEPPERS, PSTR("M84"));
  MENU_ITEM(gcode, MSG_AUTO_HOME, PSTR("G28"));
#ifdef DreamMaker_1_5_1
 MENU_ITEM(function, "Feed filament", lcd_feed_filament);
 MENU_ITEM(function, "Remove filament", lcd_remove_filament);
 MENU_ITEM(function, "Level assistant", lcd_level_assistant);  
  
#endif
//  MENU_ITEM(gcode, "software auto home", PSTR("G29"));
  
  //MENU_ITEM(gcode, MSG_SET_ORIGIN, PSTR("G92 X0 Y0 Z0"));
  MENU_ITEM(function, MSG_PREHEAT_PLA, lcd_preheat_pla);
  MENU_ITEM(function, MSG_PREHEAT_ABS, lcd_preheat_abs);
  MENU_ITEM(function, MSG_COOLDOWN, lcd_cooldown);
  MENU_ITEM(submenu, MSG_MOVE_AXIS, lcd_move_menu);
  END_MENU();
}

float move_menu_scale;
static void lcd_move_menu_axis();

static void lcd_move_x()
{
  if (encoderPosition != 0)
  {
    current_position[X_AXIS] += float((int)encoderPosition) * move_menu_scale;
    if (current_position[X_AXIS] < X_MIN_POS)
      current_position[X_AXIS] = X_MIN_POS;
    if (current_position[X_AXIS] > X_MAX_POS)
      current_position[X_AXIS] = X_MAX_POS;
    encoderPosition = 0;
    plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 600, active_extruder);
    lcdDrawUpdate = 1;
  }
  if (lcdDrawUpdate)
  {
    lcd_implementation_drawedit(PSTR("X"), ftostr31(current_position[X_AXIS]));
  }
  if (LCD_CLICKED)
  {
#ifdef DreamMaker
	isButtonReversed = false;
#endif
    lcd_quick_feedback();
    currentMenu = lcd_move_menu_axis;
    encoderPosition = 0;
  }
}
static void lcd_move_y()
{
  if (encoderPosition != 0)
  {
    current_position[Y_AXIS] += float((int)encoderPosition) * move_menu_scale;
    if (current_position[Y_AXIS] < Y_MIN_POS)
      current_position[Y_AXIS] = Y_MIN_POS;
    if (current_position[Y_AXIS] > Y_MAX_POS)
      current_position[Y_AXIS] = Y_MAX_POS;
    encoderPosition = 0;
    plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 600, active_extruder);
    lcdDrawUpdate = 1;
  }
  if (lcdDrawUpdate)
  {
    lcd_implementation_drawedit(PSTR("Y"), ftostr31(current_position[Y_AXIS]));
  }
  if (LCD_CLICKED)
  {
#ifdef DreamMaker
	isButtonReversed = false;
#endif
    lcd_quick_feedback();
    currentMenu = lcd_move_menu_axis;
    encoderPosition = 0;
  }
}
static void lcd_move_z()
{
  if (encoderPosition != 0)
  {
    current_position[Z_AXIS] += float((int)encoderPosition) * move_menu_scale;
    if (current_position[Z_AXIS] < Z_MIN_POS)
      current_position[Z_AXIS] = Z_MIN_POS;
    if (current_position[Z_AXIS] > Z_MAX_POS)
      current_position[Z_AXIS] = Z_MAX_POS;
    encoderPosition = 0;
    plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 60, active_extruder);
    lcdDrawUpdate = 1;
  }
  if (lcdDrawUpdate)
  {
    lcd_implementation_drawedit(PSTR("Z"), ftostr31(current_position[Z_AXIS]));
  }
  if (LCD_CLICKED)
  {
#ifdef DreamMaker
	isButtonReversed = false;
#endif
    lcd_quick_feedback();
    currentMenu = lcd_move_menu_axis;
    encoderPosition = 0;
  }
}
static void lcd_move_e()
{
    if (blocking_enc>=millis() || LCD_CLICKED)
    {
        while (movesplanned() < 3)
        {
            current_position[E_AXIS] += 0.5;
            lcd_implementation_drawedit(PSTR("Extruder"), ftostr31(current_position[E_AXIS]));
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 3, active_extruder);
        }
    }else{
        lcd_quick_feedback();
        currentMenu = lcd_move_menu_axis;
        encoderPosition = prevEncoderPosition;
    }
}

static void lcd_move_menu_axis()
{
  START_MENU();
  MENU_ITEM(back, MSG_MOVE_AXIS, lcd_move_menu);
  MENU_ITEM(submenu, "Move X", lcd_move_x);
  MENU_ITEM(submenu, "Move Y", lcd_move_y);
  if (move_menu_scale < 10.0)
  {
    MENU_ITEM(submenu, "Move Z", lcd_move_z);
    MENU_ITEM(submenu, "Extruder", lcd_move_e);
  }
  END_MENU();
}

static void lcd_move_menu_10mm()
{
  move_menu_scale = 10.0;
  lcd_move_menu_axis();
}
static void lcd_move_menu_1mm()
{
  move_menu_scale = 1.0;
  lcd_move_menu_axis();
}
static void lcd_move_menu_01mm()
{
  move_menu_scale = 0.1;
  lcd_move_menu_axis();
}

static void lcd_move_menu()
{
  START_MENU();
  MENU_ITEM(back, MSG_PREPARE, lcd_prepare_menu);
  MENU_ITEM(submenu, "Move 10mm", lcd_move_menu_10mm);
  MENU_ITEM(submenu, "Move 1mm", lcd_move_menu_1mm);
  MENU_ITEM(submenu, "Move 0.1mm", lcd_move_menu_01mm);
  //TODO:X,Y,Z,E
  END_MENU();
}

static void lcd_control_menu()
{
  START_MENU();
  MENU_ITEM(back, MSG_MAIN, lcd_main_menu);
  
#ifdef DreamMaker_1_5_1
  MENU_ITEM(function, "Ver.1.5.1", lcd_dummy);
#else
  MENU_ITEM(function, "Ver.1.5", lcd_dummy);
#endif
  
#ifdef SoftwareAutoLevel
  MENU_ITEM_EDIT(bool, "Auto Level", &isSoftwareAutoLevel);
#endif
  MENU_ITEM(submenu, MSG_TEMPERATURE, lcd_control_temperature_menu);
  MENU_ITEM(submenu, MSG_MOTION, lcd_control_motion_menu);
#ifdef FWRETRACT
  MENU_ITEM(submenu, MSG_RETRACT, lcd_control_retract_menu);
#endif
#ifdef EEPROM_SETTINGS
  MENU_ITEM(function, MSG_STORE_EPROM, Config_StoreSettings);
  MENU_ITEM(function, MSG_LOAD_EPROM, Config_RetrieveSettings);
#endif
  MENU_ITEM(function, MSG_RESTORE_FAILSAFE, Config_ResetDefault);
  END_MENU();
}

static void lcd_control_temperature_menu()
{
#ifdef PIDTEMP
    // set up temp variables - undo the default scaling
    raw_Ki = unscalePID_i(Ki);
    raw_Kd = unscalePID_d(Kd);
#endif

    START_MENU();
    MENU_ITEM(back, MSG_CONTROL, lcd_control_menu);
    MENU_ITEM_EDIT(int3, MSG_NOZZLE, &target_temperature[0], 0, HEATER_0_MAXTEMP - 15);
#if TEMP_SENSOR_1 != 0
  MENU_ITEM_EDIT(int3, MSG_NOZZLE1, &target_temperature[1], 0, HEATER_1_MAXTEMP - 15);
#endif
#if TEMP_SENSOR_2 != 0
  MENU_ITEM_EDIT(int3, MSG_NOZZLE2, &target_temperature[2], 0, HEATER_2_MAXTEMP - 15);
#endif
#if TEMP_SENSOR_BED != 0
  MENU_ITEM_EDIT(int3, MSG_BED, &target_temperature_bed, 0, BED_MAXTEMP - 15);
#endif
  //////change here
//    MENU_ITEM_EDIT(byte, MSG_FAN_SPEED, &fanSpeed, 0, 255);

#ifdef AUTOTEMP
    MENU_ITEM_EDIT(bool, MSG_AUTOTEMP, &autotemp_enabled);
    MENU_ITEM_EDIT(float3, MSG_MIN, &autotemp_min, 0, HEATER_0_MAXTEMP - 15);
    MENU_ITEM_EDIT(float3, MSG_MAX, &autotemp_max, 0, HEATER_0_MAXTEMP - 15);
    MENU_ITEM_EDIT(float32, MSG_FACTOR, &autotemp_factor, 0.0, 1.0);
#endif
#ifdef PIDTEMP
  MENU_ITEM_EDIT(float52, MSG_PID_P, &Kp, 1, 9990);
  // i is typically a small value so allows values below 1
  MENU_ITEM_EDIT_CALLBACK(float52, MSG_PID_I, &raw_Ki, 0.01, 9990, copy_and_scalePID_i);
  MENU_ITEM_EDIT_CALLBACK(float52, MSG_PID_D, &raw_Kd, 1, 9990, copy_and_scalePID_d);
# ifdef PID_ADD_EXTRUSION_RATE
  MENU_ITEM_EDIT(float3, MSG_PID_C, &Kc, 1, 9990);
# endif//PID_ADD_EXTRUSION_RATE
#endif//PIDTEMP
  MENU_ITEM(submenu, MSG_PREHEAT_PLA_SETTINGS, lcd_control_temperature_preheat_pla_settings_menu);
  MENU_ITEM(submenu, MSG_PREHEAT_ABS_SETTINGS, lcd_control_temperature_preheat_abs_settings_menu);
  END_MENU();
}

static void lcd_control_temperature_preheat_pla_settings_menu()
{
  START_MENU();
  MENU_ITEM(back, MSG_TEMPERATURE, lcd_control_temperature_menu);
  MENU_ITEM_EDIT(int3, MSG_FAN_SPEED, &plaPreheatFanSpeed, 0, 255);
  MENU_ITEM_EDIT(int3, MSG_NOZZLE, &plaPreheatHotendTemp, 0, HEATER_0_MAXTEMP - 15);
#if TEMP_SENSOR_BED != 0
  MENU_ITEM_EDIT(int3, MSG_BED, &plaPreheatHPBTemp, 0, BED_MAXTEMP - 15);
#endif
#ifdef EEPROM_SETTINGS
  MENU_ITEM(function, MSG_STORE_EPROM, Config_StoreSettings);
#endif
  END_MENU();
}

static void lcd_control_temperature_preheat_abs_settings_menu()
{
  START_MENU();
  MENU_ITEM(back, MSG_TEMPERATURE, lcd_control_temperature_menu);
  MENU_ITEM_EDIT(int3, MSG_FAN_SPEED, &absPreheatFanSpeed, 0, 255);
  MENU_ITEM_EDIT(int3, MSG_NOZZLE, &absPreheatHotendTemp, 0, HEATER_0_MAXTEMP - 15);
#if TEMP_SENSOR_BED != 0
  MENU_ITEM_EDIT(int3, MSG_BED, &absPreheatHPBTemp, 0, BED_MAXTEMP - 15);
#endif
#ifdef EEPROM_SETTINGS
  MENU_ITEM(function, MSG_STORE_EPROM, Config_StoreSettings);
#endif
  END_MENU();
}

#if MOTOR_CURRENT_PWM_XY_PIN > -1
static void update_motor_power()
{
    digipot_current(0, motor_current_setting[0]);
    digipot_current(1, motor_current_setting[1]);
    digipot_current(2, motor_current_setting[2]);
    Config_StoreSettings();
}
#endif

static void lcd_control_motion_menu()
{
  START_MENU();
  MENU_ITEM(back, MSG_CONTROL, lcd_control_menu);
#ifdef SoftwareAutoLevel
  MENU_ITEM_EDIT(float52, "offset", &approachSwitchOffset[Z_AXIS], 0, ApproachSwitchDistance);
#endif
  
  MENU_ITEM_EDIT(float5, MSG_ACC, &acceleration, 500, 99000);
  MENU_ITEM_EDIT(float3, MSG_VXY_JERK, &max_xy_jerk, 1, 990);
  MENU_ITEM_EDIT(float52, MSG_VZ_JERK, &max_z_jerk, 0.1, 990);
  MENU_ITEM_EDIT(float3, MSG_VE_JERK, &max_e_jerk, 1, 990);
  MENU_ITEM_EDIT(float3, MSG_VMAX MSG_X, &max_feedrate[X_AXIS], 1, 999);
  MENU_ITEM_EDIT(float3, MSG_VMAX MSG_Y, &max_feedrate[Y_AXIS], 1, 999);
  MENU_ITEM_EDIT(float3, MSG_VMAX MSG_Z, &max_feedrate[Z_AXIS], 1, 999);
  MENU_ITEM_EDIT(float3, MSG_VMAX MSG_E, &max_feedrate[E_AXIS], 1, 999);
  MENU_ITEM_EDIT(float3, MSG_VMIN, &minimumfeedrate, 0, 999);
  MENU_ITEM_EDIT(float3, MSG_VTRAV_MIN, &mintravelfeedrate, 0, 999);
  MENU_ITEM_EDIT_CALLBACK(long5, MSG_AMAX MSG_X, &max_acceleration_units_per_sq_second[X_AXIS], 100, 99000, reset_acceleration_rates);
  MENU_ITEM_EDIT_CALLBACK(long5, MSG_AMAX MSG_Y, &max_acceleration_units_per_sq_second[Y_AXIS], 100, 99000, reset_acceleration_rates);
  MENU_ITEM_EDIT_CALLBACK(long5, MSG_AMAX MSG_Z, &max_acceleration_units_per_sq_second[Z_AXIS], 100, 99000, reset_acceleration_rates);
  MENU_ITEM_EDIT_CALLBACK(long5, MSG_AMAX MSG_E, &max_acceleration_units_per_sq_second[E_AXIS], 100, 99000, reset_acceleration_rates);
  MENU_ITEM_EDIT(float5, MSG_A_RETRACT, &retract_acceleration, 100, 99000);
  MENU_ITEM_EDIT(float52, MSG_XSTEPS, &axis_steps_per_unit[X_AXIS], 5, 9999);
  MENU_ITEM_EDIT(float52, MSG_YSTEPS, &axis_steps_per_unit[Y_AXIS], 5, 9999);
  MENU_ITEM_EDIT(float51, MSG_ZSTEPS, &axis_steps_per_unit[Z_AXIS], 5, 9999);
  MENU_ITEM_EDIT(float51, MSG_ESTEPS, &axis_steps_per_unit[E_AXIS], 5, 9999);
#ifdef ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED
  MENU_ITEM_EDIT(bool, "Endstop abort", &abort_on_endstop_hit);
#endif
#if MOTOR_CURRENT_PWM_XY_PIN > -1
    MENU_ITEM_EDIT_CALLBACK(int4, "PowerXY", &motor_current_setting[0], 0, MOTOR_CURRENT_PWM_RANGE, update_motor_power);
    MENU_ITEM_EDIT_CALLBACK(int4, "PowerZ", &motor_current_setting[1], 0, MOTOR_CURRENT_PWM_RANGE, update_motor_power);
    MENU_ITEM_EDIT_CALLBACK(int4, "PowerE", &motor_current_setting[2], 0, MOTOR_CURRENT_PWM_RANGE, update_motor_power);
#endif
    END_MENU();
}

#ifdef FWRETRACT
static void lcd_control_retract_menu()
{
  START_MENU();
  MENU_ITEM(back, MSG_CONTROL, lcd_control_menu);
  MENU_ITEM_EDIT(bool, MSG_AUTORETRACT, &autoretract_enabled);
  MENU_ITEM_EDIT(float52, MSG_CONTROL_RETRACT, &retract_length, 0, 100);
  MENU_ITEM_EDIT(float3, MSG_CONTROL_RETRACTF, &retract_feedrate, 1, 999);
  MENU_ITEM_EDIT(float52, MSG_CONTROL_RETRACT_ZLIFT, &retract_zlift, 0, 999);
  MENU_ITEM_EDIT(float52, MSG_CONTROL_RETRACT_RECOVER, &retract_recover_length, 0, 100);
  MENU_ITEM_EDIT(float3, MSG_CONTROL_RETRACT_RECOVERF, &retract_recover_feedrate, 1, 999);
  END_MENU();
}
#endif

#if SDCARDDETECT == -1
static void lcd_sd_refresh()
{
  card.initsd();
  currentMenuViewOffset = 0;
}
#endif
static void lcd_sd_updir()
{
  card.updir();
  currentMenuViewOffset = 0;
}

void lcd_sdcard_menu()
{
  uint16_t fileCnt = card.getnrfilenames();
  START_MENU();
  MENU_ITEM(back, MSG_MAIN, lcd_main_menu);
  card.getWorkDirName();
  if(card.filename[0]=='/')
  {
#if SDCARDDETECT == -1
    MENU_ITEM(function, LCD_STR_REFRESH MSG_REFRESH, lcd_sd_refresh);
#endif
  }else{
    MENU_ITEM(function, LCD_STR_FOLDER "..", lcd_sd_updir);
  }
  
  for(uint16_t i=0;i<fileCnt;i++)
  {
    if (_menuItemNr == _lineNr)
    {
      card.getfilename(i);
      if (card.filenameIsDir)
      {
        MENU_ITEM(sddirectory, MSG_CARD_MENU, card.filename, card.longFilename);
      }else{
        MENU_ITEM(sdfile, MSG_CARD_MENU, card.filename, card.longFilename);
      }
    }else{
      MENU_ITEM_DUMMY();
    }
  }
  END_MENU();
}

#define menu_edit_type(_type, _name, _strFunc, scale) \
void menu_edit_ ## _name () \
{ \
if ((int32_t)encoderPosition < minEditValue) \
encoderPosition = minEditValue; \
if ((int32_t)encoderPosition > maxEditValue) \
encoderPosition = maxEditValue; \
if (lcdDrawUpdate) \
lcd_implementation_drawedit(editLabel, _strFunc(((_type)encoderPosition) / scale)); \
if (LCD_CLICKED) \
{ \
*((_type*)editValue) = ((_type)encoderPosition) / scale; \
lcd_quick_feedback(); \
currentMenu = prevMenu; \
encoderPosition = prevEncoderPosition; \
isButtonReversed=false;\
} \
} \
void menu_edit_callback_ ## _name () \
{ \
if ((int32_t)encoderPosition < minEditValue) \
encoderPosition = minEditValue; \
if ((int32_t)encoderPosition > maxEditValue) \
encoderPosition = maxEditValue; \
if (lcdDrawUpdate) \
lcd_implementation_drawedit(editLabel, _strFunc(((_type)encoderPosition) / scale)); \
if (LCD_CLICKED) \
{ \
*((_type*)editValue) = ((_type)encoderPosition) / scale; \
lcd_quick_feedback(); \
currentMenu = prevMenu; \
encoderPosition = prevEncoderPosition; \
(*callbackFunc)();\
} \
} \
static void menu_action_setting_edit_ ## _name (const char* pstr, _type* ptr, _type minValue, _type maxValue) \
{ \
isButtonReversed=true;\
prevMenu = currentMenu; \
prevEncoderPosition = encoderPosition; \
\
lcdDrawUpdate = 2; \
currentMenu = menu_edit_ ## _name; \
\
editLabel = pstr; \
editValue = ptr; \
minEditValue = minValue * scale; \
maxEditValue = maxValue * scale; \
encoderPosition = (*ptr) * scale; \
}\
static void menu_action_setting_edit_callback_ ## _name (const char* pstr, _type* ptr, _type minValue, _type maxValue, menuFunc_t callback) \
{ \
prevMenu = currentMenu; \
prevEncoderPosition = encoderPosition; \
\
lcdDrawUpdate = 2; \
currentMenu = menu_edit_callback_ ## _name; \
\
editLabel = pstr; \
editValue = ptr; \
minEditValue = minValue * scale; \
maxEditValue = maxValue * scale; \
encoderPosition = (*ptr) * scale; \
callbackFunc = callback;\
}
menu_edit_type(uint8_t, byte, itostr3, 1)
menu_edit_type(int, int3, itostr3, 1)
menu_edit_type(int, int4, itostr4, 1)
menu_edit_type(float, float3, ftostr3, 1)
menu_edit_type(float, float32, ftostr32, 100)
menu_edit_type(float, float5, ftostr5, 0.01)
menu_edit_type(float, float51, ftostr51, 10)
menu_edit_type(float, float52, ftostr52, 100)
menu_edit_type(unsigned long, long5, ftostr5, 0.01)

#ifdef REPRAPWORLD_KEYPAD
static void reprapworld_keypad_move_y_down() {
  encoderPosition = 1;
  move_menu_scale = REPRAPWORLD_KEYPAD_MOVE_STEP;
  lcd_move_y();
}
static void reprapworld_keypad_move_y_up() {
  encoderPosition = -1;
  move_menu_scale = REPRAPWORLD_KEYPAD_MOVE_STEP;
  lcd_move_y();
}
static void reprapworld_keypad_move_home() {
  //enquecommand_P((PSTR("G28"))); // move all axis home
  // TODO gregor: move all axis home, i have currently only one axis on my prusa i3
  enquecommand_P((PSTR("G28 Y")));
}
#endif

/** End of menus **/

#ifdef DreamMaker
void lcd_quick_feedback()
#else
static void lcd_quick_feedback()
#endif
{
    lcdDrawUpdate = 2;
    blocking_enc = millis() + 500;
    lcd_implementation_quick_feedback();
}

/** Menu action functions **/
static void menu_action_back(menuFunc_t data)
{
  currentMenu = data;
  encoderPosition = 0;
}
static void menu_action_submenu(menuFunc_t data)
{
    currentMenu = data;
    prevEncoderPosition = encoderPosition;
    encoderPosition = 0;
}
static void menu_action_gcode(const char* pgcode)
{
  enquecommand_P(pgcode);
}
static void menu_action_function(menuFunc_t data)
{
  (*data)();
}
static void menu_action_sdfile(const char* filename, char* longFilename)
{
  char cmd[30];
  char* c;
  sprintf_P(cmd, PSTR("M23 %s"), filename);
  for(c = &cmd[4]; *c; c++)
    *c = tolower(*c);
  enquecommand(cmd);
  enquecommand_P(PSTR("M24"));
  lcd_return_to_status();
}
static void menu_action_sddirectory(const char* filename, char* longFilename)
{
  card.chdir(filename);
  encoderPosition = 0;
}
static void menu_action_setting_edit_bool(const char* pstr, bool* ptr)
{
  *ptr = !(*ptr);
}
#endif//ULTIPANEL

/** LCD API **/
void lcd_init()
{
  lcd_implementation_init();
  
#ifdef PushButton
  pinMode(PushButtonUp, INPUT);
  pinMode(PushButtonDown, INPUT);
  pinMode(PushButtonEnter, INPUT);
#endif
  
#ifdef NEWPANEL
  pinMode(BTN_EN1,INPUT);
  pinMode(BTN_EN2,INPUT);
  pinMode(SDCARDDETECT,INPUT);
  WRITE(BTN_EN1,HIGH);
  WRITE(BTN_EN2,HIGH);
#if BTN_ENC > 0
  pinMode(BTN_ENC,INPUT);
  WRITE(BTN_ENC,HIGH);
#endif
#ifdef REPRAPWORLD_KEYPAD
  pinMode(SHIFT_CLK,OUTPUT);
  pinMode(SHIFT_LD,OUTPUT);
  pinMode(SHIFT_OUT,INPUT);
  WRITE(SHIFT_OUT,HIGH);
  WRITE(SHIFT_LD,HIGH);
#endif
#else
  pinMode(SHIFT_CLK,OUTPUT);
  pinMode(SHIFT_LD,OUTPUT);
  pinMode(SHIFT_EN,OUTPUT);
  pinMode(SHIFT_OUT,INPUT);
  WRITE(SHIFT_OUT,HIGH);
  WRITE(SHIFT_LD,HIGH);
  WRITE(SHIFT_EN,LOW);
#endif//!NEWPANEL
#if (SDCARDDETECT > 0)
  WRITE(SDCARDDETECT, HIGH);
  lcd_oldcardstatus = IS_SD_INSERTED;
#endif//(SDCARDDETECT > 0)
#ifdef DreamMaker
  
#else
  lcd_buttons_update();

#endif
#ifdef ULTIPANEL
  encoderDiff = 0;
#endif
}

#ifdef DreamMaker
static void lcd_sd_only_file()
{
  if (card.getnrfilenames() == 1) {
    card.getfilename(0);
    if (!card.filenameIsDir) {
      char cmd[30];
      char* c;
      sprintf_P(cmd, PSTR("M23 %s"), card.filename);
      for(c = &cmd[4]; *c; c++)
        *c = tolower(*c);
      enquecommand(cmd);
      enquecommand_P(PSTR("M24"));
      lcd_return_to_status();
    }
  }
}
#endif

void lcd_update()
{
  static unsigned long timeoutToStatus = 0;
#ifdef DreamMaker
  
#else
  lcd_buttons_update();
#endif
#ifdef LCD_HAS_SLOW_BUTTONS
  buttons |= lcd_implementation_read_slow_buttons(); // buttons which take too long to read in interrupt context
#endif
  
#if (SDCARDDETECT > 0)
  if((IS_SD_INSERTED != lcd_oldcardstatus))
  {
    lcdDrawUpdate = 2;
    lcd_oldcardstatus = IS_SD_INSERTED;
    lcd_implementation_init(); // to maybe revive the lcd if static electricty killed it.
    
    if(lcd_oldcardstatus)
    {
      card.initsd();
      LCD_MESSAGEPGM(MSG_SD_INSERTED);
      #ifdef DreamMaker
      lcd_sd_only_file();
      #endif
    }
    else
    {
      card.release();
      LCD_MESSAGEPGM(MSG_SD_REMOVED);
    }
  }
#endif//CARDINSERTED
  
  

  if (lcd_next_update_millis < millis())
  {
#ifdef ULTIPANEL
#ifdef REPRAPWORLD_KEYPAD
    if (REPRAPWORLD_KEYPAD_MOVE_Y_DOWN) {
      reprapworld_keypad_move_y_down();
    }
    if (REPRAPWORLD_KEYPAD_MOVE_Y_UP) {
      reprapworld_keypad_move_y_up();
    }
    if (REPRAPWORLD_KEYPAD_MOVE_HOME) {
      reprapworld_keypad_move_home();
    }
#endif
    if (encoderDiff)
    {
      lcdDrawUpdate = 1;
#ifdef PushButton
      if (isButtonReversed) {
        encoderPosition -= encoderDiff;
      }
      else{
        encoderPosition += encoderDiff;
      }
#else
  #ifdef DreamMaker
//      if (encoderDiff>0) {
//        encoderDiff++;
//      }
//      else{
//        encoderDiff--;
//      }
        encoderPosition += (encoderDiff)/2;
  #else
        encoderPosition += encoderDiff;
  #endif
#endif
      
      encoderDiff = 0;
      timeoutToStatus = millis() + LCD_TIMEOUT_TO_STATUS;
    }
    if (LCD_CLICKED)
      timeoutToStatus = millis() + LCD_TIMEOUT_TO_STATUS;
#endif//ULTIPANEL
    
#ifdef DOGLCD        // Changes due to different driver architecture of the DOGM display
    //#ifdef DreamMaker
    blink++;     // Variable for fan animation and alive dot
    (*currentMenu)();
    if (lcdDrawUpdate>=2) {
      lcdDrawUpdate=1;
    }
    if (lcdDrawUpdate==1) {
      u8g.firstPage();
      do
      {
        u8g.setFont(u8g_font_6x10_marlin);
        u8g.setPrintPos(125,0);
        if (blink % 2) u8g.setColorIndex(1); else u8g.setColorIndex(0); // Set color for the alive dot
        u8g.drawPixel(127,63); // draw alive dot
        u8g.setColorIndex(1); // black on white
        (*currentMenu)();
      } while( u8g.nextPage() );
    }
#else
    (*currentMenu)();
#endif
    
//#ifdef DreamMaker
//    SERIAL_PROTOCOLLNPGM("4:");
//    SERIAL_PROTOCOLLN(digitalRead(BEEPER));
//#endif
    
    
#ifdef LCD_HAS_STATUS_INDICATORS
    lcd_implementation_update_indicators();
#endif
    
#ifdef ULTIPANEL
    if(timeoutToStatus < millis() && currentMenu != lcd_status_screen)
    {
      lcd_return_to_status();
      lcdDrawUpdate = 2;
    }
#endif//ULTIPANEL
    if (lcdDrawUpdate == 2)
      lcd_implementation_clear();
    if (lcdDrawUpdate)
      lcdDrawUpdate--;
    lcd_next_update_millis = millis() + 100;
  }
}

void lcd_setstatus(const char* message)
{
  int strlenTemp;
  if (lcd_status_message_level > 0)
    return;
#ifdef DreamMaker
  strncpy(lcd_status_message, message, LCD_WIDTH);
  strlenTemp=strlen(lcd_status_message);

  if (strlenTemp<LCD_WIDTH) {
    memset(lcd_status_message + strlenTemp, ' ', LCD_WIDTH - strlenTemp);
  }
  SERIAL_ECHOLN(lcd_status_message);
  lcdDrawUpdate = 1;
#else
  strncpy(lcd_status_message, message, LCD_WIDTH);
  lcdDrawUpdate = 2;
#endif
}
void lcd_setstatuspgm(const char* message)
{
  int strlenTemp;
  if (lcd_status_message_level > 0)
    return;
#ifdef DreamMaker
  strncpy_P(lcd_status_message, message, LCD_WIDTH);
  strlenTemp=strlen(lcd_status_message);
  
  if (strlenTemp<LCD_WIDTH) {
    memset(lcd_status_message + strlenTemp, ' ', LCD_WIDTH - strlenTemp);
  }
  SERIAL_ECHOLN(lcd_status_message);
  lcdDrawUpdate = 1;
#else
  strncpy_P(lcd_status_message, message, LCD_WIDTH);
  lcdDrawUpdate = 2;
#endif
}
void lcd_setalertstatuspgm(const char* message)
{
  lcd_setstatuspgm(message);
  lcd_status_message_level = 1;
#ifdef ULTIPANEL
  lcd_return_to_status();
#endif//ULTIPANEL
}
void lcd_reset_alert_level()
{
  lcd_status_message_level = 0;
}


#ifdef ULTIPANEL
/* Warning: This function is called from interrupt context */
void lcd_buttons_update()
{
#ifdef PushButton
  uint8_t newbutton=0;
  static uint16_t pushButtonUpTimer=0;
  static uint16_t pushButtonDownTimer=0;
  
  if (!READ(PushButtonUp)) {
    newbutton|=EN_A;
    pushButtonUpTimer++;
    
    if (pushButtonUpTimer == 2) {
      encoderDiff--;
    }
    
    if (pushButtonUpTimer >= 500>>5) {
      if ((pushButtonUpTimer & (0x007f>>5))==0x0000) {
        encoderDiff--;
      }
      if (pushButtonUpTimer >= 3000>>5) {
        if ((pushButtonUpTimer & (0x000f>>5))==0x0000) {
          encoderDiff--;
        }
        if (pushButtonUpTimer >= 6000>>5) {
          if ((pushButtonUpTimer & (0x0003>>5))==0x0000) {
            encoderDiff--;
          }
        }
      }
      
    }
    

    
  }
  else{
    pushButtonUpTimer=0;
  }
  if (!READ(PushButtonDown)) {
    newbutton|=EN_B;
    pushButtonDownTimer++;
    
    if (pushButtonDownTimer == 2) {
      encoderDiff++;
    }
    
    if (pushButtonDownTimer >= 500>>5) {
      if ((pushButtonDownTimer & (0x007f>>5))==0x0000) {
        encoderDiff++;
      }
      
      if (pushButtonDownTimer >= 3000>>5) {
        if ((pushButtonDownTimer & (0x000f>>5))==0x0000) {
          encoderDiff++;
        }
        if (pushButtonDownTimer >= 6000>>5) {
          if ((pushButtonDownTimer & (0x0003>>5))==0x0000) {
            encoderDiff++;
          }
        }
      }
    }
    

    
  }
  else{
    pushButtonDownTimer=0;
  }
  if (!READ(PushButtonEnter)) {
    newbutton |= EN_C;
  }
  


  

  
  buttons = newbutton;

  
#else
#ifdef NEWPANEL
  uint8_t newbutton=0;
  if(READ(BTN_EN1)==0)  newbutton|=EN_A;
  if(READ(BTN_EN2)==0)  newbutton|=EN_B;
#if BTN_ENC > 0
#ifdef DreamMaker
  if(READ(BTN_ENC)==0)
    newbutton |= EN_C;
#else
  if((blocking_enc<millis()) && (READ(BTN_ENC)==0))
    newbutton |= EN_C;
#endif
#endif
#ifdef REPRAPWORLD_KEYPAD
  // for the reprapworld_keypad
  uint8_t newbutton_reprapworld_keypad=0;
  WRITE(SHIFT_LD,LOW);
  WRITE(SHIFT_LD,HIGH);
  for(int8_t i=0;i<8;i++) {
    newbutton_reprapworld_keypad = newbutton_reprapworld_keypad>>1;
    if(READ(SHIFT_OUT))
      newbutton_reprapworld_keypad|=(1<<7);
    WRITE(SHIFT_CLK,HIGH);
    WRITE(SHIFT_CLK,LOW);
  }
  newbutton |= ((~newbutton_reprapworld_keypad) << REPRAPWORLD_BTN_OFFSET); //invert it, because a pressed switch produces a logical 0
#endif
  buttons = newbutton;
#else   //read it from the shift register
  uint8_t newbutton=0;
  WRITE(SHIFT_LD,LOW);
  WRITE(SHIFT_LD,HIGH);
  unsigned char tmp_buttons=0;
  for(int8_t i=0;i<8;i++)
  {
    newbutton = newbutton>>1;
    if(READ(SHIFT_OUT))
      newbutton|=(1<<7);
    WRITE(SHIFT_CLK,HIGH);
    WRITE(SHIFT_CLK,LOW);
  }
  buttons=~newbutton; //invert it, because a pressed switch produces a logical 0
#endif//!NEWPANEL
  
  //manage encoder rotation
  uint8_t enc=0;
  if(buttons&EN_A)
    enc|=(1<<0);
  if(buttons&EN_B)
    enc|=(1<<1);
  if(enc != lastEncoderBits)
  {
    switch(enc)
    {
      case encrot0:
        if(lastEncoderBits==encrot3)
          encoderDiff++;
        else if(lastEncoderBits==encrot1)
          encoderDiff--;
        break;
      case encrot1:
        if(lastEncoderBits==encrot0)
          encoderDiff++;
        else if(lastEncoderBits==encrot2)
          encoderDiff--;
        break;
      case encrot2:
        if(lastEncoderBits==encrot1)
          encoderDiff++;
        else if(lastEncoderBits==encrot3)
          encoderDiff--;
        break;
      case encrot3:
        if(lastEncoderBits==encrot2)
          encoderDiff++;
        else if(lastEncoderBits==encrot0)
          encoderDiff--;
        break;
    }
  }
  lastEncoderBits = enc;
  
#endif
}

void lcd_buzz(long duration, uint16_t freq)
{
#ifdef LCD_USE_I2C_BUZZER
  lcd.buzz(duration,freq);
#endif
}

bool lcd_clicked()
{
  return LCD_CLICKED;
}
#endif//ULTIPANEL

/********************************/
/** Float conversion utilities **/
/********************************/
//  convert float to string with +123.4 format
char conv[8];
char *ftostr3(const float &x)
{
  return itostr3((int)x);
}

#ifdef DreamMaker

#ifdef DELTA
char *ftostr3_minus(const float &x)
{
  return itostr3_minus((int)x);
}
#endif

#endif 
char *itostr2(const uint8_t &x)
{
  //sprintf(conv,"%5.1f",x);
  int xx=x;
  conv[0]=(xx/10)%10+'0';
  conv[1]=(xx)%10+'0';
  conv[2]=0;
  return conv;
}

//  convert float to string with +123.4 format
char *ftostr31(const float &x)
{
  int xx=x*10;
  conv[0]=(xx>=0)?'+':'-';
  xx=abs(xx);
  conv[1]=(xx/1000)%10+'0';
  conv[2]=(xx/100)%10+'0';
  conv[3]=(xx/10)%10+'0';
  conv[4]='.';
  conv[5]=(xx)%10+'0';
  conv[6]=0;
  return conv;
}

//  convert float to string with 123.4 format
char *ftostr31ns(const float &x)
{
  int xx=x*10;
  //conv[0]=(xx>=0)?'+':'-';
  xx=abs(xx);
  conv[0]=(xx/1000)%10+'0';
  conv[1]=(xx/100)%10+'0';
  conv[2]=(xx/10)%10+'0';
  conv[3]='.';
  conv[4]=(xx)%10+'0';
  conv[5]=0;
  return conv;
}

char *ftostr32(const float &x)
{
  long xx=x*100;
  if (xx >= 0)
    conv[0]=(xx/10000)%10+'0';
  else
    conv[0]='-';
  xx=abs(xx);
  conv[1]=(xx/1000)%10+'0';
  conv[2]=(xx/100)%10+'0';
  conv[3]='.';
  conv[4]=(xx/10)%10+'0';
  conv[5]=(xx)%10+'0';
  conv[6]=0;
  return conv;
}

char *itostr31(const int &xx)
{
  conv[0]=(xx>=0)?'+':'-';
  conv[1]=(xx/1000)%10+'0';
  conv[2]=(xx/100)%10+'0';
  conv[3]=(xx/10)%10+'0';
  conv[4]='.';
  conv[5]=(xx)%10+'0';
  conv[6]=0;
  return conv;
}

char *itostr3(const int &xx)
{
  if (xx >= 100)
    conv[0]=(xx/100)%10+'0';
  else
    conv[0]=' ';
  if (xx >= 10)
    conv[1]=(xx/10)%10+'0';
  else
    conv[1]=' ';
  conv[2]=(xx)%10+'0';
  conv[3]=0;
  return conv;
}

#ifdef DreamMaker
#ifdef DELTA
char *itostr3_minus(const int &xx)
{
  conv[0]=(xx>=0)?'+':'-';
  int xxAbs=abs(xx);
  if (xxAbs >= 10)
    conv[1]=(xxAbs/10)%10+'0';
  else
    conv[1]=' ';
  conv[2]=(xxAbs)%10+'0';
  conv[3]=0;
  return conv;
}
#endif
#endif

char *itostr3left(const int &xx)
{
  if (xx >= 100)
  {
    conv[0]=(xx/100)%10+'0';
    conv[1]=(xx/10)%10+'0';
    conv[2]=(xx)%10+'0';
    conv[3]=0;
  }
  else if (xx >= 10)
  {
    conv[0]=(xx/10)%10+'0';
    conv[1]=(xx)%10+'0';
    conv[2]=0;
  }
  else
  {
    conv[0]=(xx)%10+'0';
    conv[1]=0;
  }
  return conv;
}

char *itostr4(const int &xx)
{
  if (xx >= 1000)
    conv[0]=(xx/1000)%10+'0';
  else
    conv[0]=' ';
  if (xx >= 100)
    conv[1]=(xx/100)%10+'0';
  else
    conv[1]=' ';
  if (xx >= 10)
    conv[2]=(xx/10)%10+'0';
  else
    conv[2]=' ';
  conv[3]=(xx)%10+'0';
  conv[4]=0;
  return conv;
}

//  convert float to string with 12345 format
char *ftostr5(const float &x)
{
  long xx=abs(x);
  if (xx >= 10000)
    conv[0]=(xx/10000)%10+'0';
  else
    conv[0]=' ';
  if (xx >= 1000)
    conv[1]=(xx/1000)%10+'0';
  else
    conv[1]=' ';
  if (xx >= 100)
    conv[2]=(xx/100)%10+'0';
  else
    conv[2]=' ';
  if (xx >= 10)
    conv[3]=(xx/10)%10+'0';
  else
    conv[3]=' ';
  conv[4]=(xx)%10+'0';
  conv[5]=0;
  return conv;
}

//  convert float to string with +1234.5 format
char *ftostr51(const float &x)
{
  long xx=x*10;
  conv[0]=(xx>=0)?'+':'-';
  xx=abs(xx);
  conv[1]=(xx/10000)%10+'0';
  conv[2]=(xx/1000)%10+'0';
  conv[3]=(xx/100)%10+'0';
  conv[4]=(xx/10)%10+'0';
  conv[5]='.';
  conv[6]=(xx)%10+'0';
  conv[7]=0;
  return conv;
}

//  convert float to string with +123.45 format
char *ftostr52(const float &x)
{
  long xx=x*100;
  conv[0]=(xx>=0)?'+':'-';
  xx=abs(xx);
  conv[1]=(xx/10000)%10+'0';
  conv[2]=(xx/1000)%10+'0';
  conv[3]=(xx/100)%10+'0';
  conv[4]='.';
  conv[5]=(xx/10)%10+'0';
  conv[6]=(xx)%10+'0';
  conv[7]=0;
  return conv;
}

// Callback for after editing PID i value
// grab the pid i value out of the temp variable; scale it; then update the PID driver
void copy_and_scalePID_i()
{
#ifdef PIDTEMP
  Ki = scalePID_i(raw_Ki);
  updatePID();
#endif
}

// Callback for after editing PID d value
// grab the pid d value out of the temp variable; scale it; then update the PID driver
void copy_and_scalePID_d()
{
#ifdef PIDTEMP
  Kd = scalePID_d(raw_Kd);
  updatePID();
#endif
}

#endif //ULTRA_LCD
