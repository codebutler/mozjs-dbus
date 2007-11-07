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
