import requests

# GET / HTTP/1.1
# Host: localhost:9999
# User-Agent: python-requests/2.22.0
# Accept-Encoding: gzip, deflate
# Accept: */*
# Connection: keep-alive

url = "http://localhost:9999/"
res = requests.get(url)

print(res)
