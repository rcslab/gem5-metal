#pragma once

#include "cpu/reg_class.hh"

namespace gem5
{
    class MetalInternalState {
    protected:
        RegVal msr;

    public:
        RegVal getMSR(void) const {
            return this->msr;
        }
        
        void setMSR(RegVal val) {
            this->msr = val;
        }
        
        void reset(void) {
            this->msr = 0;
        }
        
        void set(const MetalInternalState& other) {
            this->msr = other.msr;
        }

        MetalInternalState& operator=(const MetalInternalState& other) = delete;
        MetalInternalState(const MetalInternalState& other) {
            if (this != &other) {
                set(other);
            }
        }

        MetalInternalState(void) {
            reset();
        }
    };
}