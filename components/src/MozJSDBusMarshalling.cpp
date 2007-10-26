#include "MozJSDBusMarshalling.h"
#include "MozJSDBusCoreComponent.h"
#include "nsComponentManagerUtils.h"

nsCOMPtr<nsIWritableVariant>
MozJSDBusMarshalling::unMarshallBasic(int type, DBusMessageIter *iter)
{
    nsresult rv;
    nsCOMPtr<nsIWritableVariant> variant =
		do_CreateInstance("@mozilla.org/variant;1", &rv);
    if (NS_FAILED(rv)) {
        //PR_LOG(lm, PR_LOG_DEBUG, ("do Create Instance failed"));
        return nsnull;
    }

    switch(type) {
        case DBUS_TYPE_BYTE: 
        {
           char val;
           dbus_message_iter_get_basic(iter, &val);
           variant->SetAsChar(val);
           break;
        }
        case DBUS_TYPE_BOOLEAN: 
        {
            PRBool val;
            dbus_message_iter_get_basic(iter, &val);
            variant->SetAsBool(val);
            break;
        }
        case DBUS_TYPE_INT16: 
        {
            PRInt16 val;
            dbus_message_iter_get_basic(iter, &val);
            variant->SetAsInt16(val);
            break;
        }
        case DBUS_TYPE_UINT16: 
        {
            PRUint16 val;
            dbus_message_iter_get_basic(iter, &val);
            variant->SetAsUint16(val);
            break;
        }
        case DBUS_TYPE_INT32: 
        {
            PRInt32 val;
            dbus_message_iter_get_basic(iter, &val);
            variant->SetAsInt32(val);
            break;
        }
        case DBUS_TYPE_UINT32: 
        {
            PRUint32 val;
            dbus_message_iter_get_basic(iter, &val);
            variant->SetAsUint32(val);
            break;
        }
        case DBUS_TYPE_INT64: 
        {
            PRInt64 val;
            dbus_message_iter_get_basic(iter, &val);
            variant->SetAsInt64(val);
            break;
        }
        case DBUS_TYPE_UINT64: 
        {
            PRUint64 val;
            dbus_message_iter_get_basic(iter, &val);
            variant->SetAsUint64(val);
            break;
        }
        case DBUS_TYPE_DOUBLE: 
        {
            double val;
            dbus_message_iter_get_basic(iter, &val);
            variant->SetAsDouble(val);
            break;
        }
        case DBUS_TYPE_STRING:
        case DBUS_TYPE_SIGNATURE:
        case DBUS_TYPE_OBJECT_PATH:
        {
            char *val;
            dbus_message_iter_get_basic(iter, &val);
            variant->SetAsString(val);
            break;
        }
        default:
        {
            //PR_LOG(lm, PR_LOG_ERROR, ("Unknown data type"));
            return nsnull;
        }
    } 
    return variant;
}

