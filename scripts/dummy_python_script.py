import time
import os

if  __name__ == '__main__':
  time.sleep(10)
  tcl_comm_out_file = open('comm_files\\tcl_comm_out.txt','a')
  tcl_comm_out_file.write('log: Test started\n')
  tcl_comm_out_file.write('sync: link 1 connected\n')
  tcl_comm_out_file.write('sync: link 2 connected\n')
  tcl_comm_out_file.close()

  continue_running = True
  cycle_counter = 0
  while (continue_running):
    #let GUI know script is still running every 12 seconds
    cycle_counter = (cycle_counter + 1) % 4
    tcl_comm_out_file = open('comm_files\\tcl_comm_out.txt','a')
    tcl_comm_out_file.write('sync: test in progress\n')
    tcl_comm_out_file.close()

    #parse commands left by GUI
    if (os.path.exists('comm_files\\tcl_comm_in.txt')):
      tcl_comm_in_file = open('comm_files\\tcl_comm_in.txt','r')
      tcl_comm_in_text = tcl_comm_in_file.read().split('\n')
      tcl_comm_in_file.close()
      os.remove('comm_files\\tcl_comm_in.txt')
      for line in tcl_comm_in_text:
        if line == 'cmd: stop':
          tcl_comm_out_file = open('comm_files\\tcl_comm_out.txt','a')
          tcl_comm_out_file.write('log: Test was stopped\n')
          tcl_comm_out_file.write('sync: test stopped\n')
          tcl_comm_out_file.close()
          continue_running = False
      
    time.sleep(3)
