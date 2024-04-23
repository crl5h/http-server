import threading
import requests

url = "http://localhost:4221/files/test.txt"

def get_file(url,i):
    for _ in range(5):
        requests.get(url)

threads = []
for i in range(5):
    t = threading.Thread(target=get_file, args=(url, i))
    threads.append(t)
    t.start()

for t in threads:
    t.join()
