## Trafic

Sound is sampled at a very high rate. But since the reading is binary we pack it in an int to save some storage and ease network load a bit.

This means that we need to unpack it on the server. This small snippet unpacks the data:

```javascript
var output = [];

for(var i = 0; i < 32; i++){
    output.push([{
        time: msg.payload.time - msg.payload.measureInterval * i,
        value: msg.payload.bitarray >> i & 0x01
    }])
}

msg.payload = output;
return msg;
```
