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

/**
 * @file rdk_debug.c
 * This source file contains the APIs for RDK debug.
 */

/**
* @defgroup rdk_logger
* @{
* @defgroup src
* @{
**/


#include <rdk_debug.h>
#include <rdk_debug_priv.h>

#include <string.h> // memset
#include <rdk_utils.h>
#include <stdarg.h>

extern int global_count;

/**
 * @brief Touch the file which can be used to check whether to log or not.
 *
 * @param[in] pszFile Character string representing name of the file to be created.
 * @return None.
 */
static void TouchFile(const char * pszFile)
{
    if(NULL != pszFile)
    {
        FILE * fp = fopen(pszFile, "w");
        if(NULL != fp)
        {
            fclose(fp);
        }
    }
}

/**
 * @brief Dump the debug log. It will Dump all the current settings so that an analysis of a log
 * file will include what logging information to expect.
 *
 * @param[in] path Character string representing path of the temp file to be created.
 * @return None.
 */
void rdk_dbgDumpLog(const char* path)
{
    int mod, i;
    char config[128];
    const char *modptr = NULL;
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.OS", "\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.OS",
            "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.OS", "Stack level logging levels: \n");

    /**
     * Now just dump all the current settings so that an analysis of a log
     * file will include what logging information to expect
     */
    for (mod = 1; mod <= global_count; mod++) 
    {
        modptr = rdk_logger_envGetModFromNum(mod);
    
        memset(config, 0, sizeof(config));
        (void) rdk_dbg_priv_LogQueryOpSysIntf((char*) modptr, config, 127);
        RDK_LOG(RDK_LOG_INFO, "LOG.RDK.OS",
                "Initial Logging Level for %-10s: %s\n", modptr, config);
    }

    RDK_LOG(RDK_LOG_INFO, "LOG.RDK.OS",
            "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n");
	TouchFile(path);
}

/**
 * @brief Initialize the underlying MPEOS debug support. This API must be called only once per boot cycle.
 * @return None.
 */
static rdk_logger_Bool inited = FALSE;
void rdk_dbgInit()
{

    if (!inited)
    {
        rdk_dbg_priv_Init();
        inited = TRUE;
    }
}

void rdk_dbgDeinit()
{
  if (inited)
  {
      inited = FALSE;
  }
}

/**
 * @brief Send a debugging message to the debugging window. It is appended to the log output based
 * on configurations set in the environment file.
 *
 * @param[in] level The debug logging level.
 * @param[in] module The name of the module for which this message belongs to, it is mentioned in debug.ini.
 * @param[in] format Printf style string containing the log message.
 */
void rdk_logger_msg_printf(rdk_LogLevel level, const char *module,
        const char *format, ...)
{
    static char initialized_modules[256][64] = {0};
     int found = 0;
     for (int i = 0; i < 256;++i) {
         if (initialized_modules[i][0] == '\0') break;
         if (strcmp(initialized_modules[i], module) == 0) {
             found = 1;
             break;
         }
     }
     if (!found) {
         for (int i = 0; i < 256;++i) {
             if (initialized_modules[i][0] == '\0') {
                 strncpy(initialized_modules[i], module, sizeof(initialized_modules[i])-1);
                 // Prepare config for this module
                 rdk_logger_ext_config_t cfg;
                 memset(&cfg, 0, sizeof(cfg));
                 strncpy(cfg.module, module, sizeof(cfg.module) - 1);
                 strncpy(cfg.logdir, "/tmp", sizeof(cfg.logdir) - 1);
                 strncpy(cfg.fileName, module, sizeof(cfg.fileName) - 1);
                 cfg.loglevel = level;
                 cfg.maxCount = 2;
                 cfg.maxSize = 2097152;

                 // Call the new init function
                 rdk_logger_ext_init(&cfg);
                 break;
             }
         }
     }
#if !defined(RDK_LOG_DISABLE)
    int num;
    va_list args;

    va_start(args, format);
    rdk_debug_priv_log_msg( level, num, module, 
                format, args);
    va_end(args);
#endif /* RDK_LOG_DISABLE */
}

void rdk_logger_msg_vsprintf(rdk_LogLevel level, const char *module,
        const char *format, va_list args)
{
#if !defined(RDK_LOG_DISABLE)
    int num;

    /** Get the registered value of module */
    num = rdk_logger_envGetNum(module);
    if(num < 0)
    {
        return;
    }
    rdk_debug_priv_log_msg( level, num, module,
                format, args);
#endif /* RDK_LOG_DISABLE */
}

