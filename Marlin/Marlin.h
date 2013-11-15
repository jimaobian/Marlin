///
/// @mainpage	Marlin
///
/// @details	Description of the project
/// @n			Library
/// @n
/// @n @a		Developed with [embedXcode+](http://embedXcode.weebly.com)
///
/// @author		qiao
/// @author		qiao
///
/// @date		11/14/13 5:24 PM
/// @version	<#version#>
///
/// @copyright	(c) qiao, 2013
/// @copyright	GNU General Public License
///
/// @see		ReadMe.txt for references
///


///
/// @file		Marlin.h
/// @brief		Library header
///
/// @details	<#details#>
/// @n	
/// @n @b		Project Marlin
/// @n @a		Developed with [embedXcode+](http://embedXcode.weebly.com)
/// 
/// @author		qiao
/// @author		qiao
///
/// @date		11/14/13 5:24 PM
/// @version	<#version#>
/// 
/// @copyright	(c) qiao, 2013
/// @copyright	GNU General Public License
///
/// @see		ReadMe.txt for references
///


// Core library - IDE-based
#if defined(MPIDE) // chipKIT specific
#include "WProgram.h"
#elif defined(DIGISPARK) // Digispark specific
#include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad MSP430, Stellaris and Tiva, Experimeter Board FR5739 specific
#include "Energia.h"
#elif defined(MAPLE_IDE) // Maple specific
#include "WProgram.h"
#elif defined(CORE_TEENSY) // Teensy specific
#include "WProgram.h"
#elif defined(WIRING) // Wiring specific
#include "Wiring.h"
#elif defined(ARDUINO) && (ARDUINO >= 100) // Arduino 1.0x and 1.5x specific
#include "Arduino.h"
#elif defined(ARDUINO) && (ARDUINO < 100)  // Arduino 23 specific
#include "WProgram.h"
#endif // end IDE

#ifndef Marlin_RELEASE
#define Marlin_RELEASE 100


#endif
