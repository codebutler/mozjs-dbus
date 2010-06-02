/*
 * MozJSDBusCoreComponentModule.cpp:
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

#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "MozJSDBusCoreComponent.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(MozJSDBusCoreComponent, Init)

static const nsModuleComponentInfo components[] =
{
    {
       MY_COMPONENT_CLASSNAME, 
       MY_COMPONENT_CID,
       MY_COMPONENT_CONTRACTID,
       MozJSDBusCoreComponentConstructor,
    }
};

NS_IMPL_NSGETMODULE(MozJSDBusCoreComponentModule, components) 

