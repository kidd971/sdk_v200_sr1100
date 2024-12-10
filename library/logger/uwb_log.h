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
typedef enum log_error {
    LOG_ERR_NONE = 0,
    LOG_ERR_BUFFER_ACCESS,
    LOG_ERR_DEFERRED_DISABLED
} log_error_t;

typedef enum level {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERR,
    FATAL
} log_level_t;

typedef struct {
    uint8_t  enabled     :1;
    uint8_t  timestamp   :1;
    uint8_t  new_line    :1;
    uint8_t  deferred    :1;
    uint8_t  level       :3;
    uint16_t freq;
} log_config_t;

typedef struct {
    log_config_t  config;
    circ_buffer_t circ_buf;
    char         *buffer;
    uint16_t      buf_size;
    uint32_t    (*timestamp)(void);
    void        (*io)(char *message);
} uwb_log_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
void uwb_log_init(uwb_log_t *log, log_config_t config);
void uwb_vlog(uwb_log_t *log, log_error_t *err, log_level_t level, const char *fmt, va_list args);
void uwb_log(uwb_log_t *log, log_error_t *err, log_level_t level, const char *fmt, ...);
bool uwb_log_dump(uwb_log_t *log, log_error_t *err);
void uwb_log_set_level(uwb_log_t *log, log_level_t level);

#ifdef __cplusplus
}
#endif
#endif /* UWB_LOG_H_ */
