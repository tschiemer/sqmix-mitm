/**
* sqmix-mitm
* Copyright (C) 2025  Philip Tschiemer
*
* This program is free software: you can redistribute it and/or modify
        * it under the terms of the GNU Affero General Public License as published by
        * the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
        * but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "log.h"

#include <stdarg.h>
#include <cassert>
#include <cerrno>
#include <cstring>

namespace SQMixMitm {

    static LogLevel logLevel = LogLevelError;

    static FILE * logFile = stdout;

    LogFunction log = defaultLog;

    LogLevel getLogLevel(){
        return logLevel;
    }

    void setLogLevel(LogLevel level){
        logLevel = level;
    }

    void setLogFile(FILE * file){
        logFile = file;
    }

    void setLogFunction(LogFunction function){
        assert(function != nullptr);
        log = function;
    }

    void defaultLog(LogLevel level, const char * msg, ...){

        if (logLevel < level){
            return;
        }

        switch(level){
            case LogLevelError:
                fprintf(logFile, "ERROR ");
                break;
            case LogLevelInfo:
                fprintf(logFile, "INFO ");
                break;
            case LogLevelDebug:
                fprintf(logFile, "DEBUG ");
                break;
            default:
                break;
        }

        // print message
        va_list args;
        va_start(args, msg);

        vfprintf(logFile, msg, args);

        va_end(args);

        // NL
        fprintf(logFile, "\n");

    }

    void logError(const char * msg, ...) {

        char buffer[512];

        va_list args;
        va_start(args, msg);

        vsnprintf(buffer, sizeof(buffer), msg, args);

        va_end(args);

        log(LogLevelError,"%s: %s", buffer, strerror(errno));
    }

} // SQMixMitm