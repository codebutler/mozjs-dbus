/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* vim:sw=4 sts=4 et ft=javascript
 *
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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

EXPORTED_SYMBOLS = ["DBUS", "DBusObject"];

Cu.import("resource://mozjs_dbus/ArrayConverter.jsm");
Cu.import("resource://mozjs_dbus/ProxyUtils.jsm");

function DBusObject () {
    this.interfaces = {};

    var introspectInterface = this.defineInterface('org.freedesktop.DBus.Introspectable');
    var obj = this;

    introspectInterface.defineMethod('Introspect', '', 's', function () {
        dump("Introspect !\n");

        var xml = <node/>;

        for (var interfaceName in obj.interfaces) {
            var interfaceXML = <interface name={interfaceName} />;

            // XXX: Try replacing these with "this.obj."
            var methods = obj.interfaces[interfaceName].methods;
            var signals = obj.interfaces[interfaceName].signals;

            // This is a hack, but apparently there's no other way.
            var getFunctionArgNames = function (funcToParse) {
                var str = funcToParse.toString();
                
                var re = /^function \((.*)\) \{$/m;
                str = str.match(re)[1];
                str = str.replace(/\s/g, "");

                return str.split(',');
            };

            var appendArgs = function (element, argNames, signature, direction) {
                for (var x = 0; x < signature.length; x++) {
                    var type = signature[x];
                    var argXML;
                    if (direction != null) {
                        if (argNames == null) {
                            var name = direction + "Arg" + x;
                        } else {
                            var name = argNames[x];
                        }
                        argXML = <arg direction={direction} type={type} name={name}/>;
                    } else {
                        var name = "arg" + x;
                        argXML = <arg type={type} name={name}/>;
                    }
                    element.appendChild(argXML);
                }
            };

            for (var methodName in methods) {
                var methodInfo = methods[methodName];
                var methodXML = <method name={methodName}/>;
                var argNames = getFunctionArgNames(methodInfo.method);
                appendArgs(methodXML, argNames, methodInfo['inSignature'], "in");
                appendArgs(methodXML, null, methodInfo['outSignature'], "out");
                interfaceXML.appendChild(methodXML);
            }

            for (var signalName in signals) {
                var signalInfo = signals[signalName];
                var signalXML = <signal name={signalName}/>;
                appendArgs(signalXML, null, signalInfo['signature'], null);
                interfaceXML.appendChild(signalXML);
            }

            xml.appendChild(interfaceXML);
        }
        
        var children = this.service.findChildrenNames(this.objectPath);
        for (var x in children) {
            var childName = children[x];
            var childNode = <node name={childName}/>;
            xml.appendChild(childNode);
        }

        var doctype =
            "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n" + 
            " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n";

        return doctype + xml.toXMLString();
    });
};

DBusObject.prototype = {
    defineInterface: function (name) {
        if (this.interfaces[name] != null) {
            throw "An interface by that name has already been defined.";
        }

        var iface = new DBusInterface(this, name);
        
        this.interfaces[name] = iface;

        return iface;
    },

    methodHandler: function (interfaceName, methodName, args) {
        dump("methodHandler\n");
        var iface = this.interfaces[interfaceName];
        if (iface != null) {
            var methodInfo = iface.methods[methodName];
            if (methodInfo != null) {
                var result = methodInfo.method.apply(this, args);
                return result;
            }
        }
        throw "Method not found";
    }
};

function DBusInterface (obj, name) {
    this.obj = obj;
    this.name = name;
    this.methods = {};
    this.signals = {};
}

DBusInterface.prototype = {
    defineMethod: function (methodName, inSignature, outSignature, func) {
        var method = {
             inSignature: inSignature,
            outSignature: outSignature, 
                  method: func
        };
        this.methods[methodName] = method;
    },

    defineSignal: function (signalName, signature) {
        var signal = {
            signature: signature
        };
        this.signals[signalName] = signal;
    },

    emitSignal: function (signalName) {
        if (this.signals[signalName] == null) {
            throw "Unknown signal";
        }

        var args = Array.prototype.slice.call(arguments, 1);
        DBUS.core.EmitSignal(this.obj.connection.busName,
                             this.obj.objectPath,
                             this.name,
                             signalName,
                             args.length,
                             args);
    }
};

var DBUS = {
    get systemBus() {
        if (!this._systemBus) {
            this._systemBus = new DBusConnection("system");
        }
        return this._systemBus;
    },

    get sessionBus() {
        if (!this._sessionBus) {
            this._sessionBus = new DBusConnection("session");
        }
        return this._sessionBus;
    }
};

// Type wrappers (javascript doesn't have these!!)
// Example usage: proxyObj.callSomeMethod("HelloWorld", DBUS.Int32(42));
var types = [ 'Double', 'Float', 'Int16', 'Int32', 'Int64', 'Uint16',
              'Uint32', 'Uint64' ];
types.forEach(function (type) {
    DBUS[type] = function (val) {
        var variant = Cc["@mozilla.org/variant;1"]
            .createInstance(Ci.nsIWritableVariant); 
        variant['setAs' + type](val);
        return variant;
    };
});

var klass = Components.classes["@extremeboredom.net/mozjs_dbus/MozJSDBusCoreComponent;1"];
DBUS.core = klass.createInstance(Components.interfaces.IMozJSDBusCoreComponent);

function DBusConnection(busName) {
    if (["session", "system"].indexOf(busName) == -1) {
        throw "Invalid bus name";
    }

    /* It ain't pretty... */
    if (busName == "system")
        this.busName = Ci.IMozJSDBusCoreComponent.SYSTEM_BUS;
    if (busName == "session")
        this.busName = Ci.IMozJSDBusCoreComponent.SESSION_BUS;

    this.seenNames = [];
    this.nameAddedHandlers = [];
    this.nameRemovedHandlers =  [];

    var busInterface = this.getObject('org.freedesktop.DBus',
                                      '/org/freedesktop/DBus',
                                      'org.freedesktop.DBus');

    var addName = function (name) {
        if (this.seenNames.indexOf(name) == -1) {
            this.seenNames.push(name);

            if (this.nameAddedHandlers[name] != null) {
                for (var x = 0; x < this.nameAddedHandlers[name].length; x++) {
                    var signal = this.nameAddedHandlers[name][x];
                    signal.apply(this, null);
                }
            }
        }
    };

    var removeName = function (name) {
        if (this.seenNames.indexOf(name) != -1) {
            delete this.seenNames[this.seenNames.indexOf(name)];

            if (this.nameRemovedHandlers[name] != null) {
                for (var x = 0; x < this.nameRemovedHandlers[name].length; x++) {
                    var signal = this.nameRemovedHandlers[name][x];
                    signal.apply(this, null);
                }
            }
        }
    };

    var nameOwnerChanged = function (name, old_owner, new_owner) {
        //dump("*** Name Owner Changed: \n" + "    Name: " + name + "\n    Old: " + old_owner + "\n    New: " + new_owner + "\n");
        if (old_owner == '' && new_owner != '') {
            // Name Added!
            addName.call(this.connection, name);

        } else if (old_owner != '' && new_owner == '') {
            // Name Removed!
            removeName.call(this.connection, name);

        }
    };

    busInterface.connectToSignal('NameOwnerChanged', nameOwnerChanged);
    
    var names = busInterface.ListNames()
    for (var x = 0; x < names.length; x++) {
        addName.call(this, names[x]);
    }
}

DBusConnection.prototype = {

    // XXX: I need a cleaner way to do custom events.

    addNameAddedHandler: function(serviceName, callback) {
        // I can't decide if this is good or not...
        // If we already have the name, fire event right away.
        if (this.seenNames.indexOf(serviceName) != -1) {
            callback.apply(this, null);
        }

        if (!this.nameAddedHandlers[serviceName]) {
            this.nameAddedHandlers[serviceName] = [];
        }
        this.nameAddedHandlers[serviceName].push(callback);
    },

    removeNameAddedHandler: function(serviceName) {
        if (this.nameAddedHandlers[serviceName]) {
            delete this.nameAddedHandlers[serviceName];
        }
    },

    addNameRemovedHandler: function(serviceName, callback) {
        if (!this.nameRemovedHandlers[serviceName]) {
            this.nameRemovedHandlers[serviceName] = [];
        }
        this.nameRemovedHandlers[serviceName].push(callback);
    },

    removeNameRemovedHandler: function(serviceName) {
        if (this.nameRemovedHandlers[serviceName]) {
            delete this.nameRemovedHandlers[serviceName];
        }
    },

    getObject: function(serviceName, objectPath, iface) {

        var objectInterface = null;

        if (typeof(iface) == 'string') {
            // Get the introspection xml
            // XXX: We need to first check if this object supports introspection!!
            var xml = DBUS.core.CallMethod(this.busName,
                                           serviceName,
                                           objectPath,
                                           "org.freedesktop.DBus.Introspectable",
                                           "Introspect",
                                           0,
                                           [],
                                           null);

            // DOCTYPE isnt supported by E4X
            xml = xml.replace(/<!DOCTYPE[^>]+>/, '');

            // Nor is <?xml...
            xml = xml.replace(/<\?xml[^\?>]+\?>/, '');
            
            xml = new XML(xml);

            // Find the requested interface
            for (var x in xml.interface) {
                var interfaceElement = xml.interface[x];
                if (interfaceElement.@name == iface) {
                    var methodNames = [];
                    for (var x in interfaceElement.method) {
                        methodNames.push(interfaceElement.method[x].@name);
                    }
                    objectInterface = { name: iface, methods: methodNames };
                    break;
                }
            }

            if (objectInterface == null) {
                throw "Object does not support this interface.";
            }

        } else {
            objectInterface = iface;
        }

        var proxy = new DBusProxyObject(this, serviceName, objectPath, objectInterface);
        return proxy;
    },

    requestService: function(serviceName) {
        if (DBUS.core.RequestService(this.busName, serviceName)) {
            return new DBusService(this, serviceName);
        } else {
            throw "Failed to request service.";
        }
    },

    releaseService: function(service) {
        throw "Not Implemented Yet";
    },

    disconnectFromSignal: function (signalId) {
        DBUS.core.DisconnectFromSignal(this.busName, signalId); 
    }
};

function DBusService(connection, serviceName) {
    this.connection = connection;
    this.serviceName = serviceName;

    this.objects = {};
};

DBusService.prototype = {
    registerObject: function(objectPath, obj) {
        var handler = function (interfaceName, methodName, args) { 
            return obj.methodHandler(interfaceName, methodName, args);
        };

        var busName = this.connection.busName;
        DBUS.core.RegisterObject(busName, objectPath, handler);
        obj.connection = this.connection;
        obj.objectPath = objectPath;
        obj.service = this;

        this.objects[objectPath] = obj;
    },

    unregisterObject: function(objectPath) {
        var busName = this.connection.busName;
        DBUS.core.UnregisterObject(busName, objectPath);
        delete this.objects[objectPath];
    },

    findChildrenNames: function (objectPath) {
        var pathParts = objectPath.split('/');

        var children = [];

        objectLoop:
        for (var thisPath in this.objects) {
            var thesePathParts = thisPath.split('/');

            if (thesePathParts.length == pathParts.length+1) {
                // Compare that every part of the path leading up to the
                // additional part is identical.
                for (var x = 0; x < pathParts.length; x++) {
                    if (pathParts[x] != thesePathParts[x]) {
                        // No match, skip this one.
                        continue objectLoop;
                    }
                }
                children.push(thesePathParts[thesePathParts.length-1]);
            }
        }

        return children;
    }
};

function DBusProxyObject (connection, serviceName, objectPath, objectInterface)
{
    this.connection = connection;
    this.serviceName = serviceName;
    this.objectPath = objectPath;
    this.interfaceName = objectInterface.name;
       
    var proxy = this;

    // Create proxy functions for dbus methods
    for (var x in objectInterface.methods) {
        var methodName = objectInterface.methods[x];
        var proxyFunction = function () {
            var methodName = arguments.callee.methodName;
            var argArray = Array.prototype.slice.call(arguments);
            
            // Callback specified as last argument. Yank it out and
            // pass it separately.
            if (argArray.length > 0 && typeof(argArray[argArray.length - 1]) == "function") {
                var callback = argArray.pop();
                var handler = function (interfaceName, signalName, args) {
                    callback.apply(proxy, args);
                };
            } else {
                var handler = null;
            }
            return this.callMethod(methodName, argArray, handler);
        };
        proxyFunction.methodName = methodName;
        proxy[methodName] = proxyFunction;
    }
}

DBusProxyObject.prototype = {
    connectToSignal: function (signalName, handlerFunction) {
        var obj = this;
        var handler = function (interfaceName, signalName, args) {
            handlerFunction.apply(obj, args);
        };

        var id = DBUS.core.ConnectToSignal(this.connection.busName,
                                           this.serviceName,
                                           this.objectPath,
                                           this.interfaceName,
                                           signalName,
                                           handler);
        return id;
    },

    callMethod: function (methodName, methodArgs, callback) {
        if (methodArgs == null) {
                methodArgs = [];
        }

        var result = DBUS.core.CallMethod(this.connection.busName,
                                          this.serviceName,
                                          this.objectPath,
                                          this.interfaceName,
                                          methodName,
                                          methodArgs.length,
                                          methodArgs,
                                          callback);

        return result;
    }
};
