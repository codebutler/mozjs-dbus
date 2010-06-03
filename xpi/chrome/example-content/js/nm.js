/*
 * nm.js: 
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

Components.utils.import("resource://mozjs_dbus/DBUS.jsm");
Components.utils.import("resource://mozjs_dbus/IPAddress.jsm");

const NM_INTERFACE = "org.freedesktop.NetworkManager";
const NM_IFACE_INTERFACE = "org.freedesktop.NetworkManager.Devices";
/*
    methods: [ 'getName', 'getIP4Address', 'getType', 'getActiveNetwork' ]
};
*/

const NM_NETWORK_INTERFACE = "org.freedesktop.NetworkManager.Devices";
/*    methods: [ 'getName' ] */

function setupNM ()
{
    try {
        var nmDeviceList = document.getElementById("nmDeviceList");

        var bus = DBUS.systemBus;
      
        var nm = bus.getObject("org.freedesktop.NetworkManager",
                               "/org/freedesktop/NetworkManager",
                               NM_INTERFACE);

        var devices = nm.GetDevices();

        for (var x = 0; x < devices.length; x++) {
        
            var deviceProperties = bus.getObject('org.freedesktop.NetworkManager', 
                                                 devices[x],
                                                 'org.freedesktop.DBus.Properties');
            
            var name = deviceProperties.Get('org.freedesktop.NetworkManager.Device', 'Interface');
            
            nmDeviceList.appendItem(name, devices[x]);
        }

        nmDeviceList.selectedIndex = 0;

        var deviceChanged = function () {
            updateDeviceInfo();
        };

        nmDeviceList.addEventListener('select', deviceChanged, true);
        
        updateDeviceInfo();
    } catch (e) {
        alert(e);
        // XXX: Problem talking to network manager.
        nmDeviceList.appendItem("ERROR!!!", null);
        nmDeviceList.selectedIndex = 0;
    }
}

function updateDeviceInfo()
{
    try {
        var nmDeviceList = document.getElementById('nmDeviceList');
        var bus = DBUS.systemBus;

        var deviceId = nmDeviceList.selectedItem.value;
        
        var deviceProperties = bus.getObject('org.freedesktop.NetworkManager', deviceId, 'org.freedesktop.DBus.Properties');

        var name = deviceProperties.Get('org.freedesktop.NetworkManager.Device', 'Interface');
        var ip   = new IPAddress(deviceProperties.Get('org.freedesktop.NetworkManager.Device', 'Ip4Address'));
        var type = deviceProperties.Get('org.freedesktop.NetworkManager.Device', 'DeviceType')

        document.getElementById('nmDeviceName').value = name;
        document.getElementById('nmIPAddress').value  = ip.toString();
    } catch (e) {
        alert(e);
    }
}
