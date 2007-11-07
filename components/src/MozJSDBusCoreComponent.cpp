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

#include <stdlib.h>
#include <iostream>
#include <string>
#include <stdio.h>
using namespace std;

#include "MozJSDBusCoreComponent.h"
#include "MozJSDBusMarshalling.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

//#include "../../../toolkit/system/dbus/nsDBusService.h"

// Kitchen sink on the way
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

NS_IMPL_ISUPPORTS1(MozJSDBusCoreComponent, IMozJSDBusCoreComponent)

static DBusHandlerResult
filter_func(DBusConnection* connection,
            DBusMessage* message,
	    void* user_data)
{
	if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL) {
		const char *interface = dbus_message_get_interface(message);
		const char *name = dbus_message_get_member(message);

		string key = interface;
		key += "/";
		key += name;

		MozJSDBusCoreComponent *core =
		    (MozJSDBusCoreComponent*)user_data;

		if (core->signalCallbacks.find(key) != core->signalCallbacks.end()) {

			SignalCallbackInfo *info = core->signalCallbacks[key];

			// XXX: May need to create proxy to ensure this is run on UI thread!
			nsCOMPtr<ISignalCallback> js_callback = info->callback;
			
			// XXX: Actually, this doesn't return anything
			PRBool ret = TRUE;
			js_callback->Method(42, &ret);
		}
	}
}

MozJSDBusCoreComponent::MozJSDBusCoreComponent()
{
	DBusError error;
	dbus_error_init(&error);

	systemConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    	sessionConnection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    
	// XXX: Check for errors!

	dbus_connection_setup_with_g_main(systemConnection, NULL);
	dbus_connection_setup_with_g_main(sessionConnection, NULL);

	dbus_connection_add_filter(systemConnection, filter_func, this, NULL);
	dbus_connection_add_filter(sessionConnection, filter_func, this, NULL);
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

	const char* cBusName;
	NS_CStringGetData(busName, &cBusName);
	DBusConnection *connection = GetConnection((char*)cBusName);

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

		// This unwraps a basic type from a variant created by
		// DBUS.UInt32(), etc.
		if (dataType == nsIDataType::VTYPE_INTERFACE_IS) {
			nsISupports *blah;
			arg->GetAsISupports(&blah);

			nsIVariant *subVariant;
			rv = blah->QueryInterface(NS_GET_IID(nsIVariant), (void**)&subVariant);

			if (!NS_FAILED(rv)) {
				/*
				PRUint16 subDataType;
				rv = subVariant->GetDataType(&subDataType);
				NS_ENSURE_SUCCESS(rv, rv);
				printf("Sub data type is %d\n", subDataType);
				*/
				arg = subVariant;
			}
		}
	
		rv = MozJSDBusMarshalling::marshallVariant(message, arg, &iter);
		NS_ENSURE_SUCCESS(rv,rv);
	}

	DBusMessage *reply; 

	DBusError error;
	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &error);

	if (reply == NULL) {
		if (dbus_error_is_set(&error)) {
			printf("Error sending DBUS message: %s\n", error.message);
			dbus_error_free(&error);
			return NS_ERROR_FAILURE;
		}
	}
	
	nsCOMPtr<nsIVariant> variant;

	dbus_message_iter_init(reply, &iter);

	PRUint32 length;
	variant = MozJSDBusMarshalling::getVariantArray(&iter, &length)[0];

	if (variant) {
		NS_ADDREF(*_retval = variant);
		return NS_OK;
	} else {
		return NS_ERROR_FAILURE;
	}
}

