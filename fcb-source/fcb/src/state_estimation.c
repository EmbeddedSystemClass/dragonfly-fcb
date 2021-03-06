/****************************************************************************
 * @file    state_estimation.c
 * @brief   Module implements the Kalman state estimation algorithm
 *
 * @license
 * Dragonfly FCB firmware to control the Dragonfly quadrotor UAV
 * Copyright (C) 2016  ÅF Technology South: Dragonfly Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *****************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "state_estimation.h"

#include "stm32f3xx.h"

#include "string.h"
#include "math.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "dragonfly_fcb.pb.h"
#include "pb_encode.h"

#include "fcb_sensors.h"
#include "fcb_gyroscope.h"
#include "fcb_error.h"
#include "rotation_transformation.h"
#include "fcb_accelerometer_magnetometer.h"
#include "fcb_retval.h"
#include "usbd_cdc_if.h"
#include "rotation_transformation.h"

/* Private define ------------------------------------------------------------*/
#define USE_CTRLSIGNAL_IN_PREDICTION_MODEL      0

#define STATE_PRINT_SAMPLING_TASK_PRIO          1
#define STATE_PRINT_MINIMUM_SAMPLING_TIME       20  // updated every 2.5 ms
#define STATE_PRINT_MAX_STRING_SIZE             256

enum {
    VAR_SAMPLE_MAX = 100
};
/* number of samples for variance - max 256 */
typedef struct FcbSensorVarianceCalc {
    float32_t samples[3][VAR_SAMPLE_MAX];
} FcbSensorVarianceCalcType;

/* Private variables ---------------------------------------------------------*/
static AttitudeStateVectorType rollState = { 0.0, 0.0, 0.0 };
static AttitudeStateVectorType pitchState = { 0.0, 0.0, 0.0 };
static AttitudeStateVectorType yawState = { 0.0, 0.0, 0.0 };

static AttitudeStateVectorType rollStateInternal = { 0.0, 0.0, 0.0 };
static AttitudeStateVectorType pitchStateInternal = { 0.0, 0.0, 0.0 };
static AttitudeStateVectorType yawStateInternal = { 0.0, 0.0, 0.0 };

static KalmanFilterType rollEstimator;
static KalmanFilterType pitchEstimator;
static KalmanFilterType yawEstimator;

/* Task handle for printing of sensor values task */
static volatile uint16_t statePrintSampleTime;
static volatile uint16_t statePrintSampleDuration;
xTaskHandle StatePrintSamplingTaskHandle = NULL;

static float32_t sensorAttitudeRPY[3] = { 0.0f, 0.0f, 0.0f };
static float32_t sensorAttitudeRateRPY[3] = { 0.0f, 0.0f, 0.0f };

static portTickType magLastCorrectionTick = 0;
static portTickType accLastCorrectionTick = 0;

/* Private function prototypes -----------------------------------------------*/
static void StateInit(KalmanFilterType * Estimator, float32_t q1, float32_t q2, float32_t q3, float32_t r1, float32_t r2);
static void PredictAttitudeState(KalmanFilterType* pEstimator, AttitudeStateVectorType * pState,
        float32_t const inertia, float32_t const ctrl, float32_t const tSinceLastCorrection);
static void CorrectAttitudeState(const float32_t sensorAngle, KalmanFilterType* pEstimator,
		AttitudeStateVectorType* pStateInternal, AttitudeStateVectorType* pState);
static void CorrectAttitudeRateState(const float32_t sensorRate, KalmanFilterType* pEstimator,
        AttitudeStateVectorType* pStateInternal, AttitudeStateVectorType* pState);
static void StatePrintSamplingTask(void const *argument);

/* Exported functions --------------------------------------------------------*/

/*
 * @brief  Initializes the Kalman state estimator
 * @param  None
 * @retval None
 */
