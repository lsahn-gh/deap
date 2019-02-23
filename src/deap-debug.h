/* deap-debug.h
 *
 * Copyright 2019 Yi-Soo An <yisooan@gmail.com>
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
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* This code is based on gtd-debug.h of GNOME To Do */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

#ifndef DEAP_ENABLE_TRACE
# define DEAP_ENABLE_TRACE 1
#endif
#if DEAP_ENABLE_TRACE != 1
# undef DEAP_ENABLE_TRACE
#endif

#ifndef DEAP_LOG_LEVEL_TRACE
# define DEAP_LOG_LEVEL_TRACE ((GLogLevelFlags)(1 << G_LOG_LEVEL_USER_SHIFT))
#endif

/* To compile embedded code, gtd-log.c */
#ifndef GTD_LOG_LEVEL_TRACE
# define GTD_LOG_LEVEL_TRACE  DEAP_LOG_LEVEL_TRACE
#endif

#ifdef DEAP_ENABLE_TRACE


#define deap_log(fmt, ...) \
  g_log(G_LOG_DOMAIN, DEAP_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)

#define DEAP_TRACE_ENTRY \
  deap_log("ENTRY: %s(): %d", G_STRFUNC, __LINE__)

#define DEAP_TRACE_EXIT \
  deap_log(" EXIT: %s(): %d", G_STRFUNC, __LINE__)


#define deap_trace_msg(fmt, ...) \
  deap_log("  MSG: %s(): %d: " fmt, G_STRFUNC, __LINE__, ##__VA_ARGS__)

#define deap_info_msg(fmt, ...) \
  g_info ("  MSG: %s(): %d: " fmt, G_STRFUNC, __LINE__, ##__VA_ARGS__)

#define deap_debug_msg(fmt, ...) \
  g_debug("  MSG: %s(): %d: " fmt, G_STRFUNC, __LINE__, ##__VA_ARGS__)

#define deap_error_msg(fmt, ...) \
  g_error("  MSG: %s(): %d: " fmt, G_STRFUNC, __LINE__, ##__VA_ARGS__)

#define deap_warn_msg(fmt, ...) \
  g_warning ("  MSG: %s(): %d: " fmt, G_STRFUNC, __LINE__, ##__VA_ARGS__)

#define deap_critical_msg(fmt, ...) \
  g_critical("  MSG: %s(): %d: " fmt, G_STRFUNC, __LINE__, ##__VA_ARGS__)


#endif

G_END_DECLS
