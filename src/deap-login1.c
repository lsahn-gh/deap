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

  /* org.freedesktop.login1 */
  GDBusProxy    *login1;
  GCancellable  *cancellable;

  /* Widgets */
  GtkWidget     *lock_screen;
  GtkWidget     *session_id_entry;
};

G_DEFINE_TYPE (DeapLogin1, deap_login1, GTK_TYPE_WINDOW)


static void
execute_lock_screen_cb (GtkWidget *button,
                        gpointer   user_data)
{
  DeapLogin1 *self = DEAP_LOGIN1 (user_data);
  const gchar *session_id;
  const gchar *p;

  session_id = gtk_entry_get_text (GTK_ENTRY (self->session_id_entry));
  if (*session_id == '\0') {
    g_warning ("Session ID entry is empty");
    return;
  }

  /* Does it have any alphabets? if so, just return */
  for (p = session_id; *p != '\0'; p++) {
    if (!g_ascii_isdigit (*p)) {
      g_warning ("Only numbers are allowed");
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

static void
login1_proxy_acquired_cb (GObject      *source,
                          GAsyncResult *res,
                          gpointer      user_data)
{
  DeapLogin1 *self = DEAP_LOGIN1 (user_data);
  g_autoptr(GError) error = NULL;

  self->login1 = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (error)
    g_warning ("Error acquiring org.freedesktop.login1: %s", error->message);
  else
    g_info ("Acquired org.freedesktop.login1");
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

  gtk_widget_class_bind_template_child (widget_class, DeapLogin1, lock_screen);
  gtk_widget_class_bind_template_child (widget_class, DeapLogin1, session_id_entry);
  gtk_widget_class_bind_template_callback (widget_class, execute_lock_screen_cb);
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
