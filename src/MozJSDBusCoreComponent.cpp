/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* vim:sw=4 sts=4 et 
 *
 * MozJSDBusCoreComponent.cpp:
 *
 * Authors:
 *   Eric Butler <eric@codebutler.com>
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

#include "nsStringAPI.h"

static nsresult CheckDBusError(DBusError& aError);

static void reply_handler_func(DBusPendingCall *call, void *user_data);
static void free_callback(void *user_data);
static DBusHandlerResult filter_func(DBusConnection* connection,
                                     DBusMessage*    message,
                                     void*           user_data);
static DBusHandlerResult message_handler(DBusConnection  *connection,
                                         DBusMessage     *message,
                                         void            *user_data);

/* IMozJSDBusCoreComponent Implementation */

NS_IMPL_ISUPPORTS1(MozJSDBusCoreComponent, IMozJSDBusCoreComponent)

MozJSDBusCoreComponent::MozJSDBusCoreComponent()
{
    // Constructor
}

MozJSDBusCoreComponent::~MozJSDBusCoreComponent()
{
    // Destructor
}

nsresult
MozJSDBusCoreComponent::Init()
{
    DBusError dbus_error;
    nsresult  rv;

    dbus_error_init(&dbus_error);

    mSystemBusConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);
    rv = CheckDBusError(dbus_error);
    NS_ENSURE_SUCCESS(rv,rv);

    mSessionBusConnection = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
    rv = CheckDBusError(dbus_error);
    NS_ENSURE_SUCCESS(rv,rv);

    dbus_connection_setup_with_g_main(mSystemBusConnection, NULL);
    dbus_connection_setup_with_g_main(mSessionBusConnection, NULL);

    MOZJSDBUS_CALL_OOMCHECK(dbus_connection_add_filter(mSystemBusConnection, 
                                                       filter_func, 
                                                       this, NULL));
    MOZJSDBUS_CALL_OOMCHECK(dbus_connection_add_filter(mSessionBusConnection, 
                                                       filter_func, 
                                                       this, NULL));
    return NS_OK;
}

