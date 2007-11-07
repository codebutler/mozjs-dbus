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

EXPORTED_SYMBOLS = ["DBUS", "DBusObject"];

Components.utils.import("resource://app/modules/DBusXMLParser.jsm");
Components.utils.import("resource://app/modules/ArrayConverter.jsm");
Components.utils.import("resource://app/modules/ProxyUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function DBusObject () {
    this.interfaces = {};
    this.defineInterface = function (name) {
	var iface = {};
	iface.methods = {};
	iface.defineMethod = function (methodName, inSignature, outSignature, func) {
	    var method = {
		inSignature: inSignature,
	       outSignature: outSignature, 
		     method: func
	    };
	    this.methods[methodName] = method;
	};
	this.interfaces[name] = iface;

	return iface;
    };

    var introspectInterface = this.defineInterface('org.freedesktop.DBus.Introspectable');
    var serviceName = this.serviceName;
    var obj = this;

    introspectInterface.defineMethod('Introspect', '', 's', function () {
	var xml = <node name={serviceName}/>;

	for (var interfaceName in obj.interfaces) {
	    var interfaceXML = <interface name={interfaceName} />;

	    var methods = obj.interfaces[interfaceName].methods;

	    for (var methodName in methods) {
		var methodInfo = methods[methodName];

		var methodXML = <method name={methodName}/>;

		var appendArgs = function (signature, direction) {
			for (var x = 0; x < signature.length; x++) {
				var type = signature[x];
				var name = direction + "Arg" + x; // XXX: <-- Eh...
				var argXML = <arg direction={direction} type={type} name={name}/>;
				methodXML.appendChild(argXML);
			}
		};
		appendArgs(methodInfo['inSignature'], "in");
		appendArgs(methodInfo['outSignature'], "out");

		interfaceXML.appendChild(methodXML);
	    }
	    
	    xml.appendChild(interfaceXML);
	}

	var doctype =
	    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n" + 
	    " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n";

	return doctype + xml.toXMLString();
    });

    this.methodHandler = function (interfaceName, methodName, args) {
	var iface = obj.interfaces[interfaceName];
	if (iface != null) {
	    var methodInfo = iface.methods[methodName];
	    if (methodInfo != null) {
		var result = methodInfo.method.apply(obj, args);
		return result;
	    }
	}
	throw "Method not found";
    };
};

var DBUS = {
	getSystemBus: function() {
		return new DBusConnection("system");
	},

	getSessionBus: function() {
		return new DBusConnection("session");
	},

	// Type wrappers (javascript doesn't have these!!)
	UInt32: function(val) {
		var variant = Cc["@mozilla.org/variant;1"]
			.createInstance(Ci.nsIWritableVariant); 
		variant.setAsUint32(val);
		return variant;
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

	proxy.connectToSignal = function (signal_name, handler_function) {
		handler = {}
		handler['method'] = handler_function,
		/*
		handler['method'] = 
			ProxyUtils.getProxyOnUIThread(handler_function,
			                   Components.interfaces.nsIMyJSCallback);
		*/

		DBUS.core.ConnectToSignal(this.connection.busName,
		                          this.serviceName,
		                          this.connection.objectPath,
		                          this.interfaceName,
		                          signal_name,
		                          handler);
	};

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

DBusConnection.prototype.requestService = function(serviceName) {
    if (DBUS.core.RequestService(this.busName, serviceName)) {
	return new DBusService(this, serviceName);
    } else {
	throw "Failed to request service.";
    }
};

function DBusService(connection, serviceName) {
    this.connection = connection;
    this.serviceName = serviceName;
};

DBusService.prototype.exportObject = function(objectPath, obj) {
    var busName = this.connection.busName;
    DBUS.core.RegisterObject(busName, objectPath, obj.methodHandler);
};