void InitStatesXYZ(float32_t initAngles[3]) {
    StateInit(&rollEstimator, 	Q1_RP, 	Q2_RP, 	Q3_CAL, R1_ACCRP, 	GYRO_X_AXIS_VARIANCE);
    StateInit(&pitchEstimator, 	Q1_RP, 	Q2_RP, 	Q3_CAL, R1_ACCRP, 	GYRO_Y_AXIS_VARIANCE);
    StateInit(&yawEstimator, 	Q1_Y, 	Q2_Y, 	Q3_CAL, R1_MAG, 	GYRO_Z_AXIS_VARIANCE);

    rollState.angle = initAngles[0];
    rollState.angleRate = 0.0;
    rollState.angleRateBias = 0.0;
    rollState.angleRateUnbiased = rollState.angleRate - rollState.angleRateBias;

    pitchState.angle = initAngles[1];
    pitchState.angleRate = 0.0;
    pitchState.angleRateBias = 0.0;
    pitchState.angleRateUnbiased = pitchState.angleRate - pitchState.angleRateBias;

    yawState.angle = initAngles[2];
    yawState.angleRate = 0.0;
    yawState.angleRateBias = 0.0;
    yawState.angleRateUnbiased = yawState.angleRate - yawState.angleRateBias;

    rollStateInternal.angle = initAngles[0];
    rollStateInternal.angleRate = 0.0;
    rollStateInternal.angleRateBias = 0.0; // TODO init to gyroscope reading
    rollStateInternal.angleRateUnbiased = rollState.angleRate - rollState.angleRateBias;

    pitchStateInternal.angle = initAngles[1];
    pitchStateInternal.angleRate = 0.0;
    pitchStateInternal.angleRateBias = 0.0; // TODO init to gyroscope reading
    pitchStateInternal.angleRateUnbiased = pitchState.angleRate - pitchState.angleRateBias;

    yawStateInternal.angle = initAngles[2];
    yawStateInternal.angleRate = 0.0;
    yawStateInternal.angleRateBias = 0.0; // TODO init to gyroscope reading
    yawStateInternal.angleRateUnbiased = yawState.angleRate - yawState.angleRateBias;
}

/*
 * @brief  Initializes state estimation time-event generation timer
 * @param  None
 * @retval None
 */
StateEstimationStatus InitStateEstimationTimeEvent(void) {
    StateEstimationStatus status = STATE_EST_OK;

    /*##-1- Configure the TIM peripheral #######################################*/

    /* Set STATE_ESTIMATION_UPDATE_TIM instance */
    StateEstimationTimHandle.Instance = STATE_ESTIMATION_UPDATE_TIM;

    /* Initialize STATE_ESTIMATION_UPDATE_TIM peripheral */
    StateEstimationTimHandle.Init.Period = STATE_ESTIMATION_TIME_UPDATE_PERIOD;
    StateEstimationTimHandle.Init.Prescaler = STATE_ESTIMATION_TIME_UPDATE_PRESCALER;
    StateEstimationTimHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    StateEstimationTimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    if (HAL_TIM_Base_Init(&StateEstimationTimHandle) != HAL_OK) {
        /* Initialization Error */
        status = STATE_EST_ERROR;
        ErrorHandler();
    }

    /*##-2- Start the TIM Base generation in interrupt mode ####################*/
    if (HAL_TIM_Base_Start_IT(&StateEstimationTimHandle) != HAL_OK) {
        /* Starting Error */
        status = STATE_EST_ERROR;
        ErrorHandler();
    }

    return status;
}

/*
 * @brief  State estimation time-update
 * @param  None
 * @retval None
 */
void UpdatePredictionState(void) {
	uint32_t currentTick = xTaskGetTickCount();
	float32_t timeSinceLastAccCorrection = ((float32_t)currentTick - accLastCorrectionTick) / configTICK_RATE_HZ;
	float32_t timeSinceLastMagCorrection = ((float32_t)currentTick - magLastCorrectionTick) / configTICK_RATE_HZ;

	/* Run prediction step for attitude estimators */
    PredictAttitudeState(&rollEstimator, &rollStateInternal, IXX, GetRollControlSignal(), timeSinceLastAccCorrection);
    PredictAttitudeState(&pitchEstimator, &pitchStateInternal, IYY, GetPitchControlSignal(), timeSinceLastAccCorrection);
    PredictAttitudeState(&yawEstimator, &yawStateInternal, IZZ, GetYawControlSignal(), timeSinceLastMagCorrection);
}

