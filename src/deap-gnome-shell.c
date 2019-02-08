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

  /* org.gnome.Shell */
  GDBusProxy    *shell;
  GCancellable  *cancellable;

  /* org.gnome.Shell.Extensions */
  GDBusProxy    *ext_shell;
  GCancellable  *ext_cancellable;

  /* Widgets */
  GtkWidget     *show_applications;
  GtkWidget     *focus_search;
  GtkWidget     *exts_list_box;

  gchar         *window_title;
  GPtrArray     *shell_ext_infos;
};

typedef struct
{
  gchar *name;
  gchar *description;
  gchar *url;
  gchar *uuid;
} ShellExtInfo;

G_DEFINE_TYPE (DeapGnomeShell, deap_gnome_shell, GTK_TYPE_WINDOW)

/* value must be {sv} type. */
static gpointer
shell_ext_info_new (GVariant *value)
{
  ShellExtInfo *info;
  gchar *name;
  gchar *description;
  gchar *url;
  gchar *uuid;

  info = g_new0 (ShellExtInfo, 1);

  g_variant_lookup (value, "name", "s", &name);
  g_variant_lookup (value, "description", "s", &description);
  g_variant_lookup (value, "url", "s", &url);
  g_variant_lookup (value, "uuid", "s", &uuid);

  info->name = g_strdup (name);
  info->description = g_strdup (description);
  info->url = g_strdup (url);
  info->uuid = g_strdup (uuid);

  return (gpointer) info;
}

static void
shell_ext_info_free (gpointer user_data)
{
  ShellExtInfo *info = (ShellExtInfo *)user_data;

  g_free (info->name);
  g_free (info->description);
  g_free (info->url);
  g_free (info->uuid);
  g_free (info);
}

static GPtrArray *
parse_datas_from_serialized_resource (GVariant *resource)
{
  g_autoptr(GVariant) dict = NULL;
  g_autoptr(GVariantIter) iter = NULL;
  GPtrArray *ret = NULL;
  GVariant *child = NULL;
  gsize len;

  g_return_val_if_fail (resource != NULL, NULL);

  /* type (a{sa{sv}}) */
  g_variant_get (resource, "(@a{?*})", &dict);
  len = g_variant_n_children (dict);

  ret = g_ptr_array_new_full (len, shell_ext_info_free);

  iter = g_variant_iter_new (dict);
  while ((child = g_variant_iter_next_value (iter))) {
    const gchar *key;
    gpointer p = NULL;
    GVariant *val = NULL;

    g_variant_get (child, "{s@a{?*}}", &key, &val);

    p = shell_ext_info_new (val);
    g_ptr_array_add (ret, p);

    g_variant_unref (child);
  }

  return ret;
}

static GtkWidget *
create_row_of_shell_exts_list (gpointer user_data)
{
  ShellExtInfo *info;
  GtkWidget *row;
  GtkWidget *name;

  info = (ShellExtInfo *)user_data;

  row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  name = gtk_label_new (info->name);
  gtk_box_pack_start (GTK_BOX (row), name, TRUE, FALSE, 0);

  gtk_widget_show_all (row);

  return row;
}

static void
add_row_into_shell_exts_list_func (gpointer data,
                                   gpointer user_data)
{
  DeapGnomeShell *self;
  GtkWidget *row;

  self = DEAP_GNOME_SHELL (user_data);
  row = create_row_of_shell_exts_list (data);

  gtk_list_box_insert (GTK_LIST_BOX (self->exts_list_box), row, -1);
}

static void
get_list_extensions_finish (GObject      *source,
                            GAsyncResult *res,
                            gpointer      user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);
  g_autoptr(GVariant) ret = NULL;
  g_autoptr(GError) error = NULL;

  ret = g_dbus_proxy_call_finish (self->ext_shell,
                                        res,
                                        &error);
  if (error) {
    g_warning ("Error org.gnome.ShellExtensions.ListExtensions: %s", error->message);
    return;
  }

  self->shell_ext_infos = parse_datas_from_serialized_resource (ret);

  g_ptr_array_foreach (self->shell_ext_infos, add_row_into_shell_exts_list_func, self);
}

