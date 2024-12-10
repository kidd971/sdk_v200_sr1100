/** @file  uwb_log.c
 *  @brief Logging system.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "uwb_log.h"

/* TYPES **********************************************************************/
typedef struct log_header {
    uint32_t ts;
    uint8_t  level;
} log_header_t;

/* PRIVATE GLOBALS ************************************************************/
static const char *const level_str[] = {"TRACE : ", "DEBUG : ", "INFO : ", "WARN : ", "ERROR : ", "FATAL : "};

/* PUBLIC FUNCTIONS ***********************************************************/

/** @brief Initialize the log interface
 *
 *  The log and config structure must initialize prior to the function call.
 *
 *  @param[in] log  log struct with buffer and function pointers.
 *  @param[out] config  Configuration structure.
 *
 */
void uwb_log_init(uwb_log_t *log, log_config_t config)
{
    log->config = config;
    uwb_circ_buff_init(&log->circ_buf, log->buffer, log->buf_size, sizeof(char));
}

/** @brief Write new log.
 *
 *  Print the log to the interface declared in the log structure.
 *  if deferred mode is enabled in the config structure, logs are saved in a
 *  buffer for processor optimization. You need to call sr1000_log_dump later
 *  to empty the buffer. Otherwise, the interface function is called right away
 *  to output the log string.
 *
 *  @param[in]  log   Log struct.
 *  @param[out] err   Pointer that receive an error code.
 *  @param[in]  level Desired log level
 *      @li TRACE,
 *      @li DEBUG,
 *      @li INFO,
 *      @li WARN,
 *      @li ERROR,
 *      @li FATAL
 *  @param[in] fmt    Pointer to the string to print.
 *  @param[in] args   Arguments for the string.
 */
void uwb_vlog(uwb_log_t *log, log_error_t *err, log_level_t level, const char *fmt, va_list args)
{
    char log_buf[MAX_LOG_SIZE];
    circ_buff_error_t cb_err;
    size_t str_size = 0;
    uint32_t ts;

    *err = LOG_ERR_NONE;

    if ((bool)log->config.enabled && (level >= log->config.level)) {
        if ((bool)log->config.deferred) {
            log_header_t log_header;

            log_header.level = level;
            log_header.ts = log->timestamp();
            str_size = vsnprintf(log_buf, MAX_LOG_SIZE, fmt, args);
            uwb_circ_buff_in(&log->circ_buf, &log_header.level, sizeof(log_header.level), &cb_err);
            uwb_circ_buff_in(&log->circ_buf, &log_header.ts, sizeof(log_header.ts), &cb_err);

            if (cb_err != CIRC_BUFF_ERR_NONE) {
                *err = LOG_ERR_BUFFER_ACCESS;
                return;
            }

            for (size_t i = 0; i <= str_size; i++) {
                uwb_circ_buff_in(&log->circ_buf, &log_buf[i], sizeof(char), &cb_err);

                if (cb_err != CIRC_BUFF_ERR_NONE) {
                    *err = LOG_ERR_BUFFER_ACCESS;
                    return;
                }
            }
        } else {
            if ((bool)log->config.timestamp) {
                ts = log->timestamp();
                str_size += snprintf(log_buf + str_size, MAX_LOG_SIZE - str_size, "[%lu.%.3lu] ", ts / log->config.freq,
                                     ts % log->config.freq);
            }

            str_size += snprintf(log_buf + str_size, MAX_LOG_SIZE - str_size, "%s", level_str[level]);
            str_size += vsnprintf(log_buf + str_size, MAX_LOG_SIZE - str_size, fmt, args);

            if ((bool)log->config.new_line) {
                snprintf(log_buf + str_size, MAX_LOG_SIZE - str_size, "\n\r");
            }

            log->io(log_buf);
        }
    }
}

/** @brief Write new log.
 *
 *  Print the log to the interface declared in the log structure.
 *  if deferred mode is enabled in the config structure, logs are saved in a
 *  buffer for processor optimization. You need to call sr1000_log_dump later
 *  to empty the buffer. Otherwise, the interface function is called right away
 *  to output the log string.
 *
 *  @param[in]  log   Log struct.
 *  @param[out] err   Pointer that receive an error code.
 *  @param[in]  level Desired log level
 *      @li TRACE,
 *      @li DEBUG,
 *      @li INFO,
 *      @li WARN,
 *      @li ERROR,
 *      @li FATAL
 *  @param[in] fmt    Pointer to the string to print.
 *  @param[in] ...    Arguments for the string.
 */
void uwb_log(uwb_log_t *log, log_error_t *err, log_level_t level, const char *fmt, ...)
{
        va_list  args;

        va_start(args, fmt);
        uwb_vlog(log, err, level, fmt, args);
        va_end(args);
}

/** @brief Output log when deferred mode is enabled
 *
 *  This function output one log from the log buffer. Do not
 *  use it if deferred mode is not enabled
 *
 *  @param[in]  log   Log struct.
 *  @param[out] err   Pointer that receive an error code.
 *  @return True if the buffer is not empty. False otherwise.
 */
bool uwb_log_dump(uwb_log_t *log, log_error_t *err)
{
    log_header_t log_header;
    size_t str_size = 0;
    circ_buff_error_t cb_err;
    char log_buf[MAX_LOG_SIZE];

    *err = LOG_ERR_NONE;

    if (!(bool)log->config.deferred) {
        *err = LOG_ERR_DEFERRED_DISABLED;
        return false;
    }

    uwb_circ_buff_out(&log->circ_buf, &log_header.level, sizeof(log_header.level), &cb_err);
    uwb_circ_buff_out(&log->circ_buf, &log_header.ts, sizeof(log_header.ts), &cb_err);

    if (cb_err != CIRC_BUFF_ERR_NONE) {
        *err = LOG_ERR_BUFFER_ACCESS;
        return false;
    }

    if ((bool)log->config.timestamp) {
        str_size += snprintf(log_buf + str_size, MAX_LOG_SIZE - str_size, "[%lu.%.3lu] ",
                             log_header.ts / log->config.freq, log_header.ts % log->config.freq);
    }

    str_size += snprintf(log_buf + str_size, MAX_LOG_SIZE - str_size, "%s", level_str[log_header.level]);

    do {
        uwb_circ_buff_out(&log->circ_buf, &log_buf[str_size], sizeof(char), &cb_err);

        if (cb_err != CIRC_BUFF_ERR_NONE) {
            *err = LOG_ERR_BUFFER_ACCESS;
            return false;
        }
    } while (log_buf[str_size++] != 0);

    if (log->config.new_line) {
        snprintf(log_buf + str_size - 1, MAX_LOG_SIZE - str_size, "\n\r");
    }

    log->io(log_buf);

    return !(log->circ_buf.buf_empty);
}

/** @brief Set the logging level output
 *
 *  @param[in]  log     Log struct.
 *  @param[in]  level   max type of level to print
 *      @lo TRACE,
 *      @lo DEBUG,
 *      @lo INFO,
 *      @lo WARN,
 *      @Lo ERROR,
 *      @lo FATAL
 */
void uwb_log_set_level(uwb_log_t *log, log_level_t level)
{
    log->config.level = level;
}
