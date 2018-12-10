#include <stdio.h>
#include <system_properties.h>


int main(int argc, char* argv[]) 
{
    int ret = 0;

	ret = __system_properties_init();
	if (ret) {
		fprintf(stderr,"__system_properties_init failed.\n");
		return -1;
	}

    if (argc != 3) {
        fprintf(stderr, "usage: setprop <key> <value>\n");
        return 1;
    }

    if (property_set(argv[1], argv[2])) {
        fprintf(stderr, "could not set property\n");
        return 1;
    }

    return 0;

}
