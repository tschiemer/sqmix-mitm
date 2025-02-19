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

#ifndef SQMIX_MITM_LOG_H
#define SQMIX_MITM_LOG_H

#include <cstdio>

namespace SQMixMitm {

    enum LogLevel {
        LogLevelNone    = 0,
        LogLevelError   = 1,
        LogLevelInfo    = 2,
        LogLevelDebug   = 3
    };

    typedef void (*LogFunction)(LogLevel,const char *,...);

    LogLevel getLogLevel();
    void setLogLevel(LogLevel level);
    void setLogFile(FILE * file);
    void setLogFunction(LogFunction function);

    void defaultLog(LogLevel level, const char * msg, ...);

    extern LogFunction log;

} // SQMixMitm

#endif //SQMIX_MITM_LOG_H
