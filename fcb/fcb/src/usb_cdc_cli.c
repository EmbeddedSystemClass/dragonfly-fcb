/******************************************************************************
 * @file    usb_cdc_cli.c
 * @author  �F Dragonfly
 * @version v. 1.0.0
 * @date    2015-07-24
 * @brief   File contains functionality to use the USB CDC class with a
 *          Command Line Interface (CLI). Each command is associated with
 *          number of command parameters and a function which executes command
 *          activities. The CLI used is based on the FreeRTOS Plus CLI API.
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usb_cdc_cli.h"
#include "receiver.h"
#include "fifo_buffer.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "FreeRTOS_CLI.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MAX_DATA_TRANSFER_DELAY         1000 // [ms]

/* Private function prototypes -----------------------------------------------*/

/*
 * Function implements the "echo" command.
 */
static portBASE_TYPE CLIEchoCommandFunction( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Function implements the "echo-data" command.
 */
static portBASE_TYPE CLIEchoDataCommandFunction( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Function implements the "start-receiver-calibration" command.
 */
static portBASE_TYPE CLIStartReceiverCalibrationCommandFunction( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/*
 * Function implements the "stop-receiver-calibration" command.
 */
static portBASE_TYPE CLIStopReceiverCalibrationCommandFunction( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/* Private variables ---------------------------------------------------------*/

/* Structure that defines the "echo" command line command. */
static const CLI_Command_Definition_t echoCommand =
{
        ( const int8_t * const ) "echo",
        ( const int8_t * const ) "\r\necho <param>:\r\n Echoes one parameter\r\n",
        CLIEchoCommandFunction, /* The function to run. */
        1 /* Number of parameters expected */
};

/* Structure that defines the "echo-data" command line command. */
static const CLI_Command_Definition_t echoDataCommand =
{
        ( const int8_t * const ) "echo-data",
        ( const int8_t * const ) "\r\necho-data <param:data size>:\r\n Echoes input data with size specified by command parameter\r\n",
        CLIEchoDataCommandFunction, /* The function to run. */
        1 /* Number of parameters expected */
};

/* Structure that defines the "start-receiver-calibration" command line command. */
static const CLI_Command_Definition_t startReceiverCalibrationCommand =
{
        ( const int8_t * const ) "start-receiver-calibration",
        ( const int8_t * const ) "\r\nstart-receiver-calibration:\r\n Starts the receiver calibration procedure\r\n",
        CLIStartReceiverCalibrationCommandFunction, /* The function to run. */
        0 /* Number of parameters expected */
};

/* Structure that defines the "stop-receiver-calibration" command line command. */
static const CLI_Command_Definition_t stopReceiverCalibrationCommand =
{
        ( const int8_t * const ) "stop-receiver-calibration",
        ( const int8_t * const ) "\r\nstop-receiver-calibration:\r\n Stops the receiver calibration procedure\r\n",
        CLIStopReceiverCalibrationCommandFunction, /* The function to run. */
        0 /* Number of parameters expected */
};

extern volatile FIFOBuffer_TypeDef USBCOMRxFIFOBuffer;
extern xSemaphoreHandle USBCOMRxDataSem;

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Implements the CLI command to echo one parameter
  * @param  pcWriteBuffer : Reference to output buffer
  * @param  xWriteBufferLen : Size of output buffer
  * @param  pcCommandString : Command line string
  * @retval pdTRUE if more data follows, pdFALSE if command activity finished
  */
static portBASE_TYPE CLIEchoCommandFunction(int8_t* pcWriteBuffer, size_t xWriteBufferLen, const int8_t* pcCommandString )
{
  int8_t* pcParameter;
  portBASE_TYPE xParameterStringLength, xReturn;
  static portBASE_TYPE lParameterNumber = 0;

  /* Remove compile time warnings about unused parameters, and check the
        write buffer is not NULL.  NOTE - for simplicity, this example assumes the
        write buffer length is adequate, so does not check for buffer overflows. */
  (void) pcCommandString;
  (void) xWriteBufferLen;
  configASSERT( pcWriteBuffer );

  if(lParameterNumber == 0)
    {
      /* The first time the function is called after the command has been
                entered just a header string is returned. */
      strcpy((char*) pcWriteBuffer, "Parameter received:\r\n");

      /* Next time the function is called the parameter will be echoed back */
      lParameterNumber++;

      /* There is more data to be returned as the parameter has not been echoed back yet */
      xReturn = pdTRUE;
    }
  else
    {
      /* Obtain the parameter string */
      pcParameter = (int8_t*) FreeRTOS_CLIGetParameter
          (
              pcCommandString,                /* The command string itself. */
              lParameterNumber,               /* Return the next parameter. */
              &xParameterStringLength         /* Store the parameter string length. */
          );

      /* Sanity check something was returned. */
      configASSERT(pcParameter);

      /* Return the parameter string. */
      memset(pcWriteBuffer, 0x00, xWriteBufferLen);
      strncat((char*) pcWriteBuffer, (const char*) pcParameter, xParameterStringLength);
      strncat((char*) pcWriteBuffer, "\r\n", strlen("\r\n"));

      /* Update return value and parameter index */
      if(lParameterNumber == echoCommand.cExpectedNumberOfParameters)
        {
          /* If this is the last parameter then there are no more strings to return after this one. */
          xReturn = pdFALSE;
          lParameterNumber = 0;
        }
      else
        {
          /* There are more parameters to return after this one. */
          xReturn = pdTRUE;
          lParameterNumber++;
        }
    }

  return xReturn;
}

/**
  * @brief  Implements the CLI command to echo a specific amount of data (size specified by parameter)
  * @param  pcWriteBuffer : Reference to output buffer
  * @param  xWriteBufferLen : Size of output buffer
  * @param  pcCommandString : Command line string
  * @retval pdTRUE if more data follows, pdFALSE if command activity finished
  */
static portBASE_TYPE CLIEchoDataCommandFunction(int8_t* pcWriteBuffer, size_t xWriteBufferLen, const int8_t* pcCommandString )
{
  int8_t *pcParameter;
  portBASE_TYPE xParameterStringLength, xReturn;
  static portBASE_TYPE lParameterNumber = 0;

  /* Remove compile time warnings about unused parameters, and check the
        write buffer is not NULL.  NOTE - for simplicity, this example assumes the
        write buffer length is adequate, so does not check for buffer overflows. */
  (void) pcCommandString;
  (void) xWriteBufferLen;
  configASSERT(pcWriteBuffer);

  if(lParameterNumber == 0)
    {
      /* The first time the function is called after the command has been
                entered just a header string is returned. */
      strcpy((char*) pcWriteBuffer, "Received data:\r\n");

      /* Next time the function is called the parameter will be echoed
                back. */
      lParameterNumber++;

      /* There is more data to be returned as data has not been echoed back yet. */
      xReturn = pdTRUE;
    }
  else
    {
      /* Obtain the parameter string. */
      pcParameter = (int8_t *) FreeRTOS_CLIGetParameter
          (
              pcCommandString,                /* The command string itself. */
              lParameterNumber,               /* Return the next parameter. */
              &xParameterStringLength         /* Store the parameter string length. */
          );

      /* Sanity check something was returned. */
      configASSERT(pcParameter);

      // TODO convert pcParameter string to equivalent int (implement atoi i common.c needed?)
      // TODO Also check so that pcParameter is a valid int
      int dataLength = atoi((char*)pcParameter);

      // TODO: Pend on USBComRxDataSem for MAX_DATA_TRANSFER_DELAY while waiting for data to enter RX buffer
      // TODO: Note to self, where to read the received data? RxDataBuffer???
      // TODO: Only pend on semaphore if data count in FIFO buffer is less than specified by command param
      if(dataLength > USBCOMRxFIFOBuffer.count && pdPASS == xSemaphoreTake(USBCOMRxDataSem, MAX_DATA_TRANSFER_DELAY))
        {
          // Read out new data
        }
      else
        {
          // TODO check if all data received
          /* If all data received Return the data */
          memset(pcWriteBuffer, 0x00, xWriteBufferLen);
          strncat((char*) pcWriteBuffer, (const char*) pcParameter, xParameterStringLength);
          strncat((char*) pcWriteBuffer, "\r\n", strlen("\r\n"));

          // If not all data received, then the semaphore has timed out. Erase the writeBuffer

          // TODO If more contents is present in buffer after specified length, give semaphore to wake up RX thread to parse new commands
        }

      /* Update return value and parameter index */
      if( lParameterNumber == echoCommand.cExpectedNumberOfParameters )
        {
          /* If this is the last of the three parameters then there are no more strings to return after this one. */
          xReturn = pdFALSE;
          lParameterNumber = 0;
        }
      else
        {
          /* There are more parameters to return after this one. */
          xReturn = pdTRUE;
          lParameterNumber++;
        }
    }


  return xReturn;
}

/**
  * @brief  Implements the CLI command to start receiver calibration
  * @param  pcWriteBuffer : Reference to output buffer
  * @param  xWriteBufferLen : Size of output buffer
  * @param  pcCommandString : Command line string
  * @retval pdTRUE if more data follows, pdFALSE if command activity finished
  */
static portBASE_TYPE CLIStartReceiverCalibrationCommandFunction(int8_t* pcWriteBuffer, size_t xWriteBufferLen, const int8_t* pcCommandString)
{
//  int8_t* pcParameter;
//  portBASE_TYPE xParameterStringLength;
  portBASE_TYPE xReturn;
  static portBASE_TYPE lParameterNumber = 0;

  /* Remove compile time warnings about unused parameters, and check the
        write buffer is not NULL.  NOTE - for simplicity, this example assumes the
        write buffer length is adequate, so does not check for buffer overflows. */
  (void) pcCommandString;
  (void) xWriteBufferLen;
  configASSERT( pcWriteBuffer );

  if(lParameterNumber == 0)
    {
      /* Start the receiver calibration procedure */
      if(StartReceiverCalibration())
        strcpy((char*)pcWriteBuffer, "RC receiver calibration started. Please saturate all RC transmitter control sticks and toggle switches.\r\n");
      else
        strcpy((char*)pcWriteBuffer, "RC receiver calibration could not be started.\r\n");

      /* Return false so that this function is not called again */
      xReturn = pdFALSE;
    }

  return xReturn;
}

/**
  * @brief  Implements the CLI command to stop receiver calibration
  * @param  pcWriteBuffer : Reference to output buffer
  * @param  xWriteBufferLen : Size of output buffer
  * @param  pcCommandString : Command line string
  * @retval pdTRUE if more data follows, pdFALSE if command activity finished
  */
static portBASE_TYPE CLIStopReceiverCalibrationCommandFunction(int8_t* pcWriteBuffer, size_t xWriteBufferLen, const int8_t* pcCommandString )
{
//  int8_t* pcParameter;
//  portBASE_TYPE xParameterStringLength;
  portBASE_TYPE xReturn;
  static portBASE_TYPE lParameterNumber = 0;

  /* Remove compile time warnings about unused parameters, and check the
        write buffer is not NULL.  NOTE - for simplicity, this example assumes the
        write buffer length is adequate, so does not check for buffer overflows. */
  (void) pcCommandString;
  (void) xWriteBufferLen;
  configASSERT( pcWriteBuffer );

  if(lParameterNumber == 0)
    {
      /* Stop the receiver calibration procedure */
      if(StopReceiverCalibration())
        strcpy((char*)pcWriteBuffer, "RC receiver calibration stopped.\r\n");
      else
        strcpy((char*)pcWriteBuffer, "RC receiver calibration has to be started before it can be stopped.\r\n");

      /* Return false so that this function is not called again */
      xReturn = pdFALSE;
    }

  return xReturn;
}

/* Exported functions --------------------------------------------------------*/

void RegisterCLICommands(void)
{
  /* Register all the command line commands defined immediately above. */
  FreeRTOS_CLIRegisterCommand(&echoCommand);
  FreeRTOS_CLIRegisterCommand(&echoDataCommand);
  FreeRTOS_CLIRegisterCommand(&startReceiverCalibrationCommand);
  FreeRTOS_CLIRegisterCommand(&stopReceiverCalibrationCommand);
}

/**
 * @}
 */

/**
 * @}
 */
/*****END OF FILE****/
