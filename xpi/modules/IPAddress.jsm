/*
 * IPAddress.jsm: Parses IP addresses in different formats.
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

EXPORTED_SYMBOLS = ['IPAddress'];

function IPAddress (decimal) {
	// only if big endian host, how do I check?
	isBigEndian = false
	if (isBigEndian) {
		this.decimal = (((decimal >> 24) & 0xFF)
				| ((decimal >> 08) & 0xFF00)
				| ((decimal << 08) & 0xFF0000)
				| ((decimal << 24)));
	} else {
		this.decimal = decimal;
	}

	this.toString = function () {
		return (this.decimal & 0xFF) + "." +
			((this.decimal >> 8) & 0xFF) + "." +
			((this.decimal >> 16) & 0xFF) + "." +
			((this.decimal >> 24) & 0xFF)

	};
}
