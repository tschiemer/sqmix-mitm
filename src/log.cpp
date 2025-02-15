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

namespace SQMixMitm {

    static LogLevel logLevel = LogLevelInfo;

    static FILE * logFile = stdout;

    void setLogLevel(LogLevel level){
        logLevel = level;
    }

    void setLogFile(FILE * file){
        logFile = file;
    }

    void log(LogLevel level, const char * msg, ...){

        if (logLevel < level){
            return;
        }

        va_list args;
        va_start(args, msg);

        switch(level){
            case LogLevelInfo:
                fprintf(logFile, "INFO ");
                break;
            case LogLevelDebug:
                fprintf(logFile, "DEBUG ");
                break;
            default:
                break;
        }


        vfprintf(logFile, msg, args);
        fprintf(logFile, "\n");
        fflush(logFile);

        va_end(args);

    }

    void error(const char * msg, ...){

        va_list args;
        va_start(args, msg);

        fprintf(stderr, "ERROR ");
        vfprintf(stderr, msg, args);
        fprintf(stderr, "\n");
        fflush(stderr);

        va_end(args);

    }

} // SQMixMitm