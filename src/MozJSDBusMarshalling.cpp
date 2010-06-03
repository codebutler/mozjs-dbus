/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* vim:sw=4 sts=4 et
 *
 * MozJSDBusMarshalling.cpp: Convert XPCOM data to/from DBUS data.
 *
 * Authors:
 *   Eric Butler <eric@codebutler.com>
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

        // This unwraps a basic type from a variant created by
        // DBUS.UInt32(), etc.
        if (dataType == nsIDataType::VTYPE_INTERFACE_IS) {
            nsISupports *blah;
            rv = arg->GetAsISupports(&blah);
            NS_ENSURE_SUCCESS(rv, rv);

            nsIVariant *subVariant;
            rv = blah->QueryInterface(NS_GET_IID(nsIVariant),
                                      (void**)&subVariant);
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
MozJSDBusMarshalling::getVariantArray(DBusMessageIter *iter,
                                      PRUint32 *retLength)
{
    nsCOMPtr<nsIMutableArray>       arrayItems;
    nsCOMPtr<nsIWritableVariant>    variant;
    int                             current_type;
    nsIVariant**                    variants;
    PRUint32                        length;
    nsCOMPtr<nsIVariant>            item;
    
    arrayItems = do_CreateInstance("@mozilla.org/array;1");

    while ((current_type = dbus_message_iter_get_arg_type (iter))
            != DBUS_TYPE_INVALID)
    {
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
            case DBUS_TYPE_VARIANT:
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
                printf("Unknown data type\n");
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
        printf("do Create Instance failed\n");
        return nsnull;
    }

    switch(type) {
        case DBUS_TYPE_BYTE: 
        {
            char val; //PRUint8?
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
            printf("Unknown data type\n");
            return nsnull;
        }
    } 
    return variant;
}

nsCOMPtr<nsIWritableVariant>
MozJSDBusMarshalling::unMarshallArray(int type, DBusMessageIter *iter)
{
    DBusMessageIter                 subiter;
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
        default:
            printf("Unknown or unsupported nsIDataType: %d.\n", type);
    }

    return _ret;
}

