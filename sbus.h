#ifndef SBUS_h
#define SBUS_h

uint16_t _channels[18] = {0};

int i, iNumDevs = 0, sbus_fd, fd, _delay, _cycles, _sbus_min, _sbus_center, _sbus_max;
uint8_t packet[25] = {0}, n;
std::string _save_file, _device, _ret, _cmd, _channel1, _channel2, _channel3, _channel4, _channel5, _channel6, _channel7, _channel8, _channel9, _channel10, _channel17, _channel18;

FT_STATUS  ftStatus = FT_OK;
FT_HANDLE  ftHandle = NULL;
DWORD      bytesWritten = 0;
DWORD      bytesToWrite = 0;

std::ifstream ifs;

int main(int argc, char *argv[]);

inline std::string trim(std::string& str)
{
	str.erase(0, str.find_first_not_of(' '));       //prefixing spaces
	str.erase(str.find_last_not_of(' ')+1);         //surfixing spaces
	return str;
}

void _end(int signum);

class SBUS{
	public:
		int _begin();
		void _write(uint16_t* channels);
		void _channel_def(std::string _channel, int& _val);
		void _frmt(std::string _cmd);
		bool _cycle = false;
		int __cycles = 0;

		uint16_t _map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
			return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
		}
  private:
		const uint32_t _sbusBaud = 100000;
		const uint8_t _sbusHeader = 0x0f;
		const uint8_t _sbusFooter = 0x00;
		std::string _tmp, _types[4] = {"single","toggle","range","bit"};
		double _steps, _offset;
		int _chan=100, _val=0, _range=0, _type=0;

};

#endif

#ifndef CURRENT_TIME_H
#define CURRENT_TIME_H

#include <chrono>
#include <cstdint>

namespace TimeHelpers
{
    namespace
    {
        inline std::chrono::high_resolution_clock Clock()
        {
            static std::chrono::high_resolution_clock clock;
            return clock;
        }

        template<typename Out, typename In>
        Out TimeAs()
        {
            return std::chrono::duration_cast<In>
                (Clock().now().time_since_epoch()).count();
        }
    }

    template<typename Out>
    inline Out TimeFromEpochInMilliSeconds()
    {
        return TimeAs<Out, std::chrono::milliseconds>();
    }

    template<typename Out>
    inline Out TimeFromEpochInMicroSeconds()
    {
        return TimeAs<Out, std::chrono::microseconds>();
    }

    template<typename Out>
    inline Out TimeFromEpochInNanoSeconds()
    {
        return TimeAs<Out, std::chrono::nanoseconds>();
    }
}

#endif
