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

#define G_LOG_DOMAIN "DeapLogin1"

#include "deap-config.h"
#include "deap-debug.h"
#include "deap-login1.h"

#include <gio/gio.h>

struct _DeapLogin1
{
  GtkBox        parent_instance;

  /* org.freedesktop.login1 */
  GDBusProxy    *login1;
  GCancellable  *cancellable;

  /* Widgets */
  GtkWidget     *session_list;
  GtkWidget     *lock_screen;
  GtkWidget     *session_id_entry;

  GPtrArray     *sessions;
};

typedef struct
{
  gchar *session_id;
  gchar *user_id;
  gchar *user_name;
  gchar *seat_id;
  gchar *obj_path;
} Login1Session;

G_DEFINE_TYPE (DeapLogin1, deap_login1, GTK_TYPE_BOX)

#define LOGIN1_SESSION(_ptr)  ((Login1Session*)_ptr)


static gpointer
login1_session_new (const gchar *session_id,
                    guint32      user_id,
                    const gchar *user_name,
                    const gchar *seat_id,
                    const gchar *obj_path)
{
  Login1Session *session;

  session = g_new0 (Login1Session, 1);

  session->session_id = g_strdup (session_id);
  session->user_id = g_strdup_printf ("%d", user_id);
  session->user_name = g_strdup (user_name);
  session->seat_id = g_strdup (seat_id);
  session->obj_path = g_strdup (obj_path);

  return (gpointer) session;
}

static void
login1_session_free (gpointer user_data)
{
  Login1Session *session;

  g_return_if_fail (user_data != NULL);

  session = LOGIN1_SESSION (user_data);

  g_free (session->session_id);
  g_free (session->user_id);
  g_free (session->user_name);
  g_free (session->seat_id);
  g_free (session->obj_path);
  g_free (session);
}

static GPtrArray *
parse_from_serialized_dbus_data (GVariant *resource)
{
  g_autoptr(GVariantIter) iter = NULL;
  GPtrArray *ret;
  gsize len;

  g_return_val_if_fail (resource != NULL, NULL);

  g_variant_get (resource, "(a(susso))", &iter);
  len = g_variant_iter_n_children (iter);

  ret = g_ptr_array_new_full (len, login1_session_free);

  {
    gchar *session_id;
    guint32 user_id;
    gchar *user_name;
    gchar *seat_id;
    gchar *obj_path;

    while (g_variant_iter_loop (iter, "(susso)", &session_id, &user_id, &user_name, &seat_id, &obj_path)) {
      gpointer p;

      p = login1_session_new (session_id, user_id, user_name, seat_id, obj_path);
      g_ptr_array_add (ret, p);
    }
  }

  return ret;
}

static GtkWidget *
create_session_list_row (gpointer user_data)
{
  Login1Session *session;
  GtkWidget *row;
  GtkWidget *hbox;
  GtkWidget *session_id;
  GtkWidget *user_id;
  GtkWidget *user_name;

  session = LOGIN1_SESSION (user_data);

  row = gtk_list_box_row_new ();
  g_object_set_data (G_OBJECT (row), "session-id", g_strdup (session->session_id));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  session_id = gtk_label_new (session->session_id);
  gtk_box_pack_start (GTK_BOX (hbox), session_id, TRUE, FALSE, 0);

  user_id = gtk_label_new (session->user_id);
  gtk_box_pack_start (GTK_BOX (hbox), user_id, TRUE, FALSE, 0);

  user_name = gtk_label_new (session->user_name);
  gtk_box_pack_start (GTK_BOX (hbox), user_name, TRUE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (row), hbox);
  gtk_widget_show_all (row);

  return row;
}

static void
add_row_to_session_list_func (gpointer data,
                              gpointer user_data)
{
  DeapLogin1 *self;
  GtkWidget *row;

  self = DEAP_LOGIN1 (user_data);
  row = create_session_list_row (data);

  gtk_list_box_insert (GTK_LIST_BOX (self->session_list), row, -1);
}

