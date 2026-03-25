const socket = io();

socket.on("connect", () => { console.log("Connected to server");});

socket.on("mqtt_message", (data) => {
    console.log("MQTT Data:", data);
    const topic = data.topic;
    const payload = parseFloat(data.payload) || data.payload;

    let ALARM, MOVE, USER, KEY;

    switch(topic) {
        case "seth/esp32/alarm":
            if(payload == 0) { ALARM = "Disarmed"; }
            else { ALARM = "Active"; }

            document.getElementById("alarm").innerHTML = ALARM;
            break;

        case "seth/esp32/movement":
            if(payload == 0) { MOVE = "No Movment"; }
            else { MOVE = "Movment"; }

            document.getElementById("movement").innerHTML = MOVE;
            break;

        case "seth/esp32/user":
            if(payload == 2) { USER = "Fob"; }
            else if(payload == 1) { USER = "Card"; }
            else { USER = "Unknown"; }

            document.getElementById("user").innerHTML = USER;
            break;

        case "seth/esp32/key":
            if(payload == 2) { KEY = "Seth F"; }
            else if(payload == 1) { KEY = "Seth K"; }
            else { KEY = "Unknown"; }

            document.getElementById("key").innerHTML = KEY;
            break;

        default:
            console.log("Unknown topic:", topic);
    }
});