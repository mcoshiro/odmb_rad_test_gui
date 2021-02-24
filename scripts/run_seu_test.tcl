#tcl script for ODMB radiation test, to be controlled with Windows GUI

#run with vivado -nojournal -nolog -mode batch -notrace -source run_seu_test.tcl
#can test non-vivado commands with !/usr/bin/tclsh

#settings for Higgs
set output_file_name "comm_files/tcl_comm_out.txt"
set input_file_name "comm_files/tcl_comm_in.txt"

#settings for Higgsino
#set output_file_name "comm_files\\tcl_comm_out.txt"
#set input_file_name "comm_files\\tcl_comm_in.txt"

proc parse_report {objname propertyname index} {
  # =================================================================
  # Format of the return info consist as follows, this function returns only the value
  # ----------------------------------------------------------------
  # Property              Type    Read-only  Value
  # RX_BER                string  true       1.2844469155632799e-13
  # RX_RECEIVED_BIT_COUNT string  true       113905209031560
  # RX_PATTERN            enum    false      PRBS 7-bit
  # =================================================================
  
  set fullrep [split [report_property $objname $propertyname -return_string] "\n"]
  set valline [lindex $fullrep 1]
  set result [lindex [regexp -all -inline {\S+} $valline] $index]
  return $result 
}

proc reset_links_all {links} {
  # Reset error count
  set_property LOGIC.MGT_ERRCNT_RESET_CTRL 1 [get_hw_sio_links $links]
  commit_hw_sio [get_hw_sio_links $links]
  set_property LOGIC.MGT_ERRCNT_RESET_CTRL 0 [get_hw_sio_links $links]
  commit_hw_sio [get_hw_sio_links $links]
  
  ## Inject 1 error
  #set_property LOGIC.ERR_INJECT_CTRL 1 [get_hw_sio_links $links]
  #commit_hw_sio [get_hw_sio_links $links]
  #set_property LOGIC.ERR_INJECT_CTRL 0 [get_hw_sio_links $links]
  #commit_hw_sio [get_hw_sio_links $links]
  
  refresh_hw_sio $links
  puts "Finishing resetting all links"
}

#----------------------------------------------------------
#  main program
#----------------------------------------------------------
#open HW manager and connect to KCU105
#TODO: probably need to cofigure these properties for different machines?
open_hw
connect_hw_server -url localhost:3121 -allow_non_jtag

#uUSB cable
current_hw_target [get_hw_targets */xilinx_tcf/Digilent/210308AB0E6E]
set_property PARAM.FREQUENCY 15000000 [get_hw_targets */xilinx_tcf/Digilent/210308AB0E6E]
open_hw_target

#Red Box
#open_hw_target {localhost:3121/xilinx_tcf/Xilinx/000013ca286601}


#program board
set_property PROGRAM.FILE {ibert_fw.bit} [get_hw_devices xcku040_0]
set_property PROBES.FILE {ibert_fw.ltx} [get_hw_devices xcku040_0]
set_property FULL_PROBES.FILE {ibert_fw.ltx} [get_hw_devices xcku040_0]
current_hw_device [get_hw_devices xcku040_0]
program_hw_devices [get_hw_devices xcku040_0]
refresh_hw_device [lindex [get_hw_devices xcku040_0] 0]


#establish links
#firmware-specific: create links based on how loopbacks are connected
set all_txs [get_hw_sio_txs]
set all_rxs [get_hw_sio_rxs]

set links [list]
set seu_counters [list]
set prev_link_status [list]

set link [create_hw_sio_link [lindex $all_txs 1] [lindex $all_rxs 1]]
lappend links [lindex [get_hw_sio_links $link] 0]
lappend seu_counters 0
lappend prev_link_status "UNKNOWN"
set link [create_hw_sio_link [lindex $all_txs 4] [lindex $all_rxs 4]]
lappend links [lindex [get_hw_sio_links $link] 0]
lappend seu_counters 0
lappend prev_link_status "UNKNOWN"

after 500
reset_links_all $links
set cycle_counter 0
set comm_output_file [open $output_file_name a]
  puts $comm_output_file "log: Test started"
  puts $comm_output_file "sync: test started"
close $comm_output_file

