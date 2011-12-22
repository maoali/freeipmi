/*
 * Copyright (C) 2003-2012 FreeIPMI Core Team
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef _IPMI_COMP_CODE_OEM_SPEC_H
#define _IPMI_COMP_CODE_OEM_SPEC_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************* 
 * Fujitsu                                 *
 *******************************************/

/*
 * Fujitsu RX100 S5
 *
 * http://manuals.ts.fujitsu.com/file/4390/irmc_s2-en.pdf
 */
/* IPMI_CMD_OEM_FUJITSU_BIOS - w/ GET_CPU_INFO Command Specifier */
#define IPMI_COMP_CODE_OEM_FUJITSU_BIOS_UNPOPULATED_CPU_SOCKET 0x01
#define IPMI_COMP_CODE_OEM_FUJITSU_BIOS_UNPOPULATED_CPU_SOCKET_STR \
  "Unpopulated CPU Socket"

/*******************************************
 * Intel                                   *
 *******************************************/

/*
 * Intel S5500WB/Penguin Computing Relion 700
 */

/* IPMI_CMD_OEM_INTEL_SET_SMTP_CONFIGURATION */

#define IPMI_COMP_CODE_SET_SMTP_CONFIGURATION_PARAMETER_NOT_SUPPORTED                    0x80
#define IPMI_COMP_CODE_SET_SMTP_CONFIGURATION_PARAMETER_NOT_SUPPORTED_STR \
  "parameter not supported."

#define IPMI_COMP_CODE_SET_SMTP_CONFIGURATION_WRITE_READ_ONLY_PARAMETER                  0x82
#define IPMI_COMP_CODE_SET_SMTP_CONFIGURATION_WRITE_READ_ONLY_PARAMETER_STR \
  "attempt to write read-only parameter"

/* IPMI_CMD_OEM_INTEL_GET_SMTP_CONFIGURATION */

/* achu: document from Intel also sites a "write read-only parameter"
 * error code, but I assume that is a cut and paste typo.  Shouldn't
 * be possible for the "get" command
 */

#define IPMI_COMP_CODE_GET_SMTP_CONFIGURATION_PARAMETER_NOT_SUPPORTED                    0x80
#define IPMI_COMP_CODE_GET_SMTP_CONFIGURATION_PARAMETER_NOT_SUPPORTED_STR \
  "parameter not supported."

/*
 * Intel Node Manager
 *
 * For Intel Chips, not just Intel Motherboards.  Confirmed for:
 *
 * Intel S5500WB/Penguin Computing Relion 700
 * Intel S2600JF/Appro 512X
 * Inventec 5441/Dell Xanadu II
 * Inventec 5442/Dell Xanadu III
 * Quanta S99Q/Dell FS12-TY
 * Quanta QSSC-S4R/Appro GB812X-CN
 */

/* IPMI_CMD_OEM_INTEL_ENABLE_DISABLE_NODE_MANAGER_POLICY_CONTROL */
/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_POLICY */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_POLICY */
/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_ALERT_THRESHOLDS */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_ALERT_THRESHOLDS */
/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_POLICY_SUSPEND_PERIODS */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_POLICY_SUSPEND_PERIODS */
/* IPMI_CMD_OEM_INTEL_RESET_NODE_MANAGER_STATISTICS */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_STATISTICS */
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_POLICY_ID 0x80
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_POLICY_ID_STR \
  "Invalid Policy Id"

/* IPMI_CMD_OEM_INTEL_ENABLE_DISABLE_NODE_MANAGER_POLICY_CONTROL */
/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_POLICY */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_POLICY */
/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_ALERT_THRESHOLDS */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_ALERT_THRESHOLDS */
/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_POLICY_SUSPEND_PERIODS */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_POLICY_SUSPEND_PERIODS */
/* IPMI_CMD_OEM_INTEL_RESET_NODE_MANAGER_STATISTICS */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_STATISTICS */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_CAPABILITIES */
/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_POWER_DRAW_RANGE */
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_DOMAIN_ID 0x81
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_DOMAIN_ID_STR \
  "Invalid Domain Id"

/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_POLICY */
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_UNKNOWN_OR_UNSUPPORTED_POLICY_TRIGGER_TYPE 0x82
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_UNKNOWN_OR_UNSUPPORTED_POLICY_TRIGGER_TYPE_STR \
  "unknown or unsupported Policy Trigger Type"

#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_UNKNOWN_OR_UNSUPPORTED_POLICY_CONFIGURATION_ACTION 0x83
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_UNKNOWN_OR_UNSUPPORTED_POLICY_CONFIGURATION_ACTION_STR \
  "unknown or unsupported Policy Configuration Action"

#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_POWER_LIMIT_OUT_OF_RANGE 0x84
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_POWER_LIMIT_OUT_OF_RANGE_STR \
  "Power Limit out of range"

#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_CORRECTION_TIME_OUT_OF_RANGE 0x85
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_CORRECTION_TIME_OUT_OF_RANGE_STR \
  "Correction Time out of range"

#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_POLICY_TRIGGER_VALUE_OUT_OF_RANGE 0x86
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_POLICY_TRIGGER_VALUE_OUT_OF_RANGE_STR \
  "Policy Trigger value out of range"

#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_STATISTICS_REPORTING_PERIOD_OUT_OF_RANGE 0x89
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_STATISTICS_REPORTING_PERIOD_OUT_OF_RANGE_STR \
  "Statistics Reporting Period out of range"

/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_ALERT_THRESHOLDS */
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_LIMIT_IN_ONE_OF_THRESHOLDS_IS_INVALID 0x84
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_LIMIT_IN_ONE_OF_THRESHOLDS_IS_INVALID_STR \
  "Limit in one of thresholds is invalid"

#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_NUMBER_OF_POLICY_THRESHOLDS 0x87
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_NUMBER_OF_POLICY_THRESHOLDS_STR \
  "Invalid Number of Policy Thresholds"

/* IPMI_CMD_OEM_INTEL_SET_NODE_MANAGER_POLICY_SUSPEND_PERIODS */
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_ONE_OF_PERIODS_IN_THE_TABLE_IS_INCONSISTENT 0x85
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_ONE_OF_PERIODS_IN_THE_TABLE_IS_INCONSISTENT_STR \
  "One of the periods in the table is inconsistent.  Start time is greater than or " \
  "equal to stop time or stop time sets time beyond 1 day"

#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_NUMBER_OF_POLICY_SUSPEND_PERIODS 0x87
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_NUMBER_OF_POLICY_SUSPEND_PERIODS_STR \
  "Invalid Number of policy suspend periods"

/* IPMI_CMD_OEM_INTEL_RESET_NODE_MANAGER_STATISTICS */
/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_STATISTICS */
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_MODE 0x88
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_INVALID_MODE_STR \
  "Invalid Mode"

/* IPMI_CMD_OEM_INTEL_GET_NODE_MANAGER_CAPABILITIES */
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_UNKNOWN_POLICY_TRIGGER_TYPE 0x82
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_UNKNOWN_POLICY_TRIGGER_TYPE_STR \
  "unknown Policy Trigger Type"

#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_UNKNOWN_POLICY_TYPE 0x83
#define IPMI_COMP_CODE_OEM_INTEL_NODE_MANAGER_UNKNOWN_POLICY_TYPE_STR \
  "unknown Policy Type"

#ifdef __cplusplus
}
#endif

#endif /* _IPMI_COMP_CODE_OEM_SPEC_H */

