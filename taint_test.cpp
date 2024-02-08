#include <iostream>
#include <string>
#include <stdlib.h>
using namespace std;

int main()
{
	const char* key = "This is secret key";
	char input[0x100];
	scanf("%s", input);
	char* tmp = new char[0x100];
	memcpy(tmp, input, strlen(input));
	tmp[strlen(input)] = 0;
	cout << "your input : " << tmp << endl;
	if (!strcmp(tmp, key))
	{
		cout << "congratulation!" << endl;
	}

}
