//------------------------------------------------------------------------------
// Library headers
#include <Arduino.h>
#include <Wire.h>
#include <vl53l1x_class.h>


//------------------------------------------------------------------------------
// Pinout
#define LEDR                      15                      
#define LEDV                      2
#define XSHUT_PIN                 5
#define GPIO1_PIN                 18

//------------------------------------------------------------------------------
// Configuration
#define PPC_PROFILE               DOOR_JAM_2400
#define TRACE_PPC                 0
#include "PeopleCounting.h"

//------------------------------------------------------------------------------
// Sensor
VL53L1X *sensor_vl53l1_top;

//+=============================================================================
// Setup
//
void setup()
{
  Serial.begin(115200);
  Serial.println("\nStarting...");

  pinMode(LEDR, OUTPUT);
  pinMode(LEDV, OUTPUT);
  
  // Initialize I2C bus.
  Wire.begin();

  // Create VL53L1X top component.
  sensor_vl53l1_top = new VL53L1X(&Wire, XSHUT_PIN, GPIO1_PIN);

  // Switch off VL53L1X top component.
  sensor_vl53l1_top->VL53L1_Off();

  //Initialize all the sensors
  sensor_vl53l1_top->InitSensor(0x10);

  sensor_vl53l1_top->VL53L1X_SetDistanceMode(DISTANCE_MODE); /* 1=short, 2=long */
  sensor_vl53l1_top->VL53L1X_SetTimingBudgetInMs(TIMING_BUDGET); /* in ms possible values [15, 20, 50, 100, 200, 500] */
  sensor_vl53l1_top->VL53L1X_SetInterMeasurementInMs(TIMING_BUDGET);
  sensor_vl53l1_top->VL53L1X_SetROI(ROWS_OF_SPADS, 16); /* minimum ROI 4,4 */
  Serial.println("...");
  sensor_vl53l1_top->VL53L1X_StartRanging();   /* This function has to be called to enable the ranging */
}

void loop()
{
  int status;
  uint16_t Distance, Signal;
  uint8_t RangeStatus;
  uint8_t dataReady = 0;

  if(((millis() - lon) > 2000) && len){
    digitalWrite(LEDV, LOW);
    digitalWrite(LEDR, LOW);
  }
  
  //Poll for measurament completion top sensor
  while (dataReady == 0)
  {
    status = sensor_vl53l1_top->VL53L1X_CheckForDataReady(&dataReady);
    delay(1);
  }

  status += sensor_vl53l1_top->VL53L1X_GetRangeStatus(&RangeStatus);
  status += sensor_vl53l1_top->VL53L1X_GetDistance(&Distance);
  status += sensor_vl53l1_top->VL53L1X_GetSignalPerSpad(&Signal);
  status += sensor_vl53l1_top->VL53L1X_ClearInterrupt(); /* clear interrupt has to be called to enable next interrupt*/

  if (status != 0)
  {
    Serial.println("Error in operating the device");
  }

  status = sensor_vl53l1_top->VL53L1X_SetROICenter(center[Zone]);
  if (status != 0)
  {
    Serial.println("Error in chaning the center of the ROI");
    while(1)
    {
    }
  }

  // check the status of the ranging. In case of error, lets assume the distance is the max of the use case
  // Value RangeStatus string Comment
  // 0 VL53L1_RANGESTATUS_RANGE_VALID Ranging measurement is valid
  // 1 VL53L1_RANGESTATUS_SIGMA_FAIL Raised if sigma estimator check is above the internal defined threshold
  // 2 VL53L1_RANGESTATUS_SIGNAL_FAIL Raised if signal value is below the internal defined threshold
  // 4 VL53L1_RANGESTATUS_OUTOFBOUNDS_ FAIL Raised when phase is out of bounds
  // 5 VL53L1_RANGESTATUS_HARDWARE_FAIL Raised in case of HW or VCSEL failure
  // 7 VL53L1_RANGESTATUS_WRAP_TARGET_ FAIL Wrapped target, not matching phases
  // 8 VL53L1_RANGESTATUS_PROCESSING_ FAIL Internal algorithm underflow or overflow
  // 14 VL53L1_RANGESTATUS_RANGE_INVALID The reported range is invalid
  if ((RangeStatus == 0) || (RangeStatus == 4) || (RangeStatus == 7))
  {
    if (Distance <= MIN_DISTANCE) // wraparound case see the explanation at the constants definition place
      Distance = MAX_DISTANCE + MIN_DISTANCE;
  } else // severe error cases
  {
    Distance = MAX_DISTANCE;
  }

  // inject the new ranged distance in the people counting algorithm
  PplCounter = ProcessPeopleCountingData(Distance, Zone, RangeStatus);
  
  
 
  Zone++;
  Zone = Zone%2;
}
