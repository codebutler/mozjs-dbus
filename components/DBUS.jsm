/*
 * DBUS.jsm: 
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

EXPORTED_SYMBOLS = ["DBUS"];

Components.utils.import("resource://app/components/DBusXMLParser.jsm");
Components.utils.import("resource://app/components/ArrayConverter.jsm");

var DBUS = {
	getSystemBus: function() {
		return new DBusConnection("system");
	},

	getSessionBus: function() {
		return new DBusConnection("session");
	}
};

var klass = Components.classes["@extremeboredom.net/mozjs_dbus/MozJSDBusCoreComponent;1"];
DBUS.core = klass.createInstance(Components.interfaces.IMozJSDBusCoreComponent);

function DBusConnection(busName) {
	if (["session", "system"].indexOf(busName) == -1) {
		throw "Invalid bus name";
	}
	this.busName = busName;
}

DBusConnection.prototype.getObject = function(serviceName, objectPath, iface) {
	
	if (typeof(iface) == 'string') {
		var interfaceName = iface;

		// Get the introspection xml
		// XXX: We need to first check if this object supports introspection!!
		var xml = DBUS.core.CallMethod(this.busName,
					       serviceName,
					       objectPath,
					       "org.freedesktop.DBus.Introspectable",
					       "Introspect",
					       0,
					       []);

		// DOCTYPE isnt supported by E4X
		xml = xml.replace(/<!DOCTYPE[^>]+>/, '');

		dump(xml);

		xml = new XML(xml);

		// Parse the xml
		var objectInterface = DBusXMLParser.parse(xml);
	} else {
		var interfaceName = iface.name;

		var objectInterface = new DBusInterface();
		objectInterface.methods = iface.methods.map(function(methodName) {
			return new DBusMethod(methodName);
		});
	}

	// Create proxy object
	var proxy = {};
	proxy.connection = this;
	proxy.serviceName = serviceName;
	proxy.objectPath = objectPath;
	proxy.interfaceName = interfaceName;

	// Create wrapper function for low-level dbus call
	proxy.callMethod = function (methodName, methodArgs) {
		if (methodArgs == null) {
			methodArgs = [];
		}

		var result = DBUS.core.CallMethod(this.connection.busName,
				                  this.serviceName,
				                  this.objectPath,
				                  this.interfaceName,
				                  methodName,
		                                  methodArgs.length,
				                  methodArgs);

		return result;
	};

	// Create proxy functions for dbus methods
	objectInterface.methods.forEach(function (method) {
		var proxyFunction = function () {
			var methodName = arguments.callee.methodName;
			var argArray = Array.prototype.slice.call(arguments);
			return this.callMethod(methodName, argArray);
		};
		proxyFunction.methodName = method.name;
		proxy[method.name] = proxyFunction;
	});

	return proxy;
};

