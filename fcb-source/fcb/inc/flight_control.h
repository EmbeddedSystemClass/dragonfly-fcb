/******************************************************************************
 * @file    flight_control.h
 * @author  Dragonfly
 * @version v. 1.0.0
 * @date    2015-08-12
 * @brief   Header file for flight control
 ******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FLIGHT_CONTROL_H
#define __FLIGHT_CONTROL_H

/* Includes ------------------------------------------------------------------*/
#include "arm_math.h"
#include "fcb_sensors.h"

/* Exported constants --------------------------------------------------------*/

#define FLIGHT_CONTROL_TASK_PERIOD		10 // [ms]

/* Physical properties of aircraft */
// TODO make physical properties struct
#define LENGTH_ARM 	        0.30		// Quadcopter arm length [m] // TODO use int values mm, g, g/(m^2)
#define MASS		        2.100		// Total mass of the quadcopter [kg]
#define IXX					0.030		// X-axis moment of inertia [kg/(m^2)]
#define IYY					0.030		// Y-axis moment of inertia [kg/(m^2)]
#define IZZ					0.060		// Z-axis moment of inertia [kg/(m^2)]

#define G_ACC				(float)	   	9.815					/* Gravitational acceleration constant approx. 9.815 m/s^2 in
																 * Smygehuk, Sweden (according to Lantm�teriet) */
#define COMPASS_DECLINATION		   (float)		3.226*PI/180.0	/* For Malmoe, Sweden the compass declination is about 3.226 deg East
 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 * The total field strength is 50552 nT (505.52 mGauss) */
/* Reference signal ranges */
#define MAX_Z_VELOCITY			2.0			// Max vertical velocity (+/-) [m/s] // TODO use int values m/s, mrad, mrad/s
#define MAX_ROLLPITCH_ANGLE 	15*PI/180	// Max roll/pitch angle (+/-) [rad] (NOTE! Not in deg!)
#define MAX_YAW_RATE			30*PI/180	// Max yaw angle rate [rad/s] (NOTE! Not deg/s)

#define RECEIVER_TO_REFERENCE_ZERO_PADDING	1800	// Sets how large an area around 0 receiver value the reference signal should be set to zero

enum FlightControlMode {
	FLIGHT_CONTROL_IDLE,
	FLIGHT_CONTROL_RAW,
	FLIGHT_CONTROL_PID
};

/* Exported types ------------------------------------------------------------*/

typedef enum {
	FLIGHTCTRL_ERROR = 0, FLIGHTCTRL_OK = !FLIGHTCTRL_ERROR
} FlightControlErrorStatus;

/* Exported macro ------------------------------------------------------------*/

/* Exported function prototypes --------------------------------------------- */
void CreateFlightControlTask(void);
enum FlightControlMode GetFlightControlMode(void);
float GetZVelocityReferenceSignal(void);
float GetRollAngleReferenceSignal(void);
float GetPitchAngleReferenceSignal(void);
float GetYawAngularRateReferenceSignal(void);
float32_t GetThrustControlSignal();
float32_t GetRollControlSignal();
float32_t GetPitchControlSignal();
float32_t GetYawControlSignal();

void SendFlightControlUpdateToFlightControl(void);
/**
 * This function sends a message to the flight control queue to indicate that a new prediction shall be calculated.
 */
void SendPredictionUpdateToFlightControl(void);

/**
 * This function sends a message to the flight control queue with new sensor values.
 *
 * @param sensorType see FcbSensorIndexType
 * @param deltaT time period to previous sensor drdy in ms
 * @param xyz a 3-array of XYZ sensor readings. See wiki page "Sensors"
 */
void SendCorrectionUpdateToFlightControl(FcbSensorIndexType sensorType, uint8_t deltaTms, float32_t xyz[3]);

#endif /* __FLIGHT_CONTROL_H */

/**
 * @}
 */

/**
 * @}
 */
/*****END OF FILE****/
