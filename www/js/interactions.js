
/*
  The SPA interaction contains the JavaScript UI management and uses
  connection.js (global object ws) for server interaction.
*/

$(document).ready(function(){

    /* The binary message types sent over the WebSocket connection
     */
    const BinMsg = Object.freeze({
        // Send a file chunk to the server
        "Upload" : 1,
        // Send a file chunk to the server and signal that this is the last chunk
        "UploadEOF" : 2
    });



    /* BEGIN: modal dialog management
     */

    /* Show modal dialog with 'id'. See the HTML for all ids.
     */
    const showDialog = (id) => {
        $("#modal > div").hide(); // Hide all dialogs
        $("#"+id).show(); // Show only requested dialog
        $("#modal").removeClass("closed"); // Enable modal mode
    };

    /* Disable modal mode by adding class 'closed' to the div #modal.
       Called after successful login.
     */
    const closeModal = () => {
        $("#modal").addClass("closed");
    };

    /* Show the general 'info' dialog and insert 'html' into this div.
     */
    const showInfoDialog = (html) => {
        $("#InfoDialog span").html(html);
        showDialog("InfoDialog");
    };

    /* Show the login dialog with optional 'html' (error message)
     */
    const showLoginDialog = (html) => {
        const errElem=$("#LoginDialog span");
        if(html)
            errElem.show().html(html);
        else
            errElem.hide();
        showDialog("LoginDialog");
    }

    /* Insert device name into the login dialog.
     */
    ws.on("server", "devname", (name)=> $("#device-name").html(name));

    let nonce; // Nonce sent by server; used by authentication management.

    ws.on("server", "nonce", (n) => {
        showLoginDialog(nonce ? 'Incorrect credentials!' : null);
        nonce=atob(n); // Convert from B64 encoding to binary
    });

    $("#LoginDialog button").on("click", () => {
        const uname=$("#LoginUname").val().trim();
        const pwd=$("#LoginPwd").val().trim();
        var sha = new jsSHA("SHA-1", "TEXT");
        sha.update(pwd);
        hash = sha.getHash("BYTES");
        sha = new jsSHA("SHA-1", "BYTES");
        sha.update(hash);
        sha.update(nonce);
        ws.sendJSON("auth",{name:uname,hash:sha.getHash("HEX")});
    });

    ws.on("close", (willReconnect, emsg) => {
        let html='<h1>Connection lost</h1><p>'+emsg+'</p>';
        html += willReconnect ?
            "<h2>Re-establishing server connection, please wait...</h2>" :
            "<h2>Click the browser's reload button to continue.</h2>";
        showInfoDialog(html);
        nonce=null;
    });

    // Show startup message.
    showInfoDialog(
        "<h1>Establishing server connection</h1><h2>Please wait...</h2>")

    // END: modal dialog management






    /* BEGIN: Calculator
       The calculator shows how to use the AJAX API. See the AJAX call in function 'calc' for details.
       Tutorial: https://makoserver.net/articles/AJAX-over-WebSockets
     */

    let lastop;
    let savednum;
    let calcnum=""; // Current calculator number (string)
    const calcdisp = $(".calc-area"); // The display area

    const calc = (valA,valB) => {
        savednum=null;
        calcnum="";
        let op;
        switch(lastop) {
          case '+': op="add"; break;
          case '-': op="subtract"; break;
          case '*': op="mul"; break;
          case '/': op="div"; break;
        }
        ws.ajax("math/"+op, parseFloat(valA), parseFloat(valB)).
            then((rsp) => {
                savednum = rsp+""
                calcdisp.html(savednum);
            }).
            catch((err) => {
                calcnum="", calcdisp.html("0");
                alert(err);
            });
    };

    // number buttons
    $("#calculator .calc-num div").on("click",function() {
        calcnum += $(this).attr("data-value");
        calcdisp.html(calcnum);
        if(calcnum == "0") calcnum="";
    });

  // math buttons
    $("#calculator .math-funcs div").on("click",function() {
        const op = $(this).attr("data-value");
        switch(op) {
          case '.': 
            if(calcnum % 1 === 0) { // If integer
                calcnum += '.'; // Convert to float
                calcdisp.html(calcnum);
            }
            break;
          case 'C':
            savednum=null;
            calcnum="";
            calcdisp.html("0");
            break;
          case '+':
          case '-':
          case '*':
          case '/':
            if(savednum)
                calc(savednum,calcnum)
            else
                savednum=calcnum;
            calcnum="";
            lastop=op;
            break;
          case '=':
            if(savednum)
                calc(savednum,calcnum)
            break;
          default: console.log("Oops bug");
        }

    });
    // END: Calculator






    /* BEGIN: Leds
     */

    /* Install 'ledinfo' event function.
       The event callback takes the server info and dynamically
       creates the LED HTML by using the LED.js lib.
     */
    ws.on("server", "ledinfo", (msg)=>{
        const leds=msg.leds;
        for(i = 0; i < leds.length; i++) {
            const l=leds[i];
            led.emit(l.name, l.id, l.color, l.on); // Create HTML
        };
        //Clicking an LED button in the browser sends the 'setled' message to the server. 
        led.click((ledID, on) => {
            ws.sendJSON("setled",{id:ledID,on:on});
        });

        closeModal();
        $("#LoginDialog input").val(""); // Security: reset all values
        nonce=null;
    });

    // We update the UI when the server sends a 'setled' message
    ws.on("server", "setled", (msg) => led.set(msg.id, msg.on));

    ws.on("close", led.reset); // Remove all LEDs on close
    // END: Leds






    /* BEGIN: Real Time Data (Thermostat)
     */

    /* Thermostat
      Using: https://canvas-gauges.com
    */
    let thermostat = new LinearGauge({
        renderTo: "ThermCanvas", // See HTML for info on this element
        borderRadius: 20,
        borders: 1,
        barStrokeWidth: 20,
        minorTicks: 10,
        value: 0,
        units: "°C"
    });

    thermostat.draw();

    // Received temp in celsius x 10.
    ws.on("server", "settemp", (temp)=> thermostat.value=(temp / 10).toFixed(2));
    // END: Real Time Data (Thermostat)





    /* BEGIN: Drag'n drop firmware upload.
       Drag'n drop intro: http://apress.jensimmons.com/v5/pro-html5-programming/ch9.html

       The div with id "upload-container" is the Drag'n Drop
       area. However, we attach the dragover and drop event to the
       HTML "body" to prevent the SPA from being replaced by the
       dropped file should the user drop something outside the
       "upload-container" element's boundary. The dropped file is
       simply ignored if we get a drop event outside this area.

       Unfortunately, the various drag/drop events are spurious so we
       need a solution for handling this. The 'dragCntr' variable is
       used to track correct drop behavior and the variable should be
       larger than zero if the drop zone is active.
    */

    let dragCntr=0; // Larger than zero if drop is inside "upload-container"
    let uploading=false; // In progress

    // Set or remove the drag emphasize class 'overlay' on the 'overlay'
    // span element inside the "upload-container" div.
    // Hint, in JS functions can be accessed as obj.func() or obj["func"]()
    const emphasizeDrag=(enable)=> {
        $("#upload-overlay")[enable ? "addClass" : "removeClass"]("drag-over");
        $("#upload-info").html(enable ? "<h1>Drop File!</h1>" :
                               "<h1>Drag n' Drop</h1><h2>Firmware Upload Area</h2>");
    };
    emphasizeDrag(false); // Startup state

    // Attach 'dragenter' and 'dragleave' events on the
    // 'upload-container' div. This construction enables us to
    // visually emphasize the 'upload-container' div by using the
    // 'overlay' class on drag-over.
    $("#upload-container").on("dragenter", () => {
        console.log("dragenter");
        if(uploading) return;
        if(dragCntr < 0) dragCntr=0; //Fix spurious events
        if(++dragCntr == 1)
            emphasizeDrag(true);
    }).on("dragleave", () => {
        console.log("dragleave");
        if(--dragCntr == 0)
            emphasizeDrag(false);
    });


    /*  The upload function is called when the 'drop' event function
        below fires and after validating the 'drop' event.  This
        function and the inner functions implement the flow control as
        explained in the documentation.

       Arg fab: File ArrayBuffer
       https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer
    */
    const upload = (fab) => {

        /* Send a chunk over WebSockets
           fabc: file ArrayBuffer chunk
           eof: boolean true if last chunk
         */
        const sendChunk = (fabc,eof)=> {
            ws.sendBin(eof ? BinMsg.UploadEOF : BinMsg.Upload, fabc);
        };
        console.log(fab.byteLength);
        /* Max TCP payload should be less than 1420 so chunk fits on
           one Ethernet frame. We select 1400 since we also have some
           overhead in WebSockets and SMQ.
        */
        const len=fab.byteLength;
        let mSent=0; // Number of messages sent
        let ix=1400; // fab index
        let done=false;
        let mAck=0; // number of messages acked by server
        const total=len/1400; // Number of message chunks
        uploading=true;
        //Send next file chunk over WebSockets
        const sendNext = ()=> {
            done = ix >= len;
            sendChunk(fab.slice(ix-1400, done ? len : ix),done);
            mSent++;
            ix+=1400;
        };
        const startSending = ()=> {
            while(!done && (mSent - mAck) < 20) sendNext();
        };
        let uploadComplete; // Forward declared function. Set below.
        const onclose = ()=> uploadComplete(true);
        const uploadack = (ackno)=> {
            mAck=ackno;
            if(mSent == mAck && done) {
                uploadComplete();
            }
            else {
                const pc = Math.round(ackno*100/total)+"%";
                $("#progress-slider").css("width", pc);
                $("#upload-percentage").html(pc);
                startSending();
            }
        };
        // Updates UI when upload completes or fails.
        uploadComplete = (failed)=> {
                ws.off("server", "uploadack", uploadack);
                ws.off("close", onclose);
                $("#upload-meter").hide();
            $("#upload-info").html(failed ?
                                   "<h2>Upload aborted, connection lost!</h2>" :
                                  "<h2>Upload complete!</h2>");
            setTimeout(() => {uploading=false;emphasizeDrag(false)}, 2000);
        };
        ws.on("server", "uploadack", uploadack);
        ws.on("close", onclose);

        $("#upload-info").html("");
        $("#upload-meter").show();
        startSending();
    };


    // Attach 'dragover' and 'drop' events on the HTML 'body'. Attaching
    // to the 'body' element enables us to ignore incorrect drag'n
    // drop usage.
    $("body").on("dragover", (e) => e.preventDefault()).on("drop", (e) => {
        console.log("drop");
        e.preventDefault();
        if(uploading) return; // busy with other upload
        if(dragCntr > 0) { // If dropped on the "upload-container" div
            emphasizeDrag(false);
            dragCntr=0; // 'dragleave' not necessarily fired when file dropped.
            if(e.dataTransfer.files.length == 1) { // Accept one dropped file
                const fr = new FileReader();
                fr.addEventListener("loadend", () => upload(fr.result)); // Pass in File ArrayBuffer
                fr.readAsArrayBuffer(e.dataTransfer.files[0]);
                return;
            }
            else
                alert("Dude, you can only upload one firmware file!");
        }
    });

    // END: Drag'n drop firmware upload.





    /* BEGIN: Set New Credentials
     */

    /* New credentials dialog: Show "set new" or "credentials saved".
       See the two sub divs in the HTML for details.
     */
    const setSavedCred = (saved) => {
        $("#credentials-container > :nth-child(1)")[saved ? "hide" : "show"]();
        $("#credentials-container > :nth-child(2)")[saved ? "show" : "hide"]();
    };

    /* Save new credentials button click
     */
    $("#credentials-new").on("click", () => {
        const curUname=$("#curUname").val().trim();
        const curPwd=$("#curPwd").val().trim();
        const newUname=$("#newUname").val().trim();
        const newPwd=$("#newPwd").val().trim();
        if(newUname.length < 3 || newPwd.length < 6) {
            $("#credentials-container span").show().html("New "+
                (newUname.length < 3 ? "username" : "password") + " too short!");
            return;
        }
        $("#credentials-container span").hide();

        ws.ajax("auth/setcredentials", curUname, curPwd, newUname, newPwd).
            then(() => {
                setSavedCred(true);
                $("#credentials-container input").val(""); // Security: reset all values
            }).
            catch((err) => {
                $("#credentials-container span").show().html(err);
            });
    });

    /* Credentials saved button click
     */
    $("#credentials-saved").on("click", () => {
        setSavedCred(false);
    });


    // END: Credentials






    /* BEGIN: menu management
     */

    var activePageId="calculator";

    $("#pages > div").hide(); //All pages are hidden by default
    $("#calculator").show(); //Show the first menu item

    // Used by the desktop and mobile menu click handlers below.
    const menuClick = (self) => { // 'self' is 'this' object for clicked 'a' element
        const ix=$(self).index(); // 'a' elements index in menu
        // Remove class 'selected' from all and then add class
        // 'selected' to the selected menu item. Note get() returns DOM
        // element and not JQuery element
        $("#desktop-menu > a").removeClass("selected").get(ix).classList.add("selected");
        $("#mobile-menu > a").removeClass("selected").get(ix).classList.add("selected");

        //Show page identified by 'PageId'. See menu in HTML for details.
        activePageId=$(self).attr("PageId");
        $("#"+activePageId).show();
    };

    // When desktop menu clicked, hide all pages and show page activated by menu
    $("#desktop-menu > a").on("click",function() {
        $("#pages > div").hide();
        menuClick(this);
        return false;
    });

    //When mobile menu clicked, hide mobile menu and show page activated by menu
    $("#mobile-menu > a").on("click",function() {
        $("#menu .drop-area .drop-menu").hide();
        menuClick(this);
        return false;
    });

    //When mobile button clicked, hide all pages and show mobile menu
    $("#drop-button").on("click",function() {
        $("#pages > div").hide();
        $("#menu .drop-area .drop-menu").show();
    });

    // END: menu management

});
