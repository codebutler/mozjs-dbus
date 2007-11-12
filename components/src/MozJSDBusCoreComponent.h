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

#ifndef _MY_COMPONENT_H_
#define _MY_COMPONENT_H_

#include "nsCOMPtr.h"
#include "nsIVariant.h"

#include "IMozJSDBusCoreComponent.h"

#include <dbus/dbus.h>

#include <map>
#include <string>
using namespace std;

#define MY_COMPONENT_CONTRACTID "@extremeboredom.net/mozjs_dbus/MozJSDBusCoreComponent;1"
#define MY_COMPONENT_CLASSNAME "A Simple XPCOM Sample"
#define MY_COMPONENT_CID  { 0x597a60b0, 0x5272, 0x4284, { 0x90, 0xf6, 0xe9, 0x6c, 0x24, 0x2d, 0x74, 0x6 } }

static bool checkDBusError(DBusError error);

typedef struct
{
    const char *serviceName;
    const char *objectPath;
    const char *interface;
    const char *signalName;
    IJSCallback *callback;
} SignalCallbackInfo;

class MozJSDBusCoreComponent : public IMozJSDBusCoreComponent
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_IMOZJSDBUSCORECOMPONENT

    MozJSDBusCoreComponent();
    virtual ~MozJSDBusCoreComponent();

    // XXX: This should be a hashtable!
    std::map<string, SignalCallbackInfo*> signalCallbacks;
    
private:
    DBusConnection *systemConnection;
    DBusConnection *sessionConnection;

    DBusConnection* GetConnection(char* busName);
};

#endif //_MY_COMPONENT_H_
