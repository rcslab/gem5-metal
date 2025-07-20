from m5.objects.AbstractMemory import *
from m5.params import *
from m5.proxy import *


class MRAM(AbstractMemory):
    type = "MRAM"  
    cxx_header = "mem/metal/mram.hh"
    cxx_class = "gem5::memory::metal::MRAM"

    port = VectorResponsePort("CPU side ports")

    latency = Param.Cycles(1, "Cycles for MRAM access")

    def controller(self):
        # Simple memory doesn't use a MemCtrl
        return self
