/* The global object 'ws' is used for all asynch
    communication, AJAX, and state management.

    function ws.status(): Returns one of "closed", "open", "cannot",
    where "cannot" means the WebSocket connection cannot connect.

    function ws.on(event [,messagename], handler): Attach an event handler
    function for one or more events.
    Events:
     "open": Trigger event when WebSocket/SMQ connection is ready.
     "close": Trigger event when webSocket/SMQ connection goes down. The
              'close' event handler receives two arguments: a boolean
              value and reason why the connection was closed
              (string). The value is false if using SMQ and true if
              using a direct WebSocket connection. If the value is
              true, the connection will automatically try to reconnect
              and will loop indefinitely until connection is
              successful. An SMQ connection cannot automatically
              re-connect since the ephemeral SMQ device URL is
              calculated from the "window.location.search" argument.
     "server": Attach a function to be triggered when a message with
               the name provided by the 'messagename' argument is
               received from the server. The messagename (string) must
               be one of the messages defined in the message exchange
               specification. See also ws.sendJSON.

    function ws.off(event [,messagename], handler): Remove an event handler
    installed with ws.on. This function takes the exact same arguments
    as ws.on

    ws.sendJSON(messagename, payload): Send a JSON message to the
    server. The messagename (string) and payload
    (array/object/primitive-type) must be one of the messages defined
    in the message exchange specification.

    function ws.sendBin(msgID,data): Send a binary message to the
    server. The msgID (number) must be one of the messages defined in
    ' BinMsg' (see interactions.js).


  */
const ws={status:()=>{return "closed"}};

