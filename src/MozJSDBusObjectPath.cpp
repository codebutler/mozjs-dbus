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
 
#include <stdlib.h>
#include <iostream>
#include <string>
#include <stdio.h>
using namespace std;

#include "IMozJSDBusCoreComponent.h"
#include "MozJSDBusObjectPath.h"

#include "nsStringAPI.h"

NS_IMPL_ISUPPORTS1(MozJSDBusObjectPath, IMozJSDBusObjectPath)

MozJSDBusObjectPath::MozJSDBusObjectPath()
{
  /* member initializers and constructor code */
}

MozJSDBusObjectPath::~MozJSDBusObjectPath()
{
  /* destructor code */
}

nsresult
MozJSDBusObjectPath::Init()
{
    return NS_OK;
}

/* attribute AUTF8String path; */
NS_IMETHODIMP MozJSDBusObjectPath::GetPath(nsACString & aPath)
{
    aPath.Assign(mPath);
    return NS_OK;
}

NS_IMETHODIMP MozJSDBusObjectPath::SetPath(const nsACString & aPath)
{
    mPath = aPath;
    return NS_OK;
}
