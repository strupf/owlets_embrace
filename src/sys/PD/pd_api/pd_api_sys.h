//
//  pdext_sys.h
//  Playdate Simulator
//
//  Created by Dave Hayden on 10/6/17.
//  Copyright © 2017 Panic, Inc. All rights reserved.
//

#ifndef pdext_sys_h
#define pdext_sys_h

#include <stdarg.h>

#if TARGET_EXTENSION

typedef enum
{
	kButtonLeft		= (1<<0),
	kButtonRight	= (1<<1),
	kButtonUp		= (1<<2),
	kButtonDown		= (1<<3),
	kButtonB		= (1<<4),
	kButtonA		= (1<<5)
} PDButtons;

typedef enum
{
	kPDLanguageEnglish,
	kPDLanguageJapanese,
	kPDLanguageUnknown,
} PDLanguage;

#endif

struct PDDateTime
{
	uint16_t year;
	uint8_t month; // 1-12
	uint8_t day; // 1-31
	uint8_t weekday; // 1=monday-7=sunday
	uint8_t hour; // 0-23
	uint8_t minute;
	uint8_t second;
};

typedef struct PDMenuItem PDMenuItem;

typedef enum
{
	kNone = 0,
	kAccelerometer	= (1<<0),
	// ...
	kAllPeripherals = 0xffff
} PDPeripherals;

typedef int PDCallbackFunction(void* userdata); // return 0 when done
typedef void PDMenuItemCallbackFunction(void* userdata);
typedef int PDButtonCallbackFunction(PDButtons button, int down, uint32_t when, void* userdata);

struct playdate_sys
{
	void* (*realloc)(void* ptr, size_t size); // ptr = NULL -> malloc, size = 0 -> free
	int (*formatString)(char **ret, const char *fmt, ...);
	void (*logToConsole)(const char* fmt, ...);
	void (*error)(const char* fmt, ...);
	PDLanguage (*getLanguage)(void);
	unsigned int (*getCurrentTimeMilliseconds)(void);
	unsigned int (*getSecondsSinceEpoch)(unsigned int *milliseconds);
	void (*drawFPS)(int x, int y);

	void (*setUpdateCallback)(PDCallbackFunction* update, void* userdata);
	void (*getButtonState)(PDButtons* current, PDButtons* pushed, PDButtons* released);
	void (*setPeripheralsEnabled)(PDPeripherals mask);
	void (*getAccelerometer)(float* outx, float* outy, float* outz);

	float (*getCrankChange)(void);
	float (*getCrankAngle)(void);
	int (*isCrankDocked)(void);
	int (*setCrankSoundsDisabled)(int flag); // returns previous setting

	int (*getFlipped)(void);
	void (*setAutoLockDisabled)(int disable);

	void (*setMenuImage)(LCDBitmap* bitmap, int xOffset);
	PDMenuItem* (*addMenuItem)(const char *title, PDMenuItemCallbackFunction* callback, void* userdata);
	PDMenuItem* (*addCheckmarkMenuItem)(const char *title, int value, PDMenuItemCallbackFunction* callback, void* userdata);
	PDMenuItem* (*addOptionsMenuItem)(const char *title, const char** optionTitles, int optionsCount, PDMenuItemCallbackFunction* f, void* userdata);
	void (*removeAllMenuItems)(void);
	void (*removeMenuItem)(PDMenuItem *menuItem);
	int (*getMenuItemValue)(PDMenuItem *menuItem);
	void (*setMenuItemValue)(PDMenuItem *menuItem, int value);
	const char* (*getMenuItemTitle)(PDMenuItem *menuItem);
	void (*setMenuItemTitle)(PDMenuItem *menuItem, const char *title);
	void* (*getMenuItemUserdata)(PDMenuItem *menuItem);
	void (*setMenuItemUserdata)(PDMenuItem *menuItem, void *ud);
	
	int (*getReduceFlashing)(void);
	
	// 1.1
	float (*getElapsedTime)(void);
	void (*resetElapsedTime)(void);

	// 1.4
	float (*getBatteryPercentage)(void);
	float (*getBatteryVoltage)(void);
	
	// 1.13
	int32_t (*getTimezoneOffset)(void);
	int (*shouldDisplay24HourTime)(void);
	void (*convertEpochToDateTime)(uint32_t epoch, struct PDDateTime* datetime);
	uint32_t (*convertDateTimeToEpoch)(struct PDDateTime* datetime);

	// 2.0
	void (*clearICache)(void);
	
	// 2.4
	void (*setButtonCallback)(PDButtonCallbackFunction* cb, void* buttonud, int queuesize);
	void (*setSerialMessageCallback)(void (*callback)(const char* data));
	int (*vaFormatString)(char **outstr, const char *fmt, va_list args);
	int (*parseString)(const char *str, const char *format, ...);
};

#endif /* pdext_sys_h */
