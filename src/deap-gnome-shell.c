/* deap-gnome-shell.c
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
#include "deap-gnome-shell.h"

#include <gio/gio.h>

struct _DeapGnomeShell
{
  GtkWindow     parent_window;

  GDBusProxy    *shell;
  GCancellable  *cancellable;

  /* Widgets */
  GtkWidget     *list_box;
  GtkLabel      *shell_version;
  GtkButton     *show_applications;
};

G_DEFINE_TYPE (DeapGnomeShell, deap_gnome_shell, GTK_TYPE_WINDOW)

static gboolean
execute_show_applications_cb (GtkButton *button,
                             gpointer   user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);

  g_dbus_proxy_call (self->shell,
                     "ShowApplications",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     self);

  return TRUE;
}

static void
get_shell_version (DeapGnomeShell *self)
{
  g_autoptr(GVariant) gvar_ver = NULL;
  const gchar *ret;
  gsize len;

  g_return_if_fail (self != NULL);
  g_return_if_fail (self->shell != NULL);

  gvar_ver = g_dbus_proxy_get_cached_property (self->shell, "ShellVersion");
  if (gvar_ver == NULL) {
    g_warning ("ShellVersion property is not cached yet");
    return;
  }

  ret = g_variant_get_string (gvar_ver, &len);
  if (len == 0)
    return;

  gtk_label_set_text (self->shell_version, ret);
}

static void
shell_proxy_acquired_cb (GObject      *source,
                         GAsyncResult *res,
                         gpointer      user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);
  g_autoptr(GError) error = NULL;

  self->shell = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (error)
    g_warning ("Error acquiring org.gnome.Shell: %s", error->message);
  else {
    g_info ("Acquired org.gnome.Shell");
    get_shell_version (self);
  }
}

/* --- GObjet --- */
static void
deap_gnome_shell_dispose (GObject *object)
{
  G_OBJECT_CLASS (deap_gnome_shell_parent_class)->dispose (object);
}

static void
deap_gnome_shell_finalize (GObject *object)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (object);

  if (self->cancellable) {
    g_cancellable_cancel (self->cancellable);
    g_clear_object (&self->cancellable);
  }

  g_clear_object (&self->shell);

  G_OBJECT_CLASS (deap_gnome_shell_parent_class)->finalize (object);
}

static void
deap_gnome_shell_class_init (DeapGnomeShellClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = deap_gnome_shell_dispose;
  object_class->finalize = deap_gnome_shell_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/memnoth/Deap/deap-gnome-shell.ui");
  gtk_widget_class_bind_template_child (widget_class, DeapGnomeShell, list_box);
  gtk_widget_class_bind_template_child (widget_class, DeapGnomeShell, shell_version);
  gtk_widget_class_bind_template_child (widget_class, DeapGnomeShell, show_applications);

  gtk_widget_class_bind_template_callback (widget_class, execute_show_applications_cb);
}

static void
deap_gnome_shell_init (DeapGnomeShell *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->cancellable = g_cancellable_new ();

  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL, /* GDBusInterfaceInfo */
                            "org.gnome.Shell",
                            "/org/gnome/Shell",
                            "org.gnome.Shell",
                            self->cancellable,
                            shell_proxy_acquired_cb, /* Callback */
                            self);
}

static GtkWidget *
deap_gnome_shell_new (void)
{
  return GTK_WIDGET (g_object_new (DEAP_TYPE_GNOME_SHELL, NULL));
}

GtkWidget *
deap_gnome_shell_get_instance (void)
{
  static GtkWidget * instance = NULL;

  if (instance == NULL) {
    instance = deap_gnome_shell_new ();
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer) &instance);
  }

  return instance;
}
