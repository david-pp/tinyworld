#include <iostream>

#include <stdio.h>
#include <string.h>
#include "hashkit.h"


const char* k2text(unsigned char* key)
{
    static char text[256] = "";

    size_t i = 0;
    memset(text, 0, sizeof(text));
    for (i = 0; i < KEY_LENGTH; i++)
    	sprintf(&text[i*2], "%.2X", key[i]);

    return text;
}

int main()
{
	char key1[KEY_LENGTH] = "123456\n";

	unsigned char key[KEY_LENGTH] = "";

	genkey(key, key1, sizeof(key1), 10);

	//std::cout << key << std::endl;
	std::cout << k2text((unsigned char*)key1) << std::endl;
	std::cout << k2text((unsigned char*)key) << std::endl;

}