#pragma once

#include <peripherals/Peripheral.hpp>

class Controller : public Peripheral
{
public:
	Controller();

	virtual ~Controller() override;

	enum GamepadAxisDataType : u32
	{
		Byte = 0,
		Short = 1,
		Word = 2,
		Float = 3
	};

	void Update() override;
	bool DrawGui(bool &visible) override;
	u64 GetSize() override;

	bool HasGamepad() const;

private:

	u32 selectedGamepad = (u32)-1;

	GamepadAxisDataType dataType = Short;
	u32 boundButtons = 0;
	u32 boundAxis = 0;
};
