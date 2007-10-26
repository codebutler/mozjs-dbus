/*
 * libnotify.js: 
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

Components.utils.import("resource://app/components/DBUS.jsm");

const NOTIFY_SERVICE     = "org.freedesktop.Notifications";
const NOTIFY_INTERFACE   = "org.freedesktop.Notifications";
const NOTIFY_OBJECT_PATH = "/org/freedesktop/Notifications";

function setupLibNotify ()
{
	var sendNotificationButton = document.getElementById('sendNotificationButton');

	sendNotificationButton.addEventListener('command', sendNotification, true);
}

function sendNotification ()
{
	try {
		var bus = DBUS.getSessionBus();
		var nf  = bus.getObject(NOTIFY_SERVICE, NOTIFY_OBJECT_PATH,
		                        NOTIFY_INTERFACE);
 
 		var id = 0;

		//id = nf.Notify("test", id, icon, summary, body, actions, hints, timeout);
		id = nf.Notify("test", id, null, "summary", "body", null, null, null);

	} catch (e) {
		alert(e);
	}
}
