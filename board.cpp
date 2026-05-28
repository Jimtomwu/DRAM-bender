#include "board.h"
#include <string.h>

extern "C" {
#include <fsl.h>
}

BoardInterface::BoardInterface(int dimm_select)
{
  this->dimm_select = dimm_select;
}

BoardInterface::~BoardInterface()
{
}

int BoardInterface::init()
{
  return 0;
}

int BoardInterface::sendData(void* data, const unsigned int size)
{
  const unsigned int count = size / 4;

  for (unsigned int i = 0; i < count; i++)
    putfsl(((unsigned int*)data)[i], FSL_TX_SLOT);

  /* trailing bytes — pad to word */
  unsigned int rem = size % 4;
  if (rem) {
    unsigned int last = 0;
    memcpy(&last, (unsigned char*)data + count * 4, rem);
    putfsl(last, FSL_TX_SLOT);
  }

  return 0;
}

int BoardInterface::recvData(void* buf, const unsigned int size)
{
  const unsigned int count = size / 4;

  for (unsigned int i = 0; i < count; i++)
    getfsl(((unsigned int*)buf)[i], FSL_RX_SLOT);

  /* trailing bytes — read full word, copy what's needed */
  unsigned int rem = size % 4;
  if (rem) {
    unsigned int last;
    getfsl(last, FSL_RX_SLOT);
    memcpy((unsigned char*)buf + count * 4, &last, rem);
  }

  return size;
}