NS_IMETHODIMP 
MozJSDBusCoreComponent::CallMethod(const PRUint16    aBusType,
                                   const nsACString& aServiceName,
                                   const nsACString& aObjectPath,
                                   const nsACString& aInterface,
                                   const nsACString& aMethodName,
                                   const PRUint32    aArgsLength,
                                   nsIVariant**      aArgs,
                                   IJSCallback*      aCallback,
                                   nsIVariant**      _retval)
{
    DBusConnection          *connection;
    DBusMessage             *message;
    DBusMessageIter         iter;
    DBusMessage             *reply; 
    DBusError               dbus_error;
    nsCOMPtr<nsIVariant>    variant;
    PRUint32                length;
    nsresult                rv;

    dbus_error_init(&dbus_error);

    connection = GetConnection(aBusType);

    message = dbus_message_new_method_call(PromiseFlatCString(
                                               aServiceName).get(),
                                           PromiseFlatCString(
                                               aObjectPath).get(),
                                           PromiseFlatCString(
                                               aInterface).get(),
                                           PromiseFlatCString(
                                               aMethodName).get());
    if (message == NULL) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    dbus_message_iter_init_append(message, &iter);

    MozJSDBusMarshalling::appendArgs(message, &iter, aArgsLength, aArgs);

    if (aCallback == NULL) {
    
        reply = dbus_connection_send_with_reply_and_block(connection, message,
                                                          -1, &dbus_error);
        dbus_message_unref(message);

        rv = CheckDBusError(dbus_error);
        NS_ENSURE_SUCCESS(rv, rv);

        dbus_message_iter_init(reply, &iter);

        variant = MozJSDBusMarshalling::getVariantArray(&iter, &length)[0];
        
        if (variant) {
            NS_ADDREF(*_retval = variant);
            return NS_OK;
        } else {
            return NS_ERROR_FAILURE;
        }
    } else {
        DBusPendingCall *pending = NULL;
        
        int timeout = -1;
        if (dbus_connection_send_with_reply(connection, message, &pending, 
                                            timeout))
        {
            dbus_pending_call_set_notify(pending, reply_handler_func,
                                         aCallback, free_callback);
            NS_ADDREF(aCallback);
            return NS_OK;
        } else {
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
}

NS_IMETHODIMP
MozJSDBusCoreComponent::ConnectToSignal(const PRUint16    aBusType,
                                        const nsACString& aServiceName,
                                        const nsACString& aObjectPath,
                                        const nsACString& aInterface,
                                        const nsACString& aSignalName,
                                        IJSCallback*      callback,
                                        PRUint32*         result)
{
    string              matchRule;
    string              key;
    DBusError           dbus_error;
    DBusConnection      *connection;
    SignalCallbackInfo  *info;
    int                 id;
    nsresult            rv;

    dbus_error_init(&dbus_error);

    connection = GetConnection(aBusType);

    matchRule = "type='signal',interface='";
    matchRule += PromiseFlatCString(aInterface).get();
    matchRule += "',member='";
    matchRule += PromiseFlatCString(aSignalName).get();
    matchRule += "',path='";
    matchRule += PromiseFlatCString(aObjectPath).get();
    matchRule += "'"; 
    cout << "ADD MATCH: " << matchRule << "\n";
    dbus_bus_add_match(connection, matchRule.c_str(), &dbus_error);

    rv = CheckDBusError(dbus_error);
    NS_ENSURE_SUCCESS(rv,rv);

    key = PromiseFlatCString(aObjectPath).get();
    key += "/";
    key += PromiseFlatCString(aInterface).get();
    key += "/";
    key += PromiseFlatCString(aSignalName).get(); 

    info = (SignalCallbackInfo*) malloc(sizeof(SignalCallbackInfo));
    info->match_rule = strdup(matchRule.c_str());
    info->key        = strdup(key.c_str());
    info->callback   = callback;

    NS_ADDREF(callback);

    // XXX: Check id for uniqueness, just to be sure.
    id = rand();
    signalCallbacks[key][id] = info;

    *result = id;

    return NS_OK;
}

NS_IMETHODIMP 
MozJSDBusCoreComponent::DisconnectFromSignal(const PRUint16 aBusType,
                                             const PRUint32 aId)
{
    DBusConnection     *connection;
    DBusError          dbus_error;
    SignalCallbackInfo *info;
    string             key;
    nsresult           rv;
    map<string, map<int, SignalCallbackInfo*> >::iterator iter; 
    map<int, SignalCallbackInfo*>::iterator callbackIter; 

    dbus_error_init(&dbus_error);

    connection = GetConnection(aBusType);

    // This is just lovely...
    for (iter = signalCallbacks.begin(); iter != signalCallbacks.end(); iter++) {
        map<int, SignalCallbackInfo*> callbacks = iter->second;
        for (callbackIter = callbacks.begin(); callbackIter != callbacks.end(); callbackIter++) {
            if (callbackIter->first == aId) {
                info = callbackIter->second;
                break;
            }
        }
    }

    dbus_bus_remove_match(connection, info->match_rule, &dbus_error);
    rv = CheckDBusError(dbus_error);
    NS_ENSURE_SUCCESS(rv, rv);

    key = info->key;
    iter = signalCallbacks.find(key);
    if (iter != signalCallbacks.end()) {
        signalCallbacks.erase(iter);
    }

    free(info);

    return NS_OK;
}

NS_IMETHODIMP 
MozJSDBusCoreComponent::RequestService(const PRUint16    aBusType,
                                       const nsACString& aServiceName,
                                       PRBool*           _retval)
{
    DBusConnection  *connection;
    DBusError       dbus_error;
    nsresult        rv;

    dbus_error_init(&dbus_error);

    connection = GetConnection(aBusType);

    dbus_bus_request_name(connection,
                          PromiseFlatCString(aServiceName).get(),
                          0, &dbus_error);

    rv = CheckDBusError(dbus_error);
    NS_ENSURE_SUCCESS(rv,rv);

    *_retval = PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP 
MozJSDBusCoreComponent::EmitSignal(const PRUint16    aBusType,
                                   const nsACString& aObjectPath,
                                   const nsACString& aInterface,
                                   const nsACString& aSignalName,
                                   const PRUint32    aArgsLength,
                                   nsIVariant**      aArgs)
{
    DBusConnection  *connection;
    DBusMessage     *message;
    DBusMessageIter iter;

    connection = GetConnection(aBusType);

    message = dbus_message_new_signal(PromiseFlatCString(aObjectPath).get(),
                                      PromiseFlatCString(aInterface).get(),
                                      PromiseFlatCString(aSignalName).get());
    if (message == NULL) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (aArgs != NULL && aArgsLength > 0) {
        dbus_message_iter_init_append(message, &iter);
        MozJSDBusMarshalling::appendArgs(message, &iter, aArgsLength, aArgs);
    }

    dbus_connection_send(connection, message, NULL);
    
    dbus_message_unref(message);
    
    return NS_OK;
}

NS_IMETHODIMP 
MozJSDBusCoreComponent::RegisterObject(const PRUint16    aBusType,
                                       const nsACString& aObjectPath,
                                       IJSCallback*      aCallback)
{
    DBusObjectPathVTable vtable = { NULL, &message_handler };

    DBusConnection* connection = GetConnection(aBusType);

    MOZJSDBUS_CALL_OOMCHECK(
        dbus_connection_register_object_path(connection, 
                                             PromiseFlatCString(
                                                 aObjectPath).get(),
                                             &vtable, 
                                             aCallback));
    
    NS_ADDREF(aCallback);

    return NS_OK;
}

NS_IMETHODIMP 
MozJSDBusCoreComponent::UnregisterObject(const PRUint16    aBusType,
                                         const nsACString& aObjectPath)
{
    DBusConnection* connection = GetConnection(aBusType);

    MOZJSDBUS_CALL_OOMCHECK(
        dbus_connection_unregister_object_path(connection,
                                               PromiseFlatCString(
                                                   aObjectPath).get()));

    return NS_OK;
}

DBusConnection*
MozJSDBusCoreComponent::GetConnection(const PRUint16 aBusType)
{
  DBusConnection* connection = NULL;

  switch (aBusType) {
      case IMozJSDBusCoreComponent::SYSTEM_BUS:
      {
          connection = mSystemBusConnection;
          break;
      }
      case IMozJSDBusCoreComponent::SESSION_BUS:
      {
          connection = mSessionBusConnection;
          break;
      }
      default:
          printf("Bad bus type: %d\n", aBusType);
  }

  return connection;
}

/* Utility function */

nsresult 
CheckDBusError(DBusError& aError)
{
    if (dbus_error_is_set(&aError)) {
        printf("** DBUS ERROR **\n   Name: %s\n   Message: %s\n",
               aError.name, aError.message);

        dbus_error_free(&aError);
        return NS_ERROR_FAILURE;
    } else {
        return NS_OK;
    }
}

/* ===== Callbacks, handlers etc. ===== */

void 
reply_handler_func(DBusPendingCall *call, void *user_data)
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

void 
free_callback(void *user_data)
{
    IJSCallback* cb = reinterpret_cast<IJSCallback*>(user_data);
    NS_RELEASE(cb);
}

DBusHandlerResult
filter_func(DBusConnection* connection, DBusMessage* message, void* user_data)
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
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

DBusHandlerResult 
message_handler(DBusConnection* connection,
                DBusMessage* message,
                void* user_data)
{
    const char              *interface;
    const char              *member;
    DBusMessageIter         iter;
    nsIVariant              *result;
    nsCOMPtr<IJSCallback>   js_callback;
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
