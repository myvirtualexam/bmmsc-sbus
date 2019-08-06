#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <fstream>
#include <cmath>
#include <regex>
#include <csignal>
#include "inih/INIReader.h"
#include "../ftd2xx.h"
#include "sbus.h"

#define CMDFILE "/dev/shm/sbus"
#define SAVEFILE "sbus.save"

int main(int argc, char *argv[]) {

  signal(SIGTERM, _end);
  signal(SIGINT, _end);

  if(argc>0) {

    std::string own = argv[0];
    int pos = own.rfind('/');
    std::string path = own.substr(0,pos+1);

    _save_file = path + SAVEFILE;

    INIReader reader(path + "mve.ini");

    if (reader.ParseError() != 0) {
        printf("Failed to load 'mve.ini'\n");
        return -1;
    }

    _device = reader.Get("sbus", "device", "");
    _delay = reader.GetInteger("sbus", "delay", 7);
    _cycles = reader.GetInteger("sbus", "cycles", 2);
    _sbus_min = reader.GetInteger("sbus", "sbus_min", 712);
    _sbus_center = reader.GetInteger("sbus", "sbus_center", 1024);
    _sbus_max = reader.GetInteger("sbus", "sbus_max", 1811);
    _zoom_wake_up_time = reader.GetInteger("sbus", "zoom_wake_up_time", 0);
    _zoom_total_time = reader.GetInteger("sbus", "zoom_total_time", 0);

    _channel1 = reader.Get("sbus", "channel1", "");
    _channel2 = reader.Get("sbus", "channel2", "");
    _channel3 = reader.Get("sbus", "channel3", "");
    _channel4 = reader.Get("sbus", "channel4", "");
    _channel5 = reader.Get("sbus", "channel5", "");
    _channel6 = reader.Get("sbus", "channel6", "");
    _channel7 = reader.Get("sbus", "channel7", "");
    _channel8 = reader.Get("sbus", "channel8", "");
    _channel9 = reader.Get("sbus", "channel9", "");
    _channel10 = reader.Get("sbus", "channel10", "");
    _channel17 = reader.Get("sbus", "channel17", "");
    _channel18 = reader.Get("sbus", "channel18", "");

    SBUS s;

    s._begin();

    ifs.open(_save_file, std::ifstream::binary);
    if (ifs.is_open())
    {
      ifs.read((char*) &_channels, sizeof(_channels));
      ifs.close();
    }
    else {
      for (uint8_t i = 0; i < 16; i++) {
    		_channels[i] = _sbus_center;
    	}

      _channels[16] = 0;
      _channels[17] = 0;
    }

    // Create file to read from if it doesn't exist
    if(access(CMDFILE, F_OK) != 0) {
      std::ofstream ofs;
      ofs.open(CMDFILE, std::ofstream::out);
      ofs.close();
    }

    ifs.open(CMDFILE, std::ifstream::in);

    while (1) {

      std::getline(ifs, _ret);
      _ret = trim(_ret);
      ifs.clear();
      ifs.seekg(0, std::ios::beg);

      if(strcmp(_cmd.c_str(),_ret.c_str())!=0) {
        _cmd = _ret;

        if(strcmp(_cmd.c_str(),"")!=0) s._frmt(_cmd);
          else continue;
      }

      if(s._zoom==true) {
        if(_start_time==0) {
          _start_time = TimeHelpers::TimeFromEpochInMilliSeconds<uint64_t>();
          //printf("\npercentage: %i", _zoom_percentage);
          _zoom_time = ((_zoom_percentage>_new_zoom?_zoom_percentage-_new_zoom:_new_zoom-_zoom_percentage) * _zoom_total_time)/100;
          //printf("\nzoom time: %i, new value: %i", ((_zoom_percentage>_new_zoom?_zoom_percentage-_new_zoom:_new_zoom-_zoom_percentage) * _zoom_total_time)/100, _new_zoom);
        }
        _end_time = TimeHelpers::TimeFromEpochInMilliSeconds<uint64_t>();
        if(_end_time - _start_time >= _zoom_wake_up_time) {

          if(_zoom_start==0) {
            _zoom_start = TimeHelpers::TimeFromEpochInMilliSeconds<uint64_t>();
          }
          _zoom_end = TimeHelpers::TimeFromEpochInMilliSeconds<uint64_t>();

          if(_zoom_end - _zoom_start >= _zoom_time) {
            //printf("\nzooming done");
            _zoom_percentage = _new_zoom;
            _channels[4] = _sbus_center;
            //_zoom_time = 0;
            s._zoom=false;
          }
        }
      }

      s._write(_channels);

      if(s._cycle==true) {
        s._frmt(_cmd);
      }

  		usleep(_delay * 1000);
  	}
  }

  return 0;
}

