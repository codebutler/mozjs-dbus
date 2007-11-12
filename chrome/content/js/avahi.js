/*
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

var bus = DBUS.getSystemBus();
var server;

// Top level tree
var protocols = {};

function newService (iface, proto, name, type, domain, flags) {
    dump("Found service '" + name + "' of type '" + type + "' in domain '" + domain + "' on '" + iface + ":" + proto + "'\n");
    
    var parent = protocols[proto].interfaces[iface].domains[domain].types[type].item;
    var item = document.createElement("treeitem");
    item.setAttribute("label", name);
    parent.appendChild(item);
    item = item.appendChild(document.createElement("treechildren"));
    
    protocols[proto].interfaces[iface].domains[domain].types[type].services[name] = { item: item };
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

        serviceBrowser.connectToSignal('ItemNew', newService);
        serviceBrowser.connectToSignal("ItemRemove", removeService);
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

    typeBrowser.connectToSignal("ItemNew", newServiceType);
}

function setupAvahi() {   
        
    server = bus.getObject(AVAHI_DBUS_NAME, AVAHI_DBUS_PATH_SERVER, AVAHI_DBUS_INTERFACE_SERVER);
    
    var protocol     = 0;
    var protocolName = "IPv4";

    // XXX: Don't hard interface name, use network manager!
    var interfaceName = "eth0";
    var interfaceIndex = server.GetNetworkInterfaceIndexByName(interfaceName);
    
    var item = document.createElement("treeitem");
    item.setAttribute("container", true);
    item.setAttribute("open", true);
    item.setAttribute("label", interfaceName + " " + protocolName);
    document.getElementById("avahiTreeChildren").appendChild(item);
    item = item.appendChild(document.createElement("treechildren"));
    
    protocols[protocol] = { name: protocolName, interfaces: {} };
    protocols[protocol].interfaces[interfaceIndex] = { name: interfaceName, domains: {}, item: item };
    
    browseDomain(interfaceIndex, protocol, "local");
    

}

