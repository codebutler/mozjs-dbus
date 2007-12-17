/* vim:sw=4 sts=4 et
 *
 * MozJSDBusMarshalling.cpp: Convert XPCOM data to/from DBUS data.
 *
 * Authors:
 *   Eric Butler <eric@extremeboredom.net>
 *
 *  This file is part of mozjs-dbus, and based on code from the DBuzilla 
 *  project. See below for original license/contributor information.
 */

/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Christophe Nowicki <cscm@csquad.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsIClassInfo.h"
#include "nsIXPConnect.h"
#include "nsIArray.h"
#include "nsIMutableArray.h"
#include "nsArrayUtils.h"
#include "nsStringAPI.h"
#include "MozJSDBusMarshalling.h"
#include "MozJSDBusCoreComponent.h"
#include "nsComponentManagerUtils.h"

nsresult
MozJSDBusMarshalling::appendArgs(DBusMessage      *message,
                                 DBusMessageIter  *iter,
                                 PRUint32         argsLength,
                                 nsIVariant**     args)
{
    nsresult rv;

    for (PRUint32 x = 0; x < argsLength; x++) {
        nsIVariant *arg = args[x];
        
        PRUint16 dataType;
        rv = arg->GetDataType(&dataType);
        NS_ENSURE_SUCCESS(rv, rv);
        printf ("Data type is %d \n", dataType);

        // This unwraps a basic type from a variant created by
        // DBUS.UInt32(), etc.
        if (dataType == nsIDataType::VTYPE_INTERFACE_IS) {
            nsISupports *blah;
            arg->GetAsISupports(&blah);

            nsIVariant *subVariant;
            rv = blah->QueryInterface(NS_GET_IID(nsIVariant), (void**)&subVariant);

            if (!NS_FAILED(rv)) {
                arg = subVariant;
            }
        }
    
        rv = MozJSDBusMarshalling::marshallVariant(message, arg, iter);
        NS_ENSURE_SUCCESS(rv,rv);
    }

    return NS_OK;
}

nsIVariant**
MozJSDBusMarshalling::getVariantArray(DBusMessageIter *iter, PRUint32 *retLength)
{
    nsCOMPtr<nsIMutableArray>       arrayItems;
    nsCOMPtr<nsIWritableVariant>    variant;
    int                             current_type;
    nsresult                        rv;
    nsIVariant**                    variants;
    PRUint32                        length;
    nsCOMPtr<nsIVariant>            item;
    
    arrayItems = do_CreateInstance("@mozilla.org/array;1");

    while ((current_type = dbus_message_iter_get_arg_type (iter))
            != DBUS_TYPE_INVALID) {
    
        variant = unMarshallIter(current_type, iter);
        
        if (variant != NULL) {
            arrayItems->AppendElement(variant, PR_FALSE);
        }

        dbus_message_iter_next(iter);
    }

    arrayItems->GetLength(&length);

    variants = new nsIVariant*[length];

    for (PRUint32 x = 0; x < length; x++) {
        item = do_QueryElementAt(arrayItems, x);
        variants[x] = item;
        NS_ADDREF(item);
    }

    *retLength = length;
    return variants;
}


nsCOMPtr<nsIWritableVariant>
MozJSDBusMarshalling::unMarshallIter(int current_type, DBusMessageIter *iter)
{
    nsCOMPtr<nsIWritableVariant>    variant;
    nsresult rv;
    
    if (dbus_type_is_basic(current_type)) {
        variant = unMarshallBasic(current_type, iter);
    } else if (dbus_type_is_container(current_type)) {
        switch (current_type) {
            case DBUS_TYPE_ARRAY:
            {
                variant = unMarshallArray(current_type, iter);
                break;
            }
            case DBUS_TYPE_DICT_ENTRY:
            {
                break;
            }
            default:
            {
                break;
            }
        }
    }
    if (!variant) {
        variant = do_CreateInstance("@mozilla.org/variant;1", &rv);
    }
    return variant;
}

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

nsCOMPtr<nsIWritableVariant>
MozJSDBusMarshalling::unMarshallArray(int type, DBusMessageIter *iter)
{
    DBusMessageIter                 subiter;
    int                             current_subtype;
    nsresult                        rv;
    nsCOMPtr<nsIWritableVariant>    variant;
    nsCOMPtr<nsIMutableArray>       arrayItems;
    nsCOMPtr<nsIWritableVariant>    v;
    
    dbus_message_iter_recurse(iter, &subiter);
    
    PRUint32 length;
    
    nsIVariant** array = getVariantArray(&subiter, &length);
    
    if (length > 0) {
        variant = do_CreateInstance("@mozilla.org/variant;1", &rv);
        
        rv = variant->SetAsArray(nsIDataType::VTYPE_INTERFACE_IS,
                                 &NS_GET_IID(nsIVariant), length, array);

        if (NS_FAILED(rv)) {
            printf("AIEE!!!! PROBLEM!!!! %d\n", length);
            return NULL;
        }

        return variant;
    } else {
        return NULL;
    }
}


#define CASE_DBUS_TYPE_AS_STRING(name) \
    case DBUS_TYPE_##name: \
        _ret = DBUS_TYPE_##name##_AS_STRING; \
        break;

