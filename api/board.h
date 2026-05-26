#include <string>
#include <cstdint>

#ifndef BOARD_H
#define BOARD_H
/** This class defines how the host
 * interfaces with the board.
 */
class BoardInterface{
  const uint SEND_BUF_SIZE = 32*2048;
              // send each instruction as a 256-bit packet
  const uint RECV_BUF_SIZE = 1024*32;
  const std::string TO_FPGA_DEFAULT = "/dev/xdma0_h2c_0";
  const std::string FROM_FPGA_DEFAULT = "/dev/xdma0_c2h_0";
public:
  enum class IFACE {
      XDMA = 0,
      AXIS = 1
  };
  BoardInterface(IFACE, int dimm_select = 0);
  ~BoardInterface();
  int init();
  int sendData(void* data, const uint size);
  int recvData(void* buf , const uint size);
private:
  IFACE iface_type;
  // XDMA related constructs
  int to_card;
  int from_card;
  int dimm_select;
  void* send_buf;
  void* recv_buf;
#ifdef __linux__
  int xdma_send(void* data, const uint size);
  int xdma_recv(void* buf,  const uint size);
#endif
  // end XDMA related constructs
  // AXI-Stream related constructs
  uintptr_t axis_tx_base;
  uintptr_t axis_rx_base;
  int axis_init();
  int axis_send(void* data, const uint size);
  int axis_recv(void* buf,  const uint size);
  // end AXI-Stream related constructs
};

#endif
