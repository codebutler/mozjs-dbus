#include "nsIVariant.h"
#include "nsCOMPtr.h"

#include <dbus/dbus.h>

#include <prlog.h>
#include <prclist.h>
#ifdef DEBUG
extern	PRLogModuleInfo *lm;
#endif

class MozJSDBusMarshalling 
{
public:
    static nsIVariant** getVariantArray(DBusMessageIter *iter, PRUint32 *length);

    static nsresult marshall(DBusMessage **);

    static nsresult marshallVariant(DBusMessage *, 
            nsIVariant *, DBusMessageIter *);

private:

    static nsCOMPtr<nsIWritableVariant> unMarshallBasic(int type, DBusMessageIter *iter);
    static nsCOMPtr<nsIWritableVariant> unMarshallArray(int type, DBusMessageIter *iter);

    static const int getDataTypeSize(PRUint16);
    static const int getDataTypeAsDBusType(PRUint16);
    static const char *getDBusTypeAsSignature(int);
};
