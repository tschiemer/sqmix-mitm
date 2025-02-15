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

    enum LogLevel {LogLevelNone, LogLevelInfo, LogLevelDebug};

    inline bool isValidLogLevel(LogLevel logLevel){
        return (
                LogLevelNone == logLevel ||
                LogLevelInfo == logLevel ||
                LogLevelDebug == logLevel
        );
    }

    void setLogLevel(LogLevel level);

    void setLogFile(FILE * file);

    void log(LogLevel level, const char * msg, ...);

    void error(const char * msg, ...);

} // SQMixMitm

#endif //SQMIX_MITM_LOG_H
