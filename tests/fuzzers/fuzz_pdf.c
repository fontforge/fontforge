#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	// Create a file with the fuzz data
	char filename[256];
	sprintf(filename, "/tmp/libfuzzer.%d", getpid());
	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		return 0;
	}
	fwrite(data, size, 1, fp);
	fclose(fp);

    // parse the file
	NamesReadPDF(filename);

    // cleanup
	unlink(filename);

	return 0;
}
