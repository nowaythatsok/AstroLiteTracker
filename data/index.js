var buttons=[
            "trackerOn", "trackerOff", 
            "deg0.02", "deg0.1", "deg0.5", "deg1", "deg10", "trackerSTOP",
            "reloadSettings", "saveSettings", 
            "stepCounterReset", "stepCounterSTOP", "stepCounterStep"
        ];
var websocket;
var stepCounterCount=0;

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

    // set callbacks for controls

    ////////////////////////////////////////////////////////////

    document.getElementById("trackerOn").addEventListener("click", function() {
        if (websocket.readyState == 1){
            document.getElementById("trackerOn").disabled = true;
            document.getElementById("trackerOff").disabled = false;
            document.getElementById("trackerState").innerHTML="ON";
            doSend(JSON.stringify({type:"trackerOn"}));
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
            document.getElementById("trackerState").innerHTML="Busy";

            console.log(degrees[i] + " callback, pos: " + str(positive));
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
            let message = JSON.stringify({
            startupTone: document.getElementById("startupTone").checked,
            holdOn: document.getElementById("holdOn").checked,
            sleepOn: document.getElementById("sleepOn").checked,
            sleepLength: document.getElementById("sleepLength").value,
            nFullSteps: document.getElementById("nFullSteps").checked,
            type: "settings"
            })
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
    wsConnect(url);
}

function wsConnect(url){
    console.log("Connecting to websocket");
    // var gateway = `ws://${window.location.hostname}:1337/`;
    var gateway = `ws://${window.location.hostname}/ws`;
    websocket = new WebSocket(gateway);

    websocket.onopen    = function(evt) { onOpen(evt) };
    websocket.onclose   = function(evt) { onClose(evt) };
    websocket.onmessage = function(evt) { onMessage(evt) };
    websocket.onerror   = function(evt) { onError(evt) };

}

function onOpen(evt) {
    console.log("Connected");
    document.getElementById("trackerState").innerHTML = "Connected";
    doSend(JSON.stringify({ type: "fullState" }));

    for (let i = 0; i < buttons.length; i++) {
        if (buttons[i] == "trackerOn" || buttons[i] == "trackerOff") continue;
        document.getElementById(buttons[i]).disabled = false;
    }  
}

function onClose(evt) {
    console.log("Disconnected");
    document.getElementById("trackerState").innerHTML = "Disconnected";
    setTimeout(function() { wsConnect(url) }, 1000);
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
    console.log("Received: " + evt.data.type + " message");
    
    // Update DOM
    switch(evt.data.type) {
        case "fullState":
            document.getElementById("startupTone").checked    = evt.data.startupTone
            document.getElementById("holdOn").checked         = evt.data.holdOn
            document.getElementById("sleepOn").checked        = evt.data.sleepOn
            document.getElementById("sleepLength").value      = evt.data.sleepLength
            document.getElementById("nFullSteps").checked     = evt.data.nFullSteps

            document.getElementById("trackerState").innerHTML = evt.data.trackerState
            if (evt.data.trackerState=="ON"){
                document.getElementById("trackerOn").disabled = true;
                document.getElementById("trackerOff").disabled = false;
            }
            else if (evt.data.trackerState=="OFF"){
                document.getElementById("trackerOff").disabled = true;
                document.getElementById("trackerOn").disabled = false;
            }
            else{
                document.getElementById("trackerOff").disabled = true;
                document.getElementById("trackerOn").disabled = true;
            }
                    
            break;
        case "trackerState":
            document.getElementById("trackerState").innerHTML = evt.data.trackerState
            if (evt.data.trackerState=="ON"){
                document.getElementById("trackerOn").disabled = true;
                document.getElementById("trackerOff").disabled = false;
            }
            else if (evt.data.trackerState=="OFF"){
                document.getElementById("trackerOff").disabled = true;
                document.getElementById("trackerOn").disabled = false;
            }
            else{
                document.getElementById("trackerOff").disabled = true;
                document.getElementById("trackerOn").disabled = true;
            }
            break;
        default:
            break;
    }
}


// Call the init function as soon as the page loads
window.addEventListener("load", init, false);