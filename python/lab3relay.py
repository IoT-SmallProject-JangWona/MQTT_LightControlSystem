import paho.mqtt.client as mqtt
topic = "deviceid/wonaz/evt/light"
server = "18.209.8.146"

def on_connect(client, userdata, flags, rc):
    print("Connected with RC: :"+str(rc))
    client.subscribe(topic)

def on_message(client, userdata, msg):
    print(msg.topic+" "+msg.payload.decode('UTF-8'))
    if(msg.topic == "deviceid/wonaz/evt/light"):
        light=msg.payload.decode("UTF-8")
        light1=int(light)
        if(light1>=100):
            client.publish("deviceid/wonaz2/cmd/lamp",int(1))
        else:
            client.publish("deviceid/wonaz2/cmd/lamp",int(0))

client=mqtt.Client()
client.connect(server,1883,60)
client.on_connect = on_connect
client.on_message = on_message

client.loop_forever()