static void
ext_get_list_extensions (DeapGnomeShell *self)
{
  g_return_if_fail (self != NULL);

  g_dbus_proxy_call (self->ext_shell,
                     "ListExtensions",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback) get_list_extensions_finish,
                     self);
}

/*
 * execute_focus_search_cb
 *
 * org.gnome.Shell
 *
 * Method: FocusSearch
 * Property: None
 * Return: None
 */
static gboolean
execute_focus_search_cb (GtkButton *button,
                         gpointer   user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);

  g_dbus_proxy_call (self->shell,
                     "FocusSearch",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     self);

  return TRUE;
}

/*
 * execute_show_applications_cb
 *
 * org.gnome.Shell
 *
 * Method: ShowApplications
 * Property: None
 * Return: None
 */
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

/*
 * get_shell_version
 *
 * org.gnome.Shell
 *
 * Method: None
 * Property: ShellVersion
 * Return gnome shell version
 * Ret type: String
 */
static const gchar *
get_shell_version (DeapGnomeShell *self)
{
  g_autoptr(GVariant) gvar_ver = NULL;
  const gchar *ret;
  gsize len;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (self->shell != NULL, NULL);

  gvar_ver = g_dbus_proxy_get_cached_property (self->shell, "ShellVersion");
  if (gvar_ver == NULL) {
    g_warning ("ShellVersion property is not cached yet");
    return NULL;
  }

  ret = g_variant_get_string (gvar_ver, &len);
  if (len == 0)
    return NULL;

  return ret;
}

static void
shell_proxy_acquired_cb (GObject      *source,
                         GAsyncResult *res,
                         gpointer      user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);
  g_autoptr(GError) error = NULL;
  const gchar *version;

  self->shell = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (error)
    g_warning ("Error acquiring org.gnome.Shell: %s", error->message);
  else {
    g_info ("Acquired org.gnome.Shell");

    if ((version = get_shell_version (self)) != NULL)
      self->window_title = g_strdup_printf ("GNOME Shell %s", version);
    else
      self->window_title = g_strdup ("GNOME Shell");

    gtk_window_set_title (&self->parent_window, self->window_title);
  }
}

static void
ext_shell_proxy_acquired_cb (GObject      *source,
                             GAsyncResult *res,
                             gpointer      user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);
  g_autoptr(GError) error = NULL;

  self->ext_shell = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (error)
    g_warning ("Error acquiring org.gnome.Shell.Extensions: %s", error->message);
  else {
    g_info ("Acquired org.gnome.Shell.Extensions");
    ext_get_list_extensions (self);
  }
}

static void
register_gdbus_proxies (DeapGnomeShell *self)
{
  g_return_if_fail (self != NULL);

  /* org.gnome.Shell */
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

  /* org.gnome.Shell.Extensions */
  self->ext_cancellable = g_cancellable_new ();
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL,
                            "org.gnome.Shell",
                            "/org/gnome/Shell",
                            "org.gnome.Shell.Extensions",
                            self->ext_cancellable,
                            ext_shell_proxy_acquired_cb,
                            self);
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

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);

  g_cancellable_cancel (self->ext_cancellable);
  g_clear_object (&self->ext_cancellable);

  g_clear_object (&self->shell);
  g_clear_object (&self->ext_shell);

  if (self->shell_ext_infos) {
    g_ptr_array_unref (self->shell_ext_infos);
    self->shell_ext_infos = NULL;
  }

  if (self->window_title) {
    g_free (self->window_title);
    self->window_title = NULL;
  }

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

  /* org.gnome.Shell widgets */
  gtk_widget_class_bind_template_child (widget_class, DeapGnomeShell, show_applications);
  gtk_widget_class_bind_template_child (widget_class, DeapGnomeShell, focus_search);
  gtk_widget_class_bind_template_callback (widget_class, execute_show_applications_cb);
  gtk_widget_class_bind_template_callback (widget_class, execute_focus_search_cb);

  /* org.gnome.Shell.Extensions widgets */
  gtk_widget_class_bind_template_child (widget_class, DeapGnomeShell, exts_list_box);
}

static void
deap_gnome_shell_init (DeapGnomeShell *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  register_gdbus_proxies (self);
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
