/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "mlink.h"

struct MLinkSensor
{
  const uint16_t id;
  const char * name;
  const TelemetryUnit unit;
  const uint8_t precision;
};

const MLinkSensor mlinkSensors[] = {
  {MLINK_SPECIAL,         STR_SENSOR_SPECIAL,           UNIT_RAW,               0},
  {MLINK_RX_VOLTAGE,      STR_SENSOR_BATT,              UNIT_VOLTS,             1},
  {MLINK_VOLTAGE,         STR_SENSOR_VFAS,              UNIT_VOLTS,             1},
  {MLINK_CURRENT,         STR_SENSOR_CURR,              UNIT_AMPS,              1},
  {MLINK_VARIO,           STR_SENSOR_VSPD,              UNIT_METERS_PER_SECOND, 1},
  {MLINK_SPEED,           STR_SENSOR_SPEED,             UNIT_KMH,               1},
  {MLINK_RPM,             STR_SENSOR_RPM,               UNIT_RPMS,              0},
  {MLINK_TEMP,            STR_SENSOR_TEMP1,             UNIT_CELSIUS,           1},
  {MLINK_HEADING,         STR_SENSOR_HDG,               UNIT_DEGREE,            1},
  {MLINK_ALT,             STR_SENSOR_ALT ,              UNIT_METERS,            0},
  {MLINK_FUEL,            STR_SENSOR_FUEL,              UNIT_PERCENT,           0},
  {MLINK_CAPACITY,        STR_SENSOR_CAPACITY,          UNIT_MAH,               0},
  {MLINK_FLOW,            STR_SENSOR_FLOW,              UNIT_MILLILITERS,       0},
  {MLINK_DISTANCE,        STR_SENSOR_DIST,              UNIT_KM,                1},
  {MLINK_GRATE,           STR_SENSOR_ACCX,              UNIT_G,                 1},
  {MLINK_LQI,             STR_SENSOR_RSSI,              UNIT_RAW,               0},
  {MLINK_LOSS,            STR_SENSOR_LOSS,              UNIT_RAW,               0},
  {MLINK_TX_RSSI,         STR_SENSOR_TX_RSSI,           UNIT_RAW,               0},
  {MLINK_TX_LQI,          STR_SENSOR_TX_QUALITY,        UNIT_RAW,               0},
};

const MLinkSensor * getMLinkSensor(uint16_t id)
{
  for (const MLinkSensor * sensor = mlinkSensors; sensor->id; sensor++) {
    if (id == sensor->id)
      return sensor;
  }
  return nullptr;
}

void processMLinkPacket(const uint8_t * packet)
{
  const uint8_t * data = packet + 2;

  // Multi telem
  setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_TX_RSSI, 0, 0, (packet[0] * 100) / 31, UNIT_RAW, 0);
  setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_TX_LQI, 0, 0, packet[1], UNIT_RAW, 0);

  // M-Link telem
  if (data[0] == 0x13) {  // Telemetry type RX-9
    for (uint8_t i = 1; i < 5; i += 3) {  //2 sensors per packet
      int32_t val = (int16_t )(data[i + 2] << 8 | data[i + 1]);
      val = val >> 1; // remove alarm flag
      uint8_t adress = (data[i] & 0xF0) >> 4;
      switch (data[i] & 0x0F) {
        case MLINK_SVC:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_SPECIAL, 0, 0, val & 0x007f, UNIT_RAW, 0);
          break;
        case MLINK_VOLTAGE:
          if ((data[i] & 0xF0) == 0x00){
            setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_RX_VOLTAGE, 0, adress, val, UNIT_VOLTS, 1);
          }
          else {
            setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_VOLTAGE, 0, adress, val, UNIT_VOLTS, 1);
          }
          break;
        case MLINK_CURRENT:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_CURRENT, 0, adress, val, UNIT_AMPS, 1);
          break;
        case MLINK_VARIO:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_VARIO, 0, adress, val, UNIT_METERS_PER_SECOND, 1);
          break;
        case MLINK_SPEED:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_SPEED, 0, adress, val, UNIT_KMH, 1);
          break;
        case MLINK_RPM:
          if (val < 0) {
            val = -val * 10;
          }
          else {
            val = val * 100;
          }
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_RPM, 0, adress, val, UNIT_RPMS, 0);
          break;
        case MLINK_TEMP:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_TEMP, 0, adress, val, UNIT_CELSIUS, 1);
          break;
        case MLINK_HEADING:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_HEADING, 0, adress, val, UNIT_DEGREE, 1);
          break;
        case MLINK_ALT:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_ALT, 0, adress, val, UNIT_METERS, 0);
          break;
        case MLINK_FUEL:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_FUEL, 0, adress, val, UNIT_PERCENT, 0);
          break;
        case MLINK_CAPACITY:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_CAPACITY, 0, adress, val, UNIT_MAH, 0);
          break;
        case MLINK_FLOW:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_FLOW, 0, adress, val, UNIT_MILLILITERS, 0);
          break;
        case MLINK_DISTANCE:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_DISTANCE, 0, adress, val, UNIT_KM, 1);
          break;
        case MLINK_GRATE:
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_GRATE, 0, adress, val, UNIT_G, 1);
          break;
        case MLINK_LQI:
          uint8_t mlinkRssi = data[i + 1] >> 1;
          setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_LQI, 0, 0, mlinkRssi, UNIT_RAW, 0);
          telemetryData.rssi.set(mlinkRssi);
          if (mlinkRssi > 0) {
            telemetryStreaming = TELEMETRY_TIMEOUT10ms;
          }
          break;
      }
    }
  }
  else if (packet[2] == 0x03) {  // Telemetry type RX-5
    uint16_t mlinkRssi = (packet[4] * 100) / 35;
    setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_LQI, 0, 0, mlinkRssi, UNIT_RAW, 0);
    telemetryData.rssi.set(mlinkRssi);
    if (mlinkRssi > 0) {
      telemetryStreaming = TELEMETRY_TIMEOUT10ms;
    }
    setTelemetryValue(PROTOCOL_TELEMETRY_MLINK, MLINK_LOSS, 0, 0, packet[7], UNIT_RAW, 0);
  }
}

