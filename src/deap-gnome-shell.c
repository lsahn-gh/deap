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

#define G_LOG_DOMAIN "DeapGnomeShell"

#include "deap-config.h"
#include "deap-debug.h"
#include "deap-gnome-shell.h"

#include <gio/gio.h>

struct _DeapGnomeShell
{
  GtkBox        parent_instance;

  /* org.gnome.Shell */
  GDBusProxy    *shell;
  GCancellable  *cancellable;

  /* org.gnome.Shell.Extensions */
  GDBusProxy    *shell_extension;
  GCancellable  *extension_cancellable;

  /* Widgets */
  GtkWidget     *show_applications;
  GtkWidget     *focus_search;
  GtkWidget     *extension_list_box;
  GtkWidget     *popover_menu;

  GActionGroup  *action_group;

  gchar         *shell_version;
  GPtrArray     *shell_extension_infos;
};

typedef struct
{
  gchar *name;
  gchar *description;
  gchar *url;
  gchar *uuid;
} ShellExtensionInfo;

G_DEFINE_TYPE (DeapGnomeShell, deap_gnome_shell, GTK_TYPE_BOX)

#define G_PTR_ARRAY_GET_LENGTH(_arrptr)   ((_arrptr)->len)
#define SHELL_EXTENSION_INFO(_val)        ((ShellExtensionInfo*)_val)


/* --- Shell Extension Proxy --- */
static gpointer
shell_extension_info_new (GVariant *value   /* {sv} type */)
{
  ShellExtensionInfo *info;
  gchar *name;
  gchar *description;
  gchar *url;
  gchar *uuid;

  info = g_new0 (ShellExtensionInfo, 1);

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
shell_extension_info_free (gpointer user_data)
{
  ShellExtensionInfo *info = SHELL_EXTENSION_INFO (user_data);

  g_free (info->name);
  g_free (info->description);
  g_free (info->url);
  g_free (info->uuid);
  g_free (info);
}

static GPtrArray *
parse_from_serialized_dbus_data (GVariant *resource)
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

  ret = g_ptr_array_new_full (len, shell_extension_info_free);

  iter = g_variant_iter_new (dict);
  while ((child = g_variant_iter_next_value (iter))) {
    const gchar *key;
    gpointer p = NULL;
    GVariant *val = NULL;

    g_variant_get (child, "{s@a{?*}}", &key, &val);

    p = shell_extension_info_new (val);
    g_ptr_array_add (ret, p);

    g_variant_unref (child);
  }

  return ret;
}

static GtkWidget *
create_extension_list_row (gpointer user_data)
{
  ShellExtensionInfo *info;
  GtkWidget *row;
  GtkWidget *hbox;
  GtkWidget *name;

  info = (ShellExtensionInfo *)user_data;

  row = gtk_list_box_row_new ();
  g_object_set_data (G_OBJECT (row), "uuid", g_strdup (info->uuid));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  name = gtk_label_new (info->name);
  gtk_box_pack_start (GTK_BOX (hbox), name, TRUE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (row), hbox);
  gtk_widget_show_all (row);

  return row;
}

static void
add_row_into_extension_list_func (gpointer data,
                                  gpointer user_data)
{
  DeapGnomeShell *self;
  GtkWidget *row;

  self = DEAP_GNOME_SHELL (user_data);
  row = create_extension_list_row (data);

  gtk_list_box_insert (GTK_LIST_BOX (self->extension_list_box), row, -1);
}

static void
get_extension_list_finish (GObject      *source,
                           GAsyncResult *res,
                           gpointer      user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);
  g_autoptr(GVariant) ret = NULL;
  g_autoptr(GError) error = NULL;

  ret = g_dbus_proxy_call_finish (self->shell_extension,
                                        res,
                                        &error);
  if (error) {
    g_warning ("Error org.gnome.ShellExtensions.ListExtensions: %s", error->message);
    return;
  }

  self->shell_extension_infos = parse_from_serialized_dbus_data (ret);
  g_ptr_array_foreach (self->shell_extension_infos, add_row_into_extension_list_func, self);
}

static void
get_extension_list (DeapGnomeShell *self)
{
  g_return_if_fail (self != NULL);

  g_dbus_proxy_call (self->shell_extension,
                     "ListExtensions",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback) get_extension_list_finish,
                     self);
}

static void
shell_extension_proxy_acquired_cb (GObject      *source,
                                   GAsyncResult *res,
                                   gpointer      user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);
  g_autoptr(GError) error = NULL;

  self->shell_extension = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (error)
    g_warning ("Error acquiring org.gnome.Shell.Extensions: %s", error->message);
  else {
    g_info ("org.gnome.Shell.Extensions successfully acquired");
    get_extension_list (self);
  }
}
/* --- End of Shell Extension Proxy --- */