int SBUS::_begin()
{

  ftStatus = FT_OpenEx(const_cast<char*>(_device.c_str()), FT_OPEN_BY_SERIAL_NUMBER, &ftHandle);

  if (ftStatus != FT_OK)
	{
		printf("FT_Open(%s) failed (error %d).\n", _device.c_str(), (int)ftStatus);
		printf("Use lsmod to check if ftdi_sio (and usbserial) are present.\n");
		printf("If so, unload them using rmmod, as they conflict with ftd2xx.\n");
		return -1;
	}

  assert(ftHandle != NULL);

	ftStatus = FT_ResetDevice(ftHandle);
	if (ftStatus != FT_OK)
	{
		printf("Failure. FT_ResetDevice returned %d.\n", (int)ftStatus);
		return -1;
	}

  ftStatus = FT_SetBaudRate(ftHandle, _sbusBaud);
	if (ftStatus != FT_OK)
	{
		printf("FT_SetBaudRate failed (error %d).\n", (int)ftStatus);
		return -1;
	}

  ftStatus = FT_SetDataCharacteristics(ftHandle,
	                                     FT_BITS_8,
	                                     FT_STOP_BITS_2,
	                                     FT_PARITY_EVEN);
  if (ftStatus != FT_OK)
  {
  	printf("Failure. FT_SetDataCharacteristics returned %d.\n", (int)ftStatus);
  	return -1;
  }

	ftStatus = FT_SetTimeouts(ftHandle, 0, 100);	// 3 seconds
	if (ftStatus != FT_OK)
	{
		printf("Failure. FT_SetTimeouts returned %d\n", (int)ftStatus);
		return -1;
	}

  return 0;
}

void SBUS::_write(uint16_t* channels)
{

	packet[0] = _sbusHeader;

  // clear received channel data
  for (uint8_t i=1; i<24; i++) {
      packet[i] = 0;
  }

  // reset counters
  uint8_t ch = 0;
  uint8_t bit_in_servo = 0;
  uint8_t byte_in_sbus = 1;
  uint8_t bit_in_sbus = 0;

  // store servo data
  for (uint8_t i=0; i<176; i++) {
      if (channels[ch] & (1<<bit_in_servo)) {
          packet[byte_in_sbus] |= (1<<bit_in_sbus);
      }
      bit_in_sbus++;
      bit_in_servo++;

      if (bit_in_sbus == 8) {
          bit_in_sbus =0;
          byte_in_sbus++;
      }
      if (bit_in_servo == 11) {
          bit_in_servo =0;
          ch++;
      }
  }

  // DigiChannel 1
  if (channels[16] > 0) {
      packet[23] |= (1<<0);
  }
  // DigiChannel 2
  if (channels[17] > 0) {
      packet[23] |= (1<<1);
  }

  packet[24] = _sbusFooter;

  bytesToWrite = (DWORD)(sizeof(packet));

  ftStatus = FT_Write(ftHandle,
	                    packet,
	                    bytesToWrite,
	                    &bytesWritten);
	if (ftStatus != FT_OK)
	{
		printf("Failure. FT_Write returned %d\n", (int)ftStatus);
    exit(-1);
	}
}

