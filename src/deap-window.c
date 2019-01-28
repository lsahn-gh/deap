/* deap-window.c
 *
 * Copyright 2019 Yi-Soo An
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
 */

#include "deap-config.h"
#include "deap-window.h"

#include "deap-gnome-shell.h"

struct _DeapWindow
{
  GtkApplicationWindow  parent_instance;

  /* Template widgets */
  GtkHeaderBar        *header_bar;
  GtkButton           *button_gnome_shell;

  GtkWidget           *gnome_shell;
};

G_DEFINE_TYPE (DeapWindow, deap_window, GTK_TYPE_APPLICATION_WINDOW)

static gboolean
open_gnome_shell_dialog_cb (GtkButton *button,
                            gpointer   user_data)
{
  DeapWindow *self = DEAP_WINDOW (user_data);

  self->gnome_shell = deap_gnome_shell_get_instance ();
  g_object_add_weak_pointer (G_OBJECT (self->gnome_shell), (gpointer) &self->gnome_shell);

  gtk_widget_show_all (self->gnome_shell);

  return TRUE;
}

static void
deap_window_dispose (GObject *object)
{
  DeapWindow *self = DEAP_WINDOW (object);

  g_clear_object (&self->gnome_shell);

  G_OBJECT_CLASS (deap_window_parent_class)->dispose (object);
}

static void
deap_window_class_init (DeapWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = deap_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/memnoth/Deap/deap-window.ui");
  gtk_widget_class_bind_template_child (widget_class, DeapWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, DeapWindow, button_gnome_shell);

  gtk_widget_class_bind_template_callback (widget_class, open_gnome_shell_dialog_cb);
}

static void
deap_window_init (DeapWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
deap_window_new (DeapApplication *application)
{
  return GTK_WIDGET (g_object_new (DEAP_TYPE_WINDOW,
                                   "application", application,
                                   NULL));
}