(function() {

    /* The URL used during development (ref-dev)
     */
    const devurl="ws://device";
    //const devurl="wss://device";

    let sock; // The WebSocket object

    const lg=console.log;// Alias
    const err = (msg) => {
        lg("Err: "+msg);
        if(sock) {
            sock.onclose=undefined;
            sock.close();
        }
    };

    // Event callback list
    const evcb={
        open:[], //List of functions to call when connected
        close:[], //List of functions to call when connection goes down
        server:{}, // key is 'JSON message name', val is a list of functions to call
    };

    //Can trigger 'open and 'close' events
    const triggerEV = (evn,...args) => {
        const l=evcb[evn]
        for(const i in l) l[i](...args); // Call all registered functions
    };

    ws.on = (evn,x1,x2)=> {
        if(evcb[evn]) {
            if(evn == "server" && typeof x1 == "string" && typeof x2 == "function") {
                if(evcb.server[x1])
                    evcb.server[x1].push(x2);
                else
                    evcb.server[x1]=[x2]
                return;
            }
            if(evcb[evn].push && typeof x1 == "function" ) {
                evcb[evn].push(x1);
                return;
            }
        }
        throw("ws.on: incorrect use");
    };

    ws.off = (evn,x1,x2)=> {
        if(evcb[evn]) {
            const findAndRem = (l, func)=> {
                if(!l) return false;
                for(const i in l) {
                    if(l[i] == func) {
                        l.splice(i,1);
                        return true;
                    }
                }
                return false;
            };
            if(evn == "server" && typeof x1 == "string") {
                if(findAndRem(evcb.server[x1], x2))
                    return;
            }
            else if(findAndRem(evcb[evn], x1))
                return;
        }
        throw("ws.off: incorrect use");
    };

    let sock_sendBin; // We set this function later
    ws.sendBin = (msgID,data)=> {
        const payload = new ArrayBuffer(data.byteLength+1);
        const u8a = new Uint8Array(payload);
        u8a[0]= msgID;
        u8a.set(new Uint8Array(data),1);
        sock_sendBin(u8a);
    };

    ws.sendJSON = (msg,data)=> {
        sock.send(JSON.stringify([msg,data]));
    };

    /* function wsurl([checkIfFile])

       Function wsurl returns a WebSocket URL pointing to 'origin
       server' if the page is loaded via a server, otherwise, the
       function returns 'devurl' (ref-dev). The 'devurl' is returned
       if the web page is simply dropped into a browser window.

       The function can optionally check if the file is loaded locally
       or via a server by setting the optional argument 'checkIfFile'
       to true. The function will then return 'true' if loaded
       locally, otherwise false is returned.
     */
     const wsurl = (checkIfFile)=> {
        const l = window.location;
        if(!l.protocol || l.protocol == "file:")
            return checkIfFile ? true : devurl;
         if(checkIfFile) return false;
        return ((l.protocol === "https:") ? "wss://" : "ws://") +
            l.hostname +
            (l.port!=80 && l.port!=443 && l.port.length!=0 ? ":" + l.port : "") + "/";
    };

    let isopen=false;

    const onopen = ()=> {
        ws.status = ()=> {return "open"}
        isopen=true;
        triggerEV("open");
    };

    const ajaxCallbacks={} // saved AJAX callbacks: key=id, val=function

    /* Copied from the AJAX over WebSocket tutorial and modified to
     * blend with this code. AJAX response is managed in parseMessage().
     */
    ws.ajax = (service, ...args) => {
        return new Promise(function(resolve, reject) {
            var rpcID; // Find a unique ID
            do {
                rpcID=Math.floor(Math.random() * 100000);
            } while(ajaxCallbacks[rpcID]); // while collisions
            // Save promise ajaxCallbacks: rpcID is the key.
            ajaxCallbacks[rpcID]={resolve:resolve,reject:reject};
            // Encapsulate AJAX as an async message.
            ws.sendJSON("AJAX", [service, rpcID, args]);
        });
    };

    const parseMessage = (data) => {
        let j=JSON.parse(data);
        if(j) {
            /* First element in array (i.e. j[0]) should be 'JSON
               message name' We use the 'JSON message name' as a key
               for looking up the list of event handlers attached to
               this message name.
            */
            const l=evcb.server[j[0]];
            if(l) {
                if(l.length > 0) {
                    j=j[1]; // Payload
                    for(const i in l) l[i](j);
                }
            }
            else if(j[0] == 'AJAX') { // AJAX is a reserved name
                try {
                    const ax=j[1]; // ax (AJAX) must be an array
                    var promise=ajaxCallbacks[ax[0]]; // Find the two promise ajaxCallbacks
                    const resp=ax[1]; // Payload
                    if( ! promise || typeof resp != "object")
                        throw("Invalid AJAX response: "+ data);
                    delete ajaxCallbacks[ax[0]]; // Release
                    if(resp.rsp != null) promise.resolve(resp.rsp);
                    else promise.reject(resp.err);
                }
                catch(e) {
                    err("Invalid AJAX response: "+e.toString());
                }
            }
            /* The 'err' call below: we should have at least one
               registered event function for the JSON message,
               otherwise, the message would be lost.
            */
            else
                err("WS server hndl '"+j[0]+"' not registered: ");
        }
        else
            err("WS JSON parse"); // Very unlikely
    };

    // Cleanup in-flight AJAX on close
    const ajaxOnClose = (emsg) => {
        for(const i in ajaxCallbacks)
            try { ajaxCallbacks[i].reject(emsg); } catch(e) {err(e.toString());}
    };

    // Manage WS or SMQ on close
    const onClose = (willReconnect, emsg) => {
        ajaxOnClose(emsg);
        triggerEV("close",willReconnect, emsg);
    };



    /*
      We defer the execution of the following code until all resources
      are loaded. The code checks if SMQ is included by the main HTML
      page. The code sets up an SMQ connection If SMQ is included in
      the HTML page and the page is loaded from a server and not from
      local file system.

      A WebSocket connection is setup if SMQ is not included. The
      WebSocket connection is to 'origin server' if the page is loaded
      via a web server. The WebSocket connection is set to the content
      of variable ' devurl' if the page is loaded from the local file
      system.

     */

    window.addEventListener('load', function() {
        sock_sendBin = (data)=> { sock.send(data); };
        const hasSMQ = () => { try {return SMQ;} catch(e) {return null;} };
         // Use SMQ if not loaded from file and SMQ is included
        if( ! wsurl(true) && hasSMQ() ) {
            lg("SMQ URL:"+SMQ.wsURL("/minnow-smq.lsp")+window.location.search)
            let devTid
            const smq = SMQ.Client(SMQ.wsURL("/minnow-smq.lsp")+window.location.search);
            smq.subscribe("self", 1, { // subtid 1 used for JSON messages
                 // We set the dataype to text and not JSON since
                 // parseMessage is set to parse text
                datatype:"text",
                onmsg:(msg, ptid)=> {
                    devTid=ptid;
                    try { parseMessage(msg); }
                    catch(e) { err(e.toString()); }
                }
            });
            smq.subscribe("self", 2, { // subtid 2 is used for binary messages
                onmsg:(msg, ptid)=> {
                    err("Received binary data on SMQ");
                }
            });
            smq.onmsg = ()=> { Err("Received unnexpected SMQ message"); };
            smq.onclose = (msg)=> { onClose(false,"SMQ: "+msg); };
            smq.onconnect = onopen;
            sock = { send:(data)=> {smq.publish(data, devTid, 1);} };
            sock_sendBin = (data)=> {smq.publish(data, devTid, 2);};
        }
        else { // Using a direct WebSocket connection and not SMQ
            //WebSocket receive event
            const onmessage = (m) => {
                try {
                    if(typeof m.data === "string")
                        parseMessage(m.data);
                    /* else: The implementation is currently not designed for
                       handling binary WebSocket frames received from server
                    */
                    else
                        err("WS binary");
                }
                catch(e) {
                    err(e.toString());
                }
            };

            const connect = ()=> {
                sock = new WebSocket(wsurl());
                sock.onopen=onopen;
                sock.onmessage=onmessage;
                sock.onclose=() => {
                    ws.status = isopen ? ()=> {return "closed"} : ()=> {
                        return "cannot"
                    };
                    if(isopen)
                        onClose(true,"WebSocket closed");
                    isopen=false;
                    connect();
                };
            };
            connect();
        };
    });
})();
