/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Christian Kellner, Samuel Cormier-Iijima
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 */

#include <config.h>
#include <glib.h>

#include "gsocketaddress.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "gnetworkingprivate.h"
#include "glibintl.h"
#include "gioenumtypes.h"

#ifdef G_OS_UNIX
#include "gunixsocketaddress.h"
#endif

#include "gioalias.h"

/**
 * SECTION:gsocketaddress
 * @short_description: Abstract base class representing endpoints for
 * socket communication
 *
 * #GSocketAddress is the equivalent of <type>struct sockaddr</type>
 * in the BSD sockets API. This is an abstract class; use
 * #GInetSocketAddress for internet sockets, or #GUnixSocketAddress
 * for UNIX domain sockets.
 **/

/**
 * GSocketAddress:
 *
 * A socket endpoint address, corresponding to <type>struct sockaddr</type>
 * or one of its subtypes.
 **/

enum
{
  PROP_NONE,
  PROP_FAMILY
};

G_DEFINE_ABSTRACT_TYPE (GSocketAddress, g_socket_address, G_TYPE_OBJECT);

/**
 * g_socket_address_get_family:
 * @address: a #GSocketAddress
 *
 * Gets the socket family type of @address.
 *
 * Returns: the socket family type of @address.
 *
 * Since: 2.22
 */
GSocketFamily
g_socket_address_get_family (GSocketAddress *address)
{
  g_return_val_if_fail (G_IS_SOCKET_ADDRESS (address), 0);

  return G_SOCKET_ADDRESS_GET_CLASS (address)->get_family (address);
}

static void
g_socket_address_get_property (GObject *object, guint prop_id,
			       GValue *value, GParamSpec *pspec)
{
  GSocketAddress *address = G_SOCKET_ADDRESS (object);

  switch (prop_id)
    {
     case PROP_FAMILY:
      g_value_set_enum (value, g_socket_address_get_family (address));
      break;

     default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_socket_address_class_init (GSocketAddressClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = g_socket_address_get_property;

  g_object_class_install_property (gobject_class, PROP_FAMILY,
                                   g_param_spec_enum ("family",
						      _("Address family"),
						      _("The family of the socket address"),
						      G_TYPE_SOCKET_FAMILY,
						      G_SOCKET_FAMILY_INVALID,
						      G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));
}

static void
g_socket_address_init (GSocketAddress *address)
{

}

/**
 * g_socket_address_get_native_size:
 * @address: a #GSocketAddress
 *
 * Gets the size of @address's native <type>struct sockaddr</type>.
 * You can use this to allocate memory to pass to
 * g_socket_address_to_native().
 *
 * Returns: the size of the native <type>struct sockaddr</type> that
 * @address represents
 *
 * Since: 2.22
 */
gssize
g_socket_address_get_native_size (GSocketAddress *address)
{
  g_return_val_if_fail (G_IS_SOCKET_ADDRESS (address), -1);

  return G_SOCKET_ADDRESS_GET_CLASS (address)->get_native_size (address);
}

/**
 * g_socket_address_to_native:
 * @address: a #GSocketAddress
 * @dest: a pointer to a memory location that will contain the native
 * <type>struct sockaddr</type>.
 * @destlen: the size of @dest. Must be at least as large as
 * g_socket_address_get_native_size().
 *
 * Converts a #GSocketAddress to a native <type>struct
 * sockaddr</type>, which can be passed to low-level functions like
 * connect() or bind().
 *
 * Returns: %TRUE if @dest was filled in, %FALSE if @address is invalid
 * or @destlen is too small.
 *
 * Since: 2.22
 */
gboolean
g_socket_address_to_native (GSocketAddress *address,
			    gpointer        dest,
			    gsize           destlen)
{
  g_return_val_if_fail (G_IS_SOCKET_ADDRESS (address), FALSE);

  return G_SOCKET_ADDRESS_GET_CLASS (address)->to_native (address, dest, destlen);
}

/**
 * g_socket_address_new_from_native:
 * @native: a pointer to a <type>struct sockaddr</type>
 * @len: the size of the memory location pointed to by @native
 *
 * Creates a #GSocketAddress subclass corresponding to the native
 * <type>struct sockaddr</type> @native.
 *
 * Returns: a new #GSocketAddress if @native could successfully be converted,
 * otherwise %NULL.
 *
 * Since: 2.22
 */
GSocketAddress *
g_socket_address_new_from_native (gpointer native,
				  gsize    len)
{
  gshort family;

  if (len < sizeof (gshort))
    return NULL;

  family = ((struct sockaddr *) native)->sa_family;

  if (family == AF_UNSPEC)
    return NULL;

  if (family == AF_INET)
    {
      struct sockaddr_in *addr = (struct sockaddr_in *) native;
      GInetAddress *iaddr = g_inet_address_new_from_bytes ((guint8 *) &(addr->sin_addr), AF_INET);
      GSocketAddress *sockaddr;

      sockaddr = g_inet_socket_address_new (iaddr, g_ntohs (addr->sin_port));
      g_object_unref (iaddr);
      return sockaddr;
    }

  if (family == AF_INET6)
    {
      struct sockaddr_in6 *addr = (struct sockaddr_in6 *) native;
      GInetAddress *iaddr = g_inet_address_new_from_bytes ((guint8 *) &(addr->sin6_addr), AF_INET6);
      GSocketAddress *sockaddr;

      sockaddr = g_inet_socket_address_new (iaddr, g_ntohs (addr->sin6_port));
      g_object_unref (iaddr);
      return sockaddr;
    }

#ifdef G_OS_UNIX
  if (family == AF_UNIX)
    {
      struct sockaddr_un *addr = (struct sockaddr_un *) native;

      return g_unix_socket_address_new (addr->sun_path);
    }
#endif

  return NULL;
}

#define __G_SOCKET_ADDRESS_C__
#include "gioaliasdef.c"