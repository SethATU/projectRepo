from flask import Flask, render_template
from paho.mqtt import client as mqtt
from flask_socketio import SocketIO

#flask setup
app = Flask(__name__)
socketio = SocketIO(app)

@app.route('/')
def home():
    return render_template('index.html')

@app.route('/data')
def data():
    return render_template('data.html')

@app.route('/alarm')
def alarm():
    return render_template('alarm.html')

@app.route('/location')
def location():
    return render_template('location.html')

@app.route('/stats')
def stats():
    return render_template('stats.html')

#mqtt config
broker = "10.187.190.216"
port = 1883
topic = "esp32/#"

#mqtt 
def on_connect(client, userdata, flags, reasonCode, properties):
    print("Connected with result code", reasonCode)
    client.subscribe(topic)
    
def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    topicName = msg.topic
    print(topicName, payload)
    socketio.emit("mqtt_message", {"topic": topicName, "payload": payload})
    
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message
try:
    client.connect(broker, port, 60)
    client.loop_start()
except Exception as e:
    print(f"Failed to connect to MQTT broker: {e}")
    
if __name__ == '__main__':
    socketio.run(app, debug=True, host='0.0.0.0')
