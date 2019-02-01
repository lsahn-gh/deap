/* deap-login1.c
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

#include "deap-config.h"
#include "deap-login1.h"

#include <gio/gio.h>

struct _DeapLogin1
{
  GtkWindow     parent_window;

  GtkWidget     *lock_screen;
};

G_DEFINE_TYPE (DeapLogin1, deap_login1, GTK_TYPE_WINDOW)

/* --- GObject --- */
static void
deap_login1_class_init (DeapLogin1Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  /*
  object_class->dispose = deap_login1_dispose;
  object_class->finalize = deap_login1_finalize;
   */

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/memnoth/Deap/deap-login1.ui");

  gtk_widget_class_bind_template_child (widget_class, DeapLogin1, lock_screen);
  gtk_widget_class_bind_template_callback (widget_class, execute_lock_screen_cb);
}

static void
deap_login1_init (DeapLogin1 *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static GtkWidget *
deap_login1_new (void)
{
  return GTK_WIDGET (g_object_new (DEAP_TYPE_LOGIN1, NULL));
}

GtkWidget *
deap_login1_get_instance (void)
{
  static GtkWidget * instance = NULL;

  if (instance == NULL) {
    instance = deap_login1_new ();
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer) &instance);
  }

  return instance;
}