/* GetRoll
 * @brief  Gets the roll angle
 * @param  None
 * @retval Roll angle state
 */
float32_t GetRollAngle(void) {
    return rollState.angle;
}

/* GetPitch
 * @brief  Gets the pitch angle
 * @param  None
 * @retval Pitch angle state
 */
float32_t GetPitchAngle(void) {
    return pitchState.angle;
}

/* GetYaw
 * @brief  Gets the yaw angle
 * @param  None
 * @retval Yaw angle state
 */
float32_t GetYawAngle(void) {
    return yawState.angle;
}

/*
 * @brief  Gets the (unbiased) roll angular rate
 * @param  None
 * @retval Roll rate
 */
float32_t GetRollRate(void) {
    return rollState.angleRateUnbiased;
}

/* GetPitch
 * @brief  Gets the (unbiased) pitch angular rate
 * @param  None
 * @retval Pitch rate
 */
float32_t GetPitchRate(void) {
    return pitchState.angleRateUnbiased;
}

/* GetYaw
 * @brief  Gets the (unbiased) yaw angular rate
 * @param  None
 * @retval Yaw rate
 */
float32_t GetYawRate(void) {
    return yawState.angleRateUnbiased;
}


/* Private functions ---------------------------------------------------------*/

/*
 * @brief  Initializes an angular state Kalman estimator
 * @param  Estimator : Kalman estimator struct
 * @param  q1 : Attitude state model noise variance
 * @param  q2 : Attitude rate state model noise variance
 * @param  q3 : Bias state model noise variance
 * @param  r1 : Attitude sensor noise variance
 * @param  r2 : Attitude rate sensor noise variance
 * @retval None
 */
static void StateInit(KalmanFilterType * Estimator, float32_t q1, float32_t q2, float32_t q3, float32_t r1, float32_t r2) {
    /* P matrix init is the Identity matrix*/
    Estimator->p11 = 0.1;
    Estimator->p12 = 0.0;
    Estimator->p13 = 0.0;
    Estimator->p21 = 0.0;
    Estimator->p22 = 1.0;
    Estimator->p23 = 0.0;
    Estimator->p31 = 0.0;
    Estimator->p32 = 0.0;
    Estimator->p33 = 0.01;

    Estimator->q1 = q1;
    Estimator->q2 = q2;
    Estimator->q3 = q3;
    Estimator->r1 = r1;
    Estimator->r2 = r2;

    Estimator->h = 1.0/((float32_t)(SystemCoreClock/(STATE_ESTIMATION_TIME_UPDATE_PERIOD+1)/STATE_ESTIMATION_TIME_UPDATE_PRESCALER));
}

/*
 * @brief  State estimation sensor value update
 * @param  sensorType : Type of sensor (accelerometer, gyroscope, magnetometer)
 * @param  pXYZ : Pointer to sensor values array
 * @retval None
 */
