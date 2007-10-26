#include "nsIVariant.h"
#include "nsCOMPtr.h"

#include <dbus/dbus.h>

class MozJSDBusMarshalling 
{
public:
	static nsCOMPtr<nsIWritableVariant> unMarshallBasic(int type, DBusMessageIter *iter);
};
