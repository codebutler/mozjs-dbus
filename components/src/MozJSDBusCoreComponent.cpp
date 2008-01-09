/* vim:sw=4 sts=4 et 
 *
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

#include "nsStringAPI.h"

NS_IMPL_ISUPPORTS1(MozJSDBusCoreComponent, IMozJSDBusCoreComponent)

static void reply_handler_func(DBusPendingCall *call, void *user_data)
{
    DBusMessage             *message;
    DBusMessageIter         iter;
    nsCOMPtr<IJSCallback>   js_callback;
    PRUint32                length;
    nsIVariant**            args;
    nsIVariant              *result;
    const char*             interface;
    const char*             name;
    
    js_callback = (IJSCallback*)user_data;
    
    message = dbus_pending_call_steal_reply(call);
    
    if (message != NULL) {
        // XXX: These might always be null?
        interface = dbus_message_get_interface(message);
        name      = dbus_message_get_member(message);
        
        dbus_message_iter_init(message, &iter);
        
        args = MozJSDBusMarshalling::getVariantArray(&iter, &length);

        js_callback->Method(interface, name, args, length, &result);
    } else {
        // Timeout?
        js_callback->Method(NULL, NULL, NULL, NULL, &result);
    }
}

static void free_callback(void *user_data) {
    // XXX: Uhh.. what' the opposite of NS_ADDREF?
}

static DBusHandlerResult
filter_func(DBusConnection* connection,
            DBusMessage*    message,
            void*           user_data)
{
    const char             *interface;
    const char             *name;
    const char             *path;
    string                 key;
    MozJSDBusCoreComponent *core;
    DBusMessageIter        iter;
    nsIVariant**           args;
    PRUint32               length;
    SignalCallbackInfo     *info;
    map<int, SignalCallbackInfo*> handlers;
    map<int, SignalCallbackInfo*>::iterator handlerIter;

    if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_SIGNAL) {
        interface = dbus_message_get_interface(message);
        name = dbus_message_get_member(message);
        path = dbus_message_get_path(message);
        
        key = path;
        key += "/";
        key += interface;
        key += "/";
        key += name;

        core = (MozJSDBusCoreComponent*)user_data;

        if (core->signalCallbacks.find(key) != core->signalCallbacks.end()) {

            handlers = core->signalCallbacks.find(key)->second;

            for (handlerIter = handlers.begin(); handlerIter != handlers.end(); handlerIter++) {
                
                info = handlerIter->second;
 
                dbus_message_iter_init(message, &iter);
        
                args = MozJSDBusMarshalling::getVariantArray(&iter, &length);
               
                nsIVariant *result;
                info->callback->Method(interface, name, args, length, &result);
            }
        } /* else {
            cout << "DID NOT FIND KEY: " << key << " !!\n";
        } */
    }
}

