var buttons=[
            "trackerOn", "trackerOff", 
            "deg0.02", "deg0.1", "deg0.5", "deg1", "deg10", "trackerSTOP",
            "reloadSettings", "saveSettings", 
            "stepCounterReset", "stepCounterSTOP", "stepCounterStep"
        ];
var rbuttons = ["siderealTr", "lunarTr", "solarTr"];
var websocket;
var stepCounterCount=0;
var trackingMode="sidereal";

// This is called when the page finishes loading
function init() {

    // set all buttons as disabled
    for (let i = 0; i < buttons.length; i++) {
        try {
            document.getElementById(buttons[i]).disabled = true;
        } catch (error) {
            console.error(error);
            console.error(buttons[i]);
        }
        
    }   
    for (let i = 0; i < rbuttons.length; i++) {
        try {
            document.getElementById(rbuttons[i]).disabled = true;
        } catch (error) {
            console.error(error);
            console.error(rbuttons[i]);
        }
        
    }            

    // set callbacks for controls

    ////////////////////////////////////////////////////////////
    function sendTrackerOption(name){
        if (websocket.readyState == 1){
            if (document.getElementById(name).checked){
                trackingMode = name.substring(0, str.length-2);
                doSend(JSON.stringify({type:"trackerOn", trackingMode:trackingMode }));
                console.log("trackerOn callback "+trackingMode);
            }
        }
        else{
            console.error("tracking option selaction not executed, connection is "+str(websocket.readyState));
        }
    }
    for (let i = 0; i < rbuttons.length; i++) document.getElementById(rbuttons[i]).addEventListener("click", 
        // https://stackoverflow.com/questions/30354136/function-call-with-parameters-to-addeventlistener-inside-a-loop
        (function(x){
            return function(){
                sendTrackerOption(x);
            }
        })(rbuttons[i])
    );
        

    document.getElementById("trackerOn").addEventListener("click", function() {
        if (websocket.readyState == 1){
            document.getElementById("trackerOn").disabled = true;
            document.getElementById("trackerOff").disabled = false;
            document.getElementById("trackerState").innerHTML="ON";
            doSend(JSON.stringify({type:"trackerOn", trackingMode:trackingMode}));
            console.log("trackerOn callback");
        }
        else{
            console.error("trackerOn not executed, connection is "+str(websocket.readyState));
        }
    });
    document.getElementById("trackerOff").addEventListener("click", function() {
        if (websocket.readyState == 1){
            document.getElementById("trackerOff").disabled = true;
            document.getElementById("trackerOn").disabled = false;
            document.getElementById("trackerState").innerHTML="OFF";
            doSend(JSON.stringify({type:"trackerOff"}));
            console.log("trackerOff callback");
        }
        else{
            console.error("trackerOff not executed, connection is "+str(websocket.readyState));
        }
    });

    ////////////////////////////////////////////////////////////

    let degrees = ["deg0.02", "deg0.1", "deg0.5", "deg1", "deg10"];
    for (let i = 0; i < degrees.length; i++) {
        document.getElementById(degrees[i]).addEventListener("click", function() {
            if (websocket.readyState == 1){
            let positive = document.getElementById("leftRightBtn").checked;
            let d = parseFloat(degrees[i].slice(3));
            if (!positive) d*=-1;
            doSend(JSON.stringify({type:"stepDegrees", value:d}));

            document.getElementById("trackerOff").disabled = true;
            document.getElementById("trackerOn").disabled = true;

            document.getElementById("siderealTr").disabled = true;
            document.getElementById("lunarTr").disabled = true;
            document.getElementById("solarTr").disabled = true;

            document.getElementById("trackerState").innerHTML="Busy";

            console.log(degrees[i] + " callback, pos: " + positive.toString());
            }
            else{
            console.error("manual shift not executed, connection is "+str(websocket.readyState));
            }
        });
    }
    document.getElementById("trackerSTOP").addEventListener("click", function() {
        if (websocket.readyState == 1){
            document.getElementById("trackerState").innerHTML="OFF";
            doSend(JSON.stringify({type:"trackerSTOP"}));
            console.log("trackerSTOP callback");
        }
        else{
            console.error("trackerSTOP not executed, connection is "+str(websocket.readyState));
        }
    });

    ////////////////////////////////////////////////////////////

    document.getElementById("reloadSettings").addEventListener("click", function() {
        if (websocket.readyState == 1){
            doSend(JSON.stringify({ type: "fullState" }));
            console.log("reloadSettings callback");
        }
        else{
            console.error("reloadSettings not executed, connection is "+str(websocket.readyState));
        }
    });
    document.getElementById("saveSettings").addEventListener("click", function() {
        if (websocket.readyState == 1){
            let response = {
                    startupTone: document.getElementById("startupTone").checked,
                    holdOn: document.getElementById("holdOn").checked,
                    sleepOn: document.getElementById("sleepOn").checked,
                    sleepLength: document.getElementById("sleepLength").value,
                    nFullSteps: document.getElementById("nFullSteps").value,
                    trackingMode:trackingMode,
                    type: "settings"
            }
            if (document.getElementById("ssidWifi").value != "" && document.getElementById("pwdWifi").value != ""){
                response.ssidWifi = document.getElementById("ssidWifi").value;
                response.pwdWifi  = document.getElementById("pwdWifi").value;
            }

            let message = JSON.stringify(response)
            doSend(message);
            console.log("saveSettings callback");
        }
        else{
            console.error("saveSettings not executed, connection is "+str(websocket.readyState));
        }
    });

    ////////////////////////////////////////////////////////////
    document.getElementById("stepCounterReset").addEventListener("click", function() {
        stepCounterCount = 0;
        document.getElementById("stepCounterCount").innerHTML="0";
    });
    document.getElementById("stepCounterSTOP").addEventListener("click", function() {
        if (websocket.readyState == 1){
            document.getElementById("trackerState").innerHTML="OFF";
            doSend(JSON.stringify({type:"trackerSTOP"}));
            console.log("stepCounterSTOP callback");
        }
        else{
            console.error("stepCounterSTOP not executed, connection is "+str(websocket.readyState));
        }
    });
    document.getElementById("stepCounterStep").addEventListener("click", function() {
        if (websocket.readyState == 1){
            
            // turn off normal tracking
            document.getElementById("trackerOff").disabled = true;
            document.getElementById("trackerOn").disabled = true;

            document.getElementById("siderealTr").disabled = true;
            document.getElementById("lunarTr").disabled = true;
            document.getElementById("solarTr").disabled = true;

            document.getElementById("trackerState").innerHTML="Busy";
            doSend(JSON.stringify({type:"trackerOff"}));

            // get increment
            let v = document.getElementById("stepCounterNIncr").value;
            doSend(JSON.stringify({type:"stepSteps", value:v}));
            stepCounterCount += v;
            document.getElementById("stepCounterCount").innerHTML=stepCounterCount;
            console.log("stepCounterStep callback");
        }
        else{
            console.error("stepCounterStep not executed, connection is "+str(websocket.readyState));
        }
    });

    document.getElementById("stepCounterNIncr").value = 100000;
    
    // Connect to WebSocket server
    wsConnect();
}

