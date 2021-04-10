#include "fieldparser.h"

FieldParser::FieldParser(const uint8_t * ptr, uint32_t size, bool ignoreIncompleteData)
{
    if(!readHeader(ptr, size))
        return;

    if(sizeWithHeader() > size) // Data out of bounds?
    {
        if(ignoreIncompleteData)
            dataSize = (ptr + size) - dataPtr; // Truncate
        else
            dataPtr = nullptr; // Mark as invalid
    }
}

uint32_t FieldParser::sizeWithHeader() const
{
    return (dataPtr - headerPtr) + dataSize;
}

FieldParser FieldParser::subField(uint16_t id) const
{
    const uint8_t *remainingData = dataPtr;
    uint32_t remainingSize = dataSize;

    FieldParser ret(remainingData, remainingSize);

    while(ret.isValid() && ret.id() != id)
    {
        uint32_t sizeToNextField = ret.sizeWithHeader();
        if(sizeToNextField > remainingSize)
            return FieldParser{nullptr, 0};

        remainingData += sizeToNextField;
        remainingSize -= sizeToNextField;
        ret = FieldParser(remainingData, remainingSize);
    }

    return ret;
}

bool FieldParser::isValid() const
{
    return dataPtr != nullptr;
}

bool FieldParser::readHeader(const uint8_t * const ptr, const uint32_t size)
{
    if(ptr == nullptr || size < 2)
        return false;

    headerPtr = ptr;
    idWithSize = (ptr[0] << 8) | ptr[1];

    switch(idWithSize & 0xF)
    {
    default:
        dataSize = idWithSize & 0xF;
        dataPtr = &ptr[2];
        break;
    case 0xD: // 8 bit size
        if(size < 3)
            return false;
        dataSize = ptr[2];
        dataPtr = &ptr[3];
        break;
    case 0xE: // 16 bit size
        if(size < 4)
            return false;
        dataSize = (ptr[2] << 8) | ptr[3];
        dataPtr = &ptr[4];
        break;
    case 0xF: // 32 bit size
        if(size < 6)
            return false;
        dataSize = (ptr[2] << 24) | (ptr[3] << 16) | (ptr[4] << 8) | ptr[5];
        dataPtr = &ptr[6];
        break;
    }

    return true;
}
