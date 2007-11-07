
EXPORTED_SYMBOLS = ["ProxyUtils"];

var ProxyUtils = {
	getProxyOnUIThread: function (aObject, aInterface) {
	    var eventQSvc = Components.
		    classes["@mozilla.org/event-queue-service;1"].
		    getService(Components.interfaces.nsIEventQueueService);
	    var uiQueue = eventQSvc.
		    getSpecialEventQueue(Components.interfaces.
		    nsIEventQueueService.UI_THREAD_EVENT_QUEUE);
	    var proxyMgr = Components.
		    classes["@mozilla.org/xpcomproxy;1"].
		    getService(Components.interfaces.nsIProxyObjectManager);

	    return proxyMgr.getProxyForObject(uiQueue,
		    aInterface, aObject, 5);
	    // 5 == PROXY_ALWAYS | PROXY_SYNC
	}
};
