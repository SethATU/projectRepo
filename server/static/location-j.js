const socket = io();
socket.on("connect", () => { console.log("Connected to server");});

let map;
let marker;
let latPos = 0;
let lonPos = 0;

socket.on("mqtt_message", (data) => {
    console.log("MQTT Data:", data);
    const topic = data.topic;
    const payload = parseFloat(data.payload) || data.payload;

    if (topic === "esp32/latitude") latPos = payload;
    if (topic === "esp32/longitude") lonPos = payload;
    updateMap();
});

(g=>{var h,a,k,p="The Google Maps JavaScript API",c="google",l="importLibrary",q="__ib__",m=document,b=window;b=b[c]||(b[c]={});var d=b.maps||(b.maps={}),r=new Set,e=new URLSearchParams,u=()=>h||(h=new Promise(async(f,n)=>{await (a=m.createElement("script"));e.set("libraries",[...r]+"");for(k in g)e.set(k.replace(/[A-Z]/g,t=>"_"+t[0].toLowerCase()),g[k]);e.set("callback",c+".maps."+q);a.src=`https://maps.${
c}apis.com/maps/api/js?`+e;d[q]=f;a.onerror=()=>h=n(Error(p+" could not load."));a.nonce=m.querySelector("script[nonce]")?.nonce||"";m.head.append(a)}));d[l]?console.warn(p+" only loads once. Ignoring:",g):d[l]=(f,...n)=>r.add(f)&&u().then(()=>d[l](f,...n))})
({key: "", v: "weekly"});
    
async function initMap() {
    const { Map } = await google.maps.importLibrary("maps");
    const { AdvancedMarkerElement } = await google.maps.importLibrary("marker");

    map = new Map(document.getElementById("map"), {
    zoom: 15,
    center: { lat: latPos, lng: lonPos },
    mapId: "9040f5107606389e1876cc8e",
    });

    marker = new AdvancedMarkerElement({
    map,
    position: { lat: latPos, lng: lonPos },
    title: "ESP32 Position",
    });
}
initMap();

function updateMap() {
    if (!map || !marker) return;

    const newPos = { lat: latPos, lng: lonPos };
    map.setCenter(newPos);
    marker.position = newPos;
}