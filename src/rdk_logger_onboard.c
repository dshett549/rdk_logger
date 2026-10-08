/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2016 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#define MAX_BUF_SIZE 1024

#ifndef ONBOARDING_FILE
#define ONBOARDING_FILE         /rdklogs/logs/OnBoardingLog.txt.0
#endif

#define STRINGIFY_VALUE(x)      #x
#define STRINGIFY_MACRO(x)      STRINGIFY_VALUE(x)
#define ONBOARDING_LOG_FILE     STRINGIFY_MACRO(ONBOARDING_FILE)

#define DEVICE_ONBOARDED        "/nvram/.device_onboarded"
#define DISABLE_ONBOARDING      "/nvram/DISABLE_ONBOARD_LOGGING"

static void sanitize_log_message(char *value)
{
    if(value == NULL)
        return;

    while(*value != '\0') 
    {
        unsigned char ch = (unsigned char)*value;
        if(ch < 0x20 || ch == 0x7f)
        {
            *value = ' ';
        }
        value++;
    }
}

void rdk_logger_log_onboard(const char *module, const char *msg, ...)
{
    va_list arg_ptr;
    char buf[MAX_BUF_SIZE];
    char safe_module[MAX_BUF_SIZE];
    int nbytes;
    struct tm * l_sTimeInfo;
    char l_cLocalTime[32] = {0};
    time_t l_sNowTime;
    FILE *l_fOnBoardingLogFile = NULL;

    if (access(DEVICE_ONBOARDED, F_OK) != -1 || access(DISABLE_ONBOARDING, F_OK) != -1)
    {
        return;
    }
    if(msg == NULL)
        return;

    time(&l_sNowTime);
    l_sTimeInfo = localtime(&l_sNowTime);

    strftime(l_cLocalTime,32, "%y%m%d-%X",l_sTimeInfo);
    va_start(arg_ptr, msg);
    nbytes = vsnprintf(buf, MAX_BUF_SIZE, msg, arg_ptr);
    va_end(arg_ptr);

    if( nbytes < 0 )
    {
        buf[0] = '\0';
    }
    else if( nbytes >=  MAX_BUF_SIZE )
    {
        buf[ MAX_BUF_SIZE - 1 ] = '\0';
    }
    else
    {
        buf[nbytes] = '\0';
    }

    sanitize_log_message(buf);
    if(module != NULL) {
        snprintf(safe_module, sizeof(safe_module), "%s", module);
        sanitize_log_message(safe_module);
    }

    l_fOnBoardingLogFile = fopen(ONBOARDING_LOG_FILE, "a+");
    if (NULL != l_fOnBoardingLogFile)
    {
        if(module != NULL)
        {
            fprintf(l_fOnBoardingLogFile, "%s [%s] %s\n", l_cLocalTime, safe_module, buf);
        }
        else
        {
            fprintf(l_fOnBoardingLogFile, "%s %s\n", l_cLocalTime, buf);
        }
        fclose(l_fOnBoardingLogFile);
    }
    else //fopen of on boarding file failed atleast write on the console
    {
        if(module != NULL)
        {
            printf("%s [%s] %s\n", l_cLocalTime, safe_module, buf);
        }
        else
        {
            printf("%s %s\n", l_cLocalTime, buf);
        }
    }
}
