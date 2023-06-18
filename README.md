HTTP API

```
/api/relay?metod=on&port=(1-4|all) HTTP/1.1 200 Ok
/api/relay?metod=off&port=(1-4|all) HTTP/1.1 200 Ok
/api/relay?metod=status&port=(1-4|all) [{"port": 1,"status": "on"}] [{"port":1,"status":"off"},{"port":2,"status":"on"},{"port":3,"status": "off"},{"port": 4,"status":"off"}]
```
