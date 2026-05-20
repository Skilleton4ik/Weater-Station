import requests

city = "Kamianske"
url = f"http://geocoding-api.open-meteo.com/v1/search?name={city}&count=1&format=json"
print(requests.get(url).text)