static void
get_session_list_finish (GObject      *source,
                         GAsyncResult *res,
                         gpointer      user_data)
{
  DeapLogin1 *self = DEAP_LOGIN1 (user_data);
  g_autoptr(GVariant) ret = NULL;
  g_autoptr(GError) error = NULL;

  ret = g_dbus_proxy_call_finish (self->login1,
                                  res,
                                  &error);
  if (error) {
    deap_warn_msg ("Error org.freedesktop.login1.Manager.ListSessions: %s", error->message);
    return;
  }

  self->sessions = parse_from_serialized_dbus_data (ret);
  g_ptr_array_foreach (self->sessions, add_row_to_session_list_func, self);
}

static void
get_session_list (DeapLogin1 *self)
{
  g_dbus_proxy_call (self->login1,
                     "ListSessions",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     (GAsyncReadyCallback) get_session_list_finish,
                     self);
}

static void
login1_proxy_acquired_cb (GObject      *source,
                          GAsyncResult *res,
                          gpointer      user_data)
{
  DeapLogin1 *self = DEAP_LOGIN1 (user_data);
  g_autoptr(GError) error = NULL;

  self->login1 = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (error)
    deap_warn_msg ("Error acquiring org.freedesktop.login1: %s", error->message);
  else {
    deap_info_msg ("org.freedesktop.login1 successfully acquired");
    get_session_list (self);
  }
}

static void
register_gdbus_proxies (DeapLogin1 *self)
{
  g_return_if_fail (self != NULL);

  self->cancellable = g_cancellable_new ();
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL, /* GDBusInterfaceInfo */
                            "org.freedesktop.login1",
                            "/org/freedesktop/login1",
                            "org.freedesktop.login1.Manager",
                            self->cancellable,
                            login1_proxy_acquired_cb,
                            self);
}

/* --- Callbacks for Widgets --- */
static void
on_session_list_row_selected_cb (GtkListBox    *box,
                                 GtkListBoxRow *row,
                                 gpointer       user_data)
{
  DeapLogin1 *self = DEAP_LOGIN1 (user_data);
  const gchar *session_id = NULL;

  session_id = g_object_get_data (G_OBJECT (row), "session-id");

  gtk_entry_set_text (GTK_ENTRY (self->session_id_entry), session_id);
}

static void
execute_lock_screen_cb (GtkWidget *button,
                        gpointer   user_data)
{
  DeapLogin1 *self = DEAP_LOGIN1 (user_data);
  const gchar *session_id;
  const gchar *p;

  session_id = gtk_entry_get_text (GTK_ENTRY (self->session_id_entry));
  if (*session_id == '\0') {
    deap_warn_msg ("Session ID entry is empty");
    return;
  }

  /* Does it have any alphabets? if so, just return */
  for (p = session_id; *p != '\0'; p++) {
    if (!g_ascii_isdigit (*p)) {
      deap_warn_msg ("Only numbers are allowed");
      return;
    }
  }

  g_dbus_proxy_call (self->login1,
                     "LockSession",
                     g_variant_new ("(s)", session_id),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     NULL,
                     NULL,
                     self);
}
/* --- End of Callbacks --- */


/* --- GObject --- */
static void
deap_login1_dispose (GObject *object)
{
  G_OBJECT_CLASS (deap_login1_parent_class)->dispose (object);
}

static void
deap_login1_finalize (GObject *object)
{
  DeapLogin1 *self = DEAP_LOGIN1 (object);

  if (self->cancellable) {
    g_cancellable_cancel (self->cancellable);
    self->login1 = NULL;
  }

  g_clear_object (&self->login1);

  G_OBJECT_CLASS (deap_login1_parent_class)->finalize (object);
}

static void
deap_login1_class_init (DeapLogin1Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = deap_login1_dispose;
  object_class->finalize = deap_login1_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/memnoth/Deap/deap-login1.ui");

  gtk_widget_class_bind_template_child (widget_class, DeapLogin1, session_list);
  gtk_widget_class_bind_template_child (widget_class, DeapLogin1, lock_screen);
  gtk_widget_class_bind_template_child (widget_class, DeapLogin1, session_id_entry);
  gtk_widget_class_bind_template_callback (widget_class, execute_lock_screen_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_session_list_row_selected_cb);
}

static void
deap_login1_init (DeapLogin1 *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  register_gdbus_proxies (self);
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
