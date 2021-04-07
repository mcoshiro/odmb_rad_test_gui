import time
import os

if  __name__ == '__main__':
  #simulate time to load firmware
  time.sleep(10)
  tcl_comm_out_file = open('comm_files\\tcl_comm_out.txt','a')
  tcl_comm_out_file.write('sync: test started\n')
  tcl_comm_out_file.write('log: Script started and firmware loaded\n')
  tcl_comm_out_file.close()

  continue_running = True
  script_paused = True
  cycle_counter = 0
  while (continue_running):
    #let GUI know script is still running every 12 seconds
    if not script_paused:
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
          tcl_comm_out_file.write('log: Script stopped\n')
          tcl_comm_out_file.write('sync: test stopped\n')
          tcl_comm_out_file.close()
          continue_running = False
          script_paused = True
        elif line == 'cmd: start':
          tcl_comm_out_file = open('comm_files\\tcl_comm_out.txt','a')
          tcl_comm_out_file.write('log: Test started\n')
          tcl_comm_out_file.write('sync: test unpaused\n')
          tcl_comm_out_file.write('sync: link 0 connected\n')
          tcl_comm_out_file.write('sync: link 1 connected\n')
          tcl_comm_out_file.write('sync: link 0 status: 4.371\n')
          tcl_comm_out_file.write('sync: link 1 status: 12.243\n')
          tcl_comm_out_file.write('sync: link 0 PLL locked\n')
          tcl_comm_out_file.write('sync: link 1 PLL locked\n')
          tcl_comm_out_file.close()
          script_paused = False
        elif line == 'cmd: pause':
          if not script_paused:
            tcl_comm_out_file = open('comm_files\\tcl_comm_out.txt','a')
            tcl_comm_out_file.write('log: Test stopped\n')
            tcl_comm_out_file.write('sync: test paused\n')
            tcl_comm_out_file.close()
        #elif line == 'cmd: reset links':
          #tcl_comm_out_file = open('comm_files\\tcl_comm_out.txt','a')
          #tcl_comm_out_file.write('log: link 0 now has 2 SEUs\n')
          #tcl_comm_out_file.write('sync: link 0 SEU count 2\n')
          #tcl_comm_out_file.write('log: link 1 now has 2 SEUs\n')
          #tcl_comm_out_file.write('sync: link 1 SEU count 2\n')
          #tcl_comm_out_file.close()
        elif line == 'cmd: inject error':
          if not script_paused:
            tcl_comm_out_file = open('comm_files\\tcl_comm_out.txt','a')
            tcl_comm_out_file.write('log: Link 0 now has 2 SEUs\n')
            tcl_comm_out_file.write('sync: link 0 SEU count 2\n')
            tcl_comm_out_file.write('log: Link 1 now has 2 SEUs\n')
            tcl_comm_out_file.write('sync: link 1 SEU count 2\n')
            tcl_comm_out_file.close()
      
    time.sleep(3)
