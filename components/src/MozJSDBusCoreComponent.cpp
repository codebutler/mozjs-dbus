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

#include <stdio.h>
#include "MozJSDBusCoreComponent.h"

#include "../../../toolkit/system/dbus/nsDBusService.h"

// Kitchen sink on the way
#include "nsIArray.h"
#include "nsTArray.h"
#include "nsStringAPI.h"
#include "nsEmbedString.h"
#include "nsIPropertyBag.h"
#include "nsIWritablePropertyBag.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsXPCOMCID.h"
#include "nsISupportsImpl.h"
#include "nsISupportsPrimitives.h"
#include "nsIVariant.h"
#include "nsIMutableArray.h"

NS_IMPL_ISUPPORTS1(MozJSDBusCoreComponent, IMozJSDBusCoreComponent)

MozJSDBusCoreComponent::MozJSDBusCoreComponent()
{
	// Constructor?
}

MozJSDBusCoreComponent::~MozJSDBusCoreComponent()
{
	// Destructor?
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

	DBusError error;
	dbus_error_init(&error);

	DBusConnection *connection;

	// XXX: Use a switch and an enum here instead of this nonsense!
	if (busName == "system") {
		connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	} else if (busName == "session") {
		connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
	} else {
		printf("bad bus name");
		return NS_ERROR_FAILURE;
	}

	// XXX: Do error checking here

	DBusMessage *message;
	DBusMessageIter iter;

	// XXX: Is there a macro that could clean all this up?

	const char* cServiceName;
	NS_CStringGetData(serviceName, &cServiceName);

	const char* cObjectPath;
	NS_CStringGetData(objectPath, &cObjectPath);

	const char* cInterface;
	NS_CStringGetData(interface, &cInterface);

	const char* cMethodName;
	NS_CStringGetData(methodName, &cMethodName);

	message = dbus_message_new_method_call(cServiceName,
					       cObjectPath,
					       cInterface,
					       cMethodName);

	dbus_message_iter_init_append(message, &iter);

	for (PRUint32 x = 0; x < argsLength; x++) {
		nsIVariant *arg = args[x];
		
		PRUint16 dataType;
		rv = arg->GetDataType(&dataType);
		NS_ENSURE_SUCCESS(rv, rv);

		printf ("Data type is %d \n", dataType);
	
		// Append each arg!
	 	// dbus_message_iter_append_basic(&iter, arg->GetAsF0obar);
	}

	DBusMessage *reply; 

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &error);

	if (reply == NULL) {
		if (dbus_error_is_set(&error)) {
			printf("Error sending DBUS message: %s\n", error.message);
			dbus_error_free(&error);
			return NS_ERROR_FAILURE;
		}
	}
	
	nsCOMPtr<nsIWritableVariant> variant =
		do_CreateInstance("@mozilla.org/variant;1", &rv);
	if (NS_FAILED(rv)) {
		PR_LOG(lm, PR_LOG_DEBUG, ("do Create Instance failed"));
		return NS_ERROR_FAILURE;
	}

	dbus_message_iter_init(reply, &iter);
	int current_type;
	while ((current_type = dbus_message_iter_get_arg_type (&iter)) != DBUS_TYPE_INVALID) {
		printf ("type in response %d \n ", current_type);
		if (current_type == DBUS_TYPE_STRING) {
			char* value;
			dbus_message_iter_get_basic(&iter, &value);

			variant->SetAsString(value);
		}

		break;
   		//dbus_message_iter_next (&iter);
	}

	NS_ADDREF(*_retval = variant);
	return NS_OK;
}

