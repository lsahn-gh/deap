/* deap-screen-brightness.c
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
#include "deap-screen-brightness.h"

struct _DeapScreenBrightness
{
  GtkWindow     parent_window;

  /* Widgets */
  GtkWidget     *list_box;
};

G_DEFINE_TYPE (DeapScreenBrightness, deap_screen_brightness, GTK_TYPE_WINDOW)

static void
deap_screen_brightness_dispose (GObject *object)
{
  DeapScreenBrightness *self = DEAP_SCREEN_BRIGHTNESS (object);

  G_OBJECT_CLASS (deap_screen_brightness_parent_class)->dispose (object);
}

static void
deap_screen_brightness_class_init (DeapScreenBrightnessClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = deap_screen_brightness_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/memnoth/Deap/deap-screen-brightness.ui");
  gtk_widget_class_bind_template_child (widget_class, DeapScreenBrightness, list_box);
}

static void
deap_screen_brightness_init (DeapScreenBrightness *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static GtkWidget *
deap_screen_brightness_new (void)
{
  return GTK_WIDGET (g_object_new (DEAP_TYPE_SCREEN_BRIGHTNESS, NULL));
}

GtkWidget *
deap_screen_brightness_get_instance (void)
{
  static GtkWidget * instance = NULL;

  if (instance == NULL) {
    instance = deap_screen_brightness_new ();
    g_object_add_weak_pointer (G_OBJECT (instance), &instance);
  }

  return instance;
}
