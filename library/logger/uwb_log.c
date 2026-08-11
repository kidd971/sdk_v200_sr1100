/** @file  uwb_log.c
 *  @brief Logging system.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "uwb_log.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

/* TYPES **********************************************************************/
/** @brief Log header structure stored in the circular buffer for deferred logging.
 */
typedef struct log_header {
    /*! Timestamp of the log entry. */
    uint32_t ts;
    /*! Log level of the entry. */
    uint8_t level;
} log_header_t;

/* PRIVATE GLOBALS ************************************************************/
static const char *const level_str[] = {"TRACE : ", "DEBUG : ", "INFO : ", "WARN : ", "ERROR : ", "FATAL : "};

/* PUBLIC FUNCTIONS ***********************************************************/
void uwb_log_init(uwb_log_t *log, log_config_t config)
{
    log->config = config;
    uwb_circ_buff_init(&log->circ_buf, log->buffer, log->buf_size, sizeof(char));
}

void uwb_vlog(uwb_log_t *log, log_error_t *err, log_level_t level, const char *fmt, va_list args)
{
    char log_buf[MAX_LOG_SIZE];
    circ_buff_error_t cb_err = CIRC_BUFF_ERR_NONE;
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
                str_size += snprintf(log_buf + str_size, MAX_LOG_SIZE - str_size, "[%" PRIu32 ".%.3" PRIu32 "] ",
                                     ts / log->config.freq, ts % log->config.freq);
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

void uwb_log(uwb_log_t *log, log_error_t *err, log_level_t level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    uwb_vlog(log, err, level, fmt, args);
    va_end(args);
}

bool uwb_log_dump(uwb_log_t *log, log_error_t *err)
{
    log_header_t log_header = {0};
    size_t str_size = 0;
    circ_buff_error_t cb_err = CIRC_BUFF_ERR_NONE;
    char log_buf[MAX_LOG_SIZE] = {0};

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
        str_size += snprintf(log_buf + str_size, MAX_LOG_SIZE - str_size, "[%" PRIu32 ".%.3" PRIu32 "] ",
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

void uwb_log_set_level(uwb_log_t *log, log_level_t level)
{
    log->config.level = level;
}
