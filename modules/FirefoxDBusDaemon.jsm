/* vim:sw=4 sts=4 et ft=javascript
 *
 * FirefoxDBusDaemon.jsm: A rather hideous proof-of-concept Firefox
 *                        D-Bus service.
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

Cu.import("resource://mozjs_dbus/DBUS.jsm");

EXPORTED_SYMBOLS = ["FirefoxDBusDaemon"];

//=================================================
// Singleton that creates and controls the Firefox
// DBus service.
var FirefoxDBusDaemon = {
    loaded: false,

    // Increase for every new window. This is required because I can't find any
    // other way to uniquely identify a window.
    _windowCount: 0,

    // Maps chrome windows to dbus window objects.
    _windows: {},

    init: function () {
        this.loaded = true;

        var bus = DBUS.sessionBus;

        var firefoxService = bus.requestService("org.mozilla.Firefox");

        // --------------------------------------------------------------------
        // Firefox application object (/org/mozilla/Firefox/)
        // --------------------------------------------------------------------

        var firefoxObject = new DBusObject();

        var firefoxInterface = firefoxObject.defineInterface("org.mozilla.Firefox");

        // D-Bus Method: getActiveWindow
        firefoxInterface.defineMethod("getActiveWindow", "", "o", function () {
            var activeWindow = Utilities.windowMediator.getMostRecentWindow("navigator:browser");
            return FirefoxDBusDaemon.getObjectPathForWindow(activeWindow);
        });

        // D-Bus Method: getWindows
        firefoxInterface.defineMethod("getWindows", "", "ao", function () {
            var result = [];
            for (var win in FirefoxDBusDaemon._windows) {
                var id = FirefoxDBusDaemon._windows[win]._windowId;
                result.push("/org/mozilla/Firefox/Windows/" + id);
            }
            return result;
        });

        firefoxService.registerObject("/org/mozilla/Firefox", firefoxObject);

        firefoxService.registerObject("/org/mozilla/Firefox/Windows", new DBusObject());

        // Save this around for later
        this._service = firefoxService;
    },

    windowOpened: function (win) {
        if (win == null) {
            throw "'win' can't be null!";
        }

        // --------------------------------------------------------------------
        // Firefox Window objects (/org/mozilla/Firefox/Window/*)
        // --------------------------------------------------------------------

        var dbusWindow = new DBusObject();
        var dbusWindowInterface = dbusWindow.defineInterface("org.mozilla.Firefox.Window");

        // D-Bus Method: open
        dbusWindowInterface.defineMethod("open", "s", "", function (uri) {
            var browser = win.getBrowser().addTab(uri).linkedBrowser;
            
        });
        
        // D-Bus Method: getActiveTab
        dbusWindowInterface.defineMethod("getActiveTab", "", "o", function () {
            //var browser = win.getBrowser().selectedBrowser;
            var tab = win.activeTab;
        });

        // D-Bus Method: getTabs
        dbusWindowInterface.defineMethod("getTabs", "", "ao", function () {
            return "cheesE";
        });

        this._windowCount++;
        dbusWindow._windowId = this._windowCount;
        this._windows[win] = dbusWindow;

        this._service.registerObject(this.getObjectPathForWindow(win), dbusWindow);
    },

    windowClosed: function (win) {
        this._service.unregisterObject(this.getObjectPathForWindow(win));
        delete this._windows[win];
    },

    tabOpened: function (tab) {

    },

    tabClosed: function (tab) {

    },

    getObjectPathForWindow: function (win) {
        var windowNumber = this._windows[win]._windowId;
        return "/org/mozilla/Firefox/Windows/" + windowNumber;
    }
};

//=================================================
// Singleton that holds services and utilities
var Utilities = {
  _windowMediator : null,
  get windowMediator() {
    if (!this._windowMediator) {
      this._windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                             getService(Ci.nsIWindowMediator);
    }
    return this._windowMediator;
  }
};
