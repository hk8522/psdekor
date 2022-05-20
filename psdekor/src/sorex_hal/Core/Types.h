// Defines basic types.

#ifndef __TYPES_H
#define __TYPES_H

#include <stdbool.h>

//! \todo: native types lowercase (e.g. byte), others CamelCase e.g. Status,

//! \todo Types für ms & 16bit int ?


/*!
 * \brief Defines a byte as numeric type. Different to the <b>char</b>
 *        type according to the value type:
 *
 * - byte: 0 .. 255
 * - char: 'A' .. 'Z', 'a' .. 'z', '0' .. '9', ...
 */
typedef char byte;

/*!
 * \brief Defines a byte as percentage. Different to the <b>char</b> type
 *        according to the value type: percentage: 0 .. 100
 */
typedef char Percentage;

/*!
 * \brief Return value of all functions. S_OK indicates success, while all
 *        other values indicated an error.
 */
typedef char Status;

/*!
 * \brief Identifies a "unit" such as RFID chip etc.
 */
typedef char UnitId;


/*!
 * \brief Identifies a "User"
 */
typedef uint16_t UserId;

/*!
 * \enum Mode
 * \brief Defines a general operation mode.
 */
typedef enum {
	Off = 0,     /**<= 0 Deactivate something */
	On = 1       /**<= 1 Activate something */
} Mode;

/*!
 * \enum TransferMode
 * \brief TransferMode defines R/W
 */
typedef enum {
	Read = 0,       /**<= 0 Read from a source. */
	Write = 1       /**<= 1 Write to a destination. */
} TransferMode;

/**
 * \enum Orientation
 * \brief Defines directions.

 */
typedef enum {
	Left = 0,    /**<= 0 LEFT */
	Right = 1,   /**<= 1 RIGHT */
	Up = 2,      /**<= 2 UP */
	Down = 3,    /**<= 3 DOWN */
} Orientation;

/**
 * \enum DataMode
 * \brief Defines data-transfer mode.
 */
typedef enum {
	Master = 0,  /**< Master Mode */
	Slave = 1    /**< Slave Mode */
} DataMode;

 /**
 * \enum Parity
 * \brief Defines the parity Mode
 */
typedef enum {
	NoParity,    /**<No Parity Bit */
	EvenParity,  /**<Even Parity Bit */
	OddParity    /**<Odd Parity Bit*/
} Parity;

 /**
 * \enum Baud
 * \brief Defines the BAUD rate for a Transmission
 */
typedef enum {
	Baud9600,     /**<9600 Baud */
	Baud19200,    /**<19200 Baud */
	Baud38400,    /**<38400 Baud */
	Baud57600,    /**<57600 Baud*/
	Baud112500,   /**<112500 Baud*/
} Baud;


/**
 * \struct PwmSettings
 * \brief Defines A PWM control for the Motor or other stuff
 *
 *example:
 *- PwmSettings InitialMotorSettings;
 * - InitialMotorSettings.DutyCycle = 50;
 * - InitialMotorSettings.ActiveTime = 200;
 * - InitialMotorSettings.CurrentLimit = 120;
 * - InitialMotorSettings.CurrentMessure = True;
 */
typedef struct {
	Percentage DutyCycle;/**< in percentage 0...100% */
	byte ActiveTime; /**<for how long the PWM should last [ms] */
	byte CurrentLimit;	/**<The current Limit for the Motor in [mA] */
	bool CurrentMessure; /**<should the current be measured?   */
	byte LastMessuredCurrent; /**<saves the last messured Current in that
				   *  setting */

}PwmSettings;

#endif // __TYPES_H