/* --- Shell Proxy --- */
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

  self->shell = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (error)
    g_warning ("Error acquiring org.gnome.Shell: %s", error->message);
  else {
    g_info ("org.gnome.Shell successfully acquired");
    self->shell_version = g_strdup (get_shell_version (self));
  }
}
/* --- End of Shell Proxy --- */


/* --- Callbacks for Widgets --- */
static const gchar *
get_uuid_from_row (GtkListBoxRow *row)
{
  return g_object_get_data (G_OBJECT (row), "uuid");
}

static gboolean
is_uuid_in_row (GtkListBoxRow *row)
{
  return get_uuid_from_row (row) != NULL;
}

static void
extension_option_launch_cb (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);
  GtkListBoxRow *row = NULL;
  const gchar *uuid = NULL;

  DEAP_TRACE_ENTRY;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (self->extension_list_box));
  if (row == NULL) {
    deap_warn_msg ("There is no selected row");
    return;
  }

  uuid = get_uuid_from_row (row);
  if (uuid == NULL) {
    deap_warn_msg ("The selected row has no UUID");
    return;
  }

  g_dbus_proxy_call (self->shell_extension,
                     "LaunchExtensionPrefs",
                     g_variant_new ("(s)", g_strdup (uuid)),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     NULL);
  deap_trace_msg ("UUID: %s", uuid);

  DEAP_TRACE_EXIT;
}

static gboolean
on_listbox_button_press_cb (GtkWidget      *widget,
                            GdkEventButton *event,
                            gpointer        user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);

  if (event->button == 3) {
    GtkListBoxRow *row = gtk_list_box_get_selected_row (GTK_LIST_BOX (self->extension_list_box));

    if (row == NULL)
      return FALSE;

    gtk_popover_set_relative_to (GTK_POPOVER (self->popover_menu), GTK_WIDGET (row));
    gtk_popover_popup (GTK_POPOVER (self->popover_menu));

    return TRUE;
  }

  return FALSE;
}

static void
update_selection_actions (GActionGroup *action_group,
                          gboolean      has_selection)
{
  GAction *launch_action;

  launch_action = g_action_map_lookup_action (G_ACTION_MAP (action_group), "launch");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (launch_action), has_selection);
}

static void
on_listbox_row_selected_cb (GtkListBox    *box,
                            GtkListBoxRow *row,
                            gpointer       user_data)
{
  DeapGnomeShell *self = DEAP_GNOME_SHELL (user_data);

  update_selection_actions (self->action_group, row != NULL && is_uuid_in_row (row));
}
/* --- End of Callbacks --- */


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

  g_cancellable_cancel (self->extension_cancellable);
  g_clear_object (&self->extension_cancellable);

  g_clear_object (&self->shell);
  g_clear_object (&self->shell_extension);

  if (self->shell_extension_infos) {
    g_ptr_array_unref (self->shell_extension_infos);
    self->shell_extension_infos = NULL;
  }

  if (self->shell_version) {
    g_free (self->shell_version);
    self->shell_version = NULL;
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
  gtk_widget_class_bind_template_child (widget_class, DeapGnomeShell, popover_menu);
  gtk_widget_class_bind_template_callback (widget_class, execute_show_applications_cb);
  gtk_widget_class_bind_template_callback (widget_class, execute_focus_search_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_listbox_button_press_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_listbox_row_selected_cb);

  /* org.gnome.Shell.Extensions widgets */
  gtk_widget_class_bind_template_child (widget_class, DeapGnomeShell, extension_list_box);
}

static void
create_action_group (DeapGnomeShell *self)
{
  const GActionEntry entries[] = {
      { "launch", extension_option_launch_cb }
  };
  GSimpleActionGroup *group;

  group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (group), entries, G_N_ELEMENTS (entries), self);

  self->action_group = G_ACTION_GROUP (group);
  gtk_widget_insert_action_group (GTK_WIDGET (self), "extension", self->action_group);
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
  self->extension_cancellable = g_cancellable_new ();
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL,
                            "org.gnome.Shell",
                            "/org/gnome/Shell",
                            "org.gnome.Shell.Extensions",
                            self->extension_cancellable,
                            shell_extension_proxy_acquired_cb,
                            self);
}

static void
deap_gnome_shell_init (DeapGnomeShell *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  create_action_group (self);
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
