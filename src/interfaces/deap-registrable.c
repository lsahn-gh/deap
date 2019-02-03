/* deap-registrable.c
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

#include "deap-registrable.h"

G_DEFINE_INTERFACE (DeapRegistrable, deap_registrable, G_TYPE_OBJECT)

static void
deap_registrable_default_init (DeapRegistrableInterface *iface)
{

}

void
deap_registrable_register_dbus (DeapRegistrable *registrable)
{
  g_return_if_fail (DEAP_IS_REGISTRABLE (registrable));
  g_return_if_fail (DEAP_REGISTRABLE_GET_IFACE (registrable)->register_dbus);

  return DEAP_REGISTRABLE_GET_IFACE (registrable)->register_dbus (registrable);
}
