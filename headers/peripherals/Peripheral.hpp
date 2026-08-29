#pragma once

#include <string>
#include <Types.hpp>

class Peripheral
{
public:
	Peripheral() {}

	virtual ~Peripheral();

	virtual void Update() = 0;
	virtual u64 GetSize() = 0;
	virtual bool DrawGui(bool &visible) = 0;

	void SetAddress(u64 value);

protected:
	bool DrawGuiBase(const char *title, bool &open, bool &visible);
	bool DrawGuiEnd();
	
protected:
	u64 offset = 0;
	u64 address = 0;
	std::string offsetAsText;
};