NS_IMETHODIMP MozJSDBusCoreComponent::ConnectToSignal(const nsACString &busName,
                                      const nsACString &serviceName,
				      const nsACString &objectPath,
				      const nsACString &interface,
				      const nsACString &signalName,
				      ISignalCallback *callback)
{
	const char		*cServiceName;
	const char		*cObjectPath;
	const char		*cInterface;
	const char		*cSignalName;
	const char		*cBusName;
	string			matchRule;
	string			key;
	DBusError		error;
	DBusConnection		*connection;
	SignalCallbackInfo	*info;

	dbus_error_init(&error);

	NS_CStringGetData(serviceName, &cServiceName);
	NS_CStringGetData(objectPath, &cObjectPath);
	NS_CStringGetData(interface, &cInterface);
	NS_CStringGetData(signalName, &cSignalName);

	info = (SignalCallbackInfo*) malloc(sizeof(SignalCallbackInfo));
	info->serviceName = strdup(cServiceName);
	info->objectPath  = strdup(cObjectPath);
	info->interface   = strdup(cInterface);
	info->signalName  = strdup(cSignalName);
	info->callback    = callback;
	NS_ADDREF(callback);
	
	NS_CStringGetData(busName, &cBusName);
	connection = GetConnection((char*)cBusName);

	matchRule = "type='signal',interface='";
	matchRule += cInterface;
	matchRule += "',member='";
	matchRule += cSignalName;
	matchRule += "'";
	dbus_bus_add_match(connection, matchRule.c_str(), &error);

	checkDBusError(error);

	key = cInterface;
	key += "/";
	key += cSignalName;

	signalCallbacks[key] = info;
}

NS_IMETHODIMP MozJSDBusCoreComponent::RequestService(const nsACString &busName,
                                      const nsACString &serviceName,
				      PRBool *_retval)
{
    DBusConnection  *connection;
    const char	    *cBusName;
    const char	    *cServiceName;
    DBusError	    dbus_error;

    dbus_error_init(&dbus_error);

    NS_CStringGetData(serviceName, &cServiceName);

    NS_CStringGetData(busName, &cBusName);
    connection = GetConnection((char*)cBusName);

    dbus_bus_request_name(connection, cServiceName, 0, &dbus_error);
    checkDBusError(dbus_error);

    *_retval = PR_TRUE;

    return NS_OK;
}

static DBusHandlerResult message_handler(DBusConnection  *connection,
                                         DBusMessage     *message,
					 void            *user_data)
{
    const char			*interface;
    const char			*member	    = dbus_message_get_member(message);
    DBusMessageIter		iter;
    nsIVariant			*result;
    nsCOMPtr<IMethodHandler>	js_callback;
    nsresult			rv;
    nsIVariant** args;

    interface = dbus_message_get_interface(message);

    js_callback = (IMethodHandler*)user_data;

    dbus_message_iter_init(message, &iter);
    
    PRUint32 length;
    args = MozJSDBusMarshalling::getVariantArray(&iter, &length);

    js_callback->Method(interface, member, args, length, &result);

    DBusMessage *reply = dbus_message_new_method_return(message);

    if (result != NULL) {
	dbus_message_iter_init_append(reply, &iter);
	MozJSDBusMarshalling::marshallVariant(reply, result, &iter);
    }

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
}

NS_IMETHODIMP MozJSDBusCoreComponent::RegisterObject(const nsACString &busName,
                                      const nsACString &objectPath,
				      IMethodHandler *callback)
{
    DBusConnection  *connection;
    const char	    *cBusName;
    const char	    *cObjectPath;

    NS_CStringGetData(busName, &cBusName);
    connection = GetConnection((char*)cBusName);

    NS_CStringGetData(objectPath, &cObjectPath);

    printf("Register obj %s\n", cObjectPath);

    DBusObjectPathVTable vtable = { NULL, &message_handler };
    dbus_connection_register_object_path(connection, cObjectPath, &vtable, callback);
    
    NS_ADDREF(callback);
}

DBusConnection* MozJSDBusCoreComponent::GetConnection(char* busName)
{
	DBusConnection *connection;

	// XXX: Don't use strings here
	if (strcmp(busName, "system") == 0) {
		connection = systemConnection;
	} else if (strcmp(busName, "session") == 0) {
		connection = sessionConnection;
	} else {
		printf("bad bus name: '%s'\n", busName);
		return NULL;
	}

	return connection;
}

static void checkDBusError(DBusError error)
{
    if (dbus_error_is_set(&error)) {
	printf("** DBUS ERROR!!!\n Name: %s \n Message: %s \n",
	    error.name, error.message);

	// XXX: Throw an XPCOM exception here or something.
    }
}
