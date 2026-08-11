/** @file  uwb_log.h
 *  @brief Logging system.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef UWB_LOG_H_
#define UWB_LOG_H_

/* INCLUDES *******************************************************************/
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "uwb_circular_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#ifndef MAX_LOG_SIZE
#define MAX_LOG_SIZE 128
#endif

/* TYPES **********************************************************************/
/** @brief Logger error codes.
 */
typedef enum log_error {
    /*! No error. */
    LOG_ERR_NONE = 0,
    /*! Circular buffer access error. */
    LOG_ERR_BUFFER_ACCESS,
    /*! Deferred logging is disabled. */
    LOG_ERR_DEFERRED_DISABLED
} log_error_t;

/** @brief Logger severity levels.
 */
typedef enum level {
    /*! Trace level. */
    TRACE,
    /*! Debug level. */
    DEBUG,
    /*! Informational level. */
    INFO,
    /*! Warning level. */
    WARN,
    /*! Error level. */
    ERR,
    /*! Fatal level. */
    FATAL
} log_level_t;

/** @brief Logger runtime configuration.
 */
typedef struct {
    /*! Enable or disable logging. */
    uint8_t enabled : 1;
    /*! Enable or disable timestamp formatting. */
    uint8_t timestamp : 1;
    /*! Append a new line after each log message. */
    uint8_t new_line : 1;
    /*! Enable or disable deferred logging mode. */
    uint8_t deferred : 1;
    /*! Minimum severity level to output. */
    uint8_t level : 3;
    /*! Timestamp frequency in hertz. */
    uint16_t freq;
} log_config_t;

/** @brief Logger instance context.
 */
typedef struct {
    /*! Active logger configuration. */
    log_config_t config;
    /*! Circular buffer used for deferred logging. */
    circ_buffer_t circ_buf;
    /*! Storage backing the circular buffer. */
    char *buffer;
    /*! Backing storage size in bytes. */
    uint16_t buf_size;
    /*! Timestamp callback returning the current tick/time value. */
    uint32_t (*timestamp)(void);
    /*! Output callback used to write a formatted message. */
    void (*io)(char *message);
} uwb_log_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the logging system.
 *
 *  @param[in] log     Logger instance.
 *  @param[in] config  Logger configuration.
 */
void uwb_log_init(uwb_log_t *log, log_config_t config);

/** @brief Format and enqueue a log entry using a variable argument list.
 *
 *  @param[in]  log    Logger instance.
 *  @param[out] err    Logger error code.
 *  @param[in]  level  Log level.
 *  @param[in]  fmt    Log format string.
 *  @param[in]  args   Variable argument list.
 */
void uwb_vlog(uwb_log_t *log, log_error_t *err, log_level_t level, const char *fmt, va_list args);

/** @brief Format and enqueue a log entry.
 *
 *  @param[in]  log    Logger instance.
 *  @param[out] err    Logger error code.
 *  @param[in]  level  Log level.
 *  @param[in]  fmt    Log format string.
 *  @param[in]  ...    Format arguments.
 */
void uwb_log(uwb_log_t *log, log_error_t *err, log_level_t level, const char *fmt, ...);

/** @brief Flush deferred log messages.
 *
 *  @param[in]  log  Logger instance.
 *  @param[out] err  Logger error code.
 *  @retval true   Deferred messages were dumped.
 *  @retval false  Dump operation failed.
 */
bool uwb_log_dump(uwb_log_t *log, log_error_t *err);

/** @brief Set the logger threshold level.
 *
 *  @param[in] log    Logger instance.
 *  @param[in] level  Minimum level to log.
 */
void uwb_log_set_level(uwb_log_t *log, log_level_t level);

#ifdef __cplusplus
}
#endif
#endif /* UWB_LOG_H_ */
