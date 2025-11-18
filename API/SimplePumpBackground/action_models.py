from pydantic import BaseModel
from typing import Optional

class PumpCommand(BaseModel):
    state: bool = False
    speed: int = 2000
    dir: bool = True
    time: int = 10000  # ms

class ActionRequest(BaseModel):
    id: str
    pumpA: Optional[PumpCommand] = None
    pumpB: Optional[PumpCommand] = None
    pumpC: Optional[PumpCommand] = None
