/* deap-application.c
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
#include "deap-debug.h"
#include "deap-application.h"
#include "deap-window.h"

#include "gtd-log.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

struct _DeapApplication
{
  DzlApplication    parent_instance;
  
  GtkWidget         *window;
};

G_DEFINE_TYPE (DeapApplication, deap_application, DZL_TYPE_APPLICATION)

static void
deap_application_startup (GApplication *application)
{
  DeapApplication *self = DEAP_APPLICATION (application);
  
  G_APPLICATION_CLASS (deap_application_parent_class)->startup (application);
  
  /* Window */
  self->window = deap_window_new (self);
}

static void
deap_application_activate (GApplication *application)
{
  DeapApplication *self = DEAP_APPLICATION (application);
  
  G_APPLICATION_CLASS (deap_application_parent_class)->activate (application);
  
  gtk_window_present (GTK_WINDOW (self->window));
}

static gint
deap_application_handle_local_options (GApplication *application,
                                       GVariantDict *options)
{
  if (g_variant_dict_contains (options, "debug"))
    gtd_log_init ();

  return -1;
}

static void
deap_application_class_init (DeapApplicationClass *klass)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);
  
  application_class->startup = deap_application_startup;
  application_class->activate = deap_application_activate;
  application_class->handle_local_options = deap_application_handle_local_options;
}

static void
deap_application_init (DeapApplication *self)
{
  static GOptionEntry command_options[] = {
      { "debug", 'd', 0, G_OPTION_ARG_NONE, NULL, N_("Enable debug mode"), NULL },
      { NULL }
  };
  
  g_application_add_main_option_entries (G_APPLICATION (self), command_options);
}

DeapApplication *
deap_application_new (void)
{
  return DEAP_APPLICATION (g_object_new (DEAP_TYPE_APPLICATION, NULL));
}
