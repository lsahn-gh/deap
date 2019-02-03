/* deap-registrable.h
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

#pragma once

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DEAP_TYPE_REGISTRABLE (deap_registrable_get_type ())

G_DECLARE_INTERFACE (DeapRegistrable, deap_registrable, DEAP, REGISTRABLE, GObject)

struct _DeapRegistrableInterface
{
  GTypeInterface parent;

  void (*register_dbus) (DeapRegistrable  *self);
};

void deap_registrable_register_dbus (DeapRegistrable  *self);

G_END_DECLS
