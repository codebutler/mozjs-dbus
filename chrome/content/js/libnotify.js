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

Components.utils.import("resource://app/modules/DBUS.jsm");

const NOTIFY_SERVICE     = "org.freedesktop.Notifications";
const NOTIFY_INTERFACE   = "org.freedesktop.Notifications";
const NOTIFY_OBJECT_PATH = "/org/freedesktop/Notifications";

function setupLibNotify ()
{
	var sendNotificationButton = document.getElementById('sendNotificationButton');

	sendNotificationButton.addEventListener('command', sendNotification, true);

	var bus = DBUS.getSessionBus();
	var nf  = bus.getObject(NOTIFY_SERVICE, NOTIFY_OBJECT_PATH,
				NOTIFY_INTERFACE);

	nf.connectToSignal('NotificationClosed', function (id, reason) {
		alert('Notification Closed!');
	});

	nf.connectToSignal('ActionInvoked', function (id, action_key) {
		alert('Action Invoked!');
	});
}

function sendNotification ()
{
	try {
		var bus = DBUS.getSessionBus();
		var nf  = bus.getObject(NOTIFY_SERVICE, NOTIFY_OBJECT_PATH,
		                        NOTIFY_INTERFACE);
 
 		var id = 0;

		var summary = document.getElementById("summaryText").value;
		var body = document.getElementById("bodyText").value;
		
		id = nf.Notify("test", DBUS.UInt32(id), "", summary, body, [""], {}, 0);

	} catch (e) {
		alert(e);
	}
}