void UpdateCorrectionState(FcbSensorIndexType sensorType, float32_t const * pXYZ) {

    switch (sensorType) { /* interpret values according to sensor type */
    case GYRO_IDX: {
        /* run correction step */
        float32_t const * pSensorAngleRate = pXYZ;
        TransformationErrorStatus status = TRANSF_OK;
        status = GetEulerAngularRates(sensorAttitudeRateRPY, pSensorAngleRate, GetRollAngle(), GetPitchAngle());
        if (status != TRANSF_OK) {
            ErrorHandler();
        }

        CorrectAttitudeRateState(sensorAttitudeRateRPY[ROLL_IDX], &rollEstimator, &rollStateInternal, &rollState);
        CorrectAttitudeRateState(sensorAttitudeRateRPY[PITCH_IDX], &pitchEstimator, &pitchStateInternal, &pitchState);
        CorrectAttitudeRateState(sensorAttitudeRateRPY[YAW_IDX], &yawEstimator, &yawStateInternal, &yawState);
    }
        break;
    case ACC_IDX: {
        /* run correction step */
        float32_t const * pAccMeterXYZ = pXYZ; /* interpret values as accelerations */
        GetAttitudeFromAccelerometer(sensorAttitudeRPY, pAccMeterXYZ);
        CorrectAttitudeState(sensorAttitudeRPY[ROLL_IDX], &rollEstimator, &rollStateInternal, &rollState);
        CorrectAttitudeState(sensorAttitudeRPY[PITCH_IDX], &pitchEstimator, &pitchStateInternal, &pitchState);

        accLastCorrectionTick = xTaskGetTickCount();
    }
        break;
    case MAG_IDX: {
        /* run correction step */
        float32_t const * pMagMeter = pXYZ;
        sensorAttitudeRPY[YAW_IDX] = GetMagYawAngle((float32_t*) pMagMeter, GetRollAngle(), GetPitchAngle());
        CorrectAttitudeState(sensorAttitudeRPY[YAW_IDX], &yawEstimator, &yawStateInternal, &yawState);

        magLastCorrectionTick = xTaskGetTickCount();
    }
        break;
    case BARO_IDX: {

    }
    	break;
    default:
        ErrorHandler();
    }
}

/*
 * @brief	Performs the prediction step of the Kalman filtering.
 * @param 	pEstimator: Pointer to KalmanFilterType struct (roll pitch or yaw estimator)
 * @param 	pState: Pointer to struct member of AngleStateVectorType (roll, pitch or yaw)
 * @param   inertia: Rotational inertia around axis
 * @param   ctrl: Physical control action ([Nm] for attitude)
 * @retval 	None
 */
static void PredictAttitudeState(KalmanFilterType* pEstimator,
                                 AttitudeStateVectorType * pState,
                                 float32_t const inertia,
                                 float32_t const ctrl,
                                 float32_t const tSinceLastCorrection) {
    float32_t p11_tmp = pEstimator->p11;
    float32_t p12_tmp = pEstimator->p12;
    float32_t p13_tmp = pEstimator->p13;
    float32_t p21_tmp = pEstimator->p21;
    float32_t p22_tmp = pEstimator->p22;
    float32_t p23_tmp = pEstimator->p23;
    float32_t p31_tmp = pEstimator->p31;
    float32_t p32_tmp = pEstimator->p32;
    float32_t p33_tmp = pEstimator->p33;
    float32_t h = pEstimator->h;
    float32_t deltaT = MIN(pEstimator->h, tSinceLastCorrection);

    /* Prediction */
    /* Step 1: Calculate a priori state estimation*/

#if USE_CTRLSIGNAL_IN_PREDICTION_MODEL
    pState->angle += deltaT * (pState->angleRate - pState->angleRateBias) + h*h/(2 * inertia) * ctrl;
    pState->angleRate += h / inertia * ctrl;
#else
    pState->angle += deltaT * (pState->angleRate - pState->angleRateBias);
#endif

    /* pState->angleRateBias not estimated, see equations in section "State Estimation Theory" */
    pState->angleRateUnbiased = pState->angleRate - pState->angleRateBias; // Update the unbiased rate state
    toMaxRadian(&pState->angle);

    /* Step 2: Calculate a priori error covariance matrix P*/
    pEstimator->p11 = p11_tmp + h*(p12_tmp-p13_tmp+p21_tmp-p31_tmp) + h*h*(p22_tmp-p23_tmp-p32_tmp+p33_tmp) + pEstimator->q1;
    pEstimator->p12 = p12_tmp + h*(p22_tmp-p32_tmp);
    pEstimator->p13 = p13_tmp + h*(p23_tmp-p33_tmp);

    pEstimator->p21 = p21_tmp + h*(p22_tmp-p23_tmp);
    pEstimator->p22 = p22_tmp + pEstimator->q2;
    pEstimator->p23 = p23_tmp;

    pEstimator->p31 = p31_tmp + h*(p32_tmp-p33_tmp);
    pEstimator->p32 = p32_tmp;
    pEstimator->p33 = p33_tmp + pEstimator->q3;
}

