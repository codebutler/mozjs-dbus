/* vim:sw=4 sts=4 et
 *
 * service.js: 
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

function setupService () 
{
    var bus = DBUS.sessionBus;
    
    var service = bus.requestService('com.codebutler.MozJSDBus.TestService');
    
    var iface = null;
    
    service.registerObject('/Test', function () {
        this.defineInterface("com.codebutler.MozJSDBus.TestInterface", function () {
            iface = this;
            this.defineMethod('sum', 'ii', 'i', function(first, second) {
                return first+second;
            });
            
            this.defineMethod('hello', '', 's', function() {
                return "Hello World!";
            });

            this.defineSignal('somethingHappen', 'ssi');
        });
    });

    var emitSignalButton = document.getElementById('emitSignalButton');
    emitSignalButton.addEventListener('command', function () {
        iface.emitSignal('somethingHappen', 'foo', 'bar', 42);
    }, true);
}
