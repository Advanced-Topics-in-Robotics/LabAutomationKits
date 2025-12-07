import math
import typing
import requests
import time

target = [121, 28, 38] # 4 ml red, 2ml water


def growAndMeasure(individual,generation,indvNumber): #Call tile functions here

    print(generation,indvNumber)

    print("Fill phase") # Fill water so that sensor can measure color properly
    json_data = {
        "id": "string",
        "pumpA": {
            "state": True,
            "speed": 10000,
            "dir": False,
            "time": 20000
        }
    }


    waitForDevice()
    print("Sending fill command...")
    print("Individual", individual)
    response_pumps = requests.post("http://127.0.0.1:8000/actions", json=json_data).json()


    json_data = {
        "id": "string",
        "pumpA": {
            "state": True,
            "speed": 10000,
            "dir": False,
            "time": int(1050 * individual[0])
        },
        "pumpB": {
            "state": True,
            "speed": 10000,
            "dir": False,
            "time": int(1050 * individual[1])
        }
    }


    waitForDevice()
    print("Sending grow command...")
    response_pumps = requests.post("http://127.0.0.1:8000/actions", json=json_data).json()

    print("Response:", response_pumps)

    # print("Evaluating: ",grownIndividual)
    #overallSum = sum(grownIndividual)

    waitForDevice()
    print("Requesting color measurement...")
    response = requests.get("http://127.0.0.1:8000/get_color").json()

    measure = response['color']

    print(measure)

    print("Cleaning cycle")
    requests.post("http://127.0.0.1:8000/cleaning_cycle", json=json_data).json()
    print("Cleaned")

    return measure

def evaluate(individual,generation,indvNumber):

      measure = growAndMeasure(individual,generation,indvNumber) #Call a function to start the tiles and return a sensor value

      distances  = [10000] * len(target)

      for i in range(len(distances)):
        distances[i] = target[i]-measure[i]
        distances[i] = distances[i]*distances[i]

      errorSum = sum(distances)
      meanSquaredError = errorSum/(len(target))

      return meanSquaredError

def waitForDevice():
    while True:
        status_response = requests.get("http://127.0.0.1:8000/status").json()
        print("Device busy:", status_response['busy'])
        if not status_response['busy']:
            break
        time.sleep(0.5)