/*
 * @brief	Performs the correction part of the Kalman filtering.
 * @param 	newAngle: Pointer to measured angle using accelerometer or magnetometer (roll, pitch or yaw)
 * @param 	pEstimator: Pointer to KalmanFilterType struct (roll pitch or yaw estimator)
 * @param 	stateAngle: Pointer to struct member of StateVectorType (roll, pitch or yaw)
 * @param 	stateRateBias: Pointer to struct member of StateVectorType (rollRateBias, pitchRateBias or yawRateBias)
 * @retval 	None
 */
static void CorrectAttitudeState(const float32_t sensorAngle, KalmanFilterType* pEstimator,
		AttitudeStateVectorType* pStateInternal, AttitudeStateVectorType* pState) {
    float32_t y1, s11, s12, s21, s22, InvDetS;
    float32_t p11_tmp, p12_tmp, p13_tmp, p21_tmp, p22_tmp, p23_tmp, p31_tmp, p32_tmp, p33_tmp;

    p11_tmp = pEstimator->p11; p12_tmp = pEstimator->p12; p13_tmp = pEstimator->p13;
    p21_tmp = pEstimator->p21; p22_tmp = pEstimator->p22; p23_tmp = pEstimator->p23;
    p31_tmp = pEstimator->p31; p32_tmp = pEstimator->p32; p33_tmp = pEstimator->p33;

    /* Correction */
    /* Step3: Calculate y, difference between a-priori state and measurement z */
    y1 = sensorAngle - pStateInternal->angle;
    toMaxRadian(&y1);

    /* Step 4: Calculate innovation covariance matrix S */
    s11 = p11_tmp + pEstimator->r1;
    s12 = p12_tmp;
    s21 = p21_tmp;
    s22 = p22_tmp + pEstimator->r2;

    /* Step 5: Calculate Kalman gain */
    InvDetS = 1/(s11*s22 - s12*s21);
    pEstimator->k11 = InvDetS * (p11_tmp*s22 - p12_tmp*s21);
    pEstimator->k21 = InvDetS * (p21_tmp*s22 - p22_tmp*s21);
    pEstimator->k31 = InvDetS * (p31_tmp*s22 - p32_tmp*s21);

    /* Step 6: Update a posteriori state estimation */
    pStateInternal->angle += pEstimator->k11*y1;
    pStateInternal->angleRate += pEstimator->k21*y1;
    pStateInternal->angleRateBias += pEstimator->k31*y1;
    toMaxRadian(&pStateInternal->angle);

    /* Step 7: Update a posteriori error covariance matrix P
     * NOTE: This is only half of the P matrix update, i.e. the parts that are related to the attitude measurement */

    pEstimator->p11 = p11_tmp - p11_tmp*pEstimator->k11;
    pEstimator->p12 = p12_tmp - p12_tmp*pEstimator->k11;
    pEstimator->p13 = p13_tmp - p13_tmp*pEstimator->k11;

    pEstimator->p21 = p21_tmp - p11_tmp*pEstimator->k21;
    pEstimator->p22 = p22_tmp - p12_tmp*pEstimator->k21;
    pEstimator->p23 = p23_tmp - p13_tmp*pEstimator->k21;

    pEstimator->p31 = p31_tmp - p11_tmp*pEstimator->k31;
    pEstimator->p32 = p32_tmp - p12_tmp*pEstimator->k31;
    pEstimator->p33 = p33_tmp - p13_tmp*pEstimator->k31;

    /* Update real states (i.e. filter output) by copying internal state from correction */
    pState->angle = pStateInternal->angle;
}

