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

#if !defined(_RDK_DBG_PRIV_H)
#define _RDK_DBG_PRIV_H

#include <rdk_logger_types.h>
#include <rdk_debug.h>
#include <rdk_error.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Maximum limit on the number of Modules logged*/

#define RDK_MAX_MOD_COUNT 512

/** This variable is described in rdk_dbg_priv_log.c - to be used by
 * rdk_debug_priv_.* files only.
 */
extern uint32_t rdk_g_logControlTbl[];
extern rdk_logger_Bool dbg_logViaUDP;

/**
 * The <i>rdk_logger_envGet()</i> function will get the value of the specified
 * environment variable.
 *
 * @param name is a pointer to the name of the target environment variable.
 *
 * @return a pointer to the associated string value of the target environment
 * variable or NULL if the variable can't be found.
 */
const char* rdk_logger_envGet(const char *name);

/**
 * The <i>rdk_logger_env_add_conf_file()</i> function sets up the environment variable
 * storage by parsing configuration file.
 *
 * @param path Path of the file.
 * @return Returns relevant RDK_OSAL error code on failure, otherwise <i>RDK_SUCCESS</i>
 *          is returned.
 */
rdk_Error rdk_logger_env_add_conf_file(const char * path);
rdk_Error rdk_logger_env_rem_conf_details(void);

void 			rdk_dbg_priv_LogControlInit(void);
void 			rdk_dbg_priv_Init(void);
void 			rdk_dbg_priv_DeInit(void);
void rdk_dbg_priv_Shutdown();
const char * 	rdk_dbg_priv_LogControlOpSysIntf(char *logName, char *cfgStr);
const char * 	rdk_dbg_priv_LogQueryOpSysIntf(char *modName, char *cfgStr, int cfgStrMaxLen);
void rdk_dbg_priv_ext_Init(const char* logdir, const char* log_file_name, long maxCount, long maxSize);
void rdk_debug_priv_log_msg( rdk_LogLevel level,
        int module, const char *module_name, const char* format, va_list args);
void RDK_LOG_ControlCB(const char *moduleName, const char *subComponentName, const char *loggingLevel, int log_status);
void rdk_dbgDumpLog(const char* path);
void rdk_dbgInit();
void rdk_dbgDeinit();

#ifdef __cplusplus
}
#endif

#endif /* _RDK_DBG_PRIV_H */

