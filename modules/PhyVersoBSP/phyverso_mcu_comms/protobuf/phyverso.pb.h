/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.6 */

#ifndef PB_PHYVERSO_PB_H_INCLUDED
#define PB_PHYVERSO_PB_H_INCLUDED
#include <nanopb/pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _CpState { 
    CpState_STATE_A = 0, 
    CpState_STATE_B = 1, 
    CpState_STATE_C = 2, 
    CpState_STATE_D = 3, 
    CpState_STATE_E = 4, 
    CpState_STATE_F = 5 
} CpState;

typedef enum _ResetReason { 
    ResetReason_USER = 0, 
    ResetReason_WATCHDOG = 1 
} ResetReason;

/* Struct definitions */
typedef struct _ErrorFlags { 
    bool diode_fault;
    bool rcd_selftest_failed;
    bool rcd_triggered;
    bool ventilation_not_available;
    bool connector_lock_failed;
    bool cp_signal_fault;
} ErrorFlags;

typedef struct _KeepAlive { 
    uint32_t time_stamp;
    uint32_t hw_type;
    uint32_t hw_revision;
    char sw_version_string[51];
} KeepAlive;

/* This container message is send from EVerest to MCU and may contain any allowed message in that direction. */
typedef struct _EverestToMcu { 
    pb_size_t which_payload;
    union {
        KeepAlive keep_alive;
        bool firmware_update;
        bool connector_lock; /* false: unlock, true: lock */
        uint32_t pwm_duty_cycle; /* in 0.01 %, 0 = State F, 10000 = X1 */
        bool allow_power_on;
        bool reset;
    } payload;
    int32_t connector; /* 0: None, 1: Connector 1, 2: Connector 2 */
} EverestToMcu;

/* This container message is send from MCU to EVerest and may contain any allowed message in that direction. */
typedef struct _McuToEverest { 
    pb_size_t which_payload;
    union {
        KeepAlive keep_alive;
        ResetReason reset;
        CpState cp_state;
        bool relais_state; /* false: relais are off, true: relais are on */
        ErrorFlags error_flags;
    } payload;
    int32_t connector; /* 0: None, 1: Connector 1, 2: Connector 2 */
} McuToEverest;


/* Helper constants for enums */
#define _CpState_MIN CpState_STATE_A
#define _CpState_MAX CpState_STATE_F
#define _CpState_ARRAYSIZE ((CpState)(CpState_STATE_F+1))

#define _ResetReason_MIN ResetReason_USER
#define _ResetReason_MAX ResetReason_WATCHDOG
#define _ResetReason_ARRAYSIZE ((ResetReason)(ResetReason_WATCHDOG+1))


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define EverestToMcu_init_default                {0, {KeepAlive_init_default}, 0}
#define McuToEverest_init_default                {0, {KeepAlive_init_default}, 0}
#define ErrorFlags_init_default                  {0, 0, 0, 0, 0, 0}
#define KeepAlive_init_default                   {0, 0, 0, ""}
#define EverestToMcu_init_zero                   {0, {KeepAlive_init_zero}, 0}
#define McuToEverest_init_zero                   {0, {KeepAlive_init_zero}, 0}
#define ErrorFlags_init_zero                     {0, 0, 0, 0, 0, 0}
#define KeepAlive_init_zero                      {0, 0, 0, ""}

/* Field tags (for use in manual encoding/decoding) */
#define ErrorFlags_diode_fault_tag               1
#define ErrorFlags_rcd_selftest_failed_tag       2
#define ErrorFlags_rcd_triggered_tag             3
#define ErrorFlags_ventilation_not_available_tag 4
#define ErrorFlags_connector_lock_failed_tag     5
#define ErrorFlags_cp_signal_fault_tag           6
#define KeepAlive_time_stamp_tag                 1
#define KeepAlive_hw_type_tag                    2
#define KeepAlive_hw_revision_tag                3
#define KeepAlive_sw_version_string_tag          6
#define EverestToMcu_keep_alive_tag              1
#define EverestToMcu_firmware_update_tag         2
#define EverestToMcu_connector_lock_tag          3
#define EverestToMcu_pwm_duty_cycle_tag          4
#define EverestToMcu_allow_power_on_tag          5
#define EverestToMcu_reset_tag                   6
#define EverestToMcu_connector_tag               7
#define McuToEverest_keep_alive_tag              1
#define McuToEverest_reset_tag                   2
#define McuToEverest_cp_state_tag                3
#define McuToEverest_relais_state_tag            4
#define McuToEverest_error_flags_tag             5
#define McuToEverest_connector_tag               6

