#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

int main() {
	char password[20] = "AAAAAAAAAAAA";
	int digit = 11;
	int cnt = 0;
	while(1) {
		unsigned char XOR = password[0];
		unsigned char ADD = password[0];
		for(int i=1; i<12; i++) {
			XOR ^= password[i];
			ADD += password[i];
		}

		unsigned int add = ADD;
		unsigned int xor = XOR;

		add += 0x48;
		xor ^= 0xffffff9e;

		//??
		xor %= 256;

		add *= 0xc5;
		add += xor;

		// cltd ?
		int64_t t = add;
		if(t % 0x101 == 0x62) {
			//printf("%s\n", password);
			char cmd[128] = "./L3 ";
			strcat(cmd, password);
			system(cmd);
			cnt++;
			if(cnt >= 1024)
				break;
		}

		password[digit] += 1;
		while(password[digit] == 'Z') {
			password[digit] = 'A';
			digit--;
			password[digit] += 1;
		}
		digit = 11;
	}

	return 0;
}
