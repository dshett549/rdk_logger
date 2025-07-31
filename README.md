Overview
--------
 
rdklogger is a general-purpose logging mechanism that utilizes log4c for formatting, log writing, and log rotation. It supports multiple log levels and modules. The log level for each module is controlled by log4c category priorities, which are set via configuration files (such as `debug.ini` or `log4crc`) or dynamically at runtime. For each module, only messages at or above the configured priority are emitted.
 
rdklogger supports overriding the configuration at runtime by providing a persistent path to files like `/opt/debug.ini` or `/nvram/debug.ini` or `/etc/debug.ini`. It includes a CLI utility called `rdklogctrl`, which is used to dynamically change log levels for modules at runtime.
 
Log filtering is handled by log4c's priority mechanism: for each category, only messages at or above the configured priority are logged. You control which log levels are enabled for each module by setting the appropriate priority in your configuration.

If module not found in debug.ini, it is set with priority of parent category.
If module found in debug.ini, it will be set with its respective priority.

Sample debug.ini
----------------
 
The `sample_debug.ini` file demonstrates how to configure log levels for different modules. Each line specifies a module name and its desired log level.
 
You can adjust the log level for each module by editing debug.ini and restarting the application, or by using `rdklogctrl` for dynamic changes.
