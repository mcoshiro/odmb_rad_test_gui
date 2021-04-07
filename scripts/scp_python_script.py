#script that allows the radiation test software GUI to be run on one computer 
#and Vivado to be running on another using scp
#only polls every 10 seconds, so very high latency

import time
import os

if  __name__ == '__main__':
  continue_running = True
  while (continue_running):
    if (os.path.exists('comm_files/tcl_comm_in.txt')):
      os.system('scp comm_files/tcl_comm_in.txt tau:odmb/software/rad_test_sw/comm_files/tcl_comm_in.txt')
      os.system('rm comm_files/tcl_comm_in.txt')
    os.system('scp tau:odmb/software/rad_test_sw/comm_files/tcl_comm_out.txt comm_files/tcl_comm_out.txt')
    time.sleep(10)
