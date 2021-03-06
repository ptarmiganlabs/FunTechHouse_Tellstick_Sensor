/**
 * @file HumiditySensor.cpp
 * @author Johan Simonsson
 * @brief A humidity and temperature sensor class with alarm logic
 */

/*
 * Copyright (C) 2014 Johan Simonsson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <QString>

#include "HumiditySensor.h"
#include "UnixTime.h"

/**
 * Default constructur,
 * please note that all alarm are disabled.
 */
HumiditySensor::HumiditySensor()
{
        valueSendTime = 0;

        temperatureDiffMax = 0.3;
        temperatureOffset  = 0;

        humidityDiffMax = 2;
        humidityOffset  = 0;

        alarmHighTemperatureActive = false;
        alarmLowTemperatureActive  = false;
        alarmHighHumidityActive    = false;
        alarmLowHumidityActive     = false;

        alarmHighTemperatureSent = false;
        alarmLowTemperatureSent  = false;
        alarmHighHumiditySent    = false;
        alarmLowHumiditySent     = false;

        alarmHystTemperature = 5.0;
        alarmHystHumidity    = 5.0;
}


bool HumiditySensor::timeToSend(QString temperature, QString humidity, char* mqttstr, int maxsize)
{
    bool res = valueTimeToSend(temperature.toDouble(), humidity.toDouble());

    if(res)
    {
        snprintf(mqttstr, maxsize, "temperature=%.02f ; rh=%.02f%c", 
                temperatureWork, humidityWork, '%');
    }
    return res;
}

/**
 * Is it time to send a new value to the server,
 * this is triggered either on change or timeout.
 *
 * @param temperature The new temperature in deg C
 * @param humidity The new relative humidity in 0..100%
 * @return true if it is time to send
 */
bool HumiditySensor::valueTimeToSend(double temperature, double humidity)
{
    temperatureWork = temperature + temperatureOffset;
    humidityWork    = humidity    + humidityOffset;

    //Timeout lets send anyway
    if( (0 == valueSendTime) || ((UnixTime::get()-valueSendTime)>=ALWAYS_SEND_TIMEOUT) )
    {
        temperatureSent = temperatureWork;
        humiditySent    = humidityWork;
        valueSendTime = UnixTime::get();
        return true;
    }

    //Check if diff is bigger than diff max
    if(temperatureWork>temperatureSent)
    {
        if((temperatureWork-temperatureSent) > temperatureDiffMax)
        {
            temperatureSent = temperatureWork;
            humiditySent    = humidityWork;
            valueSendTime = UnixTime::get();
            return true;
        }
    }

    //Check if diff is bigger than diff max
    if(humidityWork>humiditySent)
    {
        if((humidityWork-humiditySent) > humidityDiffMax)
        {
            temperatureSent = temperatureWork;
            humiditySent    = humidityWork;
            valueSendTime = UnixTime::get();
            return true;
        }
    }

    //Check if negative diff is bigger than diff max
    if(temperatureWork<temperatureSent)
    {
        if((temperatureSent-temperatureWork) > temperatureDiffMax)
        {
            temperatureSent = temperatureWork;
            humiditySent    = humidityWork;
            valueSendTime = UnixTime::get();
            return true;
        }
    }

    //Check if negative diff is bigger than diff max
    if(humidityWork<humiditySent)
    {
        if((humiditySent-humidityWork) > humidityDiffMax)
        {
            temperatureSent = temperatureWork;
            humiditySent    = humidityWork;
            valueSendTime = UnixTime::get();
            return true;
        }
    }

    return false;
}


/**
 * Activate and set levels for high and low alarm.
 * Alarms exists for humidity and temperature.
 *
 * @param HighTemperature high temperature alarm level
 * @param HighTemperatureActive true will activate the high temperature alarm
 * @param LowTemperature low temperature alarm level
 * @param LowTemperatureActive true will activate the high temperature alarm
 * @param HighHumidity high humidity alarm level
 * @param HighHumidityActive true will activate the high humidity alarm
 * @param LowHumidity low humidity alarm level
 * @param LowHumidityActive true will activate the low humidity alarm
 */
