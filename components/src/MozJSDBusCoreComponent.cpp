/*
 * MozJSDBusCoreComponent.cpp:
 *
 * Authors:
 *   Eric Butler <eric@extremeboredom.net>
 *
 *  This file is part of mozjs-dbus.
 *
 *  mozjs-dbus is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  mozjs-dbus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with mozjs-dbus.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "MozJSDBusCoreComponent.h"
#include "nsIPropertyBag.h"
#include "nsIWritablePropertyBag.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsIArray.h"
#include "nsTArray.h"
#include "nsXPCOMCID.h"
#include "nsISupportsImpl.h"
#include "nsISupportsPrimitives.h"
#include "nsIVariant.h"
#include "nsEmbedString.h"
#include "nsIMutableArray.h"
#include <stdio.h>

NS_IMPL_ISUPPORTS1(MozJSDBusCoreComponent, IMozJSDBusCoreComponent)

MozJSDBusCoreComponent::MozJSDBusCoreComponent()
{
  /* member initializers and constructor code */
}

MozJSDBusCoreComponent::~MozJSDBusCoreComponent()
{
  /* destructor code */
}

NS_IMETHODIMP MozJSDBusCoreComponent::Introspect(const nsACString &busName,
                                      const nsACString &serviceName,
				      const nsACString &objectPath,
				      nsACString &result)
{
	result.Assign("<node name=\"/org/freedesktop/sample_object\">           <interface name=\"org.freedesktop.SampleInterface\">             <method name=\"Frobate\">               <arg name=\"foo\" type=\"i\" direction=\"in\"/>               <arg name=\"bar\" type=\"s\" direction=\"out\"/>               <arg name=\"baz\" type=\"a{us}\" direction=\"out\"/>               <annotation name=\"org.freedesktop.DBus.Deprecated\" value=\"true\"/>             </method>             <method name=\"Bazify\">               <arg name=\"bar\" type=\"(iiu)\" direction=\"in\"/>               <arg name=\"bar\" type=\"v\" direction=\"out\"/>             </method>             <method name=\"Mogrify\">               <arg name=\"bar\" type=\"(iiav)\" direction=\"in\"/>             </method>             <signal name=\"Changed\">               <arg name=\"new_value\" type=\"b\"/>             </signal>             <property name=\"Bar\" type=\"y\" access=\"readwrite\"/>           </interface>           <node name=\"child_of_sample_object\"/>           <node name=\"another_child_of_sample_object\"/>        </node>        ");

	return NS_OK;
}

NS_IMETHODIMP MozJSDBusCoreComponent::CallMethod(const nsACString &busName,
                                      const nsACString &serviceName,
				      const nsACString &objectPath,
				      const nsACString &interface,
				      const nsACString &methodName,
				      const PRUint32 argsLength,
				      nsIVariant **args,
				      nsIVariant **_retval)
{
	nsresult rv;
/*
	DBusMessage *message;
	DBusMessageIter iter;

	message = dbus_message_new_method_call(serviceName,
					       objectPath,
					       interface,
					       methodName);

	dbus_message_iter_init_append(message, &iter);
 */

	for (PRUint32 x = 0; x < argsLength; x++) {
		nsIVariant *arg = args[x];
		
		PRUint16 dataType;
		rv = arg->GetDataType(&dataType);
		NS_ENSURE_SUCCESS(rv, rv);

		printf ("Data type is %d \n", dataType);
	
		// Append each arg!
	 	// dbus_message_iter_append_basic(&iter, arg->GetAsF0obar);
	}

	// dbus_connection_send(dbus_conn, message, NULL);
	// dbus_message_unref(message);

	nsCOMPtr<nsIWritableVariant> variant =
		do_CreateInstance("@mozilla.org/variant;1", &rv);

	if (NS_FAILED(rv)) {
		PR_LOG(lm, PR_LOG_DEBUG, ("do Create Instance failed"));
		return NS_ERROR_FAILURE;
	}

	variant->SetAsString("happydance");

	NS_ADDREF(*_retval = variant);
	return NS_OK;
}


