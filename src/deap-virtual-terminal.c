/* deap-virtual-terminal.c
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

#define G_LOG_DOMAIN "DeapVirtualTerminal"

#include "deap-debug.h"
#include "deap-virtual-terminal.h"

#include <vte/vte.h>

struct _DeapVirtualTerminal
{
  GtkBox      parent_instance;

  GtkWidget   *main_box;
  GtkWidget   *terminal;
};

G_DEFINE_TYPE (DeapVirtualTerminal, deap_virtual_terminal, GTK_TYPE_BOX)


static void
internal_spawn_terminal (DeapVirtualTerminal *self)
{
  gchar* command[2] = { NULL };

  DEAP_TRACE_ENTRY;

  command[0] = vte_get_user_shell ();
  vte_terminal_spawn_async (VTE_TERMINAL (self->terminal),
                            VTE_PTY_DEFAULT,
                            NULL,
                            command,
                            NULL,
                            G_SPAWN_DEFAULT,
                            NULL, NULL, NULL,
                            5000,
                            NULL,
                            NULL,
                            NULL);

  DEAP_TRACE_EXIT;
}

static void
internal_respawn_terminal_cb (VteTerminal *vteterminal,
                              gint         status,
                              gpointer     user_data)
{
  DeapVirtualTerminal *self;

  DEAP_TRACE_ENTRY;

  self = DEAP_VIRTUAL_TERMINAL (user_data);
  internal_spawn_terminal (self);

  DEAP_TRACE_EXIT;
}

/* --- GObject --- */
static void
deap_virtual_terminal_finalize (GObject *object)
{
  G_OBJECT_CLASS (deap_virtual_terminal_parent_class)->finalize (object);
}
static void
deap_virtual_terminal_class_init (DeapVirtualTerminalClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = deap_virtual_terminal_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/memnoth/Deap/deap-virtual-terminal.ui");

  gtk_widget_class_bind_template_child (widget_class, DeapVirtualTerminal, main_box);
}

static void
deap_virtual_terminal_init (DeapVirtualTerminal *self)
{

  gtk_widget_init_template (GTK_WIDGET (self));

  self->terminal = vte_terminal_new ();
  internal_spawn_terminal (self);
  g_signal_connect (self->terminal,
                    "child-exited",
                    G_CALLBACK (internal_respawn_terminal_cb),
                    self);

  gtk_container_add (GTK_CONTAINER (self->main_box), self->terminal);
  gtk_widget_show_all (self->main_box);
}

static GtkWidget *
deap_virtual_terminal_new (void)
{
  return GTK_WIDGET (g_object_new (DEAP_TYPE_VIRTUAL_TERMINAL, NULL));
}

GtkWidget *
deap_virtual_terminal_get_instance (void)
{
  static GtkWidget * instance = NULL;

  if (instance == NULL) {
    instance = deap_virtual_terminal_new ();
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer) &instance);
  }

  return instance;
}
