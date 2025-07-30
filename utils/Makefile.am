##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

bin_PROGRAMS = rdklogctrl

rdklogctrl_SOURCES=rdklogctrl.c
rdklogctrl_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/include -I./usr/include
rdklogctrl_LDFLAGS = -I./usr/lib

if IS_MILESTONE_ENABLED
bin_PROGRAMS       += rdkLogMileStone
rdkLogMileStone_SOURCES = rdklogmilestone.c
rdkLogMileStone_LDADD = $(top_builddir)/src/librdkloggers.la
rdkLogMileStone_CFLAGS = -I$(top_srcdir)/include
endif

if IS_ONBOARDLOG_ENABLED
bin_PROGRAMS            = onboarding_log
onboarding_log_SOURCES  = rdk_logger_onboard_main.c
onboarding_log_LDADD    = $(top_builddir)/src/librdkloggers.la
onboarding_log_CFLAGS   = -I$(top_srcdir)/include -I$(top_srcdir)/src/include -I./usr/include
onboarding_log_LDFLAGS  = -I./usr/lib
endif
