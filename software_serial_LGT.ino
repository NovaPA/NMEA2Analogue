

typedef enum NMPARSE_MESSAGE_TYPE {
  NMEA_MESSAGE_TYPE_UNKNOWN,
  NMEA_MESSAGE_TYPE_VHW,
  NMEA_MESSAGE_TYPE_VTG
} NMPARSE_MESSAGE_TYPE;

typedef struct NMPARSE_MESSAGE {
  NMPARSE_MESSAGE_TYPE type;
  float speed_water_knots;
  float speed_water_kilometers;
  float speed_ground_knots;
  float speed_ground_kilometers;
} NMPARSE_MESSAGE;

/**
 * Validate checksum of NMEA message stored in the buffer
 *
 * @param[in] buffer - input buffer
 * @param[in] buflen - input buffer length
 *
 * @return true on success, false on error
 */
bool nmparse_checksum_validate(const char *buffer, int buflen)
{
  char *toval = strchr(buffer, '*');
  char *head = (char *)buffer + 1;

  int cs = 0;
  while (head != toval)
  {
    cs ^= *head;
    head++;
  }

  char* end;
  if (strtol(toval + 1, &end, 16) == cs)
  {
    return true;
  }

  return false;
}

/**
 * Get start position and length of NMEA message stored in the buffer
 *
 * @param[in] buffer - input buffer
 * @param[in] buflen - input buffer length
 * @param[out] nmeastart - start offset of NMEA message
 * @param[out] nmealen - length of NMEA message
 *
 * @return true on success, false on error
 */
bool nmparse_peek(const char *buffer, int buflen, int *nmeastart, int *nmealen)
{
  if (!nmeastart || !nmealen)
  {
    return false;
  }

  *nmeastart = 0;
  *nmealen = 0;

  bool has_start = false;
  for (int i = 0; i < buflen; i++)
  {
    if (buffer[i] == '$')
    {
      *nmeastart = i;
      has_start = true;
    }
  }

  if (has_start)
  {
     int spos = *nmeastart;
    for (int i = spos; i < buflen; i++)
    {
      if (buffer[i] == '\n')
      {
        if (i < 5)
        {
          return false;
        }
        *nmealen = i + 1;
        if (nmparse_checksum_validate(buffer + spos, *nmealen))
        {
          return true;
        }
      }
    }
  }

  return false;
}

/**
 * Get type of NMEA message stored in the buffer
 *
 * @param[in] buffer - input buffer
 * @param[in] buflen - input buffer length
 *
 * @return NMEA message type
 */
NMPARSE_MESSAGE_TYPE nmparse_get_type(const char *buffer, int buflen)
{
  if (!buffer || buflen < 5)
  {
    return NMEA_MESSAGE_TYPE_UNKNOWN;
  }

  for (int i = 0; i < buflen; i++)
  {
    if (buffer[i] == '$')
    {
      if (!strncmp(buffer + i + 3, "VHW", 3))
      {
        return NMEA_MESSAGE_TYPE_VHW;
      }
      else if (!strncmp(buffer + i + 3, "VTG", 3))
      {
        return NMEA_MESSAGE_TYPE_VTG;
      }
    }
  }

  return NMEA_MESSAGE_TYPE_UNKNOWN;
}

/**
 * Parse NMEA message
 *
 * @param[in] buffer - input buffer
 * @param[in] buflen - input buffer length
 * @param[inout] result - parse result
 *
 * @return NMEA message type
 */
void nmparse_parse_core(const char *buffer, int buflen, NMPARSE_MESSAGE *result)
{
  int comma_num = 0;
  char *param_start = NULL;
  char *param_end = NULL;
  switch (result->type)
  {
    case NMEA_MESSAGE_TYPE_VHW:
    for (int i = 0; i < buflen; i++)
    {
      if (buffer[i] == ',')
      {
        comma_num++;
        if (comma_num == 5)
        {
          result->speed_water_knots = atof(buffer + i + 1);
        }
        else if (comma_num == 7)
        {
          result->speed_water_kilometers = atof(buffer + i + 1);
          return;
        }
      }
    }
    break;
    case NMEA_MESSAGE_TYPE_VTG:
    for (int i = 0; i < buflen; i++)
    {
      if (buffer[i] == ',')
      {
        comma_num++;
        if (comma_num == 5)
        {
          result->speed_ground_knots = atof(buffer + i + 1);
        }
        else if (comma_num == 7)
        {
          result->speed_ground_kilometers = atof(buffer + i + 1);
          return;
        }
      }
    }
    break;
    default:
    break;
  }
}

