/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Local Includes
#include "../etherdreaminterface.h"

// External Includes
#include <etherdream.h>
#include <nap/logger.h>
#include <thread>


// Helper method used for retrieving dac
static etherdream* getEtherDreamDac(int number)
{
    return etherdream_get(static_cast<unsigned long>(number));
}


namespace nap
{
	EtherDreamInterface::EtherDreamInterface()     { }


	bool EtherDreamInterface::init()
	{
		mAvailableDacs = 0;
		if (etherdream_lib_start() != 0)
			return false;

		// The etherdream dac emits a signal once every second, make sure we wait long
		// enough to gather all available dacs (according to docs and verified)
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		// Gather all available dacs
		mAvailableDacs = etherdream_dac_count();
		return true;
	}


	int EtherDreamInterface::getCount() const
	{
		return mAvailableDacs;
	}


	std::string	EtherDreamInterface::getName(int number) const
	{
		etherdream* dac = getEtherDreamDac(number);
		if(dac == nullptr)
		{
			return "";
		}

		/**
		 * The *nix drivers for Etherdream don't provide the same getName method as in the Windows driver.
		 * getName on Windows just returns the last three parts of the MAC address as hex, whereas etherdream_get_id
		 * returns the same section of the MAC but bitshifted into a long.
		 *
		 * Here we do some convert from the bitshifted version of the MAC address to the hex format.
		 */

		// Grab the bitshifted id
		unsigned long bitwise_etherdream_mac = etherdream_get_id(dac);

		// Decode first segment and re-shift for later calculations
		std::array<unsigned long, 3> segments = { 0, 0, 0 };
		segments[0] = bitwise_etherdream_mac >> 16;
		unsigned long first_segment_shifted = segments[0] << 16;

		// Decode second segment and re-shift for final calculation
		segments[1] = (bitwise_etherdream_mac - first_segment_shifted) >> 8;
		unsigned long second_segment_shifted = segments[1] << 8;

		// Subtract previous shifted values from original for third segment
		segments[2] = bitwise_etherdream_mac - first_segment_shifted - second_segment_shifted;

		// Convert to hex
		std::stringstream stream;
		stream << std::hex << segments[0] << segments[1] << segments[2];
		return stream.str();
	}


	bool EtherDreamInterface::connect(int number)
	{
		etherdream* dac = getEtherDreamDac(number);
		if(dac == nullptr)
		{
			nap::Logger::warn("can't connect to etherdream dac: %d, invalid dac number", number);
			return false;
		}
		return (etherdream_connect(dac) == 0);
	}


	bool EtherDreamInterface::stop(int number)
	{
		etherdream* dac = getEtherDreamDac(number);
		if(dac == nullptr)
		{
			nap::Logger::warn("can't stop etherdream dac: %d, invalid dac number", number);
			return false;
		}
		return (etherdream_stop(dac) == 0);
	}


	void EtherDreamInterface::disconnect(int number)
	{
		etherdream* dac = getEtherDreamDac(number);
		if(dac == nullptr)
		{
			nap::Logger::warn("can't disconnect etherdream dac: %d, invalid dac number", number);
			return;
		}
		etherdream_disconnect(dac);
	}


	void EtherDreamInterface::close()
	{
		// OSX / Linux driver doesn't support general close
		return;
	}


	EtherDreamInterface::EStatus nap::EtherDreamInterface::getStatus(int number) const
	{
		etherdream* dac = getEtherDreamDac(number);
		if(dac == nullptr)
		{
			nap::Logger::warn("can't query etherdream status for dac: %d, invalid dac number", number);
			return EtherDreamInterface::EStatus::ERROR;
		}

		int eth_stat = etherdream_is_ready(dac);
		switch(eth_stat)
		{
			case 0:
				return EtherDreamInterface::EStatus::BUSY;
			case 1:
				return EtherDreamInterface::EStatus::READY;
			default:
				return EtherDreamInterface::EStatus::ERROR;
		}
	}


	bool EtherDreamInterface::writeFrame(int number, const EtherDreamPoint* data, uint npoints, uint pps, uint repeatCount)
	{
		etherdream* dac = getEtherDreamDac(number);
		if(dac == nullptr)
		{
			nap::Logger::warn("can't write frame to etherdream dac: %d, invalid dac number", number);
			return false;
		}

		// perform a c-style cast to etherdream point struct
		// the memory layout is the same
		etherdream_point* point_data = (etherdream_point*)(data);

		// Write frame
		return (etherdream_write(dac, point_data, static_cast<int>(npoints), static_cast<int>(pps), static_cast<int>(repeatCount)) == 0);
	}
}