#main loop- check link status and communicate with controlling GUI
set continue_test "1"
while { $continue_test != "0" } {
  #print 
  puts "DEBUG: SEU test in progress"

  #check status on the various links
  set nlinks [llength $links]
  refresh_hw_sio $links
  for {set ilink 0} {$ilink < $nlinks} {incr ilink} {
    set link [lindex $links $ilink]
    set seu_counter [lindex $seu_counters $ilink]
    # record bit error rate, number of bits received, prbs pattern
    set rx_ber [parse_report [get_hw_sio_links $link] "RX_BER" 3]
    set link_status [parse_report [get_hw_sio_links $link] "STATUS" 3]
    set rx_bits [parse_report [get_hw_sio_links $link] "RX_RECEIVED_BIT_COUNT" 3]
    set err_count [parse_report [get_hw_sio_links $link] "LOGIC.ERRBIT_COUNT" 3]
    #set RX_pattern [parse_report [get_hw_sio_links $link] "RX_PATTERN" 4]
    
    #write log if new SEUs
    set formatted_err_count [string trimleft $err_count 0]
    if { $formatted_err_count == "" } {
      set formatted_err_count "0"
    }
    #puts "DEBUG: previously $seu_counter errors, now $formatted_err_count"
    if { $formatted_err_count > $seu_counter } {
      #new SEU since last check
      puts "SEU detected"
      set comm_output_file [open $output_file_name a]
        puts $comm_output_file "log: link $ilink now has $formatted_err_count SEUs"
      close $comm_output_file
      lset seu_counters $ilink $formatted_err_count
    }

    #write log if link broken or recovered
    puts "DEBUG: link $ilink status: $link_status"
    if { [expr {$link_status == "NO"}] && [expr {[lindex $prev_link_status $ilink] != "NO"}] } {
      #link lost since last check
      set comm_output_file [open $output_file_name a]
        puts $comm_output_file "log: link $ilink lost"
        puts $comm_output_file "sync: link $ilink broken"
      close $comm_output_file
      lset prev_link_status $ilink "NO"
    } elseif { [expr {$link_status != "NO"}] && [expr {[lindex $prev_link_status $ilink] != "LINKED"}] } {
      #link connected since last check
      set comm_output_file [open $output_file_name a]
        puts $comm_output_file "log: link $ilink connected"
        puts $comm_output_file "sync: link $ilink connected"
      close $comm_output_file
      lset prev_link_status $ilink "LINKED"
    }

    #if link is broken, try to reset TX/RX
    if { [expr {$link_status == "NO"}] } {
      puts "DEBUG: Attemptign to reset TX/RX"
      set_property LOGIC.TX_RESET_DATAPATH 1 [get_hw_sio_links $link]
      set_property LOGIC.RX_RESET_DATAPATH 1 [get_hw_sio_links $link]
      commit_hw_sio [get_hw_sio_links $link]
      after 200
      set_property LOGIC.TX_RESET_DATAPATH 0 [get_hw_sio_links $link]
      set_property LOGIC.RX_RESET_DATAPATH 0 [get_hw_sio_links $link]
      commit_hw_sio [get_hw_sio_links $link]
    }
  #end loop over links
  }

  #check for communication from GUI
  #note: for some reason, there is a large lag between when the file is generated and when Vivado responds
  if { [file exist $input_file_name] } {
    set comm_input_file [open $input_file_name r]
    while { [gets $comm_input_file input_data] >= 0 } {
      if { $input_data == "cmd: stop" } {
        puts "Stopping test"
        set continue_test "0"
      } elseif { $input_data == "cmd: inject error" } {
        puts "Injecting error"
        set_property LOGIC.ERR_INJECT_CTRL 1 [get_hw_sio_links $links]
        commit_hw_sio [get_hw_sio_links $links]
        after 200
        set_property LOGIC.ERR_INJECT_CTRL 0 [get_hw_sio_links $links]
        commit_hw_sio [get_hw_sio_links $links]
      } elseif { $input_data == "cmd: reset links" } {
        puts "Resetting links"
        set_property LOGIC.TX_RESET_DATAPATH 1 [get_hw_sio_links $links]
        set_property LOGIC.RX_RESET_DATAPATH 1 [get_hw_sio_links $links]
        commit_hw_sio [get_hw_sio_links $links]
        after 200
        set_property LOGIC.TX_RESET_DATAPATH 0 [get_hw_sio_links $links]
        set_property LOGIC.RX_RESET_DATAPATH 0 [get_hw_sio_links $links]
        commit_hw_sio [get_hw_sio_links $links]
      } elseif { $input_data == "cmd: reset seus" } {
        puts "Resetting SEUs"
        set_property LOGIC.MGT_ERRCNT_RESET_CTRL 1 [get_hw_sio_links $links]
        commit_hw_sio [get_hw_sio_links $links]
        after 200
        set_property LOGIC.MGT_ERRCNT_RESET_CTRL 0 [get_hw_sio_links $links]
        commit_hw_sio [get_hw_sio_links $links]
        #reset script SEU counters
        set nlinks [llength $links]
        for {set ilink 0} {$ilink < $nlinks} {incr ilink} {
          lset seu_counters $ilink 0
        }
      }
    }
    close $comm_input_file
    file delete $input_file_name
  }
  
  after 3000
  #continue to next loop after 3s
  set cycle_counter [expr {[expr {$cycle_counter + 1}] % 4}]
  if {$cycle_counter == 3} {
    set comm_output_file [open $output_file_name a]
      puts $comm_output_file "sync: test in progress"
    close $comm_output_file
  }
}

set comm_output_file [open $output_file_name a]
  puts $comm_output_file "log: Test stopped"
  puts $comm_output_file "sync: test stopped"
close $comm_output_file

close_hw

#TX/RX reset commands: 
#set_property LOGIC.TX_RESET_DATAPATH 1 [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#commit_hw_sio [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#set_property LOGIC.TX_RESET_DATAPATH 0 [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#commit_hw_sio [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#set_property LOGIC.RX_RESET_DATAPATH 1 [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#commit_hw_sio [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#set_property LOGIC.RX_RESET_DATAPATH 0 [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#commit_hw_sio [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#
#reset SEU commands
#set_property LOGIC.MGT_ERRCNT_RESET_CTRL 1 [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#commit_hw_sio [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#set_property LOGIC.MGT_ERRCNT_RESET_CTRL 0 [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
#commit_hw_sio [get_hw_sio_links {localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/TX->localhost:3121/xilinx_tcf/Digilent/210308AB0E6E/0_1_0_0/IBERT/Quad_226/MGT_X0Y9/RX}]
