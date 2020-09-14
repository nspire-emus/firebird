#ifndef FIELDPARSER_H
#define FIELDPARSER_H

#include <cstdint>

/* Class to parse the "Fields" commonly found in various Nspire images.
 * Format (big endian):
 * 10 0X <X bytes of data>
 * 10 0D SZ <SZ bytes of data>
 * 10 0E SI ZE <SIZE bytes of data>
 * 10 0F QU AD SI ZE <QUADSIZE bytes of data>
 * The data can contain one or more fields itself. */
class FieldParser
{
public:
    FieldParser(const uint8_t *ptr, uint32_t size, bool ignoreIncompleteData = false);

    const uint8_t *data() const { return dataPtr; }
    uint32_t sizeOfData() const { return dataSize; }
    uint32_t sizeWithHeader() const;
    uint16_t id() const { return idWithSize & ~0xF; }
    FieldParser subField(uint16_t id) const;
    bool isValid() const;

private:
    bool readHeader(const uint8_t * const ptr, const uint32_t size);

    uint16_t idWithSize; // Tag including the size nibble
    uint32_t dataSize = 0; // Size of contained data
    const uint8_t *dataPtr = nullptr, // Pointer to the first data byte
                  *headerPtr; // Pointer to the first header byte
};

#endif // FIELDPARSER_H
