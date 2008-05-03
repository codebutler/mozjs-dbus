/* -*- Mode: C++; tab-aWidth: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:sw=4 sts=4 et
 *
 * MozJSDBusCoreComponent.h:
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

#ifndef _MOZJSDBUSCORECOMPONENT_H_
#define _MOZJSDBUSCORECOMPONENT_H_

#include "nsCOMPtr.h"
#include "nsIVariant.h"

#include "IMozJSDBusCoreComponent.h"

#include <dbus/dbus.h>

#include <map>
#include <string>
using namespace std;

#define MY_COMPONENT_CONTRACTID \
    "@extremeboredom.net/mozjs_dbus/MozJSDBusCoreComponent;1"
#define MY_COMPONENT_CLASSNAME "A Simple XPCOM Sample"
#define MY_COMPONENT_CID                             \
{ /* ae7d8adc-e081-4c6b-8b25-e233ba4872b5*/          \
    0xae7d8adc,                                      \
    0xe081,                                          \
    0x4c6b,                                          \
    {0x8b, 0x25, 0xe2, 0x33, 0xba, 0x48, 0x72, 0xb5} \
}

/* Utility macro to wrap calls to libdbus and check for OOM conditions */
#define MOZJSDBUS_CALL_OOMCHECK(_exp)      \
    PR_BEGIN_MACRO                         \
        if (!(_exp)) {                     \
            return NS_ERROR_OUT_OF_MEMORY; \
        }                                  \
    PR_END_MACRO

typedef struct
{
    const char  *match_rule;
    const char  *key;
    IJSCallback *callback;
} SignalCallbackInfo;

class MozJSDBusCoreComponent : public IMozJSDBusCoreComponent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_IMOZJSDBUSCORECOMPONENT
      
    MozJSDBusCoreComponent();

    nsresult Init();

    std::map<string, std::map<int, SignalCallbackInfo*> > signalCallbacks;
    
private:
    ~MozJSDBusCoreComponent();

    DBusConnection *mSystemBusConnection;
    DBusConnection *mSessionBusConnection;

    DBusConnection* GetConnection(const PRUint16 aBusType);
};

#endif //_MOZJSDBUSCORECOMPONENT_H_