#define MARSHALL_CASE(type, name)               \
    case nsIDataType::VTYPE_##type: {           \
        PR##name val;                           \
        nv = param->GetAs##name(&val);          \
        NS_ENSURE_SUCCESS(nv, nv);              \
        if (!dbus_message_iter_append_basic(it, \
            DBUS_TYPE_##type, &val)) {          \
            return NS_ERROR_OUT_OF_MEMORY;      \
        }                                       \
        break;                                  \
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
        /* Let INT8 fall through to INT16 - GetAsInt16 converts cleanly
           with implicit type conversion. DBUS doesn't have an 8-bit signed
           int type, so it will be marshalled as a 16-bit signed int.
         */
        case nsIDataType::VTYPE_INT8:
        MARSHALL_CASE(INT16, Int16)
        MARSHALL_CASE(INT32, Int32)
        MARSHALL_CASE(INT64, Int64)
        case nsIDataType::VTYPE_UINT8:
            PRUint8 val;
            nv = param->GetAsUint8(&val);
            NS_ENSURE_SUCCESS(nv, nv);

            MOZJSDBUS_CALL_OOMCHECK(
                dbus_message_iter_append_basic(it,
                                               DBUS_TYPE_BYTE,
                                               &val));
            break;
        MARSHALL_CASE(UINT16, Uint16)
        MARSHALL_CASE(UINT32, Uint32)
        MARSHALL_CASE(UINT64, Uint64)
        case nsIDataType::VTYPE_FLOAT:
        case nsIDataType::VTYPE_DOUBLE: {
            /* GetAsDouble for a VTYPE_FLOAT uses C++ implicit type
               conversion for the underlying float value, which is
               promotion from a lower-precision to a higher-precision 
               type, so this *should* be OK.
             */
            double val;
            nv = param->GetAsDouble(&val);
            NS_ENSURE_SUCCESS(nv, nv);
            
            MOZJSDBUS_CALL_OOMCHECK(
                dbus_message_iter_append_basic(it,
                                               DBUS_TYPE_DOUBLE,
                                               &val));
            break;
        }
        case nsIDataType::VTYPE_BOOL: {
            PRBool val;
            nv = param->GetAsBool(&val);
            NS_ENSURE_SUCCESS(nv, nv);
            
            MOZJSDBUS_CALL_OOMCHECK(
                dbus_message_iter_append_basic(it,
                                               DBUS_TYPE_BOOLEAN,
                                               &val));
            break;
        }
        case nsIDataType::VTYPE_VOID: {
            break;
        }
        case nsIDataType::VTYPE_ID: {
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

            nv = param->GetAsInterface(&iid, (void **) &interface);
            NS_ENSURE_SUCCESS(nv, nv);

            nsCOMPtr<nsIXPConnectWrappedJS> wrapped =
                do_QueryInterface(interface);

            if (wrapped != NULL) {
                printf ("Info was something !!\n");

                nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(),
                                                         &rv));
                NS_ENSURE_SUCCESS(rv, rv);

                nsAXPCNativeCallContext* ncc = nsnull;
                rv = xpc->GetCurrentNativeCallContext(&ncc);
                NS_ENSURE_SUCCESS(rv, rv);

                JSContext* cx = nsnull;
                rv = ncc->GetJSContext(&cx);
                NS_ENSURE_SUCCESS(rv, rv);

                {
                JSAutoRequest ar(cx);

                JSObject* jsObject;
                rv = wrapped->GetJSObject(&jsObject);
                NS_ENSURE_SUCCESS(rv, rv);
                 
                rv = marshallJSObject(cx, jsObject, it);
                NS_ENSURE_SUCCESS(rv, rv);
                } //for the autorequest
            } else {
                printf ("Info was null !!\n");
            }

            /*DBusMessageIter sub;
            dbus_message_iter_open_container(it, DBUS_TYPE_ARRAY,
                                             "{sv}", &sub);
                                             dbus_message_iter_close_container(it, &sub);*/
            
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
            NS_ENSURE_SUCCESS(nv, nv);

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
                     ptr = *((nsIVariant**)data + i))
                {
                    nv = marshallVariant(msg, ptr, &sub);
                    NS_ENSURE_SUCCESS(nv, nv);
                }

                dbus_message_iter_close_container(it, &sub);
                break;
            }

            int dbus_type = getDataTypeAsDBusType(type);
            int data_type_size = getDataTypeSize(type);
            const char* sign = getDBusTypeAsSignature(dbus_type);
            unsigned char *ptr;
            PRUnichar* c_ptr;

            dbus_message_iter_open_container(it, 
                                             DBUS_TYPE_ARRAY, sign, &sub);

            if (type == nsIDataType::VTYPE_WCHAR_STR) {
                c_ptr = *((PRUnichar**)data);
            } else {
                ptr = (unsigned char *) data;
            }

            for (PRUint32 i = 0; i < count; i++) {
                if (type == nsIDataType::VTYPE_WCHAR_STR) {
                    nsCAutoString utf8 = PromiseFlatCString(
                                             NS_ConvertUTF16toUTF8(
                                                 nsDependentString(c_ptr)));
                    const char* buf = utf8.get();

                    MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_append_basic(&sub, 
                                                       dbus_type, 
                                                       &buf));
                    c_ptr = *((PRUnichar**)data + i + 1);
                } else {
                    MOZJSDBUS_CALL_OOMCHECK(
                        dbus_message_iter_append_basic(&sub, 
                                                       dbus_type,
                                                       ptr));
                    ptr += data_type_size;  
                }
            }

            dbus_message_iter_close_container(it, &sub);
            break;
        }
        case nsIDataType::VTYPE_CHAR:
        case nsIDataType::VTYPE_CHAR_STR:
        case nsIDataType::VTYPE_CSTRING:
        case nsIDataType::VTYPE_STRING_SIZE_IS: {
            /* All based on single-width character buffers. */
            char *cstr;
            nv = param->GetAsString(&cstr); //Hmm, guaranteed null-term?
            NS_ENSURE_SUCCESS(nv, nv);
        
            MOZJSDBUS_CALL_OOMCHECK(
                dbus_message_iter_append_basic(it, 
                                               DBUS_TYPE_STRING, 
                                               &cstr));
            break;
        }
        case nsIDataType::VTYPE_ASTRING:
        case nsIDataType::VTYPE_DOMSTRING:
        case nsIDataType::VTYPE_UTF8STRING:
        case nsIDataType::VTYPE_WCHAR:
        case nsIDataType::VTYPE_WCHAR_STR:
        case nsIDataType::VTYPE_WSTRING_SIZE_IS: {
            /* For VTYPE_UTF8STRING this actually involves conversion
               to UTF-16 in the GetAsAString() call and then again
               back to UTF-8. I doubt the performance suffers much.
            */
            nsAutoString utf16str;
            nv = param->GetAsAString(utf16str);
            NS_ENSURE_SUCCESS(nv, nv);

            nsCAutoString utf8str = PromiseFlatCString(
                                        NS_ConvertUTF16toUTF8(utf16str));
            const char *utf8strbuf = utf8str.get();
        
            MOZJSDBUS_CALL_OOMCHECK(
                dbus_message_iter_append_basic(it, 
                                               DBUS_TYPE_STRING, 
                                               &utf8strbuf));
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
            printf("Unknown DataType.\n");
            return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


PRBool
MozJSDBusMarshalling::JSObjectHasVariantValues(JSContext* cx,
                                               JSObject* aObj)
{
    //Always use "a{sv}" as the signature for now
    return PR_TRUE;
}

nsresult
MozJSDBusMarshalling::marshallJSObject(JSContext* cx,
                                       JSObject*  aObj,
                                       DBusMessageIter* iter)
{
    DBusMessageIter container_iter;
    nsresult rv;

    PRBool hasVariantValues = JSObjectHasVariantValues(cx, aObj);

    MOZJSDBUS_CALL_OOMCHECK(
        dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                                         DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                         DBUS_TYPE_STRING_AS_STRING
                                         DBUS_TYPE_VARIANT_AS_STRING
                                         DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                         &container_iter));

    JSObject* js_iter = JS_NewPropertyIterator(cx, aObj);
    jsid propid;
    if (!JS_NextProperty(cx, js_iter, &propid)) {
        return NS_ERROR_FAILURE;
    }

    while (propid != JSVAL_VOID) {
        jsval propval;
        if (!JS_IdToValue(cx, propid, &propval))
            return NS_ERROR_FAILURE;

        rv = marshallJSProperty(cx, aObj, propval, &container_iter, 
                                hasVariantValues);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!JS_NextProperty(cx, js_iter, &propid)) {
            return NS_ERROR_FAILURE;
        }
    }

    dbus_message_iter_close_container(iter, &container_iter);
    printf("marshalled object\n");
    return NS_OK;
}

