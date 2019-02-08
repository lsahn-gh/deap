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

  GtkWidget           *prefs_view;

  GtkWidget           *gnome_shell;
  GtkWidget           *login1;
};

typedef struct
{
  const gchar *title;
  const gchar *description;
  const gchar *page_name;
  GtkWidget   *widget;
} PrefsItem;

G_DEFINE_TYPE (DeapWindow, deap_window, GTK_TYPE_APPLICATION_WINDOW)


static void
add_preferences (DzlPreferences  *prefs,
                 const PrefsItem *items)
{
  gsize i;

  for (i = 0; items[i].title; i++) {
    dzl_preferences_add_page (prefs, items[i].page_name, items[i].title, i);
    dzl_preferences_add_group (prefs, items[i].page_name, "basic", NULL, i);
    dzl_preferences_add_custom (prefs, items[i].page_name, "basic", items[i].widget, NULL, 0);
  }

  dzl_preferences_set_page (prefs, items[0].page_name, NULL);
}

/* --- GObject --- */
static void
deap_window_dispose (GObject *object)
{
  DeapWindow *self = DEAP_WINDOW (object);

  (void)self;

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
  gtk_widget_class_bind_template_child (widget_class, DeapWindow, prefs_view);
}

static void
deap_window_init (DeapWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* title, description, page_name, widget */
  static PrefsItem item_table[] = {
      { "org.gnome.Shell", "", "gnome-shell", NULL },
      { "org.freedesktop.login1", "", "freedesktop-login1", NULL },
      { NULL }
  };

  self->gnome_shell = deap_gnome_shell_get_instance ();
  item_table[0].widget = self->gnome_shell;

  self->login1 = deap_login1_get_instance ();
  item_table[1].widget = self->login1;

  add_preferences (DZL_PREFERENCES (self->prefs_view), item_table);
}

GtkWidget *
deap_window_new (DeapApplication *application)
{
  return GTK_WIDGET (g_object_new (DEAP_TYPE_WINDOW,
                                   "application", application,
                                   NULL));
}
