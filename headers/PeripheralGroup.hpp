#pragma once

#include <vector>
#include <string>
#include <Types.hpp>
#include <Peripheral.hpp>

class PeripheralGroup
{
public:
	PeripheralGroup() {}

	~PeripheralGroup();

	void SearchForPattern(const std::vector<u8> &pattern, u64 startAddress = 0);
	void SetPattern(const std::vector<u8> &pattern);
	void BindAt(u64 targetAddress);
	void AddPeripheral(Peripheral *periph);
	const std::vector<Peripheral *> &GetPeripherals() const { return peripherals; }

	void Update();
	void BackgroundUpdate();
	bool DrawGui();
	bool IsBound() const;

protected:
	void UpdatePatternText();
	void ParsePatternText();
	void UpdatePeripheralsAddress();

private:
	std::vector<Peripheral*> peripherals;
	std::vector<u8> targetPattern;
	std::string patternAsText;
	std::string addressAsText;
	u64 boundAddress = 0;
	u64 startAddress = 0;
	u32 bindAttempts = 0;
	bool tryAutoBind = false;
	bool tryBindGui = true;
};
