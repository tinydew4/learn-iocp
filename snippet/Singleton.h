#pragma once

template <class T>
class CSingleton
{
private:
	CSingleton() {}
	~CSingleton() {}
public:
	CSingleton(CSingleton const &) = delete;
	void operator=(CSingleton const &) = delete;
	static T* Instance(void)
	{
		static T Instance;
		return &Instance;
	}
};
