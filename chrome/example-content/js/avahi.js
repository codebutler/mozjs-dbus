/* vim:sw=4 sts=4 et ft=javascript
 *
 * avahi.js: Example avahi-discover clone.
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

Components.utils.import("resource://mozjs_dbus/DBUS.jsm");

const AVAHI_DBUS_NAME = "org.freedesktop.Avahi";
const AVAHI_DBUS_INTERFACE_SERVER = AVAHI_DBUS_NAME + ".Server";
const AVAHI_DBUS_PATH_SERVER = "/";
const AVAHI_DBUS_INTERFACE_ENTRY_GROUP = AVAHI_DBUS_NAME + ".EntryGroup";
const AVAHI_DBUS_INTERFACE_DOMAIN_BROWSER = AVAHI_DBUS_NAME + ".DomainBrowser";
const AVAHI_DBUS_INTERFACE_SERVICE_TYPE_BROWSER = AVAHI_DBUS_NAME + ".ServiceTypeBrowser";
const AVAHI_DBUS_INTERFACE_SERVICE_BROWSER = AVAHI_DBUS_NAME + ".ServiceBrowser";
const AVAHI_DBUS_INTERFACE_ADDRESS_RESOLVER = AVAHI_DBUS_NAME + ".AddressResolver";
const AVAHI_DBUS_INTERFACE_HOST_NAME_RESOLVER = AVAHI_DBUS_NAME + ".HostNameResolver";
const AVAHI_DBUS_INTERFACE_SERVICE_RESOLVER = AVAHI_DBUS_NAME + ".ServiceResolver";
const AVAHI_DBUS_INTERFACE_RECORD_BROWSER = AVAHI_DBUS_NAME + ".RecordBrowser";

const AVAHI_PROTOCOLS = { 0: "IPv4" };

var bus = DBUS.systemBus;
var server;

var allSignalHandlers = [];

// Top level tree
var protocols = {};

// Ech...
var serviceNames = {};

function newService (iface, proto, name, type, domain, flags) {
    dump("Found service '" + name + "' of type '" + type + "' in domain '" + domain + "' on '" + iface + ":" + proto + "'\n");
    
    var childrenItem = document.createElement("treechildren");
    
    var parent = protocols[proto].interfaces[iface].domains[domain].types[type].item;
    var item = document.createElement("treeitem");
    item.setAttribute("label", name);
    parent.appendChild(item);
    item.appendChild(childrenItem);
    
    var service = { iface: iface, proto: proto, name: name, type: type, domain: domain, flags: flags, item: childrenItem };
    protocols[proto].interfaces[iface].domains[domain].types[type].services[name] = service;
    serviceNames[name] = service;
}

function removeService(iface, proto, name, type, domain, flags) {
    dump("Service '" + name + "' of type '" + type + "' in domain '" + domain + "' on " + iface + "." + proto + " disappeared.\n");

    var item = protocols[proto].interfaces[iface].domains[domain].types[type].services[name].item.parentNode;
    item.parentNode.removeChild(item);
    
    delete protocols[proto].interfaces[iface].domains[domain].types[type].services[name];
}

function newServiceType(iface, proto, type, domain, flags) {
    dump("Browsing for services of type '" + type + " in domain '" + domain + "' on '" + iface + ":" + proto + "'\n");

    if (protocols[proto].interfaces[iface].domains[domain].types[type] == null) {
        var parent = protocols[proto].interfaces[iface].domains[domain].item;
        var item = document.createElement("treeitem");
        item.setAttribute("container", true);
        item.setAttribute("open", true);
        item.setAttribute("label", type);
        parent.appendChild(item);
        item = item.appendChild(document.createElement("treechildren"));
        
        protocols[proto].interfaces[iface].domains[domain].types[type] = { services: {}, item: item };
        
        var serviceBrowserPath = server.ServiceBrowserNew(iface, proto, type, domain, DBUS.Uint32(0));
        var serviceBrowser = bus.getObject(AVAHI_DBUS_NAME, serviceBrowserPath, AVAHI_DBUS_INTERFACE_SERVICE_BROWSER);

        allSignalHandlers.push(serviceBrowser.connectToSignal("ItemNew", newService));
        allSignalHandlers.push(serviceBrowser.connectToSignal("ItemRemove", removeService));
    }
}

function browseDomain(iface, proto, domain) {
    var typeBrowserPath = server.ServiceTypeBrowserNew(iface, proto, domain, DBUS.Uint32(0));
    var typeBrowser = bus.getObject(AVAHI_DBUS_NAME, typeBrowserPath, AVAHI_DBUS_INTERFACE_SERVICE_TYPE_BROWSER); 

    var parent = protocols[proto].interfaces[iface].item;
    var item = document.createElement("treeitem");
    item.setAttribute("container", true);
    item.setAttribute("open", true);
    item.setAttribute("label", domain);
    parent.appendChild(item);
    item = item.appendChild(document.createElement("treechildren"));
    
    protocols[proto].interfaces[iface].domains[domain] = { types: {}, item: item };

    allSignalHandlers.push(typeBrowser.connectToSignal("ItemNew", newServiceType));
}

function rowSelected(e) {
    var tree = e.target;
    var item = tree.contentView.getItemAtIndex(tree.currentIndex);
    var name = tree.view.getCellText(tree.currentIndex, tree.columns.getColumnAt(0));

    var service = serviceNames[name];
    
    if (service != null) {
        dump("Trying to resolve service '" + service.name + "'...\n");
            
        var callback = function (iface, proto, name, type, domain, host, aproto, address, port, txt, flags) {
            dump ("Service data for service '" + name + "' of type '" + type + "' in domain '" + domain + "' on " + iface + "." + proto + "\n");
            dump ("Host " + host + " (" + address + "), port " + port + "\n");
            
            var ifaceName = server.GetNetworkInterfaceNameByIndex(iface);
            
            var value = $H({
                "Service Type" : type,
                "Service Name" : name,
                "Domain Name"  : domain,
                "Interface"    : ifaceName + " " + AVAHI_PROTOCOLS[proto],
                "Address"      : host + "/" + address + ":" + port
            });
            
            if (txt != null) {
                txt.forEach(function(i) {
                    var item = i.join("").split("=");
                    value["TXT " + item[0]] = item[1];
                });
            }
            
            value = value.map(function(i) {
                return "<html:b>" + i[0] + "</html:b>: " + i[1];
            }).join("<html:br/>");
            
            document.getElementById("avahiServiceInfo").innerHTML = value;
        };
        
        server.ResolveService(service.iface, service.proto, service.name, 
                              service.type, service.domain, -1, DBUS.Uint32(0), 
                              callback);
    } else {
        document.getElementById("avahiServiceInfo").innerHTML = "";
    }
}

function setupAvahi() {
    bus.addNameAddedHandler(AVAHI_DBUS_NAME, avahiFound);
    bus.addNameRemovedHandler(AVAHI_DBUS_NAME, avahiLost);
}

function avahiFound() {
    try {
        server = bus.getObject(AVAHI_DBUS_NAME, AVAHI_DBUS_PATH_SERVER, AVAHI_DBUS_INTERFACE_SERVER);
        
        var devices;

        try {
            var nm = bus.getObject("org.freedesktop.NetworkManager",
                                   "/org/freedesktop/NetworkManager",
                                   NM_INTERFACE);

            devices = nm.getDevices(); 
            devices = devices.map(function (deviceObjectPath) {
                var device = bus.getObject("org.freedesktop.NetworkManager",
                                            deviceObjectPath, NM_IFACE_INTERFACE);
                return device.getName();
            });

        } catch (e) {
            // NetworkManager not available.
            devices = [ "eth0" ];
        }

        for (var protocol in AVAHI_PROTOCOLS) {
            var protocolName = AVAHI_PROTOCOLS[protocol];

            protocol = parseInt(protocol);

            protocols[protocol] = { name: protocolName, interfaces: {} };

            for (var x = 0; x < devices.length; x++) {

                var interfaceName = devices[x];
                var interfaceIndex = server.GetNetworkInterfaceIndexByName(interfaceName);
            
                var item = document.createElement("treeitem");
                item.setAttribute("container", true);
                item.setAttribute("open", true);
                item.setAttribute("label", interfaceName + " " + protocolName);
                document.getElementById("avahiTreeChildren").appendChild(item);
                item = item.appendChild(document.createElement("treechildren"));
                
                protocols[protocol].interfaces[interfaceIndex] = { name: interfaceName, domains: {}, item: item };
                
                browseDomain(interfaceIndex, protocol, "local");
                
                document.getElementById("avahiTree").addEventListener("select", rowSelected, true);
            }
        }
    
    } catch (e) {
        alert("Problem with AVAHI! " + e);
    }
}

function avahiLost() {
    dump("Lost avahi!\n");
    
    // Disconnect all signals
    for (var x = 0; x < allSignalHandlers.length; x++) {
        var signalId = allSignalHandlers[x];
        bus.disconnectFromSignal(signalId);
    }
    allSignalHandlers = [];

    // Clear the tree
    var children = document.getElementById("avahiTreeChildren");
    while (children.hasChildNodes()) {
        children.removeChild(children.firstChild);
    }

    // Clear the cache
    protocols = {};
    serviceNames = {};
}