/* Struct field encoding specification for nanopb */
#define EverestToMcu_FIELDLIST(X, a) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,keep_alive,payload.keep_alive),   1) \
X(a, STATIC,   ONEOF,    BOOL,     (payload,firmware_update,payload.firmware_update),   2) \
X(a, STATIC,   ONEOF,    BOOL,     (payload,connector_lock,payload.connector_lock),   3) \
X(a, STATIC,   ONEOF,    UINT32,   (payload,pwm_duty_cycle,payload.pwm_duty_cycle),   4) \
X(a, STATIC,   ONEOF,    BOOL,     (payload,allow_power_on,payload.allow_power_on),   5) \
X(a, STATIC,   ONEOF,    BOOL,     (payload,reset,payload.reset),   6) \
X(a, STATIC,   SINGULAR, INT32,    connector,         7)
#define EverestToMcu_CALLBACK NULL
#define EverestToMcu_DEFAULT NULL
#define EverestToMcu_payload_keep_alive_MSGTYPE KeepAlive

#define McuToEverest_FIELDLIST(X, a) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,keep_alive,payload.keep_alive),   1) \
X(a, STATIC,   ONEOF,    UENUM,    (payload,reset,payload.reset),   2) \
X(a, STATIC,   ONEOF,    UENUM,    (payload,cp_state,payload.cp_state),   3) \
X(a, STATIC,   ONEOF,    BOOL,     (payload,relais_state,payload.relais_state),   4) \
X(a, STATIC,   ONEOF,    MESSAGE,  (payload,error_flags,payload.error_flags),   5) \
X(a, STATIC,   SINGULAR, INT32,    connector,         6)
#define McuToEverest_CALLBACK NULL
#define McuToEverest_DEFAULT NULL
#define McuToEverest_payload_keep_alive_MSGTYPE KeepAlive
#define McuToEverest_payload_error_flags_MSGTYPE ErrorFlags

#define ErrorFlags_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, BOOL,     diode_fault,       1) \
X(a, STATIC,   SINGULAR, BOOL,     rcd_selftest_failed,   2) \
X(a, STATIC,   SINGULAR, BOOL,     rcd_triggered,     3) \
X(a, STATIC,   SINGULAR, BOOL,     ventilation_not_available,   4) \
X(a, STATIC,   SINGULAR, BOOL,     connector_lock_failed,   5) \
X(a, STATIC,   SINGULAR, BOOL,     cp_signal_fault,   6)
#define ErrorFlags_CALLBACK NULL
#define ErrorFlags_DEFAULT NULL

#define KeepAlive_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UINT32,   time_stamp,        1) \
X(a, STATIC,   SINGULAR, UINT32,   hw_type,           2) \
X(a, STATIC,   SINGULAR, UINT32,   hw_revision,       3) \
X(a, STATIC,   SINGULAR, STRING,   sw_version_string,   6)
#define KeepAlive_CALLBACK NULL
#define KeepAlive_DEFAULT NULL

extern const pb_msgdesc_t EverestToMcu_msg;
extern const pb_msgdesc_t McuToEverest_msg;
extern const pb_msgdesc_t ErrorFlags_msg;
extern const pb_msgdesc_t KeepAlive_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define EverestToMcu_fields &EverestToMcu_msg
#define McuToEverest_fields &McuToEverest_msg
#define ErrorFlags_fields &ErrorFlags_msg
#define KeepAlive_fields &KeepAlive_msg

/* Maximum encoded size of messages (where known) */
#define ErrorFlags_size                          12
#define EverestToMcu_size                        83
#define KeepAlive_size                           70
#define McuToEverest_size                        83

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