void HumiditySensor::setAlarmLevels(
        double highTemperature, bool highTemperatureActive,
        double lowTemperature,  bool lowTemperatureActive,
        double highHumidity,    bool highHumidityActive,
        double lowHumidity,     bool lowHumidityActive)
{
        alarmHighTemperatureSent = false,
        alarmHighHumiditySent    = false;
        alarmLowTemperatureSent  = false;
        alarmLowHumiditySent     = false;

        alarmHighTemperature       = highTemperature;
        alarmHighTemperatureActive = highTemperatureActive;
        alarmHighHumidity          = highHumidity;
        alarmHighHumidityActive    = highHumidityActive;
        alarmLowTemperature        = lowTemperature;
        alarmLowTemperatureActive  = lowTemperatureActive;
        alarmLowHumidity           = lowHumidity;
        alarmLowHumidityActive     = lowHumidityActive;
}


void HumiditySensor::setAlarmHyst(double hystTemperature, double hystHumidity)
{
    alarmHystTemperature = hystTemperature;
    alarmHystHumidity    = hystHumidity;
}


/**
 * How much must the value change before we send it?
 *
 * @param temperature value is set to X, then we send directly if the messured value diffs more than X from the last sent value.
 * @param humidity value is set to X, then we send directly if the messured value diffs more than X from the last sent value.
 */
void HumiditySensor::setDiffToSend(double temperature, double humidity)
{
    temperatureDiffMax = temperature;
    humidityDiffMax    = humidity;
}


/**
 * If a sensor has a static measurment error,
 * this offset value can be added to correct such a error.
 *
 * @param temperature the offset value that will be added
 * @param humidity the offset value that will be added
 */
void HumiditySensor::setValueOffset(double temperature, double humidity)
{
    temperatureOffset = temperature;
    humidityOffset    = humidity;
}

bool HumiditySensor::alarmCheck(char* responce, int maxSize)
{
    bool sendAlarm = false;

    int pos = snprintf(responce, maxSize, "Alarm");
    responce += pos;
    maxSize  -= pos;

    // Check High temperature alarm
    if(temperatureWork > alarmHighTemperature)
    {
        if(alarmHighTemperatureActive && !alarmHighTemperatureSent)
        {
            sendAlarm = true;
            alarmHighTemperatureSent = true;

            pos = snprintf(responce, maxSize, " : High Temperature=%.02f(%.02f)",
                    temperatureWork, alarmHighTemperature);
            responce += pos;
            maxSize  -= pos;
        }
    }
    else if( temperatureWork < (alarmHighTemperature-alarmHystTemperature) )
    {
        alarmHighTemperatureSent = false;
    }


    // Check Low temperature alarm
    if(temperatureWork < alarmLowTemperature)
    {
        if(alarmLowTemperatureActive && !alarmLowTemperatureSent)
        {
            sendAlarm = true;
            alarmLowTemperatureSent = true;

            pos = snprintf(responce, maxSize, " : Low Temperature=%.02f(%.02f)",
                    temperatureWork, alarmLowTemperature);
            responce += pos;
            maxSize  -= pos;
        }
    }
    else if( temperatureWork > (alarmLowTemperature+alarmHystTemperature) )
    {
        alarmLowTemperatureSent = false;
    }

    // Check High humidity alarm
    if(humidityWork > alarmHighHumidity)
    {
        if(alarmHighHumidityActive && !alarmHighHumiditySent)
        {
            sendAlarm = true;
            alarmHighHumiditySent = true;

            pos = snprintf(responce, maxSize, " : High Humidity=%.02f(%.02f)", 
                    humidityWork, alarmHighHumidity);
            responce += pos;
            maxSize  -= pos;
        }
    }
    else if( humidityWork < (alarmHighHumidity-alarmHystHumidity) )
    {
        alarmHighHumiditySent = false;
    }


    // Check Low Humidity alarm
    if(humidityWork < alarmLowHumidity)
    {
        if(alarmLowHumidityActive && !alarmLowHumiditySent)
        {
            sendAlarm = true;
            alarmLowHumiditySent = true;

            pos = snprintf(responce, maxSize, " : Low Humidity=%.02f(%.02f)",
                    humidityWork, alarmLowHumidity);
            responce += pos;
            maxSize  -= pos;
        }
    }
    else if( humidityWork > (alarmLowHumidity+alarmHystHumidity) )
    {
        alarmLowHumiditySent = false;
    }


    return sendAlarm;
}


/**
 * Tell the logic that we did not send that alarm.
 */
void HumiditySensor::alarmFailed()
{
    alarmHighTemperatureSent = false;
    alarmLowTemperatureSent = false;
    alarmHighHumiditySent = false;
    alarmLowHumiditySent = false;
}