nsresult
MozJSDBusMarshalling::marshallJSProperty(JSContext* cx,
                                         JSObject* aObj,
                                         jsval& propval,
                                         DBusMessageIter* iter,
                                         const PRBool& isVariant)
{
    DBusMessageIter dictentry_iter;
    DBusMessageIter variant_iter;
    printf("marshalljsproperty\n");
    JSString* propname = JS_ValueToString(cx, propval);
    JS_GetUCProperty(cx, aObj, JS_GetStringChars(propname),
                     JS_GetStringLength(propname), &propval);

    MOZJSDBUS_CALL_OOMCHECK(
        dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY,
                                         NULL, &dictentry_iter));

    nsDependentString propdepstring(JS_GetStringChars(propname),
                                    JS_GetStringLength(propname));
    nsCAutoString propstring = 
        PromiseFlatCString(NS_ConvertUTF16toUTF8(propdepstring));

    const char* propstringbuf = propstring.get();
    printf("* PROPERTY: %s\n", propstringbuf);

    MOZJSDBUS_CALL_OOMCHECK(
        dbus_message_iter_append_basic(&dictentry_iter,
                                       DBUS_TYPE_STRING,
                                       &propstringbuf));

    JSType type = JS_TypeOfValue(cx, propval);
    switch (type) {
        case JSTYPE_NUMBER:
        {
            /*Impossible to tell what DBus type JSTYPE_NUMBER
              corresponds to. Assume INT32 for now.
             */
            PRInt32 intVal;
            JS_ValueToInt32(cx, propval, &intVal);
            printf("  VALUE: %d\n", intVal);
            if (isVariant) {
                MOZJSDBUS_CALL_OOMCHECK(
                    dbus_message_iter_open_container(
                        &dictentry_iter, 
                        DBUS_TYPE_VARIANT,
                        DBUS_TYPE_INT32_AS_STRING,
                        &variant_iter));
                MOZJSDBUS_CALL_OOMCHECK(
                    dbus_message_iter_append_basic(&variant_iter,
                                                   DBUS_TYPE_INT32,
                                                   &intVal));
                dbus_message_iter_close_container(&dictentry_iter,
                                                  &variant_iter);
            } else {
                MOZJSDBUS_CALL_OOMCHECK(
                    dbus_message_iter_append_basic(&dictentry_iter,
                                                   DBUS_TYPE_INT32,
                                                   &intVal));
            }
            break;
        }
        case JSTYPE_FUNCTION:
        case JSTYPE_STRING:
        {
            //Treat functions as strings for now.
            JSString* propvaljsstring = JS_ValueToString(cx, propval);
            nsDependentString propvaldepstring(
                JS_GetStringChars(propvaljsstring),
                JS_GetStringLength(propvaljsstring));
            nsCAutoString propvalstring = 
                PromiseFlatCString(NS_ConvertUTF16toUTF8(propvaldepstring));

            const char* propvalstringbuf = propvalstring.get();
            printf("  VALUE: %s\n", propvalstringbuf);
            if (isVariant) {
                MOZJSDBUS_CALL_OOMCHECK(
                    dbus_message_iter_open_container(
                        &dictentry_iter, 
                        DBUS_TYPE_VARIANT,
                        DBUS_TYPE_STRING_AS_STRING,
                        &variant_iter));
                MOZJSDBUS_CALL_OOMCHECK(
                    dbus_message_iter_append_basic(&variant_iter,
                                                   DBUS_TYPE_STRING,
                                                   &propvalstringbuf));
                dbus_message_iter_close_container(&dictentry_iter,
                                                  &variant_iter);
            } else {
                MOZJSDBUS_CALL_OOMCHECK(
                    dbus_message_iter_append_basic(&dictentry_iter,
                                                   DBUS_TYPE_STRING,
                                                   &propvalstringbuf));
            }
            break;
        }
        case JSTYPE_BOOLEAN:
        {
            JSBool propbool;
            JS_ValueToBoolean(cx, propval, &propbool);
            printf("  VALUE: %s\n", propbool ? "true" : "false");
            if (isVariant) {
                MOZJSDBUS_CALL_OOMCHECK(
                    dbus_message_iter_open_container(
                        &dictentry_iter, 
                        DBUS_TYPE_VARIANT,
                        DBUS_TYPE_BOOLEAN_AS_STRING,
                        &variant_iter));
                MOZJSDBUS_CALL_OOMCHECK(
                    dbus_message_iter_append_basic(&variant_iter,
                                                   DBUS_TYPE_BOOLEAN,
                                                   &propbool));
                dbus_message_iter_close_container(&dictentry_iter,
                                                  &variant_iter);
            } else {
                MOZJSDBUS_CALL_OOMCHECK(
                    dbus_message_iter_append_basic(&dictentry_iter,
                                                   DBUS_TYPE_BOOLEAN,
                                                   &propbool));
            }
            break;
        }
        case JSTYPE_OBJECT:
        {
            /*JSObject* childObject;
            JS_ValueToObject(cx, propval, &childObject);
            nsresult rv = marshallJSObject(cx, childObject, &dictentry_iter);
            NS_ENSURE_SUCCESS(rv, rv);
            break;*/
        }
        case JSTYPE_VOID:
        default:
            return NS_ERROR_FAILURE;
    }
    dbus_message_iter_close_container(iter, &dictentry_iter);
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
        default:
           printf("Unknown or unsupported nsIDataType: %d.\n", type);
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
        default:
            printf("Unknown or unsupported nsIDataType: %d.\n", type);
    }
    return _ret;
}
