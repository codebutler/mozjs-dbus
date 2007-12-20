/* vim:sw=4 sts=4 et ft=javascript
 *
 * DBusXMLParser.jsm: Parses DBUS object introspection XML.
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

EXPORTED_SYMBOLS = [
    "DBusXMLParser",

    "DBusInterface",
    "DBusMethod",
    "DBusMethodArg",
    "DBusSignal",
    "DBusProperty",
    "DBusAnnotation"
];

var DBusXMLParser = {

    parse: function(rootElement) {
        var interfaces = [];

        var interfaceElements = rootElement.interface;
        
        for (var i = 0; i < interfaceElements.length(); i++) {
            
            var interfaceElement = interfaceElements[i];

            var interfaceName = interfaceElement.@name;
            var interface = new DBusInterface(interfaceName);

            // Parse methods
            var methodElements = interfaceElement.method;
            for (var x = 0; x < methodElements.length(); x++) {
                var methodElement = methodElements[x];
                var methodArgElements = methodElement.arg;

                var methodName = methodElement.@name;

                var method = new DBusMethod(methodName);

                for (var y = 0; y < methodArgElements.length(); y++) {
                    var methodArgElement = methodArgElements[y];

                    if (methodArgElement.@direction == 'in') {
                        var methodArgName = methodArgElement.@name;
                        var methodArg = new DBusMethodArg(methodArgName);
                        method.args.push(methodArg);
                    } else {
                        // What to do?
                    }
                }

                interface.methods.push(method);
            }

            // XXX: Parse properties
            // XXX: Parse Signals
            
            interfaces.push(interface);
        }
        return interfaces;
    }

};

/** Types for object representation of xml data **/

function DBusInterface (name) {
    this.name = name;
}
DBusInterface.prototype.methods = [];

function DBusMethod (name) {
    this.name = name;
}
DBusMethod.prototype.args = [];

function DBusMethodArg (name) {
    this.name = name;
}

function DBusSignal () {
}

function DBusProperty () {
}

function DBusAnnotation () {
}
