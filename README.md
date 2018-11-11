# QtLogger
Extension for QMessageLogger, provides log into UDP socket, file and std stream through
standard qDebug, qWarning etc. macro.
You can control logger either by UDP packages, runtime configuration files or command line args.

## Control
Is made by commands wich can be passed through UDP package, runtime configuration file
or command line args.
Configuration file have to be placed to the same dir as an executable and had name `qtlogger-<process-name>-rc`
or `qtlogger-default-rc` for all executables in this dir.
Command line arg are passed as `--qtlogger="<command-pattern>"` option.

Command is a formated string: `<process-name> <command> <arg> ...` where `<process-name>`
can be passed as a regular expression.

### Commands
Command names can be reduced to a loss of certainty.
* `status <port>` Sends logger status on `<port>` or on `default-dest-port`
if no one specified.
* `echo mute` Mutes logger.
* `echo udp <port>` Redirects log messages to `<port>`
or on `default-dest-port` if no one specified.
* `echo file <file-path> <flush-period>` Redirects log messages to `<file-path>`
or on `<process-name>.log` if no one specidied. File will be flushed every `<flush-period>`
msec or `default-flush-period` if no one specidied.
* `echo stderr` Switches logger to stderr stream.
* `filter <operation> <type> <arg>`

`filter add <type> <arg>` Adds filter.

`filter del <type> <arg>` Removes filter.

`filter clear <type>` Removes all `<type>` filters or all type filters if no type specified.

`filter <operation> level <level>` Controls filtering by `<level>` (debug, info, warning, critical, fatal) wich
can be specified as regular expression.

`filter <operation> file <file>` Control filtering by filename wich can be specified as regular expression.

`filter <operation> function <function>` Control filtering by function wich can be specified as regular expression.

## Getting started
### UDP
```
term1 $ myapp --qtlogger="echo udp 6061"
term2 $ nc -lu 6061
```
```
term1 $ myapp
term2 $ nc -lu 6061
term3 $ nc -u 0.0.0.0 6060
myapp echo udp 6061
```
### File
```
term1 $ myapp --qtlogger="echo file"
```
```
term1 $ myapp
term3 $ nc -u 0.0.0.0 6060
myapp echo file
```