/*

#define MARSHALL_CASE(type, name) \
    case nsIDataType::VTYPE_##type: { \
        PR##name val; \
        nv = param->GetAs##name(&val); \
        if (NS_FAILED(nv)) { \
            return nv; \
        } \
        if (!dbus_message_iter_append_basic(it, \
            DBUS_TYPE_##type, &val)) { \
            return NS_ERROR_OUT_OF_MEMORY; \
        } \
        break; \
    }

nsresult
nsDBusMessageImpl::marshallVariant(DBusMessage *msg, 
        nsIVariant *param, DBusMessageIter *it) const 
{
    PRUint16 type;
    nsresult nv;

    param->GetDataType(&type);
    switch (type) {
        case nsIDataType::VTYPE_INT8: { // FIXME : better check
            PRUint8 val;
            param->GetAsInt8(&val);
            if (!dbus_message_iter_append_basic(it, 
                        DBUS_TYPE_INT16, &val)) { 
                return NS_ERROR_OUT_OF_MEMORY;
            } 
            break;
        } 
        MARSHALL_CASE(INT16, Int16)
        MARSHALL_CASE(INT32, Int32)
        MARSHALL_CASE(INT64, Int64)
        case nsIDataType::VTYPE_UINT8: { // FIXME : better check
            PRUint8 val;
            nv = param->GetAsUint8(&val);
            if (NS_FAILED(nv)) {
                return nv;
            }
            if (!dbus_message_iter_append_basic(it, 
                        DBUS_TYPE_INT16, &val)) { 
                return NS_ERROR_OUT_OF_MEMORY;
            } 
            break;
        }
        MARSHALL_CASE(UINT16, Uint16)
        MARSHALL_CASE(UINT32, Uint32)
        MARSHALL_CASE(UINT64, Uint64)

        case nsIDataType::VTYPE_FLOAT: { // TODO: check precision
            float val;
            nv = param->GetAsFloat(&val);
            if (NS_FAILED(nv)) {
                return nv;
            }
            if (!dbus_message_iter_append_basic(it, 
                        DBUS_TYPE_DOUBLE, &val)) { 
                return NS_ERROR_OUT_OF_MEMORY;
            } 
            break;
        }
        case nsIDataType::VTYPE_DOUBLE: {
            double val;
            nv = param->GetAsDouble(&val);
            if (NS_FAILED(nv)) {
                return nv;
            }
            if (!dbus_message_iter_append_basic(it, 
                        DBUS_TYPE_DOUBLE, &val)) { 
                return NS_ERROR_OUT_OF_MEMORY;
            } 
            break;
        }
        case nsIDataType::VTYPE_BOOL: {
            PRBool val;
            nv = param->GetAsBool(&val);
            if (NS_FAILED(nv)) {
                return nv;
            }
            if (!dbus_message_iter_append_basic(it, 
                        DBUS_TYPE_BOOLEAN, &val)) { 
                return NS_ERROR_OUT_OF_MEMORY;
            } 
            break;
        }
        case nsIDataType::VTYPE_CHAR: {
            break;
        }
        case nsIDataType::VTYPE_WCHAR: {
            break;
        }
        case nsIDataType::VTYPE_VOID: {
            break;
        }
        case nsIDataType::VTYPE_ID: {
            break;
        }
        case nsIDataType::VTYPE_DOMSTRING: {
            break;
        }
        case nsIDataType::VTYPE_CHAR_STR: {
            break;
        }
        case nsIDataType::VTYPE_WCHAR_STR: {
            break;
        }
        case nsIDataType::VTYPE_INTERFACE: {
            break;
        }
        case nsIDataType::VTYPE_INTERFACE_IS: { 
            // TODO: Interface are object, need to query class info
            // and build STRUCT
            nsIID *iid;
            nsISupports *interface;
            nsresult ns;

            nv = param->GetAsInterface(&iid, (void **) &interface);
            if(NS_FAILED(nv)) {
                return NS_ERROR_FAILURE;
            }
            
            //printf("IID %s\n", iid.ToString()); 
            break;
        }
        case nsIDataType::VTYPE_ARRAY: {
            DBusMessageIter sub; 
            PRUint16 type;
            nsIID iid;
            PRUint32 count;
            void *data;

            nv = param->GetAsArray(&type , &iid , &count , &data);
            if (NS_FAILED(nv)) {
                return nv;
            }

            // if type == VTYPE_INTERFACE_IS, it's an mixed array
            // all D-BUS array elements must be the same, 
            // so we transfom array to struct
            if (type == nsIDataType::VTYPE_INTERFACE_IS) {
                if (!iid.Equals(NS_GET_IID(nsIVariant))) {
                    return NS_ERROR_FAILURE;
                } 

                nsIVariant *ptr = *((nsIVariant**)data);

                dbus_message_iter_open_container(it, DBUS_TYPE_STRUCT, 
                        NULL, &sub);
                
                for (PRUint32 i = 0; i < count; i++, 
                    ptr = *((nsIVariant**)data + i)) {
                    nv = this->marshallVariant(msg, ptr, &sub);
                    if (NS_FAILED(nv))
                        return nv;
                }
                
                dbus_message_iter_close_container(it, &sub);
                break;
            }

            int dbus_type = getDataTypeAsDBusType(type);
            int data_type_size = getDataTypeSize(type);
            const char* sign = getDBusTypeAsSignature(dbus_type);
            unsigned char *ptr;
            PRUnichar* c_ptr;
            unsigned char *c_value;

            dbus_message_iter_open_container(it, 
                    DBUS_TYPE_ARRAY, sign, &sub);

            if (type == nsIDataType::VTYPE_WCHAR_STR) {
                c_ptr = *((PRUnichar**)data);
            } else {
                ptr = (unsigned char *) data;
            }

            for (PRUint32 i = 0; i < count; i++) {

                if (type == nsIDataType::VTYPE_WCHAR_STR) {
                    c_value = (unsigned char *) 
                        NS_ConvertUTF16toUTF8(c_ptr).get();
                    if (!dbus_message_iter_append_basic(&sub, 
                        dbus_type, &c_value)) { 
                        return NS_ERROR_OUT_OF_MEMORY;
                    }
                    c_ptr = *((PRUnichar**)data + i + 1);
                } else {
                    if (!dbus_message_iter_append_basic(&sub, 
                        dbus_type, ptr)) { 
                        return NS_ERROR_OUT_OF_MEMORY;
                    }
                    ptr += data_type_size;  
                }
            }

            dbus_message_iter_close_container(it, &sub);
            break;
        }
        case nsIDataType::VTYPE_STRING_SIZE_IS: {
            PRUint32 size;
            char *str;
            param->GetAsStringWithSize(&size , &str);
            if (!dbus_message_iter_append_basic(it, 
                        DBUS_TYPE_STRING, &str)) { 
                return NS_ERROR_OUT_OF_MEMORY;
            } 
            break;
        }
        case nsIDataType::VTYPE_WSTRING_SIZE_IS: {
            PRUint32 size;
            PRUnichar *str;
            const char *c_str;
            param->GetAsWStringWithSize(&size , &str);
            c_str = NS_ConvertUTF16toUTF8(str).get();
            if (!dbus_message_iter_append_basic(it, 
                        DBUS_TYPE_STRING, &c_str)) { 
                return NS_ERROR_OUT_OF_MEMORY;
            } 
            break;
        }
        case nsIDataType::VTYPE_UTF8STRING: {
            break;
        }
        case nsIDataType::VTYPE_CSTRING: {
            break;
        }
        case nsIDataType::VTYPE_ASTRING: {
            break;
        }
        case nsIDataType::VTYPE_EMPTY_ARRAY: {
            DBusMessageIter sub; 

            dbus_message_iter_open_container(it, 
                    DBUS_TYPE_ARRAY, "i", &sub);
            dbus_message_iter_close_container(it, &sub);
            break;
        }
        case nsIDataType::VTYPE_EMPTY: {
            break;
        }
        default:
            PR_LOG(lm, PR_LOG_ERROR, 
                    ("Unknown DataType.\n") );
            return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

*/
