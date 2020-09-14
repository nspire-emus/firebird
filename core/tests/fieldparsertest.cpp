#include <cassert>

#include <cstdio>

#include "fieldparser.h"

int main()
{
	uint8_t invalidData[] = {};
	FieldParser invalid(invalidData, sizeof(invalidData));
	assert(!invalid.isValid());

	uint8_t outOfBoundsData[] = {0x10, 0x01};
	FieldParser outOfBounds(outOfBoundsData, sizeof(outOfBoundsData));
	assert(!outOfBounds.isValid());
	// Try again with ignoreIncompleteData=true
	outOfBounds = FieldParser(outOfBoundsData, sizeof(outOfBoundsData), true);
	assert(outOfBounds.isValid());
	assert(outOfBounds.sizeOfData() == 0);

	uint8_t validData[] = {0x10, 0x01, 0x42};
	FieldParser valid(validData, sizeof(validData));
	assert(valid.isValid());
	assert(valid.sizeOfData() == 1);
	assert(valid.data()[0] == 0x42);
	assert(valid.sizeWithHeader() == sizeof(validData));
	assert(valid.id() == 0x1000);

	uint8_t nestedData[] = {0x10, 0x03, 0x10, 0x11, 0x42};
	FieldParser nested(nestedData, sizeof(nestedData));
	auto nestedChild = nested.subField(0x1010);
	assert(nested.isValid());
	assert(nestedChild.isValid());
	assert(nestedChild.sizeOfData() == 1);
	assert(nestedChild.sizeWithHeader() == 3);
	assert(nestedChild.data()[0] != 0x42);
}
