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

namespace SQMixMitm {

    static LogLevel logLevel = LogLevelError;

    static FILE * logFile = stdout;

    LogFunction log = defaultLog;

    LogLevel getLogLevel(){
        return logLevel;
    }

    void setLogLevel(LogLevel level){
        assert(isValidLogLevel(level));
        logLevel = level;
    }

    void setLogFile(FILE * file){
        logFile = file;
    }

    void setLogFunction(LogFunction &function){
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

//        fflush(logFile);
    }

    void error(const char * msg, ...){

        fprintf(stderr, "ERROR ");

        va_list args;
        va_start(args, msg);
        vfprintf(stderr, msg, args);
        va_end(args);

        fprintf(stderr, "\n");

//        fflush(stderr);
    }

} // SQMixMitm