const char *
MozJSDBusMarshalling::getDBusTypeAsSignature(int type)
{
    const char *_ret = NULL;

    switch(type) {
        CASE_DBUS_TYPE_AS_STRING(BYTE)
        CASE_DBUS_TYPE_AS_STRING(BOOLEAN)
        CASE_DBUS_TYPE_AS_STRING(INT16)
        CASE_DBUS_TYPE_AS_STRING(UINT16)
        CASE_DBUS_TYPE_AS_STRING(INT32)
        CASE_DBUS_TYPE_AS_STRING(UINT32)
        CASE_DBUS_TYPE_AS_STRING(INT64)
        CASE_DBUS_TYPE_AS_STRING(UINT64)
        CASE_DBUS_TYPE_AS_STRING(DOUBLE)
        CASE_DBUS_TYPE_AS_STRING(STRING)
        //default:
    //      PR_LOG(lm, PR_LOG_ERROR, 
        //          ("Unknown or unsupported nsIDataType: %d.\n", type) );
    }

    return _ret;
}

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
MozJSDBusMarshalling::marshallVariant(DBusMessage     *msg, 
                                      nsIVariant      *param, 
                                      DBusMessageIter *it)
{
    PRUint16 type;
    nsresult nv;
    nsresult rv;

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
            // XXX: Create a DICT_ENTRY array
            // XXX: User may want a STRUCT instead?

            nsIID *iid;
            nsISupports *interface;
            nsresult ns;

            nv = param->GetAsInterface(&iid, (void **) &interface);
            if(NS_FAILED(nv)) {
                return NS_ERROR_FAILURE;
            }
    
            PRUint32 count;
            nsIID **array;

            nsCOMPtr<nsIXPConnectWrappedJS> wrapped = do_QueryInterface(interface);

            if (wrapped != NULL) {
                printf ("Info was something !!\n");
                
                JSObject* jsObject;
                rv = wrapped->GetJSObject(&jsObject);
                NS_ENSURE_SUCCESS(rv, rv);
                
            } else {
                printf ("Info was null !!\n");
            }


            DBusMessageIter sub;
            dbus_message_iter_open_container(it, DBUS_TYPE_ARRAY, "{sv}", &sub);
            dbus_message_iter_close_container(it, &sub);
            
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
                    nv = marshallVariant(msg, ptr, &sub);
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
        
        PRUnichar *str;
        PRUint32 size;
        param->GetAsWStringWithSize(&size, &str);

        // XXX: Obviously, we want to use UTF8 here, not ASCII,
        // however I cannot get this to work. I think the problem is
        // related to this library being compiled with -fshort-wchar,
        // and the dbus library being compiled without it.
        nsCAutoString ascii_str =
        NS_LossyConvertUTF16toASCII(str, size);

        const char *c_str = ascii_str.get();

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

            // XXX: We cant just always assume its an interger array!!

            dbus_message_iter_open_container(it, 
                    DBUS_TYPE_ARRAY, "i", &sub);
            dbus_message_iter_close_container(it, &sub);
            break;
        }
        case nsIDataType::VTYPE_EMPTY: {
        printf("AAAHH!!!\n");
            break;
        }
        default:
            //PR_LOG(lm, PR_LOG_ERROR, 
            //      ("Unknown DataType.\n") );
            return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

#define CASE_DATA_TYPE_SIZE(name, type) \
    case nsIDataType::VTYPE_##name: \
        _ret = sizeof( type ); \
        break;

const int
MozJSDBusMarshalling::getDataTypeSize(PRUint16 type)
{
    int _ret = 0;
    switch(type) {
        CASE_DATA_TYPE_SIZE(INT8,  PRUint16)
        CASE_DATA_TYPE_SIZE(UINT8,  PRUint16)
        CASE_DATA_TYPE_SIZE(BOOL,  PRBool)
        CASE_DATA_TYPE_SIZE(INT16, PRInt16)
        CASE_DATA_TYPE_SIZE(UINT16, PRUint16)
        CASE_DATA_TYPE_SIZE(INT32, PRInt32)
        CASE_DATA_TYPE_SIZE(UINT32, PRUint32)
        CASE_DATA_TYPE_SIZE(INT64, PRInt64)
        CASE_DATA_TYPE_SIZE(UINT64, PRUint64)
        CASE_DATA_TYPE_SIZE(DOUBLE, double)
        case nsIDataType::VTYPE_WCHAR_STR: 
        case nsIDataType::VTYPE_WSTRING_SIZE_IS: 
            _ret = -1; 
            break;
       // default:
          //PR_LOG(lm, PR_LOG_ERROR, 
              //    ("Unknown or unsupported nsIDataType: %d.\n", type) );
    }
    return _ret;

}

const int
MozJSDBusMarshalling::getDataTypeAsDBusType(PRUint16 type)
{
    int _ret = 0;

    switch(type) {
        case nsIDataType::VTYPE_INT8:
        case nsIDataType::VTYPE_UINT8:
            _ret = DBUS_TYPE_BYTE; break;
        case nsIDataType::VTYPE_BOOL:
            _ret = DBUS_TYPE_BOOLEAN; break;
        case nsIDataType::VTYPE_INT16:
            _ret = DBUS_TYPE_INT16; break;
        case nsIDataType::VTYPE_UINT16:
            _ret = DBUS_TYPE_UINT16; break;
        case nsIDataType::VTYPE_INT32:
            _ret = DBUS_TYPE_INT32; break;
        case nsIDataType::VTYPE_UINT32:
            _ret = DBUS_TYPE_UINT32; break;
        case nsIDataType::VTYPE_INT64:
            _ret = DBUS_TYPE_INT64; break;
        case nsIDataType::VTYPE_UINT64:
            _ret = DBUS_TYPE_UINT64; break;
        case nsIDataType::VTYPE_DOUBLE:
            _ret = DBUS_TYPE_DOUBLE; break;
        case nsIDataType::VTYPE_WCHAR_STR:
        case nsIDataType::VTYPE_WSTRING_SIZE_IS:
            _ret = DBUS_TYPE_STRING; break;
    //    default:
          //PR_LOG(lm, PR_LOG_ERROR, 
              //    ("Unknown or unsupported nsIDataType: %d.\n", type) );
    }
    return _ret;
}

