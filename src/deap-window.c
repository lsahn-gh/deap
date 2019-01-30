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
#include "deap-login1.h"

struct _DeapWindow
{
  GtkApplicationWindow  parent_instance;

  /* Template widgets */
  GtkHeaderBar        *header_bar;
  GtkListBox          *list_box;

  GtkWidget           *gnome_shell;
  GtkWidget           *login1_window;
};

typedef struct
{
  const gchar *name;
  const gchar *description;
  const gchar *tooltip;

  void (*fptr)(GtkWidget *button,
               gpointer   user_data);
} RowData;

G_DEFINE_TYPE (DeapWindow, deap_window, GTK_TYPE_APPLICATION_WINDOW)


static void
open_login1_window_cb (GtkWidget *button,
                       gpointer   user_data)
{
  DeapWindow *self = DEAP_WINDOW (user_data);

  self->login1_window = deap_login1_get_instance ();
  g_object_add_weak_pointer (G_OBJECT (self->login1_window),
                             (gpointer) &self->login1_window);

  gtk_widget_show_all (self->login1_window);
}

static void
open_gnome_shell_dialog_cb (GtkWidget *button,
                            gpointer   user_data)
{
  DeapWindow *self = DEAP_WINDOW (user_data);

  self->gnome_shell = deap_gnome_shell_get_instance ();
  g_object_add_weak_pointer (G_OBJECT (self->gnome_shell), (gpointer) &self->gnome_shell);

  gtk_widget_show_all (self->gnome_shell);
}

static GtkWidget *
create_list_box_row (DeapWindow    *self,
                     const RowData *data)
{
  GtkWidget *row;
  GtkWidget *button;
  GtkWidget *icon;
  GtkWidget *name;
  GtkWidget *vbox;

  row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (row), 5);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_box_pack_start (GTK_BOX (row), vbox, TRUE, FALSE, 0);

  name = gtk_label_new (data->name);
  gtk_box_pack_start (GTK_BOX (vbox), name, TRUE, FALSE, 0);

  button = gtk_button_new ();
  icon = gtk_image_new_from_icon_name ("media-playback-start-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), icon);
  gtk_widget_set_tooltip_text (button, data->tooltip);
  gtk_widget_set_margin_end (button, 20);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (data->fptr), self);
  gtk_box_pack_end (GTK_BOX (row), button, FALSE, FALSE, 0);

  gtk_widget_show_all (row);

  return row;
}

static const RowData row_table[] = {
    { "org.gnome.Shell", "", "", open_gnome_shell_dialog_cb },
    { "org.freedesktop.login1", "", "", open_login1_window_cb },
    { NULL }
};

static void
create_list_box (DeapWindow *self)
{
  GtkListBox *list_box = self->list_box;
  gsize i;

  for (i = 0; row_table[i].name; i++) {
    GtkWidget *row;

    row = create_list_box_row (self, &row_table[i]);
    gtk_list_box_insert (list_box, row, -1);
  }
}

/* --- GObject --- */
static void
deap_window_dispose (GObject *object)
{
  DeapWindow *self = DEAP_WINDOW (object);

  g_clear_object (&self->gnome_shell);
  g_clear_object (&self->login1_window);

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
  gtk_widget_class_bind_template_child (widget_class, DeapWindow, list_box);

  gtk_widget_class_bind_template_callback (widget_class, open_gnome_shell_dialog_cb);
}

static void
deap_window_init (DeapWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  create_list_box (self);
}

GtkWidget *
deap_window_new (DeapApplication *application)
{
  return GTK_WIDGET (g_object_new (DEAP_TYPE_WINDOW,
                                   "application", application,
                                   NULL));
}
