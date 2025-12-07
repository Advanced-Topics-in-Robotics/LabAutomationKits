from fastapi import FastAPI, BackgroundTasks, WebSocket, WebSocketDisconnect, HTTPException
from driver import RealMicrocontrollerService, get_logger

import time
from typing import Set
import json

from action_models import ActionRequest, PumpCommand

import asyncio
import os
import httpx

app = FastAPI()
micro = RealMicrocontrollerService()

log = get_logger(__name__)
busy = False
stop_requested = False

main_event_loop = None

@app.on_event("startup")
async def startup_event():
    global main_event_loop
    main_event_loop = asyncio.get_running_loop()

@app.post("/actions")
async def perform_actions(request: ActionRequest, background_tasks: BackgroundTasks):
    global busy, stop_requested
    if busy:
        return {"status": "busy"}
    busy = True
    stop_requested = False
    # Send 'acknowledged' webhook immediately

    background_tasks.add_task(action_task, request)
    return {"status": "accepted", "id": request.id}

# --- Helper methods ---
def send_command_to_hardware(pumpA, pumpB, pumpC):
    print("pump a state", pumpA.state)
    # Extract pump parameters
    stateA = pumpA.state if pumpA else False
    speedA = pumpA.speed if pumpA else 0
    dirA = pumpA.dir if pumpA else True
    durationA = pumpA.time if pumpA else 0

    stateB = pumpB.state if pumpB else False
    speedB = pumpB.speed if pumpB else 0
    dirB = pumpB.dir if pumpB else True
    durationB = pumpB.time if pumpB else 0

    stateC = pumpC.state if pumpC else False
    speedC = pumpC.speed if pumpC else 0
    dirC = pumpC.dir if pumpC else True
    durationC = pumpC.time if pumpC else 0


    # Send command to hardware
    micro.set_state(
        stateA, speedA, dirA, durationA,
        stateB, speedB, dirB, durationB,
        stateC, speedC, dirC, durationC,
    )

    return max(durationA, durationB, durationC)

def handle_stop(job_id):
    """Handles stopping everything, fetching/logging/streaming state, and sending notifications."""
    micro.stopPumps()  # Unified stop for all motors (pumps and mixer)
    try:
        current_state = micro.getState()
        log.info(f"Fetched state after stopping everything: {current_state}")
        state_msg = json.dumps({
            "event": "state_update",
            "state": current_state,
            "timestamp": time.time()
        })
    except Exception as e:
        log.error(f"Error fetching state after stopping everything: {e}")

def monitor_operations(job_id, pumpA, pumpB, pumpC, step_time):
    global busy, stop_requested
    pump_done = False if (pumpA) else True
    start_time = time.time()

    interval = 0.1

    while not (pump_done):
        now = time.time()
        elapsed = now - start_time
        if stop_requested:
            busy = False
            log.info("monitor_operations: busy set to False after stop_requested")
            # Send 'stopped' webhook
            return  # Exit the function gracefully after handling stop
        # --- HARD TIMEOUT ---
        if elapsed >= ((step_time / 1000)+1):
            log.info("monitor_operations: hard timeout reached, forcing completion")
            if not pump_done:
                pump_done = True
            break
        # --- NORMAL COMPLETION ---
        if not pump_done and micro.check_for_step_done():
            pump_done = True
        time.sleep(interval)

    busy = False
    log.info("monitor_operations: busy set to False after completion")

# --- Unified background task ---
def action_task(request: ActionRequest):
    job_id = request.id
    pumpA = request.pumpA
    pumpB = request.pumpB
    pumpC = request.pumpC
      # Send motor command
    step_time = send_command_to_hardware(pumpA, pumpB, pumpC)
    # Monitor operation
    monitor_operations(job_id, pumpA, pumpB, pumpC, step_time)

@app.post("/stop")
async def emergency_stop():
    global stop_requested, busy
    stop_requested = True
    handle_stop("emergency_stop")
    busy = False  # Fallback: ensure busy is reset if stop is called
    log.info("/stop called: busy set to False")
    return {"status": "stopped"}


@app.get("/status")
def get_status():
    global busy
    return {"busy": busy}

@app.get("/get_color")
def get_color():
    # check if busy
    if busy:
        raise HTTPException(status_code=409, detail="Device is busy")

    log.info("Getting color from microcontroller")
    micro.readColor()
    color = micro.receiveColor()

    return {"color": color}

@app.post("/cleaning_cycle")
def start_cleaning_cycle(background_tasks: BackgroundTasks):
    global busy, stop_requested
    if busy:
        return {"status": "busy"}
    busy = True
    stop_requested = False

    pumpA = PumpCommand(state=False, speed=2000, dir=False, time=5000)
    pumbB = PumpCommand(state=False, speed=0, dir=False, time=0)
    pumpC = PumpCommand(state=True, speed=10000, dir=True, time=40000)
    request = ActionRequest(
        id="cleaning_cycle",
        pumpA=pumpA,
        pumpB=pumbB,
        pumpC=pumpC
    )

    background_tasks.add_task(action_task, request)
    return {"status": "accepted", "id": request.id}
