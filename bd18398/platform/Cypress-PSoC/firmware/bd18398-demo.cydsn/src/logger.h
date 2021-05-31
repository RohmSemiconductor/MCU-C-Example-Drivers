// SPDX-License-Identifier: MIT
/*
 * The MIT License (MIT)
 * Copyright (c) 2021 ROHM Co.,Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file
 * @brief Utilities for printing diagnostic messages via the PSoC's UART.
 *
 * The log level is configured at compile time with the `LOGGER_ENABLE_LVL`
 * define. To ensure that a certain log level is set, use the compiler's
 * define flag to define `LOGGER_ENABLE_LVL` to the desired level. If the
 * log level is not provided, then a default log level (dependent on the
 * presence of `NDEBUG`) is used.
 *
 * Calls to any logging functions will silently fail until UART_Start() has been
 * called.
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#define LOGGER_EOL "\r\n"

/* Log levels. Higher values are more verbose. No logging is always indicated by
 * zero.
 */
#define LOGGER_LVL_NONE 0
#define LOGGER_LVL_ERROR 1
#define LOGGER_LVL_WARN 2
#define LOGGER_LVL_INFO 3
#define LOGGER_LVL_TRACE 4

#ifndef LOGGER_ENABLE_LVL
#ifndef NDEBUG
#define LOGGER_ENABLE_LVL LOGGER_LVL_INFO
#else				/* NDEBUG */
#define LOGGER_ENABLE_LVL LOGGER_LVL_NONE
#endif				/* NDEBUG */
#endif				/* LOGGER_ENABLE_LVL */

/** Return whether a log level is enabled or not. */
#define LOGGER_IS_LVL_ENABLED(level) (level && LOGGER_ENABLE_LVL >= level)

/* Print to UART if the log level is enabled.
 * Otherwise do nothing. */
#define LOGGER_LOG(level, msg)                      \
    do {                                            \
        if (LOGGER_IS_LVL_ENABLED(level)) {         \
            printf("%s", msg);                      \
        }                                           \
    } while (0)

/* Print to UART (with CR+LF termination) if the log level is enabled.
 * Otherwise do nothing. */
#define LOGGER_LOGLN(level, msg)                    \
    do {                                            \
        if (LOGGER_IS_LVL_ENABLED(level)) {         \
            printf("%s%s", msg, LOGGER_EOL);        \
        }                                           \
    } while (0)

/* Printf to UART (with CR+LF termination) if the log level is enabled.
 * Otherwise do nothing. */
#define LOGGER_LOGLNF(level, ...)                   \
    do {                                            \
        if (LOGGER_IS_LVL_ENABLED(level)) {         \
            printf(__VA_ARGS__);                    \
            printf("%s", LOGGER_EOL);               \
        }                                           \
    } while (0)

/* Convenience defines for logging. */
#define LOGGER_ERRORF(...) LOGGER_LOGLNF(LOGGER_LVL_ERROR, __VA_ARGS__)
#define LOGGER_WARNF(...) LOGGER_LOGLNF(LOGGER_LVL_WARN, __VA_ARGS__)
#define LOGGER_INFOF(...) LOGGER_LOGLNF(LOGGER_LVL_INFO, __VA_ARGS__)
#define LOGGER_TRACEF(...) LOGGER_LOGLNF(LOGGER_LVL_TRACE, __VA_ARGS__)

#endif				/* LOGGER_H */
