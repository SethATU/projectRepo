const socket = io();

socket.on("connect", () => { console.log("Connected to server");});

socket.on("mqtt_message", (data) => {
    console.log("MQTT Data:", data);
    const topic = data.topic;
    const payload = parseFloat(data.payload) || data.payload;

    switch(topic) {
        case "esp32/alarm":
            document.getElementById("alarm").innerHTML = payload;
            break;

        case "esp32/movement":
            document.getElementById("movement").innerHTML = payload;
            break;

        case "esp32/user":
            document.getElementById("user").innerHTML = "User: " + payload;
            break;

        case "esp32/key":
            document.getElementById("key").innerHTML = "Key: " + payload;
            break;

        default:
            console.log("Unknown topic:", topic);
    }
});