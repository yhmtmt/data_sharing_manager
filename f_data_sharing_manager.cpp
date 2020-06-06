// Copyright(c) 2013-2020 Yohei Matsumoto, All right reserved. 

// f_data_sharing_manager.cpp is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// f_data_sharing_manager.cpp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with f_data_sharing_manager.cpp.  If not, see <http://www.gnu.org/licenses/>. 

#include "f_data_sharing_manager.hpp"
DEFINE_FILTER(f_data_sharing_manager);

///////////////////////////////////////////////// f_data_sharing_manager
bool f_data_sharing_manager::init_run()
{
  m_sock = socket(AF_INET, SOCK_DGRAM, 0);	
  if(set_sock_nb(m_sock) != 0)
    return false;
  
  if(m_host_dst[0]){
    m_svr = false;
    m_client_fixed = true;
    m_sock_addr_snd.sin_family =AF_INET;
    m_sock_addr_snd.sin_port = htons(m_port_dst);
    m_sz_sock_addr_snd = sizeof(m_sock_addr_snd);
    set_sockaddr_addr(m_sock_addr_snd, m_host_dst);
    spdlog::info("[{}] Socket {} and the destination {}:{} is established.", get_name(), m_host_dst, m_port_dst); 
  }else{
    m_svr = true;
    m_client_fixed = false;
    m_sock_addr_rcv.sin_family =AF_INET;
    m_sock_addr_rcv.sin_port = htons(m_port);
    set_sockaddr_addr(m_sock_addr_rcv);
    if(::bind(m_sock, (sockaddr*)&m_sock_addr_rcv, sizeof(m_sock_addr_rcv)) == SOCKET_ERROR){
      spdlog::error("[{}] Socket error.", get_name());
      return false;
    }
    spdlog::info("[{}] Socket {} bound on port {}", get_name(), m_sock, m_port);
  }

  if(m_fname_out[0]){
    m_fout.open(m_fname_out);	
    if(!m_fout.is_open())
      return false;
  }
  
  if(m_fname_in[0]){
    m_fin.open(m_fname_in);
    if(!m_fin.is_open())
      return false;
  }

  m_len_pkt_rcv = sizeof(m_cur_time);
  for (int och = 0; och < m_chout.size(); och++){
    m_len_pkt_rcv += (int) m_chout[och]->get_dsize();
  }
  
  m_len_pkt_snd = sizeof(m_cur_time);
  for (int ich = 0; ich < m_chin.size(); ich++){
    m_len_pkt_snd += (int) m_chin[ich]->get_dsize();
  }
 
  m_rbuf = new char [m_len_pkt_rcv];
  if(m_rbuf == NULL)
    return false;

  m_wbuf = new char [m_len_pkt_snd];
  if(m_wbuf == NULL){
    delete[] m_rbuf;
    m_rbuf = NULL;
    return false;
  }

  if(m_test){
    if(m_chin.size() != 1 ||
       ((test_in = dynamic_cast<ch_ctrl_data*>(m_chin[0])) == nullptr)){
      spdlog::error("[{}] In test mode, input channel list have one ctrl_data channel.", get_name());
      return false;
    }
    if(m_chout.size() != 1 ||
       ((test_out = dynamic_cast<ch_ctrl_data*>(m_chout[0])) == nullptr)){
      spdlog::error("[{}] In test mode, output channel list have one ctrl_data channel.", get_name());
      return false;
    }

    test_buf = new unsigned char[test_in->get_data_size()];
    test_count = 0;
    srand(0);
  }
  
  return true;
}

void f_data_sharing_manager::destroy_run()
{
  delete[] m_rbuf;
  m_rbuf = NULL;
  delete[] m_wbuf;
  m_wbuf = NULL;

  if(m_fout.is_open()){
    m_fout.close();
  }

  if(m_fin.is_open()){
    m_fin.close();
  }

  closesocket(m_sock);
  m_sock = -1;

  delete [] test_buf;
  test_buf = nullptr;
  for(int i = 0; i < test_data.size(); i++)
    delete [] test_data[i];

  test_data.clear();
  test_data_size.clear();
}

