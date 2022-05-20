/*
 * sounds.c
 *
 * Created: 19.11.2013 17:57:29
 *  Author: huber
 */ 
#include "buzzer/sounds.h"

const char SignalTestOn[] = "i";
const char SignalTestOff[] = "u";

const char SignalModeFast[] = "e-";
const char SignalModeSlow[] = "eee---";

const char SignalPositive[] = "ui.";
const char SignalNegative[] = "abababab";

const char SignalAllDeleted[] = ".EEEEE";

const char SignalVccLow[] = "VVVVVVVVVVVVVVV";

const char SignalRepeatedAlarm[] = "EEEEE";

const char SignalFirstAlarm[] = "i--i--i--i--i--i--i----EEEE--";

const char SignalSetSoftware[] = "beeeebeeeeb";

const char SignalError[] = "Eaaa-Eaaa-";

const char SignalWakeUp[] = "e.";

const char SignalHello[] = "e-";

static struct tone_setting settings[] = {{
		.key             = 'e' /* duty = 0.500000 */,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 290,
		.repetitions     = 59,
		.ccreg           = 145,
		.control         = FADE_DISABLED_gc,
		.fade_step       = 0 /* diff = 0 */,
	}, {
		.key             = 'a' /* duty = 0.498695 */,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 383,
		.repetitions     = 248,
		.ccreg           = 191,
		.control         = FADE_DISABLED_gc,
		.fade_step       = 0 /* diff = 0 */,
	}, {
		.key             = 'b' /* duty = 0.000000 */,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 1021,
		.repetitions     = 60,
		.ccreg           = 2000,
		.control         = FADE_DISABLED_gc,
		.fade_step       = 0 /* diff = 0 */,
	}, {
		.key             = 'u' /* start_duty = 0.498695, stop_duty = 0.986945 off = 1*/,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 383,
		.repetitions     = 499,
		.ccreg           = 191,
		.control         = FADE_IN_gc | FADE_DIV_8_gc,
		.fade_step       = 3 /* diff = 1 */,
	}, {
		.key             = 'i' /* start_duty = 0.500000, stop_duty = 0.989655 off = 0*/,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 290,
		.repetitions     = 379,
		.ccreg           = 145,
		.control         = FADE_IN_gc | FADE_DIV_8_gc,
		.fade_step       = 3 /* diff = 0 */,
	}, {
		.key             = 'E' /* duty = 0.500000 */,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 290,
		.repetitions     = 862,
		.ccreg           = 145,
		.control         = FADE_DISABLED_gc,
		.fade_step       = 0 /* diff = 0 */,
	}, {
		.key             = 'V' /* duty = 0.500000 */,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 300,
		.repetitions     = 3333,
		.ccreg           = 150,
		.control         = FADE_DISABLED_gc,
		.fade_step       = 0 /* diff = 0 */,
	}, {
		.key             = '.' /* duty = 0.000000 */,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 1021,
		.repetitions     = 30,
		.ccreg           = 2000,
		.control         = FADE_DISABLED_gc,
		.fade_step       = 0 /* diff = 0 */,
	}, {
		.key             = '-' /* duty = 0.000000 */,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 1021,
		.repetitions     = 300,
		.ccreg           = 2000,
		.control         = FADE_DISABLED_gc,
		.fade_step       = 0 /* diff = 0 */,
	}, {
		.key             = 'a' /* duty = 0.498695 */,
		.divider         = TC_CLKSEL_DIV1_gc,
		.per             = 383,
		.repetitions     = 131,
		.ccreg           = 191,
		.control         = FADE_DISABLED_gc,
		.fade_step       = 0 /* diff = 0 */,
	}
};

extern s8 SoundsInitialize (void)
{
	return buzzerInitialize (settings, sizeof(settings) /
	                                   sizeof(settings[0]));
}
