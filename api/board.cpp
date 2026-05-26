#include "board.h"
#include "board_axis.h"
#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#endif
#include <iostream>
#include <cassert>
#include <string.h>

BoardInterface::BoardInterface(IFACE iface_type, int dimm_select)
{
  this -> iface_type = iface_type;
  this -> dimm_select = dimm_select;
  to_card = -1;
  from_card = -1;
  send_buf = nullptr;
  recv_buf = nullptr;
  axis_tx_base = 0;
  axis_rx_base = 0;
}
BoardInterface::~BoardInterface()
{
  if (send_buf)
    free(send_buf);
  if (recv_buf)
    free(recv_buf);
#ifdef __linux__
  if (iface_type == IFACE::XDMA) {
    if (to_card >= 0)
      close(to_card);
    if (from_card >= 0)
      close(from_card);
  }
#endif
}

int BoardInterface::init()
{
  switch(iface_type)
  {
#ifdef __linux__
    case IFACE::XDMA:
    {
      int fpga_fd;
      if (dimm_select == 0)
        fpga_fd = open(TO_FPGA_DEFAULT.c_str(), O_RDWR);
      else
        fpga_fd = open("/dev/xdma0_h2c_1", O_RDWR);

      if(fpga_fd<0)
      {
        std::cerr << "Open to card failed!" << std::endl;
        return 1;
      }
      else
        std::cout << "Opened " << (dimm_select == 0 ? TO_FPGA_DEFAULT : "/dev/xdma0_h2c_1") <<  " -> " << fpga_fd << std::endl;
      to_card = fpga_fd;
      if (dimm_select == 0)
        fpga_fd = open(FROM_FPGA_DEFAULT.c_str(), O_RDWR);
      else
        fpga_fd = open("/dev/xdma0_c2h_1", O_RDWR);
      if(fpga_fd<0)
      {
        std::cerr << "Open to host failed!" << std::endl;
        return 1;
      }
      else
        std::cout << "Opened " << (dimm_select == 0 ? FROM_FPGA_DEFAULT : "/dev/xdma0_c2h_1") <<  " -> " << fpga_fd << std::endl;
      from_card = fpga_fd;
      // allocate page size aligned X page size regions to our buffers
      if (posix_memalign((void **)&send_buf, 4096 /*alignment */ , SEND_BUF_SIZE + (4096-(SEND_BUF_SIZE % 4096))) != 0)
      {
        std::cerr << "Send buffer allocation failed!" << std::endl;
        return 1;
      }
      if (posix_memalign((void **)&recv_buf, 4096 /*alignment */ , RECV_BUF_SIZE + (4096-(RECV_BUF_SIZE % 4096))))
      {
        std::cerr << "Receive buffer allocation failed!" << std::endl;
        return 1;
      }
      if( (!send_buf) || (!recv_buf) )
      {
        std::cerr << "Buffers cannot be allocated!" << std::endl;
        return 1;
      }
      return 0;
    }
#endif
    case IFACE::AXIS:
      return axis_init();
    default:
      std::cerr << "Unknown iface_type!" << std::endl;
      return 1;
  }
}

int BoardInterface::sendData(void* data, const uint size)
{
  switch(iface_type)
  {
#ifdef __linux__
    case IFACE::XDMA:
      return xdma_send(data,size);
#endif
    case IFACE::AXIS:
      return axis_send(data,size);
    default:
      std::cerr << "Unknown iface_type!" << std::endl;
      return 1;
  }
}

int BoardInterface::recvData(void* buf, const uint size)
{
  switch(iface_type)
  {
#ifdef __linux__
    case IFACE::XDMA:
      return xdma_recv(buf,size);
#endif
    case IFACE::AXIS:
      return axis_recv(buf,size);
    default:
      std::cerr << "Unknown iface_type!" << std::endl;
      return 1;
  }
}

#ifdef __linux__
int BoardInterface::xdma_send(void* data, const uint size)
{
  memcpy((char*)send_buf, (char*)data, size);

  int fd = to_card;
  ssize_t rc;
  uint64_t count = 0;
  char *buf = (char*) send_buf;

  while (count < size) {
    rc = write(fd, buf + count, size - count);
    if (rc == 0) {
      std::cerr << "xdma_send: write returned 0 (device busy or closed)" << std::endl;
      return 1;
    }
    if (rc < 0) {
      std::cerr << "xdma_send: write error" << std::endl;
      return 1;
    }
    count += rc;
  }

  if (count != size)
    return 1;

  return 0;
}

int BoardInterface::xdma_recv(void* buf, const uint size)
{
  if (size > RECV_BUF_SIZE) {
    std::cerr << "xdma_recv: given read size is too large" << std::endl;
    return 0;
  }

  int fd = from_card;
  uint64_t count = 0;
  count = read(fd, (char*) recv_buf, size);

  if (count > size) {
    std::cerr << "xdma_recv: read more than expected" << std::endl;
    return 0;
  }

  memcpy(buf, (char*) recv_buf, count);
  return count;
}
#endif

int BoardInterface::axis_init()
{
  axis_tx_base = AXIS_TX_FIFO_BASEADDR;
  axis_rx_base = AXIS_RX_FIFO_BASEADDR;
  send_buf = malloc(SEND_BUF_SIZE);
  recv_buf = malloc(RECV_BUF_SIZE);
  if (!send_buf || !recv_buf)
    return 1;
  return 0;
}

int BoardInterface::axis_send(void* data, const uint size)
{
  return axis_fifo_send(axis_tx_base, data, size);
}

int BoardInterface::axis_recv(void* buf, const uint size)
{
  return axis_fifo_recv(axis_rx_base, buf, size);
}