/*
 * @brief   Performs the correction part of the Kalman filtering.
 * @param   newAngle: Pointer to measured angle using accelerometer or magnetometer (roll, pitch or yaw)
 * @param   pEstimator: Pointer to KalmanFilterType struct (roll pitch or yaw estimator)
 * @param   stateAngle: Pointer to struct member of StateVectorType (roll, pitch or yaw)
 * @param   stateRateBias: Pointer to struct member of StateVectorType (rollRateBias, pitchRateBias or yawRateBias)
 * @retval  None
 */
static void CorrectAttitudeRateState(const float32_t sensorRate, KalmanFilterType* pEstimator,
        AttitudeStateVectorType* pStateInternal, AttitudeStateVectorType* pState) {
    float32_t y2, s11, s12, s21, s22, InvDetS;
    float32_t p11_tmp, p12_tmp, p13_tmp, p21_tmp, p22_tmp, p23_tmp, p31_tmp, p32_tmp, p33_tmp;

    p11_tmp = pEstimator->p11; p12_tmp = pEstimator->p12; p13_tmp = pEstimator->p13;
    p21_tmp = pEstimator->p21; p22_tmp = pEstimator->p22; p23_tmp = pEstimator->p23;
    p31_tmp = pEstimator->p31; p32_tmp = pEstimator->p32; p33_tmp = pEstimator->p33;

    /* Correction */
    /* Step3: Calculate y, difference between a-priori state and measurement z */
    y2 = sensorRate - pStateInternal->angleRate;

    /* Step 4: Calculate innovation covariance matrix S */
    s11 = p11_tmp + pEstimator->r1;
    s12 = p12_tmp;
    s21 = p21_tmp;
    s22 = p22_tmp + pEstimator->r2;

    /* Step 5: Calculate Kalman gains */
    InvDetS = 1/(s11*s22 - s12*s21);
    pEstimator->k12 = InvDetS * (p12_tmp*s11 - p11_tmp*s12);
    pEstimator->k22 = InvDetS * (p22_tmp*s11 - p21_tmp*s12);
    pEstimator->k32 = InvDetS * (p32_tmp*s11 + p31_tmp*s12);

    /* Step 6: Update a posteriori state estimation */
    pStateInternal->angle = pStateInternal->angle + pEstimator->k12 * y2;
    pStateInternal->angleRate = pStateInternal->angleRate + pEstimator->k22 * y2;
    pStateInternal->angleRateBias = pStateInternal->angleRateBias + pEstimator->k32 * y2;
    pStateInternal->angleRateUnbiased = pStateInternal->angleRate - pStateInternal->angleRateBias; // Update the unbiased rate state

    /* Step 7: Update a posteriori error covariance matrix P
     * NOTE: This is only half of the P matrix update, i.e. the parts that are related to the attitude rate measurement */
    pEstimator->p11 = p11_tmp - p21_tmp*pEstimator->k12;
    pEstimator->p12 = p12_tmp - p22_tmp*pEstimator->k12;
    pEstimator->p13 = p13_tmp - p23_tmp*pEstimator->k12;

    pEstimator->p21 = p21_tmp - p21_tmp*pEstimator->k22;
    pEstimator->p22 = p22_tmp - p22_tmp*pEstimator->k22;
    pEstimator->p23 = p23_tmp - p23_tmp*pEstimator->k22;

    pEstimator->p31 = p31_tmp - p21_tmp*pEstimator->k32;
    pEstimator->p32 = p32_tmp - p22_tmp*pEstimator->k32;
    pEstimator->p33 = p33_tmp - p23_tmp*pEstimator->k32;

    /* Update real states (i.e. filter output) by copying internal state from correction */
    pState->angleRate = pStateInternal->angleRate;
    pState->angleRateBias = pStateInternal->angleRateBias;
    pState->angleRateUnbiased = pStateInternal->angleRateUnbiased;
}

///////////////////////////////////////////////////////////////////////////////
//                 Debug printing functions
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief  Task code handles state (angle, rate, ratebias) print sampling
 * @param  argument : Unused parameter
 * @retval None
 */