bool f_data_sharing_manager::proc()
{
  int res;
  fd_set fr, fw, fe;
  timeval tv;

  if(m_test){
    if(!m_svr){
      // pushing test data to test_in
      unsigned int num_data = rand() % test_in->get_queue_len();
      for(int idata = test_data.size(); idata < num_data; idata++){
	unsigned int data_size = (rand() % (test_in->get_data_size() - 1)) + 1;
	
	unsigned char * data = new unsigned char[data_size];
	  for(int ibyte = 0; ibyte < data_size; ibyte++)
	    data[ibyte] = rand() % 256;
	
	test_in->push(data, data_size);
	test_data.push_back(data);
	test_data_size.push_back(data_size);
      }
    }
    
    // poping test data from test_out
    while(1){
      unsigned int data_size;
      test_out->pop(test_buf, data_size);


      if(data_size != 0){
	if(m_svr){ // server only loop back
	  test_in->push(test_buf, data_size);
	  continue;
	}
	
	// client checks the test data 	
	if(data_size == test_data_size[0] &&
	   memcmp(test_data[0], test_buf, data_size) == 0){
	  delete[] test_data[0];
	  test_data.erase(test_data.begin());
	  test_data_size.erase(test_data_size.begin());
	  test_count++;
	}else{
	  spdlog::error("[{}] Test Failed at count {}.",
			get_name(), test_count);
	  spdlog::error("[{}] data_size expected {} actual {}.",
			get_name(), test_data_size[0], data_size);
	  return false;
	}	
      }else{
	break;
      }
    }
  }
  
  // sending phase
  if(m_svr && m_client_fixed || !m_svr){
    m_wbuf_head = m_wbuf_tail = 0;
    (*(long long*)m_wbuf) = m_cur_time;
    m_wbuf_tail = sizeof(m_cur_time);
    for(int ich = 0; ich < m_chin.size(); ich++)
      m_wbuf_tail += (int)(m_chin[ich]->read_buf(m_wbuf + m_wbuf_tail));
    
    while(m_wbuf_tail > m_wbuf_head){
      FD_ZERO(&fe);
      FD_ZERO(&fw);
      FD_SET(m_sock, &fe);
      FD_SET(m_sock, &fw);
      tv.tv_sec = 0;
      tv.tv_usec = 10000;
      
      res = select((int) m_sock + 1, NULL, &fw, &fe, &tv);
      
      if(FD_ISSET(m_sock, &fw)){
	res = sendto(m_sock, 
		     (char*) m_wbuf + m_wbuf_head, 
		     m_wbuf_tail - m_wbuf_head, 0, 
		     (sockaddr*) &m_sock_addr_snd, m_sz_sock_addr_snd);
	if(res == -1)
	  break;
	else if (res == 0){
	  spdlog::error("[{}] Socket has been closed, trying reconnect.",
			get_name());
	  return reconnect();
	}
	m_wbuf_head += res;
      }else if(FD_ISSET(m_sock, & fe)){
	spdlog::error("[{}] Socket error during sending packet. Closing socket.", get_name());
	return reconnect();
      }else{
	// time out;
	break;
      }
    }
    if(m_verb){
      cout << "Inputs: t=" << m_tshare <<  endl;
      for(int ich = 0; ich < m_chin.size(); ich++)
	m_chin[ich]->print(cout);
    }
  }

  // recieving phase
  m_rbuf_head = m_rbuf_tail = 0;
  while(m_rbuf_tail != m_len_pkt_rcv){
    FD_ZERO(&fr);
    FD_ZERO(&fe);
    FD_SET(m_sock, &fr);
    FD_SET(m_sock, &fe);
    tv.tv_sec = 0;
    tv.tv_usec = 1000; 
      
    res = select((int) m_sock + 1, &fr, NULL, &fe, &tv);
    if(FD_ISSET(m_sock, &fr)){
      int res = 0;
      //res = recv(m_sock, m_rbuf, m_len_pkt_rcv - m_rbuf_tail, 0);
      m_sz_sock_addr_snd = sizeof(m_sock_addr_snd);
      res = recvfrom(m_sock, 
		     (char*) m_rbuf + m_rbuf_tail, 
		     m_len_pkt_rcv - m_rbuf_tail,
		     0,
		     (sockaddr*) & m_sock_addr_snd, 
		     &m_sz_sock_addr_snd);	
      if(res == -1)
	break;
      else if (res == 0){
	spdlog::error("[{}] Socket has been closed, trying reconnect.", get_name());
	return reconnect();
      }else
	m_rbuf_tail += res;
      m_client_fixed = true;
      m_tshare = *((long long*)m_rbuf);
      m_rbuf_head = sizeof(m_tshare);
      for(int och = 0; och < m_chout.size(); och++){
	m_rbuf_head += (int)(m_chout[och]->write_buf(m_rbuf + m_rbuf_head));
      }
      if(m_verb){
	cout << "Outputs: t=" << m_tshare << " " <<
	  m_rbuf_tail << "/" << res << endl;
	for(int och = 0; och < m_chout.size(); och++)
	  m_chout[och]->print(cout);
      }
      m_rbuf_head = m_rbuf_tail = 0;
    }else if(FD_ISSET(m_sock, &fe)){
      spdlog::error("[{}] Socket error during recieving packet in {}. Now closing socket.", get_name());
      return reconnect();
    }else{
      // time out;
      return true;
    }
  }
  
  return true;
}
