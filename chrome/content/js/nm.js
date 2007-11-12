/*
 * nm.js: 
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

Components.utils.import("resource://app/modules/DBUS.jsm");
Components.utils.import("resource://app/modules/IPAddress.jsm");

// NetworkManager does not support introspection, so we specify
// what this interface supports.
const NM_INTERFACE = {
       name: "org.freedesktop.NetworkManager",
    methods: [ 'getDevices',
           'getActiveDevice',
           'setActiveDevice',
           'status' ]
};

/*
const NM_IFACE_INTERFACE = {
       name: "org.freedesktop.NetworkManager.Devices",
    methods: {          'getName': [ DBUS.STRING_TYPE, DBUS.INTEGER_TYPE ],
                  'getIP4Address': [ DBUS.BOOL_TYPE ],
           'getActiveNetwork': [] }
};
*/

const NM_IFACE_INTERFACE = {
       name: "org.freedesktop.NetworkManager.Devices",
    methods: [ 'getName', 'getIP4Address', 'getActiveNetwork' ]
};

const NM_NETWORK_INTERFACE = {
       name: "org.freedesktop.NetworkManager.Devices",
    methods: [ 'getName' ]
};

function setupNM ()
{
    try {
        var nmDeviceList = document.getElementById('nmDeviceList');

        var bus = DBUS.getSystemBus();
        
        var nm = bus.getObject("org.freedesktop.NetworkManager",
                       "/org/freedesktop/NetworkManager",
                       NM_INTERFACE);

        var devices = nm.getDevices();

        for (var x = 0; x < devices.length; x++) {
            // Interface name is last part of path, avoid dbus call by
            // just extracting that.
            /*
            var name = devices[x].split('/')[-1];
            alert(devices[x] + "   " + name);
            */

            var device = bus.getObject("org.freedesktop.NetworkManager",
                           devices[x], NM_IFACE_INTERFACE);
            var name = device.getName();
            nmDeviceList.appendItem(name, devices[x]);
        }

        /*
        var activeDevice = nm.getActiveDevice();
        alert(activeDevice);
        */

        nmDeviceList.selectedIndex = 0;

        var deviceChanged = function () {
            updateDeviceInfo();
        };

        nmDeviceList.addEventListener('select', deviceChanged, true);
        
        updateDeviceInfo();
    } catch (e) {
        // XXX: Problem talking to network manager.
        nmDeviceList.appendItem("ERROR!!!", null);
        nmDeviceList.selectedIndex = 0;
    }
}

function updateDeviceInfo()
{
    try {
        var nmDeviceList = document.getElementById('nmDeviceList');
        var bus = DBUS.getSystemBus();

        var deviceId = nmDeviceList.selectedItem.value;
        var device = bus.getObject("org.freedesktop.NetworkManager",
                       deviceId, NM_IFACE_INTERFACE);

        var ip = new IPAddress(device.getIP4Address());

        /*
        var network = bus.getObject("org.freedesktop.NetworkManager",
                        ath0.getActiveNetwork(),
                        nmNetworkInterface);
        */

        document.getElementById('nmDeviceName').value = device.getName();
        document.getElementById('nmIPAddress').value = ip.toString();
        //document.getElementById('nmESSID').value = network.getName();
        
        dump('aye');
    } catch (e) {
        alert(e);
    }
}
