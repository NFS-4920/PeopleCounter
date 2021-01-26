//------------------------------------------------------------------------------
// Status
#define NOBODY                    0
#define SOMEONE                   1
#define LEFT                      0
#define RIGHT                     1

//------------------------------------------------------------------------------
// Modes
#define DOOR_JAM_2400             1
#define DOOR_JAM_2000             2
#define ON_SIDE                   3
#define DISTANCE_MODE_LONG        2

//------------------------------------------------------------------------------
// DOOR_JAM_2400 Profile
#if PPC_PROFILE == DOOR_JAM_2400
#define PROFILE_STRING                               "DOOR_JAM_2400"
#define DISTANCES_ARRAY_SIZE                         10    // nb of samples
#define MAX_DISTANCE                                 2500  // mm
#define MIN_DISTANCE                                 0     // mm
#define DIST_THRESHOLD                               2000  // mm
#define ROWS_OF_SPADS                                8     // 8x16 SPADs ROI
#define TIMING_BUDGET                                20    // was 20 ms, I found 33 ms has better succes rate with lower reflectance target
#define DISTANCE_MODE                                DISTANCE_MODE_LONG
#endif

//------------------------------------------------------------------------------
// SPADS configs
#if ROWS_OF_SPADS == 4
#define FRONT_ZONE_CENTER                            151
#define BACK_ZONE_CENTER                             247
#elif ROWS_OF_SPADS == 6
#define FRONT_ZONE_CENTER                            159
#define BACK_ZONE_CENTER                             239
#elif ROWS_OF_SPADS == 8
#define FRONT_ZONE_CENTER                            175 // was 167, see UM2555 on st.com, centre = 175 has better return signal rate for the ROI #1
#define BACK_ZONE_CENTER                             231
#endif

//------------------------------------------------------------------------------
// People Counting variables
int PplCounter = 0;
int center[2] = {FRONT_ZONE_CENTER, BACK_ZONE_CENTER}; /* these are the spad center of the 2 4*16 zones */
int Zone = 0;

//------------------------------------------------------------------------------
// Leds variables (Extra)
unsigned long lon = 0;
bool          len = 0;

