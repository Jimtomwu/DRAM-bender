#ifndef BOARD_H
#define BOARD_H

/**
 * MicroBlaze FSL-based interface to the DRAM-Bender FPGA core.
 *
 * Uses putfsl / getfsl instructions to stream data over the
 * Fast Simplex Link — a point-to-point FIFO built into the
 * MicroBlaze pipeline.  No DMA buffers or OS dependencies.
 */
class BoardInterface {
public:
  BoardInterface(int dimm_select = 0);
  ~BoardInterface();

  int init();
  int sendData(void* data, const unsigned int size);
  int recvData(void* buf,  const unsigned int size);

private:
  static const int FSL_TX_SLOT = 0;  /* putfsl → FPGA (commands / instructions) */
  static const int FSL_RX_SLOT = 1;  /* getfsl ← FPGA (read data)               */
  int dimm_select;
};

#endif