static void StatePrintSamplingTask(void const *argument) {
    (void) argument;

    portTickType xLastWakeTime;
    portTickType xSampleStartTime;

    /* Initialize the xLastWakeTime variable with the current time */
    xLastWakeTime = xTaskGetTickCount();
    xSampleStartTime = xLastWakeTime;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, statePrintSampleTime);

        PrintStateValues();

        /* If sampling duration exceeded, delete task to stop sampling */
        if (xTaskGetTickCount() >= xSampleStartTime + statePrintSampleDuration * configTICK_RATE_HZ)
            StopStateSamplingTask();
    }
}

/*
 * @brief  Creates a task to print states over USB.
 * @param  sampleTime : Sets how often a sample should be printed.
 * @param  sampleDuration : Sets for how long sampling should be performed.
 * @retval MOTORCTRL_OK if thread started, else MOTORCTRL_ERROR.
 */
FcbRetValType StartStateSamplingTask(const uint16_t sampleTime, const uint32_t sampleDuration) {

    // TODO do not start a new task if one is already running, just update sampleTime/sampleDuration

    if (sampleTime < STATE_PRINT_MINIMUM_SAMPLING_TIME) {
        statePrintSampleTime = STATE_PRINT_MINIMUM_SAMPLING_TIME;
    } else {
        statePrintSampleTime = sampleTime;
    }

    statePrintSampleDuration = sampleDuration;

    /* State value print sampling handler thread creation
     * Task function pointer: StatePrintSamplingTask
     * Task name: STATE_PRINT_SAMPL
     * Stack depth: 2*configMINIMAL_STACK_SIZE
     * Parameter: NULL
     * Priority: STATE_PRINT_SAMPLING_TASK_PRIO (0 to configMAX_PRIORITIES-1 possible)
     * Handle: StatePrintSamplingTaskHandle
     **/
    if (pdPASS
            != xTaskCreate((pdTASK_CODE )StatePrintSamplingTask, (signed portCHAR*)"STATE_PRINT_SAMPL",
                    3*configMINIMAL_STACK_SIZE, NULL, STATE_PRINT_SAMPLING_TASK_PRIO, &StatePrintSamplingTaskHandle)) {
        ErrorHandler();
        return FCB_ERR;
    }
    return FCB_OK;
}

/*
 * @brief  Stops motor control print sampling by deleting the task.
 * @param  None.
 * @retval MOTORCTRL_OK if task deleted, MOTORCTRL_ERROR if not.
 */
FcbRetValType StopStateSamplingTask(void) {
    if (StatePrintSamplingTaskHandle != NULL) {
        vTaskDelete(StatePrintSamplingTaskHandle);
        StatePrintSamplingTaskHandle = NULL;
        return FCB_OK;
    }
    return FCB_ERR;
}

/*
 * @brief  Prints the state values over USB com
 * @param  serializationType: Data serialization type enum
 * @retval None
 */
void PrintStateValues(void) {
    static char stateString[STATE_PRINT_MAX_STRING_SIZE]; // TODO when debug printing is cleaned up, this shouldn't be needed as static

    snprintf((char*) stateString, STATE_PRINT_MAX_STRING_SIZE,
            "States [deg / deg/s]:\nroll: %1.3f\npitch: %1.3f\nyaw: %1.3f\nrollRate: %1.3f\npitchRate: %1.3f\nyawRate: %1.3f\nrollRateBias: %1.3f\npitchRateBias: %1.3f\nyawRateBias: %1.3f\r\n",
            Radian2Degree(rollState.angle), Radian2Degree(pitchState.angle), Radian2Degree(yawState.angle),
            Radian2Degree(rollState.angleRate), Radian2Degree(pitchState.angleRate), Radian2Degree(yawState.angleRate),
            Radian2Degree(rollState.angleRateBias), Radian2Degree(pitchState.angleRateBias), Radian2Degree(yawState.angleRateBias));

    USBComSendString(stateString); // Send string over USB
}


/**
 * @}
 */

/**
 * @}
 */
/*****END OF FILE****/