void _end(int signum) {
  (void)FT_Close(ftHandle);
  ifs.close();
  std::ofstream ofs;
  ofs.open(_save_file, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
  ofs.write((char*) &_channels, sizeof(_channels));
  ofs.close();
  exit(signum);
}

void SBUS::_channel_def(std::string _channel, int& _val) {

  std::regex base_regex("\\[([0-9]+)\\]");
  std::smatch base_match;
  try {
    std::sregex_iterator next(_channel.begin(), _channel.end(), base_regex);
    std::sregex_iterator end;
    if(next != end) {
      base_match = *next;
      _type = stoi(base_match[1]);
      next++;
    }
    if(next != end) {
      base_match = *next;
      _range = stoi(base_match[1]);
      if(_val>_range) _val = _range;
    }

    if(strcmp(_types[_type].c_str(), "single")==0) {
      if(_cycle == true && __cycles >= _cycles) {
        _val = _sbus_center;
        _cycle = false;
        __cycles = 0;
        std::ofstream ofs;
        ofs.open(CMDFILE, std::ofstream::out);
        ofs << "";
        ofs.close();
      }
      else {
        _val = _sbus_max;
        _cycle = true;
        __cycles++;
      }
    }
    if(strcmp(_types[_type].c_str(), "toggle")==0) {
      if(_cycle == true && __cycles >= _cycles) {
        _val = _sbus_center;
        _cycle = false;
        __cycles = 0;
        std::ofstream ofs;
        ofs.open(CMDFILE, std::ofstream::out);
        ofs << "";
        ofs.close();
      }
      else {
        if(_val==1) {
          _val = _sbus_max;
        }
        else if(_val==-1) {
          _val = _sbus_min;
        }
        _cycle = true;
        __cycles++;
      }
    }
    if(strcmp(_types[_type].c_str(), "range")==0) {
      _steps = (_sbus_max-_sbus_min)/_range;//163.9
      _offset = _steps/2;//81.95
      if(_val < 1) _val = 1;
      _val = _sbus_min + _offset + ((_val-1)*_steps);
    }
    if(strcmp(_types[_type].c_str(), "bit")==0) {
      if(_cycle == true && __cycles >= _cycles) {
        _val = 0;
        _cycle = false;
        __cycles = 0;
        std::ofstream ofs;
        ofs.open(CMDFILE, std::ofstream::out);
        ofs << "";
        ofs.close();
      }
      else {
        _val = 1;
        _cycle = true;
        __cycles++;
      }
    }
    if(strcmp(_types[_type].c_str(), "zoom")==0) {
      _zoom = true;
      _new_zoom = _val;
      _start_time = 0;
      _zoom_start = 0;
      if(_val > _zoom_percentage) {
        _val = _sbus_max;
      }
      else {
        _val = _sbus_min;
      }
      std::ofstream ofs;
      ofs.open(CMDFILE, std::ofstream::out);
      ofs << "";
      ofs.close();
    }
    _val = round(_val);
  } catch (std::regex_error& e) {
    printf("Compilation error\n");
  }
}

void SBUS::_frmt(std::string _cmd) {
  _tmp = _cmd.substr(0, _cmd.find(' '));
  if(strcmp(_tmp.c_str(), "")!=0) _chan = stoi(_tmp);
  _tmp = _cmd.substr(_cmd.find(' ')+1, 3);
  _tmp = trim(_tmp);
  if(strcmp(_tmp.c_str(), "")!=0) {
    _val = stoi(_tmp);
  }

  switch(_chan){
    case 1:
      _channel_def(_channel1, _val);
      break;
    case 2:
      _channel_def(_channel2, _val);
      break;
    case 3:
      _channel_def(_channel3, _val);
      break;
    case 4:
      _channel_def(_channel4, _val);
      break;
    case 5:
      _channel_def(_channel5, _val);
      break;
    case 6:
      _channel_def(_channel6, _val);
      break;
    case 7:
      _channel_def(_channel7, _val);
      break;
    case 8:
      _channel_def(_channel8, _val);
      break;
    case 9:
      _channel_def(_channel9, _val);
      break;
    case 10:
      _channel_def(_channel10, _val);
      break;
    case 17:
      _channel_def(_channel17, _val);
      break;
    case 18:
      _channel_def(_channel18, _val);
      break;
  }

  _channels[_chan-1] = _val;
}
