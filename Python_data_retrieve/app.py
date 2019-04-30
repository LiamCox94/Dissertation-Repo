###########
#
#LIAM COX 2019
#
###########
import paho.mqtt.client as mqtt
import json
import base64
import hashlib
import ssl
from tkinter import *

APPEUI = "70B3D57ED0014C0D"
APPID  = "encryption_app"
PSW    = 'ttn-account-v2.DLQeQUoSLa19oSK7ILq0KwZ2SbzbigsHMQmpoSuk2Lk'
    
# gives connection message
def on_connect(mqttc, mosq, obj,rc):
    print("Connected with result code:"+str(rc))
    # subscribe for all devices of user
    mqttc.subscribe('+/devices/+/up')

# gives message from device
def on_message(mqttc,obj,msg):
    global convert_hash
    global originalVal
    try:
        x = json.loads(msg.payload.decode('utf-8'))
        device = x["dev_id"]
        counter = x["counter"]
        payload_raw = x["payload_raw"]
        payload_fields = x["payload_fields"]
        datetime = x["metadata"]["time"]
        gateways = x["metadata"]["gateways"]
        # print for every gateway that has received the message and extract RSSI
        for gw in gateways:
            gateway_id = gw["gtw_id"]
            rssi = gw["rssi"]
            data = payload_fields
            print(datetime + ", " + device + ", " + str(counter) + ", "+ gateway_id + ", "+ str(rssi) + ", " + str(payload_fields))
            tempval = str(payload_fields)            
        
        tempval=tempval.replace("{'hashed_Val': '",'')
        tempval=tempval.replace("'}",'')
        tempval=tempval.split(',')
        loop_val=tempval
        index=0
        del tempval[34]
        del tempval[34]
        while index < len(tempval):
               loop_val=int (tempval[index])
               x=hex(loop_val)[2:]
               tempval[index] = x
               if len(x) < 2:
                   tempval[index] = "0"+x
               index += 1

        originalVal = tempval[:]

        for x in range(2,34):
            del originalVal[2]        

        originalVal=''.join(originalVal)
        originalVal = originalVal.replace("3",'',2)
        originalValDisp.set(originalVal)
        generate_hash.set(hashlib.sha256(originalVal.encode('ascii')).hexdigest())

        del tempval[0]
        del tempval[0]
        tempval=''.join(tempval)
        convert_hash.set(tempval)
        p = convert_hash.get() == generate_hash.get()
        if p == 1:
            hash_check.set("True")
        else:
            hash_check.set("False")
    except Exception as e:
        print(e)
        pass

def on_publish(mosq, obj, mid):
    print("mid: " + str(mid))

def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed: " + str(mid) + " " + str(granted_qos))

def on_log(mqttc,obj,level,buf):
    print("message:" + str(buf))
    print("userdata:" + str(obj))


root = Tk()
originalValDisp = StringVar()
originalValDisp.set("##################")
convert_hash = StringVar()
convert_hash.set("##################")
hash_check = StringVar()
hash_check.set("##################")
generate_hash = StringVar()
generate_hash.set("##################")

root.geometry('700x400')

hash_data_tag = Label(root, text="Original Value", bg="pink").pack()
hash_data_box = Entry(root, textvariable = originalValDisp).pack(fill=X)
hash_data_tag = Label(root, text="Python generated Hash", bg="light green").pack()
hash_data_box = Entry(root, textvariable = convert_hash).pack(fill=X)
hash1_data_tag = Label(root, text="LoRaWAN Hash", bg="red").pack()
hash1_data_box = Entry(root, textvariable = generate_hash).pack(fill=X)
hash1_data_tag = Label(root, text="Do they match?", bg="sky blue").pack()
hash1_data_box = Entry(root, textvariable = hash_check).pack(fill=X)


mqttc= mqtt.Client()
mqttc.on_connect=on_connect
mqttc.on_message=on_message
mqttc.tls_set('/home/student/Desktop/mqtt-ca.pem' , tls_version=ssl.PROTOCOL_TLSv1_2)
mqttc.username_pw_set(APPID, PSW)
mqttc.connect("eu.thethings.network",8883,10)#uses encrypted port with certificates 

# and listen to server

run = True
while run:
    mqttc.loop_start()
    root.mainloop()

print("loop ended")
