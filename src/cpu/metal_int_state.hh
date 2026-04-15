#pragma once

#include "base/flags.hh"
#include "cpu/reg_class.hh"
#include <sstream>

namespace gem5
{
    namespace metal
    {
        typedef gem5::Flags<uint64_t> InternalFlags;

        enum : uint64_t {
            // low 32 bits: architecture specific flags

            // high 32 bits: reserved for internal use
            FLAG_INST_INTERCEPT_MASK_TEMP = 0x100000000,
            FLAG_EXC_INTERCEPT_MASK_TEMP = 0x200000000,
            FLAG_INTERRUPT_MASK_TEMP = 0x400000000
            // XXX: include the permanent feature flags so that
            // modifying those becomes non-serializing
        };

        class InternalState {
        private:
            unsigned int level;
            InternalFlags flags;
        public:
            std::string toStr() const
            {
                std::stringstream ss;
                ss << "level " << level << ", flags 0x" << std::hex << flags;
                return ss.str();
            }

            void setLevel(unsigned int level) {
                this->level = level;
            }

            unsigned int getLevel() const {
                return level;
            }

            InternalFlags getFlags(void) const {
                return flags;
            }

            void setFlags(InternalFlags flags) {
                this->flags.set(flags);
            }

            void clearFlags(InternalFlags flags) {
                this->flags.clear(flags);
            }

            void reset(void) {
                level = 0;
                flags = 0;
            }

            void set(const InternalState & other) {
                this->flags = other.flags;
                this->level = other.level;
            }

            InternalState& operator=(const InternalState& other) {
                if (this != &other) {
                    set(other);
                }
                return *this;
            }

            InternalState(const InternalState& other) {
                if (this != &other) {
                    set(other);
                }
            }

            InternalState(void) : level(0), flags(0) {
            }
        };
    }
}