/**
 * Try to parse NMEA message from buffer
 *
 * @param[in] buffer - input buffer
 * @param[in] buflen - input buffer length
 * @param[out] result - parse result
 *
 * @return true on success, false on error
 */
bool nmparse_try_parse(const char *buffer, int buflen, NMPARSE_MESSAGE *result)
{
  if (!buffer || !result)
  {
    return false;
  }

  memset(result, 0, sizeof(NMPARSE_MESSAGE));

  int pos;
  int len;
  if (nmparse_peek(buffer, buflen, &pos, &len))
  {
    buffer += pos;
    result->type = nmparse_get_type(buffer, len);
    if (result->type != NMEA_MESSAGE_TYPE_UNKNOWN)
    {
      nmparse_parse_core(buffer, len, result);
    }
    return true;
  }

  return false;
}

/*
 * [MAIN]
 * MCU code starts here
 *
*/



#define NMEA_MAX_MSG_LENGTH 82
#define SERIAL_BUFFER_SIZE 64

static NMPARSE_MESSAGE s_current_result;

static float vout;
static int anaout;
    float zeroset;
static int s_s0_collected = 0;
static char s_s0_rx[(NMEA_MAX_MSG_LENGTH * 2) + (SERIAL_BUFFER_SIZE + 1)];

//static int s_s1_collected = 0;
//static char s_s1_rx[(NMEA_MAX_MSG_LENGTH * 2) + (SERIAL_BUFFER_SIZE + 1)];

void setup()
{
  memset(s_s0_rx, 0, sizeof(s_s0_rx));
  //memset(s_s1_rx, 0, sizeof(s_s1_rx));
  memset(&s_current_result, 0, sizeof(NMPARSE_MESSAGE));

  pinMode(0, INPUT);
  pinMode(1, OUTPUT);
  pinMode (5, OUTPUT);
  pinMode (4, OUTPUT);
  pinMode (8, OUTPUT);
  Serial.begin(4800);

  for (int ze=0; ze<1000; ze=ze+1) {
    if (ze<500) {digitalWrite(8, HIGH); digitalWrite(4, LOW); };
    if (ze>500) {digitalWrite(4, HIGH); digitalWrite(8, LOW); };
    int ana=analogRead(A2);
   zeroset = static_cast<float>(ana) / 1024 * 0.7;
    Serial.println(zeroset);
      analogWrite(5, zeroset*255/5);
      
  }

  
}

void loop()
{
  int avail = Serial.available();
  NMPARSE_MESSAGE result;
digitalWrite(8, LOW); digitalWrite(4, LOW);
  if (avail)
  {

    if ((s_s0_collected + avail) > (sizeof(s_s0_rx)))
    {
      //Buffer overflow. This should never happen during normal operation, so just force reset the buffer
      memset(s_s0_rx, 0, sizeof(s_s0_rx));
      digitalWrite(8, LOW); digitalWrite(4, HIGH);
      s_s0_collected = 0;
    }

    s_s0_collected += Serial.readBytes(s_s0_rx + s_s0_collected, avail);
    if (nmparse_try_parse(s_s0_rx, s_s0_collected, &result))
    {
      if (result.type != NMEA_MESSAGE_TYPE_UNKNOWN);

      {
        memcpy(&s_current_result, &result, sizeof(NMPARSE_MESSAGE));
              digitalWrite(8, HIGH); digitalWrite(4, LOW);
      }
      memset(s_s0_rx, 0, sizeof(s_s0_rx));
      s_s0_collected = 0;
      digitalWrite(4, HIGH); digitalWrite(8, LOW);
    }
      Serial.println(s_current_result.speed_water_knots);
      digitalWrite(8, LOW); digitalWrite(4, HIGH);
  } else {digitalWrite(8, LOW); digitalWrite(4, HIGH);}

  // OUTPUT HERE FROM s_current_result
  // ...

  vout = s_current_result.speed_water_knots * 0.0875 + zeroset - s_current_result.speed_water_knots * s_current_result.speed_water_knots * 0.0008;
  anaout = 255/5*vout;

  analogWrite(5, anaout);


  //s_softSerial.print("knots discreets: ");
  //s_softSerial.println(anaout);

}
