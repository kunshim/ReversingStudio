#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
namespace WINDOWS
{
#include <Windows.h>
}
namespace System
{
	static bool init;
	static std::ifstream argsFile;
	static std::vector<std::string> args;
	static std::unordered_map<std::string, std::string> map;
	/// <summary>
	/// Return AppData\Local\Temp
	/// </summary>
	/// <returns></returns>
	static const std::string getSystemPath()
	{
		char buf[256];
		WINDOWS::GetTempPathA(sizeof(buf), buf);
		return buf;
	}
	//Must call before PIN_StartProgram
	static void Init()
	{
		char line[256];
		argsFile.open((getSystemPath() + "\\rsinput.txt").c_str(), std::ifstream::in);
		if (argsFile.fail())
		{
			std::cout << "Fail to open args file\n";
			std::cout << (getSystemPath() + "\\rsinput.txt") << std::endl;
			exit(-1);
		}
		while (argsFile.getline(line, 256))
		{
			std::string tmp(line);
			if (tmp.find('=') != std::string::npos)
			{
				map[tmp.substr(0, tmp.find('='))] = tmp.substr(tmp.find('=') + 1);
			}
			else
				args.push_back(line);
		}
		init = true;
	}
	static const std::string getCustomArgs(int idx)
	{
		if (!init)
			Init();
		if (idx < args.size())
			return args[idx];
		return "";
	}
	static const std::string getCustomValue(std::string key)
	{
		if (!init)
			Init();
		if (System::map.find(key) != System::map.end())
		{
			return System::map[key];
		}
		return "invalid";
	}

	static int getCustomArgsCount()
	{
		if (!init)
			Init();
		return args.size();
	}
};