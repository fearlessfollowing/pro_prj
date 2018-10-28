#include <stdio.h>
#include <system_properties.h>


int main(int argc, char* argv[]) 
{
    int ret = 0;
	char buf[PROP_VALUE_MAX];
	
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: getprop <key>\n");
        return 1;
    }

	ret = __system_properties_init();
	if (ret) {
		fprintf(stderr, "__system_properties_init failed.\n");
		return -1;
	}



	if (argc == 2) {	/* 查询指定属性 */
		printf("prop %s:[%s]\n", argv[1], property_get(argv[1]));
	} else {
	}

    return 0;

}

