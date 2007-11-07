#include "nsIGenericFactory.h"
#include "MozJSDBusCoreComponent.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(MozJSDBusCoreComponent)

static nsModuleComponentInfo components[] =
{
    {
       MY_COMPONENT_CLASSNAME, 
       MY_COMPONENT_CID,
       MY_COMPONENT_CONTRACTID,
       MozJSDBusCoreComponentConstructor,
    }
};

NS_IMPL_NSGETMODULE("MozJSDBusCoreComponentModule", components) 

