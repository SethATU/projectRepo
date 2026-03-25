const socket = io();

socket.on("connect", () => { console.log("Connected to server");});

socket.on("mqtt_message", (data) => {
    console.log("MQTT Data:", data);
    const topic = data.topic;
    const payload = parseFloat(data.payload) || data.payload;

    switch(topic) {
        case "seth/esp32/celcius":
            document.getElementById("celcius").innerHTML = payload + "°C";
            break;

        case "seth/esp32/farenheight":
            document.getElementById("farenheight").innerHTML = payload + "°F";
            break;

        case "seth/esp32/humidity":
            document.getElementById("humidity").innerHTML = payload + "%";
            break;

        case "seth/esp32/distance":
            document.getElementById("distance").innerHTML = payload + " cm";
            break;

        default:
            console.log("Unknown topic:", topic);
    }
});
