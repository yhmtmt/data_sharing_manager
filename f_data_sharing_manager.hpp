// Copyright(c) 2013-2019 Yohei Matsumoto, Tokyo University of Marine
// Science and Technology, All right reserved. 

// f_data_sharing_manager.hpp is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// f_data_sharing_manager.hpp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with f_data_sharing_manager.hpp.  If not, see <http://www.gnu.org/licenses/>. 

#ifndef F_DATA_SHARING_MANAGER_HPP
#define F_DATA_SHARING_MANAGER_HPP

#include "filter_base.hpp"

class f_data_sharing_manager: public f_base
{
private:
  char m_host_dst[1024];
  unsigned short m_port, m_port_dst;
  int m_len_pkt_snd, m_len_pkt_rcv;
  SOCKET m_sock;
  sockaddr_in m_sock_addr_snd, m_sock_addr_rcv;
  socklen_t m_sz_sock_addr_snd;
  bool m_svr;
  bool m_client_fixed;
  bool m_verb;

  // buffers
  char * m_rbuf, * m_wbuf;
  int m_rbuf_head, m_wbuf_head;
  int m_rbuf_tail, m_wbuf_tail;
  int m_len_buf;
  
  // log file
  char m_fname_out[1024];
  ofstream m_fout;
  char m_fname_in[1024];
  ofstream m_fin;

  long long m_tshare;
  bool reconnect(){
    destroy_run();
    return init_run();
  }
  
public:
  f_data_sharing_manager(const char * fname) : f_base(fname), m_verb(false),
				   m_port(20100), m_port_dst(20101),
				   m_len_pkt_snd(1024), m_len_pkt_rcv(1024), m_sock(-1),
				   m_svr(false),
				   m_rbuf(NULL), m_wbuf(NULL), m_rbuf_head(0), m_wbuf_head(0),
				   m_rbuf_tail(0), m_wbuf_tail(0), m_len_buf(0), m_tshare(0)
  {
    m_fname_out[0] = '\0';
    m_fname_in[0] = '\0';
    m_host_dst[0] = '\0';
    register_fpar("verb", &m_verb, "For debug.");
    register_fpar("port", &m_port, "UDP port.");
    register_fpar("port_dst", &m_port_dst, "Destination UDP port.");
    register_fpar("host_dst", m_host_dst, 1024, "Host address.");
    //register_fpar("lpkt", &m_len_pkt, "Packet length");
    register_fpar("fout", m_fname_out, 1024, "Output log file.");
    register_fpar("fin", m_fname_in, 1024, "Input log file.");
    register_fpar("tshare", &m_tshare, "Time data sharing");
  }
  
  virtual ~f_data_sharing_manager()
  {
  }
  
  virtual bool init_run();
  virtual void destroy_run();
  virtual bool proc();
};

#endif
