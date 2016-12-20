#include <iostream>
#include <yaml-cpp/yaml.h>

int main()
{
	try 
	{
		YAML::Node config = YAML::LoadFile("config.yaml")["default"];

		std::cout << "listen=" << config["listen"] << std::endl;

		YAML::Node caches = config["caches"];
		std::cout << caches.size() << std::endl;
		for (YAML::const_iterator it=caches.begin();it!=caches.end();++it)
		{
			std::cout << *it << std::endl;
		}

	}
	catch (const std::exception& err)
	{
		std::cout << "error:" << err.what() << std::endl;
	}

	return 0;
}