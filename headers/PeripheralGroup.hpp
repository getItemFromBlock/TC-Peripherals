#pragma once

#include <vector>
#include <Types.hpp>
#include <Peripheral.hpp>

class PeripheralGroup
{
public:
	PeripheralGroup() {}

	~PeripheralGroup();

	void SearchForPattern(const std::vector<u8> &pattern, u64 startAddress = 0);
	void BindAt(u64 targetAddress);
	void AddPeripheral(Peripheral *periph);
	const std::vector<Peripheral *> &GetPeripherals() const { return peripherals; }

	void Update();
	bool IsBound() const;

private:
	std::vector<Peripheral*> peripherals;
	std::vector<u8> targetPattern;
	u64 boundAddress = 0;
	u64 startAddress = 0;
	u32 bindAttempts = 0;
	bool tryAutoBind = false;
};