//+=============================================================================
// People Counting Logic
//
int ProcessPeopleCountingData(int16_t Distance, uint8_t zone, uint8_t RangeStatus) {
  static int PathTrack[] = {0, 0, 0, 0};
  static int PathTrackFillingSize = 1; // init this to 1 as we start from state where nobody is any of the zones
  static int LeftPreviousStatus = NOBODY;
  static int RightPreviousStatus = NOBODY;
  static int PeopleCount = 0;
  static uint16_t Distances[2][DISTANCES_ARRAY_SIZE];
  static uint8_t DistancesTableSize[2] = {0, 0};
  uint16_t MinDistance;
  uint8_t i;

  (void)RangeStatus;

#ifdef TRACE_PPC
  #define TIMES_WITH_NO_EVENT 10// was 40
  static uint32_t trace_count = TIMES_WITH_NO_EVENT;  // replace by 0 if you want to trace the first TIMES_WITH_NO_EVENT values
#endif

  int CurrentZoneStatus = NOBODY;
  int AllZonesCurrentStatus = 0;
  int AnEventHasOccured = 0;

  // Add just picked distance to the table of the corresponding zone
  if (DistancesTableSize[zone] < DISTANCES_ARRAY_SIZE)
  {
    Distances[zone][DistancesTableSize[zone]] = Distance;
    DistancesTableSize[zone] ++;
  } else
  {
    for (i = 1; i < DISTANCES_ARRAY_SIZE; i++)
      Distances[zone][i - 1] = Distances[zone][i];
    Distances[zone][DISTANCES_ARRAY_SIZE - 1] = Distance;
  }

  // pick up the min distance
  MinDistance = Distances[zone][0];
  if (DistancesTableSize[zone] >= 2)
  {
    for (i = 1; i < DistancesTableSize[zone]; i++)
    {
      if (Distances[zone][i] < MinDistance)
        MinDistance = Distances[zone][i];
    }
  }

  if (MinDistance < DIST_THRESHOLD)
  {
    // Someone is in !
    CurrentZoneStatus = SOMEONE;
  }

  if (zone == LEFT) // left zone
  {
    if (CurrentZoneStatus != LeftPreviousStatus)
    {
      // event in left zone has occured
      AnEventHasOccured = 1;

      if (CurrentZoneStatus == SOMEONE)
      {
        AllZonesCurrentStatus += 1;
      }
      // need to check right zone as well ...
      if (RightPreviousStatus == SOMEONE)
      {
        // event in left zone has occured
        AllZonesCurrentStatus += 2;
      }
      // remember for next time
      LeftPreviousStatus = CurrentZoneStatus;
    }
  } else // right zone
  {

    if (CurrentZoneStatus != RightPreviousStatus)
    {
      // event in left zone has occured
      AnEventHasOccured = 1;
      if (CurrentZoneStatus == SOMEONE)
      {
        AllZonesCurrentStatus += 2;
      }
      // need to left right zone as well ...
      if (LeftPreviousStatus == SOMEONE)
      {
        // event in left zone has occured
        AllZonesCurrentStatus += 1;
      }
      // remember for next time
      RightPreviousStatus = CurrentZoneStatus;
    }
  }

#ifdef TRACE_PPC
  // print debug data only when someone is within the field of view
  trace_count++;
  if ((CurrentZoneStatus == SOMEONE) || (LeftPreviousStatus == SOMEONE) || (RightPreviousStatus == SOMEONE))
    trace_count = 0;

  if (trace_count < TIMES_WITH_NO_EVENT)
  {
    Serial.println("ZONE: " + String(zone) + " -- Distance: " + String(Distance) + "mm");
    /*
    Serial.print(zone);
    Serial.print(",");
    Serial.print(Distance);
    Serial.print(",");
    Serial.print(MinDistance);
    Serial.print(",");
    Serial.print(RangeStatus);
    Serial.print(",");
    Serial.println(PeopleCount);
    */
  }
#endif

  // if an event has occured
  if (AnEventHasOccured)
  {
    if (PathTrackFillingSize < 4)
    {
      PathTrackFillingSize ++;
    }

    // if nobody anywhere lets check if an exit or entry has happened
    if ((LeftPreviousStatus == NOBODY) && (RightPreviousStatus == NOBODY))
    {
      // check exit or entry only if PathTrackFillingSize is 4 (for example 0 1 3 2) and last event is 0 (nobobdy anywhere)
      if (PathTrackFillingSize == 4)
      {
        // check exit or entry. no need to check PathTrack[0] == 0 , it is always the case
        if ((PathTrack[1] == 1)  && (PathTrack[2] == 3) && (PathTrack[3] == 2))
        {
          // This an entry
          PeopleCount ++;
          // reset the table filling size in case an entry or exit just found
          DistancesTableSize[0] = 0;
          DistancesTableSize[1] = 0;
          Serial.print("Walk In, People Count=");
          Serial.println(PeopleCount);
          digitalWrite(LEDV, HIGH);
          Serial.println("Z0:" + String(PathTrack[0]) + " -- Z1:" + String(PathTrack[1]) + " -- Z2:" + String(PathTrack[2]) + + " -- Z3:" + String(PathTrack[3]));
          len = 1; 
          lon = millis();
        } else if ((PathTrack[1] == 2)  && (PathTrack[2] == 3) && (PathTrack[3] == 1))
        {
          // This an exit
          PeopleCount --;
          // reset the table filling size in case an entry or exit just found
          DistancesTableSize[0] = 0;
          DistancesTableSize[1] = 0;
          Serial.print("Walk Out, People Count=");
          Serial.println(PeopleCount);
          Serial.println("Z0:" + String(PathTrack[0]) + " -- Z1:" + String(PathTrack[1]) + " -- Z2:" + String(PathTrack[2]) + + " -- Z3:" + String(PathTrack[3]));
          digitalWrite(LEDR, HIGH);
          lon = millis();
          len = 1;
          
        } else
        {
          // reset the table filling size also in case of unexpected path
          DistancesTableSize[0] = 0;
          DistancesTableSize[1] = 0;
          Serial.println("Wrong path");
          Serial.println("Z0:" + String(PathTrack[0]) + " -- Z1:" + String(PathTrack[1]) + " -- Z2:" + String(PathTrack[2]) + + " -- Z3:" + String(PathTrack[3]));
        }
      }

      PathTrackFillingSize = 1;
    } else
    {
      // update PathTrack
      // example of PathTrack update
      // 0
      // 0 1
      // 0 1 3
      // 0 1 3 1
      // 0 1 3 3
      // 0 1 3 2 ==> if next is 0 : check if exit
      PathTrack[PathTrackFillingSize - 1] = AllZonesCurrentStatus;
    }
  }

  // output debug data to main host machine
  return (PeopleCount);
}