void mlinkSetDefault(int index, uint16_t id, uint8_t subId, uint8_t instance)
{
  TelemetrySensor &telemetrySensor = g_model.telemetrySensors[index];
  telemetrySensor.id = id;
  telemetrySensor.subId = subId;
  telemetrySensor.instance = instance;

  const MLinkSensor * sensor = getMLinkSensor(id);
  if (sensor) {
    TelemetryUnit unit = sensor->unit;
    uint8_t prec = min<uint8_t>(2, sensor->precision);
    telemetrySensor.init(sensor->name, unit, prec);
    if (unit == UNIT_RPMS) {
      telemetrySensor.custom.ratio = 1;
      telemetrySensor.custom.offset = 1;
    }
  }
  else {
    telemetrySensor.init(id);
  }

  storageDirty(EE_MODEL);
}

void processExternalMlinkSerialData(void* ctx, uint8_t data, uint8_t* buffer, uint8_t* len)
{
  static bool destuff = false;

  if(*len > (2 * MSB_EXT_MODULE_PACKET_LEN)) {// shouldn't happen but
    *len = 0;                                 // prevent buffer overflow after max. 2 packets
    return;
  }

  if (data == MSB_ETX) {                      // start byte detected
    destuff = false;                          // init 
    *len = 0;
    return;
  }

  if(destuff) {                               // byte requires treatment
    buffer[(*len)++] = data - MSB_STUFF_OFFSET;
    destuff = false;
    return;
  }

  if(data == MSB_STUFF_ESC) {                 // ignore stuffing byte
    destuff = true;                           // and treat next byte
    return;
  }

  if(data != MSB_STX) {                       // store any other byte than end byte
    buffer[(*len)++] = data;
  } else {
    if(buffer[6] != MSB_NORMAL &&             // status must be normal mode with or without fast response
       buffer[6] != MSB_NORMAL_FAST &&        // or range test mode with or without fast response
       buffer[6] != MSB_RANGE &&              // to have valid telemetry    
       buffer[6] != MSB_RANGE_FAST )
       return;
    
    if(*len != (MSB_EXT_MODULE_PACKET_LEN-2))
      return;

    buffer[0] = 5;                            // dummy MLINK_TX_RSSI                   
    buffer[1] = 66;                           // dummy MLINK_TX_LQI
    buffer[2] = MSB_VALID_TELEMETRY;          // indicate valid telemetry
    buffer[3] = buffer[7];                    // first telemetry parameter
    buffer[4] = buffer[8];
    buffer[5] = buffer[9];
    buffer[6] = buffer[10];                   // second telemetry parameter
    buffer[7] = buffer[11];
    buffer[8] = buffer[12];

    processMLinkPacket(buffer);               // process telemetry packet
                                              // as if it came from MPM
  }
}