MozJSDBusCoreComponent::MozJSDBusCoreComponent()
{
    DBusError error;
    dbus_error_init(&error);

    systemConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    checkDBusError(error);

    sessionConnection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    checkDBusError(error);

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
                                                 const PRUint32   argsLength,
                                                 nsIVariant       **args,
                                                 IJSCallback      *callback,
                                                 nsIVariant       **_retval)
{
    const char*             cBusName;
    DBusConnection          *connection;
    DBusMessage             *message;
    DBusMessageIter         iter;
    const char*             cServiceName;
    const char*             cObjectPath;
    const char*             cInterface;
    const char*             cMethodName;
    DBusMessage             *reply; 
    DBusError               error;
    nsCOMPtr<nsIVariant>    variant;
    PRUint32                length;

    dbus_error_init(&error);

    NS_CStringGetData(busName, &cBusName);
    connection = GetConnection((char*)cBusName);

    NS_CStringGetData(serviceName, &cServiceName);
    NS_CStringGetData(objectPath,  &cObjectPath);
    NS_CStringGetData(interface,   &cInterface);
    NS_CStringGetData(methodName,  &cMethodName);

    message = dbus_message_new_method_call(cServiceName,
                                           cObjectPath,
                                           cInterface,
                                           cMethodName);
    if (message == NULL) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    dbus_message_iter_init_append(message, &iter);

    MozJSDBusMarshalling::appendArgs(message, &iter, argsLength, args);

    if (callback == NULL) {
    
        reply = dbus_connection_send_with_reply_and_block(connection, message, -1, 
                                                          &error);
        dbus_message_unref(message);
        
        if (checkDBusError(error) == FALSE) {

            dbus_message_iter_init(reply, &iter);

            variant = MozJSDBusMarshalling::getVariantArray(&iter, &length)[0];
        
            if (variant) {
                NS_ADDREF(*_retval = variant);
                return NS_OK;
            } else {
                return NS_ERROR_FAILURE;
            }
        } else {
            return NS_ERROR_FAILURE;
        }
         
    } else {
        DBusPendingCall *pending = NULL;
        
        int timeout = -1;
        if (dbus_connection_send_with_reply(connection, message, &pending, timeout)) {
            dbus_pending_call_set_notify(pending, reply_handler_func, callback, free_callback);
            NS_ADDREF(callback);
            return NS_OK;
        } else {
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
}

NS_IMETHODIMP MozJSDBusCoreComponent::ConnectToSignal(const nsACString &busName,
                                                      const nsACString &serviceName,
                                                      const nsACString &objectPath,
                                                      const nsACString &interface,
                                                      const nsACString &signalName,
                                                      IJSCallback      *callback,
                                                      PRUint32         *result)
{
    const char          *cServiceName;
    const char          *cObjectPath;
    const char          *cInterface;
    const char          *cSignalName;
    const char          *cBusName;
    string              matchRule;
    string              key;
    DBusError           error;
    DBusConnection      *connection;
    SignalCallbackInfo  *info;
    int                 id;
    
    dbus_error_init(&error);

    NS_CStringGetData(serviceName, &cServiceName);
    NS_CStringGetData(objectPath, &cObjectPath);
    NS_CStringGetData(interface, &cInterface);
    NS_CStringGetData(signalName, &cSignalName);
    NS_CStringGetData(busName, &cBusName);

    connection = GetConnection((char*)cBusName);

    matchRule = "type='signal',interface='";
    matchRule += cInterface;
    matchRule += "',member='";
    matchRule += cSignalName;
    matchRule += "',path='";
    matchRule += cObjectPath;
    matchRule += "'";
    cout << "ADD MATCH: " << matchRule << "\n";
    dbus_bus_add_match(connection, matchRule.c_str(), &error);

    checkDBusError(error);

    key = cObjectPath;
    key += "/";
    key += cInterface;
    key += "/";
    key += cSignalName;

    info = (SignalCallbackInfo*) malloc(sizeof(SignalCallbackInfo));
    info->match_rule = strdup(matchRule.c_str());
    info->key        = strdup(key.c_str());
    info->callback   = callback;

    NS_ADDREF(callback);
    // XXX: Check id for uniqueness, just to be sure.
    id = rand();
    signalCallbacks[key][id] = info;

    (*result) = id;
}

NS_IMETHODIMP MozJSDBusCoreComponent::DisconnectFromSignal(const nsACString &busName,
                                                           const PRUint32   id)
{
    DBusConnection     *connection;
    const char         *cBusName;
    DBusError          dbus_error;
    SignalCallbackInfo *info;
    string             key;
    map<string, map<int, SignalCallbackInfo*> >::iterator iter; 
    map<int, SignalCallbackInfo*>::iterator callbackIter; 

    dbus_error_init(&dbus_error);

    NS_CStringGetData(busName, &cBusName);
    connection = GetConnection((char*)cBusName);

    // This is just lovely...
    for (iter = signalCallbacks.begin(); iter != signalCallbacks.end(); iter++) {
        map<int, SignalCallbackInfo*> callbacks = iter->second;
        for (callbackIter = callbacks.begin(); callbackIter != callbacks.end(); callbackIter++) {
            if (callbackIter->first == id) {
                info = callbackIter->second;
                break;
            }
        }
    }

    dbus_bus_remove_match(connection, info->match_rule, &dbus_error);

    key = info->key;
    iter = signalCallbacks.find(key);
    if (iter != signalCallbacks.end()) {
        signalCallbacks.erase(iter);
    }

    free(info);

    return NS_OK;
}

NS_IMETHODIMP MozJSDBusCoreComponent::RequestService(const nsACString &busName,
                                                     const nsACString &serviceName,
                                                     PRBool *_retval)
{
    DBusConnection  *connection;
    const char      *cBusName;
    const char      *cServiceName;
    DBusError       dbus_error;

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
    const char              *interface;
    const char              *member;
    DBusMessageIter         iter;
    nsIVariant              *result;
    nsCOMPtr<IJSCallback>   js_callback;
    nsresult                rv;
    nsIVariant**            args;
    PRUint32                length;
    DBusMessage             *reply;

    member      = dbus_message_get_member(message);
    interface   = dbus_message_get_interface(message);
    js_callback = (IJSCallback*)user_data;

    dbus_message_iter_init(message, &iter);
    
    args = MozJSDBusMarshalling::getVariantArray(&iter, &length);

    js_callback->Method(interface, member, args, length, &result);

    reply = dbus_message_new_method_return(message);
    if (reply == NULL) {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    if (result != NULL) {
        dbus_message_iter_init_append(reply, &iter);
        MozJSDBusMarshalling::marshallVariant(reply, result, &iter);
    }

    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
}

NS_IMETHODIMP MozJSDBusCoreComponent::EmitSignal(const nsACString &busName,
                                                 const nsACString &objectPath,
                                                 const nsACString &interface,
                                                 const nsACString &signalName,
                                                 const PRUint32 argsLength,
                                                 nsIVariant **args)
{
    DBusConnection  *connection;
    const char      *cBusName;
    const char      *cInterface;
    const char      *cSignalName;
    const char      *cObjectPath;
    DBusMessage     *message;
    DBusMessageIter iter;

    NS_CStringGetData(busName, &cBusName);
    connection = GetConnection((char*)cBusName);

    NS_CStringGetData(objectPath, &cObjectPath);
    NS_CStringGetData(interface,  &cInterface);
    NS_CStringGetData(signalName, &cSignalName);

    message = dbus_message_new_signal(cObjectPath, cInterface, cSignalName);
    if (message == NULL) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (args != NULL && argsLength > 0) {
        dbus_message_iter_init_append(message, &iter);
        MozJSDBusMarshalling::appendArgs(message, &iter, argsLength, args);
    }

    dbus_connection_send(connection, message, NULL);
    
    dbus_message_unref(message);
    
    return NS_OK;
}

NS_IMETHODIMP MozJSDBusCoreComponent::RegisterObject(const nsACString &busName,
                                                     const nsACString &objectPath,
                                                     IJSCallback *callback)
{
    DBusConnection          *connection;
    const char              *cBusName;
    const char              *cObjectPath;
    DBusObjectPathVTable    vtable = { NULL, &message_handler };

    NS_CStringGetData(busName, &cBusName);
    connection = GetConnection((char*)cBusName);

    NS_CStringGetData(objectPath, &cObjectPath);

    if (!dbus_connection_register_object_path(connection, cObjectPath, &vtable, 
                                              callback)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    NS_ADDREF(callback);

    return NS_OK;
}

NS_IMETHODIMP MozJSDBusCoreComponent::UnregisterObject(const nsACString &busName,
                                                       const nsACString &objectPath)
{
    DBusConnection          *connection;
    const char              *cBusName;
    const char              *cObjectPath;

    NS_CStringGetData(busName, &cBusName);
    connection = GetConnection((char*)cBusName);

    NS_CStringGetData(objectPath, &cObjectPath);

    if (!dbus_connection_unregister_object_path(connection, cObjectPath)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
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

static bool checkDBusError(DBusError error)
{
    if (dbus_error_is_set(&error)) {
        printf("** DBUS ERROR!!!\n Name: %s \n Message: %s \n",
               error.name, error.message);
        
        dbus_error_free(&error);
        
        return true;
        // XXX: Throw an XPCOM exception here or something.
    } else {
        return false;
    }
}