function wsConnect(){
    console.log("Connecting to websocket");
    let addr = window.location.hostname || "astrolite.local";
    let gateway = `ws://${addr}/ws`;
    console.log("gateway: " + gateway);
    websocket = new WebSocket(gateway);

    websocket.onopen    = function(evt) { onOpen(evt) };
    websocket.onclose   = function(evt) { onClose(evt) };
    websocket.onmessage = function(evt) { onMessage(evt) };
    websocket.onerror   = function(evt) { onError(evt) };

}

function onOpen(evt) {
    console.log("Connected");
    document.getElementById("trackerState").innerHTML = "Connected";
    // now this comes from the server automatically doSend(JSON.stringify({ type: "fullState" }));

    for (let i = 0; i < buttons.length; i++) {
        if (buttons[i] == "trackerOn" || buttons[i] == "trackerOff") continue;
        document.getElementById(buttons[i]).disabled = false;
    }  
}

function onClose(evt) {
    console.log("Disconnected");
    document.getElementById("trackerState").innerHTML = "Disconnected";
    setTimeout(function() { wsConnect() }, 1000);
}

// Called when a WebSocket error occurs
function onError(evt) {
    console.log("WSERROR: " + evt.data);
}

// Sends a message to the server (and prints it to the console)
function doSend(message) {
    console.log("Sending: " + message);
    websocket.send(message);
}

// Called when a message is received from the server
function onMessage(evt) {

    // Print out our received message
    console.log("Received message: " + evt.data );
    let data = JSON.parse(evt.data);
    
    // Update DOM
    switch(data.type) {
        case "fullState":
            console.log("Received fullState")
            document.getElementById("startupTone").checked    = data.startupTone
            document.getElementById("holdOn").checked         = data.holdOn
            document.getElementById("sleepOn").checked        = data.sleepOn
            document.getElementById("sleepLength").value      = data.sleepLength
            document.getElementById("nFullSteps").value       = data.nFullSteps
            document.getElementById("ssidWifi").value         = data.ssidWifi || ""
            document.getElementById("pwdWifi").value          = data.pwdWifi || ""

            // no break!!
        case "trackerState":
            if (data.type == "trackerState") console.log("Received trackerState");

            document.getElementById("siderealTr").checked     = (data.trackingMode == "sidereal")
            document.getElementById("lunarTr").checked        = (data.trackingMode == "lunar")
            document.getElementById("solarTr").checked        = (data.trackingMode == "solar")
            trackingMode = data.trackingMode;

            document.getElementById("trackerState").innerHTML = data.trackerState;
            if (data.trackerState=="ON"){
                document.getElementById("trackerOn").disabled = true;
                document.getElementById("trackerOff").disabled = false;

                document.getElementById("siderealTr").disabled = false;
                document.getElementById("lunarTr").disabled = false;
                document.getElementById("solarTr").disabled = false;
            }
            else if (data.trackerState=="OFF"){
                document.getElementById("trackerOff").disabled = true;
                document.getElementById("trackerOn").disabled = false;

                document.getElementById("siderealTr").disabled = false;
                document.getElementById("lunarTr").disabled = false;
                document.getElementById("solarTr").disabled = false;
            }
            else{
                document.getElementById("trackerOff").disabled = true;
                document.getElementById("trackerOn").disabled = true;

                document.getElementById("siderealTr").disabled = true;
                document.getElementById("lunarTr").disabled = true;
                document.getElementById("solarTr").disabled = true;
            }
            break;
        default:
            break;
    }
}


// Call the init function as soon as the page loads
window.addEventListener("load", init, false);