/* 
 * MozJSDBusObjectPath.cpp:
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
 
#include "nsCOMPtr.h"
#include "nsIVariant.h"
#include "nsStringAPI.h"

#include "IMozJSDBusCoreComponent.h"

#define MOZJS_DBUS_OBJECT_PATH_CONTRACTID "@codebutler.com/MozJSDBus/MozJSDBusObjectPath;1"
#define MOZJS_DBUS_OBJECT_PATH_CLASSNAME "DBus Object Path"
#define MOZJS_DBUS_OBJECT_PATH_CID                             \
{ /* d96b36b0-54b3-012d-e500-64b9e8be7216 */         \
    0xd96b36b0,                                      \
    0xe54b3,                                         \
    0x012d,                                          \
    {0xe5, 0x00, 0x64, 0xb9, 0xe8, 0xbe, 0x72, 0x16} \
}

class MozJSDBusObjectPath : public IMozJSDBusObjectPath
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMOZJSDBUSOBJECTPATH

  MozJSDBusObjectPath();
  
  nsresult Init();

private:
  ~MozJSDBusObjectPath();

protected:
  nsCString mPath;
};
